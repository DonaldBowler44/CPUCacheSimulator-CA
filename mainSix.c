#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define OneKB 1024
#define VAbitDatabus 32
#define ValidPlusDirtyBit 2
#define bitstobyte 8
#define costPerKB 0.09 

// Define a structure for the cache
typedef struct Cache {
    int size; // Cache size in bytes
    int block_size; // Block size in bytes
    int associativity; // Associativity
    char policy[12]; // Replacement policy (RR or RND)
    int num_blocks; // num of blocks in the cache

    int index_bits;     // Number of index bits
    int tag_bits;       // Number of tag bits
    int Offset_bits;     // Number of offset bits

    int physicalMemory; // To be stored as phyiscal memory,
    int overhead;
    int rows;
    int implementMemory;
    int impMeminKB;
    float cost;

} Cache;

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

//-----------------------------------------------------------------------END OF QUEUE OPERATIONS//



int getCountofLines() {
    int count = 0;
    FILE *fpCount;
    char c;

    fpCount = fopen("A-9_new_1.5.pdf.trc", "r");
    if (fpCount == NULL) {
        perror("Error opening file");
        return(-1);
    }

    // Extract characters from file and store in character c
    for (c = getc(fpCount); c != EOF; c = getc(fpCount))
        if (c == '\n') // Increment count if this character is newline
            count = count + 1;
 
    // Close the file
    fclose(fpCount);
    //printf("The file has %d lines\n ", count);
    return count + 1;

};

int calculate_num_blocks(int cache_size_kb, int block_size) {
    return (cache_size_kb * OneKB) / block_size;
}

int calculate_block_offset(int block_size){
	int result;  
	return result = log2(block_size); ; 
}

int calculate_rows(int size, int block_size, int associativity){
    int index, cacheSize;
    cacheSize = size * OneKB;
    index = cacheSize / (block_size * associativity);
    return index;
    //    return result = log2(index);
}

int calculate_tagbits(int index, int offset){
    int tag;
    return tag = VAbitDatabus - index - offset;

}

int calculate_Overhead(int tag, int associativity, int rows){
    int result;
    return result = associativity *(tag + 1) * rows/8;
    
}

int calculate_Implementation_menory(int size, int overhead)
{
     int result, cacheSize;
     cacheSize = size * OneKB;
     return result = cacheSize + overhead;
}
// Initialize the cache
Cache* init_cache(int size, int block_size, int associativity, const char * policy, int physicalMemory) {
    Cache* cache = (Cache*)malloc(sizeof(Cache));
     //cacheSize=size,blockSize=blocksize
     //1 KB = 1024

    cache->size = size;
    cache->block_size = block_size;
    cache->associativity = associativity;
    strcpy(cache->policy, policy);
    cache->physicalMemory = physicalMemory;

    // Calculate the number of blocks and set it in the cache
    cache->num_blocks = calculate_num_blocks(size, block_size);
    cache->Offset_bits = calculate_block_offset(block_size);
    cache->rows = calculate_rows(size, block_size, associativity);
    cache->index_bits = log2(cache->rows);
    cache->tag_bits = calculate_tagbits(cache->index_bits, cache->Offset_bits);
    cache->overhead = calculate_Overhead(cache->tag_bits, cache->associativity,  cache->rows);
    cache->implementMemory = calculate_Implementation_menory(cache->size, cache->overhead);
    cache->impMeminKB = cache->implementMemory / OneKB;
    cache->cost = cache->impMeminKB * costPerKB;

    return cache;
}

void parseEIPLine(const char* line, Queue* queue) {
    int byteLength;
    unsigned int eipAddress;

    //printf("Eip line: %s\n", line);
    if (sscanf(line, "EIP (%d): %8x", &byteLength, &eipAddress) == 2) {
        // Store values in the AddressInfo struct
            //printf("EIP (%d): %x\n", byteLength, eipAddress);
        enqueueEIPByte(queue, eipAddress, byteLength);
    }
    // Parse the line starting with "EIP"
    // Extract the necessary information and perform any required processing
    // For example:
    // sscanf(line, "EIP (%*d): %x %*x %*x %*x %*x", &EIPAddress);
    // ...
}

