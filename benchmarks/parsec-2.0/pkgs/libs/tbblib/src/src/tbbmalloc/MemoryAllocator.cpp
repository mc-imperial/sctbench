/*
    Copyright 2005-2008 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/


//#define MALLOC_TRACE

#ifdef MALLOC_TRACE
#define TRACEF printf
#else
static inline int TRACEF(const char *arg, ...)
{
    return 0;
}
#endif /* MALLOC_TRACE */

#define ASSERT_TEXT NULL

#include "TypeDefinitions.h"

#if USE_PTHREAD
    // Some pthreads documentation says that <pthreads.h> must be first header.
    #include <pthread.h>
    #define TlsSetValue_func pthread_setspecific
    #define TlsGetValue_func pthread_getspecific
    typedef pthread_key_t tls_key_t;
    #include <sched.h>
    inline void do_yield() {sched_yield();}

#elif USE_WINTHREAD
    #define _WIN32_WINNT 0x0400
    #include <windows.h>
    #define TlsSetValue_func TlsSetValue
    #define TlsGetValue_func TlsGetValue
    typedef DWORD tls_key_t;
    inline void do_yield() {SwitchToThread();}

#else
    #error Must define USE_PTHREAD or USE_WINTHREAD

#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>

#if __linux__||__APPLE__ || __FreeBSD__ || __sun
#include <fcntl.h>
#endif

/**
*** Define various compile-time options
**/

//! Define the main syncronization method
/** It should be specified before including LifoQueue.h */
#define FINE_GRAIN_LOCKS
#include "LifoQueue.h"

#define COLLECT_STATISTICS 0
#include "Statistics.h"

#define FREELIST_NONBLOCKING 1

// If USE_MALLOC_FOR_LARGE_OBJECT is nonzero, then large allocations are done via malloc.
// Otherwise large allocations are done using the scalable allocator's block allocator.
// As of 06.Jun.17, using malloc is about 10x faster on Linux.
#define USE_MALLOC_FOR_LARGE_OBJECT 1

#if USE_MALLOC_FOR_LARGE_OBJECT
#include <stdlib.h>
#else
extern "C" void exit( int ); /* to not include stdlib.h */
#endif /* USE_MALLOC_FOR_LARGE_OBJECT */


