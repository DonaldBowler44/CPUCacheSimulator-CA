#ifndef CACHE_H
#define CACHE_H

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

typedef struct Block_Info {
    int tag;
    bool valid;
    int timestamp;
    struct Block_Info* next;
} Block_Info;

// Struct for single node in outer queue
typedef struct Cache_block {
    int index;
    Block_Info* block_info;
    struct Cache_block* next;
} Cache_block;

extern int currentTimestamp; // Global variable to track time quanta, victim counter

int calculate_num_blocks(int cache_size_kb, int block_size);
int calculate_block_offset(int block_size);
int calculate_rows(int size, int block_size, int associativity);
int calculate_tagbits(int index, int offset);
int calculate_Overhead(int tag, int associativity, int rows);
int calculate_Implementation_menory(int size, int overhead);
void enqueueBIStruct(Block_Info** info, unsigned int tag);
bool checkIfIndex(Cache_block* outerQueue, unsigned int targetIndex);
bool checkIfCapacity(Cache_block* outerQueue, unsigned int targetIndex, int assciativity);
void QueueOperations(unsigned int indexToEnqueue, unsigned int tagToEnqueue, Cache_block** outerQueue, int associativity, FILE* outputFile, Cache* cache);
void printAllTags(Cache_block* outerQueue, FILE* outputFileTwo); 
void freeInnerQueue(Block_Info* block_info);
void freeOuterQueue(Cache_block* outerQueue);
void replaceOldestTag(Block_Info** info, unsigned int tag); //using RR to replace
void replaceRandomTag(Block_Info** info, unsigned int tag); //using random to replace
void findValidTagsAndCount(Cache_block* outerQueue, int* validCount, int* setTagWithDataCount);

#endif