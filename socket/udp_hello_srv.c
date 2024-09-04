/* $Id: udp_hello_srv.c,v d8130de1d3eb 2014-11-10 21:59:11Z hchen $ */
/* $HeadURL$ */
/*
 *	udp_hello_srv.c
 */
/* Copyrignt 2014 Hui Chen, All Rights Reserved. */
static const char scmverid[] = "$Id: udp_hello_srv.c,v d8130de1d3eb 2014-11-10 21:59:11Z hchen $";

/* Description */
/* 
 * A UDP'ed "hello, world"-kind of  server program which dumps whatever it
 * recevies 
 */


#if defined(_WIN32)
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define SERVER_PORT	50002
#ifndef SERVER_IP
#define SERVER_IP	"127.0.0.1"
#endif

void dumpbuf(char *buf, int nrecv);

int main(int argc, char *argv[])
{
	int srvfd = -1, clifd = -1, nrecv;
#if defined(_WIN32)
	int cliaddrlen = sizeof(struct sockaddr_in);
#else
	socklen_t cliaddrlen = sizeof(struct sockaddr_in);
#endif


	struct sockaddr_in srvaddr, cliaddr;
	char msgbuf[65507];

#if defined(_WIN32)
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
		goto cleanup;
	}
#endif

	srvfd = socket(PF_INET, SOCK_DGRAM, 0);
	if (srvfd < 0) {
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
		goto cleanup;
	}

	srvaddr.sin_family = AF_INET;
	srvaddr.sin_port = htons(SERVER_PORT);
#if defined BIND_TO_ANY
	srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
#else
#if defined(_WIN32)
	srvaddr.sin_addr.s_addr = inet_addr(SERVER_IP);
#else
	if (!inet_aton(SERVER_IP, &srvaddr.sin_addr)) {
		fprintf(stderr, "%s: Invalid IP address\n", argv[0]);
		goto cleanup;
	}
#endif
#endif

	if (bind(srvfd, (struct sockaddr*)&srvaddr, sizeof(struct sockaddr_in))
				!= 0) {
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
		goto cleanup;
	}

	do {
		nrecv = recvfrom(srvfd, msgbuf, sizeof(msgbuf), 0,
					(struct sockaddr*)&cliaddr, &cliaddrlen);
		if (nrecv < 0) {
			fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
			goto cleanup;
		}
		dumpbuf(msgbuf, nrecv);
	} while(msgbuf[0] != 0 || nrecv != 1);

cleanup:
	if (srvfd >= 0)
#if defined(_WIN32)
		closesocket(clifd);
#else
		close(srvfd);
#endif
	if (clifd >=0)
#if defined(_WIN32)
		closesocket(clifd);
#else
		close(clifd);
#endif
#if defined(_WIN32)
		WSACleanup();
#endif
	return 0;
}

void dumpbuf(char *buf, int nrecv)
{
#define LINE_WIDTH		74
#define INDEX_LEN		4
#define INDEX_GAP_LEN	2
#define COLUMN_GAP_LEN	4
#define HEX_COLUMN_START	(INDEX_LEN + INDEX_GAP_LEN)
#define HEX_COLUMN_WIDTH	\
				((LINE_WIDTH - HEX_COLUMN_START - COLUMN_GAP_LEN)/4*3)
#define PRINT_COLUMN_START	\
				(HEX_COLUMN_START + HEX_COLUMN_WIDTH + COLUMN_GAP_LEN)
#define PRINT_COLUMN_WIDTH	(HEX_COLUMN_WIDTH/3)
#define	CHAR_PER_LINE	(PRINT_COLUMN_WIDTH)
			
	char idxbuf[INDEX_LEN+1], 
		 hexbuf[HEX_COLUMN_WIDTH+1], 
		 printbuf[PRINT_COLUMN_WIDTH+1];
	int i = 0, hexpos = 0, printpos = 0;


	sprintf(idxbuf, "%0*d", INDEX_LEN, i);
	while (nrecv > 0 && i < nrecv) {
		if (i % CHAR_PER_LINE == 0) {
			printbuf[printpos] = '\0';
			hexpos = 0;
			printpos = 0;
			if (i > 0)
				printf("%s%*c%-*s%*c%s\n", idxbuf, INDEX_GAP_LEN, ' ', 
						HEX_COLUMN_WIDTH, hexbuf, COLUMN_GAP_LEN,
						' ', printbuf);
			sprintf(idxbuf, "%0*x", INDEX_LEN, i);
		}
		hexpos += sprintf(hexbuf+hexpos, "%02x ", (unsigned char)buf[i]); 
		if (isprint(buf[i]))
			printbuf[printpos ++] = buf[i];
		else
			printbuf[printpos ++] = '.';
		i ++;
	}

	if (nrecv > 0) {
		printbuf[printpos] = '\0';
		printf("%s%*c%-*s%*c%s\n", idxbuf, INDEX_GAP_LEN, ' ', 
				HEX_COLUMN_WIDTH, hexbuf, COLUMN_GAP_LEN, ' ', printbuf);
	}
}

