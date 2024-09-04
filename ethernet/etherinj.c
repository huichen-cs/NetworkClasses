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
 * Inject Ethernet frames into network via a network inface. 
 *
 * The program reads payload of the Ethernet frames from a message as a command
 * line argument or from a file whose name is provided as a command line
 * argument. 
 *
 * In addition, users must provide source Ethernet address,  destination
 * Ethernet address, and network interface name via the command line arguments.
 * The source Ethernet address needs not to match the Ethernet address of the
 * network interface. Perhaps, it is useful to check out literature on the
 * "man-in-the-middle" attack with regard to this purposeful design of this
 * program.
 *
 * Usage:
 *
 *   etherinj -s src -d dst [-f file | -m msg] <interface>
 *
 * The program uses raw socket and requires (1) effective UID 0 (root)
 * privilege or (2) CAP_NET_RAW capability. 
 *
 * (1) for instance, to run the program to inject a frame contains payload
 *     "Hello, World" with source address as 11:22:33:44:55:66 and 
 *     destination address as 11:22:33:44:55:77 on interface eth0 with root
 *     privilege using sudo,
 *
 *     sudo ./etherinj -s 11:22:33:44:55:66 -d 11:22:33:44:55:77 -m "Hello, World" eth0
 *
 * (2) for instance, to run the program with CAP_NET_RAW capability, first give
 *     the program the effective and permissible capability of CAP_NET_RAW,
 *
 *     sudo setcap cap_net_raw+ep ./etherinj
 *
 *     then run the program on interface eth0
 *
 *     ./etherinj -s 11:22:33:44:55:66 -d 11:22:33:44:55:77 -m "Hello, World" eth0
 *
 *     Note that to be able to set capabilities for a file, the filesystem that
 *     the file resides must support capabilities (see capabilities(7)). 
 *
 * (3) for instance, to run program to inject frames based on content of a
 *     given file, myfile.txt, in this case, 
 *   
 *     sudo ./etherinj -s 11:22:33:44:55:66 -d 11:22:33:44:55:77 -f myfile.txt eth0
 *
 *     or 
 *
 *     ./etherinj -s 11:22:33:44:55:66 -d 11:22:33:44:55:77 -m "Hello, World" eth0
 *
 *     if the program has CAP_NEW_RAW capability set.
 *
 *     in the above example, if the given file is large than maximum payload
 *     size of a frame, the content of the file is sent in a few frames. 
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <netpacket/packet.h>
#include <netinet/ether.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sighandler.h"
#include "buffer.h"

enum msgtype {UNDEFINED = 0, SENDFILE = 1, SENDMSG = 2};

struct filemsgstate {
    int filefd;
    int msglen;
};

struct strmsgstate {
    char *msg;
    int msglen;
    int bufpos;
};

struct msgsource {
    int type;
    struct msgstate *ms;
    int (*init)(void *ms, char *msg);
    int (*copymsgpart)(void *ms, char *buf, const int len);
    int (*cleanup)(void *ms);
    int (*getmsglen)(void *ms);
};

static void parse_cmd_line_arguments(int argc, char *argv[], 
        char **src, char **dst, enum msgtype *opt_fm, char **msg, char **intf);
static void cleanup(int s);
static void usage();
static int sendwholemsg(const int sockfd, const char *intf,
        char *src, char *dst, char *msg, const enum msgtype opt_fm);
static int strmsginit(void *ms, char *msg);
static int strmsgcppart(void *ms, char *buf, const int len);
static int strmsggetlen(void *ms);
static int filemsginit(void *ms, char *filename);
static int filemsgcppart(void *ms, char *buf, const int len);
static int filemsgcleanup(void *ms);
static int filemsggetlen(void *ms);


static int sockfd = -1,
           filefd = -1;

