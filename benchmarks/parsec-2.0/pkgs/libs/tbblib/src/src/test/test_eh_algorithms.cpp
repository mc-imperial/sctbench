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

#include "stddef.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/tbb_exception.h"
#include "tbb/task.h"
#include "tbb/atomic.h"
#include "tbb/parallel_for.h"
#include "tbb/parallel_reduce.h"
#include "tbb/blocked_range.h"

#include <cmath>
#include <typeinfo>

#include "harness.h"
#include "harness_trace.h"


#if __TBB_EXCEPTIONS

//------------------------------------------------------------------------
// Utility definitions
//------------------------------------------------------------------------

#define ITER_RANGE  100000
#define ITER_GRAIN  1000
#define NESTING_RANGE  100
#define NESTING_GRAIN  10
#define NESTED_RANGE  (ITER_RANGE / NESTING_RANGE)
#define NESTED_GRAIN  (ITER_GRAIN / NESTING_GRAIN)
#define EXCEPTION_DESCR "Test exception"

namespace internal = tbb::internal;
using internal::intptr;

namespace util {

#if _WIN32 || _WIN64

    typedef DWORD tid_t;

    tid_t get_my_tid () { return GetCurrentThreadId(); }

    void sleep ( int ms ) { ::Sleep(ms); }

#else /* !WIN */

    typedef pthread_t tid_t;

    tid_t get_my_tid () { return pthread_self(); }

    void sleep ( int ms ) {
        timespec  requested = { ms / 1000, (ms % 1000)*1000000 };
        timespec  remaining = {0};
        nanosleep(&requested, &remaining);
    }

#endif /* !WIN */

inline intptr num_subranges ( intptr length, intptr grain ) {
    return (intptr)std::pow(2., std::ceil(std::log((double)length / grain) / std::log(2.)));
}

} // namespace util

int g_max_concurrency = 0;
int g_num_threads = 0;

inline void yield_if_singlecore() { if ( g_max_concurrency == 1 ) __TBB_Yield(); }


class test_exception : public std::exception
{
    const char* my_description;
public:
    test_exception ( const char* description ) : my_description(description) {}

    const char* what() const throw() { return my_description; }
};

class solitary_test_exception : public test_exception
{
public:
    solitary_test_exception ( const char* description ) : test_exception(description) {}
};


tbb::atomic<intptr> g_cur_executed, // number of times a body was requested to process data
                    g_exc_executed, // snapshot of execution statistics at the moment of the 1st exception throwing
                    g_catch_executed, // snapshot of execution statistics at the moment when the 1st exception is caught
                    g_exceptions; // number of exceptions exposed to TBB users (i.e. intercepted by the test code)

util::tid_t  g_master = 0;

volatile intptr g_exception_thrown = 0;
volatile bool g_throw_exception = true;
volatile bool g_no_exception = true;
volatile bool g_unknown_exception = false;
volatile bool g_task_was_cancelled = false;

bool    g_exception_in_master = false;
bool    g_solitary_exception = true;
volatile bool   g_wait_completed = false;


void reset_globals () {
    g_cur_executed = g_exc_executed = g_catch_executed = 0;
    g_exceptions = 0;
    g_exception_thrown = 0;
    g_throw_exception = true;
    g_no_exception = true;
    g_unknown_exception = false;
    g_task_was_cancelled = false;
    g_wait_completed = false;
}

void throw_test_exception ( intptr throw_threshold ) {
    if ( !g_throw_exception  ||  g_exception_in_master ^ (util::get_my_tid() == g_master) )
        return; 
    if ( !g_solitary_exception ) {
        __TBB_CompareAndSwapW(&g_exc_executed, g_cur_executed, 0);
        TRACE ("About to throw one of multiple test_exceptions (thread %08x):", util::get_my_tid());
        throw (test_exception(EXCEPTION_DESCR));
    }
    while ( g_cur_executed < throw_threshold )
        yield_if_singlecore();
    if ( __TBB_CompareAndSwapW(&g_exception_thrown, 1, 0) == 0 ) {
        g_exc_executed = g_cur_executed;
        TRACE ("About to throw solitary test_exception... :");
        throw (solitary_test_exception(EXCEPTION_DESCR));
    }
}

