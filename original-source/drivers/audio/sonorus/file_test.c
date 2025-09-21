// Copyright 2001, Be Incorporated. All Rights Reserved 
// This file may be used under the terms of the Be Sample Code License 
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include "multi_audio.h"
#include <math.h>
#include <signal.h>
#include <OS.h>

#define MAX_CHANNELS 32
#define OUT_PAIR		0

char dev_name[256];
char dev_name_1[] = "/dev/audio/multi/sonorus/1";
char dev_name_2[] = "/dev/audio/multi/gina/1";
char dev_name_3[] = "/dev/audio/multi/envy24/1";



multi_description		MD;
multi_channel_info	MCI[MAX_CHANNELS];

multi_buffer_list 	MBL;
buffer_desc				play_buffer_list0[MAX_CHANNELS];
buffer_desc				play_buffer_list1[MAX_CHANNELS];
buffer_desc 			*play_buffer_desc[2] = { play_buffer_list0, 
															 play_buffer_list1 };
buffer_desc				record_buffer_list0[MAX_CHANNELS];
buffer_desc				record_buffer_list1[MAX_CHANNELS];
buffer_desc 			*record_buffer_desc[2] = { record_buffer_list0, 
															  	record_buffer_list1 };
multi_buffer_info		MBI;
multi_channel_enable MCE;
uint32					mce_enable_bits;

multi_format_info 		MFI;

multi_mix_channel_info  MMCI;
multi_mix_control_info  MMCIx;

int						done = 0;
char * cache_buffer;
char * cache_buffer_rec;
int cache_pagesize;
int cache_pgsize;
sem_id data_sem;
sem_id file_sem;
sem_id data_rec_sem;
sem_id file_rec_sem;

static status_t loader(void * handle)
{
	int fd = (int)handle;
	int page = 0;
	while (1) {
		int rd;
		acquire_sem_etc(file_sem, cache_pagesize, 0, 0);
		rd = read(fd, cache_buffer+page*cache_pagesize, cache_pagesize);
		if (rd < cache_pagesize) {
			lseek(fd, 0, 0);
			read(fd, cache_buffer+page*cache_pagesize+rd, cache_pagesize-rd);
		}
		page = (page + 1) % 2;
		release_sem_etc(data_sem, cache_pagesize, 0);
	}
}

static status_t dumper(void * handle)
{
	int fd = (int)handle;
	int page = 0;
	while (1) {
		acquire_sem_etc(file_rec_sem, cache_pgsize, 0, 0);
		write(fd, cache_buffer_rec+page*cache_pgsize, cache_pgsize);
		page = (page + 1) % 2;
		release_sem_etc(data_rec_sem, cache_pgsize, 0);
	}
}


static void init()
{
	cache_pagesize = 256*1024L;
	cache_pgsize   = 10*4*2*4*1024L;
	cache_buffer 	 = malloc(cache_pagesize*2);
	cache_buffer_rec = malloc(cache_pgsize*2);
	memset(cache_buffer, 0, cache_pagesize*2);
	memset(cache_buffer_rec, 0, cache_pgsize*2);
	data_sem = create_sem(0, "data_sem");
	file_sem = create_sem(cache_pagesize*2, "file_sem");
	data_rec_sem = create_sem(cache_pgsize*2, "data_sem_rec");
	file_rec_sem = create_sem(0, "file_sem_rec");

}

static int copy(void * dest, int size)
{
	static char * data;
	static char * end;
	if (!data) {
		data = cache_buffer;
		end = cache_buffer+2*cache_pagesize;
	}
	if (acquire_sem_etc(data_sem, size, B_TIMEOUT, 10000L) < 0) {
		return 0;
	}
	if (size+data > end) {
		int rest;
		memcpy(dest, data, end-data);
		rest = size-(end-data);
		memcpy(((char *)dest)+(end-data), cache_buffer, rest);
		data = rest+cache_buffer;
	}
	else {
		memcpy(dest, data, size);
		data += size;
	}
	release_sem_etc(file_sem, size, 0);
	return size;
}
	
static int copy_rec(void * src, int size)
{
	static char * data;
	static char * end;
	if (!data) {
		data = cache_buffer_rec;
		end = cache_buffer_rec+2*cache_pgsize;
	}
	if (acquire_sem_etc(data_rec_sem, size, B_TIMEOUT, 10000L) < 0) {
		return 0;
	}
	if (size+data > end) {
		int rest;
		memcpy(data, src, end-data);
		rest = size-(end-data);
		memcpy(cache_buffer_rec, ((char *)src)+(end-data), rest);
		data = rest+cache_buffer_rec;
	}
	else {
		memcpy(data, src, size);
		data += size;
	}
	release_sem_etc(file_rec_sem, size, 0);
	return size;
}
	

