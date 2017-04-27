#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include <stdio.h>



#include"packet.h"
#include"common.h"

#define STDIN_FD    0
#define RETRY  1500 //milli second 
#define CWND 10

int next_seqno=0;
int send_base=0;
int last_acked = 0;
int window_size = 1;
int unackPackets;

int sockfd, serverlen;
struct sockaddr_in serveraddr;
struct itimerval timer; 
tcp_packet *sndpkt;
tcp_packet *recvpkt;
sigset_t sigmask;    

typedef struct node_packet{
    tcp_packet *packet;
    struct node_packet *nextPacket;
} node_packet;
node_packet *cwnd_begin = 0;   
node_packet *cwnd_end;


/* .pushes a packet onto the cwnd lifo. Because the
    cwnd is a fixed size window, it is our
    responsibility to ensure that we don't cross
    the cwnd boundary.
*/
void push_packet(tcp_packet *packet){
    if (cwnd_begin == 0){
            cwnd_begin = malloc(sizeof(node_packet));
            cwnd_begin-> packet = packet;
            cwnd_begin-> nextPacket = 0;
            cwnd_end = cwnd_begin;
    }
    else{
        cwnd_end -> nextPacket = malloc(sizeof(node_packet));
        cwnd_end = cwnd_end -> nextPacket;
        cwnd_end -> packet = packet;
        cwnd_end -> nextPacket = 0;
    }
    
    
}

/* pop frees the address pointed to by begin and reassigns it
    to the next element in LIFO.
    */
void pop_packet(){
    if (cwnd_begin == 0){
        return;
    }
    else{
        node_packet *temp = cwnd_begin;
        cwnd_begin = cwnd_begin -> nextPacket;
        free(temp);
        if (cwnd_begin != 0)
        VLOG(DEBUG, "...Current window head at %d", cwnd_begin->packet->hdr.seqno);
        return;
    }

}

void resend_packets(int sig)
{
    if (sig == SIGALRM)
    {
        //Resend all packets range between 
        //sendBase and nextSeqNum
        VLOG(INFO, "Timout happend");
        /*
        if(sendto(sockfd, sndpkt, sizeof(*sndpkt), 0, 
                    ( const struct sockaddr *)&serveraddr, serverlen) < 0)
        {
            error("sendto");
        }
        */
    }
}


void start_timer()
{
    sigprocmask(SIG_UNBLOCK, &sigmask, NULL);
    setitimer(ITIMER_REAL, &timer, NULL);
}


void stop_timer()
{
    sigprocmask(SIG_BLOCK, &sigmask, NULL);
}


/*
 * init_timer: Initialize timeer
 * delay: delay in milli seconds
 * sig_handler: signal handler function for resending unacknoledge packets
 */
void init_timer(int delay, void (*sig_handler)(int)) 
{
    signal(SIGALRM, resend_packets);
    timer.it_interval.tv_sec = delay / 1000;    // sets an interval of the timer
    timer.it_interval.tv_usec = (delay % 1000) * 1000;  
    timer.it_value.tv_sec = delay / 1000;       // sets an initial value
    timer.it_value.tv_usec = (delay % 1000) * 1000;

    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGALRM);
}
/* Sleep for given number of seconds. Can be interrupted by
 singnals.
*/
void waitFor (unsigned int secs) {
    unsigned int retTime = time(0) + secs;   // Get finishing time.
    while (time(0) < retTime);               // Loop until it arrives.
}


/* Resends all the unacknowledged packets.
*/
void releaseAllPackets(){
    int j = 1;
    node_packet *temp = cwnd_begin;
    VLOG(DEBUG, "Going to send packets");
    while (temp != 0){
        VLOG(DEBUG, "Sending packet %d with size %lu to %s", 
             j++, sizeof(*(temp -> packet)), inet_ntoa(serveraddr.sin_addr));


        if(sendto(sockfd, temp -> packet, TCP_HDR_SIZE+temp->packet->hdr.data_size, 0, 
                  ( const struct sockaddr *)&serveraddr, serverlen) < 0){
            error("sendto");
        }
        VLOG(DEBUG, "Seq number sent %d ", temp ->packet ->hdr.seqno);
        //waitFor(1);
            //pop_packet();
        temp = temp -> nextPacket;
    }

}


