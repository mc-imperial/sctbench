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

/* This file contains the TBB task scheduler. There are many classes
   lumped together here because very few are exposed to the outside
   world, and by putting them in a single translation unit, the
   compiler's optimizer might be able to do a better job. */

#if USE_PTHREAD

    // Some pthreads documentation says that <pthreads.h> must be first header.
    #include <pthread.h>

#elif USE_WINTHREAD

    #include <windows.h>
    #include <process.h>        /* Need _beginthreadex from there */
    #include <malloc.h>         /* Need _alloca from there */

#else

    #error Must define USE_PTHREAD or USE_WINTHREAD

#endif

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <new>
#include "tbb/tbb_stddef.h"

/* Temporarily change "private" to "public" while including "tbb/task.h"
   This hack allows us to avoid publishing types Arena and CustomScheduler
   in the public header files. */
#define private public
#include "tbb/task.h"
#if __TBB_EXCEPTIONS
#include "tbb/tbb_exception.h"
#endif /* __TBB_EXCEPTIONS */
#undef private

#include "tbb/task_scheduler_init.h"
#include "tbb/cache_aligned_allocator.h"
#include "tbb/tbb_stddef.h"
#include "tbb/tbb_machine.h"
#include "tbb_misc.h"
#include "tbb/mutex.h"
#include "tbb/atomic.h"
#include "tbb/gate.h"
#if __TBB_SCHEDULER_OBSERVER
#include "tbb/task_scheduler_observer.h"
#include "tbb/spin_rw_mutex.h"
#include "tbb/aligned_space.h"
#endif /* __TBB_SCHEDULER_OBSERVER */
#if __TBB_EXCEPTIONS
#include "tbb/spin_mutex.h"
#endif /* __TBB_EXCEPTIONS */

#if defined(_MSC_VER) && defined(_Wp64)
    // Workaround for overzealous compiler warnings in /Wp64 mode
    #pragma warning (disable: 4312 4244 4267)
#endif /* _MSC_VER && _Wp64 */

#include "tbb/partitioner.h"

#if DO_TBB_TRACE
#include <cstdio>
#define TBB_TRACE(x) ((void)std::printf x)
#else
#define TBB_TRACE(x) ((void)(0))
#endif /* DO_TBB_TRACE */

#if TBB_DO_ASSERT
#define COUNT_TASK_NODES 1
#endif /* TBB_DO_ASSERT */

/* If nonzero, then gather statistics */
#ifndef STATISTICS
#define STATISTICS 0
#endif /* STATISTICS */

#if STATISTICS
#define GATHER_STATISTIC(x) (x)
#else
#define GATHER_STATISTIC(x) ((void)0)
#endif /* STATISTICS */

#if __TBB_EXCEPTIONS
// The standard offsetof macro does not work for us since its usage is restricted 
// by POD-types only. Using 0x1000 (not NULL) is necessary to appease GCC.
#define __TBB_offsetof(class_name, member_name) ((ptrdiff_t)&(reinterpret_cast<class_name*>(0x1000)->member_name) - 0x1000)
// Returns address of the oject containing member with the given name and address
#define __TBB_get_object_addr(class_name, member_name, member_addr) \
    reinterpret_cast<class_name*>((char*)member_addr - __TBB_offsetof(class_name, member_name))
#endif /* __TBB_EXCEPTIONS */

namespace tbb {

using namespace std;

namespace internal {

//! Alignment for a task object
const size_t task_alignment = 16;

//! Number of bytes reserved for a task prefix
/** If not exactly sizeof(task_prefix), the extra bytes *precede* the task_prefix. */
static const size_t task_prefix_reservation_size = ((sizeof(internal::task_prefix)-1)/task_alignment+1)*task_alignment;

template<typename SchedulerTraits> class CustomScheduler;

typedef task::depth_type depth_type;

class mail_outbox;

struct task_proxy: public task {
    static const intptr pool_bit = 1;
    static const intptr mailbox_bit = 2;
    /* All but two low-order bits represent a (task*).
       Two low-order bits mean:
       1 = proxy is/was/will be in task pool
       2 = proxy is/was/will be in mailbox */
    intptr task_and_tag;

    //! Pointer to next task_proxy in a mailbox
    task_proxy* next_in_mailbox;

    //! Mailbox to which this was mailed.
    mail_outbox* outbox;
};

//! Internal representation of mail_outbox, without padding.
class unpadded_mail_outbox {
protected:
    //! Pointer to first task_proxy in mailbox, or NULL if box is empty. 
    task_proxy* my_first;

    //! Pointer to last task_proxy in mailbox, or NULL if box is empty. 
    /** Low-order bit set to 1 to represent lock on the box. */
    task_proxy* my_last;

    //! Owner of mailbox is not executing a task, and has drained its own task pool.
    bool my_is_idle;
};

//! Class representing where mail is put.
/** Padded to occupy a cache line. */
class mail_outbox: unpadded_mail_outbox {
    char pad[NFS_MaxLineSize-sizeof(unpadded_mail_outbox)];

    //! Acquire lock on the box.
    task_proxy* acquire() {
        ExponentialBackoff backoff;
        for(;;) {
            // No fence on load, because subsequent compare-and-swap has the necessary fence.
            intptr last = (intptr)my_last;
            if( (last&1)==0 && __TBB_CompareAndSwapW(&my_last,last|1,last)==last) {
                __TBB_ASSERT( (my_first==NULL)==((intptr(my_last)&~1)==0), NULL );
                return (task_proxy*)last;
            }
            backoff.pause();
        }
    }
    task_proxy* internal_pop() {
        //! No fence on load of my_first, because if it is NULL, there's nothing further to read from another thread.
        task_proxy* result = my_first;
        if( result ) {
            if( task_proxy* f = __TBB_load_with_acquire(result->next_in_mailbox) ) {
                // No lock required
                __TBB_store_with_release( my_first, f );
            } else {
                // acquire() has the necessary fence.
                task_proxy* l = acquire();
                __TBB_ASSERT(result==my_first,NULL); 
                if( !(my_first = result->next_in_mailbox) ) 
                    l=0;
                __TBB_store_with_release( my_last, l );
            }
        }
        return result;
    }
public:
    friend class mail_inbox;

    //! Push task_proxy onto the mailbox queue of another thread.
    void push( task_proxy& t ) {
        __TBB_ASSERT(&t!=NULL, NULL);
        t.next_in_mailbox = NULL; 
        if( task_proxy* l = acquire() ) {
            l->next_in_mailbox = &t;
        } else {
            my_first=&t;
        }
        // Fence required because caller is sending the task_proxy to another thread.
        __TBB_store_with_release( my_last, &t );
    }
#if TBB_DO_ASSERT
    //! Verify that *this is initialized empty mailbox.
    /** Raise assertion if *this is not in initialized state, or sizeof(this) is wrong.
        Instead of providing a constructor, we provide this assertion, because for
        brevity and speed, we depend upon a memset to initialize instances of this class */
    void assert_is_initialized() const {
        __TBB_ASSERT( sizeof(*this)==NFS_MaxLineSize, NULL );
        __TBB_ASSERT( !my_first, NULL );
        __TBB_ASSERT( !my_last, NULL );
        __TBB_ASSERT( !my_is_idle, NULL );
    }
#endif /* TBB_DO_ASSERT */

    //! Drain the mailbox 
    intptr drain() {
        intptr k = 0;
        // No fences here because other threads have already quit.
        for( ; task_proxy* t = my_first; ++k ) {
            my_first = t->next_in_mailbox;
            NFS_Free((char*)t-task_prefix_reservation_size);
        }
        return k;  
    }

    //! True if thread that owns this mailbox is looking for work.
    bool recipient_is_idle() {
        return my_is_idle;
    }
};

//! Class representing source of mail.
class mail_inbox {
    //! Corresponding sink where mail that we receive will be put.
    mail_outbox* my_putter;
public:
    //! Construct unattached inbox
    mail_inbox() : my_putter(NULL) {}

    //! Attach inbox to a corresponding outbox. 
    void attach( mail_outbox& putter ) {
        __TBB_ASSERT(!my_putter,"already attached");
        my_putter = &putter;
    }
    //! Detach inbox from its outbox
    void detach() {
        __TBB_ASSERT(my_putter,"not attached");
        my_putter = NULL;
    }
    //! Get next piece of mail, or NULL if mailbox is empty.
    task_proxy* pop() {
        return my_putter->internal_pop();
    }
    void set_is_idle( bool value ) {
        if( my_putter ) {
            __TBB_ASSERT( my_putter->my_is_idle==!value, NULL );
            my_putter->my_is_idle = value;
        }
    }
#if DO_ITT_NOTIFY
    //! Get pointer to corresponding outbox used for ITT_NOTIFY calls.
    void* outbox() const {return my_putter;}
#endif /* DO_ITT_NOTIFY */ 
};

#if __TBB_SCHEDULER_OBSERVER
//------------------------------------------------------------------------
// observer_proxy
//------------------------------------------------------------------------
class observer_proxy {
    friend class task_scheduler_observer_v3;
    //! Reference count used for garbage collection.
    /** 1 for reference from my task_scheduler_observer.
        1 for each local_last_observer_proxy that points to me. 
        No accounting for predecessor in the global list. 
        No accounting for global_last_observer_proxy that points to me. */
    atomic<int> gc_ref_count;
    //! Pointer to next task_scheduler_observer 
    /** Valid even when *this has been removed from the global list. */
    observer_proxy* next; 
    //! Pointer to previous task_scheduler_observer in global list.
    observer_proxy* prev; 
    //! Associated observer
    task_scheduler_observer* observer;
    //! Account for removing reference from p.  No effect if p is NULL.
    void remove_ref_slow();
    void remove_from_list(); 
    observer_proxy( task_scheduler_observer_v3& wo ); 
public:
    static observer_proxy* process_list( observer_proxy* local_last, bool is_worker, bool is_entry );
};
#endif /* __TBB_SCHEDULER_OBSERVER */

//------------------------------------------------------------------------
// TaskPool
//------------------------------------------------------------------------

//! Prefix to a TaskPool
class TaskPoolPrefix {
    static const unsigned null_arena_index = ~0u;

    unsigned arena_index;

    //! Index of first non-empty element of TaskPool::array
    depth_type steal_begin;

    friend class GenericScheduler;
    friend class TaskPool;
}; // class TaskPoolPrefix

//! Pool of tasks, organized as a deque.
class TaskPool {
    typedef size_t size_type;

    static const size_type min_array_size = (NFS_MaxLineSize-sizeof(TaskPoolPrefix))/sizeof(task*);

    /** Must be last field, because it is really array of indeterminate length. */
    task* array[min_array_size];

    //! Get reference to prefix portion
    TaskPoolPrefix& prefix() const {return ((TaskPoolPrefix*)(void*)this)[-1];}

    //! Return number of bytes required to allocate a pool with given number of array elements.
    static size_t byte_size( size_type array_size ) {
        return sizeof(TaskPoolPrefix)+array_size*sizeof(task*);
    }

    //! Allocate TaskPool object with given number of array elements.
    static TaskPool* allocate_task_pool( size_type array_size ) {
        __TBB_ASSERT( array_size>0, NULL );
        size_t n = byte_size(array_size);
        unsigned char* storage = (unsigned char*)NFS_Allocate( n, 1, NULL );
        memset( storage, 0, n );
        return (TaskPool*)(storage+sizeof(TaskPoolPrefix));
    }

    //! Deallocate a TaskPool that was allocated by method allocate.
    void free_task_pool() {
        __TBB_ASSERT( this, "attempt to free NULL TaskPool" );
        NFS_Free( &prefix() );
    }

    friend class GenericScheduler;
    template<typename SchedulerTraits> friend class internal::CustomScheduler;

#if TBB_DO_ASSERT
    bool assert_okay() const {
        __TBB_ASSERT( this!=NULL, NULL );
        __TBB_ASSERT( prefix().steal_begin>=-4, NULL );
        return true;
    }
#endif /* TBB_DO_ASSERT */
};

//------------------------------------------------------------------------
// Arena
//------------------------------------------------------------------------

class Arena;
class GenericScheduler;

struct WorkerDescriptor {
    Arena* arena;
    //! NULL until worker is published.  -1 if worker should not be published.
    GenericScheduler* scheduler;
#if USE_WINTHREAD
    //! Handle of the owning thread.
    HANDLE thread_handle;
#elif USE_PTHREAD
    //! Handle of the owning thread.
    pthread_t thread_handle;
#else
    unsigned long dummy_handle;
#endif /* USE_WINTHREAD */

    //! Stack size for working threads.
    stack_size_type thread_stack_size;
    
    //! Start worker thread for this descriptor.
    void start_one_worker_thread();
};

//! The useful contents of an ArenaPrefix
class UnpaddedArenaPrefix {
    friend class GenericScheduler;
    template<typename SchedulerTraits> friend class internal::CustomScheduler;
    friend class Arena;
    friend size_t get_initial_auto_partitioner_divisor();
    friend class tbb::task_scheduler_init;
    friend class affinity_partitioner_base_v3;

    //! One more than index of highest arena slot currently in use.
    atomic<size_t> limit;

    //! Number of masters that own this arena.
    /** This may be smaller than the number of masters who have entered the arena. */
    unsigned number_of_masters;

    //! Total number of slots in the arena
    const unsigned number_of_slots;

    //! Number of workers that belong to this arena
    const unsigned number_of_workers;

    //! Number of workers still using this arena (plus one if a master is still using the arena)
    atomic<int> gc_ref_count;

    //! Array of workers.
    WorkerDescriptor* worker_list;

#if COUNT_TASK_NODES
    //! Net number of nodes that have been allocated from heap.
    /** Updated each time a scheduler is destroyed. */
    atomic<intptr> task_node_count;
#endif /* COUNT_TASK_NODES */

    //! Gate at which worker threads wait until a master spawns a task.
    Gate gate;
 
protected:
    UnpaddedArenaPrefix( unsigned number_of_slots_, unsigned number_of_workers_ ) :
        number_of_masters(1),
        number_of_slots(number_of_slots_),
        number_of_workers(number_of_workers_)
    {
#if COUNT_TASK_NODES
        task_node_count = 0;
#endif /* COUNT_TASK_NODES */
        limit = number_of_workers_;
        gc_ref_count = number_of_workers_+1; 
    }
};

//! The prefix to Arena with padding.
class ArenaPrefix: public UnpaddedArenaPrefix {
    //! Padding to fill out to multiple of cache line size.
    char pad[(sizeof(UnpaddedArenaPrefix)/NFS_MaxLineSize+1)*NFS_MaxLineSize-sizeof(UnpaddedArenaPrefix)];

public:
    ArenaPrefix( unsigned number_of_slots_, unsigned number_of_workers_ ) :
        UnpaddedArenaPrefix(number_of_slots_,number_of_workers_)
    {
    }
};

struct UnpaddedArenaSlot {
    //! Holds copy of task_pool->deepest and a lock bit
    /** Computed as 2*task_pool->deepest+(is_locked).
        I.e., the low order bit indicates whether the slot is locked. 
        -2 denotes an empty task pool
        -3 denotes an unused arena slot that is locked
        -4 denotes an unused arena slot that is unlocked */
    depth_type steal_end;
    TaskPool* task_pool;
    bool owner_waits;
};

struct ArenaSlot: UnpaddedArenaSlot {
    char pad[NFS_MaxLineSize-sizeof(UnpaddedArenaSlot)];
};

class Arena {
    friend class GenericScheduler;
    template<typename SchedulerTraits> friend class internal::CustomScheduler;
    friend size_t get_initial_auto_partitioner_divisor();
    friend class tbb::task_scheduler_init;

    //! Get reference to prefix portion
    ArenaPrefix& prefix() const {return ((ArenaPrefix*)(void*)this)[-1];}

    friend class tbb::internal::affinity_partitioner_base_v3;

    //! Get reference to mailbox corresponding to given affinity_id.
    mail_outbox& mailbox( affinity_id id ) {
        __TBB_ASSERT( 0<id, "id must be positive integer" );
        __TBB_ASSERT( id <= prefix().number_of_slots, "id out of bounds" );
        return ((mail_outbox*)&prefix())[-(int)id];
    }

    //! Allocate the arena
    /** Allocates an instance of Arena and sets TheArena to point to it.
        Creates the worker threads, but does not wait for them to start. */
    static Arena* allocate_arena( unsigned number_of_slots, unsigned number_of_workers, stack_size_type stack_size );

    void free_arena() {
        NFS_Free( &mailbox(prefix().number_of_slots) );
    }

    //! Terminate worker threads
    /** Wait for worker threads to complete. */
    void terminate_workers();

    //! Remove a reference to the arena, and free the arena if no references remain.
    void remove_gc_reference();

#if COUNT_TASK_NODES
    //! Returns the number of task objects "living" in worker threads
    inline intptr workers_task_node_count();
#endif

