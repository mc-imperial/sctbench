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

// Declarations for rock-bottom simple test harness.
// Just include this file to use it.
// Every test is presumed to have a command line of the form "foo [-v] [nthread]"
// The default for nthread is 2.

#if __SUNPRO_CC
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <cstdio>
#include <cstdlib>
#include <cstring>
#endif
#include <new>
#include "harness_assert.h"

#if __linux__||__APPLE__||__FreeBSD__ || __sun
    #include <pthread.h>
#elif _WIN32||WIN64
    #include <windows.h>
    #include <process.h>
#else
    #error unknown OS
#endif

static void ReportError( int line, const char* expression, const char * message, bool is_error ) {
    if ( is_error ) {
        fprintf(stderr,"Line %d, assertion %s: %s\n", line, expression, message ? message : "failed" );
#if TBB_EXIT_ON_ASSERT
        exit(1);
#else
        abort();
#endif /* TBB_EXIT_ON_ASSERT */
    }
    else
        printf("WARNING: at line %d, assertion %s: %s\n", line, expression, message ? message : "failed" );
}

#if !HARNESS_NO_PARSE_COMMAND_LINE
//! Controls level of commentary.
/** If true, makes the test print commentary.  If false, test should print "done" and nothing more. */
static bool Verbose;

//! Minimum number of threads
/** The default is 0, which is typically interpreted by tests as "run without TBB". */
static int MinThread = 0;

//! Maximum number of threads
static int MaxThread = 2;

//! NThread exists for backwards compatibility.  Eventually it will be removed.
#define NThread (MaxThread==MinThread?MaxThread : (ReportError(__LINE__,"NThread","thread range not supported"),-1))

//! Parse command line of the form "name [-v] [nthread]"
/** Sets Verbose, MinThread, and MaxThread accordingly.
    The nthread argument can be a single number or a range of the form m:n.
    A single number m is interpreted as if written m:m. 
    The numbers must be non-negative.  
    Clients often treat the value 0 as "run sequentially." */
static void ParseCommandLine( int argc, char* argv[] ) {
    int i = 1;  
    if( i<argc ) {
        if( strcmp( argv[i], "-v" )==0 ) {
            Verbose = true;
            ++i;
        }
    }
    if( i<argc ) {
        char* endptr;
        MinThread = strtol( argv[i], &endptr, 0 );
        if( *endptr==':' )
            MaxThread = strtol( endptr+1, &endptr, 0 );
        else if( *endptr=='\0' ) 
            MaxThread = MinThread;
        if( *endptr!='\0' ) {
            fprintf(stderr,"garbled nthread range\n");
            exit(1);
        }    
        if( MinThread<0 ) {
            fprintf(stderr,"nthread must be nonnegative\n");
            exit(1);
        }
        if( MaxThread<MinThread ) {
            fprintf(stderr,"nthread range is backwards\n");
            exit(1);
        }
        ++i;
    }
    if( i!=argc ) {
        fprintf(stderr,"Usage: %s [-v] [nthread|minthread:maxthread]\n", argv[0] );
        exit(1);
    }
}
#endif /* HARNESS_NO_PARSE_COMMAND_LINE */\

//! For internal use by template function NativeParallelFor
template<typename Range, typename Body>
class NativeParalleForTask {
public:
    NativeParalleForTask( const Range& range_, const Body& body_ ) :
        range(range_),
        body(body_)
    {}

    //! Start task
    void start() {
#if __linux__||__APPLE__||__FreeBSD__ || __sun
        int status = pthread_create(&thread_id, NULL, thread_function, this);
        ASSERT(0==status, "NativeParallelFor: pthread_create failed");
#else
        unsigned thread_id;
        thread_handle = (HANDLE)_beginthreadex( NULL, 0, thread_function, this, 0, &thread_id );
        ASSERT( thread_handle!=0, "NativeParallelFor: _beginthreadex failed" );
#endif
    }

