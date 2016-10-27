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

#include "harness.h"
#include <cstdio>

#if USE_PTHREAD

#include <cstdlib>
#include "tbb/spin_mutex.h"
#include "tbb/queuing_mutex.h"
#include "tbb/queuing_rw_mutex.h"
#include "tbb/spin_rw_mutex.h"
#include "tbb/tick_count.h"
#include "tbb/atomic.h"


// This test deliberately avoids a "using tbb" statement,
// so that the error of putting types in the wrong namespace will be caught.

template<typename M>
struct Counter {
    typedef M mutex_type;
    M mutex;
    volatile long value;
};

template<typename C>
void AddOne (C& counter, size_t mode)
/** Increments counter once for each iteration in the iteration space. */
{
    if( mode&1 ) {
        // Try implicit acquire and explicit release
        typename C::mutex_type::scoped_lock lock(counter.mutex);
        counter.value = counter.value+1;
        lock.release();
    } else {
        // Try explicit acquire and implicit release
        typename C::mutex_type::scoped_lock lock;
        lock.acquire(counter.mutex);
        counter.value = counter.value+1;
    }
}

template<typename M, size_t N>
struct Invariant {
    typedef M mutex_type;
    M mutex;
    const char* mutex_name;
    volatile long value[N];
    volatile long single_value;
    Invariant( const char* mutex_name_ ) :
        mutex_name(mutex_name_)
    {
        single_value = 0;
        for( size_t k=0; k<N; ++k )
            value[k] = 0;
    }
    void update() {
        for( size_t k=0; k<N; ++k )
            ++value[k];
    }
    bool value_is( long expected_value ) const {
        long tmp;
        for( size_t k=0; k<N; ++k )
//            if( value[k]!=expected_value )
//                return false;
            if( (tmp=value[k])!=expected_value ) {
                printf("ATTN! %ld!=%ld\n", tmp, expected_value);
                return false;
            }
        return true;
    }
    bool is_okay() {
        return value_is( value[0] );
    }
};

template<typename I>
void TwiddleInvariant( I& invariant, size_t mode )
{
    //! Every 8th access is a write access
    bool write = (mode%8)==7;
    bool okay = true;
    bool lock_kept = true;
    if( (mode/8)&1 ) {
        // Try implicit acquire and explicit release
        typename I::mutex_type::scoped_lock lock(invariant.mutex,write);
        if( write ) {
            long my_value = invariant.value[0];
            invariant.update();
            if( mode%16==7 ) {
                lock_kept = lock.downgrade_to_reader();
                if( !lock_kept )
                    my_value = invariant.value[0] - 1;
                okay = invariant.value_is(my_value+1);
            }
        } else {
            okay = invariant.is_okay();
            if( mode%8==3 ) {
                long my_value = invariant.value[0];
                lock_kept = lock.upgrade_to_writer();
                if( !lock_kept )
                    my_value = invariant.value[0];
                invariant.update();
                okay = invariant.value_is(my_value+1);
            }
        }
        lock.release();
    } else {
        // Try explicit acquire and implicit release
        typename I::mutex_type::scoped_lock lock;
        lock.acquire(invariant.mutex,write);
        if( write ) {
            long my_value = invariant.value[0];
            invariant.update();
            if( mode%16==7 ) {
                lock_kept = lock.downgrade_to_reader();
                if( !lock_kept )
                    my_value = invariant.value[0] - 1;
                okay = invariant.value_is(my_value+1);
            }
        } else {
            okay = invariant.is_okay();
            if( mode%8==3 ) {
                long my_value = invariant.value[0];
                lock_kept = lock.upgrade_to_writer();
                if( !lock_kept )
                    my_value = invariant.value[0];
                invariant.update();
                okay = invariant.value_is(my_value+1);
            }
        }
    }
    if( !okay ) {
        std::printf( "ERROR for %s at %ld: %s %s %s %s\n",invariant.mutex_name, long(mode),
                     write?"write,":"read,", write?(mode%16==7?"downgrade,":""):(mode%8==3?"upgrade,":""),
                     lock_kept?"lock kept,":"lock not kept,", (mode/8)&1?"imp/exp":"exp/imp" );
    }
}

template<typename M>
class Work
{
public:
    static tbb::atomic<size_t> order;
    static const long mutex_test_size;
    static const long readwrite_test_size;
    static const long chunk;