    /** Must be last field */
    ArenaSlot slot[1];
};

//------------------------------------------------------------------------
//! Traits classes for scheduler
//------------------------------------------------------------------------

struct DefaultSchedulerTraits {
    static const int id = 0;
    static const bool itt_possible = true;
    static const bool has_slow_atomic = false;
};

struct IntelSchedulerTraits {
    static const int id = 1;
    static const bool itt_possible = false;
#if __TBB_x86_32||__TBB_x86_64
    static const bool has_slow_atomic = true;
#else
    static const bool has_slow_atomic = false;
#endif /* __TBB_x86_32||__TBB_x86_64 */
};

//------------------------------------------------------------------------
// Class __TBB_InitOnce
//------------------------------------------------------------------------

//! Class handles acquisition and release of global resources during startup and shutdown. 
/** Currently, its job is to deal with initializing/deleting
    OneTimeInitializationCriticalSection (on Windows only)
    and allocating/freeing thread-local storage. */
class __TBB_InitOnce {
    static atomic<int> count;

    //! Platform specific code to acquire resources.
    static void acquire_resources();

    //! Platform specific code to release resources.
    static void release_resources();
public:
    //! Add initial reference to resources. 
    /** We assume that dynamic loading of the library prevents any other threads from entering the library
        until this constructor has finished running. */
    __TBB_InitOnce() { add_ref(); }

    //! Remove the initial reference to resources, and remove the extra reference from DoOneTimeInitializations if present.
    /** This is not necessarily the last reference if other threads are still running . */
    ~__TBB_InitOnce();

    //! Add reference to resources.  If first reference added, acquire the resources.
    static void add_ref() {
        if( ++count==1 ) 
            acquire_resources();
    }
    //! Remove reference to resources.  If last reference added, release the resources.
    static void remove_ref() {
        int k = --count;
        __TBB_ASSERT(k>=0,"removed __TBB_InitOnce ref that was not added?"); 
        if( k==0 ) 
            release_resources();
    }
};

//------------------------------------------------------------------------
// Begin shared data layout.
//
// The follow global data items are read-only after initialization.
// The first item is aligned on a 128 byte boundary so that it starts a new cache line.
//------------------------------------------------------------------------

static internal::Arena * TheArena;
static mutex TheArenaMutex;

#if __TBB_SCHEDULER_OBSERVER
typedef spin_rw_mutex::scoped_lock task_scheduler_observer_mutex_scoped_lock;
/** aligned_space used here to shut up warnings when mutex destructor is called while threads are still using it. */
static aligned_space<spin_rw_mutex,1> the_task_scheduler_observer_mutex;
static observer_proxy* global_first_observer_proxy;
static observer_proxy* global_last_observer_proxy;
#endif /* __TBB_SCHEDULER_OBSERVER */

//! Number of hardware threads
/** One more than the default number of workers. */
static int DefaultNumberOfThreads;

//! T::id for the scheduler traits type T to use for the scheduler
/** For example, the default value is DefaultSchedulerTraits::id. */
static int SchedulerTraitsId;

} // namespace internal

} // namespace tbb

#include "itt_notify.h"

namespace tbb {
namespace internal {

//! Flag that is set to true after one-time initializations are done.
static bool OneTimeInitializationsDone;

//! Counter of references to TLS.
atomic<int> __TBB_InitOnce::count;

#if _WIN32||_WIN64
static CRITICAL_SECTION OneTimeInitializationCriticalSection;
//! Index for thread-local storage.
/** The original version of TBB used __declspec(thread) for thread-local storage.
    Unfortunately, __declspec(thread) does not work on pre-Vista OSes for DLLs
    called from plug-ins. */
static DWORD TLS_Index;
#if __TBB_TASK_CPP_DIRECTLY_INCLUDED
static __TBB_InitOnce __TBB_InitOnceHiddenInstance;
#endif

#else /* not Windows */
static pthread_mutex_t OneTimeInitializationMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t TLS_Key;
#if (__linux__ || __APPLE__) && __GNUC__ && !(__INTEL_COMPILER && __TBB_ipf)
// Use GCC-style attribute to set the highest initialization priority (the lowest possible number)
// ICC for IA-64 has a bug in handling init_priority so skipping in this case
static __TBB_InitOnce __TBB_InitOnceHiddenInstance __attribute__((init_priority (101)));
#else
static __TBB_InitOnce __TBB_InitOnceHiddenInstance;
#endif

#endif /* _WIN32||_WIN64 */

//! Table of primes used by fast random-number generator.
/** Also serves to keep anything else from being placed in the same
    cache line as the global data items preceding it. */
static const unsigned Primes[] = {
    0x9e3779b1, 0xffe6cc59, 0x2109f6dd, 0x43977ab5,
    0xba5703f5, 0xb495a877, 0xe1626741, 0x79695e6b,
    0xbc98c09f, 0xd5bee2b3, 0x287488f9, 0x3af18231,
    0x9677cd4d, 0xbe3a6929, 0xadc6a877, 0xdcf0674b,
    0xbe4d6fe9, 0x5f15e201, 0x99afc3fd, 0xf3f16801,
    0xe222cfff, 0x24ba5fdb, 0x0620452d, 0x79f149e3,
    0xc8b93f49, 0x972702cd, 0xb07dd827, 0x6c97d5ed,
    0x085a3d61, 0x46eb5ea7, 0x3d9910ed, 0x2e687b5b,
    0x29609227, 0x6eb081f1, 0x0954c4e1, 0x9d114db9,
    0x542acfa9, 0xb3e6bd7b, 0x0742d917, 0xe9f3ffa7,
    0x54581edb, 0xf2480f45, 0x0bb9288f, 0xef1affc7,
    0x85fa0ca7, 0x3ccc14db, 0xe6baf34b, 0x343377f7,
    0x5ca19031, 0xe6d9293b, 0xf0a9f391, 0x5d2e980b,
    0xfc411073, 0xc3749363, 0xb892d829, 0x3549366b,
    0x629750ad, 0xb98294e5, 0x892d9483, 0xc235baf3,
    0x3d2402a3, 0x6bdef3c9, 0xbec333cd, 0x40c9520f
};

#if STATISTICS
//! Class for collecting statistics
/** There should be only one instance of this class. 
    Results are written to a file "statistics.txt" in tab-separated format. */
static class statistics {
public:
    statistics() {
        my_file = fopen("statistics.txt","w");
        if( !my_file ) {
            perror("fopen(\"statistics.txt\"\")");
            exit(1);
        }
        fprintf(my_file,"%13s\t%13s\t%13s\t%13s\t%13s\t%13s\n", "execute", "steal", "mail", "proxy_execute", "proxy_steal", "proxy_bypass" );
    }
    ~statistics() {
        fclose(my_file);
    }
    void record( long execute_count, long steal_count, long mail_received_count, 
                 long proxy_execute_count, long proxy_steal_count, long proxy_bypass_count ) {
        mutex::scoped_lock lock(my_mutex);
        fprintf (my_file,"%13ld\t%13ld\t%13ld\t%13ld\t%13ld\t%13ld\n", execute_count, steal_count, mail_received_count, 
                                                           proxy_execute_count, proxy_steal_count, proxy_bypass_count );
    }
private:
    //! File into which statistics are written.
    FILE* my_file;
    //! Mutex that serializes accesses to my_file
    mutex my_mutex;
} the_statistics;
#endif /* STATISTICS */

#if __TBB_EXCEPTIONS
    struct scheduler_list_node_t {
        scheduler_list_node_t *my_prev,
                              *my_next;
    };

    //! Head of the list of master thread schedulers.
    static scheduler_list_node_t the_scheduler_list_head;

    //! Mutex protecting access to the list of schedulers.
    static mutex the_scheduler_list_mutex;

//! Counter that is incremented whenever new cancellation signal is sent to a task group.
/** Together with GenericScheduler::local_cancel_count forms cross-thread signaling
    mechanism that allows to avoid locking at the hot path of normal execution flow.

    When a descendant task group context is being registered or unregistered,
    the global and local counters are compared. If they differ, it means that 
    a cancellation signal is being propagated, and registration/deregistration
    routines take slower branch that may block (at most one thread of the pool
    can be blocked at any moment). Otherwise the control path is lock-free and fast. **/
    static uintptr_t global_cancel_count = 0;

    //! Context to be associated with dummy tasks of worker threads schedulers.
    /** It is never used for its direct purpose, and is introduced solely for the sake 
        of avoiding one extra conditional branch in the end of wait_for_all method. **/
    static task_group_context dummy_context(task_group_context::isolated);
#endif /* __TBB_EXCEPTIONS */

//------------------------------------------------------------------------
// End of shared data layout
//------------------------------------------------------------------------

//! Amount of time to pause between steals.
/** The default values below were found to be best empirically for K-Means
    on the 32-way Altix and 4-way (*2 for HT) fxqlin04. */
#if __TBB_ipf
static const long PauseTime = 1500;
#else 
static const long PauseTime = 80;
#endif

//------------------------------------------------------------------------
// One-time Initializations
//------------------------------------------------------------------------


//! Defined in cache_aligned_allocator.cpp
extern void initialize_cache_aligned_allocator();

//! Perform lazy one-time initializations. */
void DoOneTimeInitializations() {
#if USE_PTHREAD
    int status = 0;
    pthread_mutex_lock( &OneTimeInitializationMutex );
#else
    EnterCriticalSection( &OneTimeInitializationCriticalSection );
#endif /* USE_PTHREAD */
    // No fence required for load of OneTimeInitializationsDone, because we are inside a critical section.
    if( !OneTimeInitializationsDone ) {
        __TBB_InitOnce::add_ref();
        if( GetBoolEnvironmentVariable("TBB_VERSION") )
            PrintVersion();
        bool have_itt = false;
#if DO_ITT_NOTIFY
        have_itt = InitializeITT();
#endif /* DO_ITT_NOTIFY */
        initialize_cache_aligned_allocator();
        if( !have_itt )
            SchedulerTraitsId = IntelSchedulerTraits::id;
        PrintExtraVersionInfo( "SCHEDULER",
                               SchedulerTraitsId==IntelSchedulerTraits::id ? "Intel" : "default" );
#if __TBB_EXCEPTIONS
        the_scheduler_list_head.my_next = &the_scheduler_list_head;
        the_scheduler_list_head.my_prev = &the_scheduler_list_head;
#endif /* __TBB_EXCEPTIONS */
        OneTimeInitializationsDone = true;
    }
#if USE_PTHREAD
    pthread_mutex_unlock( &OneTimeInitializationMutex );
    if( status )
        handle_perror(status,"pthread_key_create");
#else
    LeaveCriticalSection( &OneTimeInitializationCriticalSection );
#endif /* USE_PTHREAD */
}

//------------------------------------------------------------------------
// Methods of class __TBB_InitOnce
//------------------------------------------------------------------------

__TBB_InitOnce::~__TBB_InitOnce() { 
    remove_ref();
    // It is assumed that OneTimeInitializationsDone is not set after file-scope destructors start running,
    // and thus no race on OneTimeInitializationsDone is possible.
    if( __TBB_load_with_acquire(OneTimeInitializationsDone) ) {
        // Remove reference that we added in DoOneTimeInitializations.
        remove_ref();  
    }
} 

#if _WIN32||_WIN64
void __TBB_InitOnce::acquire_resources() {
    TLS_Index = TlsAlloc();
    if( TLS_Index!=TLS_OUT_OF_INDEXES ) {
        InitializeCriticalSection(&OneTimeInitializationCriticalSection);
    } else {
#if TBB_DO_ASSERT
        // Issue diagnostic here, not failing assertion, because client
        // might want to test graceful recovery from this problem.
        fprintf( stderr, "TBB failed to initialize: TLS is out of indices\n" );
#endif /* TBB_DO_ASSERT */
    }
}

void __TBB_InitOnce::release_resources() {
    DeleteCriticalSection(&OneTimeInitializationCriticalSection);
#if TBB_DO_ASSERT
    if( TlsGetValue(TLS_Index) ) {
        fprintf( stderr, "TBB is unloaded while tbb::task_scheduler_init object is alive?" );
    }
#endif /* TBB_DO_ASSERT */
    TlsFree(TLS_Index);
    TLS_Index = 0;
}

#if !__TBB_TASK_CPP_DIRECTLY_INCLUDED
//! Windows "DllMain" that handles startup and shutdown of dynamic library.
extern "C" bool WINAPI DllMain( HANDLE hinstDLL, DWORD reason, LPVOID lpvReserved );
bool WINAPI DllMain( HANDLE hinstDLL, DWORD reason, LPVOID lpvReserved ) {
    switch( reason ) {
        case DLL_PROCESS_ATTACH:
            __TBB_InitOnce::add_ref();
            break;
        case DLL_PROCESS_DETACH:
            __TBB_InitOnce::remove_ref();
            // It is assumed that OneTimeInitializationsDone is not set after DLL_PROCESS_DETACH,
            // and thus no race on OneTimeInitializationsDone is possible.
            if( __TBB_load_with_acquire(OneTimeInitializationsDone) ) {
                // Remove reference that we added in DoOneTimeInitializations.
                __TBB_InitOnce::remove_ref();
            }
            break;
    }
    return true;
}
#endif /* !__TBB_TASK_CPP_DIRECTLY_INCLUDED */

#else /* not Windows */
void __TBB_InitOnce::acquire_resources() {
    // Create key for thread-local storage
    int status = pthread_key_create( &TLS_Key, NULL );
    if( status )
        handle_perror(status, "pthread_key_create");
}

void __TBB_InitOnce::release_resources() {
    // Delete thread-local storage key
    int status = pthread_key_delete( TLS_Key );
    if( status )
        handle_perror(status, "pthread_key_delete");
}

#endif /* _WIN32||_WIN64 */

//------------------------------------------------------------------------
// Routines for thread-specific global data
//------------------------------------------------------------------------

static inline void SetThreadSpecific( GenericScheduler* s ) {
#if USE_WINTHREAD
    TlsSetValue( TLS_Index, s );
#else
    pthread_setspecific( TLS_Key, s );
#endif /* USE_WINTHREAD */
}

//! Get scheduler belonging to the current thread.
/** Returns NULL if this is the first time the thread has requested a scheduler.
    It's the client's responsibility to check for the NULL, because in many
    contexts, we can prove that it cannot be NULL. */
static inline GenericScheduler* GetThreadSpecific() {
    GenericScheduler *result;
    // The assertion on OneTimeInitializationsDone checks that we can safely
    // use TLS_Key/TLS_Index; i.e., that TLS_Key/TLS_Index has been initialized.
    // The assertion message is intended to help end users.  Even though
    // OneTimeInitializationsDone might be set for other reasons, if it is
    // *not* set when a thread reaches here, the reason is almost
    // certainly that the thread failed to create a task_scheduler_init object.
    __TBB_ASSERT( OneTimeInitializationsDone, "thread did not activate a task_scheduler_init object?" );
#if USE_WINTHREAD
    result = (GenericScheduler*)TlsGetValue( TLS_Index );
#else
    result = (GenericScheduler*)pthread_getspecific( TLS_Key );
#endif /* USE_WINTHREAD */
    return result;
}

//------------------------------------------------------------------------
// FastRandom
//------------------------------------------------------------------------

//! A fast random number generator.
/** Uses linear congruential method. */
class FastRandom {
    unsigned x, a;
public:
    //! Get a random number.
    unsigned short get() {
        unsigned short r = x>>16;
        x = x*a+1;
        return r;
    }
    //! Construct a random number generator.
    FastRandom( unsigned seed ) {
        x = seed;
        a = Primes[seed%(sizeof(Primes)/sizeof(Primes[0]))];
    }
};

//------------------------------------------------------------------------
// GenericScheduler
//------------------------------------------------------------------------

//  A pure virtual destructor should still have a body
//  so the one for tbb::internal::scheduler::~scheduler() is provided here
scheduler::~scheduler( ) {}

//! Cilk-style task scheduler.
/** None of the fields here are every read or written by threads other than
    the thread that creates the instance.

    Class GenericScheduler is an abstract base class that contains most of the scheduler,
    except for tweaks specific to processors and tools (e.g. VTune).
    The derived template class CustomScheduler<SchedulerTraits> fills in the tweaks. */
class GenericScheduler: public scheduler {
    typedef task::depth_type depth_type;
    friend class tbb::task;
    friend class tbb::task_scheduler_init;
    friend struct WorkerDescriptor;
    friend class Arena;
    friend class allocate_root_proxy;
#if __TBB_EXCEPTIONS
    friend class allocate_root_with_context_proxy;
    friend class tbb::task_group_context;
#endif /* __TBB_EXCEPTIONS */
#if __TBB_SCHEDULER_OBSERVER
    friend class task_scheduler_observer_v3;
#endif /* __TBB_SCHEDULER_OBSERVER */
    friend class scheduler;
    template<typename SchedulerTraits> friend class internal::CustomScheduler;

    //! If sizeof(task) is <=quick_task_size, it is handled on a free list instead of malloc'd.
    static const size_t quick_task_size = 256-task_prefix_reservation_size;

