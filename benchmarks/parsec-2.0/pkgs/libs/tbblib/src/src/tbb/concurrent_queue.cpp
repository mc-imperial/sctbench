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

#include <cstring>
#include <cstdio>

#include "tbb/tbb_machine.h"
#include "tbb/cache_aligned_allocator.h"
#include "tbb/spin_mutex.h"
#include "tbb/atomic.h"
#include "tbb/concurrent_queue.h"
#include "tbb/tbb_exception.h"
#include "tbb_misc.h"
#include "itt_notify.h"
#if __SUNPRO_CC
#include <memory.h>
#endif

using namespace std;

// enable sleep support
#define __TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE 1

#if defined(_MSC_VER) && defined(_Wp64)
    // Workaround for overzealous compiler warnings in /Wp64 mode
    #pragma warning (disable: 4267)
#endif /* _MSC_VER && _Wp64 */

#define RECORD_EVENTS 0


#if __TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE
#if !_WIN32&&!_WIN64
#include <pthread.h>
#endif
#endif

namespace tbb {

namespace internal {

class concurrent_queue_rep;

typedef size_t ticket;

//! A queue using simple locking.
/** For efficient, this class has no constructor.  
    The caller is expected to zero-initialize it. */
struct micro_queue {
    typedef concurrent_queue_base::page page;

    friend class micro_queue_pop_finalizer;

    atomic<page*> head_page;
    atomic<ticket> head_counter;

    atomic<page*> tail_page;
    atomic<ticket> tail_counter;

    spin_mutex page_mutex;
    
    class push_finalizer {
        ticket my_ticket;
        micro_queue& my_queue;
    public:
        push_finalizer( micro_queue& queue, ticket k ) :
            my_ticket(k), my_queue(queue)
        {}
        ~push_finalizer() {
            my_queue.tail_counter = my_ticket;
        }
    };

    void push( const void* item, ticket k, concurrent_queue_base& base );

    bool pop( void* dst, ticket k, concurrent_queue_base& base );
};

// we need to yank it out of micro_queue because of concurrent_queue_base::deallocate_page being virtual.
class micro_queue_pop_finalizer {
    typedef concurrent_queue_base::page page;
    ticket my_ticket;
    micro_queue& my_queue;
    page* my_page; 
    concurrent_queue_base &base;
public:
    micro_queue_pop_finalizer( micro_queue& queue, concurrent_queue_base& b, ticket k, page* p ) :
        my_ticket(k), my_queue(queue), my_page(p), base(b)
    {}
    ~micro_queue_pop_finalizer() {
        page* p = my_page;
        if( p ) {
            spin_mutex::scoped_lock lock( my_queue.page_mutex );
            page* q = p->next;
            my_queue.head_page = q;
            if( !q ) {
                my_queue.tail_page = NULL;
            }
        }
        my_queue.head_counter = my_ticket;
        if( p )
           base.deallocate_page( p );
    }
};

//! Internal representation of a ConcurrentQueue.
/** For efficient, this class has no constructor.  
    The caller is expected to zero-initialize it. */
class concurrent_queue_rep {
public:
#if __TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE
# if _WIN32||_WIN64
    typedef HANDLE waitvar_t;
    typedef CRITICAL_SECTION mutexvar_t;
# else 
    typedef pthread_cond_t  waitvar_t;
    typedef pthread_mutex_t mutexvar_t;
# endif
#endif /* __TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE */
private:
    friend struct micro_queue;

    //! Approximately n_queue/golden ratio
    static const size_t phi = 3;

public:
    //! Must be power of 2
    static const size_t n_queue = 8; 

    //! Map ticket to an array index
    static size_t index( ticket k ) {
        return k*phi%n_queue;
    }

#if __TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE
    atomic<ticket> head_counter;
    waitvar_t  var_wait_for_items;
    mutexvar_t mtx_items_avail;
    uint16_t n_waiting_consumers;
#if _WIN32||_WIN64
    uint16_t consumer_wait_generation;
    uint16_t n_consumers_to_wakeup;
    char pad1[NFS_MaxLineSize-((sizeof(atomic<ticket>)+sizeof(waitvar_t)+sizeof(mutexvar_t)+sizeof(uint16_t)+sizeof(uint16_t)+sizeof(uint16_t))&(NFS_MaxLineSize-1))];
#else
    char pad1[NFS_MaxLineSize-((sizeof(atomic<ticket>)+sizeof(waitvar_t)+sizeof(mutexvar_t)+sizeof(uint16_t))&(NFS_MaxLineSize-1))];
#endif

