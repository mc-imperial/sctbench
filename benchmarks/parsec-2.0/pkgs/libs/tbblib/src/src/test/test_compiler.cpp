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

#include <stdio.h>

union char2bool {
    unsigned char c;
    volatile bool b;
} u;

#include "harness.h"

// The function proves the compiler uses 0 or 1 to store a bool. It
// inspects what a compiler does when it loads a bool.  A compiler that
// uses a value other than 0 or 1 to represent a bool will have to normalize
// the value to 0 or 1 when the bool is cast to an unsigned char.
// Compilers that pass this test do not do the normalization, and thus must
// be assuming that a bool is a 0 or 1.
int test_bool_representation() {
    for( unsigned i=0; i<256; ++i ) {
        u.c = (unsigned char)i;
        unsigned char x = (unsigned char)u.b;
        if( x != i ) {
            if( Verbose )
                fprintf(stderr, "Test failed at %d iteration\n",i);
            return 1;
        }
    }
    return 0;
}

int main( int argc, char* argv[] ) {
    ParseCommandLine(argc, argv);
    if( test_bool_representation()!=0 )
        fprintf(stderr, "ERROR: bool representation test failed\n");
    else
        printf("done\n");
    return 0;
}