    //! Definitions for bits in task_prefix::extra_state
    enum internal_state_t {
        //! Tag for TBB <3.0 tasks.
        es_version_2_task = 0,
        //! Tag for TBB 3.0 tasks.
        es_version_3_task = 1,
        //! Tag for TBB 3.0 task_proxy.
        es_task_proxy = 2,
        //! Set if ref_count might be changed by another thread.  Used for debugging.
        es_ref_count_active = 0x40,
        //! Set if ref_count might be changed by another thread.
        es_pending_exception = 0x80
    };
    
    static bool is_version_3_task( task& t ) {
        return (t.prefix().extra_state & 0x3F)==0x1;
    }

    //! Deepest non-empty level.
    /** Not read by thieves. -1 if array is empty. */
    depth_type deepest;

    //! The physical number of slots in "array".
    TaskPool::size_type array_size;

    //! Dummy slot used when scheduler is not in arena
    UnpaddedArenaSlot dummy_slot;

    //! Pointer to my slot in the arena
    mutable UnpaddedArenaSlot* arena_slot;

    //! The arena that I own (if master) or belong to (if worker)
    Arena* const arena;

    //! Random number generator used for picking a random victim from which to steal.
    FastRandom random;

    //! Free list of small tasks that can be reused.
    task* free_list;

    //! Innermost task whose task::execute() is running.
    task* innermost_running_task;

    //! Fake root task created by slave threads.
    /** The task is used as the "parent" argument to method wait_for_all. */
    task* dummy_task;

    //! Reference count for scheduler
    /** Number of task_scheduler_init objects that point to this scheduler */
    long ref_count;

    mail_inbox inbox;

    void attach_mailbox( affinity_id id ) {
        __TBB_ASSERT(id>0,NULL);
        inbox.attach( arena->mailbox(id) );
        my_affinity_id = id;
    }

    //! The mailbox id assigned to this scheduler.
    /** The id is assigned upon first entry into the arena. 
        FIXME - how are id's being garbage collected? 
        FIXME - master thread may enter arena and leave and then reenter.
                We want to give it the same affinity_id upon reentry, if practical.
      */
    affinity_id my_affinity_id;

    //! True if this is assigned to thread local storage.
    /** Located here because space is otherwise just padding after my_affinity_id. */
    bool occupy_tls;

    //! Register scheduler in thread local storage
    void register_in_thread_local_storage() {
        __TBB_ASSERT( !occupy_tls, NULL );  
        occupy_tls = true;
        __TBB_InitOnce::add_ref();
        SetThreadSpecific(this);
    }

    //! Unregister scheduler from thread local storage
    void unregister_from_thread_local_storage() {
        if( occupy_tls ) {
            SetThreadSpecific(NULL);
            occupy_tls = false;
            __TBB_InitOnce::remove_ref();
        }
    }

#if __TBB_SCHEDULER_OBSERVER
    //! Last observer_proxy processed by this scheduler
    observer_proxy* local_last_observer_proxy;

    //! Notify any entry observers that have been created since the last call by this thread.
    void notify_entry_observers() {
        local_last_observer_proxy = observer_proxy::process_list(local_last_observer_proxy,is_worker(),/*is_entry=*/true);
    }
 
    //! Notify all exit observers that this thread is no longer participating in task scheduling.
    void notify_exit_observers( bool is_worker ) {
        observer_proxy::process_list(local_last_observer_proxy,is_worker,/*is_entry=*/false);
    }
#endif /* __TBB_SCHEDULER_OBSERVER */

#if COUNT_TASK_NODES
    //! Net number of big task objects that have been allocated but not yet freed.
    intptr task_node_count;
#endif /* COUNT_TASK_NODES */

#if STATISTICS
    long current_active;
    long current_length;
    //! Number of big tasks that have been malloc'd.
    /** To find total number of tasks malloc'd, compute (current_big_malloc+small_task_count) */
    long current_big_malloc;
    long execute_count;
    //! Number of tasks stolen
    long steal_count;
    //! Number of tasks received from mailbox
    long mail_received_count;
    long proxy_execute_count;
    long proxy_steal_count;
    long proxy_bypass_count;
#endif /* STATISTICS */

    //! Try to enter the arena
    /** On return, guaranteess that task pool has been acquired. */
    void try_enter_arena();

    //! Leave the arena
    void leave_arena( bool compress );

    void acquire_task_pool() const;

    void release_task_pool() const;

    //! Get task from ready pool.
    /** Called only by the thread that owns *this.
        Gets task only if there is one at depth d or deeper in the pool.
        If successful, unlinks the task and returns a pointer to it.
        Otherwise returns NULL. */
    task* get_task( depth_type d );

    //! Attempt to get a task from the mailbox.
    /** Called only by the thread that owns *this.
        Gets a task only if there is one not yet executed by another thread.
        If successful, unlinks the task and returns a pointer to it.
        Otherwise returns NULL. */
    task* get_mailbox_task();

    //! True if t is a task_proxy
    static bool is_proxy( const task& t ) {
        return t.prefix().extra_state==es_task_proxy;
    }

    /** Return NULL if underlying task was claimed by mailbox. */
    task* strip_proxy( task_proxy* result );

    //! Steal task from another scheduler's ready pool.
    task* steal_task( UnpaddedArenaSlot& arena_slot, depth_type d );

    //! Grow "array" to at least "minimum_size" elements.
    /** Does nothing if array is already that big.
        Returns &array[minimum_size-1] */
    void grow( TaskPool::size_type minimum_size );

    //! Call destructor for a task and put it on the free list (or free it if it is big).
    void destroy_task( task& t ) {
        TBB_TRACE(("%p.destroy_task(%p)\n",this,&t));
        __TBB_ASSERT( t.is_owned_by_current_thread(), "task owned by different thread" );
        t.~task();
        free_task<no_hint>( t );
    }
    static GenericScheduler* create_master( Arena* a );

    /** The workers are started up as a binary tree, where each vertex in the tree
        starts any children it has.  The tree is implicitly arranged in TheWorkerList
        like a binary heap. */
    static GenericScheduler* create_worker( WorkerDescriptor& w );

    //! Top-level routine for worker threads
    /** Argument arg is a WorkerDescriptor*, cast to a (void*). */
#if USE_WINTHREAD
    static unsigned WINAPI worker_routine( void* arg );
#else
    static void* worker_routine( void* arg );
#endif /* USE_WINTHREAD */

    //! Called by slave threads to free memory and wait for other threads.
    static void cleanup_worker_thread( void* arg );

protected:
    GenericScheduler( Arena* arena );

#if TBB_DO_ASSERT || TEST_ASSEMBLY_ROUTINES
    //! Check that internal data structures are in consistent state.
    /** Raises __TBB_ASSERT failure if inconsistency is found. */
    bool assert_okay() const;
#endif /* TBB_DO_ASSERT || TEST_ASSEMBLY_ROUTINES */

public:
    /*override*/ void spawn( task& first, task*& next );

    /*override*/ void spawn_root_and_wait( task& first, task*& next );

    static GenericScheduler* allocate_scheduler( Arena* arena );

    //! Destroy and deallocate scheduler that was created with method allocate.
    void free_scheduler();

    //! Allocate task object, either from the heap or a free list.
    /** Returns uninitialized task object with initialized prefix. */
#if __TBB_EXCEPTIONS
    task& allocate_task( size_t number_of_bytes, depth_type depth, task* parent, task_group_context* context );
#else
    task& allocate_task( size_t number_of_bytes, depth_type depth, task* parent );
#endif /* __TBB_EXCEPTIONS */

    //! Optimization hint to free_task that enables it omit unnecessary tests and code.
    enum hint {
        //! No hint 
        no_hint=0,
        //! Task is known to have been allocated by this scheduler
        is_local=1,
        //! Task is known to be a small task.
        /** Task should be returned to the free list of *some* scheduler, possibly not this scheduler. */
        is_small=2,
        //! Bitwise-OR of is_local and is_small.  
        /** Task should be returned to free list of this scheduler. */
        is_small_local=3
    };

    //! Put task on free list.
    /** Does not call destructor. */
    template<hint h>
    void free_task( task& t );

    void free_task_proxy( task_proxy& tp ) {
#if TBB_DO_ASSERT
        poison_pointer( tp.outbox );
        poison_pointer( tp.next_in_mailbox );
        tp.task_and_tag = 0xDEADBEEF;
#endif /* TBB_DO_ASSERT */
        free_task<is_small>(tp);
    }

    //! Return task object to the memory allocator.
    void deallocate_task( task& t ) {
#if TBB_DO_ASSERT
        task_prefix& p = t.prefix();
        p.state = 0xFF;
        p.extra_state = 0xFF; 
        poison_pointer(p.next);
#endif /* TBB_DO_ASSERT */
        NFS_Free((char*)&t-task_prefix_reservation_size);
#if COUNT_TASK_NODES
        task_node_count -= 1;
#endif /* COUNT_TASK_NODES */
    }

    //! True if running on a worker thread, false otherwise.
    inline bool is_worker() {
        return (dummy_slot.task_pool->prefix().arena_index < arena->prefix().number_of_workers);
    }

    //! No tasks to steal since last snapshot was taken
    static const Gate::state_t SNAPSHOT_EMPTY = 0;

    //! At least one task has been offered for stealing since the last snapshot started
    static const Gate::state_t SNAPSHOT_FULL = -1;

    //! Gate is permanently open
    static const Gate::state_t SNAPSHOT_PERMANENTLY_OPEN = -2;

    //! If necessary, inform gate that task was added to pool recently.
    void mark_pool_full();

    //! Wait while pool is empty
    /** Return true if pool transitioned from empty to non-empty while thread was waiting. */
    bool wait_while_pool_is_empty();
                 
#if TEST_ASSEMBLY_ROUTINES
    /** Defined in test_assembly.cpp */
    void test_assembly_routines();
#endif /* TEST_ASSEMBLY_ROUTINES */

#if COUNT_TASK_NODES
    intptr get_task_node_count( bool count_arena_workers = false ) {
        return task_node_count + (count_arena_workers? arena->workers_task_node_count(): 0);
    }
#endif /* COUNT_TASK_NODES */

    //! Special value used to mark return_list as not taking any more entries.
    static task* plugged_return_list() {return (task*)(intptr)(-1);}

    //! Number of small tasks that have been allocated by this scheduler. 
    intptr small_task_count;

    //! List of small tasks that have been returned to this scheduler by other schedulers.
    task* return_list;

    //! Free a small task t that that was allocated by a different scheduler 
    void free_nonlocal_small_task( task& t ); 

#if __TBB_EXCEPTIONS
    //! Padding isolating thread local members from members that can be written to by other threads.
    char _padding1[NFS_MaxLineSize - sizeof(context_list_node_t)];

    //! Head of the thread specific list of task group contexts.
    context_list_node_t context_list_head;

    //! Mutex protecting access to the list of task group contexts.
    spin_mutex context_list_mutex;

    //! Used to form the list of master thread schedulers.
    scheduler_list_node_t my_node;

    //! Thread local counter of cancellation requests.
    /** When this counter equals global_cancel_count, the cancellation state known
        to this thread is synchronized with the global cancellation state.
        \sa #global_cancel_count **/
    uintptr_t local_cancel_count;

    //! Propagates cancellation request to all descendants of the argument context.
    void propagate_cancellation ( task_group_context* ctx );

    //! Propagates cancellation request to contexts registered by this scheduler.
    void propagate_cancellation ();
#endif /* __TBB_EXCEPTIONS */
}; // class GenericScheduler

#if __TBB_EXCEPTIONS
//------------------------------------------------------------------------
// auto_empty_task
//------------------------------------------------------------------------

//! Smart holder for the empty task class with automatic destruction
class auto_empty_task {
    task* my_task;
    GenericScheduler* my_scheduler;
public:
#if __TBB_EXCEPTIONS
    auto_empty_task ( GenericScheduler *s, int depth, task_group_context* context ) 
        : my_task( new(&s->allocate_task(sizeof(empty_task), depth, NULL, context)) empty_task )
#else
    auto_empty_task ( GenericScheduler *s, int depth )
        : my_task( new(&s->allocate_task(sizeof(empty_task), depth, NULL)) empty_task )
#endif /* __TBB_EXCEPTIONS */
        , my_scheduler(s)
    {}
    // empty_task has trivial destructor, so call free_task directly instead of going through destroy_task.
    ~auto_empty_task () { my_scheduler->free_task<GenericScheduler::is_small_local>(*my_task); }

