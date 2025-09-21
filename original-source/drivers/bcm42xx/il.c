/*
    EXTERNAL SOURCE RELEASE on 05/16/2000 - Subject to change without notice.

*/
/*
    Copyright 2000, Broadcom Corporation
    All Rights Reserved.
    
    This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
    the contents of this file may not be disclosed to third parties, copied or
    duplicated in any form, in whole or in part, without the prior written
    permission of Broadcom Corporation.
    

*/
/*
 * BSD/ifnet iline10 driver ioctl swiss army knife command.
 * $Id: il.c,v 1.8 2000/03/08 05:21:44 abagchi Exp $
 */

#include <stdio.h>

#ifndef linux
#include <pcap.h>
#endif
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
//#include <sys/sysctl.h>
#include <sys/time.h>
#include <net/if.h>
#include <netinet/in.h>
#include <typedefs.h>
#include <ilsockio.h>

static void usage(char *av0);
static void syserr(char *s);

#ifdef linux
char *interface = "eth1";
#elif defined(__BEOS__)
char *interface = "net/bcm42xx/0";
#else
char *interface = "il0";
#endif
char buf[4096];

#define VECLEN		2

int optind;

main(int ac, char *av[])
{
	struct ifreq ifr;
	uint arg;
	int vecarg[VECLEN], i;
	int c;
	int s;

	if (ac < 2)
		usage(av[0]);

	optind = 1;

	if (av[1][0] == '-') {
		if ((av[1][1] != 'a') && (av[1][1] != 'i'))
			usage(av[0]);
		if (ac < 4)
			usage(av[0]);
		interface = av[2];
		optind += 2;
	}

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		syserr("socket");

	strncpy(ifr.ifr_name, interface, sizeof (ifr.ifr_name));

	if (strcmp(av[optind], "txdown") == 0) {
		if (ioctl(s, SIOCSILTXDOWN, (caddr_t)&ifr) < 0)
			syserr("siocsiltxdown");
	}
	else if (strcmp(av[optind], "loop") == 0) {
		if (optind >= (ac -1)) usage(av[0]);
		arg = atoi(av[optind + 1]);
		ifr.ifr_data = (caddr_t) &arg;
		if (ioctl(s, SIOCSILLOOP, (caddr_t)&ifr) < 0)
			syserr("siocsilloop");
	}
	else if (strcmp(av[optind], "dump") == 0) {
		ifr.ifr_data = buf;
		if (ioctl(s, SIOCGILDUMP, (caddr_t)&ifr) < 0)
			syserr("siocgildump");
		printf("%s\n", buf);
	}
	else if (strcmp(av[optind], "scbdump") == 0) {
		ifr.ifr_data = buf;
		if (ioctl(s, SIOCGILDUMPSCB, (caddr_t)&ifr) < 0)
			syserr("siocgildumpscb");
		printf("%s\n", buf);
	}
	else if (strcmp(av[optind], "pesdump") == 0) {
		if (optind >= (ac -1))
			usage(av[0]);
		arg = atoi(av[optind + 1]);
		bcopy((void *)&arg, buf, sizeof(int));
		ifr.ifr_data = buf;
		if (ioctl(s, SIOCGILDUMPPES, (caddr_t)&ifr) < 0) {
			syserr("siocgildumppes");
		} else {
			printf("%s\n", buf);
		}
	}
	else if (strcmp(av[optind], "pesset") == 0) {
		if ((optind >= (ac -1)) || (ac < 4))
			usage(av[0]);
		for (i = 0; i < 2; i++) {
			vecarg[i] = atoi(av[optind + 1 + i]);
		}
		ifr.ifr_data = (caddr_t) &(vecarg[0]);
		if (ioctl(s, SIOCSILPESSET, (caddr_t)&ifr) < 0)
			syserr("siocsilpesset");
	}
	else if (strcmp(av[optind], "txbpb") == 0) {
		if (optind >= (ac -1))
			usage(av[0]);
		arg = atoi(av[optind + 1]);
		ifr.ifr_data = (caddr_t) &arg;
		if (ioctl(s, SIOCSILTXBPB, (caddr_t)&ifr) < 0)
			syserr("siocsiltxbpb");
	}
	else if (strcmp(av[optind], "txpri") == 0) {
		if (optind >= (ac -1))
			usage(av[0]);
		arg = atoi(av[optind + 1]);
		ifr.ifr_data = (caddr_t) &arg;
		if (ioctl(s, SIOCSILTXPRI, (caddr_t)&ifr) < 0)
			syserr("siocsiltxpri");
	}
	else if (strcmp(av[optind], "msglevel") == 0) {
		if (optind >= (ac -1))
			usage(av[0]);
		arg = atoi(av[optind + 1]);
		ifr.ifr_data = (caddr_t) &arg;
		if (ioctl(s, SIOCSILSETMSGLEVEL, (caddr_t)&ifr) < 0)
			syserr("siocsilsetmsglevel");
	}
	else if (strcmp(av[optind], "promisctype") == 0) {
		if (optind >= (ac -1))
			usage(av[0]);
		arg = atoi(av[optind + 1]);
		ifr.ifr_data = (caddr_t) &arg;
		if (ioctl(s, SIOCSILPROMISCTYPE, (caddr_t)&ifr) < 0)
			syserr("siocsilpromisctype");
	}
	else if (strcmp(av[optind], "linkint") == 0) {
		if (optind >= (ac -1))
			usage(av[0]);
		arg = atoi(av[optind + 1]);
		ifr.ifr_data = (caddr_t) &arg;
		if (ioctl(s, SIOCSILLINKINT, (caddr_t)&ifr) < 0)
			syserr("siocsillinkint");
	}
#if !defined(linux) && !defined(__BEOS__)
	else if (strcmp(av[optind], "bpfext") == 0) {
		static pcap_t *pd;
		char ebuf[PCAP_ERRBUF_SIZE];
		if (optind >= (ac -1))
			usage(av[0]);
		arg = atoi(av[optind + 1]);
		pd = pcap_open_live(interface, 68, 0, 1000, ebuf);
		if (pd == NULL) {
			fprintf(stderr, "%s\n", ebuf); 
			exit(1);
		}
		ifr.ifr_data = (caddr_t) &arg;
		if (ioctl(s, SIOCSILBPFEXT, (caddr_t)&ifr) < 0)
			syserr("siocsilbpfext");
		pcap_close(pd);
	}
#endif
	else if (strcmp(av[optind], "hpnamode") == 0) {
		if (optind >= (ac -1))
			usage(av[0]);
		arg = atoi(av[optind + 1]);
		ifr.ifr_data = (caddr_t) &arg;
		if (ioctl(s, SIOCSILHPNAMODE, (caddr_t)&ifr) < 0)
			syserr("siocsilhpnamode");
	}
	else if (strcmp(av[optind], "csa") == 0) {
		if (optind >= (ac -1))
			usage(av[0]);
		arg = atoi(av[optind + 1]);
		ifr.ifr_data = (caddr_t) &arg;
		if (ioctl(s, SIOCSILCSA, (caddr_t)&ifr) < 0)
			syserr("siocsilcsa");
	}
	else if (strcmp(av[optind], "csahpnamode") == 0) {
		if (optind >= (ac -1))
			usage(av[0]);
		arg = atoi(av[optind + 1]);
		ifr.ifr_data = (caddr_t) &arg;
		if (ioctl(s, SIOCSILCSAHPNAMODE, (caddr_t)&ifr) < 0)
			syserr("siocsilcsahpnamode");
	}
	else if (strcmp(av[optind], "larqdump") == 0) {
		ifr.ifr_data = buf;
		if (ioctl(s, SIOCGILLARQDUMP, (caddr_t)&ifr) < 0)
			syserr("siocgillarqdump");
		printf("%s\n", buf);
	}
	else if (strcmp(av[optind], "larq") == 0) {
		if (optind >= (ac -1))
			usage(av[0]);
		arg = atoi(av[optind + 1]);
		ifr.ifr_data = (caddr_t) &arg;
		if (ioctl(s, SIOCSILLARQONOFF, (caddr_t)&ifr) < 0)
			syserr("siocsillarqonoff");
	}
	else
		usage(av[0]);
}

static void
usage(char *av0)
{
	fprintf(stderr, "usage: %s [ -a if ] [ -i if ] [txdown loop dump scbdump "
		"pesdump pesset txbpb txpri msglevel promisctype linkint bpfext "
		"hpnamode csa csahpnamode larqdump larq] [ arg ]\n", av0);
	exit(1);
}

static void
syserr(char *s)
{
	fprintf(stderr, "%s: ", interface);
	perror(s);
	exit(1);
}
