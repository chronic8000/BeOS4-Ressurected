#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
	int fd;
	unsigned char data[2];
	if(argc != 3) return;
	data[0] = atoi(argv[1]);
	data[1] = atoi(argv[2]);
	if((fd = open("/dev/misc/dt300",O_RDWR)) < 0) {
		fprintf(stderr,"damn\n");
		return 1;
	}
	write(fd, data, 2);
	close(fd);
	return 0;	
}