int main (int argc, char **argv)
{
    int portno, len, fileEnd = 0;
    int next_seqno;
    int i;
    char *hostname;
    char buffer[DATA_SIZE];
    FILE *fp;

    /* check command line arguments */
    if (argc != 4) {
        fprintf(stderr,"usage: %s <hostname> <port> <FILE>\n", argv[0]);
        exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);
    fp = fopen(argv[3], "r");
    if (fp == NULL) {
        error(argv[3]);
    }

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");


    /* initialize server server details */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serverlen = sizeof(serveraddr);

    /* covert host into network byte order */
    if (inet_aton(hostname, &serveraddr.sin_addr) == 0) {
        fprintf(stderr,"ERROR, invalid host %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(portno);

    assert(MSS_SIZE - TCP_HDR_SIZE > 0);

    //Stop and wait protocol

    init_timer(RETRY, resend_packets);
    next_seqno = 0;

    /* Start the control window.
     * We start by filling the control window with cwnd
     * number of packets. Once we have done that, we realease
     * all the packets into the UDP socket.
    */


    // . fill the cwnd buffer.]
    unackPackets = 0;
    for (i = 0; (i < CWND) && (fileEnd != 1); i++){
        len = fread(buffer, 1, DATA_SIZE, fp);
        if ( len <= 0){
            VLOG(INFO, "EOF File has been reached");
            sndpkt = make_packet(0);
            push_packet(sndpkt);
            unackPackets += 1;
            fileEnd = 1;
            continue;
        }
 
            

        send_base = next_seqno;
        next_seqno = send_base + len;
        sndpkt = make_packet(len);
        memcpy(sndpkt->data, buffer, len);
        sndpkt->hdr.seqno = send_base;
        push_packet(sndpkt);
        unackPackets+= 1;
    }
    

        
        
    
    releaseAllPackets();


    /* RDT transmission window in action.
     * Once we have released all the initial packets,
     * we get into a loop which runs until the whole file
     * has been sent across the network.
     *
     * The loop is controlled by the number of unacked
     * packets. There are three main actions at work here.
     * 1. An ACK is received.
     * 1.1. The ACK is not a double ACK, we slide the window.
     * 1.2. If it's a double ACK, we do fast retransmit.
     * 2. No ACK is received, but the timer event fires
     * SIGALRM signal. Then we resend all the packets in the 
     * cwnd buffer.
     */
    int doubleACKS = 0;
    do{ 
        if(recvfrom(sockfd, buffer, MSS_SIZE, 0,
            (struct sockaddr *) &serveraddr, (socklen_t *)&serverlen) < 0){
                error("recvfrom");
        }

        recvpkt = (tcp_packet *)buffer;
        VLOG(DEBUG, "Received packet with ack no. %d", recvpkt->hdr.ackno);

        if (recvpkt->hdr.ackno < last_acked){
            VLOG(DEBUG, "DOUBLE ACK seqno.");
            doubleACKS++;
            if (doubleACKS == 3){
                doubleACKS = 0;
                releaseAllPackets();
            }
            continue;
        }

        last_acked = recvpkt -> hdr.ackno;
        while((cwnd_begin !=0) && cwnd_begin->packet->hdr.seqno != last_acked){

            pop_packet();
            --unackPackets;
            VLOG(DEBUG, "Current unACK packets %d", unackPackets);

            if (fileEnd == 1){
                continue;
            }
        
            len = fread(buffer, 1, DATA_SIZE, fp);
            if (len <= 0){
                VLOG(INFO, "End Of File has been reached");
                sndpkt = make_packet(0);
                push_packet(sndpkt);
                fileEnd = 1;
            }

            else{
                send_base = next_seqno;
                next_seqno = send_base + len;
                sndpkt = make_packet(len);
                memcpy(sndpkt->data, buffer, len);
                sndpkt->hdr.seqno = send_base;
                push_packet(sndpkt);
            }
            if(sendto(sockfd, cwnd_end -> packet, TCP_HDR_SIZE+cwnd_end->packet->hdr.data_size, 0, 
                  ( const struct sockaddr *)&serveraddr, serverlen) < 0){
            error("sendto");
            }
            VLOG(DEBUG, "Seq number sent %d ", cwnd_end ->packet ->hdr.seqno);
            unackPackets += 1;

            //waitFor(1);
        }   
            
    } while (unackPackets != 0);
    
    return 0;
}



