// Copyright 2001, Be Incorporated. All Rights Reserved 
// This file may be used under the terms of the Be Sample Code License 
#include <stdio.h>
#include <OS.h>
#include "multi_audio.h"


char dev_name[] = "/dev/audio/multi/sonorus/1";
char dev_name_1[] = "/dev/audio/multi/gina/1";
char dev_name_2[] = "/dev/audio/multi/envy24/1";

multi_description		MD;
multi_channel_info *	MCI;

multi_buffer_list 		MBL;
multi_buffer_info		MBI;
multi_channel_enable 	MCE;
uint32					mce_enable_bits;

multi_format_info 		MFI;

multi_mix_channel_info  MMCI;
multi_mix_control_info  MMCIx;

multi_mix_value_info	MMVI;


int main(int nargs,char *args[])
{
	int 			driver_handle;
	int				rval, i, loop = 1;
	char			device[256];
	//
	// Open the driver
	//
	if (nargs < 2)
		strcpy(device,dev_name);
	else
		strcpy(device,dev_name_2);
				
	driver_handle = open(device,3);
	if (-1 == driver_handle)
	{
		printf("Failed to open %s\n",device);
		return -1;
	}
	printf("\nOpened %s\n",device);
	
while(loop++) {
	int32 x,y;
	printf("----------------0 Exit-----------------------%d\n",loop);
	printf("1  Get Description      \t10 Get Mix\n");
	printf("2  Set Enabled Channels \t11 Set Mix\n");
	printf("3  Get Enabled Channels \t12 List Mix Channels\n");
	printf("4  Get Global Format    \t13 List Mix Controls\n");
	printf("5  Set Global Format    \t14 List Mix Connections\n");
	printf("6  Get Buffers          \t15 List Modes\n");
	printf("7  Set Buffers          \t16 Get Mode\n");
	printf("8  Force Stop           \t17 Set Mode\n");
	printf("20 List Mode Extensions \t30 Start At [10 S from now]\n");
	printf("21 Get Mode Extension   \t31 Get Event Info\n");
	printf("22 Set Mode Extension   \t32 Get Event\n");
	printf("23 xxxxxxxxxxxxxxxxxx   \t33 Set Event Info\n");
	printf("----------------90 Test Du Jour---------------\n");
	if (!scanf("%d",&x) || 0 == x) {
		break;
	}
	printf("\n");
	switch(x) {
	case 1:
		{
		//
		// Get description
		//
		int32 max_channels;
		MD.info_size = sizeof(MD);	
		MD.request_channel_count = 0;
		MD.channels = NULL;
		rval = ioctl(driver_handle,B_MULTI_GET_DESCRIPTION,&MD,0);
		if (B_OK != rval)
		{
			printf("Failed on B_MULTI_GET_DESCRIPTION\n");
			break;
		}
		max_channels = 	MD.output_channel_count + MD.input_channel_count + MD.output_bus_channel_count +
							MD.input_bus_channel_count + MD.aux_bus_channel_count;

		MD.request_channel_count = max_channels;
		MCI = (multi_channel_info *) malloc(sizeof(multi_channel_info) * max_channels);
		if (MCI == NULL)
		{
			break;
		}	
		MD.channels = MCI;
		rval = ioctl(driver_handle,B_MULTI_GET_DESCRIPTION,&MD,0);
		if (B_OK != rval)
		{
			printf("Failed on B_MULTI_GET_DESCRIPTION\n");
			break;
		}
	
		printf("Friendly name:\t%s\nVendor:\t\t%s\n",
					MD.friendly_name,MD.vendor_info);
		printf("%ld outputs\t%ld inputs\n%ld out busses\t%ld in busses\n",
					MD.output_channel_count,MD.input_channel_count,
					MD.output_bus_channel_count,MD.input_bus_channel_count);
		printf("\nChannels\n"
				 "ID\tKind\tDesig\tConnectors\n");
	
		for (i = 0 ; i < (MD.output_channel_count + MD.input_channel_count + MD.output_bus_channel_count + MD.input_bus_channel_count + MD.aux_bus_channel_count); i++)
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
		
		free(MCI);
		}
		break;	

	case  2:
		//
		// Set enabled buffers
		//
		printf("Enter channels to set as hex number\n");
		if (!scanf("%x",&y)) {
			break;
		}

		mce_enable_bits = y;
		MCE.info_size = sizeof(multi_channel_enable);
		MCE.lock_source = B_MULTI_LOCK_INTERNAL;
		MCE.enable_bits = (uchar *) &mce_enable_bits;
		rval = ioctl(driver_handle,B_MULTI_SET_ENABLED_CHANNELS,&MCE,0);
		if (B_OK != rval)
		{
			printf("Failed on B_MULTI_SET_ENABLED_CHANNELS 0x%x 0x%x\n", MCE.enable_bits, *(MCE.enable_bits));
			printf("You probably didn't open with READ/WRITE permission\n\n");
		}
		break;

	case  3:
		//
		// Get enabled buffers
		//
		MCE.info_size = sizeof(MCE);
		MCE.enable_bits = (uchar *) &mce_enable_bits;
		rval = ioctl(driver_handle,B_MULTI_GET_ENABLED_CHANNELS,&MCE,sizeof(MCE));
		if (B_OK != rval)
		{
			printf("Failed on B_MULTI_GET_ENABLED_CHANNELS \n\n",sizeof(MCE));
			break;
		}
		printf("Enabled channels are: 0x%x\n\n", mce_enable_bits);
		break;

	case  4:
		//
		// Get the sample rate
		//
		MFI.info_size = sizeof(MFI);
		rval = ioctl(driver_handle,B_MULTI_GET_GLOBAL_FORMAT,&MFI,0);
		if (B_OK != rval)
		{
			printf("Failed on B_MULTI_GET_GLOBAL_FORMAT\n");
		}
		else {
			printf("Output rate 0x%x is 44.1 0x%x is 48\n",B_SR_44100,B_SR_48000);
			printf("24 bit format is 0x%x\n",B_FMT_24BIT);
			printf("output cvsr = 0x%x\n", MFI.output.cvsr);
			printf("output rate = 0x%x\n", MFI.output.rate);
			printf("output format 0x%x\n", MFI.output.format);
			printf("input cvsr  = 0x%x\n", MFI.input.cvsr);
			printf("input rate  = 0x%x\n", MFI.input.rate);
			printf("input format  0x%x\n", MFI.input.format);
		}
		break;

	case  5:
		//
		// Set the sample rate
		//
		{
			int32 iy;
			printf("Enter 0 for 44100, 1 for 48000\n");
			if (!scanf("%d",&iy)) {
				break;
			}
			if (iy)	{
				MFI.output.rate = B_SR_48000;
				printf("Setting SR to 48000, 24 bit\n");
			}
			else {	
				MFI.output.rate = B_SR_44100;
				printf("Setting SR to 44100, 24 bit\n");
			}
		}
		MFI.info_size = sizeof(MFI);
		MFI.output.cvsr = 0;
		MFI.output.format = B_FMT_24BIT;
		MFI.input.rate = MFI.output.rate;
		MFI.input.cvsr = MFI.output.cvsr;
		MFI.input.format = MFI.output.format;
		rval = ioctl(driver_handle,B_MULTI_SET_GLOBAL_FORMAT,&MFI,0);
		if (B_OK != rval)
		{
			printf("Failed on B_MULTI_SET_GLOBAL_FORMAT\n");
			printf("You probably didn't open with READ/WRITE permission\n\n");
		}
		break;

	case 8:
		//
		// Force Stop
		//
		rval = ioctl(driver_handle,B_MULTI_BUFFER_FORCE_STOP, NULL,0);
		if (B_OK != rval)
		{
			printf("Failed on B_MULTI_FORCE_STOP\n");
			printf("You probably didn't open with READ/WRITE permission\n\n");
		}
		break;

	case 11:
		//
		// Set mix
		//
		{ /* set_mix */
			float f; int32 ix,iy; bool b;
			char c;
			multi_mix_value * pmmv;
			MMVI.info_size = sizeof(MMVI);
			MMVI.item_count= 1;
			pmmv = (multi_mix_value *)malloc(MMVI.item_count * sizeof(multi_mix_value));
			if (pmmv == NULL) {
				printf("malloc failed\n");
				break;
			}
			MMVI.values = pmmv;

			printf("Enter control id\n");
			if (!scanf("%d",&iy)) {
				free(pmmv);
				break;
			}
			pmmv->id = iy;
			printf("%d\n",iy);
			{			
				multi_mix_control mmc;
				MMCIx.info_size = sizeof(multi_mix_control_info);
				MMCIx.control_count = 1;
				MMCIx.controls = &mmc;
				mmc.id = iy;
				rval = ioctl(driver_handle,B_MULTI_LIST_MIX_CONTROLS,&MMCIx ,0);
				if (B_OK != rval)
				{
					printf("Failed on B_MULTI_LIST_MIX_CONTROLS \n");
				}
				if ( mmc.flags & B_MULTI_MIX_GAIN ) {
					printf("Enter float value for control %d\n", iy);
					if (!scanf("%f",&f)) {
						free(pmmv);
						break;
					}	
					pmmv->u.gain = f;
				}
				else {
					printf("Enter int/bool value for control %d\n", y);
					if (!scanf("%d",&ix)) {
						free(pmmv);
						break;
					}	
					pmmv->u.enable = (bool)ix;
					pmmv->u.mux    = ix;
				}			
			}	
				
			rval = ioctl(driver_handle,B_MULTI_SET_MIX, &MMVI ,0);
			if (B_OK != rval)
			{
				printf("Failed on B_MULTI_SET_MIX\n");
			}
			free(pmmv);	
		} /* set_mix */
		break;

	case 10:
	case 12:
	case 13:
		//
		// Mix stuff
		//
		{
			int32 channel = 0;					
			int32 * ctls, * freeme;
			int32 ** ctlsx;
			int32 max_channels;
			
			MD.info_size = sizeof(MD);
			MD.request_channel_count = 0;
			MD.channels = NULL;
			rval = ioctl(driver_handle,B_MULTI_GET_DESCRIPTION,&MD,0);
			if (B_OK != rval)
			{
				printf("Failed on B_MULTI_GET_DESCRIPTION\n");
				break;
			}
			max_channels = 	MD.output_channel_count + MD.input_channel_count + MD.output_bus_channel_count +
							MD.input_bus_channel_count + MD.aux_bus_channel_count;
			
			for ( i=0; i<max_channels; i++)
			{
				MMCI.info_size = sizeof(multi_mix_channel_info);
				MMCI.channel_count = 1;
				channel = i; 
				MMCI.channels = &channel;
				MMCI.max_count = 0;
				MMCI.controls = NULL;					
				rval = ioctl(driver_handle,B_MULTI_LIST_MIX_CHANNELS,&MMCI ,0);
				if (B_OK != rval)
				{
					printf("Failed on B_MULTI_LIST_MIX_CHANNELS \n");
					break; /* for loop */
				}
				MMCI.max_count = MMCI.actual_count;
				freeme = ctls = (int32 *)malloc(sizeof(int32) * MMCI.actual_count);
//				printf("actual count is %ld, ctls is %p\n", MMCI.actual_count,ctls);
				if (ctls == NULL)
				{
					break; /* for loop */
				}
				ctlsx = &ctls;
				MMCI.controls = ctlsx;

				rval = ioctl(driver_handle,B_MULTI_LIST_MIX_CHANNELS,&MMCI ,0);
				if (B_OK != rval)
				{
					printf("Failed on B_MULTI_LIST_MIX_CHANNELS \n");
					free(freeme);
					break; /* for loop */
				}
				else				
				{
					int32 tmp;
					if (x == 12)
					{
						printf("ch:%d ",i);
					}
					for (tmp = 0; tmp < MMCI.actual_count; tmp++)
					{
						if (x == 12)
						{
							printf("ctl:%d ",*ctls);
						}
						else if (x == 13)
						{
							multi_mix_control mmc;
							MMCIx.info_size = sizeof(multi_mix_control_info);
							MMCIx.control_count = 1;
							MMCIx.controls = &mmc;
							mmc.id = *ctls;
							rval = ioctl(driver_handle,B_MULTI_LIST_MIX_CONTROLS,&MMCIx ,0);
							printf("ctl:%d flags:%x master:%d name:%s \n",mmc.id, mmc.flags, mmc.master, mmc.name);
							if (mmc.flags & B_MULTI_MIX_GAIN)
							{
								printf("\tMin:%f  Max:%f  Granularity:%f\n",mmc.u.gain.min_gain,
																			mmc.u.gain.max_gain, mmc.u.gain.granularity);
							}
							if (B_OK != rval)
							{
								printf("Failed on B_MULTI_LIST_MIX_CONTROLS \n");
							}
						}
						else
						{
							multi_mix_value  mmv;
							MMVI.info_size = sizeof(MMVI);
							MMVI.item_count= 1;
							MMVI.values = &mmv;
							mmv.id = *ctls;
							rval = ioctl(driver_handle,B_MULTI_GET_MIX, &MMVI ,0);
							if (B_OK != rval)
							{
								printf("Failed on B_MULTI_GET_MIX 2nd time\n");
							}
							else
							{
								printf("%ld",mmv.id);
								printf("\t%f",mmv.u.gain);
								printf("\t\t%d",mmv.u.enable);
								printf("\t\t\t%x",mmv.u.mux);
								printf("\t\t\t\t%ld",mmv.ramp);
								printf("\n");
							}	
						}	
						ctls++;	
					}
//					printf("freeing %p\n",freeme);
					free(freeme);
					ctls = NULL;
					if (x == 12)
					{
						printf("\n");
					}	
				}
			}
		}
		break;

	case 14:
		//
		// List mix connections
		//
		{
			multi_mix_connection * mmc;
			multi_mix_connection_info mmci;
			mmci.info_size = sizeof(multi_mix_connection_info);
			mmci.max_count = 0;
			rval = ioctl(driver_handle,B_MULTI_LIST_MIX_CONNECTIONS,&mmci ,0);
			if (B_OK != rval)
			{
				printf("Failed on B_MULTI_LIST_MIX_CONNECTIONS (1st time)\n");
			}
			mmci.info_size   = sizeof(multi_mix_connection_info);
			mmci.max_count   = mmci.actual_count;
			mmc = malloc(sizeof(multi_mix_connection) * mmci.max_count);
			if (mmc == NULL) {
				break;
			}	
			mmci.connections = mmc;
			rval = ioctl(driver_handle,B_MULTI_LIST_MIX_CONNECTIONS,&mmci ,0);
			for( i=0; i<mmci.max_count; i++) {
				printf("from:%d to:%d \n",mmc[i].from, mmc[i].to);
			}	
			free(mmc);
			if (B_OK != rval)
			{
				printf("Failed on B_MULTI_LIST_MIX_CONNECTIONS (2nd time)\n");
			}
		}
		break;

	case 20:
		//
		// List mode extensions
		//
		{
			multi_extension_list mel;
			multi_extension_info * mui;
			mel.info_size = sizeof(multi_extension_list);
			mel.max_count = 0;
			rval = ioctl(driver_handle, B_MULTI_LIST_EXTENSIONS, &mel, 0);
			if (B_OK != rval)
			{
				printf("Failed on B_MULTI_LIST_EXTENSIONS (1st time)\n");
			}
			mel.info_size   = sizeof(multi_extension_info);
			mel.max_count   = mel.actual_count;
			mui = malloc(sizeof(multi_extension_info) * mel.max_count);
			if (mui == NULL) {
				break;
			}	
			mel.extensions = mui;
			rval = ioctl(driver_handle,B_MULTI_LIST_EXTENSIONS, &mel, 0);
			printf("Number of extensions is %d \n", mel.actual_count);
			for( i=0; i<mel.actual_count; i++) {
				printf("Code %d Flags %x Name %s \n",mui[i].code, mui[i].flags, mui[i].name);
			}	
			free(mui);
			if (B_OK != rval)
			{
				printf("Failed on B_MULTI_LIST_EXTENSIONS (2nd time)\n");
			}
		}
		break;
	case 30:
		//
		// Start at
		//
		{
			bigtime_t the_time = system_time() + 10000000;
			rval = ioctl(driver_handle,B_MULTI_SET_START_TIME, &the_time,0);
			if (B_OK != rval)
			{
				printf("Failed on B_MULTI_SET_START_TIME\n");
				printf("You probably didn't open with READ/WRITE permission\n\n");
			}
		}
		break;
	case 31:
		//
		// Get event info
		//
		{
			multi_get_event_info mgei;
			mgei.info_size = sizeof(mgei);
			rval = ioctl(driver_handle,B_MULTI_GET_EVENT_INFO, &mgei,0);
			if (B_OK != rval)
			{
				printf("Failed on B_MULTI_GET_EVENT_INFO\n");
			}
			else
			{
				printf("\tsupported_mask\t0x%x\n",mgei.supported_mask);
				printf("\tcurrent_mask\t0x%x\n",mgei.current_mask);
				printf("\tqueue_size\t%d\n",mgei.queue_size);
				printf("\tevent_count\t%d\n",mgei.event_count);
			}
		}
		break;
	case 32:
		//
		// Get event
		//
		{
			multi_get_event mge;
			mge.info_size = sizeof(mge);
			rval = ioctl(driver_handle,B_MULTI_GET_EVENT, &mge,0);
			if (B_OK != rval)
			{
				printf("Failed on B_MULTI_GET_EVENT\n");
			}
			else
			{
				int32 ctpb;
				printf("\tevent\t\t0x%x\n",mge.event);
				printf("\ttimestamp\t0x%x\n",mge.timestamp);
				printf("\tcount\t\t%d\n",mge.count);
				for (ctpb = 0; ctpb < mge.count; ctpb++) {
					switch (mge.event) {
						case B_MULTI_EVENT_CONTROL_CHANGED:
							printf("\t\tcontrol #%d\n",mge.u.controls[ctpb]);
							break;
						default:
							printf("\t\tblah blah\n");
							break;	
					}
				}
			}	
		}
		break;
	case 33:
		//
		// Set event info
		//
		{
			multi_set_event_info msei;
			msei.info_size = sizeof(msei);
			printf("Enter events as a hex number\n");
			if (!scanf("%x",&y)) {
				break;
			}

			msei.in_mask = y;
			printf("msei.in_mask is 0x%x\n", msei.in_mask);
			/* current version is non-blocking (no notification) */
			msei.semaphore = -1;
			msei.queue_size = 16;
			rval = ioctl(driver_handle,B_MULTI_SET_EVENT_INFO, &msei,0);
			if (B_OK != rval)
			{
				printf("Failed on B_MULTI_SET_EVENT_INFO\n");
			}
			else
			{
				printf("Currently non-blocking, you must poll\n");
			}
		}
		break;

	case 90:
		//
		// Soup
		//
		{
			printf("yello.\n");
		}
		break;

	default:
		printf("This function is not currently supported\n\n");
		break;
	}
}	
exit:	
	close(driver_handle);
	return 0;
}


