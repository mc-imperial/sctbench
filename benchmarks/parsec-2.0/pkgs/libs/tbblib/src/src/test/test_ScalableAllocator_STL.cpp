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

// Test whether scalable_allocator works with some of the host's STL containers.

/* to avoid dependency on TBB shared library, the following macro should be
   defined to non-zero _before_ including any TBB or test harness headers.
   Also tbb_assert_impl.h from src/tbb should be included right after */
#define __TBB_NO_IMPLICIT_LINKAGE 1
#include "../tbb/tbb_assert_impl.h"

#define HARNESS_NO_PARSE_COMMAND_LINE 1
#include "tbb/scalable_allocator.h"

// the actual body of the test is there:
#include "test/test_allocator.h"

#include <vector>
#include <list>
#include <deque>

int main() {
    TestContainer<std::vector<int,tbb::scalable_allocator<int> > >();
    TestContainer<std::list<int,tbb::scalable_allocator<int> > >();
    TestContainer<std::deque<int,tbb::scalable_allocator<int> > >();
    printf("done\n");
    return 0;
}
