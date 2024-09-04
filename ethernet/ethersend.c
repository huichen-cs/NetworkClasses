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
 * send a short message via a given Ethernet interface.
 *
 * usage:
 *
 *   ethersend -d dst -m msg -i inf
 *
 * where dst is desintation address in the standard hex-digits-and-colons
 * notation, msg is the message to be sent, and inf is the interface name
 *
 * The program uses raw socket and requires (1) effective UID 0 (root)
 * privilege or (2) CAP_NET_RAW capability. 
 *
 * (1) for instance, to run the program on interface eth0 with root privilege
 *     using sudo,
 *
 *     sudo ./ethersend -d 11:22:33:44:55:66 -m "Hello, World" -i eth0
 *
 * (2) for instance, to run the program with CAP_NET_RAW capability, first give
 *     the program the effective and permissible capability of CAP_NET_RAW,
 *
 *     sudo setcap cap_net_raw+ep ./ethersend
 *
 *     then run the program on interface eth0
 *
 *     ./ethersend -d 11:22:33:44:55:66 -m "Hello, World" -i eth0
 *
 *     Note that to be able to set capabilities for a file, the filesystem
 *     that the file resides must support capabilities (see capabilities(7)). 
 */

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_arp.h> 
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"

/* protocol number is a todo */
/*
#define CSCI445_ETHER_SEND_RECV_RPOTO 60001
*/

/* #define USE_BIND_AND_SEND */
/* #define VERBOSE */


struct ether_frame {
    struct ether_header hdr; /* declared in net/ethernet.h and already packed */
    char payload[ETHERMTU];
} __attribute__ ((__packed__));

struct cmd_line_args {
    char *dst;
    char *inf;
    char *msg;
};

static void usage();
static int parse_cmd_line(int argc, char *argv[], struct cmd_line_args *args);
static int build_ether_frame(int sockfd, struct cmd_line_args *args, struct ether_frame *frame, int *frame_len);
static int build_sockaddr_ll(int sockfd, struct cmd_line_args *args, struct sockaddr_ll *addr);
static int get_if_ether_addr(int sockfd, char *inf, u_int8_t *hostaddr);
static int get_if_index(int sockfd, char *inf);

int main(int argc, char *argv[])
{
    struct cmd_line_args args;
    struct ether_frame frame;
    struct sockaddr_ll sll_addr;
    int sockfd, frame_len;

    /* parse command line */
    if (!parse_cmd_line(argc, argv, &args)) {
        usage();
        return 1;
    }

    /* open a raw packet socket */
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd == -1) {
        fprintf(stderr, 
            "ERROR: calling socket(AF_PACKET, SOCK_RAW, ... %s.\n",
            strerror(errno));
        close(sockfd);
        return 1;
    }

    /* build ethernet frame */
    if (!build_ether_frame(sockfd, &args, &frame, &frame_len)) {
        fprintf(stderr, "ERROR: failed to build frame\n");
        close(sockfd);
        return 1;
    }

    /* build ethernet address structure */
    if (!build_sockaddr_ll(sockfd, &args, &sll_addr)) {
        fprintf(stderr, "ERROR: failed to build sockaddr\n");
        close(sockfd);
        return 1;
    }
#if defined VERBOSE 
    printf("dumping socket address structure:\n");
    dumpbuf((char *)&sll_addr, sizeof(sll_addr));
#endif