    atomic<ticket> tail_counter;
    waitvar_t  var_wait_for_slots;
    mutexvar_t mtx_slots_avail;
    uint16_t n_waiting_producers;
#if _WIN32||_WIN64
    uint16_t producer_wait_generation;
    uint16_t n_producers_to_wakeup;
    char pad2[NFS_MaxLineSize-((sizeof(atomic<ticket>)+sizeof(waitvar_t)+sizeof(mutexvar_t)+sizeof(uint16_t)+sizeof(uint16_t)+sizeof(uint16_t))&(NFS_MaxLineSize-1))];
#else
    char pad2[NFS_MaxLineSize-((sizeof(atomic<ticket>)+sizeof(waitvar_t)+sizeof(mutexvar_t)+sizeof(uint16_t))&(NFS_MaxLineSize-1))];
#endif
#else /* !__TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE */
    atomic<ticket> head_counter;
    char pad1[NFS_MaxLineSize-sizeof(atomic<ticket)>];
    atomic<ticket> tail_counter;
    char pad2[NFS_MaxLineSize-sizeof(atomic<ticket>)];
#endif /* __TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE */
    micro_queue array[n_queue];    

    micro_queue& choose( ticket k ) {
        // The formula here approximates LRU in a cache-oblivious way.
        return array[index(k)];
    }

