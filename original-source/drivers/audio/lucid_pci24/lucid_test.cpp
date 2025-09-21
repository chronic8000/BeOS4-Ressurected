#include "lucid_pci24.h"
#include <stdio.h>
#include <Application.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <errno.h>

volatile int quitf = 0;


static void
quit(
	int sig)
{
	quitf = 1;
	if (be_app) {
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
}


#define NSAMP 100000


static status_t
pcm(
	void * arg)
{
	int i = 0;
	char path[200];
	sprintf(path, "/dev/%s", arg);
	int fd = open((char *)path, O_RDWR);
	if (fd < 0) {
		perror("lucid_test: cannot open lucid pci24 audio");
		return -1;
	}

	pcm_cfg config;
	config.sample_rate = 44100.0;
	config.channels = 2;
	config.format = 0x2;
	config.big_endian = 0;
	config.buf_header = 0;
	printf("lucid_test: testing ioctl(B_AUDIO_SET_AUDIO_FORMAT)\n");
	if (ioctl(fd, B_AUDIO_SET_AUDIO_FORMAT, &config, 0) < 0) {
		close(fd);
		perror("lucid_test: cannot set audio format");
		return -1;
	}
	printf("lucid_test: testing ioctl(B_AUDIO_GET_AUDIO_FORMAT)\n");
	if (ioctl(fd, B_AUDIO_GET_AUDIO_FORMAT, &config, 0) < 0) {
		close(fd);
		perror("lucid_test: cannot get audio format");
		return -1;
	}
	printf("lucid_test: past the ioctlz\n");
	printf("lucid_test: format = sr %g  ch %d  fo %x  es %d  bh %d\n",
		config.sample_rate, config.channels, config.format, 
		config.big_endian, config.buf_header);
	int m = config.channels*(config.format&0xf);
	void *buf = (void*)malloc((NSAMP+1)*m);
	if (!buf) {
		close(fd);
		perror("lucid_test: cannot malloc()");
		return -1;
	}
	while (1) {
//read and write test
#if 1
	if(read(fd, buf, NSAMP*m) == NSAMP*m){
		printf("lucid_test: read completed\n");
		int16 *snd = (int16 *)buf;
/*		for(int i=0; i<NSAMP*m/2; i++){
			printf("%x, %x\n", *(snd+i), *(snd+i+2));
		}
*/		if(write(fd, buf, NSAMP*m) == NSAMP*m){
			printf("lucid_test: write completed\n");
		}
	}
#endif
//write tone test
#if 0
		int c = config.channels;
		int16 sample = 0;
		int16 rng = 30000;
		int16 period = 200 + i;
		
		i += 10;
		
		for (int ix=0; ix<NSAMP; ix++) {
			if (ix % period < period/2) {
				sample = rng/2;
			}
			else {
				sample = rng/-2;
			}
			((short*)buf)[ix*c] = ((short*)buf)[ix*c+1] = sample;
		}
		//printf("press return to write\n");
		//getchar();
		write(fd, buf, NSAMP*m);
#endif
//write sound file test
#if 0
    	int ffd;
		char buff[1024*64];

		ffd = open("/boot/optional/sound/TheMasque", O_RDONLY);
		if (ffd < 0)
	 		exit(5);
	
		size_t written = 0;
		bigtime_t start = system_time();
	
		while(read(ffd, buff, sizeof(buff)) == sizeof(buff)) {
/*
			int i, x;
			for(i=0; i < sizeof(buff); i+=2) {
				buff[i] ^= buff[i+1];
				buff[i+1] ^= buff[i];
				buff[i] ^= buff[i+1];
			}
*/
			written += write(fd, buff, sizeof(buff));
		}
	
		bigtime_t end = system_time();
		float sr = (written/4.0)/((end-start)/1000000.0);
		printf("approx sample rate = %f samples per second\n", sr);
	
		close(ffd);
#endif

	}
	close(fd);
	if (be_app) {
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	return B_OK;
}

void
main(
	int argc,
	char * argv[])
{
	status_t status;
	const char * driver;
	if (argc > 2) {
		fprintf(stderr, "usage: lucid_test [driver]\n");
	}
	if (argc == 2) {
		driver = argv[1];
	}
	else {
		driver = "lucid_pci24_1";
	}
	BApplication app("application/x-lucid_test");
	thread_id pcm_t = spawn_thread(pcm, "pcm", B_REAL_TIME_PRIORITY, (void*)driver);
	resume_thread(pcm_t);
	app.Run();
	wait_for_thread(pcm_t, &status);
	exit(0);
}

