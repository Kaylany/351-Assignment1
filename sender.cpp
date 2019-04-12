
#include <iostream>
#include <sys/shm.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include "msg.h"    /* For the message struct */

/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000

using namespace std;

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void* sharedMemPtr;

/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory
 * @param msqid - the id of the shared memory
 */

void init(int& shmid, int& msqid, void*& sharedMemPtr)
{

	/* TODO:
        1. Create a file called keyfile.txt containing string "Hello world" (you may do
 		    so manually or from the code).
     */

    printf("Making keyfile\n");
    ofstream file;
    file.open("keyfile.txt");
	file>>"Hello this is our assignment 1";

    if (!file) {
        cout << "Unable to open file." << endl;
        exit(1);
    }

    file.close();

	/*    2. Use ftok("keyfile.txt", 'a') in order to generate the key. */

	/*	3. Use the key in the TODO's below. Use the same key for the queue
		    and the shared memory segment. This also serves to illustrate the difference
		    between the key and the id used in message queues and shared memory. The id
		    for any System V objest (i.e. message queues, shared memory, and sempahores)
		    is unique system-wide among all SYstem V objects. Two objects, on the other hand,
		    may have the same key.
	 */

    key_t key = ftok ("keyfile.txt", 'a');

    if (key == -1) {
        perror ("ftok");
        exit(1);
    }

	/* TODO: Get the id of the shared memory segment. The size of the segment must be SHARED_MEMORY_CHUNK_SIZE */
 // IPC_CREAT -> tells the system to create a new memory segment for the shared memory
 // The file's owner can read and write (06)
 // Users in the same group as the file's owner can read (first 4)
 // All users can read (second 4)
 // shmid = shmget(key_t k, int size, int flag);
    shmid = shmget (key, SHARED_MEMORY_CHUNK_SIZE, 0644 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

	/* TODO: Attach to the shared memory */

    sharedMemPtr = shmat (shmid, (void *) 0, 0);
    if (sharedMemPtr == (char *) (-1)) {
        perror("shmat");
        exit(1);
    }

	/* TODO: Attach to the message queue */

    msqid = msgget (key, 0644 | IPC_CREAT);
    if (msqid == -1) {
        perror ("msgget");
        exit (1);
    }

	/* Store the IDs and the pointer to the shared memory region in the corresponding parameters */

}

/**
 * Performs the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */

void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
	/* TODO: Detach from shared memory */
    if (shmdt(sharedMemPtr) < 0) {
        perror ("shmdt");
        exit(-1);
    }
}

/**
 * The main send function
 * @param fileName - the name of the file
 */
void send(const char* fileName)
{
	/* Open the file for reading */
	FILE* fp = fopen(fileName, "r");


	/* A buffer to store message we will send to the receiver. */
	message sndMsg;

	/* A buffer to store message received from the receiver. */
	message rcvMsg;

	/* Was the file open? */
	if(!fp)
	{
		perror("fopen");
		exit(-1);
	}

printf("Reading loop\n");
	/* Read the whole file */
	while(!feof(fp))
	{
		/* Read at most SHARED_MEMORY_CHUNK_SIZE from the file and store them in shared memory.
 		 * fread will return how many bytes it has actually read (since the last chunk may be less
 		 * than SHARED_MEMORY_CHUNK_SIZE).
 		 */
		if((sndMsg.size = fread(sharedMemPtr, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp)) < 0)
		{
			perror("fread");
			exit(-1);
		}


		/* Send a message to the receiver telling him that the data is ready
 		 * (message of type SENDER_DATA_TYPE
 		 */
     printf("Sending to receiver\n");
     sndMsg.mtype = SENDER_DATA_TYPE;
     if (msgsnd(msqid, &sndMsg, sizeof(sndMsg), 0) < 0) {
       perror("msgsnd: ");
       exit(-1);
     }

		/* Wait until the receiver sends us a message of type RECV_DONE_TYPE telling us
 		 * that he finished saving the memory chunk.
 		 */
     printf("Waiting for receiver\n");
     if (msgrcv(msqid, &rcvMsg, sizeof(sndMsg), RECV_DONE_TYPE, 0) < 0) {
       perror("msgrcv: ");
       exit(-1);
     }
	}


	/** once we are out of the above loop, we have finished sending the file.
 	  * Lets tell the receiver that we have nothing more to send. We will do this by
 	  * sending a message of type SENDER_DATA_TYPE with size field set to 0.
	  */
    printf("End send loop\n");
    sndMsg.size = 0;
    if (msgsnd(msqid, &sndMsg, sizeof(sndMsg), 0) < 0) {
      perror("msgsnd");
      exit(-1);
    }


	/* Close the file */
	fclose(fp);

}


int main(int argc, char** argv)
{

  printf("Starting Sender\n");
	/* Check the command line arguments */
	if(argc < 2)
	{
		fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
		exit(-1);
	}

  printf("Connecting Shared Memory and Message Queue\n");
	/* Connect to shared memory and the message queue */
	init(shmid, msqid, sharedMemPtr);

  printf("Sending File\n");
	/* Send the file */
	send(argv[1]);

  printf("Cleaning Up\n");
	/* Cleanup */
	cleanUp(shmid, msqid, sharedMemPtr);

  printf("Finished\n");

	return 0;
}
