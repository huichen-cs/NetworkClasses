#if defined (_WIN32)
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#endif
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int clifd = -1, msglen;
	struct sockaddr_in srvaddr;
	struct hostent *host;
	char msgbuf[2048],
		 *server_port,
		 *server_addr;



#if defined(_WIN32)
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
		goto cleanup;
	}
#endif

	if (argc < 3) {
		fprintf(stderr, "Usage: tcp_cli server port\n");
		return 0;
	}

	server_addr = argv[1];
	server_port = argv[2];


	clifd = socket(PF_INET, SOCK_STREAM, 0);
	if (clifd < 0) {
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
		goto cleanup;
	}

	srvaddr.sin_family = AF_INET;
	srvaddr.sin_port = htons(atoi(server_port));
	host = gethostbyname(server_addr);
	if (!host) {
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
		goto cleanup;
	}
	memcpy(&(srvaddr.sin_addr.s_addr), host->h_addr_list[0], host->h_length);

	if (connect(clifd, (struct sockaddr*)&srvaddr,
				sizeof(struct sockaddr_in)) != 0) {
		fprintf(stderr, "%s: %s", argv[0], strerror(errno));
		goto cleanup;
	}

	fprintf(stderr, "%s: %s@%s is open\n", argv[0], server_port, server_addr);

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