namespace ThreadingSubstrate {

namespace Internal {

/*********** Code to provide thread ID and a thread-local void pointer **********/

typedef intptr_t ThreadId;

static ThreadId ThreadIdCount;

static tls_key_t TLS_pointer_key;
static tls_key_t Tid_key;

static inline ThreadId  getThreadId(void)
{
    ThreadId result;
    result = reinterpret_cast<ThreadId>(TlsGetValue_func(Tid_key));
    if( !result ) {
        // Thread-local value is zero -> first call from this thread,
        // need to initialize with next ID value (IDs start from 1)
        result = AtomicIncrement(ThreadIdCount); // returned new value!
        TlsSetValue_func( Tid_key, reinterpret_cast<void*>(result) );
    }
    return result;
}

static inline void* getThreadMallocTLS() {
    void *result;
    result = TlsGetValue_func( TLS_pointer_key );
// The assert below is incorrect: with lazy initialization, it fails on the first call of the function.
//    MALLOC_ASSERT( result, "Memory allocator not initialized" );
    return result;
}

static inline void  setThreadMallocTLS( void * newvalue ) {
    TlsSetValue_func( TLS_pointer_key, newvalue );
}

/*********** End code to provide thread ID and a TLS pointer **********/

/********** Various numeric parameters controlling allocations ********/

/*
 * There are bins for all 8 byte aligned objects less than this segregated size; 8 bins in total
 */
static const uint32_t maxSmallObjectSize = 64;

/*
 * There are 4 bins between each couple of powers of 2 [64-128-256-...]
 * from maxSmallObjectSize till this size; 16 bins in total
 */
static const uint32_t maxSegregatedObjectSize = 1024;

/*
 * And there are 5 bins with the following allocation sizes: 1792, 2688, 3968, 5376, 8064.
 * They selected to fit 9, 6, 4, 3, and 2 sizes per a block, and also are multiples of 128.
 * If sizeof(Block) changes from 128, these sizes require close attention!
 */
static const uint32_t fittingSize1 = 1792;
static const uint32_t fittingSize2 = 2688;
static const uint32_t fittingSize3 = 3968;
static const uint32_t fittingSize4 = 5376;
static const uint32_t fittingSize5 = 8064;

/*
 * Objects of this size and larger are considered large objects.
 */
static const uint32_t minLargeObjectSize = fittingSize5 + 1;

/*
 * This number of bins in the TLS that leads to blocks that we can allocate in.
 */
static const uint32_t numBlockBinLimit = 32;
static const uint32_t numBlockBins = 29;

/*
 * blockSize - the size of a block, it must be larger than maxSegregatedObjectSize.
 * we may well want to play around with this, a 4K page size is another interesting size.
 *
 */
static const uintptr_t blockSize = 16384;

/*
 * Get virtual memory in pieces of this size: 0x0100000 is 1 megabyte decimal
 */
static size_t mmapRequestSize = 0x0100000;

/********** End of numeric parameters controlling allocations *********/

/********* The data structures and global objects        **************/

typedef struct FreeObject *FreeObjectPtr;

struct FreeObject {
    FreeObjectPtr  next;
};

/*
 * The following constant is used to define the size of struct Block, the block header.
 * The intent is to have the size of a Block multiple of the cache line size, this allows us to
 * get good alignment at the cost of some overhead equal to the amount of padding included in the Block.
  */

#define ALIGNED_SIZE 64 // 64 is a common size of a cache line

typedef struct Block* BlockPtr;

/* The 'next' field in the block header has to maintain some invariants:
 *   it needs to be on a 16K boundary and the first field in the block.
 *   Any value stored there needs to have the lower 14 bits set to 0
 *   so that various assert work. This means that if you want to smash this memory
 *   for debugging purposes you will need to obey this invariant.
 * The total size of the header needs to be a power of 2 to simplify
 * the alignement requirements. For now it is a 128 byte structure.
 * To avoid false sharing, the fields changed only locally are separated 
 * from the fields changed by foreign threads.
 * Changing the size of the block header would require to change
 * some bin allocation sizes, in particular "fitting" sizes (see above).
 */

struct LocalBlockFields {
    BlockPtr     next;     /* This field needs to be on a 16K boundary and the first field in the block
                              so non-blocking LifoQueues will work. */
    BlockPtr     previous; /* Use double linked list to speed up removal */
    unsigned int objectSize;
    unsigned int owner;
    FreeObject  *bumpPtr;         /* bump pointer moves from the end to the beginning of a block */
    FreeObject  *freeList;
    unsigned int allocatedCount;  /* Number of objects allocated (obviously by the owning thread) */
    unsigned int isFull;
};

struct Block : public LocalBlockFields {
    size_t       __pad_local_fields[(ALIGNED_SIZE-sizeof(LocalBlockFields))/sizeof(size_t)];
    FreeObject  *publicFreeList;
    BlockPtr     nextPrivatizable;
    size_t       __pad_public_fields[(ALIGNED_SIZE-2*sizeof(void*))/sizeof(size_t)];
};

struct Bin {
    BlockPtr  activeBlk;
    BlockPtr  mailbox;
    MallocMutex mailLock;
};

/*
 * The size of the TLS should be enough to hold twice as many as numBlockBinLimit pointers
 * the first sequence of pointers is for lists of blocks to allocate from
 * the second sequence is for lists of blocks that have non-empty publicFreeList
 */
static const uint32_t tlsSize = numBlockBinLimit * sizeof(Bin);

/*
 * This is a lifo queue that one can init, push or pop from */
static LifoQueue freeBlockQueue;

/*
 * When a block that is not completely free is returned for reuse by other threads
 * this is where the block goes.
 *
 */

static char globalBinSpace[sizeof(LifoQueue)*numBlockBinLimit];
static LifoQueue* globalSizeBins = (LifoQueue*)globalBinSpace;

/********* End of the data structures                    **************/

/*********** Code to acquire memory from the OS or other executive ****************/

#if USE_DEFAULT_MEMORY_MAPPING
#include "MapMemory.h"
#else
/* assume MapMemory and UnmapMemory are customized */
#endif

/*
 * Returns 0 if failure for any reason
 * otherwise returns a pointer to the newly available memory.
 */
static void* getMemory (size_t bytes)
{
    void *result = 0;
    MALLOC_ASSERT( bytes>=minLargeObjectSize, "request too small" );
    result = MapMemory(bytes);
#ifdef MALLOC_TRACE
    if (!result) {
        TRACEF("ScalableMalloc trace - getMemory unsuccess, can't get %d bytes from OS\n", bytes);
    } else {
        TRACEF("ScalableMalloc trace - getMemory success returning %p\n", result);
    }
#endif
    return result;
}

static void returnMemory(void *area, size_t bytes)
{
    int retcode = UnmapMemory(area, bytes);
#ifdef MALLOC_TRACE
    if (retcode) {
        TRACEF("ScalableMalloc trace - returnMemory unsuccess for %p; perhaps it has already been freed or was never allocated.\n", area);
    }
#endif
    return;
}

/********* End memory acquisition code ********************************/

/********* Now some rough utility code to deal with indexing the size bins. **************/

/*
 * Given a number return the highest non-zero bit in it. It is intended to work with 32-bit values only.
 * Moreover, on IPF, for sake of simplicity and performance, it is narrowed to only serve for 64 to 1023.
 * This is enough for current algorithm of distribution of sizes among bins.
 */
#if _WIN64 && _MSC_VER>=1400 && !__INTEL_COMPILER
extern "C" unsigned char _BitScanReverse( unsigned long* i, unsigned long w );
#pragma intrinsic(_BitScanReverse)
#endif
static inline unsigned int highestBitPos(unsigned int number)
{
    unsigned int pos;
#if __ARCH_x86_32||__ARCH_x86_64

# if __linux__||__APPLE__||__FreeBSD__ || __sun
    __asm__ ("bsr %1,%0" : "=r"(pos) : "r"(number));
# elif (_WIN32 && (!_WIN64 || __INTEL_COMPILER))
    __asm
    {
        bsr eax, number
        mov pos, eax
    }
# elif _WIN64 && _MSC_VER>=1400
    _BitScanReverse((unsigned long*)&pos, (unsigned long)number);
# else
#   error highestBitPos() not implemented for this platform
# endif

#elif __ARCH_ipf || __ARCH_other
    static unsigned int bsr[16] = {0,6,7,7,8,8,8,8,9,9,9,9,9,9,9,9};
    MALLOC_ASSERT( number>=64 && number<1024, ASSERT_TEXT );
    pos = bsr[ number>>6 ];
#else
#   error highestBitPos() not implemented for this platform
#endif
    return pos;
}

/*
 * Given a size return the index into the bins for object of this size.
 *
 */
static unsigned int getIndex (unsigned int size)
{
    // If this fails then getIndex has problems since it doesn't return the correct index all the time.
    if (size <= maxSmallObjectSize) {
        return ((size - 1) >> 3); /* Index 0 holds up to 8 bytes, Index 1 16 and so forth */
    }
    else if (size <= maxSegregatedObjectSize ) {
        unsigned order = highestBitPos(size-1);
        MALLOC_ASSERT( order>=6 && order<=9, ASSERT_TEXT );
        return ( (size-1)>>(order-2) ) + 4*(order-5);
    }
    else {
        if( size <= fittingSize3 ) {
            if( size <= fittingSize2 ) {
                if( size <= fittingSize1 )
                    return 24;
                else
                    return 25;
            } else
                return 26;
        } else {
            if( size <= fittingSize5 ) {
                if( size <= fittingSize4 )
                    return 27;
                else
                    return 28;
            } else {
                MALLOC_ASSERT( 0,ASSERT_TEXT ); // this should not happen
                return (unsigned)-1;
            }
        }
    }
}

/*
 * Given a size return the size of the object in the block used to
 * allocate such an object.
 *
 */
static unsigned int getAllocatedSize (unsigned int size)
{
    // If this fails then get_index has problems since it doesn't return the correct index all the time.
    if (size <= maxSmallObjectSize) {
        return ((size + 7) & ~7); /* Index 0 holds up to 8 bytes, Index 1 16 and so forth */
    }
    else if (size <= maxSegregatedObjectSize) {
        unsigned aligner = 127 >> (9-highestBitPos(size-1));
        MALLOC_ASSERT( aligner==127 || aligner==63 || aligner==31 || aligner==15, ASSERT_TEXT );
        return (size+aligner)&~aligner;
    }
    else {
        if( size <= fittingSize3 ) {
            if( size <= fittingSize2 ) {
                if( size <= fittingSize1 )
                    return fittingSize1;
                else
                    return fittingSize2;
            } else
                return fittingSize3;
        } else {
            if( size <= fittingSize5 ) {
                if( size <= fittingSize4 )
                    return fittingSize4;
                else
                    return fittingSize5;
            } else {
                MALLOC_ASSERT( 0,ASSERT_TEXT ); // this should not happen
                return (unsigned)-1;
            }
        }
    }
}

/*
 * Initialization code.
 *
 */

/*
 * Big Blocks are the blocks we get from the OS or some similar place using getMemory above.
 * They are placed on the freeBlockQueue once they are acquired.
 */

static inline void *alignBigBlock(void *unalignedBigBlock)
{
    void *alignedBigBlock;
    /* align the entireHeap so all blocks are aligned. */
    alignedBigBlock = (void *) ( ((uintptr_t)unalignedBigBlock + blockSize - 1) & ~(blockSize - 1) );
    return alignedBigBlock;
}

/* Divide the big block into smaller bigBlocks that hold this many blocks.
 * This is done since we really need a lot of blocks on the freeBlockQueue or there will be
 * contention problems.
 */
static const unsigned int blocksPerBigBlock = 16;

/* Returns 0 if unsuccessful, otherwise 1. */
static int mallocBigBlock()
{
    void *unalignedBigBlock;
    void *alignedBigBlock;
    void *bigBlockCeiling;
    Block *splitBlock;
    void *splitEdge;
    size_t bigBlockSplitSize;

    unalignedBigBlock = getMemory(mmapRequestSize);

    if (!unalignedBigBlock) {
        TRACEF(" in mallocBigBlock, getMemory returns 0\n");
        /* We can't get any more memory from the OS or executive so return 0 */
        return 0;
    }

    alignedBigBlock = alignBigBlock(unalignedBigBlock);
    bigBlockCeiling = (void*)((uintptr_t)unalignedBigBlock + mmapRequestSize);

    bigBlockSplitSize = blocksPerBigBlock * blockSize;

    splitBlock = (Block*)alignedBigBlock;

    while ( ((uintptr_t)splitBlock + blockSize) <= (uintptr_t)bigBlockCeiling ) {
        splitEdge = (void*)((uintptr_t)splitBlock + bigBlockSplitSize);
        if( splitEdge > bigBlockCeiling) {
            // align down to blockSize
            splitEdge = (void*)((uintptr_t)bigBlockCeiling & ~(blockSize - 1));
        }
        splitBlock->bumpPtr = (FreeObject*)splitEdge;
        freeBlockQueue.push((void**) splitBlock);
        splitBlock = (Block*)splitEdge;
    }

    TRACEF("in mallocBigBlock returning 1\n");
    return 1;
}

/*
 * The malloc routines themselves need to be able to occasionally malloc some space,
 * in order to set up the structures used by the thread local structures. This
 * routine preforms that fuctions.
 */

/*
 * Forward Refs
 */
static void initEmptyBlock(Block *block, size_t size);
static Block *getEmptyBlock(size_t size);

static MallocMutex bootStrapBlockLock;

static Block *bootStrapBlock = NULL;
static Block *bootStrapBlockUsed = NULL;

static void *bootStrapMalloc(size_t size)
{
    void *result;

    MALLOC_ASSERT( size == tlsSize, ASSERT_TEXT );

    { // Lock with acquire
        MallocMutex::scoped_lock scoped_cs(bootStrapBlockLock);

        if (!bootStrapBlock) {
            bootStrapBlock = getEmptyBlock(size);
        }

        result = bootStrapBlock->bumpPtr;
        bootStrapBlock->bumpPtr = (FreeObject *)((uintptr_t)bootStrapBlock->bumpPtr - bootStrapBlock->objectSize);
        if ((uintptr_t)bootStrapBlock->bumpPtr < (uintptr_t)bootStrapBlock+sizeof(Block)) {
            bootStrapBlock->bumpPtr = NULL;
            bootStrapBlock->next = bootStrapBlockUsed;
            bootStrapBlockUsed = bootStrapBlock;
            bootStrapBlock = NULL;
        }

    } // Unlock with release

    memset (result, 0, size);
    return result;
}

/********* End rough utilitiy code  **************/

/********* Thread and block related code      *************/

#ifdef MALLOC_DEBUG
/* The debug version verifies the TLSBin as needed */
static void verifyTLSBin (Bin* bin, size_t size)
{
    Block* temp;
    Bin*   tls;
    uint32_t index = getIndex(size);
    uint32_t objSize = getAllocatedSize(size);

    tls = (Bin*)getThreadMallocTLS();
    MALLOC_ASSERT( bin == tls+index, ASSERT_TEXT );

    if (tls[index] && tls[index].activeBlk) {
        MALLOC_ASSERT( tls[index].activeBlk->owner == getThreadId(), ASSERT_TEXT );
        MALLOC_ASSERT( tls[index].activeBlk->objectSize == objSize, ASSERT_TEXT );

        for (temp = tls[index].activeBlk->next; temp; temp=temp->next) {
            MALLOC_ASSERT( temp!=tls[index].activeBlk, ASSERT_TEXT );
            MALLOC_ASSERT( temp->owner == getThreadId(), ASSERT_TEXT );
            MALLOC_ASSERT( temp->objectSize == objSize, ASSERT_TEXT );
            MALLOC_ASSERT( temp->previous->next == temp, ASSERT_TEXT );
            if (temp->next) {
                MALLOC_ASSERT( temp->next->previous == temp, ASSERT_TEXT );
            }
        }
        for (temp = tls[index].activeBlk->previous; temp; temp=temp->previous) {
            MALLOC_ASSERT( temp!=tls[index], ASSERT_TEXT );
            MALLOC_ASSERT( temp->owner == getThreadId(), ASSERT_TEXT );
            MALLOC_ASSERT( temp->objectSize == objSize, ASSERT_TEXT );
            MALLOC_ASSERT( temp->next->previous == temp, ASSERT_TEXT );
            if (temp->previous) {
                MALLOC_ASSERT( temp->previous->next == temp, ASSERT_TEXT );
            }
        }
    }
}
#else
inline static void verifyTLSBin (Bin*, size_t) {}
#endif

/*
 * Add a block to the start of this tls bin list.
 */
static void pushTLSBin (Bin* bin, Block* block)
{
    /* The objectSize should be defined and not a parameter
       because the function is applied to partially filled blocks as well */
    unsigned int size = block->objectSize;
    Block* activeBlk;

    MALLOC_ASSERT( block->owner == getThreadId(), ASSERT_TEXT );
    MALLOC_ASSERT( block->objectSize != 0, ASSERT_TEXT );
    MALLOC_ASSERT( block->next == NULL, ASSERT_TEXT );
    MALLOC_ASSERT( block->previous == NULL, ASSERT_TEXT );

    MALLOC_ASSERT( bin, ASSERT_TEXT );
    verifyTLSBin(bin, size);
    activeBlk = bin->activeBlk;

    block->next = activeBlk;
    if( activeBlk ) {
        block->previous = activeBlk->previous;
        activeBlk->previous = block;
        if( block->previous )
            block->previous->next = block;
    } else {
        bin->activeBlk = block;
    }

    verifyTLSBin(bin, size);
}

/*
 * Take a block out of its tls bin (e.g. before removal).
 */
static void outofTLSBin (Bin* bin, Block* block)
{
    unsigned int size = block->objectSize;

    MALLOC_ASSERT( block->owner == getThreadId(), ASSERT_TEXT );
    MALLOC_ASSERT( block->objectSize != 0, ASSERT_TEXT );

    MALLOC_ASSERT( bin, ASSERT_TEXT );
    verifyTLSBin(bin, size);

    if (block == bin->activeBlk) {
        bin->activeBlk = block->previous? block->previous : block->next;
    }
    /* Delink the block */
    if (block->previous) {
        MALLOC_ASSERT( block->previous->next == block, ASSERT_TEXT );
        block->previous->next = block->next;
    }
    if (block->next) {
        MALLOC_ASSERT( block->next->previous == block, ASSERT_TEXT );
        block->next->previous = block->previous;
    }
    block->next = NULL;
    block->previous = NULL;

    verifyTLSBin(bin, size);
}

/*
 * init the TLS bins and return the bin,
 * an array of *blocks initialized to NULL.
 */
static Bin* initMallocTLS (void)
{
    Bin* tls;
    int i;
    MALLOC_ASSERT( tlsSize >= sizeof(Bin) * numBlockBins, ASSERT_TEXT );
    tls = (Bin*) bootStrapMalloc(tlsSize);
    /* the block contains zeroes after bootStrapMalloc, so bins are initialized */
#ifdef MALLOC_DEBUG
    for (i = 0; i < numBlockBinLimit; i++) {
        MALLOC_ASSERT( tls[i].activeBlk == 0, ASSERT_TEXT );
        MALLOC_ASSERT( tls[i].mailbox == 0, ASSERT_TEXT );
    }
#endif
    setThreadMallocTLS(tls);
    return tls;
}

static Bin* getAllocationBin(size_t size)
{
    uint32_t index;
    Bin*   tls;

    index = getIndex(size);
    tls = (Bin*)getThreadMallocTLS();
    if( !tls ) {
        tls = initMallocTLS();
    }
    MALLOC_ASSERT(tls, ASSERT_TEXT);
    return tls+index;
}

static const float emptyEnoughRatio = 1.0 / 4.0; /* "Reactivate" a block if this share of its objects is free. */

static unsigned int emptyEnoughToUse (Block *mallocBlock)
{
    const float threshold = (blockSize - sizeof(Block)) * (1-emptyEnoughRatio);

    if (mallocBlock->bumpPtr) {
        /* If we are still using a bump ptr for this block it is empty enough to use. */
        STAT_increment(mallocBlock->owner, getIndex(mallocBlock->objectSize), examineEmptyEnough);
        mallocBlock->isFull = 0;
        return 1;
    }

    /* allocatedCount shows how many objects in the block are in use; however it still counts
       blocks freed by other threads; so prior call to privatizePublicFreeList() is recommended */
    mallocBlock->isFull = (mallocBlock->allocatedCount*mallocBlock->objectSize > threshold)? 1: 0;
#if COLLECT_STATISTICS
    if (mallocBlock->isFull)
        STAT_increment(mallocBlock->owner, getIndex(mallocBlock->objectSize), examineNotEmpty);
    else
        STAT_increment(mallocBlock->owner, getIndex(mallocBlock->objectSize), examineEmptyEnough);
#endif
    return 1-mallocBlock->isFull;
}

/* Restore the bump pointer for an empty block that is planned to use */
static void restoreBumpPtr (Block *block)
{
    MALLOC_ASSERT (block->allocatedCount == 0, ASSERT_TEXT);
    MALLOC_ASSERT (block->publicFreeList == NULL, ASSERT_TEXT);
    STAT_increment(block->owner, getIndex(block->objectSize), freeRestoreBumpPtr);
    block->bumpPtr = (FreeObject *)((uintptr_t)block + blockSize - block->objectSize);
    block->freeList = NULL;
    block->isFull = 0;
}

// #ifdef FINE_GRAIN_LOCKS

#if !(FREELIST_NONBLOCKING)
static MallocMutex publicFreeListLock; // lock for changes of publicFreeList
#endif

const uintptr_t UNUSABLE = 0x1;
inline bool isSolidPtr( void* ptr )
{
    return (UNUSABLE|(uintptr_t)ptr)!=UNUSABLE;
}
inline bool isNotForUse( void* ptr )
{
    return (uintptr_t)ptr==UNUSABLE;
}

static void freePublicObject (Block *block, FreeObject *objectToFree)
{
    Bin* theBin;
    FreeObject *publicFreeList;

#if FREELIST_NONBLOCKING
    FreeObject *temp = block->publicFreeList;
    MALLOC_ITT_SYNC_RELEASING(&block->publicFreeList);
    do {
        publicFreeList = objectToFree->next = temp;
        temp = (FreeObject*)AtomicCompareExchange(
                                (intptr_t&)block->publicFreeList,
                                (intptr_t)objectToFree, (intptr_t)publicFreeList );
        //no backoff necessary because trying to make change, not waiting for a change
    } while( temp != publicFreeList );
#else
    STAT_increment(getThreadId(), ThreadCommonCounters, lockPublicFreeList);
    {
        MallocMutex::scoped_lock scoped_cs(publicFreeListLock);
        publicFreeList = objectToFree->next = block->publicFreeList;
        block->publicFreeList = objectToFree;
    }
#endif

    if( publicFreeList==NULL ) {
        // if the block is abandoned, its nextPrivatizable pointer should be UNUSABLE
        // otherwise, it should point to the bin the block belongs to.
        // reading nextPrivatizable is thread-safe below, because:
        // 1) the executing thread atomically got publicFreeList==NULL and changed it to non-NULL;
        // 2) only owning thread can change it back to NULL,
        // 3) but it can not be done until the block is put to the mailbox
        // So the executing thread is now the only one that can change nextPrivatizable
        if( !isNotForUse(block->nextPrivatizable) ) {
            MALLOC_ASSERT(block->nextPrivatizable!=NULL, ASSERT_TEXT);
            MALLOC_ASSERT(block->owner!=0, ASSERT_TEXT);
            theBin = (Bin*) block->nextPrivatizable;
#ifdef MALLOC_DEBUG
            { // check that nextPrivatizable points to the bin the block belongs to
                uint32_t index = getIndex( block->objectSize );
                Bin* tls = (Bin*)getThreadMallocTLS();
                MALLOC_ASSERT(theBin==tls+index, ASSERT_TEXT);
            }
#endif
// the counter should be changed            STAT_increment(getThreadId(), ThreadCommonCounters, lockPublicFreeList);
            MallocMutex::scoped_lock scoped_cs(theBin->mailLock);
            block->nextPrivatizable = theBin->mailbox;
            theBin->mailbox = block;
        } else {
            MALLOC_ASSERT(block->owner==0, ASSERT_TEXT);
        }
    }
    STAT_increment(getThreadId(), ThreadCommonCounters, freeToOtherThread);
    STAT_increment(block->owner, getIndex(block->objectSize), freeByOtherThread);
}

static void privatizePublicFreeList (Block *mallocBlock)
{
    FreeObject *temp, *publicFreeList;

    MALLOC_ASSERT( mallocBlock->owner == getThreadId(), ASSERT_TEXT );
#if FREELIST_NONBLOCKING
    temp = mallocBlock->publicFreeList;
    do {
        publicFreeList = temp;
        temp = (FreeObject*)AtomicCompareExchange(
                                (intptr_t&)mallocBlock->publicFreeList,
                                0, (intptr_t)publicFreeList);
        //no backoff necessary because trying to make change, not waiting for a change
    } while( temp != publicFreeList );
    MALLOC_ITT_SYNC_ACQUIRED(&mallocBlock->publicFreeList);
#else
    STAT_increment(mallocBlock->owner, ThreadCommonCounters, lockPublicFreeList);
    {
        MallocMutex::scoped_lock scoped_cs(publicFreeListLock);
        publicFreeList = mallocBlock->publicFreeList;
        mallocBlock->publicFreeList = NULL;
    }
    temp = publicFreeList;
#endif

    MALLOC_ASSERT(publicFreeList && publicFreeList==temp, ASSERT_TEXT); // there should be something in publicFreeList!
    if( !isNotForUse(temp) ) { // return/getPartialBlock could set it to UNUSABLE
        MALLOC_ASSERT(mallocBlock->allocatedCount <= (blockSize-sizeof(Block))/mallocBlock->objectSize, ASSERT_TEXT);
        /* other threads did not change the counter freeing our blocks */
        mallocBlock->allocatedCount--;
        while( isSolidPtr(temp->next) ){ // the list will end with either NULL or UNUSABLE
            temp = temp->next;
            mallocBlock->allocatedCount--;
        }
        MALLOC_ASSERT(mallocBlock->allocatedCount < (blockSize-sizeof(Block))/mallocBlock->objectSize, ASSERT_TEXT);
        /* merge with local freeList */
        temp->next = mallocBlock->freeList;
        mallocBlock->freeList = publicFreeList;
        STAT_increment(mallocBlock->owner, getIndex(mallocBlock->objectSize), allocPrivatized);
    }
}

static Block* getPublicFreeListBlock (Bin* bin)
{
    Block* block;
    MALLOC_ASSERT( bin, ASSERT_TEXT );
// the counter should be changed    STAT_increment(getThreadId(), ThreadCommonCounters, lockPublicFreeList);
    {
        MallocMutex::scoped_lock scoped_cs(bin->mailLock);
        block = bin->mailbox;
        if( block ) {
            MALLOC_ASSERT( block->owner == getThreadId(), ASSERT_TEXT );
            MALLOC_ASSERT( !isNotForUse(block->nextPrivatizable), ASSERT_TEXT );
            bin->mailbox = block->nextPrivatizable;
            block->nextPrivatizable = (Block*) bin;
        }
    }
    if( block ) {
        MALLOC_ASSERT( isSolidPtr(block->publicFreeList), ASSERT_TEXT );
        privatizePublicFreeList(block);
    }
    return block;
}

static Block *getPartialBlock(Bin* bin, unsigned int size)
{
    Block *result;
    MALLOC_ASSERT(bin, ASSERT_TEXT);
    unsigned int index = getIndex(size);
    result = (Block *) globalSizeBins[index].pop();
    if (result) {
        result->next = NULL;
        result->previous = NULL;
        MALLOC_ASSERT( result->publicFreeList!=NULL, ASSERT_TEXT );
        /* There is not a race here since no other thread owns this block */
        MALLOC_ASSERT(result->owner == 0, ASSERT_TEXT);
        result->owner = getThreadId();
        // It is safe to change nextPrivatizable, as publicFreeList is not null
        MALLOC_ASSERT(isNotForUse(result->nextPrivatizable), ASSERT_TEXT);
        result->nextPrivatizable = (Block*)bin;
        // the next call is required to change publicFreeList to 0
        privatizePublicFreeList(result);
        if( result->allocatedCount ) {
            // check its fullness and set result->isFull
            emptyEnoughToUse(result);
        } else {
            restoreBumpPtr(result);
        }
        MALLOC_ASSERT( !isNotForUse(result->publicFreeList), ASSERT_TEXT );
        STAT_increment(result->owner, index, allocBlockPublic);
    }
    return result;
}

static void returnPartialBlock(Bin* bin, Block *block)
{
    unsigned int index = getIndex(block->objectSize);
    MALLOC_ASSERT(bin, ASSERT_TEXT);
    MALLOC_ASSERT (block->owner==getThreadId(), ASSERT_TEXT);
    STAT_increment(block->owner, index, freeBlockPublic);
    // need to set publicFreeList to non-zero, so other threads
    // will not change nextPrivatizable and it can be zeroed.
    if ((intptr_t)block->nextPrivatizable==(intptr_t)bin) {
        void* oldval;
#if FREELIST_NONBLOCKING
        oldval = (void*)AtomicCompareExchange((intptr_t&)block->publicFreeList, (intptr_t)UNUSABLE, 0);
#else
        STAT_increment(block->owner, ThreadCommonCounters, lockPublicFreeList);
        {
            MallocMutex::scoped_lock scoped_cs(publicFreeListLock);
            if ( (oldval=block->publicFreeList)==NULL )
                (uintptr_t&)(block->publicFreeList) = UNUSABLE;
        }
#endif
        if ( oldval!=NULL ) {
            // another thread freed an object; we need to wait until it finishes.
            // I believe there is no need for exponential backoff, as the wait here is not for a lock;
            // but need to yield, so the thread we wait has a chance to run.
            while( (intptr_t)const_cast<Block* volatile &>(block->nextPrivatizable)==(intptr_t)bin ) {
                int count = 256;
                if (--count==0) {
                    do_yield();
                    count = 256;
                }
            }
        }
    } else {
        MALLOC_ASSERT(isSolidPtr(block->publicFreeList), ASSERT_TEXT);
    }
    MALLOC_ASSERT( block->publicFreeList!=NULL, ASSERT_TEXT );
    // now it is safe to change our data
    block->previous = NULL;
    block->owner = 0;
    /* it is caller responsibility to ensure that the list of blocks
     * formed by nextPrivatizable pointers is kept consistent if required.
     * if only called from thread shutdown code, it does not matter */
    (uintptr_t&)(block->nextPrivatizable) = UNUSABLE;
    globalSizeBins[index].push((void **)block);
}

// #endif /* FINE_GRAIN_LOCKS */

static void initEmptyBlock(Block *block, size_t size)
{
    unsigned int allocatedSize = getAllocatedSize(size);
    Bin* tls = (Bin*)getThreadMallocTLS();
//    unsigned int allocationSpace = blockSize - sizeof(Block);
//    unsigned int objectsInBlock = allocationSpace / allocatedSize;
#ifdef MALLOC_DEBUG
    memset (block, 0x0e5, blockSize);
#endif
    block->next = NULL;
    block->previous = NULL;
    // bump pointer should be prepared for first allocation - thus mode it down to allocatedSize
    block->bumpPtr = (FreeObject *)((uintptr_t)block + blockSize - allocatedSize);
    // each block should have the address where the head of the list of "privatizable" blocks is kept
    // the only exception is a block for boot strap which is initialized when TLS is yet NULL
    block->nextPrivatizable = tls? (Block*)(tls + getIndex(size)) : NULL;
    block->allocatedCount = 0;
    block->freeList = NULL;
    block->publicFreeList = NULL;
    block->isFull = 0;
    block->objectSize = allocatedSize;
    block->owner = getThreadId();
    TRACEF ("Empty block %p is initialized, owner is %d, objectSize is %d, bumpPtr is %p\n",
        block, block->owner, block->objectSize, block->bumpPtr);
  }

/* Return an empty uninitialized block in a non-blocking fashion. */
static Block *getEmptyBlock(size_t size)
{
    Block *result;
    Block *bigBlock;
    int success;

    result = NULL;

    bigBlock = (Block *) freeBlockQueue.pop();

    while (!bigBlock) {
        /* We are out of blocks so got to the OS and get another one */
        success = mallocBigBlock();
        if (!success) {
            return NULL;
        }
        bigBlock = (Block *) freeBlockQueue.pop();
    }

    // check alignment
    MALLOC_ASSERT( !( (uintptr_t)bigBlock & (uintptr_t)(blockSize-1) ), ASSERT_TEXT);
    MALLOC_ASSERT( !( (uintptr_t)bigBlock->bumpPtr & (uintptr_t)(blockSize-1) ), ASSERT_TEXT);
    // block should be at least as big as blockSize; otherwise the previous block can be damaged.
    MALLOC_ASSERT( (uintptr_t)bigBlock->bumpPtr >= (uintptr_t)bigBlock + blockSize, ASSERT_TEXT);
    bigBlock->bumpPtr = (FreeObject *)((uintptr_t)bigBlock->bumpPtr - blockSize);
    result = (Block *)bigBlock->bumpPtr;
    if ( result!=bigBlock ) {
        TRACEF("Pushing partial rest of block back on.\n");
        freeBlockQueue.push((void **)bigBlock);
    }
    initEmptyBlock(result, size);
    STAT_increment(result->owner, getIndex(result->objectSize), allocBlockNew);

    return result;
}

/* We have a block give it back to the malloc block manager */
static void returnEmptyBlock (Block *block, bool keepTheBin = true)
{
    // it is caller's responsibility to ensure no data is lost before calling this
    MALLOC_ASSERT(block->allocatedCount==0, ASSERT_TEXT);
    MALLOC_ASSERT(block->publicFreeList==NULL, ASSERT_TEXT);
    if (keepTheBin) {
        /* We should keep the TLS bin structure */
        MALLOC_ASSERT( block->next == NULL, ASSERT_TEXT );
        MALLOC_ASSERT( block->previous == NULL, ASSERT_TEXT );
    }
    STAT_increment(block->owner, getIndex(block->objectSize), freeBlockBack);

    block->allocatedCount = 0;
    // for an empty block, bump pointer should point right after the end of the block
    block->bumpPtr = (FreeObject *)((uintptr_t)block + blockSize);
    block->nextPrivatizable = NULL;
    block->freeList = NULL;
    block->publicFreeList = NULL;
    block->next = NULL;
    block->previous = NULL;
    block->owner = (unsigned)-1;
    block->isFull = 0;
    block->objectSize = 0;
    freeBlockQueue.push((void **)block);
}

inline static Block* getActiveBlock( Bin* bin )
{
    MALLOC_ASSERT( bin, ASSERT_TEXT );
    return bin->activeBlk;
}

inline static void setActiveBlock (Bin* bin, Block *block)
{
    MALLOC_ASSERT( bin, ASSERT_TEXT );
    MALLOC_ASSERT(block->owner == getThreadId(), ASSERT_TEXT);
    // it is the caller responsibility to keep bin consistence (i.e. ensure this block is in the bin list)
    bin->activeBlk = block;
}

inline static Block* setPreviousBlockActive( Bin* bin )
{
    MALLOC_ASSERT( bin && bin->activeBlk, ASSERT_TEXT );
    Block* temp = bin->activeBlk->previous;
    if( temp ) {
        MALLOC_ASSERT( temp->isFull == 0, ASSERT_TEXT );
        bin->activeBlk = temp;
    }
    return temp;
}

/********* End thread related code  *************/

/********* The malloc show          *************/

/*
 * The program wants a large object that we are not prepared to deal with.
 * so we pass the problem on to the OS. Large Objects are the only objects in
 * the system that begin on a 16K byte boundary since the blocks used for smaller
 * objects have the Block structure at each 16K boundary.
 *
 */

static unsigned int isLargeObject(void *object); /* Forward Ref */

struct LargeObjectHeader {
    void        *unalignedResult;   /* The base of the memory returned from getMemory, this is what is used to return this to the OS */
    size_t       unalignedSize;     /* The size that was requested from getMemory */
    size_t       objectSize;        /* The size originally requested by a client */
};

static inline void *mallocLargeObject (size_t size)
{
    void *result;
    void *alignedArea;
    void *unalignedArea;
    LargeObjectHeader *header;

    // TODO: can the requestedSize be optimized somehow?
    size_t requestedSize = size + sizeof(LargeObjectHeader) + blockSize;
#if USE_MALLOC_FOR_LARGE_OBJECT
    unalignedArea = malloc(requestedSize);
#else
    unalignedArea = getMemory(requestedSize);
#endif /* USE_MALLOC_FOR_LARGE_OBJECT */
    if (!unalignedArea) {
        /* We can't get any more memory from the OS or executive so return 0 */
        return 0;
    }
    alignedArea = (void *)( ((uintptr_t)unalignedArea + blockSize - 1) & ~(blockSize - 1) );
    header = (LargeObjectHeader *)alignedArea;
    header->unalignedResult = unalignedArea;
    header->unalignedSize = requestedSize;
    header->objectSize = size;
    result = (void *)((uintptr_t)alignedArea + sizeof(LargeObjectHeader));
    MALLOC_ASSERT(isLargeObject(result), ASSERT_TEXT);
    STAT_increment(getThreadId(), ThreadCommonCounters, allocLargeSize);
    return result;
}

static inline void freeLargeObject(void *object)
{
    LargeObjectHeader *header;
    STAT_increment(getThreadId(), ThreadCommonCounters, freeLargeSize);
    header = (LargeObjectHeader *)((uintptr_t)object - sizeof(LargeObjectHeader));
#if USE_MALLOC_FOR_LARGE_OBJECT
    free(header->unalignedResult);
#else
    returnMemory(header->unalignedResult, header->unalignedSize);
#endif /* USE_MALLOC_FOR_LARGE_OBJECT */
}

/* Does this object start on a 16K boundary + the size of a large object header? */
static inline unsigned int isLargeObject(void *object)
{
    return ((uintptr_t)object & (blockSize - 1)) == sizeof(LargeObjectHeader);
}

static inline unsigned int getLargeObjectSize(void *object)
{
    LargeObjectHeader *header;
    header = (LargeObjectHeader *)((uintptr_t)object & ~(blockSize - 1));
    return header->objectSize;
}

static FreeObject *allocateFromFreeList(Block *mallocBlock)
{
    FreeObject *result;

    if (!mallocBlock->freeList) {
        return NULL;
    }

    result = mallocBlock->freeList;
    MALLOC_ASSERT (result, ASSERT_TEXT);

    mallocBlock->freeList = result->next;
    MALLOC_ASSERT(mallocBlock->allocatedCount < (blockSize-sizeof(Block))/mallocBlock->objectSize, ASSERT_TEXT);
    mallocBlock->allocatedCount++;
    STAT_increment(mallocBlock->owner, getIndex(mallocBlock->objectSize), allocFreeListUsed);

    return result;
}

static FreeObject *allocateFromBumpPtr(Block *mallocBlock)
{
    FreeObject *result = mallocBlock->bumpPtr;
    if (result) {
        mallocBlock->bumpPtr =
            (FreeObject *) ((uintptr_t) mallocBlock->bumpPtr - mallocBlock->objectSize);
        if ( (uintptr_t)mallocBlock->bumpPtr < (uintptr_t)mallocBlock+sizeof(Block) ) {
            mallocBlock->bumpPtr = NULL;
        }
        MALLOC_ASSERT(mallocBlock->allocatedCount < (blockSize-sizeof(Block))/mallocBlock->objectSize, ASSERT_TEXT);
        mallocBlock->allocatedCount++;
        STAT_increment(mallocBlock->owner, getIndex(mallocBlock->objectSize), allocBumpPtrUsed);
    }
    return result;
}

inline static FreeObject* allocateFromBlock( Block *mallocBlock )
{
    FreeObject *result;

    MALLOC_ASSERT( mallocBlock->owner == getThreadId(), ASSERT_TEXT );
    /*
     * for better cache locality, first looking in the free list
     */
    if ( (result = allocateFromFreeList(mallocBlock)) ) {
        return result;
    }
    MALLOC_ASSERT( !mallocBlock->freeList, ASSERT_TEXT );
    /*
     * if free list is empty, try thread local bump pointer allocation.
     */
    if ( (result = allocateFromBumpPtr(mallocBlock)) ) {
        return result;
    }
    MALLOC_ASSERT( !mallocBlock->bumpPtr, ASSERT_TEXT );
    /*
     * the block is considered full
     */
    mallocBlock->isFull = 1;
    return NULL;
}

inline void* set_errno_if_NULL(void* arg)
{
   if ( arg==NULL )
       errno = ENOMEM;
   return arg;
}

} // namespace Internal
} // namespace ThreadingSubstrate

