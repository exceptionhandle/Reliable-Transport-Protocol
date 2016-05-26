#include "../include/simulator.h"
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <string.h>
using namespace std;
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define deliver_data( AorB, data) tolayer5(AorB, data)
#define udt_send( AorB, packet) tolayer3(AorB, packet)
#define getPacket(x) *(sndpkt[x])

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
static int base;
static int nextseqnum;
static float RTT_Delay;
static int window;
static int totalJourneyTime;
static int DelaySameCount;
static vector<struct pkt *> sndpkt;
static int cap;
static int RTTaverage;
static int count;
inline int createChecksum(struct pkt packet){
    char *mssg = packet.payload;
    int localchecksum = packet.seqnum + packet.acknum;
    for(int i = 0; i < 20 && mssg[i] != '\0'; i++)
    {
        localchecksum += mssg[i];
    }
    return localchecksum;
}


inline struct pkt *make_pkt(int seq, struct msg message)
{
    struct pkt *packet = (struct pkt *)malloc(sizeof(struct pkt));
    (*packet).seqnum = seq;
    (*packet).acknum = -1;
    strcpy((*packet).payload,message.data);
    (*packet).checksum = createChecksum((*packet));
    return packet;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    struct pkt *packet = make_pkt(nextseqnum,message);
    sndpkt.push_back(packet);
    if(base==nextseqnum)
    {
      // stoptimer(0);
      starttimer(0,RTT_Delay);  
    }
    if(nextseqnum < base + window)
    {
       udt_send(0,getPacket(nextseqnum-1));
     //  starttimer(0,RTT_Delay);
    }
    nextseqnum++;
}

inline void checkRTT(struct pkt packet)
{
  totalJourneyTime = get_sim_time() - totalJourneyTime;
  if(DelaySameCount >= 5 && packet.acknum >= base && (packet.acknum - base + 1) * RTT_Delay < totalJourneyTime)
  {
      RTT_Delay = totalJourneyTime / (packet.acknum - base + 1);
      DelaySameCount = 1;
      cap++;
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
     if( createChecksum(packet) == packet.checksum && packet.acknum >= base )
     {
        int oldbase = base;
        if( base < packet.acknum + 1)
        {
           base = packet.acknum + 1;
           stoptimer(0);
        }
        for(int i = oldbase+window;i<nextseqnum && i<base+window;i++)
        {
            udt_send(0,getPacket(i-1));
        }
        starttimer(0,RTT_Delay);
     }
/*     if( createChecksum(packet) == packet.checksum && packet.acknum == base-1 )
     {
        stoptimer(0);
        A_timerinterrupt();
     }	*/
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    DelaySameCount++;
    int win = base + window;
    for(int i = base; i < win && i < nextseqnum; i++)
    {
       udt_send(0,getPacket(i-1));
    }
    starttimer(0,RTT_Delay);
}


/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
   window = getwinsize();
   base = 1;
   nextseqnum = 1;
   RTT_Delay = 14.0;
   DelaySameCount = 1;
   cap = 1;
   RTTaverage = 0;
   count = 0;

}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
static int B_expectedseqnum;

inline struct pkt *make_ack(int seq){
    struct pkt *packet = (struct pkt *)malloc(sizeof(struct pkt));
    packet->acknum = seq;
    packet->checksum = seq;
    packet->seqnum = 0;
    memset(packet->payload,'\0',20);
    return packet;
}
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    if(createChecksum(packet) == packet.checksum && packet.seqnum == B_expectedseqnum){
       deliver_data(1, packet.payload);
       struct pkt *B_packet = make_ack(B_expectedseqnum);
       udt_send(1, *B_packet);
       B_expectedseqnum++;
    }
    else if(createChecksum(packet) == packet.checksum && packet.seqnum < B_expectedseqnum)
    {
       struct pkt *B_packet = make_ack(B_expectedseqnum-1);
       udt_send(1, *B_packet);
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    B_expectedseqnum=1;
}