#define TRY()   \
    bool no_exception = true, unknown_exception = false;    \
    try {

#define CATCH()     \
    } catch ( tbb::captured_exception& e ) {     \
        g_catch_executed = g_cur_executed;  \
        ASSERT (strcmp(e.name(), (g_solitary_exception ? typeid(solitary_test_exception) : typeid(test_exception)).name() ) == 0, "Unexpected original exception name");    \
        ASSERT (strcmp(e.what(), EXCEPTION_DESCR) == 0, "Unexpected original exception info");   \
        TRACE ("Executed at throw moment %d; upon catch %d", (intptr)g_exc_executed, (intptr)g_catch_executed);  \
        g_no_exception = no_exception = false;   \
        ++g_exceptions; \
    }   \
    catch ( ... ) { \
        g_no_exception = false;   \
        g_unknown_exception = unknown_exception = true;   \
    }

#define ASSERT_EXCEPTION()     \
    ASSERT (!g_no_exception, "no exception occurred");    \
    ASSERT (!g_unknown_exception, "unknown exception was caught");

#define CATCH_AND_ASSERT()     \
    CATCH() \
    ASSERT_EXCEPTION()

#define ASSERT_TEST_POSTCOND()


//------------------------------------------------------------------------
// Tests
//------------------------------------------------------------------------

typedef size_t count_type;
typedef tbb::blocked_range<count_type> range_type;


template<class Body>
intptr test_num_subranges_calculation ( intptr length, intptr grain, intptr nested_length, intptr nested_grain ) {
    reset_globals();
    g_throw_exception = false;
    intptr  nesting_body_calls = util::num_subranges(length, grain),
            nested_body_calls = util::num_subranges(nested_length, nested_grain),
            calls_in_normal_case = nesting_body_calls * (nested_body_calls + 1);
    tbb::parallel_for( range_type(0, length, grain), Body() );
    ASSERT (g_cur_executed == calls_in_normal_case, "Wrong estimation of bodies invocation count");
    return calls_in_normal_case;
}

class no_throw_pfor_body {
public:
    void operator()( const range_type& r ) const {
        volatile long x;
        count_type end = r.end();
        for( count_type i=r.begin(); i<end; ++i )
            x = 0;
    }
};

void Test0 () {
    TRACEP ("");
    reset_globals();
    tbb::simple_partitioner p;
    for( size_t i=0; i<10; ++i ) {
        tbb::parallel_for( range_type(0, 0, 1), no_throw_pfor_body() );
        tbb::parallel_for( range_type(0, 0, 1), no_throw_pfor_body(), p );
        tbb::parallel_for( range_type(0, 128, 8), no_throw_pfor_body() );
        tbb::parallel_for( range_type(0, 128, 8), no_throw_pfor_body(), p );
    }
} // void Test0 ()


class simple_pfor_body {
public:
    void operator()( const range_type& r ) const {
        volatile long x;
        count_type end = r.end();
        for( count_type i=r.begin(); i<end; ++i )
            x = 0;
        ++g_cur_executed;
        if ( g_exception_in_master ^ (util::get_my_tid() == g_master) )
        {
            //while ( g_no_exception ) __TBB_Yield();
            // Make absolutely sure that worker threads on multicore machines had a chance to steal something
            util::sleep(10);
        }
        throw_test_exception(1);
    }
};

void Test1 () {
    TRACEP ("");
    reset_globals();
    TRY();
        tbb::parallel_for( range_type(0, ITER_RANGE, ITER_GRAIN), simple_pfor_body() ); // , tbb::simple_partitioner()
    CATCH_AND_ASSERT();
    ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
    TRACE ("Executed at the end of test %d; number of exceptions", (intptr)g_cur_executed);
    ASSERT (g_exceptions == 1, "No try_blocks in any body expected in this test");
    if ( !g_solitary_exception )
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
} // void Test1 ()


class nesting_pfor_body {
public:
    void operator()( const range_type& ) const {
        ++g_cur_executed;
        if ( util::get_my_tid() == g_master )
            yield_if_singlecore();
        tbb::parallel_for( tbb::blocked_range<size_t>(0, NESTED_RANGE, NESTED_GRAIN), simple_pfor_body() );
    }
};

//! Uses parallel_for body containing a nested parallel_for with the default context not wrapped by a try-block.
/** Nested algorithms are spawned inside the new bound context by default. Since 
    exceptions thrown from the nested parallel_for are not handled by the caller 
    (nesting parallel_for body) in this test, they will cancel all the sibling nested 
    algorithms. **/