using namespace ThreadingSubstrate;
using namespace ThreadingSubstrate::Internal;

//! Value indicating state of package initialization.
/* 0 = initialization not started.
   1 = initialization started but not finished.
   2 = initialization finished.
   In theory, we only need values 0 and 2.  But value 1 is nonetheless useful for
   detecting errors in the double-check pattern. */
static int mallocInitialized;   // implicitly initialized to 0
static MallocMutex initAndShutMutex;

extern "C" void mallocThreadShutdownNotification(void*);

/*
 * Obviously this needs to be called before malloc is available.
 */
/*  THIS IS DONE ON-DEMAND ON FIRST MALLOC, SO ASSUME MANUAL CALL TO IT IS NOT REQUIRED  */
static void initMemoryManager()
{
    int result;
    TRACEF("sizeof(Block) is %d (expected 128); sizeof(uintptr_t) is %d\n", sizeof(Block), sizeof(uintptr_t));
    MALLOC_ASSERT( 2*ALIGNED_SIZE == sizeof(Block), ASSERT_TEXT );

// Create keys for thread-local storage and for thread id
// TODO: add error handling
#if USE_WINTHREAD
    TLS_pointer_key = TlsAlloc();
    Tid_key = TlsAlloc();
#else
    int status = pthread_key_create( &TLS_pointer_key, mallocThreadShutdownNotification );
    status = pthread_key_create( &Tid_key, NULL );
#endif /* USE_WINTHREAD */
    // no more necessary: lifoQueueInit(&freeBlockQueue);
    TRACEF("Asking for a mallocBigBlock\n");
    result = mallocBigBlock();
    if (!result) {
        printf ("The memory manager cannot access sufficient memory to initialize; aborting \n");
        exit(0);
    }
}