int main(int argc, char *argv[])
{
    char *src, *dst, *msg, *intf;
    enum msgtype opt_fm = UNDEFINED;

    /* handling CTRL-C */
    setupsignal(SIGINT, cleanup); 

    /* check usage */
    if (argc < 8) {
        usage();
        exit(0);
    }

    /* parse command line arguments */
    parse_cmd_line_arguments(argc, argv, &src, &dst, &opt_fm, &msg, &intf);

    fprintf(stderr, "Transmitting src = [%s] -> dst = [%s] "
            "via Interface = [%s] for %s = [%s]\n",
            src, dst, intf, opt_fm==SENDMSG?"msg":"file", msg);

    /* open a raw packet socket */
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd == -1) {
        perror("socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL) failed");
        exit(1);
    }

    /* send the whole message in one or more ethernet frames */
    sendwholemsg(sockfd, intf, src, dst, msg, opt_fm);

    /* clean house before exiting */
    close(sockfd);
    sockfd = -1;

    return 0;
}

static void parse_cmd_line_arguments(int argc, char *argv[], 
        char **src, char **dst, enum msgtype *opt_fm, char **msg, char **intf) 
{
    /* parse command line arguments */
    argc --;
    argv ++;
    while (argc && *argv[0] == '-') {
        if (!strcmp(*argv, "-s")) {
            if (*(argv+1)[0] == '-') {
                usage();
                exit(1);
            }
            *src = *(argv + 1);
        }
        else if (!strcmp(*argv, "-d")) {
            if (*(argv+1)[0] == '-') {
                usage();
                exit(1);
            }
            *dst = *(argv + 1);
        }
        else if (!strcmp(*argv, "-f")) {
            if (*opt_fm == SENDMSG) {
                fprintf(stderr,
                        "Usage: -f and -m cannot occure at the same time\n");
                exit(1);
            }
            if (*(argv+1)[0] == '-') {
                usage();
                exit(1);
            }
            *msg = *(argv + 1);
            *opt_fm = SENDFILE;
        }
        else if(!strcmp(*argv, "-m")) {
            if (*opt_fm == SENDFILE) {
                fprintf(stderr,
                        "Usage: -f and -m cannot occure at the same time\n");
                exit(1);
            }
            if (*(argv+1)[0] == '-') {
                usage();
                exit(1);
            }
            *msg = *(argv + 1);
            *opt_fm = SENDMSG;
        }
        else {
            usage();
            exit(1);
        }

        argc -= 2;
        argv += 2;
    }

    *intf = *argv;
}


static void usage() 
{
    fprintf(stderr, 
            "Usage: etherinj -s src -d dst [-f file | -m msg] <interface>\n");
}

static void cleanup(int s __attribute__((unused)))  
{
    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }

    if (filefd >= 0) {
        close(filefd);
        filefd = -1;
    }
}