#if defined USE_BIND_AND_SEND
    /*  
     *  According to packet(7), for bind only sll_protocol and sll_ifindex are
     *  used. However, the test indicates that sll_family needs to be filled
     *  as well.
     *
     *  We can use sll_addr to bind the socket with the address of the
     *  network interface with index sll_ifindex, although sll_addr is to 
     *  serve as the destination address and the sll_ifindex is the index
     *  of the network interface that has the source address. 
     **/
    if (bind(sockfd, (struct sockaddr *)&sll_addr, sizeof(sll_addr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind.\n");
        close(sockfd);
        return 1;
    }

    /* 
     * we can now use send(...) since the process is bound to the ethernet 
     * interface, although we can still use sendto(...) if the dest_addr
     * argument is properly filled. see comment in  build_sockaddr_ll()...
     * */
    if (send(sockfd, 
            &frame, 
            frame_len, 
            0) == -1) {
        fprintf(stderr, "ERROR: failed to send(...): %s\n", strerror(errno));
    }
    printf("Sent: %s\n", args.msg);

#if defined VERBOSE 
    printf("dumping frame sent:\n");
    dumpbuf((char *)&frame, frame_len);
#endif

#else

    if (sendto(sockfd, 
            &frame, 
            frame_len, 
            0,
            (struct sockaddr *)&sll_addr, 
            sizeof(sll_addr)) == -1) {
        fprintf(stderr, "ERROR: failed to sendto(...): %s\n", strerror(errno));
    }
    printf("Sent: %s\n", args.msg);

#if defined VERBOSE 
    printf("dumping frame sent:\n");
    dumpbuf((char *)&frame, frame_len);
#endif
#endif
    

    close(sockfd);
    return 0;
}

static int parse_cmd_line(int argc, char *argv[], struct cmd_line_args *args)
{
    memset(args, '\0', sizeof(*args));

    argc --;     
    argv ++;
    while (argc && *argv[0] == '-') {
        if (!strcmp(*argv, "-d")) {
            if (*(argv + 1)[0] == '-') {
                return 0;
            }
            args->dst = *(argv + 1);
        } else if (!strcmp(*argv, "-m")) {
            if (*(argv + 1)[0] == '-') {
                return 0;
            }
            args->msg = *(argv + 1);
        } else if (!strcmp(*argv, "-i")) {
            if (*(argv + 1)[0] == '-') {
                return 0;
            }
            args->inf = *(argv + 1);
        }

        argc -= 2;
        argv += 2;
    }

    if (!args->dst || !args->msg || !args->inf) 
        return 0;

    return 1;
}

static int build_ether_frame(int sockfd, 
    struct cmd_line_args *args, struct ether_frame *frame, int *frame_len)
{
    int padding_len = 0, payload_len = 0;

    if (get_if_ether_addr(sockfd, args->inf, (frame->hdr).ether_shost) == 0) {
        fprintf(stderr, 
            "ERROR: cannot build frame without destination address\n");
        return 0;
    }

    if (ether_aton_r(args->dst, 
            (struct ether_addr *)&((frame->hdr).ether_dhost)) == NULL) {
        fprintf(stderr, 
            "WARN: %s is not in valid hex-digits-and-colons format\n", 
            args->dst);
        return 0;
    }
    
    payload_len = strlen(args->msg);

    frame->hdr.ether_type = htons(payload_len);

    /*
     * In the following, we fill the palyload length (or type of frame) field
     * with payload length. When multiple sender programs, other than ethersend
     * on the same host transmit ethernet frames, the receiver (etherrecv) will
     * receive frames from multiple sender and cannot differentiate which
     * sender program the frame is orginated although it does know which host
     * the frame is originated using the source ethernet address as a cue. 
     *
     * To overcome the problem, we can design a demultiplexer mechanism that
     * fill the type of frame field with a protocol number (or the type of
     * frame). The program ethersend will be assigned a unique protocol number
     * of its own, we can now use it to tell whether the frame is from this
     * program (ethersend) or other programs. 
     *
     * The protocol number is a To-Do or an exercise of your own. 
     *
     * In addition, even with the protocol number, you cannot differentiate
     * which process of an ethersend program from which a frame is originated 
     * if you run multiple instances of program ethersend. 
     *
     */
    if (payload_len > ETHERMTU) {
        fprintf(stderr, "WARN: message is truncated.\n"); 
        memcpy(frame->payload, args->msg, ETHERMTU);
        payload_len = ETHERMTU;
    } else {
        payload_len = strlen(args->msg);
        memcpy(frame->payload, args->msg, payload_len);
        padding_len = ETH_ZLEN - sizeof(frame->hdr) - payload_len;
        if (padding_len > 0) {
            memset(frame->payload + payload_len, '\0', padding_len);
            payload_len += padding_len;
        }
    }

    *frame_len = sizeof(frame->hdr) + payload_len;

    return 1;
}

static int build_sockaddr_ll(int sockfd, 
    struct cmd_line_args *args, struct sockaddr_ll *addr)
{
    int ifindex;

    memset(addr, '\0', sizeof(*addr));
    addr->sll_family = AF_PACKET;

    ifindex = get_if_index(sockfd, args->inf);
    if (ifindex == -1) {
        fprintf(stderr, "ERROR: could not obtain interface index\n");
        return 0;
    }
    addr->sll_ifindex = ifindex;


#if defined USE_BIND_AND_SEND
    /*  
     *  According to packet(7), for bind only sll_protocol and sll_ifindex are
     *  used. However, the test indicates that sll_family needs to be filled
     *  as well.
     *
     *  We can use sll_addr to bind the socket with the address of the
     *  network interface with index sll_ifindex, although sll_addr is to 
     *  serve as the destination address and the sll_ifindex is the index
     *  of the network interface that has the source address. 
     **/
    /* just to provide a comment block. Nothing else needs to done here */
#else
    /* 
     * When you send packets it is enough to specify sll_family, sll_addr, 
     * sll_halen, sll_ifindex.  The other fields should be 0. 
     *
     * See packet(7)
     * */

    if (get_if_ether_addr(sockfd, args->inf, addr->sll_addr) == 0) {
        fprintf(stderr, 
                "ERROR: interface %s is not a standard Ethernet interface\n",
                args->inf);
        return 0;
    }

    addr->sll_halen = ETH_ALEN;
#endif

    return 1;
}

static int get_if_ether_addr(int sockfd, char *inf, u_int8_t *hostaddr)
{
    struct ifreq ifr;

    ifr.ifr_name[IFNAMSIZ-1] = '\0';
    strncpy(ifr.ifr_name, inf, IFNAMSIZ-1);

    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == -1) {
        fprintf(stderr, "ERROR: calling ioctl(sockfd, SIOCGIFHWADDR, ...): %s\n",
            strerror(errno));
        return 0;
    }

    if (ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER) {
        memcpy(hostaddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
        return 1;
    } else {
        return 0;
    }
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

static void usage() 
{
    fprintf(stderr, "Usage: ethersend -d dst -m msg -i inf\n");
}