//! Ensures that initMemoryManager() is called once and only once.
/** Does not return until initMemoryManager() has been completed by a thread.
    There is no need to call this routine if mallocInitialized==2 . */
static void checkInitialization()
{
    MallocMutex::scoped_lock lock( initAndShutMutex );
    if(mallocInitialized!=2) {
        MALLOC_ASSERT(mallocInitialized==0, ASSERT_TEXT);
        mallocInitialized = 1;
        initMemoryManager();
#ifdef  MALLOC_EXTRA_INITIALIZATION
        MALLOC_EXTRA_INITIALIZATION;
#endif /* MALLOC_EXTRA_INITIALIZATION */
        MALLOC_ASSERT(mallocInitialized==1, ASSERT_TEXT);
        mallocInitialized = 2;
    }
    MALLOC_ASSERT(mallocInitialized==2, ASSERT_TEXT); /* It can't be 0 or I would have initialized it */
}

/*
 * When a thread is shuting down this routine should be called to remove all the thread ids
 * from the malloc blocks and replace them with a NULL thread id.
 *
 */
static unsigned int threadGoingDownCount = 0;
/*
 * for pthreads, the function is set as a callback in pthread_key_create for TLS bin.
 * it will be automatically called at thread exit with the key value as the argument.
 *
 * for Windows, it should be called directly e.g. from DllMain; the argument can be NULL
 * one should include "TypeDefinitions.h" for the declaration of this function.
*/
extern "C" void mallocThreadShutdownNotification(void* arg)
{
    Bin   *tls;
    Block *threadBlock;
    Block *threadlessBlock;
    unsigned int index;

    {
        MallocMutex::scoped_lock lock( initAndShutMutex );
        if ( mallocInitialized == 0 ) return;
    }

    TRACEF("Thread id %d blocks return start %d\n", getThreadId(),  threadGoingDownCount++);
#ifdef USE_WINTHREAD
    tls = (Bin*)getThreadMallocTLS();
#else
    tls = (Bin*)arg;
#endif
    if (tls) {
        for (index = 0; index < numBlockBins; index++) {
            if (tls[index].activeBlk==NULL)
                continue;
            threadlessBlock = tls[index].activeBlk->previous;
            while (threadlessBlock) {
                threadBlock = threadlessBlock->previous;
                if (threadlessBlock->allocatedCount==0 && threadlessBlock->publicFreeList==NULL) {
                    /* we destroy the thread, no need to keep its TLS bin -> the second param is false */
                    returnEmptyBlock(threadlessBlock, false);
                } else {
                    returnPartialBlock(tls+index, threadlessBlock);
                }
                threadlessBlock = threadBlock;
            }
            threadlessBlock = tls[index].activeBlk;
            while (threadlessBlock) {
                threadBlock = threadlessBlock->next;
                if (threadlessBlock->allocatedCount==0 && threadlessBlock->publicFreeList==NULL) {
                    /* we destroy the thread, no need to keep its TLS bin -> the second param is false */
                    returnEmptyBlock(threadlessBlock, false);
                } else {
                    returnPartialBlock(tls+index, threadlessBlock);
                }
                threadlessBlock = threadBlock;
            }
            tls[index].activeBlk = 0;
        }
        // TODO - Free up this tls stuff.....         somehow free the tls structure... then call setThreadMallocTLS(0);
    }

#ifndef USE_WINTHREAD
// on Windows,  all statistics will be flushed in mallocProcessShutdownNotification
#if COLLECT_STATISTICS
    STAT_print(getThreadId());
#endif
#endif

    TRACEF("Thread id %d blocks return end\n", getThreadId());
}