static int sendwholemsg(const int sockfd, const char *intf, 
        char *src, char *dst, char *msg, const enum msgtype opt_fm)
{
    struct sockaddr_ll sll_addr_dst;      /* see packet(7)    */
    struct ifreq ifr;                     /* see netdevice(7) */
    struct ether_addr ethersrc, 
                      etherdst;           /* net/ethernet.h   */
    struct msgsource msgsrc;
    char *bufpos, 
         *buf;
    short msglen, 
          payloadlen, 
          netlen, 
          minpayloadlen = ETH_ZLEN - ETH_HLEN, 
          bufsize;

    /* 
     * convert hex-digits-and-colons notation into binary data 
     * in network byte order
     * */
    if (ether_aton_r(src, &ethersrc) == NULL) {
        fprintf(stderr, "ether_aton_r(src ...) failed: Ethernet address must "
                           "be in the standard hex-digits-and-colons notation");
        exit(1);
    }

    if (ether_aton_r(dst, &etherdst) == NULL) {
        fprintf(stderr, "ether_aton_r(dst ...) failed: Ethernet address must "
                           "be in the standard hex-digits-and-colons notation");
        exit(1);
    }

    /* 
     * When  you  send  packets  it is enough to specify sll_family, sll_addr,
     * sll_halen, sll_ifindex.  The other fields should be 0.
     *
     * see packet(7)
     *
     * Note: although this sockaddr_sll structure is to be supplied to
     * the sendto(...) API as dest_addr and sll_addr is to be filled with
     * the destination address, sll_ifindex has to be the index of the 
     * interface via which the frame is transmitted -- that is to say,
     * one may interpret that sll_addr is the destination address
     * while sll_ifindex is the index of the network interface that
     * has the source address.
     *
     * */
    memset(&sll_addr_dst, 0, sizeof(sll_addr_dst));
    sll_addr_dst.sll_family = AF_PACKET;
    
    /* 
     * obtain injecting interface's address, and use it to set sll_addr 
     * 
     * note that one may obtain hardware address of a network interface 
     * via its name using an ioctl call,
     *
     * ioctl(sockfd, SIOCGIFHWADDR, &ifr)
     *
     * for more, see netdevice(7) that states,
     *
     * Get  or  set  the hardware address of a device using ifr_hwaddr.  The
     * hardware address is specified in a struct sockaddr.  sa_family
     * contains  the ARPHRD_* device type, sa_data the L2 hardware address
     * starting from byte 0.  Setting the hardware address is a privileged
     * operation.
     *
     *
     * */
    ifr.ifr_name[IFNAMSIZ-1] = '\0';
    strncpy(ifr.ifr_name, intf, IFNAMSIZ-1);
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == -1) {
        perror("ioctl(sockfd, SIOCGIFHWADDR, &ifr)");
        exit(1);
    }
    memcpy(sll_addr_dst.sll_addr, &(ifr.ifr_hwaddr.sa_data), sizeof(sll_addr_dst.sll_addr));

    /* set sll_halen */
    if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
        fprintf(stderr, 
                "WARN: interface %s is not a standard Ethernet device and "
                "this program may not function.\n", intf);
    }
    sll_addr_dst.sll_halen = ETH_ALEN;

    /* 
     * obtain injecting interface's interface index, and use it to set
     * sll_ifindex 
     * 
     * note that one may obtain a network interface's index via its name using
     * an ioctl call,
     *
     * ioctl(sockfd, SIOCGIFINDEX, &ifr)
     *
     * for more, see netdevice(7)
     *
     * */
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1) {
        perror("ioctl(sockfd, SIOCGIFINDEX, &ifr)");
        exit(1);
    }
    sll_addr_dst.sll_ifindex = ifr.ifr_ifindex;
    /* dumpbuf((char *)&sll_addr_dst, sizeof(sll_addr_dst)); */


    /* before we proceed, we need to obtain MTU to determine buffer size */
    ifr.ifr_addr.sa_family = AF_PACKET;
    safe_strncpy(ifr.ifr_name, intf, IFNAMSIZ);
    if (ioctl(sockfd, SIOCGIFMTU, &ifr) != 0) {
        perror("ioctl(sockfd, SIOCGIFMTU, &ifr)");
        exit(1);
    }
    bufsize = ifr.ifr_mtu + ETHER_HDR_LEN;
    if ((buf = malloc(bufsize)) == NULL) {
        fprintf(stderr, "malloc(%d): insufficient memory\n", bufsize);
        exit(1);
    }


    /* 
     * fill destination and source address fields of Ethernet frames
     * to be sent 
     * */
    bufpos = buf;
    memcpy(bufpos, &etherdst, ETH_ALEN);
    bufpos += ETH_ALEN;
    memcpy(bufpos, &ethersrc, ETH_ALEN);
    bufpos += ETH_ALEN;

    /*
     * initialize message handler based on message type, a message from
     * command line, or the content of a file specified in command line
     * */
    msgsrc.type = opt_fm;
    switch(opt_fm) {
    case SENDMSG:
        msgsrc.ms = malloc(sizeof(struct strmsgstate));
        msgsrc.init = strmsginit;
        msgsrc.copymsgpart = strmsgcppart;
        msgsrc.cleanup = NULL;
        msgsrc.getmsglen = strmsggetlen;
        break;
    case SENDFILE:
        msgsrc.ms = malloc(sizeof(struct filemsgstate));
        msgsrc.init = filemsginit;
        msgsrc.copymsgpart = filemsgcppart;
        msgsrc.cleanup = filemsgcleanup;
        msgsrc.getmsglen = filemsggetlen;
        break;
    case UNDEFINED:
        fprintf(stderr, "Case UNDEFINED cannot be handled\n");
        exit(1);
    }

    msgsrc.init(msgsrc.ms, msg);
    msglen = msgsrc.getmsglen(msgsrc.ms);

    while (msglen > 0) { 
        /*
         * compute frame payload length 
         * */
        if (msglen > ETH_DATA_LEN) {
            payloadlen = ETH_DATA_LEN;
            msglen -= ETH_DATA_LEN;
        } else {
            payloadlen = msglen;
            msglen = 0;
        }

        /*
         * fill frame payload 
         * */
        msgsrc.copymsgpart(msgsrc.ms, bufpos+2, payloadlen);

        /*
         * pad the frame with 0's when the frame's payload is too
         * small to meet frame's minimum length requirement, i.e.,
         * MIN_ZLEN without including CRC
         * */
        if (payloadlen < minpayloadlen) {
            memset(bufpos+2+payloadlen, '\0', minpayloadlen - payloadlen);
            payloadlen = minpayloadlen;
        }

        /*
         *  fill frame length/type field with payload length
         * */
        netlen = htons(payloadlen);
        memcpy(bufpos, &netlen, 2);

        /*
         * send the frame 
         * */
        if (sendto(sockfd, buf, payloadlen+ETH_HLEN, 0, 
                (struct sockaddr*)&sll_addr_dst, sizeof(sll_addr_dst)) < 0) {
            perror("sendto(sockfd ...");
            exit(1);
        }
        /* dumpbuf((char *)&sll_addr_dst, sizeof(sll_addr_dst)); */

        /* 
         * dump the frame to stdout
         * */
        fprintf(stdout, 
                "Frame transmitted (Payload Length = [%d]): \n", payloadlen);
        dumpbuf(buf, payloadlen+ETH_HLEN);
    }

    /* clean house */
    if (msgsrc.cleanup)
        msgsrc.cleanup(msgsrc.ms);
    free(buf);

    return 0;
}