    //! Value for effective_capacity that denotes unbounded queue.
    static const ptrdiff_t infinite_capacity = ptrdiff_t(~size_t(0)/2);
};

#if _MSC_VER && !defined(__INTEL_COMPILER)
#pragma warning( push )
// unary minus operator applied to unsigned type, result still unsigned
#pragma warning( disable: 4146 )
#endif /* _MSC_VER && !defined(__INTEL_COMPILER) */


static void* invalid_page;

//------------------------------------------------------------------------
// micro_queue
//------------------------------------------------------------------------
void micro_queue::push( const void* item, ticket k, concurrent_queue_base& base ) {
    static concurrent_queue_base::page dummy = {static_cast<page*>((void*)1), 0};
    k &= -concurrent_queue_rep::n_queue;
    page* p = NULL;
    size_t index = (k/concurrent_queue_rep::n_queue & base.items_per_page-1);
    if( !index ) {
        try {
            p = base.allocate_page();
        } catch (...) {
            // mark it so that no more pushes are allowed.
            invalid_page = &dummy;
            spin_mutex::scoped_lock lock( page_mutex );
            tail_counter = k+concurrent_queue_rep::n_queue+1;
            if( page* q = tail_page )
                q->next = static_cast<page*>(invalid_page);
            else
                head_page = static_cast<page*>(invalid_page); 
            tail_page = static_cast<page*>(invalid_page);
            throw;
        }
        p->mask = 0;
        p->next = NULL;
    }
    {
        push_finalizer finalizer( *this, k+concurrent_queue_rep::n_queue ); 
        if( tail_counter!=k ) {
            ExponentialBackoff backoff;
            do {
                backoff.pause();
                // no memory. throws an exception
                if( tail_counter&0x1 ) throw bad_last_alloc();
            } while( tail_counter!=k ) ;
        }
        
        if( p ) {
            spin_mutex::scoped_lock lock( page_mutex );
            if( page* q = tail_page )
                q->next = p;
            else
                head_page = p; 
            tail_page = p;
        } else {
            p = tail_page;
        }
        ITT_NOTIFY( sync_acquired, p );
        base.copy_item( *p, index, item );
        ITT_NOTIFY( sync_releasing, p );
        // If no exception was thrown, mark item as present.
        p->mask |= uintptr(1)<<index;
    }
}

bool micro_queue::pop( void* dst, ticket k, concurrent_queue_base& base ) {
    k &= -concurrent_queue_rep::n_queue;
    SpinwaitUntilEq( head_counter, k );
    SpinwaitWhileEq( tail_counter, k );
    page& p = *head_page;
    __TBB_ASSERT( &p, NULL );
    size_t index = (k/concurrent_queue_rep::n_queue & base.items_per_page-1);
    bool success = false; 
    {
        micro_queue_pop_finalizer finalizer( *this, base, k+concurrent_queue_rep::n_queue, index==base.items_per_page-1 ? &p : NULL ); 
        if( p.mask & uintptr(1)<<index ) {
            success = true;
#if DO_ITT_NOTIFY
            if( ((intptr_t)dst&1) ) {
                dst = (void*) ((intptr_t)dst&~1);
                ITT_NOTIFY( sync_acquired, dst );
            }
#endif
            ITT_NOTIFY( sync_acquired, head_page );
            base.assign_and_destroy_item( dst, p, index );
            ITT_NOTIFY( sync_releasing, head_page );
        }
    }
    return success;
}

#if _MSC_VER && !defined(__INTEL_COMPILER)
#pragma warning( pop )
#endif /* _MSC_VER && !defined(__INTEL_COMPILER) */

//------------------------------------------------------------------------
// concurrent_queue_base
//------------------------------------------------------------------------
concurrent_queue_base_v3::concurrent_queue_base_v3( size_t item_size ) {
    items_per_page = item_size<=8 ? 32 :
                     item_size<=16 ? 16 : 
                     item_size<=32 ? 8 :
                     item_size<=64 ? 4 :
                     item_size<=128 ? 2 :
                     1;
    my_capacity = size_t(-1)/(item_size>1 ? item_size : 2); 
    my_rep = cache_aligned_allocator<concurrent_queue_rep>().allocate(1);
    __TBB_ASSERT( (size_t)my_rep % NFS_GetLineSize()==0, "alignment error" );
    __TBB_ASSERT( (size_t)&my_rep->head_counter % NFS_GetLineSize()==0, "alignment error" );
    __TBB_ASSERT( (size_t)&my_rep->tail_counter % NFS_GetLineSize()==0, "alignment error" );
    __TBB_ASSERT( (size_t)&my_rep->array % NFS_GetLineSize()==0, "alignment error" );
    memset(my_rep,0,sizeof(concurrent_queue_rep));
    this->item_size = item_size;
#if __TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE
#if _WIN32||_WIN64
    my_rep->var_wait_for_items = CreateEvent( NULL, TRUE/*manual reset*/, FALSE/*not signalled initially*/, NULL);
    my_rep->var_wait_for_slots = CreateEvent( NULL, TRUE/*manual reset*/, FALSE/*not signalled initially*/, NULL);
    InitializeCriticalSection( &my_rep->mtx_items_avail );
    InitializeCriticalSection( &my_rep->mtx_slots_avail );
#else 
    // initialize pthread_mutex_t, and pthread_cond_t
    pthread_mutexattr_t m_attr;
    pthread_mutexattr_init( &m_attr );
#ifdef PTHREAD_PRIO_INHERIT 
    pthread_mutexattr_setprotocol( &m_attr, PTHREAD_PRIO_INHERIT );
#endif
    pthread_mutex_init( &my_rep->mtx_items_avail, &m_attr );
    pthread_mutex_init( &my_rep->mtx_slots_avail, &m_attr );
    pthread_mutexattr_destroy( &m_attr );

    pthread_condattr_t c_attr;
    pthread_condattr_init( &c_attr );
    pthread_cond_init( &my_rep->var_wait_for_items, &c_attr );
    pthread_cond_init( &my_rep->var_wait_for_slots, &c_attr );
    pthread_condattr_destroy( &c_attr );
#endif
#endif /* __TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE */
}

concurrent_queue_base_v3::~concurrent_queue_base_v3() {
    size_t nq = my_rep->n_queue;
    for( size_t i=0; i<nq; i++ )
        __TBB_ASSERT( my_rep->array[i].tail_page==NULL, "pages were not freed properly" );
#if __TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE
# if _WIN32||_WIN64
    CloseHandle( my_rep->var_wait_for_items );
    CloseHandle( my_rep->var_wait_for_slots );
    DeleteCriticalSection( &my_rep->mtx_items_avail );
    DeleteCriticalSection( &my_rep->mtx_slots_avail );
# else
    pthread_mutex_destroy( &my_rep->mtx_items_avail );
    pthread_mutex_destroy( &my_rep->mtx_slots_avail );
    pthread_cond_destroy( &my_rep->var_wait_for_items );
    pthread_cond_destroy( &my_rep->var_wait_for_slots );
# endif
#endif /* __TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE */
    cache_aligned_allocator<concurrent_queue_rep>().deallocate(my_rep,1);
}

void concurrent_queue_base_v3::internal_push( const void* src ) {
    concurrent_queue_rep& r = *my_rep;
#if !__TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE
    concurrent_queue_rep::ticket k  = r.tail_counter++;
    ptrdiff_t e = my_capacity;
    if( e<concurrent_queue_rep::infinite_capacity ) {
        AtomicBackoff backoff;
        for(;;) {
            if( (ptrdiff_t)(k-r.head_counter)<e ) break;
            backoff.pause();
            e = const_cast<volatile ptrdiff_t&>(my_capacity);
        }
    } 
    r.choose(k).push(src,k,*this);
#elif _WIN32||_WIN64
    ticket k = r.tail_counter++;
    ptrdiff_t e = my_capacity;
    AtomicBackoff backoff;
#if DO_ITT_NOTIFY
    bool sync_prepare_done = false;
#endif

    while( (ptrdiff_t)(k-r.head_counter)>=e ) {
#if DO_ITT_NOTIFY
        if( !sync_prepare_done ) {
            ITT_NOTIFY( sync_prepare, &sync_prepare_done );
            sync_prepare_done = true;
        }
#endif
        if( !backoff.bounded_pause() ) {
            EnterCriticalSection( &r.mtx_slots_avail );
            r.n_waiting_producers++;
            while( (ptrdiff_t)(k-r.head_counter)>=const_cast<volatile ptrdiff_t&>(my_capacity) ) {
                int my_generation = r.producer_wait_generation;
                for( ;; ) {
                    LeaveCriticalSection( &r.mtx_slots_avail );
                    WaitForSingleObject( r.var_wait_for_slots, INFINITE );
                    EnterCriticalSection( &r.mtx_slots_avail );
                    if( r.n_producers_to_wakeup > 0 && r.producer_wait_generation != my_generation )
                        break;
                }
                if( --r.n_producers_to_wakeup == 0 )
                    ResetEvent( r.var_wait_for_slots );
            }
            LeaveCriticalSection( &r.mtx_slots_avail );
            break;
        }
        e = const_cast<volatile ptrdiff_t&>(my_capacity);
    }
#if DO_ITT_NOTIFY
    if( sync_prepare_done )
        ITT_NOTIFY( sync_acquired, &sync_prepare_done );
#endif

    r.choose( k ).push( src, k, *this );

    if( r.n_waiting_consumers>0 ) {
        EnterCriticalSection( &r.mtx_items_avail );
        if( r.n_waiting_consumers>0 ) {
            r.consumer_wait_generation++;
            r.n_consumers_to_wakeup = r.n_waiting_consumers;
            SetEvent( r.var_wait_for_items );
        }
        LeaveCriticalSection( &r.mtx_items_avail );
    }
#else 
    ticket k = r.tail_counter++;
    ptrdiff_t e = my_capacity;
    AtomicBackoff backoff;
#if DO_ITT_NOTIFY
    bool sync_prepare_done = false;
#endif
    while( (ptrdiff_t)(k-r.head_counter)>=e ) {
#if DO_ITT_NOTIFY
        if( !sync_prepare_done ) {
            ITT_NOTIFY( sync_prepare, &sync_prepare_done );
            sync_prepare_done = true;
        }
#endif
        if( !backoff.bounded_pause() ) {
            // queue is full.  go to sleep. let them go to sleep in order.
            pthread_mutex_lock( &r.mtx_slots_avail );
            r.n_waiting_producers++;
            while( (ptrdiff_t)(k-r.head_counter)>=const_cast<volatile ptrdiff_t&>(my_capacity) ) {
                pthread_cond_wait( &r.var_wait_for_slots, &r.mtx_slots_avail );
            }
            --r.n_waiting_producers;
            pthread_mutex_unlock( &r.mtx_slots_avail );
            break;
        }
        e = const_cast<volatile ptrdiff_t&>(my_capacity);
    }
#if DO_ITT_NOTIFY
    if( sync_prepare_done )
        ITT_NOTIFY( sync_acquired, &sync_prepare_done );
#endif
    r.choose( k ).push( src, k, *this );

    if( r.n_waiting_consumers>0 ) {
        pthread_mutex_lock( &r.mtx_items_avail );
        // pthread_cond_broadcast() wakes up all consumers. 
        if( r.n_waiting_consumers>0 )
            pthread_cond_broadcast( &r.var_wait_for_items );
        pthread_mutex_unlock( &r.mtx_items_avail );
    }
#endif /* !__TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE */
}

void concurrent_queue_base_v3::internal_pop( void* dst ) {
    concurrent_queue_rep& r = *my_rep;
#if !__TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE
    concurrent_queue_rep::ticket k;
    do {
        k = r.head_counter++;
    } while( !r.choose(k).pop(dst,k,*this) );
#elif _WIN32||_WIN64
    ticket k;
    AtomicBackoff backoff;
#if DO_ITT_NOTIFY
    bool sync_prepare_done = false;
#endif
    do {
        k=r.head_counter++;
        while( r.tail_counter<=k ) {
#if DO_ITT_NOTIFY
            if( !sync_prepare_done ) {
                ITT_NOTIFY( sync_prepare, dst );
                dst = (void*) ((intptr_t)dst | 1);
                sync_prepare_done = true;
            }
#endif
            // Queue is empty; pause and re-try a few times
            if( !backoff.bounded_pause() ) {
                // it is really empty.. go to sleep
                EnterCriticalSection( &r.mtx_items_avail );
                r.n_waiting_consumers++;
                while( r.tail_counter<=k ) {
                    int my_generation = r.consumer_wait_generation;
                    for( ;; ) {
                        LeaveCriticalSection( &r.mtx_items_avail );
                        WaitForSingleObject( r.var_wait_for_items, INFINITE );
                        EnterCriticalSection( &r.mtx_items_avail );
                        if( r.n_consumers_to_wakeup > 0 && r.consumer_wait_generation != my_generation )
                            break;
                    }
                    if( --r.n_consumers_to_wakeup == 0 )
                        ResetEvent( r.var_wait_for_items );
                }
                LeaveCriticalSection( &r.mtx_items_avail );
                backoff.reset();
                break; // break from inner while
            }
        } // break to here
    } while( !r.choose(k).pop(dst,k,*this) );

    // wake up a producer..
    if( r.n_waiting_producers>0 ) {
        EnterCriticalSection( &r.mtx_slots_avail );
        if( r.n_waiting_producers>0 ) {
            r.producer_wait_generation++;
            r.n_producers_to_wakeup = r.n_waiting_producers;
            SetEvent( r.var_wait_for_slots );
        }
        LeaveCriticalSection( &r.mtx_slots_avail );
    }
#else 
    ticket k;
    AtomicBackoff backoff;
#if DO_ITT_NOTIFY
    bool sync_prepare_done = false;
#endif
    do {
        k = r.head_counter++;
        while( r.tail_counter<=k ) {
#if DO_ITT_NOTIFY
            if( !sync_prepare_done ) {
                ITT_NOTIFY( sync_prepare, dst );
                dst = (void*) ((intptr_t)dst | 1);
                sync_prepare_done = true;
            }
#endif
            // Queue is empty; pause and re-try a few times
            if( !backoff.bounded_pause() ) {
                // it is really empty.. go to sleep
                pthread_mutex_lock( &r.mtx_items_avail );
                r.n_waiting_consumers++;
                while( r.tail_counter<=k )
                    pthread_cond_wait( &r.var_wait_for_items, &r.mtx_items_avail );
                --r.n_waiting_consumers;
                pthread_mutex_unlock( &r.mtx_items_avail );
                backoff.reset();
                break;
            }
        }
    } while( !r.choose(k).pop(dst,k,*this) );

    if( r.n_waiting_producers>0 ) {
        pthread_mutex_lock( &r.mtx_slots_avail );
        if( r.n_waiting_producers>0 )
            pthread_cond_broadcast( &r.var_wait_for_slots );
        pthread_mutex_unlock( &r.mtx_slots_avail );
    }
#endif /* !__TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE */
}

bool concurrent_queue_base_v3::internal_pop_if_present( void* dst ) {
    concurrent_queue_rep& r = *my_rep;
    ticket k;
    do {
        k = r.head_counter;
        for(;;) {
            if( r.tail_counter<=k ) {
                // Queue is empty 
                return false;
            }
            // Queue had item with ticket k when we looked.  Attempt to get that item.
            ticket tk=k;
            k = r.head_counter.compare_and_swap( tk+1, tk );
            if( k==tk )
                break;
            // Another thread snatched the item, retry.
        }
    } while( !r.choose( k ).pop( dst, k, *this ) );

#if __TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE
#if _WIN32||_WIN64
    // wake up a producer..
    if( r.n_waiting_producers>0 ) {
        EnterCriticalSection( &r.mtx_slots_avail );
        if( r.n_waiting_producers>0 ) {
            r.producer_wait_generation++;
            r.n_producers_to_wakeup = r.n_waiting_producers;
            SetEvent( r.var_wait_for_slots );
        }
        LeaveCriticalSection( &r.mtx_slots_avail );
    }
#else /* including MacOS */
    if( r.n_waiting_producers>0 ) {
        pthread_mutex_lock( &r.mtx_slots_avail );
        if( r.n_waiting_producers>0 )
            pthread_cond_broadcast( &r.var_wait_for_slots );
        pthread_mutex_unlock( &r.mtx_slots_avail );
    }
#endif
#endif /* __TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE */

    return true;
}

bool concurrent_queue_base_v3::internal_push_if_not_full( const void* src ) {
    concurrent_queue_rep& r = *my_rep;
    ticket k = r.tail_counter;
    for(;;) {
        if( (ptrdiff_t)(k-r.head_counter)>=my_capacity ) {
            // Queue is full
            return false;
        }
        // Queue had empty slot with ticket k when we looked.  Attempt to claim that slot.
        ticket tk=k;
        k = r.tail_counter.compare_and_swap( tk+1, tk );
        if( k==tk ) 
            break;
        // Another thread claimed the slot, so retry. 
    }
    r.choose(k).push(src,k,*this);

#if __TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE
#if _WIN32||_WIN64
    if( r.n_waiting_consumers>0 ) {
        EnterCriticalSection( &r.mtx_items_avail );
        if( r.n_waiting_consumers>0 ) {
            r.consumer_wait_generation++;
            r.n_consumers_to_wakeup = r.n_waiting_consumers;
            SetEvent( r.var_wait_for_items );
        }
        LeaveCriticalSection( &r.mtx_items_avail );
    }
#else /* including MacOS */
    if( r.n_waiting_consumers>0 ) {
        pthread_mutex_lock( &r.mtx_items_avail );
        if( r.n_waiting_consumers>0 )
            pthread_cond_broadcast( &r.var_wait_for_items );
        pthread_mutex_unlock( &r.mtx_items_avail );
    }
#endif
#endif /* __TBB_NO_BUSY_WAIT_IN_CONCURRENT_QUEUE */
    return true;
}

ptrdiff_t concurrent_queue_base_v3::internal_size() const {
    __TBB_ASSERT( sizeof(ptrdiff_t)<=sizeof(size_t), NULL );
    return ptrdiff_t(my_rep->tail_counter-my_rep->head_counter);
}

void concurrent_queue_base_v3::internal_set_capacity( ptrdiff_t capacity, size_t /*item_size*/ ) {
    my_capacity = capacity<0 ? concurrent_queue_rep::infinite_capacity : capacity;
}

void concurrent_queue_base_v3::internal_finish_clear() {
    size_t nq = my_rep->n_queue;
    for( size_t i=0; i<nq; i++ ) {
        page* tp = my_rep->array[i].tail_page;
        __TBB_ASSERT( my_rep->array[i].head_page==tp, "at most one page should remain" );
        if( tp!=NULL) {
            if( tp!=invalid_page ) deallocate_page( tp );
            my_rep->array[i].tail_page = NULL;
        }
    }
}

void concurrent_queue_base_v3::internal_throw_exception() const {
    throw bad_alloc();
}

//------------------------------------------------------------------------
// concurrent_queue_iterator_rep
//------------------------------------------------------------------------
class  concurrent_queue_iterator_rep {
public:
    ticket head_counter;   
    const concurrent_queue_base& my_queue;
    concurrent_queue_base::page* array[concurrent_queue_rep::n_queue];
    concurrent_queue_iterator_rep( const concurrent_queue_base& queue ) : 
        head_counter(queue.my_rep->head_counter),
        my_queue(queue)
    {
        const concurrent_queue_rep& rep = *queue.my_rep;
        for( size_t k=0; k<concurrent_queue_rep::n_queue; ++k )
            array[k] = rep.array[k].head_page;
    }
    //! Get pointer to kth element
    void* choose( size_t k ) {
        if( k==my_queue.my_rep->tail_counter )
            return NULL;
        else {
            concurrent_queue_base::page* p = array[concurrent_queue_rep::index(k)];
            __TBB_ASSERT(p,NULL);
            size_t i = k/concurrent_queue_rep::n_queue & my_queue.items_per_page-1;
            return static_cast<unsigned char*>(static_cast<void*>(p+1)) + my_queue.item_size*i;
        }
    }
};

//------------------------------------------------------------------------
// concurrent_queue_iterator_base
//------------------------------------------------------------------------
concurrent_queue_iterator_base_v3::concurrent_queue_iterator_base_v3( const concurrent_queue_base& queue ) {
    my_rep = cache_aligned_allocator<concurrent_queue_iterator_rep>().allocate(1);
    new( my_rep ) concurrent_queue_iterator_rep(queue);
    my_item = my_rep->choose(my_rep->head_counter);
}

void concurrent_queue_iterator_base_v3::assign( const concurrent_queue_iterator_base& other ) {
    if( my_rep!=other.my_rep ) {
        if( my_rep ) {
            cache_aligned_allocator<concurrent_queue_iterator_rep>().deallocate(my_rep, 1);
            my_rep = NULL;
        }
        if( other.my_rep ) {
            my_rep = cache_aligned_allocator<concurrent_queue_iterator_rep>().allocate(1);
            new( my_rep ) concurrent_queue_iterator_rep( *other.my_rep );
        }
    }
    my_item = other.my_item;
}

void concurrent_queue_iterator_base_v3::advance() {
    __TBB_ASSERT( my_item, "attempt to increment iterator past end of queue" );  
    size_t k = my_rep->head_counter;
    const concurrent_queue_base& queue = my_rep->my_queue;
    __TBB_ASSERT( my_item==my_rep->choose(k), NULL );
    size_t i = k/concurrent_queue_rep::n_queue & queue.items_per_page-1;
    if( i==queue.items_per_page-1 ) {
        concurrent_queue_base::page*& root = my_rep->array[concurrent_queue_rep::index(k)];
        root = root->next;
    }
    my_rep->head_counter = k+1;
    my_item = my_rep->choose(k+1);
}

concurrent_queue_iterator_base_v3::~concurrent_queue_iterator_base_v3() {
    //delete my_rep;
    cache_aligned_allocator<concurrent_queue_iterator_rep>().deallocate(my_rep, 1);
    my_rep = NULL;
}

} // namespace internal

} // namespace tbb
