//============================================================================
//
//% Student Name : Yoonsang You
//% Student 1 #: 301549016
//% Student 1 userid (email): yya270 (stu1@sfu.ca)
//
//% Student Name 2: Vincent Hong
//% Student 2 #: 301558858
//% Student 2 userid (email): mha200 (mha200@sfu.ca)
//
//% Below, edit to list any people who helped you with the code in this file,
//%      or put 'None' if nobody helped (the two of) you.
//
// Helpers: _everybody helped us/me with the assignment (list names or put 'None')__
//
// Also, list any resources beyond the course textbooks and the course pages on Piazza
// that you used in making your submission.
//
// Resources:  ___________
//
//%% Instructions:
//% * Put your name(s), student number(s), userid(s) in the above section.
//% * Also enter the above information in other files to submit.
//% * Edit the "Helpers" line and, if necessary, the "Resources" line.
//% * Your group name should be "P2_<userid1>_<userid2>" (eg. P2_stu1_stu2)
//% * Form groups as described at:  https://coursys.sfu.ca/docs/students
//% * Submit files to coursys.sfu.ca
//
// File Name   : ReceiverY.cpp
// Version     : September 24th, 2024
// Description : Starting point for ENSC 351 Project Part 2
// Original portions Copyright (c) 2024 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include "ReceiverY.h"

#include <string.h> // for memset()
#include <fcntl.h>
#include <stdint.h>
#include "myIO.h"
#include "VNPE.h"

//using namespace std;

ReceiverY::
ReceiverY(int d)
:PeerY(d),
 m_closeProb(1),
 m_anotherFile(0xFF),
 m_NCGbyte('C'),
 m_goodBlk(false),
 m_goodBlk1st(false),
 m_syncLoss(false), // transfer will end if syncLoss becomes true
 m_numLastGoodBlk(0)
{
}

/* Only called after an SOH character has been received.
The function receives the remaining characters to form a complete
block.
The function will set or reset a Boolean variable,
goodBlk. This variable will be set (made true) only if the
calculated checksum or CRC agrees with the
one received and the received block number and received complement
are consistent with each other.
Boolean member variable syncLoss will only be set to
true when goodBlk is set to true AND there is a
fatal loss of syncronization as described in the XMODEM
specification.
The member variable goodBlk1st will be made true only if this is the first
time that the block was received in "good" condition. Otherwise
goodBlk1st will be made false.
*/
void ReceiverY::getRestBlk()
{
    // ********* this function must be improved ***********
    PE_NOT(myReadcond(mediumD, &m_rcvBlk[1], REST_BLK_SZ_CRC, REST_BLK_SZ_CRC, 0, 0), REST_BLK_SZ_CRC);
    m_goodBlk1st = m_goodBlk = true;
    uint8_t blkNum = m_rcvBlk[SOH_OH]; // number of blk at m_rcvBlk[1]

    uint16_t CRC_extract = *(uint16_t*)(&m_rcvBlk[PAST_CHUNK]); // Extracting the required blk from m_rcvBlk array
    uint16_t CRC_computed; // For CRC computed/calculated value

    uint8_t blkNum_twosComplmt = m_rcvBlk[SOH_OH + 1];
    crc16ns(&CRC_computed, &m_rcvBlk[DATA_POS]); // computing the blk of data and stores the result in crc_computed


    if (blkNum != m_numLastGoodBlk + 1 && m_goodBlk == true){
         m_syncLoss = true;
    }
   if ((uint8_t)(~blkNum_twosComplmt) != blkNum){  // checks if blkNum is equal to the bitwise NOT of complement
         m_goodBlk = false;
         return;
      }
    if (CRC_computed != CRC_extract){  // for when its not a good blk
         m_goodBlk = false;
         return;
    }
    else{
         m_syncLoss = false;
      }

    if (blkNum == m_numLastGoodBlk + 1){
       m_numLastGoodBlk = blkNum;
       m_goodBlk1st = true;
    }
    else{
       m_goodBlk1st = false;
    }
}

