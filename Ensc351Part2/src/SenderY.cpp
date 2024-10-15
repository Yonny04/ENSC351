//============================================================================
//
//% Student Name 1: Yoonsang You
//% Student 1 #: 301549016
//% Student 1 userid (email):yya270 (yya270@sfu.ca)
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
// File Name   : SenderY.cpp
// Version     : September 23rd, 2024
// Description : Starting point for ENSC 351 Project Part 2
// Original portions Copyright (c) 2024 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include "SenderY.h"

#include <iostream>
#include <filesystem>
#include <stdio.h> // for snprintf()
#include <stdint.h> // for uint8_t
#include <string.h> // for memset(), and memcpy() or strncpy()
#include <errno.h>
#include <fcntl.h>	// for O_RDWR or O_RDONLY
#include <sys/stat.h>

#include "myIO.h"
#include "VNPE.h"

using namespace std;
using namespace std::filesystem; // C++17 and beyond

SenderY::
SenderY(vector<const char*> iFileNames, int d)
:PeerY(d),
 bytesRd(-1),
 fileName(nullptr),
 fileNames(iFileNames),
 blkNum(0)
{
}

//-----------------------------------------------------------------------------

// get rid of any characters that may have arrived from the medium.
void SenderY::dumpGlitches()
{
	const int dumpBufSz = 20;
	char buf[dumpBufSz];
	int bytesRead;
	while (dumpBufSz == (bytesRead = PE(myReadcond(mediumD, buf, dumpBufSz, 0, 0, 0))));
}

// Send the block, less the block's last byte, to the receiver.
// Returns the block's last byte.
uint8_t SenderY::sendMostBlk(blkT blkBuf)
//uint8_t SenderY::sendMostBlk(uint8_t blkBuf[BLK_SZ_CRC])
{
	const int mostBlockSize{(BLK_SZ_CRC) - 1};
	PE_NOT(myWrite(mediumD, blkBuf, mostBlockSize), mostBlockSize);
	return *(blkBuf + mostBlockSize);
}

// Send the last byte of a block to the receiver
void
SenderY::
sendLastByte(uint8_t lastByte)
{
	PE(myTcdrain(mediumD)); // wait for previous part of block to be completely drained from the descriptor
	dumpGlitches();			// dump any received glitches

	PE_NOT(myWrite(mediumD, &lastByte, sizeof(lastByte)), sizeof(lastByte));
}

/* generate a block (numbered 0) with filename and filesize */
void SenderY::genStatBlk(blkT blkBuf, const char* fileName)
//void SenderY::genStatBlk(uint8_t blkBuf[BLK_SZ_CRC], const char* fileName)
{
    blkBuf[SOH_OH] = 0;
    blkBuf[SOH_OH + 1] = ~0;
    int index{DATA_POS};
    if (strlen(fileName) > 0) {
        const auto myBasename{path( fileName ).filename().string()};
        auto c_basename = myBasename.c_str();
        const auto fileNameLengthPlus1{strlen(c_basename) + 1};
        // check for fileNameLengthPlus1 greater than 127.
        if (fileNameLengthPlus1 + 1 > CHUNK_SZ) { // need at least one decimal digit to store st.st_size below
            cout /* cerr */ << "Ran out of space in file info block!  Need block with 1024 bytes of data." << endl;
            exit(-1);
        }
        // On Linux: The maximum length for a file name is 255 bytes. The maximum combined length of both the file name and path name is 4096 bytes.
        memcpy(&blkBuf[index], c_basename, fileNameLengthPlus1);
        //strncpy(&blkBuf[index], c_basename, 12X);
        index += fileNameLengthPlus1;
        struct stat st;
        PE(stat(fileName, &st));
        int spaceAvailable = CHUNK_SZ + DATA_POS - index;
        int spaceNeeded = snprintf((char*)&blkBuf[index], spaceAvailable, "%ld", st.st_size); // check the value of CHUNK_SZ + DATA_POS - index
        if (spaceNeeded > spaceAvailable) {
            cout /* cerr */ << "Ran out of space in file info block!  Need block with 1024 bytes of data." << endl;
            exit(-1);
        }
        index += spaceNeeded + 1;

        // blkNum = 0 - 1; // initialize blkNum for the data blocks to come.
    }
    uint8_t padSize = CHUNK_SZ + DATA_POS - index;
    memset(blkBuf+index, 0, padSize);

    // check here if index is greater than 128 or so.
    blkBuf[0] = SOH; // can be pre-initialized for efficiency if no 1K blocks allowed

    /* calculate and add CRC in network byte order */
    crc16ns((uint16_t*)&blkBuf[PAST_CHUNK], &blkBuf[DATA_POS]);
    // ???
}

