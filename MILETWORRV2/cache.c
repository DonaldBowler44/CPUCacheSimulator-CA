#include "cache.h"

int currentTimestamp;

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

// Function to enqueue an element into the inner queue
void enqueueBIStruct(Block_Info** info, unsigned int tag) {
    Block_Info* newNode = (struct Block_Info*)malloc(sizeof(struct Block_Info));
    newNode->tag = tag;
    newNode->valid = true;
    newNode->timestamp = currentTimestamp++; // Assign and increment the current time quanta
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

// Function to replace the oldest tag in the inner queue based on timestamp
void replaceOldestTag(Block_Info** info, unsigned int tag) {
    // Find the oldest tag and update it with the new tag
    Block_Info* current = *info;
    Block_Info* oldestTag = *info;
    int oldestTimestamp = current->timestamp; // Initialize with the timestamp of the first element

    while (current != NULL) {
        if (current->timestamp < oldestTimestamp) {
            oldestTag = current;
            oldestTimestamp = current->timestamp;
        }
        current = current->next;
    }
    oldestTag->tag = tag;
    oldestTag->timestamp = currentTimestamp++; // Update the timestamp to the current time quanta

    printf("Replaced the oldest tag %u with new tag %u for index: %x\n", oldestTag->tag, tag);
}

// Function to replace a randomly selected tag in the inner queue
void replaceRandomTag(Block_Info** info, unsigned int tag) {
    // Calculate the number of tags in the inner queue
    int tagCount = 0;
    Block_Info* blockInfo = *info;
    while (blockInfo != NULL) {
        tagCount++;
        blockInfo = blockInfo->next;
    }
    // printf("tagCount: %d\n", tagCount);
    // int randomIndex = rand() % tagCount;
    //printf("randomIndex: %d\n", randomIndex);
    
    if (tagCount > 0) {
        // Generate a random number between 0 and (tagCount - 1)
        int randomIndex = rand() % tagCount;
        printf("randomIndex: %d\n", randomIndex);

        // Find the tag at the randomly selected index and update it with the new tag
        Block_Info* current = *info;
        for (int i = 0; i < randomIndex; i++) {
            current = current->next;
        }
        current->tag = tag;

        printf("Replaced a randomly selected tag with new tag %x\n", tag);
    } else {
        // If there are no tags in the inner queue, simply enqueue the new tag
        enqueueBIStruct(info, tag);
        printf("No tags in the inner queue. Added new tag %x\n", tag);
    }
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
                    //printf("Cache Hit Cycle: %d\n", cache->numOfCycles);
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
                //printf("Cache miss Cycle: %d\n", cache->numOfCycles);
                //cache miss 4 * (ceiling(blocksize / 4)) for numOfCycles
                
                //printf("Tag %x has been added for index: %x\n", tagToEnqueue, indexToEnqueue);
                break;  
            }
            else 
            {
                //conflict miss so replace oldest tag
                fprintf(outputFile, "Conflict miss\n");
                printf("index to be replaced: %x\n", indexToEnqueue);
                if(strcmp(cache->policy, "RR") == 0)
                {
                    printf("round robin chosen\n");
                    replaceOldestTag(&(current->block_info), tagToEnqueue);
                }
                else if (strcmp(cache->policy, "RND") == 0)
                {
                    printf("random chosen\n");
                    replaceRandomTag(&(current->block_info), tagToEnqueue);
                }
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
            fprintf(outputFileTwo, " timestamp: %d\n", blockInfo->timestamp);
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

// Function to find and count valid entries and set tags with data
void findValidTagsAndCount(Cache_block* outerQueue, int* validCount, int* setTagWithDataCount) {
    Cache_block* current = outerQueue;
    while (current != NULL) {
        Block_Info* blockInfo = current->block_info;
        while (blockInfo != NULL) {
            if (blockInfo->valid) {
                (*validCount)++; // Increment the count of valid entries
                (*setTagWithDataCount)++; // Increment the count of set tags with data
            }
            blockInfo = blockInfo->next;
        }
        current = current->next;
    }
}
//---end-of-cache-block-queue-operations-------------------------------------------------------------------//