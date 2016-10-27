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

#include "tbb/task.h"
#include "tbb/atomic.h"
#include "harness_assert.h"
#include <cstdlib>

//------------------------------------------------------------------------
// Test for task::spawn_children and task_list
//------------------------------------------------------------------------

tbb::atomic<int> Count;

class RecursiveTask: public tbb::task {
    const int my_child_count;
    const int my_depth; 
    //! Spawn tasks in list.  Exact method depends upon my_depth&bit_mask.
    void spawn_list( tbb::task_list& list, int bit_mask ) {
        if( my_depth&bit_mask ) {
            spawn(list);
            ASSERT( list.empty(), NULL );
            wait_for_all();
        } else {
            spawn_and_wait_for_all(list);
            ASSERT( list.empty(), NULL );
        }
    }
public:
    RecursiveTask( int child_count, int depth ) : my_child_count(child_count), my_depth(depth) {}
    /*override*/ tbb::task* execute() {
        ++Count;
        if( my_depth>0 ) {
            tbb::task_list list;
            ASSERT( list.empty(), NULL );
            for( int k=0; k<my_child_count; ++k ) {
                list.push_back( *new( tbb::task::allocate_child() ) RecursiveTask(my_child_count/2,my_depth-1 ) );
                ASSERT( !list.empty(), NULL );
            }
            set_ref_count( my_child_count+1 );
            spawn_list( list, 1 );
            // Now try reusing this as the parent.
            set_ref_count(2);
            list.push_back( *new (tbb::task::allocate_child() ) tbb::empty_task() );
            spawn_list( list, 2 );
        }
        return NULL;
    }
};

//! Compute what Count should be after RecursiveTask(child_count,depth) runs.
static int Expected( int child_count, int depth ) {
    return depth<=0 ? 1 : 1+child_count*Expected(child_count/2,depth-1);
}

#include "tbb/task_scheduler_init.h"
#include "harness.h"

//! Test task::spawn( task_list& )
void TestSpawnChildren( int nthread ) {
    if( Verbose ) 
        printf("testing task::spawn_children for %d threads\n",nthread);
    tbb::task_scheduler_init init(nthread);
    for( int j=0; j<50; ++j ) {
        Count = 0;
        RecursiveTask& p = *new( tbb::task::allocate_root() ) RecursiveTask(j,4);
        tbb::task::spawn_root_and_wait(p);
        int expected = Expected(j,4);
        ASSERT( Count==expected, NULL );
    }
}

//! Test task::spawn_root_and_wait( task_list& )
void TestSpawnRootList( int nthread ) {
    if( Verbose ) 
        printf("testing task::spawn_root_and_wait(task_list&) for %d threads\n",nthread);
    tbb::task_scheduler_init init(nthread);
    for( int j=0; j<5; ++j )
        for( int k=0; k<10; ++k ) {
            Count = 0;
            tbb::task_list list; 
            for( int i=0; i<k; ++i )
                list.push_back( *new( tbb::task::allocate_root() ) RecursiveTask(j,4) );
            tbb::task::spawn_root_and_wait(list);
            int expected = k*Expected(j,4);
            ASSERT( Count==expected, NULL );
        }    
}

//------------------------------------------------------------------------
// Test for task::recycle_as_safe_continuation
//------------------------------------------------------------------------

class TaskGenerator: public tbb::task {
    int my_child_count;
    int my_depth;
    
public:
    TaskGenerator( int child_count, int depth ) : my_child_count(child_count), my_depth(depth) {}
    ~TaskGenerator( ) { my_child_count = my_depth = -125; }

    /*override*/ tbb::task* execute() {
        ASSERT( my_child_count>=0 && my_depth>=0, NULL );
        if( my_depth>0 ) {
            recycle_as_safe_continuation();
            set_ref_count( my_child_count+1 );
            for( int j=0; j<my_child_count; ++j ) {
                tbb::task& t = *new( allocate_child() ) TaskGenerator(my_child_count/2,my_depth-1);
                spawn(t);
            }
            --my_depth;
            __TBB_Yield();
            ASSERT( state()==recycle && ref_count()>0, NULL);
        }
        return NULL;
    }
};