//Write chunk (data) in a received block to disk
void ReceiverY::writeChunk()
{
   // ***** Make changes so that possible padding in the last block is not written to the file.
   int byteToWrite = CHUNK_SZ;

   if(m_numLastGoodBlk){ // Checks if the current block is the last and removes any padding
      while(byteToWrite > 0 && m_rcvBlk[DATA_POS + byteToWrite - 1] == CTRL_Z){// Loops and finds the last byte before CTRLz padding
         --byteToWrite; // decreases the number of bytes to write if CTRLz is confirmed.
      }
   }

   if(byteToWrite > 0){
	PE_NOT(myWrite(transferringFileD, &m_rcvBlk[DATA_POS], CHUNK_SZ), CHUNK_SZ);
   }
}

int
ReceiverY::
openFileForTransfer()
{
    const mode_t mode{S_IRUSR | S_IWUSR}; //  | S_IRGRP | S_IROTH};
    transferringFileD = myCreat((const char *) &m_rcvBlk[DATA_POS], mode);
    return transferringFileD;
}

/* If not already closed, close file that was just received (or being received).
 * Set transferringFileD to -1 and numLastGoodBlk to 255 when file is closed.  Thus numLastGoodBlk
 * is ready for the next file to be sent.
 * Return the errno if there was an error closing the file and otherwise return 0.
 */
int
ReceiverY::
closeTransferredFile()
{
    if (transferringFileD > -1) {
        m_closeProb = myClose(transferringFileD);
        if (m_closeProb)
            return errno;
        else {
            m_numLastGoodBlk = 255;
            transferringFileD = -1;
        }
    }
    return 0;
}

//Send CAN_LEN CAN characters in a row to the XMODEM sender, to inform it of
//	the cancelling of a file transfer
void ReceiverY::cans()
{
	// no need to space in time CAN chars coming from receiver
    char buffer[CAN_LEN];
    memset( buffer, CAN, CAN_LEN);
    PE_NOT(myWrite(mediumD, buffer, CAN_LEN), CAN_LEN);
}

uint8_t
ReceiverY::
checkForAnotherFile()
{
    return (m_anotherFile = m_rcvBlk[DATA_POS]);
}

void ReceiverY::receiveFiles()
{
    ReceiverY& ctx = *this; // needed to work with SmartState-generated code
    /* we have 4 states
     * Idle
     * Sending blk
     * Resending blk
     * Finalizing
     */

    // ***** improve this member function *****

    // below is just an example template.  You can follow a
    //  different structure if you want.

    while (
            sendByte(ctx.m_NCGbyte),
            PE_NOT(myRead(mediumD, m_rcvBlk, 1), 1), // Should be SOH
            ctx.getRestBlk(), // get block 0 with fileName and filesize
            checkForAnotherFile()) {

        if(openFileForTransfer() == -1) {
            cans();
            result = "CreatError"; // include errno or so
            return;
        }
        else {
            sendByte(ACK); // acknowledge block 0 with fileName.

            // inform sender that the receiver is ready and that the
            //      sender can send the first block
            sendByte(ctx.m_NCGbyte);

            while(PE_NOT(myRead(mediumD, m_rcvBlk, 1), 1),
                  (m_rcvBlk[0] == SOH))
            {
               ctx.getRestBlk();
               if(ctx.m_goodBlk == true){
                ctx.sendByte(ACK); // assume the expected block was received correctly.
                ctx.writeChunk();
               }
               else{
                  ctx.sendByte(NAK);
               }
            };
            // assume EOT was just read in the condition for the while loop
            ctx.sendByte(NAK); // NAK the first EOT
            PE_NOT(myRead(mediumD, m_rcvBlk, 1), 1); // presumably read in another EOT

            // Check if the file closed properly.  If not, result should be "CloseError".
            if (ctx.closeTransferredFile()) {
                ; // ***** fill this in *****
                ctx.result = "CloseError";
            }
            else {
                 ctx.sendByte(ACK);  // ACK the second EOT
                 ctx.result += "Done, ";
            }
        }
    }
    sendByte(ACK); // acknowledge empty block 0.

    // remove ", " from the end of the result string.
    if (ctx.result.size())
       ctx.result.erase(ctx.result.size() - 2);
}