void parseDstMLine(const char* line, Queue* queue) {
    char *dstM_start = strstr(line, "dstM:");
    char *srcM_start = strstr(line, "srcM:");

    if (dstM_start && srcM_start) {
        unsigned int dstM_value, srcM_value;
        if (sscanf(dstM_start, "dstM: %8x", &dstM_value) == 1 &&
            sscanf(srcM_start, "srcM: %8x", &srcM_value) == 1) {
            //printf("dstM: %08x\n", dstM_value);
            //printf("srcM: %08x\n", srcM_value);
            enqueueDestSrc(queue, dstM_value, srcM_value);
        } else {
            printf("Parsing failed.\n");
        }
    } else {
        printf("dstM or srcM not found in the input.\n");
    }

}




int main(int argc, char* argv[])
{
    char *traceFile = NULL;
    int cacheSizeKB = 0;
    int blockSize = 0;
    int associativity = 0;
    const char * replacementPolicy;
    int physicalMemory = 0;
    FILE *fp;

    printf("When using -r flag, please enter Roundrobin or robin\n");
    printf("When using -s flag, please enter cacheSize in KB, if 512 KB, enter 512\n");
    printf("When using -b flag, please enter block size in Bytes, if 16 bytes, enter 16\n");
    printf("When using -p flag, please enter block size in KB, if 1 KB, enter 1\n");
    printf(" ");
    
    for (int i = 1; i < argc; i++) {
        switch (argv[i][1]) {
            case 'f':
                if (i + 1 < argc) {
                    traceFile = argv[++i];
                } 
                break;
            case 's':
                if (i + 1 < argc) {
                    cacheSizeKB = atoi(argv[++i]);
                } 
                break;
            case 'b':
                if (i + 1 < argc) {
                    blockSize = atoi(argv[++i]);
                } 
                break;
            case 'a':
                if (i + 1 < argc) {
                    associativity = atoi(argv[++i]);
                } 
                break;
            case 'r':
                if (i + 1 < argc) {
                    replacementPolicy = argv[++i];
                } 
                break;
            case 'p':
                if (i + 1 < argc) {
                    physicalMemory = atoi(argv[++i]);
                } 
                break;
            default:
                fprintf(stderr, "Unknown option: %s\n", argv[i]);
                return 1; // Exit with an error code
        }
    }

    Cache* cache = init_cache(cacheSizeKB, blockSize, associativity, replacementPolicy, physicalMemory);

    int MAX_LINES = getCountofLines();

    // Allocate memory for an array of AddressInfo pointers
    AddressInfo** addressInfoArray = malloc(MAX_LINES * sizeof(AddressInfo*)); // MAX_LINES determined by your function
    if (!addressInfoArray) {
        perror("Memory allocation failed");
    // Handle the error, free any allocated memory, and exit...
    }

    // Open the file for reading
    // AddressInfo
    //parsing file location
    fp = fopen("A-9_new_1.5.pdf.trc", "r");
    if (fp == NULL) {
        perror("Error opening file");
        return(-1);
    }

 
    char line[150];
    unsigned int EIPaddress;
    int byteLength;
    unsigned int DestAddress;
    unsigned int SrcAddress;
    Queue* queue = initializeQueue();

        while (fgets(line, sizeof(line), fp) != NULL) {
            if (strstr(line, "EIP") == line) {
                // Line starts with "EIP"
                parseEIPLine(line, queue);
            } else if (strstr(line, "dstM") == line) {
                // Line starts with "dstM"
                parseDstMLine(line, queue);
            } else if (line[0] == '\n') {
                continue;
                // Handle the empty line as needed
            }
        }
        fclose(fp);

        printf("Cache Simulator - CS 3853 – Group #08\n");
        printf("\n");
        printf("Trace File: %s\n", traceFile);
        printf("\n");
        printf("***** Cache Input Parameters *****\n");
        printf("Cache Size: %d KB\n", cache->size);
        printf("Block Size: %d bytes\n", cache->block_size);
        printf("Associativity: %d\n", cache->associativity);
        printf("replacement Policy: %s\n", cache->policy);
        printf("Physical Memory: %d\n", cache->physicalMemory);
        printf("\n");
        printf("***** Cache Calculated Values *****\n");
        printf("\n");
        printf("Total # Blocks: %d\n", cache->num_blocks);
        printf("Tag Size: %d bits\n", cache->tag_bits);
        printf("Index Size: %d bits\n", cache->index_bits);
        printf("Total # Rows: %d\n", cache->rows);
        printf("Overhead Size: %d bytes\n", cache->overhead);
        //printf("Implementation Memory Size: %d KB (%d bytes)\n", cache->impMeminKB, cache->implementMemory);
        printf("Implementation Memory Size: %.2f KB (%d bytes)\n", (float)cache->impMeminKB, cache->implementMemory);
        printf("Cost: $%.2f\n", cache->cost);
         printf("\n");


        int stopAtTwenty=0;

        while (queue != NULL && stopAtTwenty < 20){
            AddressInfo element1 = dequeue(queue);
            //printf("Combined EIPAddress and byteLength: EIP=%08x, byteLength=%d\n", element1.EIPAddress, element1.byteLength);
            printf("Address: %08x, length = %d ", element1.EIPAddress, element1.byteLength);

            AddressInfo element2 = dequeue(queue);
            //printf("Combined DestAddress and srcAddress: Dest=%08x, src=%08x\n", element2.DestAddress, element2.srcAddress);
            if ( element2.DestAddress == 0 && element2.srcAddress != 0)
            {
                printf(" No data writes and Data read at %08x, length=4\n", element2.srcAddress);
            }
            else if ( element2.DestAddress != 0 && element2.srcAddress == 0)
            {
                printf("Data write at %08x and length=4, no data reads\n", element2.srcAddress);
            }
            else if ( element2.DestAddress == 0 && element2.srcAddress == 0)
            {
                printf("No data writes/reads occurred\n");
            }
            else {
                printf("Data write at %08x, length=4 and Data read at %08x, length=4\n", element2.DestAddress, element2.srcAddress);
            }

            stopAtTwenty++;

        }

        // Call the function to read and print the values in the array


    // Don't forget to free the allocated memory when you're done

    // Validate and process the values as needed
    // (e.g., check if cache size is within range, handle different replacement policies, etc.)

    // Example: Printing the parsed values

    // printf("Cache Simulator - CS 3853 – Group #08\n");
    // printf("\n");
    // printf("Trace File: %s\n", traceFile);
    // printf("\n");
    // printf("***** Cache Input Parameters *****\n");
    // printf("Cache Size: %25d KB\n", cache->size);
    // printf("Block Size: %24d bytes\n", cache->block_size);
    // printf("Associativity: %20d\n", cache->associativity);
    // printf("replacement Policy: %17s\n", cache->policy);
    // printf("Physical Memory: %18d\n", cache->physicalMemory);
    // printf("\n");
    // printf("***** Cache Calculated Values *****\n");
    // printf("\n");
    // printf("Total # Blocks: %20d\n", cache->num_blocks);
    // printf("Tag Size: %24d2f @ ($0.09 / KB)\n", " ", cache->cost);
    //580.00 KB (593920 bytes)


// Overhead Size: 69632 bytes
// Implementation Memory Size: 580.00 KB (593920 bytes)
// Cost: $75.40 
 //bits\n", cache->tag_bits);
    // printf("Index Size: %22d bits\n", cache->tag_bits);
    // printf("Total # Rows: %22d\n", cache->rows);
    // printf("Overhead Size: %22d bytes\n", cache->overhead);
    // printf("Implementation Memory Size:  %5d KB (%d bytes)\n", cache->impMeminKB, cache->implementMemory);
    // printf("Cost: %25s$%.
    // Your program logic goes here
    // cache->size = size;
    // cache->block_size = block_size;
    // cache->associativity = associativity;
    // cache->policy = policy;
    //./mSx.exe -f trace1.trc -s 512 -b 16 -a 8 -r RND -p 1
    // gcc mainSix.c -o mSx

    return 0;
}

// int cachesizeinKB = size * OneKB;
//     int phyMeminKB = physicalMemory * OneKB;
    
//     printf("cachesizeinKB: %d\n", cachesizeinKB);
//     printf("phyMeminKB: %d\n", phyMeminKB);

//     //num of blocks
//     cache->num_blocks = (cachesizeinKB / phyMeminKB);
// tag size: in bits
// index size: in bits
// total # rows: 
// overhead size:
// Implemtation memory size: 

// if(hexValue = '00000000')
// {
//     printf("no data written.\n");
// }