static int strmsginit(void *ms, char *msg)
{
    struct strmsgstate *handler = (struct strmsgstate*)ms;
    handler->msg = msg;
    handler->bufpos = 0;
    handler->msglen = strlen(msg);

    return 0;
}

static int strmsgcppart(void *ms, char *buf, const int len) 
{
    struct strmsgstate *handler;
    handler = (struct strmsgstate*)ms;

    memcpy(buf, handler->msg+handler->bufpos, len);
    handler->bufpos += len;

    return 0;
}

static int strmsggetlen(void *ms)
{
     struct strmsgstate *handler = (struct strmsgstate*)ms;
     return handler->msglen;
}

static int filemsginit(void *ms, char *filename)
{
    struct stat fs;
    struct filemsgstate *handler = (struct filemsgstate*)ms;

    handler->filefd = open(filename, O_RDONLY);
    if (handler->filefd == -1) {
        perror("open(msg, O_RDONLY ...):");
        return 1;
    }
    
    if (fstat(handler->filefd, &fs) == -1) {
        perror("fstat(filefd, ...):");
        return 1;
    }
    handler->msglen = fs.st_size;

    return 0;
}

static int filemsgcppart(void *ms, char *buf, const int len) 
{
    struct filemsgstate *handler = (struct filemsgstate*)ms;

    if (read(handler->filefd, buf, len) != len) {
        perror("read(filefd, ...):");
        return 1;
    }
    return 0;
}

static int filemsgcleanup(void *ms)
{
    struct filemsgstate *handler = (struct filemsgstate*)ms;

    close(handler->filefd);    
    return 0;
}

static int filemsggetlen(void *ms)
{
     struct filemsgstate *handler = (struct filemsgstate*)ms;
     return handler->msglen;
}