    operator task& () { return *my_task; }
    task* operator & () { return my_task; }
    task_prefix& prefix () { return my_task->prefix(); }
}; // class auto_empty_task
#endif /* __TBB_EXCEPTIONS */

//------------------------------------------------------------------------
// GenericScheduler implementation
//------------------------------------------------------------------------

#if __TBB_EXCEPTIONS
inline task& GenericScheduler::allocate_task( size_t number_of_bytes, depth_type depth, task* parent, task_group_context* context ) {
#else
inline task& GenericScheduler::allocate_task( size_t number_of_bytes, depth_type depth, task* parent ) {
#endif /* __TBB_EXCEPTIONS */
    GATHER_STATISTIC(current_active+=1);
    task* t = free_list;
    if( number_of_bytes<=quick_task_size ) {
        if( t ) {
            GATHER_STATISTIC(current_length-=1);
            __TBB_ASSERT( t->state()==task::freed, "free list of tasks is corrupted" );
            free_list = t->prefix().next;
        } else if( return_list ) {
            // No fence required for read of return_list above, because __TBB_FetchAndStoreW has a fence.
            t = (task*)__TBB_FetchAndStoreW( &return_list, 0 );
            __TBB_ASSERT( t, "another thread emptied the return_list" );
            __TBB_ASSERT( t->prefix().origin==this, "task returned to wrong return_list" );
            ITT_NOTIFY( sync_acquired, &return_list );
            free_list = t->prefix().next;
        } else {
            t = (task*)((char*)NFS_Allocate( task_prefix_reservation_size+quick_task_size, 1, NULL ) + task_prefix_reservation_size );
#if COUNT_TASK_NODES
            ++task_node_count;
#endif /* COUNT_TASK_NODES */
            t->prefix().origin = this;
            ++small_task_count;
        }
    } else {
        GATHER_STATISTIC(current_big_malloc+=1);
        t = (task*)((char*)NFS_Allocate( task_prefix_reservation_size+number_of_bytes, 1, NULL ) + task_prefix_reservation_size );
#if COUNT_TASK_NODES
        ++task_node_count;
#endif /* COUNT_TASK_NODES */
        t->prefix().origin = NULL;
    }
    task_prefix& p = t->prefix();
#if __TBB_EXCEPTIONS
    p.context = context;
#endif /* __TBB_EXCEPTIONS */
    p.owner = this;
    p.ref_count = 0;
    p.depth = int(depth);
    p.parent = parent;
    // In TBB 3.0 and later, the constructor for task sets extra_state to indicate the version of the tbb/task.h header.
    // In TBB 2.0 and earlier, the constructor leaves extra_state as zero.
    p.extra_state = 0;
    p.affinity = 0;
    p.state = task::allocated;
    return *t;
}

template<GenericScheduler::hint h>
inline void GenericScheduler::free_task( task& t ) {
    GATHER_STATISTIC(current_active-=1);
    task_prefix& p = t.prefix();
    // Verify that optimization hints are correct.
    __TBB_ASSERT( h!=is_small_local || p.origin==this, NULL );
    __TBB_ASSERT( !(h&is_small) || p.origin, NULL );
#if TBB_DO_ASSERT
    p.depth = 0xDEADBEEF;
    p.ref_count = 0xDEADBEEF;
    poison_pointer(p.owner);
#endif /* TBB_DO_ASSERT */
    __TBB_ASSERT( 1L<<t.state() & (1L<<task::executing|1L<<task::allocated), NULL );
    p.state = task::freed;
    if( h==is_small_local || p.origin==this ) {
        GATHER_STATISTIC(current_length+=1);
        p.next = free_list;
        free_list = &t;
    } else if( !(h&is_local) && p.origin ) {
        free_nonlocal_small_task(t);
    } else {
        deallocate_task(t);
    }
}

void GenericScheduler::free_nonlocal_small_task( task& t ) {
    __TBB_ASSERT( t.state()==task::freed, NULL );
    GenericScheduler& s = *static_cast<GenericScheduler*>(t.prefix().origin);
    __TBB_ASSERT( &s!=this, NULL );
    for(;;) {
        task* old = s.return_list;
        if( old==plugged_return_list() ) 
            break;
        // Atomically insert t at head of s.return_list
        t.prefix().next = old; 
        ITT_NOTIFY( sync_releasing, &s.return_list );
        if( __TBB_CompareAndSwapW( &s.return_list, (intptr)&t, (intptr)old )==(intptr)old ) 
            return;
    }
    deallocate_task(t);
    if( __TBB_FetchAndDecrementWrelease( &s.small_task_count )==1 ) {
        // We freed the last task allocated by scheduler s, so it's our responsibility
        // to free the scheduler.
        NFS_Free( &s );
    }
}

inline void GenericScheduler::mark_pool_full() {
    // Double-check idiom that is deliberately sloppy about memory fences.
    // Technically, to avoid missed wakeups, there should be a full memory fence between the point we 
    // released the task pool (i.e. spawned task) and read the gate's state.  However, adding such a 
    // fence might hurt overall performance more than it helps, because the fence would be executed 
    // on every task pool release, even when stealing does not occur.  Since TBB allows parallelism, 
    // but never promises parallelism, the missed wakeup is not a correctness problem.
    Gate::state_t snapshot = arena->prefix().gate.get_state();
    if( snapshot!=SNAPSHOT_FULL && snapshot!=SNAPSHOT_PERMANENTLY_OPEN ) 
        arena->prefix().gate.try_update( SNAPSHOT_FULL, SNAPSHOT_PERMANENTLY_OPEN, true );
}

bool GenericScheduler::wait_while_pool_is_empty() {
    for(;;) {
        Gate::state_t snapshot = arena->prefix().gate.get_state();
        switch( snapshot ) {
            case SNAPSHOT_EMPTY:
                arena->prefix().gate.wait();
                return true;
            case SNAPSHOT_FULL: {
                // Use unique id for "busy" in order to avoid ABA problems.
                const Gate::state_t busy = Gate::state_t(this);
                // Request permission to take snapshot
                arena->prefix().gate.try_update( busy, SNAPSHOT_FULL );
                if( arena->prefix().gate.get_state()==busy ) {
                    // Got permission.  Take the snapshot.
                    size_t n = arena->prefix().limit;
                    size_t k; 
                    for( k=0; k<n; ++k ) 
                        if( arena->slot[k].steal_end>=0 ) 
                            break;
                    // Test and test-and-set.
                    if( arena->prefix().gate.get_state()==busy ) {
                        if( k>=n ) {
                            arena->prefix().gate.try_update( SNAPSHOT_EMPTY, busy );
                            continue;
                        } else {
                            arena->prefix().gate.try_update( SNAPSHOT_FULL, busy );
                        }
                    }
                } 
                return false;
            }
            default:
                // Another thread is taking a snapshot or gate is permanently open.
                return false;
        }
    }
}

//------------------------------------------------------------------------
// CustomScheduler
//------------------------------------------------------------------------

//! A scheduler with a customized evaluation loop.
/** The customization can use SchedulerTraits to make decisions without needing a run-time check. */
template<typename SchedulerTraits>
class CustomScheduler: private GenericScheduler {
    //! Scheduler loop that dispatches tasks.
    /** If child is non-NULL, it is dispatched first.
        Then, until "parent" has a reference count of 1, other task are dispatched or stolen. */
    /*override*/void wait_for_all( task& parent, task* child );

    typedef CustomScheduler<SchedulerTraits> scheduler_type;

    //! Construct a CustomScheduler
    CustomScheduler( Arena* arena ) : GenericScheduler(arena) {}

public:
    static GenericScheduler* allocate_scheduler( Arena* arena ) {
        __TBB_ASSERT( arena, "missing arena" );
        scheduler_type* s = (scheduler_type*)NFS_Allocate(sizeof(scheduler_type),1,NULL);
        new( s ) scheduler_type(  arena );
        __TBB_ASSERT( s->dummy_slot.task_pool->assert_okay(), NULL );
        return s;
    }
};

//------------------------------------------------------------------------
// AssertOkay
//------------------------------------------------------------------------
#if TBB_DO_ASSERT
/** Logically, this method should be a member of class task.
    But we do not want to publish it, so it is here instead. */
static bool AssertOkay( const task& task ) {
    __TBB_ASSERT( &task!=NULL, NULL );
    __TBB_ASSERT( (uintptr)&task % task_alignment == 0, "misaligned task" );
    __TBB_ASSERT( (unsigned)task.state()<=(unsigned)task::recycle, "corrupt task (invalid state)" );
    __TBB_ASSERT( task.prefix().depth<1L<<30, "corrupt task (absurd depth)" );
    return true;
}
#endif /* TBB_DO_ASSERT */

//------------------------------------------------------------------------
// Methods of Arena
//------------------------------------------------------------------------
Arena* Arena::allocate_arena( unsigned number_of_slots, unsigned number_of_workers, stack_size_type stack_size) {
    __TBB_ASSERT( sizeof(ArenaPrefix) % NFS_GetLineSize()==0, "ArenaPrefix not multiple of cache line size" );
    __TBB_ASSERT( sizeof(mail_outbox)==NFS_MaxLineSize, NULL );
    size_t n = sizeof(ArenaPrefix) + number_of_slots*(sizeof(mail_outbox)+sizeof(ArenaSlot));

    unsigned char* storage = (unsigned char*)NFS_Allocate( n, 1, NULL );
    memset( storage, 0, n );
    Arena* a = (Arena*)(storage + sizeof(ArenaPrefix)+ number_of_slots*(sizeof(mail_outbox)));
    __TBB_ASSERT( sizeof(a->slot[0]) % NFS_GetLineSize()==0, "Arena::slot size not multiple of cache line size" );
    __TBB_ASSERT( (uintptr)a % NFS_GetLineSize()==0, NULL );
    new( &a->prefix() ) ArenaPrefix( number_of_slots, number_of_workers );

    // Allocate the worker_list
    WorkerDescriptor * w = new WorkerDescriptor[number_of_workers];
    memset( w, 0, sizeof(WorkerDescriptor)*(number_of_workers));
    a->prefix().worker_list = w;

#if TBB_DO_ASSERT
    // Verify that earlier memset initialized the mailboxes.
    for( unsigned j=1; j<=number_of_slots; ++j )
        a->mailbox(j).assert_is_initialized();
#endif /* TBB_DO_ASSERT */

    size_t k;
    // Mark each worker slot as locked and unused
    for( k=0; k<number_of_workers; ++k ) {
        a->slot[k].steal_end = -3;
        w[k].arena = a;
        w[k].thread_stack_size = stack_size;
    }
    // Mark rest of slots as unused
    for( ; k<number_of_slots; ++k )
        a->slot[k].steal_end = -4;

    // Publish the Arena.  
    // A memory release fence is not required here, because workers have not started yet,
    // and concurrent masters inspect TheArena while holding TheArenaMutex.
    __TBB_ASSERT( !TheArena, NULL );
    TheArena = a;

    // Attach threads to workers
    if( number_of_workers>0 ) {
        a->prefix().worker_list[0].start_one_worker_thread();
    }
    return a;
}

void Arena::terminate_workers() {
    int n = prefix().number_of_workers;
    __TBB_ASSERT( n>=0, "negative number of workers; casting error?" );
    for( int i=n; --i>=0; ) {
        WorkerDescriptor& w = prefix().worker_list[i];
        if( w.scheduler || __TBB_CompareAndSwapW( &w.scheduler, intptr(-1), intptr(0) ) ) {
            // Worker published itself.  Tell worker to quit.
            ITT_NOTIFY(sync_acquired, &w.scheduler);
            task* t = __TBB_load_with_acquire(w.scheduler)->dummy_task;
            ITT_NOTIFY(sync_releasing, &t->prefix().ref_count);
            t->prefix().ref_count = 1;
        } else {
            // Worker did not publish itself yet, and we have set w.scheduler to -1, 
            // which tells the worker that it should never publish itself.
        }
    }
    // Permanently wake up sleeping workers
    prefix().gate.try_update( GenericScheduler::SNAPSHOT_PERMANENTLY_OPEN, GenericScheduler::SNAPSHOT_PERMANENTLY_OPEN, true );
    // Wait for all published workers to quit
    for( int i=n; --i>=0; ) {
        WorkerDescriptor& w = prefix().worker_list[i];
        if( intptr(w.scheduler)!=-1 ) {
#if USE_WINTHREAD
            DWORD status = WaitForSingleObject( w.thread_handle, INFINITE );
            if( status==WAIT_FAILED ) {
                fprintf(stderr,"Arena::terminate_workers: WaitForSingleObject failed\n");
                exit(1);
            }
            CloseHandle( w.thread_handle );
            w.thread_handle = (HANDLE)0;
#else
            int status = pthread_join( w.thread_handle, NULL );
            if( status )
                handle_perror(status,"pthread_join");
#endif /* USE_WINTHREAD */
        }
    }
    // All workers have quit
    // Drain mailboxes
    // FIXME - each scheduler should plug-and-drain its own mailbox when it terminates.
    intptr drain_count = 0;
    for( unsigned i=1; i<=prefix().number_of_slots; ++i )
        drain_count += mailbox(i).drain();
#if COUNT_TASK_NODES
    prefix().task_node_count -= drain_count;
#if !TEST_ASSEMBLY_ROUTINES
    if( prefix().task_node_count ) {
        fprintf(stderr,"warning: leaked %ld task objects\n", long(prefix().task_node_count));
    }
#endif /* !TEST_ASSEMBLY_ROUTINES */
#endif /* COUNT_TASK_NODES */
    remove_gc_reference();
}

void Arena::remove_gc_reference() {
    __TBB_ASSERT( this, "attempt to remove reference to NULL Arena" );
    if( --prefix().gc_ref_count==0 ) {
        delete[] prefix().worker_list;
        prefix().~ArenaPrefix();
        free_arena();
    }
}

#if COUNT_TASK_NODES
intptr Arena::workers_task_node_count() {
    intptr result = 0;
    for( unsigned i=0; i<prefix().number_of_workers; ++i ) {
        GenericScheduler* s = prefix().worker_list[i].scheduler;
        if( s )
            result += s->task_node_count;
    }
    return result;
}
#endif

//------------------------------------------------------------------------
// Methods of GenericScheduler
//------------------------------------------------------------------------
#if _MSC_VER && !defined(__INTEL_COMPILER)
#pragma warning(disable:4355)
#endif
GenericScheduler::GenericScheduler( Arena* arena_ ) :
    deepest(-1),
    array_size(0),
    arena_slot(&dummy_slot),
    arena(arena_),
    random( unsigned(this-(GenericScheduler*)NULL) ),
    free_list(NULL),
    innermost_running_task(NULL),
    dummy_task(NULL),
    ref_count(1),
    my_affinity_id(0),
    occupy_tls(false),
#if __TBB_SCHEDULER_OBSERVER
    local_last_observer_proxy(NULL),
#endif /* __TBB_SCHEDULER_OBSERVER */
#if COUNT_TASK_NODES
   task_node_count(0),
#endif /* COUNT_TASK_NODES */
#if STATISTICS
    current_active(0),
    current_length(0),
    current_big_malloc(0),
    execute_count(0),
    steal_count(0),
    mail_received_count(0),
    proxy_execute_count(0),
    proxy_steal_count(0),
    proxy_bypass_count(0),
#endif /* STATISTICS */
    small_task_count(1),   // Extra 1 is a guard reference
    return_list(NULL)
{
    TaskPool* t = TaskPool::allocate_task_pool(TaskPool::min_array_size);
    dummy_slot.task_pool = t;
    t->prefix().steal_begin = depth_type(array_size);
    t->prefix().arena_index = TaskPoolPrefix::null_arena_index;
    dummy_slot.steal_end = -2;
    dummy_slot.owner_waits = false;
    array_size = TaskPool::min_array_size;
#if __TBB_EXCEPTIONS
    dummy_task = &allocate_task( sizeof(task), -1, NULL, NULL );
    context_list_head.my_prev = &context_list_head;
    context_list_head.my_next = &context_list_head;
#else
    dummy_task = &allocate_task( sizeof(task), -1, NULL );
#endif /* __TBB_EXCEPTIONS */
    dummy_task->prefix().ref_count = 2;
    __TBB_ASSERT( assert_okay(), "constructor error" );
}

#if TBB_DO_ASSERT||TEST_ASSEMBLY_ROUTINES
bool GenericScheduler::assert_okay() const {
    __TBB_ASSERT( array_size>=TaskPool::min_array_size, NULL );
#if TBB_DO_ASSERT>=2||TEST_ASSEMBLY_ROUTINES
    acquire_task_pool();
    TaskPool* tp = dummy_slot.task_pool;
    __TBB_ASSERT( tp, NULL );
    for( depth_type k=0; k<depth_type(array_size); ++k ) {
        for( task* t = tp->array[k]; t; t=t->prefix().next ) {
            __TBB_ASSERT( deepest>=k, "deepest not set properly" );
            __TBB_ASSERT( t->prefix().depth==k, NULL );
            __TBB_ASSERT( t->prefix().owner==this, NULL );
        }
    }
    release_task_pool();
#endif /* TBB_DO_ASSERT>=2||TEST_ASSEMBLY_ROUTINES */
    return true;
}
#endif /* TBB_DO_ASSERT||TEST_ASSEMBLY_ROUTINES */

#if __TBB_EXCEPTIONS

void GenericScheduler::propagate_cancellation () {
    spin_mutex::scoped_lock lock(context_list_mutex);
    // Acquire fence is necessary to ensure that the subsequent node->my_next load 
    // returned the correct value in case it was just inserted in another thread.
    // The fence also ensures visibility of the correct my_parent value.
    context_list_node_t *node = __TBB_load_with_acquire(context_list_head.my_next);
    while ( node != &context_list_head ) {
        task_group_context *ctx = __TBB_get_object_addr(task_group_context, my_node, node);
            // The absence of acquire fence while reading my_cancellation_requested may result 
            // in repeated traversals of the same parents chain if another group (precedent or 
            // descendant) belonging to the tree being canceled sends cancellation request of 
            // its own around the same time. I assume that such situations are less frequent 
            // than uncontended cancellation, and cache coherency mechanisms are efficient enough 
            // to deliver updated values most of the time.
        if ( !ctx->my_cancellation_requested )
            ctx->propagate_cancellation_from_ancestors();
        node = node->my_next;
    }
}

/** Propagates cancellation down the tree of dependent contexts by walking each 
    thread's local list of contexts **/
void GenericScheduler::propagate_cancellation ( task_group_context* ctx ) {
    __TBB_ASSERT ( ctx->my_cancellation_requested, "No cancellation request in the context" );
    // The whole propagation algorithm is under the lock in order to ensure correctness 
    // in case of parallel cancellations at the different levels of the context tree.
    // See the note 2 at the bottom of the file.
    mutex::scoped_lock lock(the_scheduler_list_mutex);
    // Advance global cancellation state
    __TBB_FetchAndAddWrelease(&global_cancel_count, 1);
    // First propagate to workers using arena to access their context lists
    size_t num_workers = arena->prefix().number_of_workers;
    for ( size_t i = 0; i < num_workers; ++i ) {
        // No fence is necessary here since the context list of worker's scheduler 
        // can contain anything of interest only after the first stealing was done
        // by that worker. And doing it applies the necessary fence
        GenericScheduler *s = arena->prefix().worker_list[i].scheduler;
        // If the worker is in the middle of its startup sequence, skip it.
        if ( s )
            s->propagate_cancellation();
    }
    // Then propagate to masters using the global list of master's schedulers
    scheduler_list_node_t *node = the_scheduler_list_head.my_next;
    while ( node != &the_scheduler_list_head ) {
        __TBB_get_object_addr(GenericScheduler, my_node, node)->propagate_cancellation();
        node = node->my_next;
    }
    // Now sync up the local counters
    for ( size_t i = 0; i < num_workers; ++i ) {
        GenericScheduler *s = arena->prefix().worker_list[i].scheduler;
        // If the worker is in the middle of its startup sequence, skip it.
        if ( s )
            s->local_cancel_count = global_cancel_count;
    }
    node = the_scheduler_list_head.my_next;
    while ( node != &the_scheduler_list_head ) {
        __TBB_get_object_addr(GenericScheduler, my_node, node)->local_cancel_count = global_cancel_count;
        node = node->my_next;
    }
}
#endif /* __TBB_EXCEPTIONS */


void GenericScheduler::grow( TaskPool::size_type minimum_size ) {
    TBB_TRACE(("%p.grow(minimum_size=%lx)\n", this, minimum_size ));
    __TBB_ASSERT( assert_okay(), NULL );

    // Might need to resize the underlying array.
    // Get a fresh zeroed array before acquiring the old array.
    TaskPool::size_type b_size = 2*minimum_size;
    __TBB_ASSERT( b_size>=TaskPool::min_array_size, NULL );
    TaskPool* new_pool = TaskPool::allocate_task_pool( b_size );
    __TBB_ASSERT( assert_okay(), NULL );
    acquire_task_pool();
    TaskPool* old_pool = dummy_slot.task_pool;
    memcpy( &new_pool->prefix(), &old_pool->prefix(), TaskPool::byte_size(array_size) );
    arena_slot->task_pool = dummy_slot.task_pool = new_pool;
    array_size = b_size;
    release_task_pool();
    old_pool->free_task_pool();
    __TBB_ASSERT( assert_okay(), NULL );
}

GenericScheduler* GenericScheduler::allocate_scheduler( Arena* arena ) {
    switch( SchedulerTraitsId ) {
        /* DefaultSchedulerTraits::id is listed explicitly as a case so that the host compiler
           will issue an error message if it is the same as another id in the list. */
        default:
        case DefaultSchedulerTraits::id:
            return CustomScheduler<DefaultSchedulerTraits>::allocate_scheduler(arena);
        case IntelSchedulerTraits::id:
            return CustomScheduler<IntelSchedulerTraits>::allocate_scheduler(arena);
    }
}

void GenericScheduler::free_scheduler() {
    if( arena_slot!=&dummy_slot ) {
        leave_arena(/*compress=*/false);
    }
    unregister_from_thread_local_storage();
#if __TBB_EXCEPTIONS
    task_group_context* &context = dummy_task->prefix().context;
    // Only master thread's dummy task has a context
    if ( context != &dummy_context) {
        //! \todo Add assertion that master's dummy task context does not have children
        context->task_group_context::~task_group_context();
        NFS_Free(context);
        {
            mutex::scoped_lock lock(the_scheduler_list_mutex);
            my_node.my_next->my_prev = my_node.my_prev;
            my_node.my_prev->my_next = my_node.my_next;
        }
    }
#endif /* __TBB_EXCEPTIONS */
    free_task<is_small_local>( *dummy_task );

    // k accounts for a guard reference and each task that we deallocate.
    intptr k = 1;
    for(;;) {
        while( task* t = free_list ) {
            free_list = t->prefix().next;
            deallocate_task(*t);
            ++k;
        }
        if( return_list==plugged_return_list() ) 
            break;
        free_list = (task*)__TBB_FetchAndStoreW( &return_list, (intptr)plugged_return_list() );
    }

#if COUNT_TASK_NODES
    arena->prefix().task_node_count += task_node_count;
#endif /* COUNT_TASK_NODES */
#if STATISTICS
    the_statistics.record( execute_count, steal_count, mail_received_count,
                           proxy_execute_count, proxy_steal_count, proxy_bypass_count );
#endif /* STATISTICS */
    dummy_slot.task_pool->free_task_pool();
    dummy_slot.task_pool = NULL;
    // Update small_task_count last.  Doing so sooner might cause another thread to free *this.
    __TBB_ASSERT( small_task_count>=k, "small_task_count corrupted" );
    if( __TBB_FetchAndAddW( &small_task_count, -k )==k ) 
        NFS_Free( this );
}

inline void GenericScheduler::acquire_task_pool() const {
    __TBB_ASSERT( arena_slot, "arena_slot not set" );
    __TBB_ASSERT( deepest>=-1, NULL );
    __TBB_ASSERT( arena_slot->owner_waits==false, "slot ownership corrupt?" );
    ExponentialBackoff backoff;
    bool sync_prepare_done = false;
    for(;;) {
#if TEST_ASSEMBLY_ROUTINES
        __TBB_ASSERT( (arena_slot->steal_end&1)==0, "already acquired" );
#endif /* TEST_ASSEMBLY_ROUTINES */
        __TBB_ASSERT( arena_slot==&dummy_slot || arena_slot==&arena->slot[dummy_slot.task_pool->prefix().arena_index], "slot ownership corrupt?" );
        __TBB_ASSERT( arena_slot->task_pool==dummy_slot.task_pool, "slot ownership corrupt?" );
        depth_type steal_end = arena_slot->steal_end;
        if( (steal_end&1)==0 && (__TBB_CompareAndSwapW( &arena_slot->steal_end, steal_end|1, steal_end )&1)==0 ) {
            // We acquired our own slot
            ITT_NOTIFY(sync_acquired, arena_slot);
            arena_slot->owner_waits = false;
            break;
        } else {
            // Someone else acquired a lock, so pause and do exponential backoff.
            if( !sync_prepare_done ) {
                // Start waiting
                ITT_NOTIFY(sync_prepare, arena_slot);
                sync_prepare_done = true;
            } else {
                // after 2nd attempt, still can't acquire own pool;
                // need notify others that the owner is waiting
                arena_slot->owner_waits = true;
            }
            backoff.pause();
#if TEST_ASSEMBLY_ROUTINES
            __TBB_ASSERT( arena_slot->task_pool==dummy_slot.task_pool, NULL );
#endif /* TEST_ASSEMBLY_ROUTINES */
        }
    }
    __TBB_ASSERT( arena_slot->steal_end>>1 <= depth_type(array_size), NULL );
    __TBB_ASSERT( dummy_slot.task_pool->prefix().steal_begin<=depth_type(array_size), NULL );
    __TBB_ASSERT( deepest<=depth_type(array_size), NULL );
}

inline void GenericScheduler::release_task_pool() const {
    __TBB_ASSERT( arena_slot->steal_end>>1 <= depth_type(array_size), NULL );
    __TBB_ASSERT( dummy_slot.task_pool->prefix().steal_begin<=depth_type(array_size), NULL );
    __TBB_ASSERT( deepest<=depth_type(array_size), NULL );
    ITT_NOTIFY(sync_releasing, arena_slot);
    __TBB_store_with_release( arena_slot->steal_end, 2*deepest );
}

/** Conceptually, this method should be a member of class scheduler.
    But doing so would force us to publish class scheduler in the headers. */
void GenericScheduler::spawn( task& first, task*& next ) {
    __TBB_ASSERT( assert_okay(), NULL );
    TBB_TRACE(("%p.internal_spawn enter\n", this ));
    task* first_ptr = &first;
    task** link = &first_ptr;
    for( task* t = first_ptr; ; t=*link )
    {
        __TBB_ASSERT( t->state()==task::allocated, "attempt to spawn task that is not in 'allocated' state" );
        __TBB_ASSERT( t->prefix().depth==first.prefix().depth, "tasks must have same depth" );
        t->prefix().owner = this;
        t->prefix().state = task::ready;
#if TBB_DO_ASSERT
        if( task* parent = t->parent() ) {
            internal::reference_count ref_count = parent->prefix().ref_count;
            __TBB_ASSERT( ref_count>=0, "attempt to spawn task whose parent has a ref_count<0" );
            __TBB_ASSERT( ref_count!=0, "attempt to spawn task whose parent has a ref_count==0 (forgot to set_ref_count?)" );
            parent->prefix().extra_state |= es_ref_count_active;
        }
#endif /* TBB_DO_ASSERT */
        affinity_id dst_thread=t->prefix().affinity;
        __TBB_ASSERT( dst_thread==0 || is_version_3_task(*t), "backwards compatibility to TBB 2.0 tasks is broken" );
        if( dst_thread!=0 && dst_thread!=my_affinity_id ) {
#if __TBB_EXCEPTIONS
            task_proxy& proxy = (task_proxy&)allocate_task( sizeof(task_proxy), t->depth(), NULL, NULL );
#else
            task_proxy& proxy = (task_proxy&)allocate_task( sizeof(task_proxy), t->depth(), NULL );
#endif /* __TBB_EXCEPTIONS */
            // Mark as a proxy
            proxy.prefix().extra_state = es_task_proxy;
            proxy.outbox = &arena->mailbox(dst_thread);
            proxy.task_and_tag = intptr(t)|3;
            proxy.next_in_mailbox = NULL;
            // Link proxy into list where the original task was.
            *link = &proxy;
            link = &proxy.prefix().next;
            proxy.prefix().next = t->prefix().next;
            ITT_NOTIFY( sync_releasing, proxy.outbox );
            // Mail the proxy - after this point, t->prefix().next may be changed by another thread.
            proxy.outbox->push(proxy);
            goto advance;
        }
        link = &t->prefix().next;
advance:
        if( &t->prefix().next==&next )
            break;
    }
    task::depth_type d = first_ptr->prefix().depth;
    __TBB_ASSERT(depth_type(array_size)>0, "casting overflow?");
    if( d>=depth_type(array_size) ) {
        grow( d+1 );
    }
    __TBB_ASSERT( assert_okay(), NULL );
    if( arena_slot==&dummy_slot ) {
        try_enter_arena();
        __TBB_ASSERT( arena_slot->steal_end&1, NULL );
    } else {
        acquire_task_pool();
    }
    TaskPool* tp = dummy_slot.task_pool;

    *link = tp->array[d];
    tp->array[d] = first_ptr;

    if( d>deepest )
        deepest = d;
    if( d<tp->prefix().steal_begin )
        tp->prefix().steal_begin = d;

    release_task_pool();
    mark_pool_full();
    __TBB_ASSERT( assert_okay(), NULL );

    TBB_TRACE(("%p.internal_spawn exit\n", this ));
}

void GenericScheduler::spawn_root_and_wait( task& first, task*& next ) {
    __TBB_ASSERT( &first, NULL );
#if __TBB_EXCEPTIONS
    auto_empty_task dummy(this, first.prefix().depth-1, first.prefix().context);
#else
    empty_task& dummy = *new(&allocate_task( sizeof(empty_task), first.prefix().depth-1, NULL )) empty_task;
#endif /* __TBB_EXCEPTIONS */
    internal::reference_count n = 0;
    for( task* t=&first; ; t=t->prefix().next ) {
        ++n;
        __TBB_ASSERT( t->is_owned_by_current_thread(), "root task not owned by current thread" );
        __TBB_ASSERT( !t->prefix().parent, "not a root task, or already running" );
        t->prefix().parent = &dummy;
        if( &t->prefix().next==&next ) break;
#if __TBB_EXCEPTIONS
        __TBB_ASSERT( t->prefix().context == t->prefix().next->prefix().context, 
                    "all the root tasks in list must share the same context");
#endif /* __TBB_EXCEPTIONS */
    }
    dummy.prefix().ref_count = n+1;
    if( n>1 )
        spawn( *first.prefix().next, next );
    TBB_TRACE(("spawn_root_and_wait((task_list*)%p): calling %p.loop\n",&first,this));
    wait_for_all( dummy, &first );
#if !__TBB_EXCEPTIONS
    // empty_task has trivial destructor, so call free_task directly instead of going through destroy_task.
    free_task<is_small_local>( dummy );
#endif /* __TBB_EXCEPTIONS */
    TBB_TRACE(("spawn_root_and_wait((task_list*)%p): return\n",&first));
}

inline task* GenericScheduler::get_mailbox_task() {
    __TBB_ASSERT( my_affinity_id>0, "not in arena" );
    task* result = NULL;
    while( task_proxy* t = inbox.pop() ) {
        intptr tat = __TBB_load_with_acquire(t->task_and_tag);
        __TBB_ASSERT( tat==task_proxy::mailbox_bit || tat==(tat|3)&&tat!=3, NULL );
        if( tat!=task_proxy::mailbox_bit && __TBB_CompareAndSwapW( &t->task_and_tag, task_proxy::pool_bit, tat )==tat ) {
            // Sucessfully grabbed the task, and left pool seeker with job of freeing the proxy.
            ITT_NOTIFY( sync_acquired, inbox.outbox() );
            result = (task*)(tat & ~3);
            result->prefix().owner = this;
            break;
        }
        free_task_proxy( *t );
    }
    return result;
}

inline task* GenericScheduler::strip_proxy( task_proxy* tp ) {
    __TBB_ASSERT( tp->prefix().extra_state==es_task_proxy, NULL );
    intptr tat = __TBB_load_with_acquire(tp->task_and_tag);
    if( (tat&3)==3 ) {
        // proxy is shared by a pool and a mailbox.
        // Attempt to transition it to "empty proxy in mailbox" state.
        if( __TBB_CompareAndSwapW( &tp->task_and_tag, task_proxy::mailbox_bit, tat )==tat ) {
            // Successfully grabbed the task, and left the mailbox with the job of freeing the proxy.
            goto success;
        }
        __TBB_ASSERT( tp->task_and_tag==task_proxy::pool_bit, NULL );
        tat = task_proxy::pool_bit;
    } else {
        // We have exclusive access to the proxy
        __TBB_ASSERT( (tat&3)==task_proxy::pool_bit, "task did not come from pool?" );
    }
    tp->prefix().state = task::allocated;
    free_task_proxy( *tp );
    if(!(tat&~3)) {
        // Another thread grabbed the underlying task via their mailbox
        return NULL;
    }
success:
    return (task*)(tat&~3);
}

inline task* GenericScheduler::get_task( depth_type d ) {
    task* result = NULL;
    // parallel_reduce has a subtle dependence on the order in which tasks are
    // selected from the deque for execution.  If the task selected is not the
    // next immediate task -- i.e. there is a hole between the previous task and
    // the task selected -- then the selected task must be treated as stolen and
    // the body of the task split in order to ensure the correctness of the join
    // operations.
retry:
    if( deepest>=d ) {
        acquire_task_pool();
        task** a = dummy_slot.task_pool->array;
        depth_type i = deepest;
        do {
            if( (result = a[i]) ) {
                if( !(a[i] = result->prefix().next) )
                    --i;
                break;
            }
        } while( --i>=d );
        deepest = i;
        release_task_pool();
    }
    if( result ) {
        if( is_proxy(*result) ) {
            result = strip_proxy((task_proxy*)result);
            if( !result ) {
                goto retry;
            }
            GATHER_STATISTIC( ++proxy_execute_count );
            // Following assertion should be true because TBB 2.0 tasks never specify affinity, and hence are not proxied.
            __TBB_ASSERT( is_version_3_task(*result), "backwards compatibility with TBB 2.0 broken" );
            // Task affinity has changed.
            result->note_affinity(my_affinity_id);
        }
    }
    return result;
}

task* GenericScheduler::steal_task( UnpaddedArenaSlot& arena_slot, depth_type d ) {
    task* result = NULL;
    ExponentialBackoff backoff;
    bool sync_prepare_done = false;
    depth_type steal_end = arena_slot.steal_end;
    for(;;) {
        if( steal_end>>1<d ) {
            // Nothing of interest to steal
            if( sync_prepare_done )
                ITT_NOTIFY(sync_cancel, &arena_slot);
            goto done;
        }
        if( steal_end&1 ) {
            // Someone else has lock on it.
            if( arena_slot.owner_waits ) {
                // The pool owner is waiting for it, so need to abandon locking attempts
                if( sync_prepare_done )
                    ITT_NOTIFY(sync_cancel, &arena_slot);
                goto done;
            }
            if( !sync_prepare_done ) {
                ITT_NOTIFY(sync_prepare, &arena_slot);
                sync_prepare_done = true;
            }
            // While waiting, do exponential backoff
            backoff.pause();
            steal_end = arena_slot.steal_end;
        } else {
            depth_type tmp = steal_end;
            steal_end = __TBB_CompareAndSwapW( &arena_slot.steal_end, steal_end+1, steal_end );
            if( tmp==steal_end ) {
                ITT_NOTIFY(sync_acquired, &arena_slot);
                break;
            }
        }
    }
{
    TaskPool* tp = arena_slot.task_pool;
    depth_type i = tp->prefix().steal_begin;
    if( i<d )
        i = d;
    for(; i<=steal_end>>1; ++i ) {
        for( task** r = &tp->array[i]; (result=*r)!=NULL; r = &result->prefix().next ) {
            if( is_proxy(*result) ) {
                task_proxy& tp = *static_cast<task_proxy*>(result);
                // If task will likely be grabbed by whom it was mailed to, skip it.
                if( (tp.task_and_tag&3)==3 && tp.outbox->recipient_is_idle() ) 
                    continue;
            }
            // Remove result from linked list
            *r = result->prefix().next;
            // Unlike get_task, we do not adjust i if array[i] becomes NULL.
            // The reason is that it is a waste of time, because steal_task
            // is relatively infrequent compared to insert_task, and the
            // latter updates steal_begin too.
            goto found;
        }
    }
found:
    if( tp->prefix().steal_begin>=d ) {
        tp->prefix().steal_begin = i;
        if( i>steal_end>>1 ) {
            // Pool is empty.  This is important information for threads taking snapshots.
            steal_end = -2;
        }
    }
    // Release the task pool
    ITT_NOTIFY(sync_releasing, &arena_slot);
    __TBB_store_with_release( arena_slot.steal_end, steal_end );
}
done:
    return result;
}

#define CANCELLATION_INFO_PRESENT(t) __TBB_load_with_acquire(t->prefix().context->my_cancellation_requested)

template<typename SchedulerTraits>
void CustomScheduler<SchedulerTraits>::wait_for_all( task& parent, task* child ) {
    TBB_TRACE(("%p.wait_for_all(parent=%p,child=%p) enter\n", this, &parent, child));
#if TBB_DO_ASSERT
    __TBB_ASSERT( assert_okay(), NULL );
    if( child ) {
        __TBB_ASSERT( child->prefix().owner==this, NULL );
        __TBB_ASSERT( parent.ref_count()>=2, "ref_count must be at least 2" );
    } else {
        __TBB_ASSERT( parent.ref_count()>=1, "ref_count must be at least 1" );
    }
    __TBB_ASSERT( assert_okay(), NULL );
#endif /* TBB_DO_ASSERT */
#if __TBB_EXCEPTIONS
    __TBB_ASSERT( parent.prefix().context || (is_worker() && &parent == dummy_task), "parent task does not have context" );
#endif /* __TBB_EXCEPTIONS */
    task* t = child;
    depth_type d;
    if( innermost_running_task==dummy_task ) {
        // We are in the innermost task dispatch loop of a master thread.
        __TBB_ASSERT( !is_worker(), NULL );
        // Forcefully make this loop operate at zero depth.
        d = 0;
    } else {
        d = parent.prefix().depth+1;
    }
    task* old_innermost_running_task = innermost_running_task;
#if __TBB_EXCEPTIONS
exception_was_caught:
    try {
#endif /* __TBB_EXCEPTIONS */
    // Outer loop steals tasks when necessary.
    for(;;) {
        // Middle loop evaluates tasks that are pulled off "array".
        do {
            // Inner loop evaluates tasks that are handed directly to us by other tasks.
            while(t) {
                __TBB_ASSERT(!is_proxy(*t),"unexpected proxy");
                task_prefix& pref = t->prefix();
                __TBB_ASSERT( pref.owner==this, NULL );
                task* t_next = NULL;
#if __TBB_EXCEPTIONS
                if ( !pref.context->my_cancellation_requested ) {
#endif /* __TBB_EXCEPTIONS */
                __TBB_ASSERT( 1L<<t->state() & (1L<<task::allocated|1L<<task::ready|1L<<task::reexecute), NULL );
                pref.state = task::executing;
                innermost_running_task = t;
                __TBB_ASSERT(assert_okay(),NULL);
                TBB_TRACE(("%p.wait_for_all: %p.execute\n",this,t));
                GATHER_STATISTIC( ++execute_count );
                t_next = t->execute();
#if STATISTICS
                if (t_next) {
                    affinity_id next_affinity=t_next->prefix().affinity;
                    if (next_affinity != 0 && next_affinity != my_affinity_id)
                        GATHER_STATISTIC( ++proxy_bypass_count );
                 }
#endif

#if __TBB_EXCEPTIONS
                }
                else
                    pref.state = task::executing;
#endif /* __TBB_EXCEPTIONS */
                if( t_next ) {
                    __TBB_ASSERT( t_next->state()==task::allocated,
                                "if task::execute() returns task, it must be marked as allocated" );
                    // The store here has a subtle secondary effect - it fetches *t_next into cache.
                    t_next->prefix().owner = this;
                }
                __TBB_ASSERT(assert_okay(),NULL);
                switch( task::state_type(t->prefix().state) ) {
                    case task::executing:
                        // this block was copied below to case task::recycle
                        // when making changes, check it too
                        if( task* s = t->parent() ) {
                            if( SchedulerTraits::itt_possible )
                                ITT_NOTIFY(sync_releasing, &s->prefix().ref_count);
                            if( SchedulerTraits::has_slow_atomic && s->prefix().ref_count==1 ? (s->prefix().ref_count=0, true) : __TBB_FetchAndDecrementWrelease(&s->prefix().ref_count)==1 ) {
                                if( SchedulerTraits::itt_possible )
                                    ITT_NOTIFY(sync_acquired, &s->prefix().ref_count);
                                depth_type s_depth = __TBB_load_with_acquire(s->prefix().depth);
#if TBB_DO_ASSERT
                                s->prefix().extra_state &= ~es_ref_count_active;
#endif /* TBB_DO_ASSERT */
                                s->prefix().owner = this;
                                if( !t_next && s_depth>=deepest && s_depth>=d ) {
                                    // Eliminate spawn/get_task pair.
                                    // The elimination is valid because the spawn would set deepest==s_depth,
                                    // and the subsequent call to get_task(d) would grab task s and
                                    // restore deepest to its former value.
                                    t_next = s;
                                } else {
                                    CustomScheduler<SchedulerTraits>::spawn(*s, s->prefix().next );
                                    __TBB_ASSERT(assert_okay(),NULL);
                                }
                            }
                        }
                        destroy_task( *t );
                        break;

                    case task::recycle: { // state set by recycle_as_safe_continuation()
                        t->prefix().state = task::allocated;
                        // for safe continuation, need atomically decrement ref_count;
                        // the block was copied from above case task::executing, and changed.
                        // Use "s" here as name for t, so that code resembles case task::executing more closely.
                        task* const& s = t;
                        if( SchedulerTraits::itt_possible )
                            ITT_NOTIFY(sync_releasing, &s->prefix().ref_count);
                        if( SchedulerTraits::has_slow_atomic && s->prefix().ref_count==1 ? (s->prefix().ref_count=0, true) : __TBB_FetchAndDecrementWrelease(&s->prefix().ref_count)==1 ) {
                            if( SchedulerTraits::itt_possible )
                                ITT_NOTIFY(sync_acquired, &s->prefix().ref_count);
                            // Unused load is put here for sake of inserting an "acquire" fence.
                            (void)__TBB_load_with_acquire(s->prefix().depth);
#if TBB_DO_ASSERT
                            s->prefix().extra_state &= ~es_ref_count_active;
                            __TBB_ASSERT( s->prefix().owner==this, "ownership corrupt?" );
                            __TBB_ASSERT( s->prefix().depth>=d, NULL );
#endif /* TBB_DO_ASSERT */
                            if( !t_next ) {
                                t_next = s;
                            } else {
                                CustomScheduler<SchedulerTraits>::spawn(*s, s->prefix().next );
                                __TBB_ASSERT(assert_okay(),NULL);
                            }
                        }
                        break;
                    }

                    case task::reexecute: // set by recycle_to_reexecute()
                        __TBB_ASSERT( t_next, "reexecution requires that method 'execute' return a task" );
                        TBB_TRACE(("%p.wait_for_all: put task %p back into array",this,t));
                        t->prefix().state = task::allocated;
                        CustomScheduler<SchedulerTraits>::spawn( *t, t->prefix().next );
                        __TBB_ASSERT(assert_okay(),NULL);
                        break;
#if TBB_DO_ASSERT
                    case task::allocated:
                        break;
                    case task::ready:
                        __TBB_ASSERT( false, "task is in READY state upon return from method execute()" );
                        break;
                    default:
                        __TBB_ASSERT( false, "illegal state" );
#else
                    default: // just to shut up some compilation warnings
                        break;
#endif /* TBB_DO_ASSERT */
                }
                __TBB_ASSERT( !t_next||t_next->prefix().depth>=d, NULL );
                t = t_next;
            } // end of scheduler bypass loop
            __TBB_ASSERT(assert_okay(),NULL);

            t = get_task( d );
            __TBB_ASSERT(!t || !is_proxy(*t),"unexpected proxy");
#if TBB_DO_ASSERT
            __TBB_ASSERT(assert_okay(),NULL);
            if(t) {
                AssertOkay(*t);
                __TBB_ASSERT( t->prefix().owner==this, "thread got task that it does not own" );
            }
#endif /* TBB_DO_ASSERT */
        } while( t ); // end of local task array processing loop

        inbox.set_is_idle( true );
        __TBB_ASSERT( arena->prefix().number_of_workers>0||parent.prefix().ref_count==1,
                    "deadlock detected" );
        // The state "failure_count==-1" is used only when itt_possible is true,
        // and denotes that a sync_prepare has not yet been issued.
        for( int failure_count = -static_cast<int>(SchedulerTraits::itt_possible);; ++failure_count) {
            if( parent.prefix().ref_count==1 ) {
                if( SchedulerTraits::itt_possible ) {
                    if( failure_count!=-1 ) {
                        ITT_NOTIFY(sync_prepare, &parent.prefix().ref_count);
                        // Notify Intel(R) Thread Profiler that thread has stopped spinning.
                        ITT_NOTIFY(sync_acquired, this);
                    }
                    ITT_NOTIFY(sync_acquired, &parent.prefix().ref_count);
                }
                inbox.set_is_idle( false );
                goto done;
            }
            // Try to steal a task from a random victim.
            size_t n = arena->prefix().limit;
            if( n>1 ) {
                if( !my_affinity_id || !(t=get_mailbox_task()) ) {
                    size_t k = random.get() % (n-1);
                    ArenaSlot* victim = &arena->slot[k];
                    if( victim>=arena_slot )
                        ++victim;               // Adjusts random distribution to exclude self
                    t = steal_task( *victim, d );
                    if( !t ) goto fail;
                    if( is_proxy(*t) ) {
                        t = strip_proxy((task_proxy*)t);
                        if( !t ) goto fail;
                        GATHER_STATISTIC( ++proxy_steal_count );
                    }
                    GATHER_STATISTIC( ++steal_count );
                    if( is_version_3_task(*t) )
                        t->note_affinity( my_affinity_id );
                } else {
                    GATHER_STATISTIC( ++mail_received_count );
                }
                __TBB_ASSERT(t,NULL);
#if __TBB_SCHEDULER_OBSERVER
                // No memory fence required for read of global_last_observer_proxy, because prior fence on steal/mailbox suffices.
                if( local_last_observer_proxy!=global_last_observer_proxy ) {
                    notify_entry_observers();
                }
#endif /* __TBB_SCHEDULER_OBSERVER */
                {
                    __TBB_ASSERT( t->prefix().depth>=d, NULL );
                    if( SchedulerTraits::itt_possible ) {
                        if( failure_count!=-1 ) {
                            // FIXME - might be victim, or might be selected from a mailbox
                            // Notify Intel(R) Thread Profiler that thread has stopped spinning.
                            ITT_NOTIFY(sync_acquired, this);
                            // FIXME - might be victim, or might be selected from a mailbox
                        }
                    }
                    __TBB_ASSERT(t,NULL);
                    inbox.set_is_idle( false );
                    break;
                }
            }
fail:
            if( SchedulerTraits::itt_possible && failure_count==-1 ) {
                // The first attempt to steal work failed, so notify Intel(R) Thread Profiler that
                // the thread has started spinning.  Ideally, we would do this notification
                // *before* the first failed attempt to steal, but at that point we do not
                // know that the steal will fail.
                ITT_NOTIFY(sync_prepare, this);
                failure_count = 0;
            }
            // Pause, even if we are going to yield, because the yield might return immediately.
            __TBB_Pause(PauseTime);
            int yield_threshold = 2*int(n);
            if( failure_count>=yield_threshold ) {
                __TBB_Yield();
                if( failure_count>=yield_threshold+100 ) {
                    if( d==0 && is_worker() && wait_while_pool_is_empty() )
                        failure_count = 0;
                    else
                        failure_count = yield_threshold;
                }
            }
        }
        __TBB_ASSERT(t,NULL);
        __TBB_ASSERT(!is_proxy(*t),"unexpected proxy");
        t->prefix().owner = this;
    } // end of stealing loop
#if __TBB_EXCEPTIONS
    } catch ( tbb_exception& exc ) {
        if ( t->prefix().context->cancel_group_execution() ) {
            // We are the first to signal cancellation, so store the exception that caused it.
            t->prefix().context->my_exception = exc.move();
        }
    } catch ( std::exception& exc ) {
        if ( t->prefix().context->cancel_group_execution() ) {
            // We are the first to signal cancellation, so store the exception that caused it.
            t->prefix().context->my_exception = captured_exception::allocate(typeid(exc).name(), exc.what());
        }
    } catch ( ... ) {
        if ( t->prefix().context->cancel_group_execution() ) {
            // We are the first to signal cancellation, so store the exception that caused it.
            t->prefix().context->my_exception = captured_exception::allocate("...", "Unidentified exception");
        }
    }
    goto exception_was_caught;
#endif /* __TBB_EXCEPTIONS */
done:
    parent.prefix().ref_count = 0;
#if TBB_DO_ASSERT
    parent.prefix().extra_state &= ~es_ref_count_active;
#endif /* TBB_DO_ASSERT */
    innermost_running_task = old_innermost_running_task;
    if( deepest<0 && innermost_running_task==dummy_task && arena_slot!=&dummy_slot ) {
        leave_arena(/*compress=*/true);
    }
#if __TBB_EXCEPTIONS
    __TBB_ASSERT(parent.prefix().context && dummy_task->prefix().context, "");
    task_group_context* parent_ctx = parent.prefix().context;
    if ( __TBB_load_with_acquire(parent_ctx->my_cancellation_requested) ) {
        tbb_exception *e = __TBB_load_with_acquire(parent_ctx->my_exception);
        if ( innermost_running_task == dummy_task ) {
            // We are in the innermost task dispatch loop of a master thread, and 
            // the whole task tree has been collapsed. So we may clear cancellation data.
            __TBB_store_with_release(parent_ctx->my_cancellation_requested, 0);
            __TBB_ASSERT(dummy_task->prefix().context == parent_ctx || !CANCELLATION_INFO_PRESENT(dummy_task), 
                         "Unexpected exception or cancellation data in the dummy task");
            //! \todo Add assertion that master's dummy task context does not have children
        }
        if ( e )
            e->throw_self();
    }
    __TBB_ASSERT(!is_worker() || !CANCELLATION_INFO_PRESENT(dummy_task), 
                 "Worker's dummy task context modified");
    __TBB_ASSERT(innermost_running_task != dummy_task || !CANCELLATION_INFO_PRESENT(dummy_task), 
                 "Unexpected exception or cancellation data in the master's dummy task");
#endif /* __TBB_EXCEPTIONS */
    __TBB_ASSERT( assert_okay(), NULL );
    TBB_TRACE(("%p.wait_for_all(parent=%p): return\n",this,&parent));
}

void GenericScheduler::try_enter_arena() {
    __TBB_ASSERT( arena, NULL );
    __TBB_ASSERT( arena_slot, "arena_slot not set" );
    __TBB_ASSERT( arena_slot==&dummy_slot, "already in arena?" );
    // Sync up the local cancellation state with the global one.
    unsigned n = arena->prefix().number_of_slots;
    unsigned j = unsigned(arena->prefix().limit);
    for( unsigned k=j; k<n; ++k ) {
        if( arena->slot[k].steal_end==-4 && __TBB_CompareAndSwapW( &arena->slot[k].steal_end, -4|1, -4 )==-4 ) {
            __TBB_ASSERT( arena->slot[k].steal_end==-3, "slot not really acquired?" );
            ITT_NOTIFY(sync_acquired,&arena->slot[k]);
            dummy_slot.task_pool->prefix().arena_index = k;
            arena->slot[k].task_pool = dummy_slot.task_pool;
            arena->slot[k].owner_waits = false;
            arena_slot = &arena->slot[k];
            // Successfully claimed a spot in the arena.  Update arena->prefix().limit.
            do {
                j = unsigned(arena->prefix().limit.compare_and_swap( k+1, j ));
            } while( j<=k );
            attach_mailbox( k+1 );
            break;
        }
    }
    __TBB_store_with_release( arena_slot->steal_end, 2*deepest+1 );
}

void GenericScheduler::leave_arena( bool compress ) {
    __TBB_ASSERT( arena_slot!=&dummy_slot, "not in arena" );
    // Remove myself from the arena.
    acquire_task_pool();
#if TBB_DO_ASSERT
    for( depth_type i=0; i<deepest; ++i )
        __TBB_ASSERT( !dummy_slot.task_pool->array[i], "leaving arena, but have tasks to do" );
#endif /* TBB_DO_ASSERT */
    size_t k = dummy_slot.task_pool->prefix().arena_index;
    __TBB_ASSERT( &arena->slot[k]==arena_slot, NULL );
    arena_slot->task_pool = NULL;
    my_affinity_id = 0;
    inbox.detach();
    dummy_slot.task_pool->prefix().arena_index = TaskPoolPrefix::null_arena_index;
    arena_slot = &dummy_slot;
    arena_slot->owner_waits  = false;
    size_t n = arena->prefix().number_of_workers;
    __TBB_ASSERT( !compress || k>=n, "must be master to compress" );
    size_t new_limit = arena->prefix().limit;
    if( compress && new_limit==k+1 ) {
        // Garbage collect some slots
        for(;;) {
            new_limit = arena->prefix().limit.compare_and_swap( k, k+1 );
            ITT_NOTIFY(sync_releasing, &arena->slot[k]);
            __TBB_store_with_release( arena->slot[k].steal_end, -4 );
            if( new_limit!=k+1 ) break;
            if( --k<n ) break;
            if( arena->slot[k].steal_end==-4 && __TBB_CompareAndSwapW( &arena->slot[k].steal_end, -4|1, -4 )==-4 ) {
                ITT_NOTIFY(sync_acquired,&arena->slot[k]);
            } else {
                break;
            }
        }
    } else {
        ITT_NOTIFY(sync_releasing, &arena->slot[k]);
        __TBB_store_with_release( arena->slot[k].steal_end, -4 );
    }
}

GenericScheduler* GenericScheduler::create_worker( WorkerDescriptor& w ) {
    __TBB_ASSERT( (uintptr)w.scheduler+1<2u, NULL );
    unsigned n = w.arena->prefix().number_of_workers;
    WorkerDescriptor* worker_list = w.arena->prefix().worker_list;
    __TBB_ASSERT( &w >= worker_list, NULL );
    unsigned i = unsigned(&w - worker_list);
    __TBB_ASSERT( i<n, NULL );

    // Start my children
    if( 2*i+1<n ) {
        // Have a left child, so start it.
        worker_list[2*i+1].start_one_worker_thread();
        if( 2*i+2<n ) {
            // Have a right child, so start it.
            worker_list[2*i+2].start_one_worker_thread();
        }
    }

    GenericScheduler* s = GenericScheduler::allocate_scheduler(w.arena);

    // Put myself into the arena
    ArenaSlot& slot = w.arena->slot[i];
    __TBB_ASSERT( slot.steal_end==-3, "slot not allocated as locked worker?" );
    s->arena_slot = &slot;
#if __TBB_EXCEPTIONS
    s->dummy_task->prefix().context = &dummy_context;
    // Sync up the local cancellation state with the global one. No need for fence here.
    s->local_cancel_count = global_cancel_count;
#endif /* __TBB_EXCEPTIONS */
    s->attach_mailbox( i+1 );
    TaskPool* t = s->dummy_slot.task_pool;
    t->prefix().arena_index = i;
    ITT_NOTIFY(sync_releasing, &slot);
    slot.task_pool = t;
    slot.steal_end = -2;
    slot.owner_waits = false;

#if USE_WINTHREAD
    HANDLE cur_process = GetCurrentProcess();
    BOOL bRes = DuplicateHandle(cur_process, GetCurrentThread(), cur_process, &w.thread_handle, 0, FALSE, DUPLICATE_SAME_ACCESS);
    if( !bRes ) {
        printf("ERROR: DuplicateHandle failed with status 0x%08X", GetLastError());
        w.thread_handle = INVALID_HANDLE_VALUE;
    }
#else /* USE_PTHREAD */
    w.thread_handle = pthread_self();
#endif /* USE_PTHREAD */
    // Attempt to publish worker
    ITT_NOTIFY(sync_releasing, &w.scheduler);
    // Note: really need only release fence on the compare-and-swap.
    if( __TBB_CompareAndSwapW( &w.scheduler, (intptr)s, intptr(0) )==-1 ) {
        // Master thread has already commenced terminate_workers() and not waited for us to respond.
        // Thus we are responsible for cleaning up ourselves.
        s->dummy_task->prefix().ref_count = 1;
#if USE_WINTHREAD
        CloseHandle( w.thread_handle );
        w.thread_handle = (HANDLE)0;
#else /* USE_PTHREAD */
        int status = pthread_detach( w.thread_handle );
        if( status )
            handle_perror(status,"pthread_detach");
#endif /* USE_PTHREAD */
        // Do not register scheduler in thread local storage, because the storage may be gone.
    } else {
        __TBB_ASSERT( w.scheduler==s, NULL );
        s->register_in_thread_local_storage();
    }
    return s;
}

GenericScheduler* GenericScheduler::create_master( Arena* arena ) {
    GenericScheduler* s = GenericScheduler::allocate_scheduler( arena );
    task& t = *s->dummy_task;
    s->innermost_running_task = &t;
    t.prefix().ref_count = 1;
    s->register_in_thread_local_storage();
#if __TBB_EXCEPTIONS
    // Context to be used by root tasks by default (if the user has not specified one).
    // Allocation is done by NFS allocator because we cannot reuse memory allocated 
    // for task objects since the free list is empty at the moment.
    t.prefix().context = new ( NFS_Allocate(sizeof(task_group_context), 1, NULL) ) task_group_context(task_group_context::isolated);
    scheduler_list_node_t &node = s->my_node;
    {
        mutex::scoped_lock lock(the_scheduler_list_mutex);
        node.my_next = the_scheduler_list_head.my_next;
        node.my_prev = &the_scheduler_list_head;
        the_scheduler_list_head.my_next->my_prev = &node;
        the_scheduler_list_head.my_next = &node;
    }
    // Sync up the local cancellation state with the global one. No need for fence here.
    s->local_cancel_count = global_cancel_count;
#endif /* __TBB_EXCEPTIONS */
    __TBB_ASSERT( &task::self()==&t, NULL );
#if __TBB_SCHEDULER_OBSERVER
    // Process any existing observers.
    s->notify_entry_observers();
#endif /* __TBB_SCHEDULER_OBSERVER */
    return s;
}

#if USE_WINTHREAD
unsigned WINAPI GenericScheduler::worker_routine( void* arg )
#else
void* GenericScheduler::worker_routine( void* arg )
#endif /* USE_WINTHREAD */
{
    GenericScheduler& scheduler = *create_worker(*(WorkerDescriptor*)arg);

    ITT_NAME_SET(thr_name_set, "TBB Worker Thread", 17);

#if (_WIN32||_WIN64)&&!__TBB_ipf
    // Deal with 64K aliasing.  The formula for "offset" is a Fibonacci hash function,
    // which has the desirable feature of spreading out the offsets fairly evenly
    // without knowing the total number of offsets, and furthermore unlikely to
    // accidentally cancel out other 64K aliasing schemes that Microsoft might implement later.
    // See Knuth Vol 3. "Theorem S" for details on Fibonacci hashing.
    size_t offset = (scheduler.dummy_slot.task_pool->prefix().arena_index+1) * 40503U % (1U<<16);
    // The following statement is really does need "volatile", otherwise the compiler might remove the _alloca.
    void* volatile sink_for_alloca = _alloca(offset);
#else
    // Linux thread allocators avoid 64K aliasing.
#endif /* _WIN32||_WIN64 */

#if USE_PTHREAD
    pthread_cleanup_push( cleanup_worker_thread, &scheduler );
#endif /* USE_PTHREAD */

#if USE_WINTHREAD
    __try {
#endif /* USE_WINTHREAD */

    scheduler.wait_for_all(*scheduler.dummy_task,NULL);

#if USE_WINTHREAD
    } __finally {
        cleanup_worker_thread(&scheduler);
    }
    return 0;
#elif USE_PTHREAD
    pthread_cleanup_pop( true );
    return NULL;
#else
    #error Must define USE_PTHREAD or USE_WINTHREAD
#endif /* USE_PTHREAD */
}

void GenericScheduler::cleanup_worker_thread( void* arg ) {
    TBB_TRACE(("%p.cleanup_worker_thread enter\n",arg));
    GenericScheduler& s = *(GenericScheduler*)arg;
    __TBB_ASSERT( s.dummy_slot.task_pool, "cleaning up worker with missing TaskPool" );
#if __TBB_SCHEDULER_OBSERVER
    s.notify_exit_observers(/*is_worker=*/true);
#endif /* __TBB_SCHEDULER_OBSERVER */
    Arena* a = s.arena;
    __TBB_ASSERT( s.arena_slot!=&s.dummy_slot, "worker not in arena?" );
    s.free_scheduler();
    a->remove_gc_reference();
}

//------------------------------------------------------------------------
// WorkerDescsriptor
//------------------------------------------------------------------------
void WorkerDescriptor::start_one_worker_thread() {
#if USE_WINTHREAD
    unsigned thread_id;
    unsigned local_thread_stack_size=(unsigned)thread_stack_size;
    __TBB_ASSERT( ((size_t)local_thread_stack_size==thread_stack_size), "thread_stack_size is too large for windows" );
    // The return type of _beginthreadex is "uintptr_t" on new MS compilers,
    // and 'unsigned long' on old MS compilers.  Our uintptr works for both.
    uintptr status = _beginthreadex( NULL, local_thread_stack_size, GenericScheduler::worker_routine, this, 0, &thread_id );
    if( status==0 )
        handle_perror(errno,"__beginthreadex");
    else
        CloseHandle((HANDLE)status);
#else
    int status;
    pthread_attr_t stack_size;
    status = pthread_attr_init( &stack_size );
    if( status )
        handle_perror( status, "pthread_attr_init" );
    if( thread_stack_size>0 ) {
        status = pthread_attr_setstacksize( &stack_size, thread_stack_size );
        if( status )
            handle_perror( status, "pthread_attr_setstacksize" );
    }
    // this->thread_handle will be set from the thread function to avoid possible
    // race with subsequent pthread_detach or pthread_join calls.
    pthread_t handle;
    // This if is due to an Intel Compiler Bug, tracker # C70996
    // This #if should be removed as soon as the bug is fixed
#if __APPLE__ && __TBB_x86_64
    static void *(*r)(void*) = GenericScheduler::worker_routine;
    status = pthread_create( &handle, &stack_size, r, this );
#else
    status = pthread_create( &handle, &stack_size, GenericScheduler::worker_routine, this );
#endif
    if( status )
        handle_perror( status, "pthread_create" );
#endif /* USE_WINTHREAD */
}

//------------------------------------------------------------------------
// Methods of allocate_root_proxy
//------------------------------------------------------------------------
task& allocate_root_proxy::allocate( size_t size ) {
    internal::GenericScheduler* v = GetThreadSpecific();
    __TBB_ASSERT( v, "thread did not activate a task_scheduler_init object?" );
#if __TBB_EXCEPTIONS
    task_prefix& p = v->innermost_running_task->prefix();
    // New root task becomes part of the currently running task's cancellation context
    return v->allocate_task( size, p.depth+1, NULL, p.context );
#else
    return v->allocate_task( size, v->innermost_running_task->prefix().depth+1, NULL );
#endif /* __TBB_EXCEPTIONS */
}

void allocate_root_proxy::free( task& task ) {
    internal::GenericScheduler* v = GetThreadSpecific();
    __TBB_ASSERT( v, "thread does not have initialized task_scheduler_init object?" );
#if __TBB_EXCEPTIONS
    // No need to do anything here as long as there is no context -> task connection
#endif /* __TBB_EXCEPTIONS */
    v->free_task<GenericScheduler::is_local>( task );
}

#if __TBB_EXCEPTIONS
//------------------------------------------------------------------------
// Methods of allocate_root_with_context_proxy
//------------------------------------------------------------------------
task& allocate_root_with_context_proxy::allocate( size_t size ) const {
    internal::GenericScheduler* v = GetThreadSpecific();
    __TBB_ASSERT( v, "thread did not activate a task_scheduler_init object?" );
    task_prefix& p = v->innermost_running_task->prefix();
    task& t = v->allocate_task( size, p.depth+1, NULL, &my_context );
    // The supported usage model prohibits concurrent initial binding. Thus we 
    // do not need interlocked operations or fences here.
    if ( my_context.my_kind == task_group_context::binding_required ) {
        __TBB_ASSERT ( my_context.my_owner, "Context without owner" );
        __TBB_ASSERT ( !my_context.my_parent, "Parent context set before initial binding" );
        // If we are in the innermost task dispatch loop of a master thread, then
        // there is nothing to bind this context to, and we skip the binding part.
        if ( v->innermost_running_task != v->dummy_task ) {
            // By not using the fence here we get faster code in case of normal execution 
            // flow in exchange of a bit higher probability that in cases when cancellation 
            // is in flight we will take deeper traversal branch. Normally cache coherency 
            // mechanisms are efficient enough to deliver updated value most of the time.
            uintptr_t local_count_snapshot = ((GenericScheduler*)my_context.my_owner)->local_cancel_count;
            __TBB_store_with_release(my_context.my_parent, p.context);
            uintptr_t global_count_snapshot = __TBB_load_with_acquire(global_cancel_count);
            if ( local_count_snapshot == global_count_snapshot ) {
                // It is possible that there is active cancellation request in our 
                // parents chain. Fortunately the equality of the local and global 
                // counters means that if this is the case it's already been propagated
                // to our parent.
                my_context.my_cancellation_requested = p.context->my_cancellation_requested;
            }
            else if ( !my_context.my_cancellation_requested ) {
                // Another thread was propagating cancellation request at the moment 
                // when we set our parent, but since we do not use locks we could've 
                // been skipped. So we have to make sure that we get the cancellation 
                // request if one of our ancestors has been canceled.
                my_context.propagate_cancellation_from_ancestors();
            }
        }
        my_context.my_kind = task_group_context::binding_completed;
    }
    // else the context either has already been associated with its parent or is isolated
    return t;
}

void allocate_root_with_context_proxy::free( task& task ) const {
    internal::GenericScheduler* v = GetThreadSpecific();
    __TBB_ASSERT( v, "thread does not have initialized task_scheduler_init object?" );
    // No need to do anything here as long as unbinding is performed by context destructor only.
    v->free_task<GenericScheduler::is_local>( task );
}
#endif /* __TBB_EXCEPTIONS */

//------------------------------------------------------------------------
// Methods of allocate_continuation_proxy
//------------------------------------------------------------------------
task& allocate_continuation_proxy::allocate( size_t size ) const {
    task& t = *((task*)this);
    __TBB_ASSERT( AssertOkay(t), NULL );
    GenericScheduler* v = static_cast<GenericScheduler*>(t.prefix().owner);
    __TBB_ASSERT( GetThreadSpecific()==v, "thread does not own this" );
    task* parent = t.parent();
    t.prefix().parent = NULL;
#if __TBB_EXCEPTIONS
    return v->allocate_task( size, t.prefix().depth, parent, t.prefix().context );
#else
    return v->allocate_task( size, t.prefix().depth, parent );
#endif /* __TBB_EXCEPTIONS */
}

void allocate_continuation_proxy::free( task& mytask ) const {
    task& t = *((task*)this);
    // Restore the parent as it was before the corresponding allocate was called.
    t.prefix().parent = mytask.parent();
    static_cast<GenericScheduler*>(t.prefix().owner)->free_task<GenericScheduler::is_local>(mytask);
}

//------------------------------------------------------------------------
// Methods of allocate_child_proxy
//------------------------------------------------------------------------
task& allocate_child_proxy::allocate( size_t size ) const {
    task& t = *((task*)this);
    __TBB_ASSERT( AssertOkay(t), NULL );
    GenericScheduler* v = static_cast<GenericScheduler*>(t.prefix().owner);
    __TBB_ASSERT( GetThreadSpecific()==v, "thread does not own parent" );
#if __TBB_EXCEPTIONS
    return v->allocate_task( size, t.prefix().depth+1, &t, t.prefix().context );
#else
    return v->allocate_task( size, t.prefix().depth+1, &t );
#endif /* __TBB_EXCEPTIONS */
}

void allocate_child_proxy::free( task& mytask ) const {
    task& t = *((task*)this);
    GenericScheduler* v = static_cast<GenericScheduler*>(t.prefix().owner);
    v->free_task<GenericScheduler::is_local>(mytask);
}

//------------------------------------------------------------------------
// Methods of allocate_additional_child_of_proxy
//------------------------------------------------------------------------
task& allocate_additional_child_of_proxy::allocate( size_t size ) const {
    __TBB_ASSERT( AssertOkay(self), NULL );
    __TBB_FetchAndIncrementWacquire( &parent.prefix().ref_count );
    GenericScheduler* v = static_cast<GenericScheduler*>(self.prefix().owner);
#if __TBB_EXCEPTIONS
    return v->allocate_task( size, parent.prefix().depth+1, &parent, parent.prefix().context );
#else
    return v->allocate_task( size, parent.prefix().depth+1, &parent );
#endif /* __TBB_EXCEPTIONS */
}

void allocate_additional_child_of_proxy::free( task& task ) const {
    // Undo the increment.  We do not check the result of the fetch-and-decrement.
    // We could consider be spawning the task if the fetch-and-decrement returns 1.
    // But we do not know that was the programmer's intention.
    // Furthermore, if it was the programmer's intention, the program has a fundamental
    // race condition (that we warn about in Reference manual), because the
    // reference count might have become zero before the corresponding call to
    // allocate_additional_child_of_proxy::allocate.
    __TBB_FetchAndDecrementWrelease( &parent.prefix().ref_count );
    GenericScheduler* v = static_cast<GenericScheduler*>(self.prefix().owner);

    v->free_task<GenericScheduler::is_local>(task);
}

//------------------------------------------------------------------------
// Support for auto_partitioner
//------------------------------------------------------------------------
size_t get_initial_auto_partitioner_divisor() {
    // No fence required here, because TheArena does not change after the thread starts.
    Arena* arena = TheArena;
    __TBB_ASSERT( arena, "thread did not activate a task_scheduler_init object?" );
    const size_t X_FACTOR = 4;
    return X_FACTOR * arena->prefix().number_of_workers+1;
}

//------------------------------------------------------------------------
// Methods of affinity_partitioner_base_v3
//------------------------------------------------------------------------
void affinity_partitioner_base_v3::resize( unsigned factor ) {
    Arena* arena = TheArena;
    size_t new_size = arena ? factor*(arena->prefix().number_of_workers+1) : 0;
    if( new_size!=my_size ) {
        if( my_array ) {
            NFS_Free( my_array );
            // Following two assignments must be done here for sake of exception safety.
            my_array = NULL;
            my_size = 0;
        } 
        if( new_size ) {
            my_array = static_cast<affinity_id*>(NFS_Allocate(new_size,sizeof(affinity_id), NULL ));
            memset( my_array, 0, sizeof(affinity_id)*new_size );
            my_size = new_size;
        } 
    } 
}

} // namespace internal

using namespace tbb::internal;

#if __TBB_EXCEPTIONS

//------------------------------------------------------------------------
// captured_exception
//------------------------------------------------------------------------

inline 
void copy_string ( char*& dst, const char* src ) {
    if ( src ) {
        size_t len = strlen(src) + 1;
        dst = (char*)allocate_via_handler_v3(len);
        strncpy (dst, src, len);
    }
    else
        dst = NULL;
}

void captured_exception::set ( const char* name, const char* info ) throw()
{
    copy_string(const_cast<char*&>(my_exception_name), name);
    copy_string(const_cast<char*&>(my_exception_info), info);
}

void captured_exception::clear () throw() {
    deallocate_via_handler_v3 (const_cast<char*>(my_exception_name));
    deallocate_via_handler_v3 (const_cast<char*>(my_exception_info));
}

captured_exception* captured_exception::move () throw() {
    captured_exception *e = (captured_exception*)allocate_via_handler_v3(sizeof(captured_exception));
    if ( e ) {
        new (e) captured_exception();
        e->my_exception_name = my_exception_name;
        e->my_exception_info = my_exception_info;
        e->my_dynamic = true;
        my_exception_name = my_exception_info = NULL;
    }
    return e;
}

void captured_exception::destroy () throw() {
    __TBB_ASSERT ( my_dynamic, "Method destroy can be used only on objects created by clone or allocate" );
    if ( my_dynamic ) {
        this->captured_exception::~captured_exception();
        deallocate_via_handler_v3 (this);
    }
}

captured_exception* captured_exception::allocate ( const char* name, const char* info )
{
    captured_exception *e = (captured_exception*)allocate_via_handler_v3(sizeof(captured_exception));
    if ( e ) {
        new (e) captured_exception(name, info);
        e->my_dynamic = true;
    }
    return e;
}

const char* captured_exception::name() const throw() {
    return my_exception_name;
}

const char* captured_exception::what() const throw() {
    return my_exception_info;
}

//------------------------------------------------------------------------
// task_group_context
//------------------------------------------------------------------------

task_group_context::~task_group_context () {
    if ( my_kind != isolated ) {
        __TBB_ASSERT ( my_owner == GetThreadSpecific(), "Task group context is destructed by the wrong thread" );
        GenericScheduler *s = (GenericScheduler*)my_owner;
        my_node.my_next->my_prev = my_node.my_prev;
        // By not using the fence here we get faster code in case of normal execution 
        // flow in exchange for a bit higher probability that in cases when cancellation 
        // is in flight we will take the branch containing the lock. Normally cache 
        // coherency mechanisms are efficient enough to deliver updated value most 
        // of the time.
        uintptr_t local_count_snapshot = s->local_cancel_count;
        __TBB_store_with_release( my_node.my_prev->my_next, my_node.my_next );
        if ( local_count_snapshot != __TBB_load_with_acquire(global_cancel_count) ) {
            // Another thread was propagating cancellation request when we removed
            // ourselves from the list. We must ensure that it does not access us 
            // when this destructor finishes. We'll be able to acquire the lock 
            // below only after the other thread finishes with us.
            spin_mutex::scoped_lock lock(s->context_list_mutex);
        }
    }
    if ( my_exception )
        my_exception->destroy();
}

void task_group_context::init () {
    __TBB_ASSERT ( sizeof(task_group_context) == 2 * NFS_MaxLineSize, "Context class has wrong size - check padding and members alignment" );
    __TBB_ASSERT ( (uintptr_t(this) & (sizeof(my_cancellation_requested) - 1)) == 0, "Context is improperly aligned" );
    __TBB_ASSERT ( my_kind == isolated || my_kind == bound, "Context can be created only as isolated or bound" );
    my_parent = NULL;
    my_cancellation_requested = 0;
    my_exception = NULL;
    if ( my_kind == bound ) {
        GenericScheduler *s = GetThreadSpecific();
        my_owner = s;
        __TBB_ASSERT ( my_owner, "Thread has not activated a task_scheduler_init object?" );
        // Backward links are used by this thread only, thus no fences are necessary
        my_node.my_prev = &s->context_list_head;
        s->context_list_head.my_next->my_prev = &my_node;
        // The only operation on the thread local list of contexts that may be performed 
        // concurrently is its traversal by another thread while propagating cancellation
        // request. Therefore the release fence below is necessary to ensure that the new 
        // value of my_node.my_next is visible to the traversing thread 
        // after it reads new value of v->context_list_head.my_next.
        my_node.my_next = s->context_list_head.my_next;
        __TBB_store_with_release(s->context_list_head.my_next, &my_node);
    }
}

bool task_group_context::cancel_group_execution () {
    __TBB_ASSERT ( my_cancellation_requested == 0 || my_cancellation_requested == 1, "Invalid cancellation state");
    if ( my_cancellation_requested || __TBB_CompareAndSwapW(&my_cancellation_requested, 1, 0) ) {
        // This task group has already been canceled
        return false;
    }
    GetThreadSpecific()->propagate_cancellation(this);
    return true;
}

bool task_group_context::is_group_execution_cancelled () const {
    return __TBB_load_with_acquire(my_cancellation_requested) != 0;
}

// IMPORTANT: It is assumed that this method is not used concurrently!
void task_group_context::reset () {
    //! \todo Add assertion that this context does not have children
    // No fences are necessary since this context can be accessed from another thread
    // only after stealing happened (which means necessary fences were used).
    if ( my_exception )  {
        my_exception->destroy();
        my_exception = NULL;
    }
    my_cancellation_requested = 0;
}

void task_group_context::propagate_cancellation_from_ancestors () {
    task_group_context *parent = my_parent;
    while ( parent && !parent->my_cancellation_requested )
        parent = parent->my_parent;
    if ( parent ) {
        // One of our ancestor groups was canceled. Cancel all its descendants.
        task_group_context *ctx = this;
        do {
            __TBB_store_with_release(ctx->my_cancellation_requested, 1);
            ctx = ctx->my_parent;
        } while ( ctx != parent );
    }
}

#endif /* __TBB_EXCEPTIONS */

//------------------------------------------------------------------------
// task
//------------------------------------------------------------------------

void task::internal_set_ref_count( int count ) {
    __TBB_ASSERT( count>0, "count must be positive" );
    __TBB_ASSERT( !(prefix().extra_state&GenericScheduler::es_ref_count_active), "ref_count race detected" );
    ITT_NOTIFY(sync_releasing, &prefix().ref_count);
    prefix().ref_count = count;
}

task& task::self() {
    GenericScheduler* v = GetThreadSpecific();
    __TBB_ASSERT( v->assert_okay(), NULL );
    return *v->innermost_running_task;
}

bool task::is_owned_by_current_thread() const {
    return GetThreadSpecific()==prefix().owner;
}

void task::destroy( task& victim ) {
    __TBB_ASSERT( victim.prefix().ref_count==0, "victim must have reference count of zero" );
    __TBB_ASSERT( victim.state()==task::allocated, "illegal state for victim task" );
    if( task* parent = victim.parent() ) {
        __TBB_ASSERT( parent->state()==task::allocated, "attempt to destroy child of running or corrupted parent?" );
        ITT_NOTIFY(sync_releasing, &parent->prefix().ref_count);
        __TBB_FetchAndDecrementWrelease(&parent->prefix().ref_count);
        ITT_NOTIFY(sync_acquired, &parent->prefix().ref_count);
    }
    internal::GenericScheduler* v = static_cast<internal::GenericScheduler*>(prefix().owner);
    // Victim is allowed to be owned by another thread.
    victim.prefix().owner = v;
    v->destroy_task(victim);
}

void task::spawn_and_wait_for_all( task_list& list ) {
    __TBB_ASSERT( is_owned_by_current_thread(), "'this' not owned by current thread" );
    task* t = list.first;
    if( t ) {
        if( &t->prefix().next!=list.next_ptr )
            prefix().owner->spawn( *t->prefix().next, *list.next_ptr );
        list.clear();
    }
    prefix().owner->wait_for_all( *this, t );
}

/** Defined out of line so that compiler does not replicate task's vtable. 
    It's pointless to define it inline anyway, because all call sites to it are virtual calls
    that the compiler is unlikely to optimize. */
void task::note_affinity( affinity_id id ) {
}

//------------------------------------------------------------------------
// task_scheduler_init
//------------------------------------------------------------------------

const stack_size_type MByte = 1<<20;
#if !defined(__TBB_WORDSIZE)
const stack_size_type ThreadStackSize = 1*MByte;
#elif __TBB_WORDSIZE<=4
const stack_size_type ThreadStackSize = 2*MByte;
#else
const stack_size_type ThreadStackSize = 4*MByte;
#endif

void task_scheduler_init::initialize( int number_of_threads ) {
    initialize( number_of_threads, 0 );
}

void task_scheduler_init::initialize( int number_of_threads, stack_size_type thread_stack_size ) {
    if( number_of_threads!=deferred ) {
        __TBB_ASSERT( !my_scheduler, "task_scheduler_init already initialized" );
        __TBB_ASSERT( number_of_threads==-1 || number_of_threads>=1,
                    "number_of_threads for task_scheduler_init must be -1 or positive" );
        // Double-check
        if( !__TBB_load_with_acquire(OneTimeInitializationsDone) ) {
            DoOneTimeInitializations();
        }
        GenericScheduler* s = GetThreadSpecific();
        if( s ) {
            s->ref_count += 1;
        } else {
            Arena* a;
            {
                mutex::scoped_lock lock( TheArenaMutex );
                a = TheArena;
                if( a ) {
                    a->prefix().number_of_masters += 1;
                } else {
                    if( number_of_threads==-1 )
                        number_of_threads = default_num_threads();
                    // Put cold path in separate routine.
                    a = Arena::allocate_arena( 2*number_of_threads, number_of_threads-1,
                                               thread_stack_size?thread_stack_size:ThreadStackSize );
                    __TBB_ASSERT( a->prefix().number_of_masters==1, NULL );
                    __TBB_ASSERT( TheArena==a, NULL );
                }
            }
            s = GenericScheduler::create_master( a );
        }
        my_scheduler = s;
    } else {
        __TBB_ASSERT( !thread_stack_size, "deferred initialization ignores stack size setting" );
    }
}

void task_scheduler_init::terminate() {
    GenericScheduler* s = static_cast<GenericScheduler*>(my_scheduler);
    my_scheduler = NULL;
    __TBB_ASSERT( s, "task_scheduler_init::terminate without corresponding task_scheduler_init::initialize()");
    if( !--(s->ref_count) ) {
        __TBB_ASSERT( s->dummy_slot.task_pool, "cleaning up master with missing TaskPool" );
#if __TBB_SCHEDULER_OBSERVER
        s->notify_exit_observers(false);
#endif /* __TBB_SCHEDULER_OBSERVER */
        s->free_scheduler();
        Arena* a;
        {
            mutex::scoped_lock lock( TheArenaMutex );
            a = TheArena;
            __TBB_ASSERT( a, "TheArena is missing" );
            if( --(a->prefix().number_of_masters) ) {
                a = NULL;
            } else {
                TheArena = NULL;
            }
        }
        if( a ) {
            a->terminate_workers();
        }
    }
}

int task_scheduler_init::default_num_threads() {
    // No memory fence required, because at worst each invoking thread calls NumberOfHardwareThreads.
    int n = DefaultNumberOfThreads;
    if( !n ) {
        DefaultNumberOfThreads = n = DetectNumberOfWorkers();
    }
    return n;
}

#if __TBB_SCHEDULER_OBSERVER
//------------------------------------------------------------------------
// Methods of observer_proxy
//------------------------------------------------------------------------
namespace internal {

#if TBB_DO_ASSERT
static atomic<int> observer_proxy_count;

struct check_observer_proxy_count {
    ~check_observer_proxy_count() {
        if( observer_proxy_count!=0 ) {
            fprintf(stderr,"warning: leaked %ld observer_proxy objects\n", long(observer_proxy_count));
        }
    }
};

static check_observer_proxy_count the_check_observer_proxy_count;
#endif /* TBB_DO_ASSERT */

observer_proxy::observer_proxy( task_scheduler_observer_v3& tso ) : next(NULL), observer(&tso) {
#if TBB_DO_ASSERT
    ++observer_proxy_count;
#endif /* TBB_DO_ASSERT */
    // 1 for observer
    gc_ref_count = 1;
    {
        // Append to the global list
        task_scheduler_observer_mutex_scoped_lock lock(the_task_scheduler_observer_mutex.begin()[0],/*is_writer=*/true);
        observer_proxy* p = global_last_observer_proxy;
        prev = p;
        if( p ) 
            p->next=this;
        else 
            global_first_observer_proxy = this;
        global_last_observer_proxy = this;
    }
}

void observer_proxy::remove_from_list() {
    // Take myself off the global list.  
    if( next ) 
        next->prev = prev;
    else 
        global_last_observer_proxy = prev;
    if( prev )
        prev->next = next;
    else 
        global_first_observer_proxy = next;
#if TBB_DO_ASSERT
    poison_pointer(prev);
    poison_pointer(next);
    gc_ref_count = -666;
#endif /* TBB_DO_ASSERT */
}

void observer_proxy::remove_ref_slow() {
    int r = gc_ref_count;
    while(r>1) {
        __TBB_ASSERT( r!=0, NULL );
        int r_old = gc_ref_count.compare_and_swap(r-1,r);
        if( r_old==r ) {
            // Successfully decremented count.
            return;
        } 
        r = r_old;
    } 
    __TBB_ASSERT( r==1, NULL );
    // Reference count might go to zero
    {
        task_scheduler_observer_mutex_scoped_lock lock(the_task_scheduler_observer_mutex.begin()[0],/*is_writer=*/true);
        r = --gc_ref_count;
        if( !r ) {
            remove_from_list();
        } 
    }
    if( !r ) {
        __TBB_ASSERT( gc_ref_count == -666, NULL );
#if TBB_DO_ASSERT
        --observer_proxy_count;
#endif /* TBB_DO_ASSERT */
        delete this;
    }
}

observer_proxy* observer_proxy::process_list( observer_proxy* local_last, bool is_worker, bool is_entry ) {
    // Pointer p marches though the list.
    // If is_entry, start with our previous list position, otherwise start at beginning of list.
    observer_proxy* p = is_entry ? local_last : NULL;
    for(;;) { 
        task_scheduler_observer* tso=NULL;
        // Hold lock on list only long enough to advance to next proxy in list.
        { 
            task_scheduler_observer_mutex_scoped_lock lock(the_task_scheduler_observer_mutex.begin()[0],/*is_writer=*/false);
            do {
                if( local_last && local_last->observer ) {
                    // 2 = 1 for observer and 1 for local_last
                    __TBB_ASSERT( local_last->gc_ref_count>=2, NULL );  
                    // Can decrement count quickly, because it cannot become zero here.
                    --local_last->gc_ref_count;
                    local_last = NULL;
                } else {
                    // Use slow form of decrementing the reference count, after lock is released.
                }  
                if( p ) {
                    // We were already processing the list.
                    if( observer_proxy* q = p->next ) {
                        // Step to next item in list.
                        p=q;
                    } else {
                        // At end of list.
                        if( is_entry ) {  
                            // Remember current position in the list, so we can start at on the next call.
                            ++p->gc_ref_count;
                        } else {
                            // Finishin running off the end of the list
                            p=NULL;
                        }
                        goto done;
                    }
                } else {
                    // Starting pass through the list
                    p = global_first_observer_proxy;
                    if( !p ) 
                        goto done;
                } 
                tso = p->observer;
            } while( !tso );
            ++p->gc_ref_count;
            ++tso->my_busy_count;
        }
        __TBB_ASSERT( !local_last || p!=local_last, NULL );
        if( local_last )
            local_last->remove_ref_slow();
        // Do not hold any locks on the list while calling user's code.
        try {    
            if( is_entry )
                tso->on_scheduler_entry( is_worker );
            else
                tso->on_scheduler_exit( is_worker );
        } catch(...) {
            // Suppress exception, because user routines are supposed to be observing, not changing
            // behavior of a master or worker thread.
#if __TBB_DO_ASSERT
            fprintf(stderr,"warning: %s threw exception\n",is_entry?"on_scheduler_entry":"on_scheduler_exit"); 
#endif /* __TBB_DO_ASSERT */        
        }
        intptr bc = --tso->my_busy_count;
        __TBB_ASSERT_EX( bc>=0, "my_busy_count underflowed" );
        local_last = p;
    }
done:
    // Return new value to be used as local_last next time.
    if( local_last )
        local_last->remove_ref_slow();
    __TBB_ASSERT( !p || is_entry, NULL );
    return p;
}

void task_scheduler_observer_v3::observe( bool state ) {
    if( state ) {
        if( !my_proxy ) {
            if( !__TBB_load_with_acquire(OneTimeInitializationsDone) ) {
                DoOneTimeInitializations();
            }
            my_busy_count = 0;
            my_proxy = new observer_proxy(*this);
            if( GenericScheduler* s = GetThreadSpecific() ) {
                // Notify newly created observer of its own thread.
                // Any other pending observers are notified too.
                s->notify_entry_observers();
            }
        } 
    } else {
        if( observer_proxy* proxy = my_proxy ) {
            my_proxy = NULL;
            __TBB_ASSERT( proxy->gc_ref_count>=1, "reference for observer missing" );
            {
                task_scheduler_observer_mutex_scoped_lock lock(the_task_scheduler_observer_mutex.begin()[0],/*is_writer=*/true);
                proxy->observer = NULL;
            }
            proxy->remove_ref_slow();
            while( my_busy_count ) {
                __TBB_Yield();
            }
        }
    }
}

} // namespace internal */
#endif /* __TBB_SCHEDULER_OBSERVER */

} // namespace tbb