extern "C" void mallocProcessShutdownNotification(void)
{
    // for now, this function is only necessary for dumping statistics
    // and it should only be called on Windows via DLL_PROCESS_DETACH
#if COLLECT_STATISTICS
    ThreadId nThreads = ThreadIdCount;
    for( int i=1; i<=nThreads && i<MAX_THREADS; ++i )
        STAT_print(i);
#endif
}

/********* The malloc code          *************/

extern "C" void * scalable_malloc(size_t size)
{
    Bin* bin;
    Block * mallocBlock;
    FreeObject *result;

    if( !size ) size = sizeof(size_t);

    if (mallocInitialized!=2) {
        /* This returns only after malloc is initialized */
        checkInitialization();
    }
    /*
     * Use Large Object Allocation
     */
    if (size >= minLargeObjectSize) {
        return set_errno_if_NULL( mallocLargeObject(size) );
    }

    /*
     * Get an element in thread-local array corresponding to the given size;
     * It keeps ptr to the active block for allocations of this size
     */
    bin = getAllocationBin(size);

    /* Get the block of you want to try to allocate in. */
    mallocBlock = getActiveBlock(bin);

    if (mallocBlock) {
        do {
            if( (result = allocateFromBlock(mallocBlock)) ) {
                return result;
            }
            // the previous block, if any, should be empty enough
        } while( (mallocBlock = setPreviousBlockActive(bin)) );
    }
    MALLOC_ASSERT( !(bin->activeBlk) || bin->activeBlk->isFull==1, ASSERT_TEXT );

    /*
     * else privatize publicly freed objects in some block and allocate from it
     */
    mallocBlock = getPublicFreeListBlock( bin );
    if( mallocBlock ) {
        if( emptyEnoughToUse(mallocBlock) ) {
            /* move the block to the front of the bin */
            outofTLSBin(bin, mallocBlock);
            pushTLSBin(bin, mallocBlock);
        }
        MALLOC_ASSERT( mallocBlock->freeList, ASSERT_TEXT );
        if ( (result = allocateFromFreeList(mallocBlock)) ) {
            return result;
        }
        /* Else something strange happened, need to retry from the beginning; */
        TRACEF("This isn't correct reasonable local block disappears --- ScalableMalloc\n");
        return scalable_malloc(size);
    }

    /*
     * no suitable own blocks, try to get a partial block that some other thread has discarded.
     */
    mallocBlock = getPartialBlock(bin, size);
    while (mallocBlock) {
        pushTLSBin(bin, mallocBlock);
// guaranteed by pushTLSBin: MALLOC_ASSERT( *bin==mallocBlock || (*bin)->previous==mallocBlock, ASSERT_TEXT );
        setActiveBlock(bin, mallocBlock);
        if( (result = allocateFromBlock(mallocBlock)) ) {
            return result;
        }
        mallocBlock = getPartialBlock(bin, size);
    }

    /*
     * else try to get a new empty block
     */
    mallocBlock = getEmptyBlock(size);
    if (mallocBlock) {
        pushTLSBin(bin, mallocBlock);
// guaranteed by pushTLSBin: MALLOC_ASSERT( *bin==mallocBlock || (*bin)->previous==mallocBlock, ASSERT_TEXT );
        setActiveBlock(bin, mallocBlock);
        if( (result = allocateFromBlock(mallocBlock)) ) {
            return result;
        }
        /* Else something strange happened, need to retry from the beginning; */
        TRACEF("This isn't correct reasonable local block disappears --- ScalableMalloc\n");
        return scalable_malloc(size);
    }
    /*
     * else nothing works so return NULL
     */
    TRACEF("NULL Back; \n");
    errno = ENOMEM;
    return NULL;
}

