//============================================================================
//
//% Student Name 1: Yoonsang You
//% Student 1 #: 301549016
//% Student 1 userid (email): yya270- (stu1@sfu.ca)
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
// Version     : September, 2024
// Copyright   : Original Portions Copyright 2024, Craig Scratchley
// Description : Starting point for ENSC 351 Project Part 2
//============================================================================

#define HEAP_PREALLOCATION 5000

#include <stdlib.h> // EXIT_SUCCESS
#include <sys/socket.h>
#include <pthread.h>
#include <thread>

#include "myIO.h"
#include "Medium.h"

#include "VNPE.h"
#include "AtomicCOUT.h"
#include "posixThread.hpp"

#include "ReceiverY.h"
#include "SenderY.h"

using namespace std;
using namespace pthreadSupport;

enum  {Term1, Term2};
enum  {TermSkt, MediumSkt};

static int daSktPr[2];	      //Socket Pair between term1 and term2
static int daSktPrT1M[2];	  //Socket Pair between term1 and medium
static int daSktPrMT2[2];	  //Socket Pair between medium and term2

void testReceiverY(int mediumD)
{
    COUT << "Will try to receive file(s) with CRC" << endl;
    ReceiverY yReceiver(mediumD);
    yReceiver.receiveFiles();
    COUT << "yReceiver result was: " << yReceiver.result << endl  << endl;
}

void testSenderY(vector<const char*> iFileNames, int mediumD)
{
    SenderY ySender(iFileNames, mediumD);
    COUT << "test sending" << endl;
    ySender.sendFiles();
    COUT << "Sender finished with result: " << ySender.result << endl << endl;
}

void termFunc(const int termNum)
{
	// ***** modify this function to communicate with the "Kind Medium" *****

	if (Term1 == termNum) {
      //testReceiverY(daSktPr[Term1]);        // file does not exist
        testReceiverY(daSktPrT1M[TermSkt]);        // this now communicates through the kind medium
	}
	else { // Term2
		PE_0(pthread_setname_np(pthread_self(), "T2")); // give the thread (terminal 2) a name

	    vector iFileNamesA{"/doesNotExist.txt"};
	    vector iFileNamesB{"/home/xubuntu/.sudo_as_admin_successful", "/etc/mime.types"};

      //testSenderY(iFileNamesA, daSktPr[Term2]);   // file does not exist
        testSenderY(iFileNamesB, daSktPrMT2[MediumSkt]);   // this also now communicates through the kind medium
        pthreadSupport::setSchedPrio(20); // drop priority down somewhat.  FIFO?
        PE(myClose(daSktPr[termNum]));
	}

}

int Ensc351Part2()
{
	// ***** Modify this function to create the "Kind Medium" threads and communicate with it ***** done.

	PE_0(pthread_setname_np(pthread_self(), "P-T1")); // give the Primary thread (Terminal 1) a name
	// ***** switch from having one socketpair for direct connection to having two socketpairs
	//			for connection through medium threads *****

	// Two distinct socket pairs that follows the diagram
	PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, daSktPrT1M));
	PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, daSktPrMT2));

    posixThread term2Thrd(SCHED_FIFO, 70, termFunc, Term2);

    // ***** create thread with SCHED_FIFO priority 40 for medium *****
    //     have the thread run the function found in Medium.cpp:
    //          void mediumFunc(int T1d, int T2d, const char *fname)
    //          where T1d is the descriptor for the socket to Term1
    //          and T2d is the descriptor for the socket to Term2
    //          and fname is the name of the binary medium "log" file
    //          ("ymodemData.dat").
    //      Make sure that thread is created at SCHED_FIFO priority 40

   // Thread that meets the above criteria
   posixThread M1to2Thrd(SCHED_FIFO, 40, mediumFunc, daSktPrT1M[MediumSkt], daSktPrMT2[TermSkt], "ymodemData.dat");
	termFunc(Term1);

	return EXIT_SUCCESS;
}
