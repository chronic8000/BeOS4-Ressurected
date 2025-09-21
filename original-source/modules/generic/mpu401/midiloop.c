
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <OS.h>

#define COUNT 1024

int main(int argc, char *argv[])
{
	int fd,i;
	uchar buf[1];
	uchar out[1];
	bigtime_t a,b;
	uint32 min,max;
	float avg;

	uint32 t[COUNT];

	if((argc < 2) || ((fd = open(argv[1],O_RDWR)) < 0)){
		return 0;
	}

	out[0] = 0x9e;

	for(i=0;i<COUNT;i++){	
		a = system_time();
		write(fd, out, 1);
		read(fd, buf, 1);
		b = system_time();
		t[i] = b - a;
	}	
	close(fd);

	avg = 0;
	min = 10000000;
	max = 0;
	for(i=0;i<COUNT;i++){
		if(t[i] < min) min = t[i];
		if(t[i] > max) max = t[i];
		avg += t[i];
	}

	avg = avg / ((float) COUNT);

	printf("min: %5d usec\n",min);
	printf("avg: %5d usec\n",((int) avg));
	printf("max: %5d usec\n",max);

	return 0;
}
