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

#include "tbb/pipeline.h"
#include "tbb/spin_mutex.h"
#include <cstdlib>
#include <cstdio>
#include "harness.h"

struct Buffer {
    int id;
    //! True if Buffer is in use.
    bool is_busy;
    Buffer() : id(-1), is_busy(false) {}
};

class waiting_probe {
    size_t check_counter;
public:
    waiting_probe() : check_counter(0) {}
    bool required( ) {
        ++check_counter;
        return !((check_counter+1)&size_t(0x7FFF));
    }
    void probe( ); // defined below
};

static size_t InputCounter;
static const size_t MaxStreamSize = 1<<12;
//! Maximum number of filters allowed
static const int MaxFilters = 5;
static size_t StreamSize;
static const size_t MaxBuffer = 8;
static bool Done[MaxFilters][MaxStreamSize];
static waiting_probe WaitTest;

class MyFilter: public tbb::filter {
    bool* const my_done;
    const bool my_is_last;  
    const bool input_was_ordered;
public:
    MyFilter( bool is_ordered, bool done[], bool is_last, bool first_stage_is_ordered ) : 
        filter(is_ordered? tbb::filter::serial : tbb::filter::parallel) ,
        my_done(done),
        my_is_last(is_last),
        input_was_ordered(first_stage_is_ordered)
    {}
    /*override*/void* operator()( void* item ) {
        Buffer& b = *static_cast<Buffer*>(item);
        ASSERT( 0<=b.id && size_t(b.id)<StreamSize, NULL );
        ASSERT( !my_done[b.id], "duplicate processing of token?" );
        // The below assertion is no more kept if the first stage was parallel
        if( input_was_ordered )
            ASSERT( !is_serial() || b.id==0 || my_done[b.id-1], NULL );
        ASSERT( b.is_busy, NULL );
        my_done[b.id] = true;
        if( my_is_last ) {
            b.is_busy = false;
        }
        return item;
    }
};

class MyInput: public tbb::filter {
    const bool my_is_last;
    const size_t my_number_of_tokens;
    Buffer buffer[MaxBuffer];
    tbb::spin_mutex input_lock;
    bool my_is_running;
    /*override*/void* operator()( void* );
public:
    MyInput( bool is_ordered, size_t ntokens, bool is_last ) : 
        tbb::filter(is_ordered),
        my_is_last(is_last),
        my_number_of_tokens(ntokens),
        my_is_running(false)
    {
        ASSERT( my_number_of_tokens<=MaxBuffer, NULL );
    }
};

void* MyInput::operator()(void*) {
    size_t next_input, free_buffer = 0;
    if( is_serial() )
        ASSERT( !my_is_running, "premature entry to serial input stage" );
    my_is_running = true;
    { // lock protected scope
        tbb::spin_mutex::scoped_lock lock(input_lock);
        if( InputCounter>=StreamSize ) {
            my_is_running = false;
            return NULL;
        }
        next_input = InputCounter++;
        // once in a while, emulate waiting for input; this only makes sense for serial input
        if( is_serial() && WaitTest.required() )
            WaitTest.probe( );
        while( free_buffer<MaxBuffer )
            if( buffer[free_buffer].is_busy )
                ++free_buffer;
            else {
                buffer[free_buffer].is_busy = true;
                break;
            }
    }
    ASSERT( free_buffer<my_number_of_tokens, "premature reuse of buffer" );
    Buffer& b = buffer[free_buffer];
    ASSERT( &buffer[0] <= &b, NULL );
    ASSERT( &b <= &buffer[MaxBuffer-1], NULL ); 
    b.id = int(next_input);
    b.is_busy = !my_is_last;
    my_is_running = false;
    return &b;
}

//! The struct below repeats layout of tbb::pipeline.
struct hacked_pipeline {
    tbb::filter* filter_list;
    tbb::filter* filter_end;
    tbb::empty_task* end_counter;
    tbb::internal::Token input_tokens;
    tbb::internal::Token token_counter;
    bool end_of_input;

    virtual ~hacked_pipeline();
};

//! The struct below repeats layout of tbb::internal::ordered_buffer.
struct hacked_ordered_buffer {
    tbb::task** array;
    tbb::internal::Token array_size;
    tbb::internal::Token low_token;
    tbb::spin_mutex array_mutex;
};

