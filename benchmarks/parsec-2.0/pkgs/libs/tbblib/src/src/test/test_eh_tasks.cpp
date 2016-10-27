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

// to avoid usage of #pragma comment
#define __TBB_NO_IMPLICIT_LINKAGE 1

#define  COUNT_TASK_NODES 1
#define __TBB_TASK_CPP_DIRECTLY_INCLUDED 1
#include "../tbb/task.cpp"

#if __TBB_EXCEPTIONS

#include "tbb/task_scheduler_init.h"
#include "tbb/spin_mutex.h"
#include "tbb/tick_count.h"
#include <string>

#include "harness.h"
#include "harness_trace.h"

#define NUM_CHILD_TASKS                 256
#define NUM_ROOT_TASKS                  32
#define NUM_ROOTS_IN_GROUP              8
#define EXCEPTION_DESCR "Test exception"

namespace internal = tbb::internal;
using internal::intptr;

namespace util {

    void sleep ( int ms ) {
    #if _WIN32 || _WIN64
        ::Sleep(ms);
    #else
        timespec  requested = { ms / 1000, (ms % 1000)*1000000 };
        timespec  remaining = {0};
        nanosleep(&requested, &remaining);
    #endif
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


//! Statistics about number of tasks in different states
class task_statistics_t
{
    typedef tbb::spin_mutex::scoped_lock lock_t;
    //! Number of tasks allocated that was ever allocated
    volatile intptr my_existed;
    //! Number of tasks executed to the moment
    volatile intptr my_executed;
    //! Number of tasks allocated but not yet destroyed to the moment
    volatile intptr my_existing;

    mutable tbb::spin_mutex  my_mutex;

public:
    //! Assumes that assignment is noncontended for the left-hand operand
    const task_statistics_t& operator= ( const task_statistics_t& rhs ) {
        if ( this != &rhs ) {
            lock_t lock(rhs.my_mutex);
            my_existed = rhs.my_existed;
            my_executed = rhs.my_executed;
            my_existing = rhs.my_existing;
        }
        return *this;
    }

    intptr existed () const { return my_existed; }
    intptr executed () const { return my_executed; }
    intptr existing () const { return my_existing; }

    void inc_existed () { lock_t lock(my_mutex); ++my_existed; ++my_existing; }
    void inc_executed () { lock_t lock(my_mutex); ++my_executed; }
    void dec_existing () { lock_t lock(my_mutex); --my_existing; }

    //! Assumed to be used in noncontended manner only
    void reset () { my_executed = my_existing = my_existed = 0; }

    void trace (const char* prefix ) { 
        const char* separator = prefix && prefix[0] ? " " : "";
        lock_t lock(my_mutex);
        TRACE ("%s%stasks total %u, existing %u, executed %u", prefix, separator, (intptr)my_existed, (intptr)my_existing, (intptr)my_executed);
    }
};

task_statistics_t   g_cur_stat,
                    g_exc_stat; // snapshot of statistics at the moment of exception throwing

internal::GenericScheduler  *g_master = NULL;

volatile intptr g_exception_thrown = 0;
volatile bool g_throw_exception = true;
volatile bool g_no_exception = true;
volatile bool g_unknown_exception = false;
volatile bool g_task_was_cancelled = false;
tbb::atomic<int> g_tasks_started;
int g_tasks_wait_limit = 0;
const int c_timeout = 50000;

bool    g_exception_in_master = false;
bool    g_solitary_exception = true;

//volatile intptr g_num_tasks_when_last_exception = 0;


void reset_globals () {
    g_cur_stat.reset();
    g_exc_stat.reset();
    g_exception_thrown = 0;
    g_throw_exception = true;
    g_no_exception = true;
    g_unknown_exception = false;
    g_task_was_cancelled = false;
    g_tasks_started = 0;
    g_tasks_wait_limit = g_num_threads;
}

intptr num_tasks () { return g_master->get_task_node_count(true); }

void throw_test_exception ( intptr throw_threshold ) {
    if ( !g_throw_exception  ||  g_exception_in_master ^ (internal::GetThreadSpecific() == g_master) )
        return; 
    while ( g_cur_stat.existed() < throw_threshold )
        yield_if_singlecore();
    if ( !g_solitary_exception ) {
        TRACE ("About to throw one of multiple test_exceptions... :");
        throw test_exception(EXCEPTION_DESCR);
    }
    if ( __TBB_CompareAndSwapW(&g_exception_thrown, 1, 0) == 0 ) {
        g_exc_stat = g_cur_stat;
        TRACE ("About to throw solitary test_exception... :");
        throw solitary_test_exception(EXCEPTION_DESCR);
    }
}

// Default timeout value (50000) translates to 20-50 ms on most of the modern architectures
inline void wait_for_exception_with_timeout ( int timeout = 50000 ) {
    int wait_count = 0;
    while ( g_no_exception && (++wait_count < timeout) ) __TBB_Yield();
    if ( wait_count == timeout )
        TRACE("wait_for_exception_with_timeout: wait failed\n");
}

#define TRY()   \
    bool no_exception = true, unknown_exception = false;    \
    try {

// When changing this macro check test10 as well - it uses similar code
#define CATCH()     \
    } catch ( tbb::captured_exception& e ) {     \
        ASSERT (strcmp(e.name(), (g_solitary_exception ? typeid(solitary_test_exception) : typeid(test_exception)).name() ) == 0, "Unexpected original exception name");    \
        ASSERT (strcmp(e.what(), EXCEPTION_DESCR) == 0, "Unexpected original exception info");   \
        if ( g_solitary_exception ) {   \
            g_exc_stat.trace("stat at throw moment:");  \
            g_cur_stat.trace("stat upon catch     :");  \
        }   \
        g_no_exception = no_exception = false;   \
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

#define ASSERT_TEST_POSTCOND()   \
    ASSERT (g_cur_stat.existed() >= g_cur_stat.executed(), "Total number of tasks is less than executed");  \
    

class my_base_task : public tbb::task
{
    tbb::task* execute () {
        tbb::task* t = NULL;
        try { t = do_execute(); } 
        catch ( ... ) { g_cur_stat.inc_executed(); throw; }
        g_cur_stat.inc_executed();
        return t;
    }
protected:
    my_base_task ( bool throw_exception = true ) : my_throw(throw_exception) { g_cur_stat.inc_existed(); }
    ~my_base_task () { g_cur_stat.dec_existing(); }

    virtual tbb::task* do_execute () = 0;

    bool my_throw;
}; // class my_base_task

class my_leaf_task : public my_base_task
{
    tbb::task* do_execute () {
        ++g_tasks_started;
        if ( my_throw )
            throw_test_exception(NUM_CHILD_TASKS/2);
        if ( !g_throw_exception )
            __TBB_Yield();
        return NULL;
    }
public:
    my_leaf_task ( bool throw_exception = true ) : my_base_task(throw_exception) {}
};

class my_simple_root_task : public my_base_task
{
    tbb::task* do_execute () {
        set_ref_count(NUM_CHILD_TASKS + 1);
        for ( size_t i = 0; i < NUM_CHILD_TASKS; ++i ) {
            my_leaf_task &t = *new( allocate_child() ) my_leaf_task(my_throw);
            spawn(t);
            __TBB_Yield();
        }
//        ++g_tasks_started;
        if ( g_exception_in_master  ^  (internal::GetThreadSpecific() == g_master) )
        {
            // Make absolutely sure that worker threads on multicore machines had a chance to steal something
//            util::sleep(10);
            int n = 0;
//            while ( (++n < c_timeout) && (g_tasks_started < g_tasks_wait_limit) ) __TBB_Yield();
            while ( (++n < c_timeout) && !g_tasks_started ) __TBB_Yield();
            if ( n == c_timeout )
                TRACE("my_simple_root_task: wait failed\n");
        }
        wait_for_all();
        return NULL;
    }
public:
    my_simple_root_task ( bool throw_exception = true ) : my_base_task(throw_exception) {}
};

//! Default exception behavior test. 
/** Allocates a root task that spawns a bunch of children, one or several of which throw 
    a test exception in a worker or master thread (depending on the global setting). **/
void Test1 () {
    TRACEP ("");
    reset_globals();
    tbb::empty_task &r = *new( tbb::task::allocate_root() ) tbb::empty_task;
    ASSERT (!g_cur_stat.existing() && !g_cur_stat.existed() && !g_cur_stat.executed(), 
            "something wrong with the task accounting");
    r.set_ref_count(NUM_CHILD_TASKS + 1);
    for ( int i = 0; i < NUM_CHILD_TASKS; ++i ) {
        my_leaf_task &t = *new( r.allocate_child() ) my_leaf_task;
        r.spawn(t);
        // Make sure that worker threads on multicore machines had a chance to steal something
//        util::sleep(1);
        int n = 0;
//        while ( ++n < c_timeout && g_no_exception && g_tasks_started < min (i + 1, g_num_threads - 1) ) __TBB_Yield();
        while ( ++n < c_timeout && !g_tasks_started ) __TBB_Yield();
        if ( n == c_timeout )
            TRACE("Test1: wait failed\n");
    }
    TRY();
        r.wait_for_all();
    CATCH_AND_ASSERT();
    ASSERT_TEST_POSTCOND();
    r.destroy(r);
} // void Test1 ()

//! Default exception behavior test. 
/** Allocates and spawns root task that runs a bunch of children, one of which throws
    a test exception in a worker thread. (Similar to Test1, except that the root task 
    is spawned by the test function, and children are created by the root task instead 
    of the test function body.) **/
void Test2 ()
{
    TRACEP ("");
    reset_globals();
    my_simple_root_task &r = *new( tbb::task::allocate_root() ) my_simple_root_task;
    ASSERT (g_cur_stat.existing() == 1 && g_cur_stat.existed() == 1 && !g_cur_stat.executed(), 
            "something wrong with the task accounting");
    TRY();
        tbb::task::spawn_root_and_wait(r);
    CATCH_AND_ASSERT();
    ASSERT (!g_no_exception, "no exception occurred");
    ASSERT_TEST_POSTCOND();
} // void Test2 ()

//! The same as Test2() except the root task has explicit context.
/** The context is initialized as bound in order to check correctness of its associating 
    with a root task. **/
void Test3 ()
{
    TRACEP ("");
    reset_globals();
    tbb::task_group_context  ctx(tbb::task_group_context::bound);
    my_simple_root_task &r = *new( tbb::task::allocate_root(ctx) ) my_simple_root_task;
    ASSERT (g_cur_stat.existing() == 1 && g_cur_stat.existed() == 1 && !g_cur_stat.executed(), 
            "something wrong with the task accounting");
    TRY();
        tbb::task::spawn_root_and_wait(r);
    CATCH_AND_ASSERT();
    ASSERT (!g_no_exception, "no exception occurred");
    ASSERT_TEST_POSTCOND();
} // void Test2 ()

class my_root_with_context_launcher_task : public my_base_task
{
    tbb::task_group_context::kind_type my_context_kind;
 
    tbb::task* do_execute () {
        tbb::task_group_context  ctx (tbb::task_group_context::isolated);
        my_simple_root_task &r = *new( allocate_root(ctx) ) my_simple_root_task;
        TRY();
            spawn_root_and_wait(r);
            // Give a child of our siblings a chance to throw the test exception
            wait_for_exception_with_timeout();
        CATCH();
        ASSERT_WARNING (!g_no_exception, "no exception occurred - machine overloaded?");
        ASSERT (!g_unknown_exception, "unknown exception was caught");
        return NULL;
    }
public:
    my_root_with_context_launcher_task ( tbb::task_group_context::kind_type ctx_kind = tbb::task_group_context::isolated ) : my_context_kind(ctx_kind) {}
};

/** Allocates and spawns a bunch of roots, which allocate and spawn new root with 
    isolated context, which at last spawns a bunch of children each, one of which 
    throws a test exception in a worker thread. **/
void Test4 () {
    TRACEP ("");
    reset_globals();
    tbb::task_list  tl;
    for ( size_t i = 0; i < NUM_ROOT_TASKS; ++i ) {
        my_root_with_context_launcher_task &r = *new( tbb::task::allocate_root() ) my_root_with_context_launcher_task;
        tl.push_back(r);
    }
    TRY();
        tbb::task::spawn_root_and_wait(tl);
    CATCH_AND_ASSERT();
    ASSERT (no_exception, "unexpected exception intercepted");
    g_cur_stat.trace("stat at end of test :");
    intptr  num_tasks_expected = NUM_ROOT_TASKS * (NUM_CHILD_TASKS + 2);
    ASSERT (g_cur_stat.existed() == num_tasks_expected, "Wrong total number of tasks");
    if ( g_solitary_exception )
        ASSERT (g_cur_stat.executed() >= num_tasks_expected - NUM_CHILD_TASKS, "Unexpected number of executed tasks");
    ASSERT_TEST_POSTCOND();

} // void Test4 ()

class my_root_with_context_group_launcher_task : public my_base_task
{
    tbb::task* do_execute () {
        tbb::task_group_context  ctx (tbb::task_group_context::isolated);
        tbb::task_list  tl;
        for ( size_t i = 0; i < NUM_ROOT_TASKS; ++i ) {
            my_simple_root_task &r = *new( allocate_root(ctx) ) my_simple_root_task;
            tl.push_back(r);
        }
        TRY();
            spawn_root_and_wait(tl);
            // Give worker a chance to throw exception
            wait_for_exception_with_timeout();
        CATCH_AND_ASSERT();
        return NULL;
    }
};

/** Allocates and spawns a bunch of roots, which allocate and spawn groups of roots 
    with an isolated context shared by all group members, which at last spawn a bunch 
    of children each, one of which throws a test exception in a worker thread. **/
void Test5 () {
    TRACEP ("");
    reset_globals();
    tbb::task_list  tl;
    for ( size_t i = 0; i < NUM_ROOTS_IN_GROUP; ++i ) {
        my_root_with_context_group_launcher_task &r = *new( tbb::task::allocate_root() ) my_root_with_context_group_launcher_task;
        tl.push_back(r);
    }
    TRY();
        tbb::task::spawn_root_and_wait(tl);
    CATCH_AND_ASSERT();
    ASSERT (no_exception, "unexpected exception intercepted");
    g_cur_stat.trace("stat at end of test :");
    if ( g_solitary_exception )  {
        intptr  num_tasks_expected = NUM_ROOTS_IN_GROUP * (1 + NUM_ROOT_TASKS * (1 + NUM_CHILD_TASKS));
        intptr  min_num_tasks_executed = num_tasks_expected - NUM_ROOT_TASKS * (NUM_CHILD_TASKS + 1);
        ASSERT (g_cur_stat.executed() >= min_num_tasks_executed, "Too few tasks executed");
    }
    ASSERT_TEST_POSTCOND();
} // void Test5 ()

class my_throwing_root_with_context_launcher_task : public my_base_task
{
    tbb::task* do_execute () {
        tbb::task_group_context  ctx (tbb::task_group_context::bound);
        my_simple_root_task &r = *new( allocate_root(ctx) ) my_simple_root_task(false);
        TRY();
            spawn_root_and_wait(r);
        CATCH();
        ASSERT (no_exception, "unexpected exception intercepted");
        throw_test_exception(NUM_CHILD_TASKS);
        __TBB_Yield();
        g_task_was_cancelled |= is_cancelled();
        return NULL;
    }
};

class my_bound_hierarchy_launcher_task : public my_base_task
{
    bool my_recover;

    void alloc_roots ( tbb::task_group_context& ctx, tbb::task_list& tl ) {
        for ( size_t i = 0; i < NUM_ROOT_TASKS; ++i ) {
            my_throwing_root_with_context_launcher_task &r = *new( allocate_root(ctx) ) my_throwing_root_with_context_launcher_task;
            tl.push_back(r);
        }
    }

    tbb::task* do_execute () {
        tbb::task_group_context  ctx (tbb::task_group_context::isolated);
        tbb::task_list tl;
        alloc_roots(ctx, tl);
        TRY();
            spawn_root_and_wait(tl);
        CATCH_AND_ASSERT();
        ASSERT (!no_exception, "no exception occurred");
        ASSERT (!tl.empty(), "task list was cleared somehow");
        if ( g_solitary_exception )
            ASSERT (g_task_was_cancelled, "No tasks were cancelled despite of exception");
        if ( my_recover ) {
            // Test task_group_context::unbind and task_group_context::reset methods
            g_throw_exception = false;
            no_exception = true;
            tl.clear();
            alloc_roots(ctx, tl);
            ctx.reset();
            try {
                spawn_root_and_wait(tl);
            }
            catch (...) {
                no_exception = false;
            }
            ASSERT (no_exception, "unexpected exception occurred");
        }
        return NULL;
    }
public:
    my_bound_hierarchy_launcher_task ( bool recover = false ) : my_recover(recover) {}

}; // class my_bound_hierarchy_launcher_task

//! Test for bound contexts forming 2 level tree. Exception is thrown on the 1st (root) level.
/** Allocates and spawns a root that spawns a bunch of 2nd level roots sharing 
    the same isolated context, each of which in their turn spawns a single 3rd level 
    root with  the bound context, and these 3rd level roots spawn bunches of leaves 
    in the end. Leaves do not generate exceptions. The test exception is generated 
    by one of the 2nd level roots. **/
void Test6 () {
    TRACEP ("");
    reset_globals();
    my_bound_hierarchy_launcher_task &r = *new( tbb::task::allocate_root() ) my_bound_hierarchy_launcher_task;
    TRY();
        tbb::task::spawn_root_and_wait(r);
    CATCH_AND_ASSERT();
    ASSERT (no_exception, "unexpected exception intercepted");
    g_cur_stat.trace("stat at end of test :");
    // After the first of the branches (my_throwing_root_with_context_launcher_task) completes, 
    // the rest of the task tree may be collapsed before having a chance to execute leaves.
    // A number of branches running concurrently with the first one will be able to spawn leaves though.
    /// \todo: If additional checkpoints are added to scheduler the following assertion must weaken
    intptr  num_tasks_expected = 1 + NUM_ROOT_TASKS * (2 + NUM_CHILD_TASKS);
    intptr  min_num_tasks_created = 1 + g_num_threads * 2 + NUM_CHILD_TASKS;
    // 2 stands for my_bound_hierarchy_launcher_task and my_simple_root_task
    // g_num_threads corresponds to my_bound_hierarchy_launcher_task
    intptr  min_num_tasks_executed = 2 + g_num_threads + NUM_CHILD_TASKS;
    ASSERT (g_cur_stat.existed() <= num_tasks_expected, "Number of expected tasks is calculated incorrectly");
    ASSERT (g_cur_stat.existed() >= min_num_tasks_created, "Too few tasks created");
    ASSERT (g_cur_stat.executed() >= min_num_tasks_executed, "Too few tasks executed");
    ASSERT_TEST_POSTCOND();
} // void Test6 ()

//! Tests task_group_context::unbind and task_group_context::reset methods.
/** Allocates and spawns a root that spawns a bunch of 2nd level roots sharing 
    the same isolated context, each of which in their turn spawns a single 3rd level 
    root with  the bound context, and these 3rd level roots spawn bunches of leaves 
    in the end. Leaves do not generate exceptions. The test exception is generated 
    by one of the 2nd level roots. **/
void Test7 () {
    TRACEP ("");
    reset_globals();
    my_bound_hierarchy_launcher_task &r = *new( tbb::task::allocate_root() ) my_bound_hierarchy_launcher_task;
    TRY();
        tbb::task::spawn_root_and_wait(r);
    CATCH_AND_ASSERT();
    ASSERT (no_exception, "unexpected exception intercepted");
    ASSERT_TEST_POSTCOND();
} // void Test6 ()

class my_bound_hierarchy_launcher_task_2 : public my_base_task
{
    tbb::task* do_execute () {
        tbb::task_group_context  ctx;
        tbb::task_list  tl;
        for ( size_t i = 0; i < NUM_ROOT_TASKS; ++i ) {
            my_root_with_context_launcher_task &r = *new( allocate_root(ctx) ) my_root_with_context_launcher_task(tbb::task_group_context::bound);
            tl.push_back(r);
        }
        TRY();
            spawn_root_and_wait(tl);
        CATCH_AND_ASSERT();
        // Exception must be intercepted by my_root_with_context_launcher_task
        ASSERT (no_exception, "no exception occurred");
        return NULL;
    }
}; // class my_bound_hierarchy_launcher_task_2


//! Test for bound contexts forming 2 level tree. Exception is thrown in the 2nd (outer) level.
/** Allocates and spawns a root that spawns a bunch of 2nd level roots sharing 
    the same isolated context, each of which in their turn spawns a single 3rd level 
    root with  the bound context, and these 3rd level roots spawn bunches of leaves 
    in the end. The test exception is generated by one of the leaves. **/
void Test8 () {
    TRACEP ("");
    reset_globals();
    my_bound_hierarchy_launcher_task_2 &r = *new( tbb::task::allocate_root() ) my_bound_hierarchy_launcher_task_2;
    TRY();
        tbb::task::spawn_root_and_wait(r);
    CATCH_AND_ASSERT();
    ASSERT (no_exception, "unexpected exception intercepted");
    g_cur_stat.trace("stat at end of test :");
    if ( g_solitary_exception )  {
        intptr  num_tasks_expected = 1 + NUM_ROOT_TASKS * (2 + NUM_CHILD_TASKS);
        intptr  min_num_tasks_created = 1 + g_num_threads * (2 + NUM_CHILD_TASKS);
        intptr  min_num_tasks_executed = num_tasks_expected - (NUM_CHILD_TASKS + 1);
        ASSERT (g_cur_stat.existed() <= num_tasks_expected, "Number of expected tasks is calculated incorrectly");
        ASSERT (g_cur_stat.existed() >= min_num_tasks_created, "Too few tasks created");
        ASSERT (g_cur_stat.executed() >= min_num_tasks_executed, "Too few tasks executed");
    }
    ASSERT_TEST_POSTCOND();
} // void Test8 ()


class my_cancellation_root_task : public my_base_task
{
    tbb::task_group_context &my_ctx_to_cancel;
    intptr              my_cancel_threshold;

    tbb::task* do_execute () {
        while ( g_cur_stat.executed() < my_cancel_threshold )
            yield_if_singlecore();
        my_ctx_to_cancel.cancel_group_execution();
        g_exc_stat = g_cur_stat;
        return NULL;
    }
public:
    my_cancellation_root_task ( tbb::task_group_context& ctx, intptr threshold ) 
        : my_ctx_to_cancel(ctx), my_cancel_threshold(threshold)
    {}
};

class my_calculation_root_task : public my_base_task
{
    tbb::task_group_context &my_ctx;

    tbb::task* do_execute () {
        tbb::task::spawn_root_and_wait( *new( tbb::task::allocate_root(my_ctx) ) my_simple_root_task );
        return NULL;
    }
public:
    my_calculation_root_task ( tbb::task_group_context& ctx ) : my_ctx(ctx) {}
};


//! Test for cancelling a task hierarchy from outside (from a task running in parallel with it).
void Test9 () {
    TRACEP ("");
    reset_globals();
    g_throw_exception = false;
    g_tasks_wait_limit = g_num_threads - 1;
    intptr  threshold = NUM_CHILD_TASKS / 4;
    tbb::task_group_context  ctx;
    tbb::task_list  tl;
    tl.push_back( *new( tbb::task::allocate_root() ) my_calculation_root_task(ctx) );
    tl.push_back( *new( tbb::task::allocate_root() ) my_cancellation_root_task(ctx, threshold) );
    TRY();
        tbb::task::spawn_root_and_wait(tl);
    CATCH();
    ASSERT (no_exception, "Cancelling tasks should not cause any exceptions");
    // g_exc_stat contains statistics snapshot at the moment right after cancellation signal sending
    TRACE ("Threshold %d; executed: after cancellation signal %d, total %d", threshold, g_exc_stat.executed(), g_cur_stat.executed());
    // 2 - root tasks in the calculation branch
    //ASSERT_WARNING (g_exc_stat.executed() - threshold <= g_num_threads + 2, "too many tasks executed between reaching threshold and statistics cutoff");
    // 3 - all root tasks 
    ASSERT (g_cur_stat.executed() <= g_exc_stat.executed() + g_num_threads + 3, "Too many tasks were executed after cancellation");
} // void Test9 ()


//static tbb::atomic<int> g_int_exception_data_counter;

template<typename T>
void throw_movable_exception ( intptr throw_threshold, const T& data ) {
    if ( g_exception_in_master ^ (internal::GetThreadSpecific() == g_master) )
        return; 
    if ( !g_solitary_exception ) {
        TRACE ("About to throw one of multiple movable_exceptions... :");
        throw tbb::movable_exception<T>(data);
    }
    while ( g_cur_stat.existed() < throw_threshold )
        yield_if_singlecore();
    if ( __TBB_CompareAndSwapW(&g_exception_thrown, 1, 0) == 0 ) {
        g_exc_stat = g_cur_stat;
        TRACE ("About to throw solitary movable_exception... :");
        throw tbb::movable_exception<T>(data);
    }
}

const int g_int_exception_data = -375;
const std::string g_string_exception_data = "My test string";

// Exception data class implementing minimal requirements of tbb::movable_exception 
class my_exception_data_t {
    const my_exception_data_t& operator = ( const my_exception_data_t& src );
    explicit my_exception_data_t ( int n ) : my_int(n), my_string(g_string_exception_data) {}
public:
    my_exception_data_t ( const my_exception_data_t& src ) : my_int(src.my_int), my_string(src.my_string) {}
    ~my_exception_data_t () {}

    int my_int;
    std::string my_string;

    // Simple way to provide an instance when all initializing constructors are private
    // and to avoid memory reclamation problems.
    static my_exception_data_t s_data;
};

my_exception_data_t my_exception_data_t::s_data(g_int_exception_data);

typedef tbb::movable_exception<int> solitary_movable_exception;
typedef tbb::movable_exception<my_exception_data_t> multiple_movable_exception;

class my_leaf_task_with_movable_exceptions : public my_base_task
{
    bool my_int_as_data;

    tbb::task* do_execute () {
        ++g_tasks_started;
        if ( g_solitary_exception )
            throw_movable_exception<int>(NUM_CHILD_TASKS/2, g_int_exception_data);
        else {
            throw_movable_exception<my_exception_data_t>(NUM_CHILD_TASKS/2, my_exception_data_t::s_data);
        }
        return NULL;
    }
};

//! Test for movable_exception behavior and semantics. 
/** Allocates a root task that spawns a bunch of children, one or several of which throw 
    a movable exception in a worker or master thread (depending on the global settings). **/
void Test10 () {
    TRACEP ("");
    reset_globals();
    tbb::empty_task &r = *new( tbb::task::allocate_root() ) tbb::empty_task;
    ASSERT (!g_cur_stat.existing() && !g_cur_stat.existed() && !g_cur_stat.executed(), 
            "something wrong with the task accounting");
    r.set_ref_count(NUM_CHILD_TASKS + 1);
    for ( int i = 0; i < NUM_CHILD_TASKS; ++i ) {
        my_leaf_task_with_movable_exceptions &t = *new( r.allocate_child() ) my_leaf_task_with_movable_exceptions;
        r.spawn(t);
        // Make sure that worker threads on multicore machines had a chance to steal something
//        util::sleep(20);
//        __TBB_Yield();
        int n = 0;
        while ( ++n < c_timeout && !g_tasks_started ) __TBB_Yield();
        if ( n == c_timeout )
            TRACE("Test10: wait failed\n");
    }
    TRY()
        r.wait_for_all();
    } catch ( tbb::tbb_exception& e ) {
        ASSERT (strcmp(e.name(), (g_solitary_exception ? typeid(solitary_movable_exception) 
                                                       : typeid(multiple_movable_exception)).name() ) == 0, 
                                                       "Unexpected original exception name");
        ASSERT (strcmp(e.what(), "tbb::movable_exception") == 0, "Unexpected original exception info ");
        if ( g_solitary_exception ) {
            solitary_movable_exception& me = dynamic_cast<solitary_movable_exception&>(e);
            ASSERT (me.data() == g_int_exception_data, "Unexpected solitary movable_exception data");
        }
        else {
            multiple_movable_exception& me = dynamic_cast<multiple_movable_exception&>(e);
            ASSERT (me.data().my_int == g_int_exception_data, "Unexpected multiple movable_exception int data");
            ASSERT (me.data().my_string == g_string_exception_data, "Unexpected multiple movable_exception string data");
        }
        if ( g_solitary_exception ) {
            g_exc_stat.trace("stat at throw moment:");
            g_cur_stat.trace("stat upon catch     :");
        }
        g_no_exception = no_exception = false;
    }
    catch ( ... ) {
        g_no_exception = false;
        g_unknown_exception = unknown_exception = true;
    }
    ASSERT_EXCEPTION();
    ASSERT_TEST_POSTCOND();
    r.destroy(r);
} // void Test10 ()



void TestExceptionHandling ()
{
    TRACE ("Number of threads %d", g_num_threads);
    {
    tbb::task_scheduler_init init (g_num_threads);
    g_master = internal::GetThreadSpecific();

    util::sleep(20);
    Test1();
    Test2();
    Test3();
    Test4();
    Test5();
    Test6();
    Test7();
    Test8();
    Test9();
    Test10();
    util::sleep(20);
    }
    ASSERT (!g_cur_stat.existing(), "Not all tasks objects have been destroyed");
    // The following assertion must hold true because if the dummy context is not cleaned up 
    // properly none of the tasks after Test1 completion will be executed.
    ASSERT (g_cur_stat.executed(), "Scheduler's dummy task context has not been cleaned up properly");
}

#endif /* __TBB_EXCEPTIONS */

//------------------------------------------------------------------------

/** If min and max thread numbers specified on the command line are different, 
    the test is run only for 2 sizes of the thread pool (MinThread and MaxThread) 
    to be able to test the high and low contention modes while keeping the test reasonably fast **/
int main(int argc, char* argv[]) {
    ParseCommandLine( argc, argv );
    ASSERT (NUM_ROOTS_IN_GROUP < NUM_ROOT_TASKS, "Fix defines");
    MaxThread = std::min<int>(NUM_ROOTS_IN_GROUP, MaxThread);
    MinThread = std::min<int>(MinThread, MaxThread);
    ASSERT (MinThread>=2, "Minimal number of threads must be 2 or more");
#if __TBB_EXCEPTIONS
    int step = max(MaxThread - MinThread, 1);
    for ( g_num_threads = MinThread; g_num_threads <= MaxThread; g_num_threads += step ) {
        g_max_concurrency = min(g_num_threads, tbb::task_scheduler_init::default_num_threads());
        for ( size_t j = 0; j < 2; ++j ) {
            g_solitary_exception = (j & 2) == 1;
            TestExceptionHandling();
        }
    }
    printf("done\n");
#else
    printf("skipped\n");
#endif /* __TBB_EXCEPTIONS */
    return 0;
}


