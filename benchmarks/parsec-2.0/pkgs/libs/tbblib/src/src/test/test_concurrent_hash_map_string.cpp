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

// Test for problem where Microsoft VC8 compiler could not compile 
// concurrent_hash_map that uses a FastString, as defined below.  
// The root problem was a missing "const" in the copy constructor for scalable_allocator.
// Unfortunately, I (Arch Robison) was not able to reduce the test to
// something that did not involve concurrent_hash_map, so this test
// is regrettably large.  The problem did not manifest itself when
// the Intel or GNU compilers were used.

#include "tbb/concurrent_hash_map.h"
#include "tbb/scalable_allocator.h"
#include "harness_assert.h"
#include <string>

typedef std::basic_string<char,std::char_traits<char>,tbb::scalable_allocator<char> > FastString;

struct FastStringHashCompare {
    static size_t hash( const FastString& x ) {
        size_t h = 0;
        for( const char* s = x.c_str(); *s; s++ )
            h = (h*17)^*s;
        return h;
    }
    //! True if FastStrings are equal
    static bool equal( const FastString& x, const FastString& y ) {
        return x==y;
    }
};

void TestStringKey() {
    typedef tbb::concurrent_hash_map<FastString,int,FastStringHashCompare> table_type;
    table_type table;
    static const FastString fruit[] = {"apple", "banana", "cherry"};
    for( int i=0; i<12; ++i ) {
        table_type::accessor a;
        table.insert( a, fruit[i%3] );
        a->second += 1;
    }
    table_type::iterator j=table.begin();
    for( int i=0; i<3; ++i, ++j ) {
        ASSERT( j->second==4, NULL );
    }
    ASSERT( j==table.end(), NULL );
    
}

#include "harness.h"

//! Test driver
int main( int argc, char* argv[] ) {

    // Do test with string
    TestStringKey();

    printf("done\n");
    return 0;
}
