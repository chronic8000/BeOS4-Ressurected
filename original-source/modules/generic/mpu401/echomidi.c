#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char **argv)
{
	unsigned char byte;
	char *port = NULL;
	int thru = 0;
	int fd;

	argv++;	
	while(argc > 1){
		if(!strcmp("-thru",*argv)){
			thru = 1;
		}
		if(!(strncmp("/dev/midi",*argv,9))){
			port = *argv;
		}
		argv++;
		argc--;
	}
	
	if(!port) {
		fprintf(stderr,"No device specified\n");
		return 0;
	}

	if((fd = open(port,O_RDWR)) < 0) {
		fprintf(stderr,"Cannot open '%s'\n",port);
		return 0;
	}

	for(;;){
		read(fd,&byte,1);
		if(thru) write(fd,&byte,1);
		printf("%02x ",byte);
		fflush(stdout);
	}
	close(fd);
	return 0;
}
