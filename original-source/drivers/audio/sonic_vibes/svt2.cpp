#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <OS.h>

#include "midi_driver.h"


int
main(
	int argc,
	char * argv[])
{
	int fd = open("/dev/midi/sonic_vibes_1", O_RDWR);
	fprintf(stderr, "open(): %d\n", fd);
	unsigned char data[16];
	int rd;
	midi_timed_data mtd;
	if (argc != 2) {
		fprintf(stderr, "usage: svt2 { read | write | timed_read | timed_write }\n");
		exit(1);
	}
	if (!strcmp(argv[1], "read")) {
		while ((rd = read(fd, data, 16)) > 0) {
			fprintf(stderr, "%02x: ", rd);
			for (int ix=0; ix<rd; ix++) {
				fprintf(stderr, "%02x, ", data[ix]);
			}
			fprintf(stderr, "\n");
		}
	}
	else if (!strcmp(argv[1], "write")) {
		data[0] = 0x9f;
		data[1] = 0x3c;
		data[2] = 0x55;
		rd = write(fd, data, 3);
		if (rd != 3) {
			fprintf(stderr, "NoteOn: write() returns %d (should be %d)\n", rd, 3);
			exit(1);
		}
		data[0] = 0x8f;
		snooze(1000000);
		rd = write(fd, data, 3);
		if (rd != 3) {
			fprintf(stderr, "NoteOff: write() returns %d (should be %d)\n", rd, 3);
			exit(1);
		}
	}
	else if (!strcmp(argv[1], "timed_read")) {
		mtd.data = data;
		mtd.size = 16;
		while ((rd = ioctl(fd, B_MIDI_TIMED_READ, &mtd, sizeof(mtd))) >= 0) {
			if (mtd.size <= 0) {
				fprintf(stderr, "timed_read: size = %d\n", mtd.size);
				exit(2);
			}
			fprintf(stderr, "%12Lx: %x: ", mtd.when, mtd.size);
			for (int ix=0; ix<mtd.size; ix++) {
				fprintf(stderr, "%x, ", mtd.data[ix]);
			}
			fprintf(stderr, "\n");
			mtd.data = data;
			mtd.size = 16;
		}
	}
	else if (!strcmp(argv[1], "timed_write")) {
		mtd.when = system_time();
		mtd.size = 3;
		mtd.data = data;
		data[0] = 0x9f;
		data[1] = 0x3c;
		data[2] = 0x55;
		rd = ioctl(fd, B_MIDI_TIMED_WRITE, &mtd, sizeof(mtd));
		if (rd < B_OK) {
			fprintf(stderr, "NoteOn: returns %x\n", rd);
			exit(1);
		}
		data[0] = 0x8f;
		mtd.when = system_time()+1000000;
		mtd.size = 3;
		mtd.data = data;
		rd = ioctl(fd, B_MIDI_TIMED_WRITE, &mtd, sizeof(mtd));
		if (rd < B_OK) {
			fprintf(stderr, "NoteOff: returns %x\n", rd);
			exit(1);
		}
	}
	else {
		fprintf(stderr, "usage: unknown argument %s\n", argv[1]);
		exit(1);
	}
	if (fd > 0) {
		fprintf(stderr, "before sync\n");
		rd = ioctl(fd, B_MIDI_WRITE_SYNC);
		fprintf(stderr, "sync returns %d\n", rd);
		close(fd);
	}
	return 0;
}

