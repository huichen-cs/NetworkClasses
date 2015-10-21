/**
 Copyright (C) 2015 Hui Chen <huichen AT ieee DOT org>
 
 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Description */
/* 
 * capture ethernet frames on a given network interface. 
 *
 * usage: 
 *
 *     ethercap ifname
 *
 * The program uses raw socket and requires (1) effective UID 0 (root)
 * privilege or (2) CAP_NET_RAW capability. 
 *
 * (1) for instance, to run the program on interface eth0 with root privilege
 *     using sudo,
 *
 *     sudo ./ethercap eth0
 *
 * (2) for instance, to run the program with CAP_NET_RAW capability, first give
 *     the program the effective and permissible capability of CAP_NET_RAW,
 *
 *     sudo setcap cap_net_raw+ep ./ethercap
 *
 *     then run the program on interface eth0
 *
 *     ./ethercap eth0
 *
 *     Note that to be able to set capabilities for a file, the filesystem
 *     that the file resides must support capabilities (see capabilities(7)). 
 */

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sighandler.h"
#include "buffer.h"

static void cleanup(int s);
static void usage(char *prog);
static int get_if_index(const int sockfd, const char *ifname);
static void dump_physical_address(unsigned char halen, unsigned char *addr); 

static int sockfd = -1;
static char *buf = NULL; /* how big should the buffer be? */

int main(int argc, char *argv[])
{
    struct sockaddr_ll srcethaddr;  /* man 7 packet    */
    struct sockaddr_ll bindethaddr; /* man 7 packet    */
    struct packet_mreq mr;          /* man 7 packet    */
    struct ifreq ifr;               /* man 7 netdevice */
    int nbytes;
    socklen_t addrlen;

    /* how big should the buffer be?
     *
     * This can be determined by MTU. MTU can be obtained by an ioctl call:
     *
     *         ioctl(socket, SIOCGIFMTU, &ifr)
     *
     * where socket is a socket descriptor, ifr is of type struct ifreq.
     * More information can be found at manual page netdevice(7). Note that
     * one needs to give ifr_name to invoke the ioctl call. If a program 
     * retrieves frames from more than one device, the maximum MTU among
     * the devices should be used. 
     *
     * In Linux, available network devices can be obtained in multiple
     * methods, commonly, 
     * (1) by walking through the /proc/net/dev file. 
     * (2) by calling ioctl(...) with request SIOCGIFCONF
     * (3) via rtnetlink socket
     *
     * MTU does not include header/trailer. For capturing Ethernet frames,
     * the buffer size should be set no less than 
     * 6 + 6 + 2 + MTU = ETHER_HDR_LEN + MTU.
     *
     * Question: how does IEEE 802.1Q affect the required buffer size?
     * */

    int bufsize;      /* how big should the buffer be? */
    int ifindex;      /* interface index               */

    setupsignal(SIGINT, cleanup);    /* capture CTRL-C */

    if (argc < 2) {
        usage(argv[0]);
        exit(1);
    }

    /* open a raw packet socket to capture all types of ethernet frames */
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0) {
        perror("socket(AF_PACKET, SOCK_RAW, ETH_P_ALL) failed");
        exit(1);
    }

    /* obtain interface index from interface name */
    if ((ifindex = get_if_index(sockfd, argv[1])) == -1) {
        fprintf(stderr, 
                "failed to obtain interface index for interface %s\n", 
                argv[1]);
        exit(1);
    }

    /* put the interface into promiscuous mode. man 7 packet */
    memset(&mr, 0, sizeof(mr));
    mr.mr_ifindex = ifindex;
    mr.mr_type =  PACKET_MR_PROMISC;
    if (setsockopt(sockfd, SOL_PACKET, 
            PACKET_ADD_MEMBERSHIP, (char *)&mr, sizeof(mr)) != 0) {
        perror("setsockopt(sockfd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, ...)");
        exit(1);
    }
    

    /* bind the socket to the interface. see bind(2) and packet(7) */
    memset(&bindethaddr, 0, sizeof(bindethaddr));
    bindethaddr.sll_family = AF_PACKET;
    bindethaddr.sll_protocol = htons(ETH_P_ALL);
    bindethaddr.sll_ifindex = ifindex;
    if (bind(sockfd, 
		(struct sockaddr*)&bindethaddr, sizeof(bindethaddr)) != 0) {
        perror("bind(sockfd, &bindethaddr, sizeof(bindethaddr))");
        exit(1);
    }

    /* obtain MTU. see netdevice(7) */
    ifr.ifr_addr.sa_family = AF_PACKET;
    safe_strncpy(ifr.ifr_name, argv[1], IFNAMSIZ);
    if (ioctl(sockfd, SIOCGIFMTU, &ifr) != 0) {
        perror("ioctl(sockfd, SIOCGIFMTU, &ifr)");
        exit (1);
    }

    /* allocate buffer */
    bufsize = ifr.ifr_mtu + ETHER_HDR_LEN;
    if ((buf = malloc(bufsize)) == NULL) {
        fprintf(stderr, "insufficient memory\n");
        exit(1);
    }

    /* begin capturing */
    memset(&srcethaddr, 0, sizeof(srcethaddr));
    while (1) {

        addrlen = sizeof(srcethaddr);
        nbytes = recvfrom(sockfd, buf, bufsize, 
                0, (struct sockaddr*)&srcethaddr, &addrlen);

        if (nbytes < 0) {
            perror("recv(sockfd ...) failed");
            exit (1);
        }
    
        /* dump captured frame */
        printf("Captured at interface: %s frame from ", argv[1]);
        dump_physical_address(srcethaddr.sll_halen, srcethaddr.sll_addr);
        printf("\n");
        dumpbuf(buf, nbytes);
        /**
         * flush the buffer so that we don't have to rely on stdbuf, as in
         *     sudo stdbuf -o 0 ./ethercap eth0 | tee frame_captured.txt
         */
        fflush(stdout);
    }

    /* remove the interface's promiscuous mode */
    setsockopt(sockfd, SOL_PACKET, 
            PACKET_DROP_MEMBERSHIP, (char *)&mr, sizeof(mr));

    free(buf);
    close(sockfd);

    return 0;
}


static void cleanup(int s __attribute__((unused))) 
{
    fflush(stderr);
    fflush(stdout);
    fprintf(stderr, "\nUser pressed CTRL-C. Exiting ...\n");
    if (sockfd >= 0) close(sockfd);
    if (buf != NULL) free(buf);
    exit(0);
}

static void usage(char *prog) 
{
    fprintf(stderr, "\nWrong usage. "
            "You must provie a valid network interface, e.g., \n\n"
            "%s eth0\n\n", prog);
}

static int get_if_index(const int sockfd, const char *ifname)
{
    struct ifreq ifr;

    safe_strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        return -1;
    }

    return ifr.ifr_ifindex;
}

static void dump_physical_address(unsigned char halen, unsigned char *addr) 
{
    int i;
    for (i = 0; i < halen - 1; i ++) {
        printf("%02x:", addr[i]);
    }
    printf("%02x", addr[halen - 1]);
}