void Test2 () {
    TRACEP ("");
    reset_globals();
    TRY();
        tbb::parallel_for( range_type(0, NESTING_RANGE, NESTING_GRAIN), nesting_pfor_body() );
    CATCH_AND_ASSERT();
    ASSERT (!no_exception, "No exception thrown from the nesting parallel_for");
    //if ( g_solitary_exception )
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
    TRACE ("Executed at the end of test %d", (intptr)g_cur_executed);
    ASSERT (g_exceptions == 1, "No try_blocks in any body expected in this test");
    if ( !g_solitary_exception )
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
} // void Test2 ()


class nesting_pfor_with_isolated_context_body {
public:
    void operator()( const range_type& ) const {
        tbb::task_group_context ctx(tbb::task_group_context::isolated);
        ++g_cur_executed;
//        util::sleep(1); // Give other threads a chance to steal their first tasks
        __TBB_Yield();
        tbb::parallel_for( tbb::blocked_range<size_t>(0, NESTED_RANGE, NESTED_GRAIN), simple_pfor_body(), tbb::simple_partitioner(), ctx );
    }
};

//! Uses parallel_for body invoking a nested parallel_for with an isolated context without a try-block.
/** Even though exceptions thrown from the nested parallel_for are not handled 
    by the caller in this test, they will not affect sibling nested algorithms 
    already running because of the isolated contexts. However because the first 
    exception cancels the root parallel_for only the first g_num_threads subranges
    will be processed (which launch nested parallel_fors) **/
void Test3 () {
    TRACEP ("");
    reset_globals();
    typedef nesting_pfor_with_isolated_context_body body_type;
    intptr  nested_body_calls = util::num_subranges(NESTED_RANGE, NESTED_GRAIN),
            min_num_calls = (g_num_threads - 1) * nested_body_calls;
    TRY();
        tbb::parallel_for( range_type(0, NESTING_RANGE, NESTING_GRAIN), body_type() );
    CATCH_AND_ASSERT();
    ASSERT (!no_exception, "No exception thrown from the nesting parallel_for");
    TRACE ("Executed at the end of test %d", (intptr)g_cur_executed);
    if ( g_solitary_exception ) {
        ASSERT (g_cur_executed > min_num_calls, "Too few tasks survived exception");
        ASSERT (g_cur_executed <= min_num_calls + (g_catch_executed + g_num_threads), "Too many tasks survived exception");
    }
    ASSERT (g_exceptions == 1, "No try_blocks in any body expected in this test");
    if ( !g_solitary_exception )
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
} // void Test3 ()


class nesting_pfor_with_eh_body {
public:
    void operator()( const range_type& ) const {
        tbb::task_group_context ctx(tbb::task_group_context::isolated);
        ++g_cur_executed;
        TRY();
            tbb::parallel_for( tbb::blocked_range<size_t>(0, NESTED_RANGE, NESTED_GRAIN), simple_pfor_body(), tbb::simple_partitioner(), ctx );
        CATCH();
    }
};

//! Uses parallel_for body invoking a nested parallel_for (with default bound context) inside a try-block.
/** Since exception(s) thrown from the nested parallel_for are handled by the caller 
    in this test, they do not affect neither other tasks of the the root parallel_for 
    nor sibling nested algorithms. **/
void Test4 () {
    TRACEP ("");
    reset_globals();
    intptr  nested_body_calls = util::num_subranges(NESTED_RANGE, NESTED_GRAIN),
            nesting_body_calls = util::num_subranges(NESTING_RANGE, NESTING_GRAIN),
            calls_in_normal_case = nesting_body_calls * (nested_body_calls + 1);
    TRY();
        tbb::parallel_for( range_type(0, NESTING_RANGE, NESTING_GRAIN), nesting_pfor_with_eh_body() );
    CATCH();
    ASSERT (no_exception, "All exceptions must have been handled in the parallel_for body");
    TRACE ("Executed %d (normal case %d), exceptions %d, in master only? %d", (intptr)g_cur_executed, calls_in_normal_case, (intptr)g_exceptions, g_exception_in_master);
    intptr  min_num_calls = 0;
    if ( g_solitary_exception ) {
        min_num_calls = calls_in_normal_case - nested_body_calls;
        ASSERT (g_exceptions == 1, "No exception registered");
        ASSERT (g_cur_executed <= min_num_calls + g_num_threads, "Too many tasks survived exception");
    }
    else if ( !g_exception_in_master ) {
        // Each nesting body + at least 1 of its nested body invocations
        min_num_calls = 2 * nesting_body_calls;
        ASSERT (g_exceptions > 1 && g_exceptions <= nesting_body_calls, "Unexpected actual number of exceptions");
        ASSERT (g_cur_executed >= min_num_calls + (nesting_body_calls - g_exceptions) * nested_body_calls, "Too few tasks survived exception");
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived multiple exceptions");
        // Additional nested_body_calls accounts for the minimal amount of tasks spawned 
        // by not throwing threads. In the minimal case it is either the master thread or the only worker.
        ASSERT (g_cur_executed <= min_num_calls + (nesting_body_calls - g_exceptions + 1) * nested_body_calls + g_exceptions + g_num_threads, "Too many tasks survived exception");
    }
} // void Test4 ()


