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

// Program for basic correctness testing of assembly-language routines.
// This program deliberately #includes ../internal/task.cpp so that
// it can get intimate access to the scheduler.

#define TEST_ASSEMBLY_ROUTINES 1
#define __TBB_TASK_CPP_DIRECTLY_INCLUDED 1
// to avoid usage of #pragma comment
#define __TBB_NO_IMPLICIT_LINKAGE 1

#include "../tbb/task.cpp"
#include <new>
#include "harness.h"

namespace tbb {

namespace internal {

class TestTask: public task {
public:
    /*override*/ task* execute() {
        return NULL;
    }
    const char* name;
    TestTask( const char* name_ ) : name(name_) {}
};

void GenericScheduler::test_assembly_routines() {
    try_enter_arena();
    mark_pool_full();
    release_task_pool();
    ASSERT( assert_okay(), NULL );
    long steal_count = 0;
    long get_count = 0;
    const depth_type n = 5;
    const depth_type array_size_proxy = array_size; // using proxy to reduce compilation warnings
    ASSERT( array_size-array_size_proxy==0, NULL ); // check validity of the replacement
    ASSERT( array_size_proxy>=n, NULL );
    // Loop over insertion levels
    for( depth_type i=0; i<n; ++i ) 
        // Loop over values for "deepest"
        for( depth_type d=-1; d<n; ++d )     
            // Loop over values for "shallowest"
            for( depth_type s=0; s<n; ++s ) 
                // Loop over depth limits
                for( depth_type limit=0; limit<n; ++limit ) 
                    // Loop over whether to insert or not
                    for( int insert=0; insert<2; ++insert ) 
                        // Loop over whether to steal or get
                        for( int steal=0; steal<2; ++steal ) {
                            ASSERT( assert_okay(), NULL );
                            task** array = dummy_slot.task_pool->array;  
                            for( depth_type k=0; k<array_size_proxy; ++k )
                                array[k] = NULL;
                            dummy_slot.task_pool->prefix().steal_begin = s;
                            ASSERT( assert_okay(), NULL );

                            TestTask& w = *new( task::allocate_root() ) TestTask("w");
                            ASSERT( assert_okay(), NULL );
                            if( d>=0 ) {
                                w.prefix().depth = int(d);
                                w.prefix().next = NULL;
                                w.prefix().state = task::ready;
                                dummy_slot.task_pool->array[d] = &w;
                            }
                            deepest = d;
                            arena_slot->steal_end = 2*d;

                            ASSERT( assert_okay(), NULL );
                            TestTask& x = *new( task::allocate_root() ) TestTask("x");
                            TestTask& y = *new( task::allocate_root() ) TestTask("y");
                            TestTask& z = *new( task::allocate_root() ) TestTask("z");
                            ASSERT( assert_okay(), NULL );
                            x.prefix().next = &y;
                            y.prefix().next = &z;       
                            z.prefix().next = NULL;
                            ASSERT( x.prefix().next==&y, NULL );
                            for( task* p=&x; p; p=p->prefix().next ) 
                                p->prefix().depth = int(i);
                            ASSERT( assert_okay(), NULL );
                            z.prefix().next = (task*)(void*)-1;
                            if( insert ) {
                                spawn( x, z.prefix().next );
                                ASSERT( assert_okay(), NULL );
                                depth_type expected_deepest = d>=i ? d : i;
                                ASSERT( deepest==expected_deepest, NULL );
                                depth_type expected_shallowest = i<=s ? i : s;
                                ASSERT( dummy_slot.task_pool->prefix().steal_begin==expected_shallowest, NULL );
                                ASSERT( array[i]==&x, NULL );
                                ASSERT( x.prefix().next==&y, NULL );
                            } 
                            if( steal ) {
                                task* expected_task = NULL;
                                depth_type shallowest = dummy_slot.task_pool->prefix().steal_begin;
                                depth_type expected_shallowest = shallowest;
                                for( depth_type k=shallowest; k<array_size_proxy; ++k ) {
                                    if( k>=limit && array[k] ) {
                                        expected_task = array[k];
                                        if( shallowest>=limit )
                                            expected_shallowest = k;
                                        break;
                                    }
                                }
                                ASSERT( assert_okay(), NULL );
                                task* t = steal_task( *arena_slot, limit );
                                ASSERT( (arena_slot->steal_end&1)==0, "forgot to release lock?" );
                                ASSERT( assert_okay(), NULL );
                                ASSERT( t==expected_task, NULL );       
                                shallowest = dummy_slot.task_pool->prefix().steal_begin;
                                ASSERT( shallowest==expected_shallowest, NULL );        
                                ++steal_count;
                            } else {
                                task* expected_task = NULL;
                                for( depth_type k=array_size_proxy-1; k>=limit; --k ) {
                                    if( array[k] ) {
                                        expected_task = array[k];
                                        ASSERT( deepest==k, NULL ); 
                                        break;
                                    }
                                }
                                ASSERT( assert_okay(), NULL );
                                task* t = get_task( limit );
                                ASSERT( (arena_slot->steal_end&1)==0, "forgot to release lock?" );
                                ASSERT( assert_okay(), NULL );
                                ASSERT( t==expected_task, NULL );       
                                ++get_count;
                            }
                        }
    ASSERT( array_size-array_size_proxy==0, NULL ); // check for any side effects affecting array_size
    if( Verbose )
        printf("%ld successful gets and %ld successful steals\n", get_count, steal_count );
}

//! Test __TBB_CompareAndSwapW
static void TestCompareExchange() {
    ASSERT( intptr(-10)<10, "intptr not a signed integral type?" ); 
    if( Verbose ) 
        printf("testing __TBB_CompareAndSwapW\n");
    for( intptr a=-10; a<10; ++a )
        for( intptr b=-10; b<10; ++b )
            for( intptr c=-10; c<10; ++c ) {
// Workaround for a bug in GCC 4.3.0; and one more is below.
#if __GNUC__==4&&__GNUC_MINOR__==3&&__GNUC_PATCHLEVEL__==0
                intptr x;
                __TBB_store_with_release( x, a );
#else
                intptr x = a;
#endif
                intptr y = __TBB_CompareAndSwapW(&x,b,c);
                ASSERT( y==a, NULL ); 
                if( a==c ) 
                    ASSERT( x==b, NULL );
                else
                    ASSERT( x==a, NULL );
            }
}

//! Test __TBB___TBB_FetchAndIncrement and __TBB___TBB_FetchAndDecrement
static void TestAtomicCounter() {
    // "canary" is a value used to detect illegal overwrites.
    const internal::reference_count canary = ~(internal::uintptr)0/3;
    if( Verbose ) 
        printf("testing __TBB_FetchAndIncrement\n");
    struct {
        internal::reference_count prefix, i, suffix;
    } x;
    x.prefix = canary;
    x.i = 0;
    x.suffix = canary;
    for( int k=0; k<10; ++k ) {
        internal::reference_count j = __TBB_FetchAndIncrementWacquire((volatile void *)&x.i);
        ASSERT( x.prefix==canary, NULL );
        ASSERT( x.suffix==canary, NULL );
        ASSERT( x.i==k+1, NULL );
        ASSERT( j==k, NULL );
    }
    if( Verbose ) 
        printf("testing __TBB_FetchAndDecrement\n");
    x.i = 10;
    for( int k=10; k>0; --k ) {
        internal::reference_count j = __TBB_FetchAndDecrementWrelease((volatile void *)&x.i);
        ASSERT( j==k, NULL );
        ASSERT( x.i==k-1, NULL );
        ASSERT( x.prefix==canary, NULL );
        ASSERT( x.suffix==canary, NULL );
    }
}

static void TestTinyLock() {
    if( Verbose ) 
        printf("testing __TBB_LockByte\n");
    unsigned char flags[16];
    for( int i=0; i<16; ++i )
        flags[i] = i;
#if __GNUC__==4&&__GNUC_MINOR__==3&&__GNUC_PATCHLEVEL__==0
    __TBB_store_with_release( flags[8], 0 );
#else
    flags[8] = 0;
#endif
    __TBB_LockByte(flags[8]);
    for( int i=0; i<16; ++i )
        ASSERT( flags[i]==(i==8?1:i), NULL );
}

static void TestLog2() {
    if( Verbose ) 
        printf("testing __TBB_Log2\n");
    for( uintptr_t i=1; i; i<<=1 ) {
        for( uintptr_t j=1; j<1<<16; ++j ) {
            if( uintptr_t k = i*j ) {
                uintptr_t actual = __TBB_Log2(k);
                const uintptr_t ONE = 1; // warning suppression again
                ASSERT( k >= ONE<<actual, NULL );          
                ASSERT( k>>1 < ONE<<actual, NULL );        
            }
        }
    }
}

static void TestPause() {
    if( Verbose ) 
        printf("testing __TBB_Pause\n");
    __TBB_Pause(1);
}


} // namespace internal 
} // namespace tbb

using namespace tbb;

int main( int argc, char* argv[] ) {
    try {
        ParseCommandLine( argc, argv );
        TestLog2();
        TestTinyLock();
        TestCompareExchange();
        TestAtomicCounter();
        TestPause();

        task_scheduler_init init(1);

        if( Verbose ) 
            printf("testing __TBB_(scheduler assists)\n");
        GenericScheduler* scheduler = GetThreadSpecific();
        scheduler->test_assembly_routines();

    } catch(...) {
        ASSERT(0,"unexpected exception");
    }
    printf("done\n");
    return 0;
}
