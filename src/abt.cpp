#include "../include/simulator.h"
#include <iostream>
#include <list>
#include <string>
#include <string.h>
#include <stdlib.h>
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
using namespace std;
static bool A_ACK = true;
static int expectedSeq = 0;
static float RTT_Delay = 15.0;
static struct pkt lastSntPkt;
static int totalJourneyTime = get_sim_time();
int createChecksum(struct pkt);
int countofconsistentRTT = 0;

struct pkt *createPacket(struct msg message){
    struct pkt *packet = (struct pkt *)malloc(sizeof(struct pkt));
    (*packet).seqnum = expectedSeq;
    (*packet).acknum = -1;
    strcpy((*packet).payload,message.data);
    (*packet).checksum = createChecksum((*packet));
    return packet;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    if(A_ACK == true){
        starttimer(0,RTT_Delay);
        A_ACK = false;
        lastSntPkt = *createPacket(message);
         tolayer3(0, lastSntPkt);
        totalJourneyTime = get_sim_time();
    }
}

int createChecksum(struct pkt packet){
    char mssg[20];
    strcpy(mssg,packet.payload);
    int localchecksum = 0;
    for(int i = 0; i<20 && mssg[i] != '\0'; i++){
        localchecksum += mssg[i];
    }
    localchecksum += packet.seqnum;
    localchecksum += packet.acknum;
    return localchecksum;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    // check if the checksum for the acknowledgement is same as calculated and this is for the same sequence number as expected
    int checkingsum = createChecksum(packet);
    if(checkingsum == packet.checksum && packet.acknum == expectedSeq){
        A_ACK = true;
        stoptimer(0);
        expectedSeq = !expectedSeq;
         totalJourneyTime = get_sim_time() - totalJourneyTime;
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    if(A_ACK == false){
        starttimer(0,RTT_Delay);
        tolayer3(0,lastSntPkt);
    }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    A_ACK = true;
    expectedSeq = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
static int B_expectedSeq = 0;
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
    {
    if(B_expectedSeq == packet.seqnum && createChecksum(packet)==packet.checksum){
        tolayer5(1,packet.payload);
        struct pkt *ACK = (struct pkt *)malloc(sizeof(struct pkt *));
        (*ACK).acknum = packet.seqnum;
        (*ACK).checksum = packet.seqnum;
        tolayer3(1,(*ACK));
        B_expectedSeq = !B_expectedSeq;
    }
    else if(B_expectedSeq != packet.seqnum && createChecksum(packet)==packet.checksum){
        struct pkt (*ACK) = (struct pkt *) malloc(sizeof(struct pkt *));
        (*ACK).acknum = packet.seqnum;
        (*ACK).checksum = packet.seqnum;
        tolayer3(1, (*ACK));
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    B_expectedSeq = 0;
}