class my_cancellator_task : public tbb::task
{
    tbb::task_group_context &my_ctx_to_cancel;
    intptr my_cancel_threshold;

    tbb::task* execute () {
        s_cancellator_ready = true;
        while ( g_cur_executed < my_cancel_threshold )
            yield_if_singlecore();
        my_ctx_to_cancel.cancel_group_execution();
        g_catch_executed = g_cur_executed;
        return NULL;
    }
public:
    my_cancellator_task ( tbb::task_group_context& ctx, intptr threshold ) 
        : my_ctx_to_cancel(ctx), my_cancel_threshold(threshold)
    {}

    static volatile bool s_cancellator_ready;
};

volatile bool my_cancellator_task::s_cancellator_ready = false;

class pfor_body_to_cancel {
public:
    void operator()( const range_type& r ) const {
        ++g_cur_executed;
        do {
            __TBB_Yield();
        } while( !my_cancellator_task::s_cancellator_ready );
    }
};

template<class B>
class my_worker_task : public tbb::task
{
    tbb::task_group_context &my_ctx;

    tbb::task* execute () {
        tbb::parallel_for( range_type(0, ITER_RANGE, ITER_GRAIN), B(), tbb::simple_partitioner(), my_ctx );
        return NULL;
    }
public:
    my_worker_task ( tbb::task_group_context& ctx ) : my_ctx(ctx) {}
};


//! Test for cancelling an algorithm from outside (from a task running in parallel with the algorithm).
void Test5 () {
    TRACEP ("");
    reset_globals();
    g_throw_exception = false;
    intptr  threshold = util::num_subranges(ITER_RANGE, ITER_GRAIN) / 4;
    tbb::task_group_context  ctx;
    ctx.reset();
    my_cancellator_task::s_cancellator_ready = false;
    tbb::empty_task &r = *new( tbb::task::allocate_root() ) tbb::empty_task;
    r.set_ref_count(3);
    r.spawn( *new( r.allocate_child() ) my_cancellator_task(ctx, threshold) );
    __TBB_Yield();
    r.spawn( *new( r.allocate_child() ) my_worker_task<pfor_body_to_cancel>(ctx) );
    TRY();
        r.wait_for_all();
    CATCH();
    r.destroy(r);
    ASSERT (no_exception, "Cancelling tasks should not cause any exceptions");
    //ASSERT_WARNING (g_catch_executed < threshold + 2 * g_num_threads, "Too many tasks were executed between reaching threshold and signaling cancellation");
    ASSERT (g_cur_executed < g_catch_executed + g_num_threads, "Too many tasks were executed after cancellation");
} // void Test5 ()

class my_cancellator_2_task : public tbb::task
{
    tbb::task_group_context &my_ctx_to_cancel;

    tbb::task* execute () {
        util::sleep(20);  // allow the first workers to start
        my_ctx_to_cancel.cancel_group_execution();
        g_catch_executed = g_cur_executed;
        return NULL;
    }
public:
    my_cancellator_2_task ( tbb::task_group_context& ctx ) : my_ctx_to_cancel(ctx) {}
};

class pfor_body_to_cancel_2 {
public:
    void operator()( const range_type& r ) const {
        ++g_cur_executed;
        // The test will hang (and be timed out by the test system) if is_cancelled() is broken
        while( !tbb::task::self().is_cancelled() ) __TBB_Yield();
    }
};