//! The struct below repeats layout of tbb::filter.
struct hacked_filter {
    tbb::filter* next_filter_in_pipeline;
    hacked_ordered_buffer* input_buffer;
    unsigned char my_filter_mode;
    tbb::filter* prev_filter_in_pipeline;
    tbb::pipeline* my_pipeline;

    virtual ~hacked_filter();
};

const tbb::internal::Token tokens_before_wraparound = 0xF;

void TestTrivialPipeline( size_t nthread, int number_of_filters ) {
    if( Verbose ) 
        printf("testing with %d threads and %d filters\n", int(nthread), number_of_filters );
    ASSERT( number_of_filters<=MaxFilters, "too many filters" );
    ASSERT( sizeof(hacked_pipeline) == sizeof(tbb::pipeline), "layout changed for tbb::pipeline?" );
    ASSERT( sizeof(hacked_filter) == sizeof(tbb::filter), "layout changed for tbb::filter?" );
    size_t ntokens = nthread<MaxBuffer ? nthread : MaxBuffer;
    // Iterate over possible ordered/unordered filter sequences
    for( int bitmask=0; bitmask<1<<number_of_filters; ++bitmask ) {
        // Build pipeline
        tbb::pipeline pipeline;
        // A private member of pipeline is hacked there for sake of testing wrap-around immunity.
        ((hacked_pipeline*)(void*)&pipeline)->token_counter = ~tokens_before_wraparound;
        tbb::filter* filter[MaxFilters];
        bool first_stage_is_ordered = false;
        for( int i=0; i<number_of_filters; ++i ) {
            const bool is_ordered = bitmask>>i&1;
            const bool is_last = i==number_of_filters-1;
            if( i==0 ) {
                filter[i] = new MyInput(is_ordered,ntokens,is_last);
                first_stage_is_ordered = is_ordered;
            }
            else
                filter[i] = new MyFilter(is_ordered,Done[i],is_last,first_stage_is_ordered);
            pipeline.add_filter(*filter[i]);
            // The ordered buffer of serial filters is hacked as well.
            if (is_ordered)
                ((hacked_filter*)(void*)filter[i])->input_buffer->low_token = ~tokens_before_wraparound;
        }
        for( StreamSize=0; StreamSize<=MaxStreamSize; StreamSize += StreamSize/3+1 ) {
            memset( Done, 0, sizeof(Done) );
            InputCounter = 0;

            pipeline.run( ntokens );

            if( number_of_filters>0 ) 
                ASSERT( InputCounter==StreamSize, NULL );
            for( int i=1; i<MaxFilters; ++i )
                for( size_t j=0; j<StreamSize; ++j ) {
                    ASSERT( Done[i][j]==(i<number_of_filters), NULL );
                }
        }
        pipeline.clear();
        for( int i=number_of_filters; --i>=0; ) {
            delete filter[i];
            filter[i] = NULL;
        }
    }
}

#include "tbb/tick_count.h"
#include "tbb/tbb_machine.h"
#include "harness_cpu.h"

static int nthread; // knowing number of threads is necessary to call TestCPUUserTime

void waiting_probe::probe( ) {
    if( nthread==1 ) return;
    if( Verbose ) printf("emulating wait for input\n");
    // Test that threads sleep while no work.
    // The master doesn't sleep so there could be 2 active threads if a worker is waiting for input
    TestCPUUserTime(nthread, 2);
}

#include "tbb/task_scheduler_init.h"

int main( int argc, char* argv[] ) {
    // Default is at least one thread.
    MinThread = 1;
    ParseCommandLine(argc,argv);
    if( MinThread<1 ) {
        printf("must have at least one thread");
        exit(1);
    }

    // Test with varying number of threads.
    for( nthread=MinThread; nthread<=MaxThread; ++nthread ) {
        // Initialize TBB task scheduler
        tbb::task_scheduler_init init(nthread);

        // Test pipelines with n filters
        for( int n=0; n<=5; ++n )
            TestTrivialPipeline(size_t(nthread),n);

        // Test that all workers sleep when no work
        TestCPUUserTime(nthread);
    }
    printf("done\n");
    return 0;
}
