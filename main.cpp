// Author: D.S. Koshelev
// Created: 05/04/2023
// Requirement: FreeRTOS

#include <iostream>
#include <assert.h>

using namespace std;

// *****Local defines*****

#define MEM_PULL_SIZE 128
#define MEM_BLOCK_SIZE 16

#define DEBUGPRINT
// to run code without FreeRTOS
#ifdef DEBUGPRINT
    #define pvPortMalloc    malloc
    #define pvPortFree      free
#endif // #ifdef DEBUGPRINT


// *****Local types*****

// definition of structure which contain only pointer on the same structure (the next memory block) and create appropriate data type
typedef struct MemoryBlock_s{
    struct MemoryBlock_s* next;
}MemoryBlock_t;

// definition of data type structure of Memory Pool
typedef struct MemoryPool_s{
    void* memoryStart;
    void* memoryEnd;
    MemoryBlock_t* freeList;
    size_t blockSize;
    size_t poolSize;
}MemoryPool_t;

// *****Local prototypes*****

MemoryPool_t* createMemoryPool(size_t blockSize, size_t poolSize);
void* allocateBlock(MemoryPool_t* pool);
void freeBlock(MemoryPool_t* pool, void* blockAddr);
void destroyMemoryPool(MemoryPool_t* pool);

void test_createMemoryPool(size_t blocksize, size_t pullsize);
void test_allocateBlock(size_t blocksize, size_t pullsize);
void test_freeBlock(size_t blocksize, size_t pullsize);

// *****Local functions*****

// create memory pull memory allocation and status initialization
MemoryPool_t* createMemoryPool(size_t blockSize, size_t poolSize){
    assert(blockSize > 0);
    assert(poolSize > blockSize);

    // allocate memory pull for system objects data
    void* poolMemory = pvPortMalloc(poolSize);

    if (poolMemory == NULL) {
        return NULL;
    }

    // allocate memory for structure which description memory pull
    MemoryPool_t* pool = (MemoryPool_t*)pvPortMalloc(sizeof(MemoryPool_t));
    if (pool == NULL) {
        pvPortFree(poolMemory);
        return NULL;
    }
    // fill the fields of the memory pull structure
    pool->memoryStart = poolMemory;
    pool->memoryEnd = (char*)poolMemory + poolSize;
    pool->freeList = NULL;
    pool->blockSize = blockSize;
    pool->poolSize = poolSize;

    // divide the memory pull into the blocks and allocate for each one structure with address next one
    size_t numBlocks = poolSize / blockSize;
    for (size_t i = 0; i < numBlocks; ++i) {
        MemoryBlock_t* block = (MemoryBlock_t*)((char*)poolMemory + i * blockSize);
        block->next = pool->freeList;
        pool->freeList = block;
    }

    #ifdef DEBUGPRINT
        printf( "Pull memory Start = %x\n", poolMemory);
        printf( "Pull memory End = %x\n", pool->memoryEnd);
    #endif // DEBUGPRINT

    return pool;
}

// allocateBlock: allocate block from memory pool
// allocateBlock: return address of the free block from the list of free blocks
void* allocateBlock(MemoryPool_t* pool) {
    if (pool->freeList == NULL) {
        return NULL;
    }

    MemoryBlock_t* block = pool->freeList;
    pool->freeList = pool->freeList->next;

    #ifdef DEBUGPRINT
        printf( "\nNewAllocatedBlock:\n" );
        printf( "memoryNewAllocatedBlock = %x\n", block);
        printf( "memoryFreeForNextBlock = %x\n", pool->freeList);
    #endif // DEBUGPRINT

    return (void*)block;
}

// freeBlock: unchain memory block and append that to list of the free blocks
void freeBlock(MemoryPool_t* pool, void* blockAddr) {
    if (blockAddr == NULL) {
        return;
    }

    MemoryBlock_t* block = (MemoryBlock_t*)blockAddr;
    block->next = pool->freeList;
    pool->freeList = block;
    #ifdef DEBUGPRINT
        printf( "\nBreakFreeBlock:\n" );
        printf( "withStartAddress = %x\n", blockAddr);
    #endif // DEBUGPRINT
}

// destroyMemoryPool: unchain memory for memory pull
void destroyMemoryPool(MemoryPool_t* pool)
{
    if (pool == NULL) {
        return;
    }
    pvPortFree(pool->memoryStart);
    pvPortFree(pool);
    #ifdef DEBUGPRINT
        printf( "\n###destroyMemoryPool###\n" );
    #endif // DEBUGPRINT
}


//  Unit tests:
// 1) create memory pull
// 2) memory block allocation
// 3) memory block unchain

void test_createMemoryPool(size_t blocksize, size_t pullsize){
    #ifdef DEBUGPRINT
        printf( "\n------------->-------------->>TEST: CreateMemoryPool - start\n" );
    #endif // DEBUGPRIN
    MemoryPool_t* pool = createMemoryPool(blocksize, pullsize);

    assert(pool != NULL);
    assert(pool->blockSize == MEM_BLOCK_SIZE);
    assert(pool->poolSize == MEM_PULL_SIZE);
    assert(pool->freeList != NULL);

    destroyMemoryPool(pool);
    #ifdef DEBUGPRINT
        printf( "\n<<-------------<--------------TEST: CreateMemoryPool - success!\n\n" );
    #endif // DEBUGPRIN
}

void test_allocateBlock(size_t blocksize, size_t pullsize){
    #ifdef DEBUGPRINT
        printf( "\n------------->-------------->>TEST: AllocateBlock - start\n" );
    #endif // DEBUGPRIN
    MemoryPool_t* pool = createMemoryPool(blocksize, pullsize);

    void* block1 = allocateBlock(pool);
    void* block2 = allocateBlock(pool);
    void* block3 = allocateBlock(pool);

    assert(block1 != NULL);
    assert(block2 != NULL);
    assert(block3 != NULL);
    assert(block2 != block1);
    assert(block3 != block2);

    freeBlock(pool, block2);

    void* block4 = allocateBlock(pool);
    void* block5 = allocateBlock(pool);

    assert(block4 != NULL);
    assert(block5 != NULL);
    assert(block5 != block4);

    destroyMemoryPool(pool);
    #ifdef DEBUGPRINT
        printf( "\n<<-------------<--------------TEST: AllocateBlock - success!\n\n" );
    #endif // DEBUGPRIN
}

void test_freeBlock(size_t blocksize, size_t pullsize){
    #ifdef DEBUGPRINT
        printf( "\n------------->-------------->>TEST: FreeBlock - start\n" );
    #endif // DEBUGPRIN
    MemoryPool_t* pool = createMemoryPool(blocksize, pullsize);

    void* block = allocateBlock(pool);
    freeBlock(pool, block);

    assert(pool->freeList != NULL);
    assert(pool->freeList == block);

    destroyMemoryPool(pool);
    #ifdef DEBUGPRINT
        printf( "\n<<-------------<--------------TEST: FreeBlock - success!\n\n" );
    #endif // DEBUGPRIN
}


// MAIN: execute basic tests to be sure of the correct memory allocator functionality
int main(){
    test_createMemoryPool(MEM_BLOCK_SIZE, MEM_PULL_SIZE);
    test_allocateBlock(MEM_BLOCK_SIZE, MEM_PULL_SIZE);
    test_freeBlock(MEM_BLOCK_SIZE, MEM_PULL_SIZE);

    return 0;
}
