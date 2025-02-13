//============================================================================
//
//% Student Name 1: student1
//% Student 1 #: 123456781
//% Student 1 userid (email): stu1 (stu1@sfu.ca)
//
//% Student Name 2: student2
//% Student 2 #: 123456782
//% Student 2 userid (email): stu2 (stu2@sfu.ca)
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
//% * Your group name should be "P1_<userid1>_<userid2>" (eg. P1_stu1_stu2)
//% * Form groups as described at:  https://coursys.sfu.ca/docs/students
//% * Submit files to coursys.sfu.ca
//
// File Name   : SenderY.cpp
// Version     : September 5, 2024
// Description : Starting point for ENSC 351 Project
// Original portions Copyright (c) 2024 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include "SenderY.h"

#include <iostream>
#include <filesystem>
#include <stdio.h> // for snprintf()
#include <stdint.h> // for uint8_t
#include <string.h> // for memset(), and memcpy() or strncpy()
#include <errno.h>
#include <fcntl.h>	// for O_RDWR
#include <sys/stat.h>

#include "myIO.h"

using namespace std;
using namespace std::filesystem; // C++17 and beyond

SenderY::
SenderY(vector<const char*> iFileNames, int d)
:PeerY(d),
 bytesRd(-1), 
 fileNames(iFileNames),
 fileNameIndex(0),
 blkNum(0)
{
}

//-----------------------------------------------------------------------------

/* generate a block (numbered 0) with filename and filesize */
//void SenderY::genStatBlk(blkT blkBuf, const char* fileName)
void SenderY::genStatBlk(uint8_t blkBuf[BLK_SZ_CRC], const char* fileName)
{
    // TODO: ********* additional code must be written ***********

        struct stat st;
        stat(fileName, &st);
        // unsigned fileSize{st.st_size};

    /* calculate and add CRC in network byte order */
    // TODO: ********* The next couple lines need to be changed ***********
    uint16_t myCrc16ns;
    crc16ns(&myCrc16ns, &blkBuf[0]);
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
    // TODO: ********* The next line needs to be changed ***********
    if (-1 == (bytesRd = myRead(transferringFileD, &blkBuf[0], CHUNK_SZ )))
        ErrorPrinter("myRead(transferringFileD, &blkBuf[0], CHUNK_SZ )", __FILE__, __LINE__, errno);
    // TODO: ********* and additional code must be written ***********

        /* calculate and add CRC in network byte order */
        // TODO: ********* The next couple lines need to be changed ***********
        uint16_t myCrc16ns;
        crc16ns(&myCrc16ns, &blkBuf[0]);
}

void SenderY::cans()
{
    // TODO: ********* send CAN_LEN of CAN characters (write to mediumD) ***********
}

//uint8_t SenderY::sendBlk(blkT blkBuf)
void SenderY::sendBlk(uint8_t blkBuf[BLK_SZ_CRC])
{
    // TODO: ********* fill in some code here to send a block (write to mediumD) ***********
}

void SenderY::statBlk(const char* fileName)
{
    blkNum = 0;
    // assume 'C' received from receiver to enable sending with CRC
    genStatBlk(blkBuf, fileName); // prepare 0eth block
    sendBlk(blkBuf); // send 0eth block
    // assume sent block will be ACK'd
}

void SenderY::sendFiles()
{
    for (const auto fileName : fileNames) {
        transferringFileD = myOpen(fileName, O_RDONLY, 0);
        if(-1 == transferringFileD) {
            cans();
            cout /* cerr */ << "Error opening input file named: " << fileName << endl;
            result += "OpenError";
            return;
        }
        else {
            cout << "Sender will send " << fileName << endl;

            // do the protocol, and simulate a receiver that positively acknowledges every
            //	block that it receives.

            statBlk(fileName);

            // assume 'C' received from receiver to enable sending with CRC
            genBlk(blkBuf); // prepare 1st block
            while (bytesRd)
            {
                ++ blkNum; // 1st block about to be sent or previous block was ACK'd

                sendBlk(blkBuf); // send block

                // assume sent block will be ACK'd
                genBlk(blkBuf); // prepare next block
                // assume sent block was ACK'd
            };
            // finish up the file transfer, assuming the receiver behaves normally and there are no transmission errors
            // TODO: ********* fill in some code here ***********

            //(myClose(transferringFileD));
            if (-1 == myClose(transferringFileD))
                ErrorPrinter("myClose(transferringFileD)", __FILE__, __LINE__, errno);
            result += "Done, ";
        }
    }
    // indicate end of the batch.
    statBlk("");

    // remove ", " from the end of the result string.
    if (result.size())
        result.erase(result.size() - 2);
}