void clear_half_buffer(int32 *buff1,int32 *buff2,uint frames)
{
	int i;
	
	for (i = 0; i<frames; i++)
	{
		buff1[i] = 0;
		buff2[i] = 0;
	}

}

int main(int nargs,char *args[])
{
	int 			driver_handle,playfile_handle,capture_handle, half_buffer_size;
	int32			*pPlay[2][MAX_CHANNELS];
	int32			*pRecord[2][MAX_CHANNELS];	
	int 			rval,i,j,buffset,num_outputs,num_inputs,num_channels;
	int 			is_10channel = 1, record = 0, is_gina = 0, is_envy = 0, reverse = 0;
	uint32 			cntr = 0,seek=0;

	if (1 == nargs)
	{
		printf("Usage: file_test file_name [-2] [-r] [-s nnnn]\n");
		printf("       file_name is a 10 channel 24 bit left aligned file\n");
		printf("       file is assumed 10 channel 32 bit unless -2 option set\n");
		printf("       -2 indicates the file is 2 channel 16 bit\n");
		printf("       -r indicates play and record file\n");
		printf("       -s to seek to offset nnnn in file\n");
		printf("       -x to reverse channels 9/10 with 2/3 (can't use -2) \n");
		printf("       -g if using Gina\n");
		printf("       -e if using Envy24\n");
		goto exit;
	}

	//
	// "process" cl vars
	//
	i = nargs;
	while(i--) {
		if (*args[i] == '-') {
			switch (args[i][1]) {
				case '2':
					is_10channel = 0;
					break;
				case 'e':
				case 'E':
					is_envy = 1;
					break;
				case 'g':
				case 'G':
					is_gina = 1;
					break;
				case 'r':
				case 'R':
					record = 1;
					break;
				case 's':
				case 'S':
					seek = atol(args[i+1]);
					break;
				case 'x':
				case 'X':
					reverse = 1;
					break;
				default:
				 	break;					
			}
		}
	}

	//
	// Open the driver
	//
	if(is_gina)
	{
		strcpy(dev_name, dev_name_2);
	}
	else if(is_envy)
	{
		strcpy(dev_name, dev_name_3);
	}
	else
	{	
		strcpy(dev_name, dev_name_1);
	}
	driver_handle = open(dev_name,O_RDWR);
	if (-1 == driver_handle)
	{
		printf("Failed to open %s\n",dev_name);
		return -1;
	}
	printf("\nOpened %s\n",dev_name);
	
	playfile_handle = open(args[1],O_RDWR);
	if (-1 == playfile_handle)
	{
		printf("Failed to open %s\n",args[1]);
		return -1;
	}
	printf("\nOpened %s\n",args[1]);

	init();
	lseek(playfile_handle, seek, SEEK_SET);
	resume_thread(spawn_thread(loader, "loader", 100, (void *)playfile_handle));

	if (record) {	
		capture_handle = open("/Media/Media/tmp.rec",O_RDWR | O_CREAT | O_TRUNC);
		if (-1 == capture_handle)
		{
			printf("Failed to open %s\n","/Media/Media/tmp.rec");
			return -1;
		}
		resume_thread(spawn_thread(dumper, "dumper", 100, (void *)capture_handle));
		printf("\nOpened %s\n","/tmp/tmp.rec");
	}

	//
	// Get description
	//
	MD.info_size = sizeof(MD);
	MD.request_channel_count = MAX_CHANNELS;
	MD.channels = MCI;
	rval = ioctl(driver_handle,B_MULTI_GET_DESCRIPTION,&MD,0);
	if (B_OK != rval)
	{
		printf("Failed on B_MULTI_GET_DESCRIPTION\n");
		goto exit;
	}

	printf("Friendly name:\t%s\nVendor:\t\t%s\n",
				MD.friendly_name,MD.vendor_info);
	printf("%ld outputs\t%ld inputs\n%ld out busses\t%ld in busses\n",
				MD.output_channel_count,MD.input_channel_count,
				MD.output_bus_channel_count,MD.input_bus_channel_count);
	printf("\nChannels\n"
			 "ID\tKind\tDesig\tConnectors\n");

	for (i = 0 ; i < (MD.output_channel_count + MD.input_channel_count); i++)
	{
		printf("%ld\t%d\t%lu\t0x%lx\n",MD.channels[i].channel_id,
											MD.channels[i].kind,
											MD.channels[i].designations,
											MD.channels[i].connectors);
	}			 
	printf("\n");
	
	printf("Output rates\t\t0x%lx\n",MD.output_rates);
	printf("Input rates\t\t0x%lx\n",MD.input_rates);
	printf("Max CVSR\t\t%.0f\n",MD.max_cvsr_rate);
	printf("Min CVSR\t\t%.0f\n",MD.min_cvsr_rate);
	printf("Output formats\t\t0x%lx\n",MD.output_formats);
	printf("Input formats\t\t0x%lx\n",MD.input_formats);
	printf("Lock sources\t\t0x%lx\n",MD.lock_sources);
	printf("Timecode sources\t0x%lx\n",MD.timecode_sources);
	printf("Interface flags\t\t0x%lx\n",MD.interface_flags);
	printf("Control panel string:\t\t%s\n",MD.control_panel);
	printf("\n");
	
	num_outputs = MD.output_channel_count;
	num_inputs = MD.input_channel_count;
	num_channels = num_outputs + num_inputs;
	
	// Get and set enabled buffers
	MCE.info_size = sizeof(MCE);
	MCE.enable_bits = (uchar *) &mce_enable_bits;
	rval = ioctl(driver_handle,B_MULTI_GET_ENABLED_CHANNELS,&MCE,sizeof(MCE));
	if (B_OK != rval)
	{
		printf("Failed on B_MULTI_GET_ENABLED_CHANNELS len is 0x%x\n",sizeof(MCE));
		goto exit;
	}
	
	mce_enable_bits = (1 << num_channels) - 1;
	MCE.lock_source = B_MULTI_LOCK_INTERNAL;
	rval = ioctl(driver_handle,B_MULTI_SET_ENABLED_CHANNELS,&MCE,0);
	if (B_OK != rval)
	{
		printf("Failed on B_MULTI_SET_ENABLED_CHANNELS 0x%x 0x%x\n", MCE.enable_bits, *(MCE.enable_bits));
		goto exit;
	}
	
	//
	// Set the sample rate
	//
	MFI.info_size = sizeof(MFI);
	MFI.output.rate = B_SR_44100;
	MFI.output.cvsr = 0;
	MFI.output.format = B_FMT_24BIT;
	MFI.input.rate = MFI.output.rate;
	MFI.input.cvsr = MFI.output.cvsr;
	MFI.input.format = MFI.output.format;
	rval = ioctl(driver_handle,B_MULTI_SET_GLOBAL_FORMAT,&MFI,0);
	if (B_OK != rval)
	{
		printf("Failed on B_MULTI_SET_GLOBAL_FORMAT\n");
		goto exit;
	}
	
	//
	// Do the get buffer ioctl
	//
	MBL.info_size = sizeof(MBL);
	MBL.request_playback_buffer_size = 0;           // 0 will use the default......
	MBL.request_playback_buffers = 2;
	MBL.request_playback_channels = num_outputs;
	MBL.playback_buffers = (buffer_desc **) play_buffer_desc;	
	MBL.request_record_buffer_size = 0;           // 0 will use the default......
	MBL.request_record_buffers = 2;
	MBL.request_record_channels = num_inputs;
	MBL.record_buffers = (buffer_desc **) record_buffer_desc;		
	rval = ioctl(driver_handle, B_MULTI_GET_BUFFERS, &MBL, 0);

	if (B_OK != rval)
	{
		printf("Failed on B_MULTI_GET_BUFFERS\n");
		goto exit;
	}
	
	for (i = 0; i<2; i++)
		for (j=0; j < num_outputs; j++)
		{
			pPlay[i][j] = (int32 *) MBL.playback_buffers[i][j].base;
			memset(pPlay[i][j],0,MBL.return_playback_buffer_size * 4 /*samps to bytes*/);
			printf("Play buffers[%d][%d]: %p\n",i,j,pPlay[i][j]);
		}
	for (i = 0; i<2; i++)
		for (j=0; j < num_inputs; j++)
		{
			pRecord[i][j] = (int32 *) MBL.record_buffers[i][j].base;
			printf("Record buffers[%d][%d]: %p\n",i,j,pRecord[i][j]);			
		}
	half_buffer_size = MBL.return_playback_buffer_size ;
	printf("\n");
	printf("half buffer size is 0x%x (%d)samples\n", half_buffer_size, half_buffer_size);
	MBI.info_size = sizeof(MBI);
 	ioctl(driver_handle, B_MULTI_BUFFER_EXCHANGE, &MBI, 0);		
	ioctl(driver_handle, B_MULTI_BUFFER_EXCHANGE, &MBI, 0);	
	
	//
	// Do it. 
	//
	// REV. 2: don't calculate the stride every time, save it off...
	// REV. 2: add sig handler or clean up quitting....
	printf("Now looping (ctl c to quit))\n");
	set_thread_priority(find_thread(NULL), B_REAL_TIME_PRIORITY);
	while (1)
	{
		static int32 buff[1820*10];
		static int32 buffy[1820*10];
		int32 * buffer, * buffery;
		short * buffer_16, * buffery_16;
		int32 buflen;
		if (is_10channel) {
			buflen = half_buffer_size * 10 * 4;
		}
		else {
			buflen = half_buffer_size * 2 * 2;
		}
		while(copy(&buff[0], buflen) != 0) {
			buffer = &buff[0];
			buffer_16 = (short *) &buff[0];
			buffery = &buffy[0];
			buffery_16 = (short *) &buffy[0];
			if (is_10channel) {	
				for ( i = 0; i < half_buffer_size; i ++ ) {
					pPlay[cntr&1][0][i*MBL.playback_buffers[cntr&1][0].stride/4] = *buffer++;
					pPlay[cntr&1][1][i*MBL.playback_buffers[cntr&1][1].stride/4] = *buffer++;
					if (reverse) {
						pPlay[cntr&1][8][i*MBL.playback_buffers[cntr&1][8].stride/4] = *buffer++;
						pPlay[cntr&1][9][i*MBL.playback_buffers[cntr&1][9].stride/4] = *buffer++;
					}
					else {	
						pPlay[cntr&1][2][i*MBL.playback_buffers[cntr&1][2].stride/4] = *buffer++;
						pPlay[cntr&1][3][i*MBL.playback_buffers[cntr&1][3].stride/4] = *buffer++; 
					}
					pPlay[cntr&1][4][i*MBL.playback_buffers[cntr&1][4].stride/4] = *buffer++;
					pPlay[cntr&1][5][i*MBL.playback_buffers[cntr&1][5].stride/4] = *buffer++;
					pPlay[cntr&1][6][i*MBL.playback_buffers[cntr&1][6].stride/4] = *buffer++;
					pPlay[cntr&1][7][i*MBL.playback_buffers[cntr&1][7].stride/4] = *buffer++;
					if (reverse) {
						pPlay[cntr&1][2][i*MBL.playback_buffers[cntr&1][2].stride/4] = *buffer++;
						pPlay[cntr&1][3][i*MBL.playback_buffers[cntr&1][3].stride/4] = *buffer++;
					}
					else {	
						pPlay[cntr&1][8][i*MBL.playback_buffers[cntr&1][8].stride/4] = *buffer++;
						pPlay[cntr&1][9][i*MBL.playback_buffers[cntr&1][9].stride/4] = *buffer++; 
					}
					*buffery++ = pRecord[cntr&1][0][i*MBL.record_buffers[cntr&1][0].stride/4];
					*buffery++ = pRecord[cntr&1][1][i*MBL.record_buffers[cntr&1][1].stride/4];
					*buffery++ = pRecord[cntr&1][2][i*MBL.record_buffers[cntr&1][2].stride/4];
					*buffery++ = pRecord[cntr&1][3][i*MBL.record_buffers[cntr&1][3].stride/4];
					if(!is_gina) {
						*buffery++ = pRecord[cntr&1][4][i*MBL.record_buffers[cntr&1][4].stride/4];
						*buffery++ = pRecord[cntr&1][5][i*MBL.record_buffers[cntr&1][5].stride/4];
						*buffery++ = pRecord[cntr&1][6][i*MBL.record_buffers[cntr&1][6].stride/4];
						*buffery++ = pRecord[cntr&1][7][i*MBL.record_buffers[cntr&1][7].stride/4];
						*buffery++ = pRecord[cntr&1][8][i*MBL.record_buffers[cntr&1][8].stride/4];
						*buffery++ = pRecord[cntr&1][9][i*MBL.record_buffers[cntr&1][9].stride/4];
					}
					else {
						buffery++;
						buffery++;
						buffery++;
						buffery++;
						buffery++;
						buffery++;
					}
				}
				if (record) {
					copy_rec(&buffy[0], buflen);
				}
#if 0				
				printf("pl x%08x x%08x x%08x x%08x x%08x x%08x\n", pPlay[cntr&1][0][0],
						 pPlay[cntr&1][0][1], pPlay[cntr&1][0][2], pPlay[cntr&1][0][3],
						 pPlay[cntr&1][0][4], pPlay[cntr&1][0][5]);
				printf("cp x%08x x%08x x%08x x%08x x%08x x%08x\n", pRecord[cntr&1][0][0],
						pRecord[cntr&1][0][1],pRecord[cntr&1][0][2],pRecord[cntr&1][0][3],
						pRecord[cntr&1][0][4],pRecord[cntr&1][0][5]);		
#endif
			}
			
			else {
				for ( i = 0; i < half_buffer_size; i ++ ) {
					pPlay[cntr&1][0][i*MBL.playback_buffers[cntr&1][0].stride/4] = (((long)(*buffer_16++))&0x0000ffff) << 16;
					pPlay[cntr&1][1][i*MBL.playback_buffers[cntr&1][1].stride/4] = (((long)(*buffer_16++))&0x0000ffff) << 16;
					pPlay[cntr&1][2][i*MBL.playback_buffers[cntr&1][2].stride/4] = pPlay[cntr&1][4][i*MBL.playback_buffers[cntr&1][4].stride/4] = pPlay[cntr&1][0][i*MBL.playback_buffers[cntr&1][0].stride/4];
					pPlay[cntr&1][6][i*MBL.playback_buffers[cntr&1][6].stride/4] = pPlay[cntr&1][8][i*MBL.playback_buffers[cntr&1][8].stride/4] = pPlay[cntr&1][0][i*MBL.playback_buffers[cntr&1][0].stride/4];
					pPlay[cntr&1][3][i*MBL.playback_buffers[cntr&1][3].stride/4] = pPlay[cntr&1][5][i*MBL.playback_buffers[cntr&1][5].stride/4] = pPlay[cntr&1][1][i*MBL.playback_buffers[cntr&1][0].stride/4];
					pPlay[cntr&1][7][i*MBL.playback_buffers[cntr&1][7].stride/4] = pPlay[cntr&1][9][i*MBL.playback_buffers[cntr&1][9].stride/4] = pPlay[cntr&1][1][i*MBL.playback_buffers[cntr&1][0].stride/4];
					*buffery++ = pRecord[cntr&1][0][i*MBL.record_buffers[cntr&1][0].stride/4] ;
					*buffery++ = pRecord[cntr&1][1][i*MBL.record_buffers[cntr&1][1].stride/4] ;
					*buffery++ = pRecord[cntr&1][2][i*MBL.record_buffers[cntr&1][2].stride/4] ;
					*buffery++ = pRecord[cntr&1][3][i*MBL.record_buffers[cntr&1][3].stride/4] ;

					if(!is_gina) {
						*buffery++ = pRecord[cntr&1][4][i*MBL.record_buffers[cntr&1][4].stride/4] ;
						*buffery++ = pRecord[cntr&1][5][i*MBL.record_buffers[cntr&1][5].stride/4] ;
						*buffery++ = pRecord[cntr&1][6][i*MBL.record_buffers[cntr&1][6].stride/4] ;
						*buffery++ = pRecord[cntr&1][7][i*MBL.record_buffers[cntr&1][7].stride/4] ;
						*buffery++ = pRecord[cntr&1][8][i*MBL.record_buffers[cntr&1][8].stride/4] ;
						*buffery++ = pRecord[cntr&1][9][i*MBL.record_buffers[cntr&1][9].stride/4] ;
					}
					else {
						buffery++;
						buffery++;
						buffery++;
						buffery++;
						buffery++;
						buffery++;
					}
				}			

				if (record) {
					copy_rec(&buffy[0], buflen * 10);
				}
#if 0				
				printf("pl x%08x x%08x x%08x x%08x x%08x x%08x\n", pPlay[cntr&1][0][0],
						 pPlay[cntr&1][0][1], pPlay[cntr&1][0][2], pPlay[cntr&1][0][3],
						 pPlay[cntr&1][0][4], pPlay[cntr&1][0][5]);
				printf("cp x%08x x%08x x%08x x%08x x%08x x%08x\n", pRecord[cntr&1][0][0],
						pRecord[cntr&1][0][1],pRecord[cntr&1][0][2],pRecord[cntr&1][0][3],
						pRecord[cntr&1][0][4],pRecord[cntr&1][0][5]);		
#endif
			}	
			ioctl(driver_handle, B_MULTI_BUFFER_EXCHANGE, &MBI, 0);
			cntr++;
//			printf("%d\n",cntr);
		}	
	}

exit:	
	close(driver_handle);
	close(playfile_handle);
	if (record) {	
		close(capture_handle);
	}
	return 0;
}


