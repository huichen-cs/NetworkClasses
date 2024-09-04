/* $Id: tcp_hello_cli.c,v d8130de1d3eb 2014-11-10 21:59:11Z hchen $ */
/* $HeadURL$ */
/*
 *	tcp_hello_cli.c
 */
/* Copyrignt 2014 Hui Chen, All Rights Reserved. */
static const char scmverid[] = "$Id: tcp_hello_cli.c,v d8130de1d3eb 2014-11-10 21:59:11Z hchen $";

/* Description */
/* 
 * A TCP'ed "hello, world" client program 
 */


#if defined(_WIN32)
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#endif
#include <stdio.h>
#include <string.h>

#define SERVER_PORT	50002
#ifndef SERVER_IP
#define SERVER_IP	"127.0.0.1"
#endif

int main(int argc, char *argv[])
{
	int clifd = -1, msglen;
	struct sockaddr_in srvaddr;
	char msgbuf[2048];

#if defined(_WIN32)
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
		goto cleanup;
	}
#endif

	clifd = socket(PF_INET, SOCK_STREAM, 0);
	if (clifd < 0) {
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
		goto cleanup;
	}

	srvaddr.sin_family = AF_INET;
	srvaddr.sin_port = htons(SERVER_PORT);
#if defined(_WIN32)
	srvaddr.sin_addr.s_addr = inet_addr(SERVER_IP);
#else
	if (!inet_aton(SERVER_IP, &srvaddr.sin_addr)) {
		fprintf(stderr, "%s: Invalid IP address\n", argv[0]);
		goto cleanup;
	}
#endif

	if (connect(clifd, (struct sockaddr*)&srvaddr,
				sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "%s: %s", argv[0], strerror(errno));
		goto cleanup;
	}

	msglen = sprintf(msgbuf,
				"hello, world!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
				"hello, world!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
				"hello, world!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
				"hello, world!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
				"hello, world!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

	if (send(clifd, msgbuf, msglen, 0) < 0) {
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
		goto cleanup;
	}

cleanup:
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