void TestSafeContinuation( int nthread ) {
    if( Verbose ) 
        printf("testing task::recycle_as_safe_continuation for %d threads\n",nthread);
    tbb::task_scheduler_init init(nthread);
    for( int j=8; j<33; ++j ) {
        TaskGenerator& p = *new( tbb::task::allocate_root() ) TaskGenerator(j,5);
        tbb::task::spawn_root_and_wait(p);
    }
}

//------------------------------------------------------------------------
// Test affinity interface
//------------------------------------------------------------------------
tbb::atomic<int> TotalCount;

struct AffinityTask: public tbb::task {
    const tbb::task::affinity_id expected_affinity_id; 
    bool noted;
    /** Computing affinities is NOT supported by TBB, and may disappear in the future.
        It is done here for sake of unit testing. */
    AffinityTask( int expected_affinity_id_ ) : 
        expected_affinity_id(tbb::task::affinity_id(expected_affinity_id_)), 
        noted(false) 
    {
        set_affinity(expected_affinity_id);
        ASSERT( 0u-expected_affinity_id>0u, "affinity_id not an unsigned integral type?" );  
        ASSERT( affinity()==expected_affinity_id, NULL );
    } 
    /*override*/ tbb::task* execute() {
        ++TotalCount;
        return NULL;
    }
    /*override*/ void note_affinity( affinity_id id ) {
        // There is no guarantee in TBB that a task runs on its affinity thread.
        // However, the current implementation does accidentally guarantee it
        // under certain conditions, such as the conditions here.
        // We exploit those conditions for sake of unit testing.
        ASSERT( id!=expected_affinity_id, NULL );
        ASSERT( !noted, "note_affinity_id called twice!" );
        noted = true;
    }
};

/** Note: This test assumes a lot about the internal implementation of affinity.
    Do NOT use this as an example of good programming practice with TBB */
void TestAffinity( int nthread ) {
    TotalCount = 0;
    int n = tbb::task_scheduler_init::default_num_threads();
    if( n>nthread ) 
        n = nthread;
    tbb::task_scheduler_init init(n);
    tbb::empty_task* t = new( tbb::task::allocate_root() ) tbb::empty_task;
    tbb::task::affinity_id affinity_id = t->affinity();
    ASSERT( affinity_id==0, NULL );
    // Set ref_count for n-1 children, plus 1 for the wait.
    t->set_ref_count(n);
    // Spawn n-1 affinitized children.
    for( int i=1; i<n; ++i ) 
        t->spawn( *new(t->allocate_child()) AffinityTask(i) );
    if( n>1 ) {
        // Keep master from stealing
        while( TotalCount!=n-1 ) 
            __TBB_Yield();
    }
    // Wait for the children
    t->wait_for_all();
    t->destroy(*t);
}

//------------------------------------------------------------------------
// Test that recovery actions work correctly for task::allocate_* methods
// when a task's constructor throws an exception.
//------------------------------------------------------------------------

static int TestUnconstructibleTaskCount;

struct ConstructionFailure {
};

//! Task that cannot be constructed.  
template<size_t N>
struct UnconstructibleTask: public tbb::empty_task {
    char space[N];
    UnconstructibleTask() {
        throw ConstructionFailure();
    }
};

#define TRY_BAD_CONSTRUCTION(x)                  \
    {                                            \
        try {                                    \
            new(x) UnconstructibleTask<N>;       \
        } catch( ConstructionFailure ) {         \
            ASSERT( parent()==original_parent, NULL ); \
            ASSERT( ref_count()==original_ref_count, "incorrectly changed ref_count" );\
            ++TestUnconstructibleTaskCount;      \
        }                                        \
    }

