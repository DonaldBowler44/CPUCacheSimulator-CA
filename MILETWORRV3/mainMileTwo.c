#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include <math.h>

#include "queue.h"
#include "cache.h"

int getCountofLines(char *traceFile) {
    int count = 0;
    FILE *fpCount;
    char c;

    // fpCount = fopen("A-10_new_1.5_a.pdf.trc", "r");
    fpCount = fopen(traceFile, "r");
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
     srand(time(NULL));

    printf("When using -r flag, please enter Roundrobin or random\n");
    printf("When using -s flag, please enter cacheSize in KB, if 512 KB, enter 512\n");
    printf("When using -b flag, please enter block size in Bytes, if 16 bytes, enter 16\n");
    printf("When using -p flag, please enter block size in KB, if 1 KB, enter 1\n");
    printf("\n");
    printf("Please wait a moment as the program compiles. It may take a minute or two.\n");
    printf("\n");
    
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

  
    int MAX_LINES = getCountofLines(traceFile);

    // Allocate memory for an array of AddressInfo pointers
    AddressInfo** addressInfoArray = malloc(MAX_LINES * sizeof(AddressInfo*)); // MAX_LINES determined by your function
    if (!addressInfoArray) {
        perror("Memory allocation failed");
    // Handle the error, free any allocated memory, and exit...
    }

    // Open the file for reading
    // AddressInfo
    //parsing file location
    // fp = fopen("A-10_new_1.5_a.pdf.trc", "r");
     fp = fopen(traceFile, "r");
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
        int k = 0;
        while (!isEmpty(queue)){
            AddressInfo element1 = dequeue(queue);
            //printf("Combined EIPAddress and byteLength: EIP=%08x, byteLength=%d\n", element1.EIPAddress, element1.byteLength);
            //printf("Address: %08x, length = %d \n", element1.EIPAddress, element1.byteLength);

            //for(k=0; k < element1.byteLength; k++) {

                unsigned int offset = element1.EIPAddress & offset_mask;
                unsigned int index = (element1.EIPAddress >> cache->Offset_bits) & index_mask;
                unsigned int tag = element1.EIPAddress >> (cache->Offset_bits + cache->index_bits);
                cache->TotalCacheAccesses++;

                int cacheBlockCount = countCacheBlocks(outerQueue);

                     if(strcmp(cache->policy, "RR") == 0)
                {
                    //printf("round robin chosen\n");
                    handleCacheBlockRoundRobinReplacement(&outerQueue, cacheBlockCount, index, tag, cache->rows, cache->associativity, outputFile, cache);
                    
                }
                else if (strcmp(cache->policy, "RND") == 0)
                {
                    //printf("random chosen\n");
                    handleCacheBlockRandomReplacement(&outerQueue, cacheBlockCount, index, tag, cache->rows, cache->associativity, outputFile, cache);
                    
                }

                
                //element1.EIPAddress++;
            //}

            // unsigned int offset = element1.EIPAddress & offset_mask;
            // unsigned int index = (element1.EIPAddress >> cache->Offset_bits) & index_mask;
            // unsigned int tag = element1.EIPAddress >> (cache->Offset_bits + cache->index_bits);
            //cache->TotalCacheAccesses++;
            // Call the QueueOperations function to enqueue the data
           
           
           //---this is the replacement algorithim goes--------------------//
            // int cacheBlockCount = countCacheBlocks(outerQueue);
            // //void handleCacheBlockRandomReplacement(Cache_block** outerQueue, int cacheBlockCount, unsigned int newIndex, unsigned int newTag, int rows, int associativity, FILE* outputFile, Cache* cache);
            // //void handleCacheBlockRoundRobinReplacement(Cache_block** outerQueue, int cacheBlockCount, unsigned int newIndex, unsigned int newTag, int rows, int associativity, FILE* outputFile, Cache* cache);
            //    if(strcmp(cache->policy, "RR") == 0)
            //     {
            //         //printf("round robin chosen\n");
            //         handleCacheBlockRoundRobinReplacement(&outerQueue, cacheBlockCount, index, tag, cache->rows, cache->associativity, outputFile, cache);
                    
            //     }
            //     else if (strcmp(cache->policy, "RND") == 0)
            //     {
            //         //printf("random chosen\n");
            //         handleCacheBlockRandomReplacement(&outerQueue, cacheBlockCount, index, tag, cache->rows, cache->associativity, outputFile, cache);
                    
            //     }
            //-------------------------------------------------------------//

            //QueueOperations(index, tag, &outerQueue, cache->associativity, outputFile, cache);




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
            int validCount = 0;
            //int nonvalidCount = 0;
            int setTagWithDataCount = 0;

            // Call the function to find valid entries and set tags with data
            findValidTagsAndCount(outerQueue, &validCount, &setTagWithDataCount);

            // Print the counts
            //printf("Number of valid entries: %d\n", validCount);
            //printf("Number of set tags with data: %d\n", setTagWithDataCount);
            

            // Unused KB = ( (TotalBlocks-Compulsory Misses) * (BlockSize+OverheadSize) ) / 1024
            // The 1024 KB below is the total cache size for this example
            // Waste = COST/KB * Unused KB
            //nonvalidCount = cache->num_blocks - validCount; 
            double UnusedKB = ((cache->num_blocks - cache->compMisses) * (cache->block_size + (validCount)) / cache->implementMemory);
            //printf("unusedKB before / OneKB: %.2f\n", UnusedKB);
            //double UnusedKBdivided = UnusedKB / OneKB;
            //printf("unusedKB after: %.2f\n", UnusedKBdivided);

            //double UnusedKB = ((TotalBlocks - CompulsoryMisses) * (cache->block_size + OverheadSize)) / 1024.0;
            //double UnusedKB = ((cache->num_blocks - cache->compMisses) * (cache->block_size + (validCount + setTagWithDataCount))) / OneKB;
            //double UnusedKB = ((cache->num_blocks - cache->compMisses) * (cache->block_size + cache->overhead)) / OneKB;
            // Calculate the Waste
            //double Waste = cache->cost / OneKB * UnusedKB;
            double Waste = costPerKB * UnusedKB;
            //printf("Waste: %.2f\n", Waste);
            // int unused_cache_blocks = cache->num_blocks - cache->compMisses;
            int unused_cache_blocks = (UnusedKB * cache->num_blocks) / cache->impMeminKB;
            //printf("unused caache blocks: %d\n", unused_cache_blocks);
            //printf("\nUnused KB: %.2f KB\n", UnusedKB);
            //printf("Waste: %.2f\n", Waste);

            // cache->impMeminKB = cache->implementMemory / OneKB;
            //cache->cost = cache->impMeminKB * costPerKB;

            // (Hits * 100) / Total Accesses

            //------------these are all the correct print statements below-----------------------------------------------------------------//
            
            fprintf(outputFileTwo, "All Tags:\n");
            printAllTags(outerQueue, outputFileTwo);
            printf("Cache Simulator - CS 3853 - Group #08\n");
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
            
            printf("Implementation Memory Size: %.2f KB (%d bytes)\n", (float)cache->impMeminKB, cache->implementMemory);
            printf("Cost: $%.2f\n", cache->cost);
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
            printf("CPI: %.2f\n", CPI);
            printf("Unused Cache Space: %.2f KB / %.2f KB = %.2f%% Waste: $%.2f\n", UnusedKB, (float)cache->impMeminKB, (UnusedKB / (float)cache->impMeminKB) * 100, Waste);
            printf("Unused Cache Blocks: %d / %d\n", unused_cache_blocks, cache->num_blocks);

            freeOuterQueue(outerQueue);
            fclose(fp);

            //------------these are all the correct print statements above-----------------------------------------------------------------//



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
    //./mainMileTwo.exe -f trace1.trc -s 512 -b 16 -a 8 -r RND -p 1
    //./mainMileTwo.exe -f A-9_new_1.5.pdf.trc -s 512 -b 16 -a 8 -r RND -p 1
    //./mainMileTwo.exe -f A-10_new_1.5_a.pdf.trc -s 512 -b 16 -a 8 -r RND -p 1
    //use makefile to run file
    // gcc mainMILETWO.c -o mC

    return 0;
}