    static void* mutex_work(void* p)
    {
        Counter<M>* pCounter = reinterpret_cast<Counter<M>*>(p);
        long step, i;
        while( (step=order.fetch_and_add<tbb::acquire>(chunk))<mutex_test_size ){
            for( i=0; i<chunk && step<mutex_test_size; ++i, ++step ) {
                AddOne(*pCounter, step);
            }
        }
        return NULL;
    }

    static void* readwrite_work(void* p)
    {
        Invariant<M,8>* pInv = reinterpret_cast<Invariant<M,8>*>(p);
        long step, i;
        while( (step=order.fetch_and_add<tbb::acquire>(chunk))<readwrite_test_size ){
            for( i=0; i<chunk && step<readwrite_test_size; ++i, ++step ) {
                TwiddleInvariant(*pInv, step);
            }
        }
        return NULL;
    }
};

template<typename M> tbb::atomic<size_t> Work<M>::order;
template<typename M> const long Work<M>::mutex_test_size = 100000;
template<typename M> const long Work<M>::readwrite_test_size = 1000000;
template<typename M> const long Work<M>::chunk = 100;

typedef void* thread_func( void* );

//! Generic test of a TBB mutex type M.
/** Does not test features specific to reader-writer locks. */
template<typename M>
void Test( const char * name ) {
    if( Verbose )
        printf("testing %s\n",name);
    Counter<M> counter;
    counter.value = 0;
    Work<M>::order = 0;
    thread_func* mutex = reinterpret_cast<thread_func*>(&(Work<M>::mutex_work));
    pthread_t* thr = new pthread_t[NThread];

    tbb::tick_count t0 = tbb::tick_count::now();
    for (int i=0; i<NThread; ++i)
        pthread_create(thr+i, NULL, mutex, reinterpret_cast<void*>(&counter));
    for (int i=0; i<NThread; ++i)
        pthread_join(*(thr+i), NULL);
    tbb::tick_count t1 = tbb::tick_count::now();
    if( Verbose )
        printf("%s time = %g usec\n",name, (t1-t0).seconds() );
    if( counter.value!=Work<M>::mutex_test_size )
        std::printf("ERROR for %s: counter.value=%ld\n",name,counter.value);
    delete[] thr;
}


/** This test is generic so that we can test any other kinds of ReaderWriter locks we write later. */
template<typename M>
void TestReaderWriterLock( const char * mutex_name ) {
    if( Verbose )
        printf("testing %s\n",mutex_name);
    Invariant<M,8> invariant(mutex_name);
    Work<M>::order = 0;
    thread_func* readwrite = reinterpret_cast<thread_func*>(Work<M>::readwrite_work);
    pthread_t* thr = new pthread_t[NThread];
    tbb::tick_count t0 = tbb::tick_count::now();
    for (int i=0; i<NThread; ++i)
        pthread_create(thr+i, NULL, readwrite, reinterpret_cast<void*>(&invariant));
    for (int i=0; i<NThread; ++i)
        pthread_join(*(thr+i), NULL);
    tbb::tick_count t1 = tbb::tick_count::now();
    // There is either a writer or a reader upgraded to a writer for each 4th iteration
    long expected_value = Work<M>::readwrite_test_size/4;
    if( !invariant.value_is(expected_value) )
        std::printf("ERROR for %s: final invariant value is wrong\n",mutex_name);
    if( Verbose )
        printf("%s readers & writers time = %g usec\n",mutex_name,(t1-t0).seconds());
    delete[] thr;
}
#endif /* USE_PTHREAD */

int main( int argc, char * argv[] ) {
    ParseCommandLine( argc, argv );
#if USE_PTHREAD
    if( Verbose )
        printf( "testing with %d threads\n", NThread );
    Test<tbb::spin_mutex>( "Spin Mutex" );
    Test<tbb::queuing_mutex>( "Queuing Mutex" );
    Test<tbb::queuing_rw_mutex>( "Queuing RW Mutex" );
    Test<tbb::spin_rw_mutex>( "Spin RW Mutex" );
    TestReaderWriterLock<tbb::queuing_rw_mutex>( "Queuing RW Mutex" );
    TestReaderWriterLock<tbb::spin_rw_mutex>( "Spin RW Mutex" );
    std::printf("done\n");
#else
    if( Verbose )
        printf("this test need pthreads to work; define USE_PTHREAD before compilation.\n");
    printf("skip\n");
#endif /* USE_PTHREAD */
    return 0;
}
