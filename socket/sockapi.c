/* $Id: sockapi.c,v d8130de1d3eb 2014-11-10 21:59:11Z hchen $ */
/* $HeadURL$ */
/*
 *	sockapi.c
 */
/* Copyrignt 2014 Hui Chen, All Rights Reserved. */
static const char scmverid[] = "$Id: sockapi.c,v d8130de1d3eb 2014-11-10 21:59:11Z hchen $";

/* Description */
/* 
 * Describe this module briefly
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>


int main(int argc, char *argv[])
{
	struct hostent *hp;
	char **p;
	int i;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s host\n", argv[0]);
		return 0;
	}

	hp = gethostbyname(argv[1]);
	if (!hp) {
		perror("gethostbyname: ");
		return 1;
	}

	printf("%s ->\n", argv[1]);
	p = hp->h_addr_list;
	while (*p) {
		printf("\t%s\n", inet_ntoa(*((struct in_addr*)*p)));
		p ++;
	}

	printf("%s in network order ->\n", argv[1]);
	p = hp->h_addr_list;
	while (*p) {
		printf("\t");
		for (i=0; i<hp->h_length; i++)
			printf("%02x", (unsigned char)(*p)[i]);
		printf("\n");
		p ++;
	}

	return 0;
}

