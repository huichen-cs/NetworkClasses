/* $Id: msgsrv.c,v d8130de1d3eb 2014-11-10 21:59:11Z hchen $ */
/* $HeadURL$ */
/*
 *	msgsrv.c
 */
/* Copyrignt 2014 Hui Chen, All Rights Reserved. */
static const char scmverid[] = "$Id: msgsrv.c,v d8130de1d3eb 2014-11-10 21:59:11Z hchen $";

/* Description */
/* 
 * The server of a simple point-2-point full-duplex talker application
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

#define	SERVER_PORT	58008
#define BACKLOG		10

static int srvfd = -1,
		   clifd = -1;

static WINDOW *mainwin = NULL,
			  *inputwin = NULL,
			  *msgwin = NULL;

void docleanup(int s);
int readline(char *buf, int bufsize);

int main()
{
	int	rc = 0,
		msglen;
	fd_set readfds;
	struct sockaddr_in srvaddr,
					   cliaddr;
	socklen_t cliaddrlen = sizeof(struct sockaddr_in);
	char buf[512];

	signal(SIGINT, docleanup);

	fprintf(stderr, "begin\n"); 

	srvfd = socket(PF_INET, SOCK_STREAM, 0);
	if (srvfd < 0) {
		fprintf(stderr, "%s:%d:socket() -> %s\n", __FUNCTION__, __LINE__, 
				strerror(errno));
		rc = -1;
		goto cleanup;
	}
	
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_port = htons(SERVER_PORT);
	srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(srvfd, (struct sockaddr*)&srvaddr, sizeof(struct sockaddr_in)) 
			!= 0) {
		fprintf(stderr, "%s:%d:bind() -> %s\n", __FUNCTION__, __LINE__,
				strerror(errno));
		rc = -1;
		goto cleanup;
	}

	if (listen(srvfd, BACKLOG) != 0) {
		fprintf(stderr, "%s:%d:listen() -> %s\n", __FUNCTION__, __LINE__,
				strerror(errno));
		rc = -1;
		goto cleanup;
	}

	clifd = accept(srvfd, (struct sockaddr*)&cliaddr, &cliaddrlen);
	if (clifd < 0) {
		fprintf(stderr, "%s:%d:accept() -> %s\n", __FUNCTION__, __LINE__,
				strerror(errno));
		rc = -1;
		goto cleanup;
	}

	mainwin = initscr();
	cbreak();
	noecho();
	nl();
	if (mainwin == NULL) {
		fprintf(stderr, "%s:%d:initscr(): failed\n", __FUNCTION__, __LINE__);
		rc = -1;
		goto cleanup;
	}

	inputwin = newwin(8, 80, 16, 0);
	if (inputwin == NULL) {
		fprintf(stderr, "%s:%d:create msgwin failed\n", __FUNCTION__, __LINE__);
		rc = -1;
		goto cleanup;
	}
	fprintf(stderr, "111");

	msgwin = newwin(15, 80, 0, 0);
	if (msgwin == NULL) {
		fprintf(stderr, "%s:%d:create inputwin failed\n", __FUNCTION__,
				__LINE__);
		rc = -1;
		goto cleanup;
	}
	scrollok(msgwin, true);
	move(15, 0);
	hline(0, 80);
	refresh();
	wmove(inputwin, 0, 0);
	wrefresh(inputwin);

	do {
		FD_ZERO(&readfds);
		FD_SET(0, &readfds);
		FD_SET(clifd, &readfds);
		rc = select(clifd+1, &readfds, NULL, NULL, NULL);
		if (rc == -1) {
			fprintf(stderr, "%s:%d:select() -> %s\n", __FUNCTION__,
					__LINE__, strerror(errno));
			rc = -1;
			goto cleanup;
		} else if (rc) {
			if (FD_ISSET(0, &readfds)) {
				if (readline(buf, sizeof(buf))) {
					msglen = strlen(buf) + 1;
					if (send(clifd, buf, msglen, 0) < 0) {
						fprintf(stderr, "%s:%d:send() -> %s\n", __FUNCTION__,
								__LINE__, strerror(errno));
						rc = -1;
						goto cleanup;
					}
				}
			}
			if (FD_ISSET(clifd, &readfds)) {
				msglen = recv(clifd, buf, sizeof(buf), 0);
				fprintf(stderr, "received input <%s>\n", buf);
				if (msglen < 0) {
					fprintf(stderr, "%s:%d:send() -> %s\n", __FUNCTION__,
							__LINE__, strerror(errno));
					rc = -1;
					goto cleanup;
				} else if (msglen == 0) {
					fprintf(stderr, "%s:%d: client closed connection!\n",
							__FUNCTION__, __LINE__);
					rc = 0;
					goto cleanup;
				}
				wprintw(msgwin, "%s\n", buf);
				wrefresh(msgwin);
			}
		} 
	} while(1);


cleanup:
	docleanup(rc);
	return rc;
}


void docleanup(int s)
{
	if (srvfd >= 0)
		close(srvfd);
	if (clifd >= 0)
		close(clifd);
	if (inputwin)
		delwin(inputwin);
	if (msgwin)
		delwin(msgwin);
	if (mainwin)
		endwin();

	if (s)
		exit (s);
}

int readline(char *buf, int bufsize)
{
	static int idx = 0;

	if (idx < bufsize-1) 
		read(0, buf+idx, 1);


	if (buf[idx] == '\n' || buf[idx] == '\r') {
		buf[idx] = '\0';
		idx = 0;
		waddch(inputwin, '\n');
		wrefresh(inputwin);
		return 1;
	} else {
		waddch(inputwin, buf[idx]); 
		wrefresh(inputwin);
		idx ++;
		if (idx == bufsize-1) {
			beep();
		}
	}

	return 0;
}

