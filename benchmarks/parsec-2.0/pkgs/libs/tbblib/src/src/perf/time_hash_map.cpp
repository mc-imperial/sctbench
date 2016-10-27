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

// configuration:

//! enable/disable std::map tests
#define STDTABLE 1

//! enable/disable old implementation tests (correct include file also)
#define OLDTABLE 0
#define OLDTABLEHEADER "tbb/concurrent_hash_map-4078.h"//-4329

//! enable/disable experimental implementation tests (correct include file also)
#define TESTTABLE 0
#define TESTTABLEHEADER "tbb/concurrent_hash_map-4148.h"

//////////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <math.h>
#include "tbb/tbb_stddef.h"
#include <vector>
#include <map>
// needed by hash_maps
#include <stdexcept>
#include <iterator>
#include <algorithm>                 // std::swap
#include <utility>      // Need std::pair from here
#include "tbb/cache_aligned_allocator.h"
#include "tbb/tbb_allocator.h"
#include "tbb/spin_rw_mutex.h"
#include "tbb/aligned_space.h"
#include "tbb/atomic.h"
#include "../tbb/tbb_misc.h"  // tbb::internal::ExponentialBackoff
// for test
#include "tbb/spin_mutex.h"
#include "time_framework.h"

using namespace tbb;
using namespace tbb::internal;

struct IntHashCompare {
    size_t operator() ( int x ) const { return x; }
    static long hash( int x ) { return x; }
    bool equal( int x, int y ) const { return x==y; }
};

namespace version_current {
    namespace tbb { using namespace ::tbb; namespace internal { using namespace ::tbb::internal; } }
    #include "tbb/concurrent_hash_map.h"
}
typedef version_current::tbb::concurrent_hash_map<int,int,IntHashCompare> IntTable;

#if OLDTABLE
#undef __TBB_concurrent_hash_map_H
namespace version_base {
    namespace tbb { using namespace ::tbb; namespace internal { using namespace ::tbb::internal; } }
    #include OLDTABLEHEADER
}
typedef version_base::tbb::concurrent_hash_map<int,int,IntHashCompare> OldTable;
#endif

#if TESTTABLE
#undef __TBB_concurrent_hash_map_H
namespace version_new {
    namespace tbb { using namespace ::tbb; namespace internal { using namespace ::tbb::internal; } }
    #include TESTTABLEHEADER
}
typedef version_new::tbb::concurrent_hash_map<int,int,IntHashCompare> TestTable;
#define TESTTABLE 1
#endif

///////////////////////////////////////

static const char *map_testnames[] = {
    "fill", "work", "clean"
};

template<typename TableType>
struct TestTBBMap : TesterBase {
    typedef typename TableType::accessor accessor;
    typedef typename TableType::const_accessor const_accessor;
    TableType Table;
    int n_items;

    TestTBBMap() : TesterBase(3) {}
    void init() { n_items = value/threads_count; }

    std::string get_name(int testn) {
        return std::string(map_testnames[testn]);
    }

    double test(int test, int t)
    {
        switch(test) {
          case 0: // fill
            for(int i = t*n_items, e = (t+1)*n_items; i < e; i++) {
                accessor a;
                Table.insert( a, i );
                a->second = 0;
            }
            break;
          case 1: // work
            for(int i = t*n_items, e = (t+1)*n_items; i < e; i++) {
                accessor a;
                Table.find( a, i );
                a->second += 1;
            }
            break;
          case 2: // clean
            for(int i = t*n_items, e = (t+1)*n_items; i < e; i++) {
                Table.erase( i );
            }
        }
        return 0;
    }
};

template<typename M>
struct TestSTLMap : TesterBase {
    std::map<int, int> Table;
    M mutex;

    int n_items;
    TestSTLMap() : TesterBase(3) {}
    void init() { n_items = value/threads_count; }

    std::string get_name(int testn) {
        return std::string(map_testnames[testn]);
    }

    double test(int test, int t)
    {
        switch(test) {
          case 0: // fill
            for(int i = t*n_items, e = (t+1)*n_items; i < e; i++) {
                typename M::scoped_lock with(mutex);
                Table[i] = 0;
            }
            break;
          case 1: // work
            for(int i = t*n_items, e = (t+1)*n_items; i < e; i++) {
                typename M::scoped_lock with(mutex);
                Table[i] += 1;
            }
            break;
          case 2: // clean
            for(int i = t*n_items, e = (t+1)*n_items; i < e; i++) {
                typename M::scoped_lock with(mutex);
                Table.erase(i);
            }
        }
        return 0;
    }
};

class fake_mutex {
    int a;
public:
    class scoped_lock {
        fake_mutex *p;

    public:
        scoped_lock() {}
        scoped_lock( fake_mutex &m ) { p = &m; }
        ~scoped_lock() { p->a = 0; }
        void acquire( fake_mutex &m ) { p = &m; }
        void release() { }
    };
};

