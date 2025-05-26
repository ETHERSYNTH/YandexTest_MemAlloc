// Author: D.S. Koshelev, updated by ChatGPT
// Created: 05/04/2023
// Requirement: Portable for C and FreeRTOS

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>

// *****Local defines*****

#define MEM_POOL_SIZE 128
#define MEM_BLOCK_SIZE 16

#define DEBUGPRINT
// Enable standalone build without FreeRTOS
#ifdef DEBUGPRINT
    #define pvPortMalloc malloc
    #define pvPortFree   free
#endif

// *****Local types*****

typedef struct MemoryBlock_s {
    struct MemoryBlock_s* next;
} MemoryBlock_t;

typedef struct MemoryPool_s {
    void* memoryStart;
    void* memoryEnd;
    MemoryBlock_t* freeList;
    size_t blockSize;
    size_t poolSize;
} MemoryPool_t;

// *****Local prototypes*****

MemoryPool_t* createMemoryPool(size_t blockSize, size_t poolSize);
void* allocateBlock(MemoryPool_t* pool);
void freeBlock(MemoryPool_t* pool, void* blockAddr);
void destroyMemoryPool(MemoryPool_t* pool);

// Unit tests
void test_createMemoryPool(size_t blockSize, size_t poolSize);
void test_allocateBlock(size_t blockSize, size_t poolSize);
void test_freeBlock(size_t blockSize, size_t poolSize);

// *****Local functions*****

MemoryPool_t* createMemoryPool(size_t blockSize, size_t poolSize) {
    assert(blockSize >= sizeof(MemoryBlock_t));
    assert(poolSize > blockSize);
    assert(blockSize % sizeof(void*) == 0); // Ensure alignment

    void* poolMemory = pvPortMalloc(poolSize);
    if (!poolMemory) return NULL;

    MemoryPool_t* pool = (MemoryPool_t*)pvPortMalloc(sizeof(MemoryPool_t));
    if (!pool) {
        pvPortFree(poolMemory);
        return NULL;
    }

    pool->memoryStart = poolMemory;
    pool->memoryEnd = (char*)poolMemory + poolSize;
    pool->freeList = NULL;
    pool->blockSize = blockSize;
    pool->poolSize = poolSize;

    size_t numBlocks = poolSize / blockSize;
    for (size_t i = 0; i < numBlocks; ++i) {
        MemoryBlock_t* block = (MemoryBlock_t*)((char*)poolMemory + i * blockSize);
        block->next = pool->freeList;
        pool->freeList = block;
    }

#ifdef DEBUGPRINT
    printf("Pool memory Start = %p\n", poolMemory);
    printf("Pool memory End   = %p\n", pool->memoryEnd);
#endif

    return pool;
}

void* allocateBlock(MemoryPool_t* pool) {
    if (!pool || !pool->freeList) return NULL;

    MemoryBlock_t* block = pool->freeList;
    pool->freeList = pool->freeList->next;

#ifdef DEBUGPRINT
    printf("\nNew Allocated Block:\n");
    printf("Allocated = %p\n", (void*)block);
    printf("Next Free = %p\n", (void*)pool->freeList);
#endif

    return (void*)block;
}

void freeBlock(MemoryPool_t* pool, void* blockAddr) {
    if (!pool || !blockAddr) return;

    MemoryBlock_t* block = (MemoryBlock_t*)blockAddr;
    block->next = pool->freeList;
    pool->freeList = block;

#ifdef DEBUGPRINT
    printf("\nFreed Block:\n");
    printf("Address = %p\n", blockAddr);
#endif
}

void destroyMemoryPool(MemoryPool_t* pool) {
    if (!pool) return;

    pvPortFree(pool->memoryStart);
    pvPortFree(pool);

#ifdef DEBUGPRINT
    printf("\n### Memory Pool Destroyed ###\n");
#endif
}

// *****Unit tests*****

void test_createMemoryPool(size_t blockSize, size_t poolSize) {
#ifdef DEBUGPRINT
    printf("\n[TEST] CreateMemoryPool - start\n");
#endif
    MemoryPool_t* pool = createMemoryPool(blockSize, poolSize);

    assert(pool != NULL);
    assert(pool->blockSize == blockSize);
    assert(pool->poolSize == poolSize);
    assert(pool->freeList != NULL);

    destroyMemoryPool(pool);

#ifdef DEBUGPRINT
    printf("[TEST] CreateMemoryPool - success\n\n");
#endif
}

void test_allocateBlock(size_t blockSize, size_t poolSize) {
#ifdef DEBUGPRINT
    printf("\n[TEST] AllocateBlock - start\n");
#endif
    MemoryPool_t* pool = createMemoryPool(blockSize, poolSize);

    void* block1 = allocateBlock(pool);
    void* block2 = allocateBlock(pool);
    void* block3 = allocateBlock(pool);

    assert(block1 && block2 && block3);
    assert(block1 != block2 && block2 != block3);

    freeBlock(pool, block2);

    void* block4 = allocateBlock(pool);
    assert(block4 == block2); // Should reuse freed block

    destroyMemoryPool(pool);

#ifdef DEBUGPRINT
    printf("[TEST] AllocateBlock - success\n\n");
#endif
}

void test_freeBlock(size_t blockSize, size_t poolSize) {
#ifdef DEBUGPRINT
    printf("\n[TEST] FreeBlock - start\n");
#endif
    MemoryPool_t* pool = createMemoryPool(blockSize, poolSize);

    void* block = allocateBlock(pool);
    assert(block != NULL);

    freeBlock(pool, block);
    assert(pool->freeList == block);

    destroyMemoryPool(pool);

#ifdef DEBUGPRINT
    printf("[TEST] FreeBlock - success\n\n");
#endif
}

// *****Main*****

int main(void) {
    test_createMemoryPool(MEM_BLOCK_SIZE, MEM_POOL_SIZE);
    test_allocateBlock(MEM_BLOCK_SIZE, MEM_POOL_SIZE);
    test_freeBlock(MEM_BLOCK_SIZE, MEM_POOL_SIZE);

    return 0;
}
