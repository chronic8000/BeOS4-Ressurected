
#include <OS.h>
#include <stdio.h>

int
main()
{
	port_id p = create_port(20, "cs4280 logger");
	char msg[256];
	int len;
	int32 c;
	fprintf(stdout, "logger started\n");
	fflush(stdout);
	while ((len = read_port(p, &c, msg, 256)) > 0) {
		msg[len] = 0;
		fprintf(stdout, "%s", msg);
		fflush(stdout);
	}
	return 0;
}