class test_hash_map : public TestProcessor {
public:
    test_hash_map() : TestProcessor("test_hash_map") {}
    void factory(int value, int threads) {
        if(Verbose) printf("Processing with %d threads: %d...\n", threads, value);
        process( value, threads,
#if STDTABLE
            run("std::map ", new NanosecPerValue<TestSTLMap<spin_mutex> >() ),
#endif
#if OLDTABLE
            run("old::hmap", new NanosecPerValue<TestTBBMap<OldTable> >() ),
#endif
            run("tbb::hmap", new NanosecPerValue<TestTBBMap<IntTable> >() ),
#if TESTTABLE
            run("new::hmap", new NanosecPerValue<TestTBBMap<TestTable> >() ),
#endif
        end );
        //stat->Print(StatisticsCollector::Stdout);
        if(value >= 2097152) stat->Print(StatisticsCollector::HTMLFile);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////
template<typename TableType>
struct TestHashMapFind : TesterBase {
    typedef typename TableType::accessor accessor;
    typedef typename TableType::const_accessor const_accessor;
    TableType Table;
    int n_items;

    std::string get_name(int testn) {
        return std::string(!testn?"find()":"insert()");
    }

    TestHashMapFind() : TesterBase(2) {}
    void init() {
        n_items = value/threads_count;
        for(int i = 0; i < value; i++) {
            accessor a; Table.insert( a, i );
        }
    }

    double test(int test, int t)
    {
        switch(test) {
          case 0: // find
            for(int i = t*n_items, e = (t+1)*n_items; i < e; i++) {
                accessor a;
                Table.find( a, i );
                a->second = i;
            }
            break;
          case 1: // insert
            for(int i = t*n_items, e = (t+1)*n_items; i < e; i++) {
                accessor a;
                Table.insert( a, i );
                a->second = -i;
            }
            break;
        }
        return 0;
    }
};

const int test2_size = 65536;
int Data[test2_size];

template<typename TableType>
struct TestHashCountStrings : TesterBase {
    typedef typename TableType::accessor accessor;
    typedef typename TableType::const_accessor const_accessor;
    TableType Table;
    int n_items;

    std::string get_name(int testn) {
        return !testn?"example":"just find";
    }

    TestHashCountStrings() : TesterBase(2) {}
    void init() {
        n_items = value/threads_count;
    }

    double test(int testn, int t)
    {
        if(!testn) {
            for(int i = t*n_items, e = (t+1)*n_items; i < e; i++) {
                accessor a; Table.insert( a, Data[i] );
            }
        } else { // 
            for(int i = t*n_items, e = (t+1)*n_items; i < e; i++) {
                accessor a; Table.find( a, Data[i] );
            }
        }
        return 0;
    }
};

class test_hash_map_find : public TestProcessor {
public:
    test_hash_map_find() : TestProcessor("test_hash_map_find") {}
    void factory(int value, int threads) {
        if(Verbose) printf("Processing with %d threads: %d...\n", threads, value);
        process( value, threads,
#if OLDTABLE
            run("old::hashmap", new NanosecPerValue<TestHashMapFind<OldTable> >() ),
#endif
            run("tbb::hashmap", new NanosecPerValue<TestHashMapFind<IntTable> >() ),
#if OLDTABLE
            run("old::countstr", new TimeTest<TestHashCountStrings<OldTable> >() ),
#endif
            run("tbb::countstr", new TimeTest<TestHashCountStrings<IntTable> >() ),
#if TESTTABLE
            run("new::countstr", new TimeTest<TestHashCountStrings<TestTable> >() ),
#endif
        end );
        //stat->Print(StatisticsCollector::HTMLFile);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
    if(argc>1) Verbose = true;
    //if(argc>2) ExtraVerbose = true;
    MinThread = 1; MaxThread = 8;
    ParseCommandLine( argc, argv );

    ASSERT(tbb_allocator<int>::allocator_type() == tbb_allocator<int>::scalable, "expecting scalable allocator library to be loaded. Please build it by:\n\t\tmake tbbmalloc");

    {
        test_hash_map_find test_find; int o = test2_size;
        for(int i = 0; i < o; i++)
            Data[i] = i%60;
        for( int t=MinThread; t <= MaxThread; t++)
            test_find.factory(o, t);
        test_find.report.SetTitle("Nanoseconds per operation of finding operation (Mode) for %d items", o);
        test_find.report.Print(StatisticsCollector::HTMLFile|StatisticsCollector::ExcelXML);
    }
    {
        test_hash_map the_test;
        for( int t=MinThread; t <= MaxThread; t*=2)
            for( int o=/*4096*/(1<<8)*8*2; o<2200000; o*=2 )
                the_test.factory(o, t);
        the_test.report.SetTitle("Nanoseconds per operation of (Mode) for N items in container (Name)");
        the_test.report.SetStatisticFormula("1AVG per size", "=AVERAGE(ROUNDS)");
        the_test.report.Print(StatisticsCollector::HTMLFile|StatisticsCollector::ExcelXML);
    }
    return 0;
}

