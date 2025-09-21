#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <Drivers.h>
#include "dac960.h"

const char *logical_state(uint32 state)
{
	switch(state){
		case 0x03: return "ONLINE"; 
		case 0x04: return "CRITICAL"; 
		case 0xFF: return "OFFLINE"; 
		default:   return "UNKNOWN";
	}
}

const char *physical_state(uint32 state)
{
	switch(state){
		case 0x00: return "DEAD";
		case 0x02: return "WRITE ONLY"; 
		case 0x03: return "ONLINE"; 
		case 0x10: return "STANDBY";
		default: return "UNKNOWN"; 
	}
}

const char *raid_level(uint32 level)
{
	switch(level & 0x07){
		case 0: return "RAID 0 (striped)";
		case 1: return "RAID 1 (mirrored)";
		case 3: return "RAID 3";
		case 5: return "RAID 5";
		case 6: return "RAID 6 (Mylex)";
		case 7: return "JBOD (single disk)";
		default: return "UNKNOWN";
	}
}

void print_info(drive_info *di)
{
	int i,j;
	sys_drv *info = &di->sys_drv;
	device_state *ds;
	
	printf("DAC960 Logical Drive Info\n");
	printf("-----------------------------------------------------\n");
	printf("Capacity    : %dMB\n", di->blocks / 2048);
	printf("Array Type  : %s\n", raid_level(info->raid_level));
	printf("Cache Type  : %s\n", info->raid_level & 0x80 
		   ? "Write Back" : "Write Through");
	printf("Status      : %s\n", logical_state(info->state));
	printf("Spans       : %d\n",info->valid_spans);
	printf("Arms        : %d\n",info->valid_arms);
	for(i=0;i<info->valid_spans;i++){
		printf("Span %2d     : %dMB @ block %d\n",
			   i,info->span[i].blocks / 2048,info->span[i].start);
		for(j=0;j<info->valid_arms;j++){
			ds = &di->state[i][j];
			printf("  Dev %02d:%02d : %s\n",
				   info->span[i].arm[j].channel,
				   info->span[i].arm[j].target,
				   physical_state(ds->status));
		}
	}

}

int main(int argc, char *argv[])
{
	int fd;
	drive_info di;
	uint32 x[3];
	
	if(argc < 2) return;
	if(strncmp(argv[1],"/dev/disk/dac960/",16)) return 0;
	
	if((fd = open(argv[1], O_RDONLY)) < 0) return 0;
	
	if(!ioctl(fd, 5000, &di, sizeof(di))) print_info(&di);
	
	if(!ioctl(fd, 5001, x, 4*3)) {
		printf("*** Rebuilding Drive %d (%dMB of %dMB complete)\n",
				x[0], x[1]/2048 - x[2]/2048, x[1]/2048);
	}

	if(argc == 4){
		ioctl_rebuild r;
		r.channel = atoi(argv[2]);
		r.target = atoi(argv[3]);
		if(!ioctl(fd, 5002, &r, sizeof(ioctl_rebuild))){
			printf("*** Rebuild Started ***\n");
		} else {
			printf("*** Rebuild Rejected ***\n");
		}
	}
		
	close(fd);
	return 0;
}
					