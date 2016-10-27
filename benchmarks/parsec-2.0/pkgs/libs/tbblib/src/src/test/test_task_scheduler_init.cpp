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

#include "tbb/task_scheduler_init.h"
#include <cstdlib>

//! Test that task::initialize and task::terminate work when doing nothing else.
/** maxthread is treated as the "maximum" number of worker threads. */
void InitializeAndTerminate( int maxthread ) {
    for( int i=0; i<200; ++i ) {
        switch( i&3 ) {
            default: {
                tbb::task_scheduler_init init( std::rand() % maxthread + 1 );
                break;
            }
            case 0: {   
                tbb::task_scheduler_init init;
                break;
            }
            case 1: {
                tbb::task_scheduler_init init( tbb::task_scheduler_init::automatic );
                break;
            }
            case 2: {
                tbb::task_scheduler_init init( tbb::task_scheduler_init::deferred );
                init.initialize( std::rand() % maxthread + 1 );
                init.terminate();
                break;
            }
        }
    }
}

#include <cstdio>
#include <stdexcept>
#include "harness.h"
#include "tbb/blocked_range.h"

#if _WIN64
namespace std {      // 64-bit Windows compilers have not caught up with 1998 ISO C++ standard
    using ::srand;
    using ::printf;
}
#endif /* _WIN64 */

struct ThreadedInit {
    void operator()( const tbb::blocked_range<long>& r ) const {
        try {
            InitializeAndTerminate(MaxThread);
        } catch( std::runtime_error& error ) {
            std::printf("ERROR: %s\n", error.what() );
        }
    }
};

#if _MSC_VER
#include <windows.h>
#include <tchar.h>
#endif /* _MSC_VER */

//! Test driver
int main(int argc, char* argv[]) {
#if _MSC_VER && !__TBB_NO_IMPLICIT_LINKAGE
    #ifdef _DEBUG
        ASSERT(!GetModuleHandle(_T("tbb.dll")) && GetModuleHandle(_T("tbb_debug.dll")),
            "debug application links with non-debug tbb library");
    #else
        ASSERT(!GetModuleHandle(_T("tbb_debug.dll")) && GetModuleHandle(_T("tbb.dll")),
            "non-debug application links with debug tbb library");
    #endif
#endif /* _MSC_VER && !__TBB_NO_IMPLICIT_LINKAGE */
    std::srand(2);
    // Set defaults
    MaxThread = MinThread = 2;
    ParseCommandLine( argc, argv );
    try {
        InitializeAndTerminate(MaxThread);
    } catch( std::runtime_error& error ) {
        std::printf("ERROR: %s\n", error.what() );
    }
    for( int p=MinThread; p<=MaxThread; ++p ) {
        if( Verbose ) printf("testing with %d threads\n", p );
        NativeParallelFor( tbb::blocked_range<long>(0,p,1), ThreadedInit() );
    }
    std::printf("done\n");
    return 0;
}
