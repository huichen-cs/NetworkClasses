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
 * receive a short message from Ethernet.
 *
 * usage: 
 *
 *      etherrecv -s src -i intf
 *
 * where src is source address in the standard hex-digits-and-colons
 * notation, and intf is the interface name
 *
 * The program uses raw socket and requires (1) effective UID 0 (root)
 * privilege or (2) CAP_NET_RAW capability. 
 *
 * (1) for instance, to run the program on interface eth0 with root privilege
 *     using sudo,
 *
 *     sudo ./etherrecv -s 11:22:33:44:55:77 -i eth0
 *
 * (2) for instance, to run the program with CAP_NET_RAW capability, first give
 *     the program the effective and permissible capability of CAP_NET_RAW,
 *
 *     sudo setcap cap_net_raw+ep ./etherrecv
 *
 *     then run the program on interface eth0
 *
 *     ./etherrecv -s 11:22:33:44:55:77 -i eth0
 *
 *     Note that to be able to set capabilities for a file, the filesystem
 *     that the file resides must support capabilities (see capabilities(7)). 
 */

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "buffer.h"
#include "sighandler.h"

/* protocol number is a todo */
/*
 * #define CSCI445_ETHER_SEND_RECV_RPOTO 60001
 * */

struct cmd_line_args {
    char *src;
    char *inf;
};

struct ether_frame {
    struct ether_header hdr; /* declared in net/ethernet.h and already packed */
    char payload[ETHERMTU];
} __attribute__ ((__packed__));

static void usage();
static void cleanup(int s);
static int parse_cmd_line(int argc, char *argv[], struct cmd_line_args *args);
static int build_sockaddr_ll(int sockfd, 
        struct cmd_line_args *args, struct sockaddr_ll *addr);
static int get_if_index(int sockfd, char *inf);
static void print_payload(struct ether_frame *frame);

static int sockfd = -1; 

int main(int argc, char *argv[]) 
{
    struct cmd_line_args args;
    struct sockaddr_ll sll_addr;
    struct ether_frame frame;
    socklen_t addr_len; 
    ssize_t num_recv;
    int ether_type;

    /* handle CTRL-C */
    setupsignal(SIGINT, cleanup);

    /* parse command line arguments */
    if (!parse_cmd_line(argc, argv, &args)) {
        usage();
        exit(1);
    }

    /* open a raw packet socket */
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd == -1) {
        fprintf(stderr, 
            "ERROR: calling socket(AF_PACKET, SOCK_RAW ...): %s\n", 
            strerror(errno));
        exit(1);
    }
    
    /* build sockaddr_ll structure */
    build_sockaddr_ll(sockfd, &args, &sll_addr);

    /* receive frames */
    fprintf(stderr, "Waiting for a frame to arrive ...\n");
    while(1) {
        addr_len = sizeof(sll_addr);
        num_recv = recvfrom(sockfd, 
                            &frame, 
                            sizeof(frame), 
                            0, 
                            (struct sockaddr *)&sll_addr, 
                            &addr_len);

        if (num_recv == -1) {
            fprintf(stderr,
                    "Error: recvfrom(...) return error: %s\n",
                    strerror(errno));
            continue;
        }
        
        ether_type = ntohs(frame.hdr.ether_type);

        /* 
         * Below is a simple method to determine if the frame is from program
         * ethersend. 
         *
         * This may not match sender (ethersend). To avoid receiving irrelevant
         * message, we'd better to define a protocol number, fill the
         * ether_type field type at the sender, and add a header to the payload
         * packet. The header can have a field of length. 
         *
         * See comment in program ethersend 
         * */

        if (ether_type <= ETHERMTU) {
            print_payload(&frame);
            fprintf(stderr, 
                    "INFO: received %zu bytes from %s\n", num_recv, args.inf);
            dumpbuf((char *)&frame, num_recv);
            fprintf(stderr, "Waiting for a frame to arrive ...\n");
        }
    }

    close(sockfd);
    return 0;
}

static int parse_cmd_line(int argc, char *argv[], struct cmd_line_args *args)
{
    memset(args, '\0', sizeof(*args));

    argc --;     
    argv ++;
    while (argc && *argv[0] == '-') {
        if (!strcmp(*argv, "-s")) {
            if (*(argv + 1)[0] == '-') {
                return 0;
            }
            args->src = *(argv + 1);
        } else if (!strcmp(*argv, "-i")) {
            if (*(argv + 1)[0] == '-') {
                return 0;
            }
            args->inf = *(argv + 1);
        }

        argc -= 2;
        argv += 2;
    }

    if (!args->src || !args->inf) 
        return 0;

    return 1;
}

static void usage() 
{
    fprintf(stderr, "Usage: etherrecv -s src -i intf\n");
}

static void cleanup(int s __attribute__((unused)))
{
    fflush(stderr);
    fflush(stdout);
    fprintf(stderr, "\nUser pressed CTRL-C. Exiting ...\n");
    if (sockfd >= 0) close(sockfd);
    exit(0);
}

static void print_payload(struct ether_frame *frame) 
{
    int len = ntohs(frame->hdr.ether_type), i;
    printf("Received: ");
    for (i = 0; i < len; i ++)
        putchar((int)(frame->payload[i]));
    putchar('\n');
}

static int build_sockaddr_ll(int sockfd, 
    struct cmd_line_args *args, struct sockaddr_ll *addr)
{
    int ifindex;
    struct ether_addr ethaddr;

    memset(addr, '\0', sizeof(struct sockaddr_ll));

    addr->sll_family = AF_PACKET;

    addr->sll_protocol = htons(ETH_P_ALL);

    ifindex = get_if_index(sockfd, args->inf);
    if (ifindex == -1) {
        fprintf(stderr, "ERROR: could not obtain interface index\n");
        return 0;
    }
    addr->sll_ifindex = ifindex;


    if (ether_aton_r(args->src, &ethaddr) == NULL) {
        fprintf(stderr, 
            "WARN: %s is not in valid hex-digits-and-colons format\n", 
            args->src);
        return 0;
    }
    memcpy(addr->sll_addr, &ethaddr, sizeof(ethaddr));

    addr->sll_halen = ETH_ALEN;
    return 1;
}

static int get_if_index(int sockfd, char *inf) 
{
    struct ifreq ifr;

    ifr.ifr_name[IFNAMSIZ-1] = '\0';
    strncpy(ifr.ifr_name, inf, IFNAMSIZ-1);

    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1) {
        fprintf(stderr, "ERROR: calling ioctl(sockfd, SIOCGIFINDEX, ...): %s\n",
            strerror(errno));
        return -1;
    }

    return ifr.ifr_ifindex;
}