    //! Wait for task to finish
    void wait_to_finish() {
#if __linux__||__APPLE__||__FreeBSD__ || __sun
        int status = pthread_join( thread_id, NULL );
        ASSERT( !status, "pthread_join failed" );
#else
        DWORD status = WaitForSingleObject( thread_handle, INFINITE );
        ASSERT( status!=WAIT_FAILED, "WaitForSingleObject failed" );
        CloseHandle( thread_handle );
#endif 
    }

    //! Build (or precompute size of) array of tasks.
    /** Computes number of of tasks required, plus index. 
        If array!=NULL, also constructs the necessary tasks, starting at array[index].
        Top-level caller should let index default to 0. */
    static size_t build_task_array( const Range& range, const Body& body, NativeParalleForTask* array, size_t index ); 
private:
#if __linux__||__APPLE__||__FreeBSD__ || __sun
    pthread_t thread_id;
#else
    HANDLE thread_handle;
#endif

    //! Range over which task will invoke the body.
    const Range range;

    //! Body to invoke over the range.
    const Body body;

#if __linux__||__APPLE__||__FreeBSD__ || __sun
    static void* thread_function(void* object)
#else
    static unsigned __stdcall thread_function( void* object )
#endif
    {
        NativeParalleForTask& self = *static_cast<NativeParalleForTask*>(object);
        (self.body)(self.range);
        return 0;
    }
};

#include "tbb/tbb_stddef.h"

template<typename Range,typename Body>
size_t NativeParalleForTask<Range,Body>::build_task_array( const Range& range, const Body& body, NativeParalleForTask* array, size_t index ) {
    if( !range.is_divisible() ) { 
        if( array ) {
            new( &array[index] ) NativeParalleForTask(range,body);
        }
        return index+1;
    } else { 
        Range r1 = range;
        Range r2(r1,tbb::split());
        return build_task_array( r2, body, array, build_task_array(r1,body,array,index) );
    }                
}

//! NativeParallelFor is like a TBB parallel_for.h, but with each invocation of Body in a separate thread.
/** By using a blocked_range with a grainsize of 1, you can guarantee 
    that each iteration is performed by a separate thread */
template <typename Range, typename Body>
void NativeParallelFor(const Range& range, const Body& body) {
    typedef NativeParalleForTask<Range,Body> task;

    if( !range.empty() ) {
        // Compute how many tasks are needed
        size_t n = task::build_task_array(range,body,NULL,0);

        // Allocate array to hold the tasks
        task* array = static_cast<task*>(operator new( n*sizeof(task) ));

        // Construct the tasks
        size_t m = task::build_task_array(range,body,array,0);
        ASSERT( m==n, "range splitting not deterministic" );

        // Start the tasks
        for( size_t j=0; j<n; ++j )
            array[j].start();

        // Wait for the tasks
        for( size_t j=n; j>0; --j ) {
            array[j-1].wait_to_finish();
            array[j-1].~task();
        }

        // Deallocate the task array
        operator delete(array);
    }
}

//! The function to zero-initialize arrays; useful to avoid warnings
template <typename T>
void zero_fill(void* array, size_t N) {
    memset(array, 0, sizeof(T)*N);
}

#ifndef min
    //! Utility template function returning lesser of the two values.
    /** Provided here to avoid including not strict safe <algorithm>.\n
        In case operands cause sined/unsigned or size mismatch warnings it is caller's
        responsibility to do the appropriate cast before calling the function. **/
    template<typename T1, typename T2>
    const T1& min ( const T1& val1, const T2& val2 ) {
        return val1 < val2 ? val1 : val2;
    }
#endif /* !min */

#ifndef max
    //! Utility template function returning greater of the two values. Provided here to avoid including not strict safe <algorithm>.
    /** Provided here to avoid including not strict safe <algorithm>.\n
        In case operands cause sined/unsigned or size mismatch warnings it is caller's
        responsibility to do the appropriate cast before calling the function. **/
    template<typename T1, typename T2>
    const T1& max ( const T1& val1, const T2& val2 ) {
        return val1 < val2 ? val2 : val1;
    }
#endif /* !max */