//! Test for cancelling an algorithm from outside (from a task running in parallel with the algorithm).
/** This version also tests task::is_cancelled() method. **/
void Test6 () {
    TRACEP ("");
    reset_globals();
    tbb::task_group_context  ctx;
    tbb::empty_task &r = *new( tbb::task::allocate_root() ) tbb::empty_task;
    r.set_ref_count(3);
    r.spawn( *new( r.allocate_child() ) my_cancellator_2_task(ctx) );
    __TBB_Yield();
    r.spawn( *new( r.allocate_child() ) my_worker_task<pfor_body_to_cancel_2>(ctx) );
    TRY();
        r.wait_for_all();
    CATCH();
    r.destroy(r);
    ASSERT (no_exception, "Cancelling tasks should not cause any exceptions");
    ASSERT_WARNING (g_catch_executed < g_num_threads, "Somehow worker tasks started their execution before the cancellator task");
    ASSERT (g_cur_executed <= g_catch_executed, "Some tasks were executed after cancellation");
} // void Test6 ()

////////////////////////////////////////////////////////////////////////////////
// Regression test based on the contribution by the author of the following forum post:
// http://softwarecommunity.intel.com/isn/Community/en-US/forums/thread/30254959.aspx

#define LOOP_COUNT 16
#define MAX_NESTING 3
#define REDUCE_RANGE 1024 
#define REDUCE_GRAIN 256

class my_worker_t {
public:
    void doit (int & result, int nest);
};

class reduce_test_body_t {
    my_worker_t * my_shared_worker;
    int my_nesting_level;
    int my_result;
public:
    reduce_test_body_t ( reduce_test_body_t& src, tbb::split )
        : my_shared_worker(src.my_shared_worker)
        , my_nesting_level(src.my_nesting_level)
        , my_result(0)
    {}
    reduce_test_body_t ( my_worker_t *w, int nesting )
        : my_shared_worker(w)
        , my_nesting_level(nesting)
        , my_result(0)
    {}

    void operator() ( const tbb::blocked_range<size_t>& r ) {
        for (size_t i = r.begin (); i != r.end (); ++i) {
            int result = 0;
            my_shared_worker->doit (result, my_nesting_level);
            my_result += result;
        }
    }
    void join (const reduce_test_body_t & x) {
        my_result += x.my_result;
    }
    int result () { return my_result; }
};

void my_worker_t::doit ( int& result, int nest ) {
    ++nest;
    if ( nest < MAX_NESTING ) {
        reduce_test_body_t rt (this, nest);
        tbb::parallel_reduce (tbb::blocked_range<size_t>(0, REDUCE_RANGE, REDUCE_GRAIN), rt);
        result = rt.result ();
    }
    else
        ++result;
}

//! Regression test for hanging that occurred with the first version of cancellation propagation
void Test7 () {
    TRACEP ("");
    my_worker_t w;
    int result = 0;
    w.doit (result, 0);
    ASSERT ( result == 1048576, "Wrong calculation result");
}

void RunTests () {
    TRACE ("Number of threads %d", g_num_threads);
    tbb::task_scheduler_init init (g_num_threads);
    g_master = util::get_my_tid();
    
    Test0();
#if !(__GLIBC__==2&&__GLIBC_MINOR__==3)
    Test1();
    Test3();
    Test4();
#endif
    Test5();
    Test6();
    Test7();
}

#endif /* __TBB_EXCEPTIONS */


//------------------------------------------------------------------------
// Entry point
//------------------------------------------------------------------------

/** If min and max thread numbers specified on the command line are different, 
    the test is run only for 2 sizes of the thread pool (MinThread and MaxThread) 
    to be able to test the high and low contention modes while keeping the test reasonably fast **/
int main(int argc, char* argv[]) {
    ParseCommandLine( argc, argv );
    MinThread = min(MinThread, MaxThread);
    ASSERT (MinThread>=2, "Minimal number of threads must be 2 or more");
    ASSERT (ITER_RANGE >= ITER_GRAIN * MaxThread, "Fix defines");
#if __TBB_EXCEPTIONS
    int step = max(MaxThread - MinThread, 1);
    for ( g_num_threads = MinThread; g_num_threads <= MaxThread; g_num_threads += step ) {
        g_max_concurrency = min(g_num_threads, tbb::task_scheduler_init::default_num_threads());
        // Execute in all the possible modes
        for ( size_t j = 0; j < 4; ++j ) {
            g_exception_in_master = (j & 1) == 1;
            g_solitary_exception = (j & 2) == 1;
            RunTests();
        }
    }
#if __GLIBC__==2&&__GLIBC_MINOR__==3
    printf("Warning: Exception handling tests are skipped due to a known issue.\n");
#endif // workaround
    printf("done\n");
#else  /* __TBB_EXCEPTIONS */
    printf("skipped\n");
#endif /* __TBB_EXCEPTIONS */
    return 0;
}