/********* End the malloc code      *************/

/********* The free code            *************/

extern "C" void scalable_free (void *object) {
    Block *block;
    ThreadId myTid;
    FreeObject *objectToFree;

    if (!object) {
        return;
    }

    if (isLargeObject(object)) {
        freeLargeObject(object);
        return;
    }

    objectToFree = (FreeObject *)object;

    myTid = getThreadId();

    block = (Block *) ((uintptr_t)object & ~(blockSize - 1));/* mask low bits to get the block */
    MALLOC_ASSERT (block->allocatedCount, ASSERT_TEXT);

    if (myTid == block->owner) {
        ((FreeObject *)object)->next = block->freeList;
        block->freeList = (FreeObject *) object;
        block->allocatedCount--;
        MALLOC_ASSERT(block->allocatedCount < (blockSize-sizeof(Block))/block->objectSize, ASSERT_TEXT);
        Bin* bin = getAllocationBin(block->objectSize);
#if COLLECT_STATISTICS
        if (getActiveBlock(bin) != block)
            STAT_increment(myTid, getIndex(block->objectSize), freeToInactiveBlock);
        else
            STAT_increment(myTid, getIndex(block->objectSize), freeToActiveBlock);
#endif
        if (block->isFull && emptyEnoughToUse(block)) {
            /* move the block to the front of the bin */
            outofTLSBin(bin, block);
            pushTLSBin(bin, block);
        } else {
            if (block->allocatedCount==0 && block->publicFreeList==NULL) {
                if (block != getActiveBlock(bin) && block != getActiveBlock(bin)->previous ) {
                    /* We are not actively using this block; return it to the general block pool */
                    outofTLSBin(bin, block);
                    returnEmptyBlock(block);
                } else {
                    /* all objects are free - let's restore the bump pointer */
                    restoreBumpPtr(block);
                }
            }
        }
    } else { /* Slower path to add to multi writer queue, the allocatedCount is updated by the owner thread in malloc. */
        freePublicObject (block, objectToFree);
    }
}

