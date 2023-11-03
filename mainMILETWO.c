#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
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

    //cpi information
    int cacheHits;
    int TotalCacheAccesses;
    int compMisses;
    int conflictMisses;
    int numOfCycles;

} Cache;


typedef struct AddressFirstLine {
    unsigned int EIPAddress; //for the address at EIP () line
    int byteLength; //byteLength
    unsigned int srcAddress; //srcaddress
    unsigned int DestAddress; // destaddress
    unsigned int machCodeOne;
    unsigned int machCodeTwo;

} AddressInfo;
//-cache-block-queue-operations--------------------------------------------------------------------------------------//
// Struct for inner queue
typedef struct Block_Info {
    int tag;
    bool valid;
    struct Block_Info* next;
} Block_Info;

// Struct for single node in outer queue
typedef struct Cache_block {
    int index;
    Block_Info* block_info;
    struct Cache_block* next;
} Cache_block;

// Function to enqueue an element into the inner queue
void enqueueBIStruct(Block_Info** info, unsigned int tag) {
    Block_Info* newNode = (struct Block_Info*)malloc(sizeof(struct Block_Info));
    newNode->tag = tag;
    newNode->valid = true;
    newNode->next = NULL;

    if (*info == NULL) {
        *info = newNode;
    } else {
        Block_Info* current = *info;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
}

// Function to check if any outer queue has a certain index
bool checkIfIndex(Cache_block* outerQueue, unsigned int targetIndex) {
    Cache_block* current = outerQueue;
    while (current != NULL) {
        if (current->index == targetIndex) {
            //printf("current index: %u\n", current->index);
            return true; // Found the target index in the outer queue
        }
        current = current->next;
    }
    return false; // Target index not found in the outer queue
}

// Function to check if the capacity of a specific cache block is reached
bool checkIfCapacity(Cache_block* outerQueue, unsigned int targetIndex, int assciativity) {
    Cache_block* current = outerQueue;
    while (current != NULL) {
        if (current->index == targetIndex) {
            // Found the target index in the outer queue
            // Count the number of tags in the Block_Info queue
            int tagCount = 0;
            Block_Info* blockInfo = current->block_info;
            while (blockInfo != NULL) {
                tagCount++;
                blockInfo = blockInfo->next;
            }
            // Compare with MAX_ASSOCIATIVE_BLOCKS
            return (tagCount == assciativity);
        }
        current = current->next;
    }
    return false; // Target index not found in the outer queue
}

// Queue operations to find
void QueueOperations(unsigned int indexToEnqueue, unsigned int tagToEnqueue, Cache_block** outerQueue, int associativity, FILE* outputFile, Cache* cache) {
    if (!checkIfIndex(*outerQueue, indexToEnqueue)) {
        // If index doesn't exist in outer queue, create a new cache block
        Cache_block* newCacheBlock = (Cache_block*)malloc(sizeof(Cache_block));
        newCacheBlock->index = indexToEnqueue;
        newCacheBlock->block_info = NULL; // You can initialize the inner queue here
        newCacheBlock->next = NULL;

        // Add the new cache block to the outer queue
        if (*outerQueue == NULL) {
            *outerQueue = newCacheBlock;
        } else {
            Cache_block* current = *outerQueue;
            while (current->next != NULL) {
                current = current->next;
            }
            current->next = newCacheBlock;
        }
        fprintf(outputFile, "Cache Index %x added, cache block created!\n", newCacheBlock->index);
        //printf("Cache Index %x added, cache block created!\n", newCacheBlock->index);
    }

    // Enqueue block info for the cache block with the current index
    Cache_block* current = *outerQueue;
    while (current != NULL) {
        if (current->index == indexToEnqueue) {
            Block_Info* blockInfo = current->block_info;
            while (blockInfo != NULL) {
                if (blockInfo->tag == tagToEnqueue) {
                    fprintf(outputFile, "Tag %x is already in the inner queue for index: %x. Rejecting it!\n", tagToEnqueue, indexToEnqueue);
                    cache->cacheHits++;
                    cache->numOfCycles++;
                    printf("Cache Hit Cycle: %d\n", cache->numOfCycles);
                    //printf("Tag %u is already in the inner queue for index: %x. Rejecting it!\n", tagToEnqueue, indexToEnqueue);
                    return;  // Reject the duplicate tag
                }
                blockInfo = blockInfo->next;
            }

            if(!checkIfCapacity(current, indexToEnqueue, associativity)) 
            {
                enqueueBIStruct(&(current->block_info), tagToEnqueue);
                fprintf(outputFile, "Tag %x has been added for index: %x\n", tagToEnqueue, indexToEnqueue);
                cache->compMisses++;
        
                double x = (double)cache->block_size;
                int numOfreads = (int)ceil(x);

                cache->numOfCycles += (4 * (numOfreads / 4));
                printf("Cache miss Cycle: %d\n", cache->numOfCycles);
                //cache miss 4 * (ceiling(blocksize / 4)) for numOfCycles
                
                //printf("Tag %x has been added for index: %x\n", tagToEnqueue, indexToEnqueue);
                break;  
            }
            else 
            {
                fprintf(outputFile, "Conflict miss\n");
                cache->conflictMisses++;
            } 
        }
        current = current->next;
    }
}

// Function to print all tags for the associated indexes
void printAllTags(Cache_block* outerQueue, FILE* outputFileTwo) {
    Cache_block* current = outerQueue;
    while (current != NULL) {
        fprintf(outputFileTwo, "Index: %x\n", current->index);
        //printf("Index: %x\n", current->index);
        Block_Info* blockInfo = current->block_info;
        while (blockInfo != NULL) {
            fprintf(outputFileTwo, "  Tag: %x\n", blockInfo->tag);
            //printf("  Tag: %u\n", blockInfo->tag);
            blockInfo = blockInfo->next;
        }
        current = current->next;
    }
}

// Function to free the memory of the inner queue (Block_Info)
void freeInnerQueue(Block_Info* block_info) {
    while (block_info != NULL) {
        Block_Info* temp = block_info;
        block_info = block_info->next;
        free(temp);
    }
}

// Function to free the memory of the outer queue (Cache_block)
void freeOuterQueue(Cache_block* outerQueue) {
    while (outerQueue != NULL) {
        Cache_block* temp = outerQueue;
        outerQueue = outerQueue->next;

        // Free the inner queue (Block_Info) for this Cache_block
        freeInnerQueue(temp->block_info);

        free(temp);
    }
}
//---end-of-cache-block-queue-operations-------------------------------------------------------------------//

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

    //cpi info
    cache->TotalCacheAccesses=0;

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

bool isEmpty(Queue* queue) {
    return queue->front == NULL;
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
     Cache_block* outerQueue = NULL;
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
        //fclose(fp);

        // CacheBlock** cacheArray = malloc(cache->rows * sizeof(CacheBlock*));
        // for (int i = 0; i < cache->rows; i++) {
        //     cacheArray[i] = malloc(cache->associativity * sizeof(CacheBlock));
        // }

        //reading memory requires 4 cycles
        //

        // Assuming 'cache' is your Cache structure and 'element1.EIPAddress' is the address you want to split.
        unsigned int offset_mask = (1 << cache->Offset_bits) - 1;
        unsigned int index_mask = (1 << cache->index_bits) - 1;
        //CacheBlock cacheBlock;
        int num_cache_blocks = cache->num_blocks;
        

        // Open a file for writing
        FILE* outputFile = fopen("output.txt", "w");
        if (outputFile == NULL) {
            perror("Error opening output file");
            return 1;
        }

        int cacheIndexCount = 0;
        cache->cacheHits = 0;
        cache->compMisses = 0;
        cache->conflictMisses = 0;
        //if cache index equals number of associative blocks break, put this in if cache_blocks[i].tag == 0
        //it was valid and tag matched - cache hits
        //it was either not valid or tag didn't match - cache misses
        //it was not valid - complsory misses (cache is acccessed for the first time)
        //it was valid, tag did not match - conflict misses (cache set is full for the data block)
        while (!isEmpty(queue)){
            AddressInfo element1 = dequeue(queue);
            //printf("Combined EIPAddress and byteLength: EIP=%08x, byteLength=%d\n", element1.EIPAddress, element1.byteLength);
            //printf("Address: %08x, length = %d \n", element1.EIPAddress, element1.byteLength);

            unsigned int offset = element1.EIPAddress & offset_mask;
            unsigned int index = (element1.EIPAddress >> cache->Offset_bits) & index_mask;
            unsigned int tag = element1.EIPAddress >> (cache->Offset_bits + cache->index_bits);
            cache->TotalCacheAccesses++;
            // Call the QueueOperations function to enqueue the data
            QueueOperations(index, tag, &outerQueue, cache->associativity, outputFile, cache);




            // Store the extracted bits in the cache block.
            // cacheBlock.tag = tag;
            // cacheBlock.index = index;
            // cacheBlock.offset = offset;

            //printf(" index: %x\n", index);
            //printf(" tag:%x\n", tag);

            
            //printf(" index: %x\n", index);
            //printf(" tag:%x\n", tag);

            AddressInfo element2 = dequeue(queue);
            //printf("Combined DestAddress and srcAddress: Dest=%08x, src=%08x\n", element2.DestAddress, element2.srcAddress);
            if ( element2.DestAddress == 0 && element2.srcAddress != 0)
            {
                //printf(" No data writes and Data read at %08x, length=4\n", element2.srcAddress);

            }
            else if ( element2.DestAddress != 0 && element2.srcAddress == 0)
            {
                //printf("Data write at %08x and length=4, no data reads\n", element2.srcAddress);

            }
            else if ( element2.DestAddress == 0 && element2.srcAddress == 0)
            {
                //printf("No data writes/reads occurred\n");
            }
            else {
                //printf("Data write at %08x, length=4 and Data read at %08x, length=4\n", element2.DestAddress, element2.srcAddress);

            }


            }

            // Loop through all cache blocks
            // Free the allocated memory for cache_blocks when done
            // int totalCacheMisses = (cache->compMisses + cache->conflictMisses);
            // printf("cache accesses: %d\n", cache->TotalCacheAccesses );
            // printf("cache hits: %d\n", cache->cacheHits);
            // printf("comp misses: %d\n", cache->compMisses);
            // printf("conflict misses: %d\n", cache->conflictMisses);
            // printf("total cacheMisses: %d\n", totalCacheMisses);

             // Free the allocated memory when you are done with the data structures
             FILE* outputFileTwo = fopen("indexToTags.txt", "w");
            if (outputFileTwo == NULL) {
                perror("Error opening output file");
                return 1;
            }
            int totalCacheMisses = (cache->compMisses + cache->conflictMisses);
            float hitRate = (cache->cacheHits * 100.0) / cache->TotalCacheAccesses;
            float missRate = 100.0 - hitRate;
            float CPI = cache->numOfCycles / cache->TotalCacheAccesses;
            // (Hits * 100) / Total Accesses
            fprintf(outputFileTwo, "All Tags:\n");
            printAllTags(outerQueue, outputFileTwo);
            printf("Cache Simulator - CS 3853 – Group #08\n");
            printf("\n");
            printf("Trace File: %s\n", traceFile);
            printf("\n");
            printf("***** CACHE SIMULATION RESULTS *****\n");
            printf("Total Cache Accesses: %d\n", cache->TotalCacheAccesses);
            printf("Cache Hits: %d\n", cache->cacheHits);
            printf("Cache Misses: %d\n", totalCacheMisses);
            printf("--- Compulsory Misses: %d\n", cache->compMisses);
            printf("--- Conflict Misses: %d\n", cache->conflictMisses);
            printf("\n");
            printf("***** ***** CACHE SIMULATION RESULTS ***** *****\n");
            printf("\n");
            printf("Hit Rate: %.4f%%\n", hitRate);
            printf("Miss Rate: %.4f%%\n", missRate);
            printf("CPI: %.2f", CPI);

            freeOuterQueue(outerQueue);
            fclose(fp);

            // printf("Unused Cache Space: %.2f KB / %.2f KB = %.2f%%\n", unused_cache_space_kb, cache_size_kb, (unused_cache_space_kb / cache_size_kb) * 100);
            // printf("Unused Cache Blocks: %d / %d\n", unused_cache_blocks, total_cache_blocks);


        //  int numTags;
        // unsigned int* tags = get(0x151, &numTags, map, size);
        // if (tags) {
        //     printf("\nValues of key 1: [");
        //     for (int i = 0; i < numTags; i++) {
        //         printf("%u", tags[i]);
        //         if (i < numTags - 1) {
        //             printf(", ");
        //         }
        //     }
        //     printf("]\n");
        // } else {
        //     printf("\nKey 1 not found.\n");
        // }

    //./mC.exe -f trace1.trc -s 512 -b 16 -a 8 -r RND -p 1
    // gcc mainMILETWO.c -o mC

    return 0;
}