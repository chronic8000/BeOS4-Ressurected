
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct event {
	enum { NONE=0, PENDOWN, PENUP, BUTTONS } type;
	unsigned short x;
	unsigned short y;
} event;

int main(int argc, char *argv[])
{
	int fd;
	event e;

	if((fd = open("/dev/misc/dt300",O_RDONLY)) < 0) {
		fprintf(stderr,"damn\n");
		return 1;
	}
	while(read(fd,&e,6) == 6) {
		switch(e.type){
		case PENDOWN:
			printf("down %d,%d\n",e.x,e.y);
			break;
		case PENUP:
			printf("up\n");
			break;
		case BUTTONS:
			printf("buttons %02x\n",e.x);
			break;
		}
	}

	close(fd);
	return 0;
}
