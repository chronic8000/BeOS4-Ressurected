
#include <stdio.h>
#include <Drivers.h>

typedef struct
{
	uint32 devnum;
	int blocksize;
	char *path;
} devinfo;

int main(int argc, char *argv[])
{
	devinfo di;
	int e, fd;
	char ld[128];

	if(argc != 4) {
		fprintf(stderr,"usage: loopconfig <devicenum> <file> <blocksize>\n");
		exit(0);
	}

	sprintf(ld,"/dev/config/loopback");

	if((fd = open(ld,0)) < 0) {
		fprintf(stderr,"can't open %s\n",argv[1]);
		return 0;
	}

	di.devnum = atoi(argv[1]);
	di.path = argv[2];
	di.blocksize = atoi(argv[3]);

	fprintf(stderr,"%s: file='%s' blocksize=%d\n",argv[1],di.path,di.blocksize);
	if(e = ioctl(fd, B_SET_DEVICE_SIZE, &di, sizeof(di)))
		fprintf(stderr,"error: ioctl() returns %d\n",e);

	close(fd);	
	return 0;
}
