/* $Id: tcp_file_cli.c,v d8130de1d3eb 2014-11-10 21:59:11Z hchen $ */
/* $HeadURL$ */
/*
 *	tcp_hello_cli.c
 */
/* Copyrignt 2014 Hui Chen, All Rights Reserved. */
static const char scmverid[] = "$Id: tcp_file_cli.c,v d8130de1d3eb 2014-11-10 21:59:11Z hchen $";

/* Description */
/* 
 * A TCP'ed "hello, world" client program that sends the content of a file 
 * to the server.
 */

#if defined(_WIN32)
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
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
	int clifd = -1, msglen, filefd = -1;
	struct sockaddr_in srvaddr;
	char msgbuf[128000];

#if defined(_WIN32)
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
		goto cleanup;
	}
#endif

	if (argc < 2) {
		fprintf(stderr, "Usage: %s file_to_send\n", argv[0]);
		return 1;
	}

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
				sizeof(struct sockaddr_in)) != 0) {
		fprintf(stderr, "%s: %s", argv[0], strerror(errno));
		goto cleanup;
	}

#if defined(_WIN32)
	filefd = _open(argv[1], 0);
#else
	filefd = open(argv[1], 0);
#endif
	if (filefd < 0) {
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));	
		goto cleanup;
	}

	do {
		msglen = read(filefd, msgbuf, sizeof(msgbuf));
		if (msglen > 0)
			send(clifd, msgbuf, msglen, 0);
		else if (msglen < 0) {
			fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
			goto cleanup;
		}
	} while (msglen != 0);

cleanup:
	if (clifd >=0)
#if defined(_WIN32)
		closesocket(clifd);
#else
		close(clifd);
#endif
	if (filefd >=0)
#if defined(_WIN32)
		_close(filefd);
#else
		close(filefd);
#endif

#if defined(_WIN32)
		WSACleanup();
#endif
	return 0;
}
