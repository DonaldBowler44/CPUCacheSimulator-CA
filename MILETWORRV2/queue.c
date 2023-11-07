#include "queue.h"

// Function to initialize an empty queue
Queue* initializeQueue() {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    if (queue == NULL) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
    queue->front = queue->rear = NULL;
    return queue;
}

// Function to create a new node with AddressInfo data
QueueNode* createNode(AddressInfo data) {
    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    if (newNode == NULL) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

// Function to enqueue an AddressInfo combining DestAddress and srcAddress
void enqueueDestSrc(Queue* queue, unsigned int dest, unsigned int src) {
    AddressInfo data;
    data.DestAddress = dest;
    data.srcAddress = src;
    //printf("dstM: %08x srcM: %08x\n", data.DestAddress, data.srcAddress);
    QueueNode* newNode = createNode(data);

    if (queue->rear == NULL) {
        queue->front = queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
}

// Function to enqueue an AddressInfo combining EIPAddress and byteLength
void enqueueEIPByte(Queue* queue, unsigned int eip, int byteLength) {
    AddressInfo data;
    data.EIPAddress = eip;
    data.byteLength = byteLength;
    QueueNode* newNode = createNode(data);
    //printf("newNode EIPByte: %08x %d\n", data.EIPAddress, data.byteLength);

    if (queue->rear == NULL) {
        queue->front = queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
}

// Function to dequeue an element from the queue
AddressInfo dequeue(Queue* queue) {
    if (queue->front == NULL) {
        printf("Queue is empty\n");
        exit(EXIT_FAILURE);
    }

    QueueNode* temp = queue->front;
    AddressInfo data = temp->data;
    queue->front = queue->front->next;

    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    free(temp);
    return data;
}