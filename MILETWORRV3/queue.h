#ifndef QUEUE_H
#define QUEUE_H

#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>

typedef struct AddressFirstLine {
    unsigned int EIPAddress; //for the address at EIP () line
    int byteLength; //byteLength
    unsigned int srcAddress; //srcaddress
    unsigned int DestAddress; // destaddress
    unsigned int machCodeOne;
    unsigned int machCodeTwo;

} AddressInfo;

//---QUEUE OPERATIONS, TO BE PUT IN ANOTHER FILE----------------------------------------------------------//
// Define a node for the queue
typedef struct QueueNode {
    AddressInfo data;
    struct QueueNode* next;
} QueueNode;

// Define the queue structure
typedef struct Queue {
    QueueNode* front;
    QueueNode* rear;
} Queue;

Queue* initializeQueue();
QueueNode* createNode(AddressInfo data); 
void enqueueDestSrc(Queue* queue, unsigned int dest, unsigned int src);
void enqueueEIPByte(Queue* queue, unsigned int eip, int byteLength);
bool isEmpty(Queue* queue);
AddressInfo dequeue(Queue* queue);

//-----------------------------------------------------------------------END OF QUEUE OPERATIONS//

#endif