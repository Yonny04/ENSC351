/*
 * PeerY.h
 *
 *      Author: wcs
 */

#ifndef PEERY_H_
#define PEERY_H_

#include <stdint.h> // for uint8_t
#include <string>

#define CHUNK_SZ	 128
#define SOH_OH  	 1			//SOH Byte Overhead
#define BLK_NUM_AND_COMP_OH  2	//Overhead for blkNum and its complement
#define DATA_POS  	 (SOH_OH + BLK_NUM_AND_COMP_OH)	//Position of data in buffer
#define PAST_CHUNK 	 (DATA_POS + CHUNK_SZ)		//Position of checksum in buffer

#define CS_OH           1			                    //Overhead for CheckSum
#define REST_BLK_OH_CS  (BLK_NUM_AND_COMP_OH + CS_OH)	//Overhead in rest of block
#define REST_BLK_SZ_CS  (CHUNK_SZ + REST_BLK_OH_CS)
#define BLK_SZ_CS  	 	(SOH_OH + REST_BLK_SZ_CS)
#define MOST_BLK_SZ_CS	(BLK_SZ_CS - 1)

#define CRC_OH           2			                    //Overhead for CRC16
#define REST_BLK_OH_CRC  (BLK_NUM_AND_COMP_OH + CRC_OH)	//Overhead in rest of block
#define REST_BLK_SZ_CRC  (CHUNK_SZ + REST_BLK_OH_CRC)
#define BLK_SZ_CRC  	 (SOH_OH + REST_BLK_SZ_CRC)
#define MOST_BLK_SZ_CRC	 (BLK_SZ_CRC - 1)

#define CAN_LEN 2 // normally 8 // the number of CAN characters to send to cancel a transmission

#define errB 10 // the error bound.  The number of errors in a row that are allowed.

#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18 // 24  
#define	CTRL_Z 0x1A //	26

#define UNITS_PER_SEC 10

#define TM_2CHAR (2*UNITS_PER_SEC) // wait for more than 1 second (1 second plus)
#define TM_CHAR (1*UNITS_PER_SEC) // wait for 1 second

#define mSECS_PER_UNIT (1000/UNITS_PER_SEC)		//milliseconds per unit

typedef uint8_t blkT[BLK_SZ_CRC];

void 
crc16ns (uint16_t* crc16nsP, uint8_t* buf);

class PeerY {
public:
	PeerY(int d)
	;

	//Send a byte to the remote peer across the medium
	void
	sendByte(uint8_t byte)
	;

	std::string result;  // result of the file transfer
	unsigned errCnt;	 // counts the number of "errors" in a row

protected:
	int mediumD; // descriptor for serial port or delegate
	int transferringFileD;	// descriptor for file being read from or written to.
};

#endif /* PEERY_H_ */
