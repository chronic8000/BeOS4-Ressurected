/*
   Test app to diddle the /dev/ir devices

   Dominic Giampaolo
   dbg@be.com
*/   
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <KernelKit.h>
#include <Drivers.h>
#include "../ir.h"

void
error(char *str)
{
	if (str)
		fprintf(stderr, "%s\n", str);

	exit(5);
}

void
my_handler(int sig)
{
	exit(-1);
}



#define IR_PORT      "/dev/beboxhw/infrared/ir2"
#define IR_BUF_SIZE  4096


main(int argc, char **argv)
{
	int fd, ch, n, i;
	char buff[256], *ptr, junk[256];
	unsigned char data[IR_BUF_SIZE];
	int anal_data[2];
	struct sigaction sa;
	char *ir_port = IR_PORT, *prompt;

	if (argv[1] != NULL)
		ir_port = argv[1];

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = my_handler;
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);

	fd = open(ir_port, O_RDWR);
	if (fd < 0)
		error("gack! can't open ir port!");

	prompt = ir_port;

	printf("opened %s....\n", ir_port);

	printf("%s> ", prompt);
	fflush(stdout);
	
	while(fgets(buff, 256, stdin) != NULL) {
		ptr = strrchr(&buff[0], '\n');
		if (ptr)
			*ptr = '\0';   /* kill the newline */

		while(*ptr == ' ' || *ptr == '\t')
			ptr++;

		if (strcmp("reset", buff) == 0) {

			if (ioctl(fd, IR_RESET, 0) < 0)
				perror("reset");

		} else if (strcmp("listen", buff) == 0) {

			if (ioctl(fd, IR_SET_LISTEN_MODE, 0) < 0)
				perror("startl");

		} else if (strcmp("sample", buff) == 0) {

			if (ioctl(fd, IR_SET_SAMPLE_MODE, 0) < 0)
				perror("startsample");

		} else if (strcmp("analyze", buff) == 0) {

			if (ioctl(fd, IR_START_ANALYSIS, 0) < 0)
				perror("analyze");

		} else if (strcmp("stopa", buff) == 0) {

			if (ioctl(fd, IR_STOP_ANALYSIS, 0) < 0)
				perror("stopa");

		} else if (strcmp("senda", buff) == 0) {

			if (ioctl(fd, IR_SEND_ANALYSIS, &anal_data[0]) < 0)
				perror("senda");
			printf("results: 0x%x 0x%x\n", anal_data[0], anal_data[1]); 

		} else if (strncmp("loadset", buff, 7) == 0) {

			sscanf(buff, "%s %i %i", junk, &anal_data[0], &anal_data[1]);

			if (ioctl(fd, IR_LOAD_SETTINGS, &anal_data[0]) < 0)
				perror("load_settings");

		} else if (strncmp("read", buff, 4) == 0) {
			int  count;

			if (sscanf(buff, "%s %i", junk, &count) != 2) {
				count = 32;
				printf("no count specified, reading %d bytes\n", count);
			} else {
				printf("attempting to read %d bytes\n", count);
			}
			

			if ((count = read(fd, data, count)) < 0)
				perror("read");
			else {
				int i, j;

				printf("read %d bytes:\n-----------------------\n", count);
				for(i=0; i < count; i++) {
					printf("0x%.2x ", (unsigned int)data[i]);
					if (((i+1) % 16) == 0)
						printf("\n");
				}
				if ((i % 16) != 0)
					printf("\n");
			}

		} else if (strncmp("write", buff, 4) == 0) {
			int  count, i;

			if (sscanf(buff, "%s %i", junk, &count) != 2) {
				count = 12;
				printf("no count specified, writing %d bytes\n", count);
			} else {
				printf("attempting to write %d bytes\n", count);
			}
			
			if ((count = write(fd, data, count)) < 0)
				perror("write");

		} else if (strcmp("doit", buff) == 0) {

			if (ioctl(fd, IR_START_ANALYSIS, 0) < 0)
				perror("analyze");
			
			printf("press return to continue"); fflush(stdout);
			getc(stdin);
			
			if (ioctl(fd, IR_STOP_ANALYSIS, 0) < 0)
				perror("stopa");

			if (ioctl(fd, IR_SEND_ANALYSIS, &anal_data[0]) < 0)
				perror("senda");
			printf("results: 0x%x 0x%x\n", anal_data[0], anal_data[1]); 

		} else if (strcmp("quit", buff) == 0) {
			break;
		} else if (buff[0] != '\0') {
			printf("unknown cmd: %s\n", buff);
		}


		printf("%s> ", prompt);
		fflush(stdout);
	}


	close(fd);
}