template<size_t N>
struct RootTaskForTestUnconstructibleTask: public tbb::task {
    tbb::task* execute() {
        tbb::task* original_parent = parent();
        ASSERT( original_parent!=NULL, NULL );
        int original_ref_count = ref_count();
        TRY_BAD_CONSTRUCTION( allocate_root() );
        TRY_BAD_CONSTRUCTION( allocate_child() );
        TRY_BAD_CONSTRUCTION( allocate_continuation() );
        TRY_BAD_CONSTRUCTION( allocate_additional_child_of(*this) );
        return NULL;
    }
};

template<size_t N>
void TestUnconstructibleTask() {
    TestUnconstructibleTaskCount = 0;
    tbb::task_scheduler_init init;
    tbb::task* t = new( tbb::task::allocate_root() ) RootTaskForTestUnconstructibleTask<N>;
    tbb::task::spawn_root_and_wait(*t);
    ASSERT( TestUnconstructibleTaskCount==4, NULL );
}

//------------------------------------------------------------------------
// Test for alignment problems with task objects.
//------------------------------------------------------------------------

//! Task with members of type T.
/** The task recursively creates tasks. */
template<typename T> 
class TaskWithMember: public tbb::task {
    T x;
    T y;
    unsigned char count;
    /*override*/ tbb::task* execute() {
        x = y;
        if( count>0 ) { 
            set_ref_count(2);
            tbb::task* t = new( tbb::task::allocate_child() ) TaskWithMember<T>(count-1);
            spawn_and_wait_for_all(*t);
        }
        return NULL;
    }
public:
    TaskWithMember( unsigned char n ) : count(n) {}
};

template<typename T> 
void TestAlignmentOfOneClass() {
    typedef TaskWithMember<T> task_type;
    tbb::task* t = new( tbb::task::allocate_root() ) task_type(10);
    tbb::task::spawn_root_and_wait(*t);
}

#include "harness_m128.h"

void TestAlignment() {
    if( Verbose ) 
        printf("testing alignment\n");
    tbb::task_scheduler_init init;
    // Try types that have variety of alignments
    TestAlignmentOfOneClass<char>();
    TestAlignmentOfOneClass<short>();
    TestAlignmentOfOneClass<int>();
    TestAlignmentOfOneClass<long>();
    TestAlignmentOfOneClass<void*>();
    TestAlignmentOfOneClass<float>();
    TestAlignmentOfOneClass<double>();
#if HAVE_m128
    TestAlignmentOfOneClass<__m128>();
#endif /* HAVE_m128 */
}

//------------------------------------------------------------------------
// Test for recursing on left while spawning on right
//------------------------------------------------------------------------

int Fib( int n );

struct RightFibTask: public tbb::task {
    int* y;
    const int n;
    RightFibTask( int* y_, int n_ ) : y(y_), n(n_) {}
    task* execute() {
        *y = Fib(n-1);
        return 0;
    } 
};

int Fib( int n ) {
    if( n<2 ) {
        return n;
    } else {
        int y;
        tbb::task* root_task = new( tbb::task::allocate_root() ) tbb::empty_task;
        root_task->set_ref_count(2);
        root_task->spawn( *new( root_task->allocate_child() ) RightFibTask(&y,n) );
        int x = Fib(n-2);
        root_task->wait_for_all();
        tbb::task::self().destroy(*root_task);
        return y+x;
    }
}

void TestLeftRecursion( int p ) {
    tbb::task_scheduler_init init(p);
    int sum = 0; 
    for( int i=0; i<100; ++i )
        sum +=Fib(10);
    ASSERT( sum==5500, NULL );
}

//------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    srand(2);
    MinThread = 1;
    ParseCommandLine( argc, argv );
    TestUnconstructibleTask<1>();
    TestUnconstructibleTask<10000>();
    TestAlignment();
    for( int p=MinThread; p<=MaxThread; ++p ) {
        TestSpawnChildren( p );
        TestSpawnRootList( p );
        TestSafeContinuation( p );
        TestLeftRecursion( p );
        TestAffinity( p );
    }
    printf("done\n");
    return 0;
}