/* tries to generate a block.  Updates the
variable bytesRd with the number of bytes that were read
from the input file in order to create the block. Sets
bytesRd to 0 and does not actually generate a block if the end
of the input file had been reached when the previously generated block
was prepared or if the input file is empty (i.e. has 0 length).
*/
//void SenderY::genBlk(blkT blkBuf)
void SenderY::genBlk(uint8_t blkBuf[BLK_SZ_CRC])
{
	//read data and store it directly at the data portion of the buffer
	bytesRd = PE(myRead(transferringFileD, &blkBuf[DATA_POS], CHUNK_SZ ));
	if (bytesRd>0) {
		blkBuf[0] = SOH; // can be pre-initialized for efficiency
		//block number and its complement
		//uint8_t nextBlkNum = blkNum + 1;
		blkBuf[SOH_OH] = blkNum;
		blkBuf[SOH_OH + 1] = ~blkNum;

				//pad ctrl-z for the last block
				uint8_t padSize = CHUNK_SZ - bytesRd;
				memset(blkBuf+DATA_POS+bytesRd, CTRL_Z, padSize);

			/* calculate and add CRC in network byte order */
			crc16ns((uint16_t*)&blkBuf[PAST_CHUNK], &blkBuf[DATA_POS]);
			// ???
	}
}

//Send CAN_LEN copies of CAN characters in a row to the YMODEM receiver, to inform it of
//	the cancelling of a session
void SenderY::cans()
{
	// No need to space in time CAN chars for Part 2.
	// This function will be more complicated in later parts.
    char buffer[CAN_LEN];
    memset( buffer, CAN, CAN_LEN);
    PE_NOT(myWrite(mediumD, buffer, CAN_LEN), CAN_LEN);
}

/* While sending the now current block for the first time, prepare the next block if possible.
*/
void SenderY::prepStatBlk()
{
   blkNum = 0;
   if (fileNameIndex < fileNames.size()) {
      fileName = fileNames[fileNameIndex];
      openFileToTransfer(fileName);
      if(transferringFileD != -1) {
         genStatBlk(blkBuf, fileName); // prepare 0eth block
      }
   }
   else {
      transferringFileD = -2; // no more files to transfer
      genStatBlk(blkBuf, ""); // prepare 0eth block
      fileName = ""; // nullptr;
   }
   fileNameIndex++;
}

void SenderY::sendBlkPrepNext()
{
    // **** this function will need to be modified ****
    blkNum ++; // 1st block about to be sent or previous block ACK'd
    memcpy(blkBufs[1], blkBuf, BLK_SZ_CRC); // Uses Memcpy to copy the current blk for transfer
    uint8_t lastByte = sendMostBlk(blkBufs[1]); // using sendMostBlk to transfer most of the blk
    genBlk(blkBuf); // prepare next block
    sendLastByte(lastByte);
}

// Resends the block that had been sent previously to the YMODEM receiver.
void SenderY::resendBlk()
{
	// resend the block including the crc16 (or checksum if code available for that)
	//  ***** You will have to write this simple function *****

   uint8_t lastByte = sendMostBlk(blkBufs[1]); // sends every bit except the last bit
   sendLastByte(lastByte); //sends the CRC bit of the blk
}

int
SenderY::
openFileToTransfer(const char* fileName)
{
    transferringFileD = myOpen(fileName, O_RDONLY);
    return transferringFileD;
}

/* If not already closed, close file that was transferred (or being transferred).
 * Set transferringFileD to -1 when file is closed.
 * Return 0 if file is closed or 1 if file was already closed.
 */
int
SenderY::
closeTransferredFile()
{
    if (transferringFileD != -1) {
        PE(myClose(transferringFileD));
        transferringFileD = -1;
        return 0;
    }
    else
        return 1;
}

void SenderY::sendFiles()
{
   char byteToReceive;
//   EOTCounter = 0;
   SenderY& ctx = *this; // needed to work with SmartState-generated code

    // ***** modify the below code according to the protocol *****
    // below is just a starting point.  You can follow a
    //  different structure if you want.

   ctx.prepStatBlk();
   while (fileNames.size() >= fileNameIndex) {
      PE_NOT(myRead(mediumD, &byteToReceive, 1), 1); // assuming get a 'C'

      if (-1 == ctx.transferringFileD) { // if it fails
        cans(); // sending can to cancel
        ctx.result = "CreatError"; // when the result is an errno

     }

      ctx.sendBlkPrepNext();

      PE_NOT(myRead(mediumD, &byteToReceive, 1), 1); // assuming get an ACK
      PE_NOT(myRead(mediumD, &byteToReceive, 1), 1); // assuming get a 'C'

      while (ctx.bytesRd) {
         ctx.sendBlkPrepNext();
         // assuming on next line we get an ACK
         PE_NOT(myRead(mediumD, &byteToReceive, 1), 1);

         //case for NAK
         ctx.errCnt = 0; // Sets the error counter
         while(byteToReceive == NAK && ctx.errCnt < errB){ // for when error count < errorbound
            ctx.resendBlk(); // resend the byte using resenBlk function
            PE_NOT(myRead(mediumD, &byteToReceive, 1), 1);
         }

      }
      ctx.sendByte(EOT); // send the first EOT
      ctx.closeTransferredFile();
      PE_NOT(myRead(mediumD, &byteToReceive, 1), 1); // assuming get a NAK
      ctx.sendByte(EOT); // send the second EOT
      PE_NOT(myRead(mediumD, &byteToReceive, 1), 1); // assuming get an ACK
      ctx.result += "Done, ";
      ctx.prepStatBlk();
   }
   ctx.sendLastByte(ctx.sendMostBlk(blkBuf));
   PE_NOT(myRead(mediumD, &byteToReceive, 1), 1); // assuming get an ACK

   // remove ", " from the end of the result string.
   if (ctx.result.size())
      ctx.result.erase(ctx.result.size() - 2);
}