/********* End the free code        *************/

/********* Code for scalable_realloc       ***********/

/*
 * From K&R
 * "realloc changes the size of the object pointer to by p to size. The contents will
 * be unchanged up to the minimum of the old and the new sizes. If the new size is larger,
 * the new space is uninitialized. realloc returns a pointer to the new space, or
 * NULL if the request cannot be satisfied, in which case *p is unchanged."
 *
 */
extern "C" void* scalable_realloc(void* ptr, size_t sz)
{
    void *result;
    Block* block;
    size_t copySize;

    if (ptr==NULL) {
        return scalable_malloc(sz);
    }

    if (sz == 0) {
        scalable_free(ptr);
        return NULL;
    }
    block = (Block *) ((uintptr_t)ptr & ~(blockSize - 1)); /* mask low bits to get the block */
    if (isLargeObject(ptr)) {
        LargeObjectHeader* loh = (LargeObjectHeader*)block;
        copySize = loh->unalignedSize-((uintptr_t)ptr-(uintptr_t)loh->unalignedResult);
        if (sz < copySize) {
            loh->objectSize = sz;
            return ptr;
        }
        else {
           copySize = loh->objectSize;
           result = scalable_malloc(sz);
        }
    } else {
        copySize = block->objectSize;
        if (sz < copySize) {
            return ptr;
        } else {
            result = scalable_malloc(sz);
        }
    }
    if (copySize > sz) {
        copySize = sz;
    }

    if (result) {
        memcpy(result, ptr, copySize);
        scalable_free(ptr);
    } else {
        errno = ENOMEM;
    }
    return result;
}


/********* End code for scalable_realloc   ***********/

/********* Code for scalable_calloc   ***********/

/*
 * From K&R
 * calloc returns a pointer to space for an array of nobj objects, each of size size,
 * or NULL if the request cannot be satisfied. The sapce is initialized to zero bytes.
 *
 */

extern "C" void * scalable_calloc(size_t nobj, size_t size)
{
    void *result;
    size_t arraySize;
    arraySize = nobj * size;
    result = scalable_malloc(arraySize);
    if (result) {
        memset(result, 0, arraySize);
    }
    return result;
}

/********* End code for scalable_calloc   ***********/

