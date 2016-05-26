#include "../include/simulator.h"

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

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
#include <vector>
#include <map>
#include <iostream>
#include <stdlib.h>
#include <string.h>
using namespace std;

static int base;
static int nextseqnum;
static float RTT_Delay;
static int window;
static int DelaySameCount;
static int B_base;
static int B_window;
static int totalJourneyTime;
static vector<struct node *> sndpkt;
static int cap;

/**************** Sender-side functions *****************/
#define unACK(x) (((sndpkt[x]->packet)->acknum) == -1)?true:false
#define getPacket(x) *((sndpkt[x])->packet)
#define markACK(x) (sndpkt[x]->packet)->acknum = true
#define newNode(x, y) new (struct node)(make_pkt(x, y))
#define validAck(ack) ( ack >= base && ack < (base + window))?true:false
#define setTime(seq) (sndpkt[seq])->startTime = get_sim_time()
#define getTime(seq) (sndpkt[seq])->startTime 
/************* Common functions for Sender and Reciever ******************/
#define validWin(x,b,w) (x >= b && x < (b + w))?true:false
#define storeInBuffr(x) recvdBuff[x.seqnum] = new (struct pkt)(x)
#define notcorrupt(p) (createChecksum(p) == p.checksum )?true:false
#define udt_send( AorB, packet) tolayer3(AorB, packet)
#define deliver_data( AorB, data) tolayer5(AorB, data)
#define ifSeqNotInBuff(buff, seq) (buff.find(seq) == buff.end())?true:false

struct node{
    struct pkt *packet;
    int startTime;
    node(struct pkt *pack)
    {
       packet = pack;
       int startTime = 0;
    }
};

inline int createChecksum(struct pkt packet){
    char *message = packet.payload;
    int localchecksum = packet.seqnum + packet.acknum;
    for(int i = 0; i < 20 && message[i] != '\0'; i++)
    {
        localchecksum += (int)message[i];
    }
    return localchecksum;
}

inline struct pkt *make_pkt(int seq, struct msg message)
{
    struct pkt *packet = new (struct pkt)();
    (*packet).seqnum = seq;
    (*packet).acknum = -1;
    strcpy((*packet).payload,message.data);
    (*packet).checksum = createChecksum((*packet));
    return packet;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
      sndpkt.push_back( newNode(nextseqnum, message) );
      /*********** Send if this the first unacknowledged message in the window ***********/
      if(validWin(nextseqnum, base, window))
      {
         setTime(nextseqnum);
         udt_send(0, getPacket(nextseqnum));
      }
// timer started at the base
      if(base == nextseqnum)
      {
         starttimer(0,RTT_Delay);

      }
      nextseqnum ++;
}

inline void checkRTT()
{
   int totalJourneyTime = get_sim_time() - (sndpkt[base]->startTime);
   if(DelaySameCount <= 2 && RTT_Delay < totalJourneyTime)
   {
       RTT_Delay = totalJourneyTime;
       DelaySameCount = 1;
       cap++;
   }
   else
   {
       DelaySameCount ++;
   }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
     if( notcorrupt(packet) && validAck(packet.acknum))
     {

        int seqnum = packet.acknum;
        markACK(seqnum);

        if(base == seqnum)
        {
           stoptimer(0);
           /*******Check the frequency of time interrupt occuring ****************/

           base = base + 1;
           for(base; base < nextseqnum; base++)
           {
              /*********** if new message included in window from the buffer then START TIMER *************/
              if(base+window < nextseqnum && unACK(base + window)){
                 udt_send(0,getPacket(base + window));
                 setTime(base+window);
              }
              if(unACK(base)){
                 /****************** if there is un acknowledged message start timer *****************/
                 int remainDelay = RTT_Delay - (get_sim_time() - (sndpkt[base]->startTime));
                 starttimer(0,remainDelay);
                 break;
              }

           }
        }
     }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    DelaySameCount = 1;
    int min = base, secmin = base;
    //find minimum time remaining in window
    for(int i = base ; i<base + window && i < nextseqnum; i++)
    {
        if(unACK(i)&&getTime(min) - getTime(i) > RTT_Delay)//ELAY
        {
     //       secmin = min;
     //       min = i;
            udt_send(0,getPacket(i)); 
            setTime(i);  
        }  
    }
    //if(RTT_Delay - (get_sim_time() - getTime( secmin))>0) 
    //   starttimer(0,RTT_Delay - (get_sim_time() - getTime(secmin)));
   // else
    //{
       starttimer(0,RTT_Delay );
    //   setTime(secmin);
    //}
    //setTime(min);
        

  //  for(int i = base;i<base+window&&i<nextseqnum;i++)
  //     udt_send(0,getPacket(base/
}


/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
   window = getwinsize();
   base = 0;
   nextseqnum = 0;
   RTT_Delay = 14.0;
   cap = 1;
   DelaySameCount = 1;
   totalJourneyTime = get_sim_time();

}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/*********** Buffer messages to deliver in sequence later to tolayer5 ***********/
static map<int,struct pkt *> recvdBuff;
map<int,struct pkt *>::iterator it;

/*********** make Ack packet to send back ***********/
inline void sndACK(struct pkt pckt)
{
   struct pkt *packet = new (struct pkt)();
   packet->seqnum = 0;
   packet->acknum = pckt.seqnum;
   packet->checksum = pckt.seqnum;
   packet->seqnum = 0;
   memset(packet->payload,'\0',20);
   udt_send(1, *packet);
}

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    int seq = packet.seqnum;
    bool nonCorrupt = notcorrupt(packet);
    /************* check packet if corrupt or already acknowledged **************/
    if( nonCorrupt && validWin(seq, B_base, B_window) && ifSeqNotInBuff(recvdBuff, seq)){
       sndACK(packet);
       storeInBuffr(packet);
       int win = B_base + B_window;
       for(B_base; B_base < win; B_base++){
           if(ifSeqNotInBuff(recvdBuff, B_base))
           {
              break;
           }
           /*************** In sequence delivery to tolayer5 ****************/
           deliver_data(1, recvdBuff[B_base]->payload);
           recvdBuff.erase(B_base);
       }
    }
    else if(nonCorrupt && (seq < B_base+window))
    {
       sndACK(packet);
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    B_window = getwinsize();
    B_base = 0;

}
