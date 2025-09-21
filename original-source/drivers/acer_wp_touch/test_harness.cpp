#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <SupportDefs.h>

const char kDriverPath[] = "/dev/input/touchscreen/acer-wp-touch/0";

// yet another reason why this struct needs to be in a header file
typedef struct {
	bigtime_t time;
	uint16 x;
	uint16 y;
	bool pressed;
} sample_t;

int main(void)
{
	status_t err = B_OK;
	
	int fd = open(kDriverPath, O_RDONLY);
	if (fd < 0) {
		printf("Couldn't open %s: %s\n", kDriverPath, strerror(fd));
		err = fd;
		goto err1;
	}
	
	sample_t sample;
	size_t count;

	for (int i = 0; i < 16; i++) {
		count = read(fd, &sample, sizeof (sample));
		printf("read() returned %ld\n", count);
		printf("sample.time = %Ld\n", sample.time);
		printf("sample.x = %u\n", sample.x);
		printf("sample.y = %u\n", sample.y);
		printf("sample.pressed = %d\n", sample.pressed);
	}
	
	close(fd);

err1:
	return err;
}