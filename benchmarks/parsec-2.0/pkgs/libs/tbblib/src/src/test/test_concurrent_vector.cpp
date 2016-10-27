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

#include "tbb/concurrent_vector.h"
#include "tbb/cache_aligned_allocator.h"
#include "tbb/tbb_exception.h"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include "harness_assert.h"
#include "harness_allocator.h"

static bool known_issue_verbose = false;
#define KNOWN_ISSUE(msg) if(!known_issue_verbose) known_issue_verbose = true, printf(msg)

tbb::atomic<long> FooCount;
long MaxFooCount = 0;

//! Problem size
const size_t N = 500000;

//! Exception for concurrent_vector
class Foo_exception : public std::bad_alloc {
public:
    virtual const char *what() const throw() { return "out of Foo limit"; }
    virtual ~Foo_exception() throw() {}
};

struct Foo {
    int my_bar;
public:
    enum State {
        ZeroInitialized=0,
        DefaultInitialized=0x1234,
        CopyInitialized=0x8765,
        Destroyed=0x5678
    } state;
    int& zero_bar() {
        ASSERT( state==DefaultInitialized||state==CopyInitialized||state==ZeroInitialized, NULL );
        return my_bar;
    }
    int& bar() {
        ASSERT( state==DefaultInitialized||state==CopyInitialized, NULL );
        return my_bar;
    }
    int bar() const {
        ASSERT( state==DefaultInitialized||state==CopyInitialized, NULL );
        return my_bar;
    }
    static const int initial_value_of_bar = 42;
    Foo() {
        state = DefaultInitialized;
        if(MaxFooCount && FooCount >= MaxFooCount)
            throw Foo_exception();
        ++FooCount;
        my_bar = initial_value_of_bar;
    }
    Foo( const Foo& foo ) {
        state = CopyInitialized;
        if(MaxFooCount && FooCount >= MaxFooCount)
            throw Foo_exception();
        ++FooCount;
        my_bar = foo.my_bar;
    }
    ~Foo() {
        ASSERT( state==DefaultInitialized||state==CopyInitialized||(state==ZeroInitialized && !my_bar), NULL );
        state = Destroyed;
        my_bar = ~initial_value_of_bar;
        --FooCount;
    }
    bool operator==(const Foo &f) const { return my_bar == f.my_bar; }
    bool operator<(const Foo &f) const { return my_bar < f.my_bar; }
    bool is_const() const {return true;}
    bool is_const() {return false;}
private:
    char reserve[1];
};

class FooWithAssign: public Foo {
public:
    void operator=( const FooWithAssign& x ) {
        ASSERT( x.state==DefaultInitialized||x.state==CopyInitialized, NULL );
        ASSERT( state==DefaultInitialized||state==CopyInitialized, NULL );
        my_bar = x.my_bar;
    } 
};

class FooIterator: public std::iterator<std::input_iterator_tag,FooWithAssign> {
    int x_bar;
public:
    FooIterator(int x) {
        x_bar = x;
    }
    FooIterator &operator++() {
        x_bar++; return *this;
    }
    FooWithAssign operator*() {
        FooWithAssign foo; foo.bar() = x_bar;
        return foo;
    }
    bool operator!=(const FooIterator &i) { return x_bar != i.x_bar; }
};

inline void NextSize( int& s ) {
    if( s<=32 ) ++s;
    else s += s/10;
}

//! Check vector have expected size and filling
template<typename vector_t>
static void CheckVector( const vector_t& cv, size_t expected_size, size_t old_size ) {
    ASSERT( cv.size()==expected_size, NULL );
    ASSERT( cv.empty()==(expected_size==0), NULL );
    for( int j=0; j<int(expected_size); ++j ) {
        if( cv[j].bar()!=~j )
            std::printf("ERROR on line %d for old_size=%ld expected_size=%ld j=%d\n",__LINE__,long(old_size),long(expected_size),j);
    }
}

//! Test of assign, grow, copying with various sizes
void TestResizeAndCopy() {
	typedef static_counting_allocator<std::allocator<Foo>, std::size_t> allocator_t;
    typedef tbb::concurrent_vector<Foo, allocator_t> vector_t;
    allocator_t::init_counters();
    for( int old_size=0; old_size<=128; NextSize( old_size ) ) {
        for( int new_size=old_size; new_size<=1280; NextSize( new_size ) ) {
            long count = FooCount;
            vector_t v;
            ASSERT( count==FooCount, NULL );
            v.assign(old_size/2, Foo() );
            ASSERT( count+old_size/2==FooCount, NULL );
            for( int j=0; j<old_size/2; ++j )
                ASSERT( v[j].state == Foo::CopyInitialized, NULL);
            v.assign(FooIterator(0), FooIterator(old_size));
            v.grow_to_at_least(new_size);
            ASSERT( count+new_size==FooCount, NULL );
            for( int j=0; j<new_size; ++j ) {
                int expected = j<old_size ? j : Foo::initial_value_of_bar;
                if( v[j].bar()!=expected ) 
                    std::printf("ERROR on line %d for old_size=%ld new_size=%ld v[%ld].bar()=%d != %d\n",__LINE__,long(old_size),long(new_size),long(j),v[j].bar(), expected);
            }
            ASSERT( v.size()==size_t(new_size), NULL );
            for( int j=0; j<new_size; ++j ) {
                v[j].bar() = ~j;
            }
            const vector_t& cv = v;
            // Try copy constructor
            vector_t copy_of_v(cv);
            CheckVector(cv,new_size,old_size);
            ASSERT( !(v != copy_of_v), NULL );
            v.clear();
            ASSERT( v.empty(), NULL );
            swap(v, copy_of_v);
            ASSERT( copy_of_v.empty(), NULL );
            CheckVector(v,new_size,old_size);
        }
    }
    ASSERT( allocator_t::items_allocated == allocator_t::items_freed, NULL);
    ASSERT( allocator_t::allocations == allocator_t::frees, NULL);
}

//! Test reserve, compact, capacity
void TestCapacity() {
    typedef static_counting_allocator<tbb::cache_aligned_allocator<Foo>, std::size_t> allocator_t;
    typedef tbb::concurrent_vector<Foo, allocator_t> vector_t;
    allocator_t::init_counters();
    for( size_t old_size=0; old_size<=11000; old_size=(old_size<5 ? old_size+1 : 3*old_size) ) {
        for( size_t new_size=0; new_size<=11000; new_size=(new_size<5 ? new_size+1 : 3*new_size) ) {
            long count = FooCount; 
            {
                vector_t v; v.reserve(old_size);
                ASSERT( v.capacity()>=old_size, NULL );
                v.reserve( new_size );
                ASSERT( v.capacity()>=old_size, NULL );
                ASSERT( v.capacity()>=new_size, NULL );
                ASSERT( v.empty(), NULL );
                for( size_t i=0; i<2*new_size; ++i ) {
                    ASSERT( size_t(FooCount)==count+i, NULL );
                    size_t j = v.grow_by(1);
                    ASSERT( j==i, NULL );
                    v[j].bar() = int(~j);
                }
                vector_t copy_of_v(v); // should allocate first segment with same size as for compact()
                v.compact(); // TODO: how to check if optimization was performed correctly
                CheckVector(v, new_size*2, old_size); // check vector correctness
                ASSERT( v==copy_of_v, NULL ); // TODO: check also segments layout equality
            }
            ASSERT( FooCount==count, NULL );
        }
    } 
    ASSERT( allocator_t::items_allocated == allocator_t::items_freed, NULL);
    ASSERT( allocator_t::allocations == allocator_t::frees, NULL);
}

struct AssignElement {
    typedef tbb::concurrent_vector<int>::range_type::iterator iterator;
    iterator base;
    void operator()( const tbb::concurrent_vector<int>::range_type& range ) const {
        for( iterator i=range.begin(); i!=range.end(); ++i ) {
            if( *i!=0 )
                std::printf("ERROR for v[%ld]\n", long(i-base));
            *i = int(i-base);
        }
    }
    AssignElement( iterator base_ ) : base(base_) {}
};

struct CheckElement {
    typedef tbb::concurrent_vector<int>::const_range_type::iterator iterator;
    iterator base;
    void operator()( const tbb::concurrent_vector<int>::const_range_type& range ) const {
        for( iterator i=range.begin(); i!=range.end(); ++i )
            if( *i != int(i-base) )
                std::printf("ERROR for v[%ld]\n", long(i-base));
    }
    CheckElement( iterator base_ ) : base(base_) {}
};

#include "tbb/tick_count.h"
#include "tbb/parallel_for.h"
#include "harness.h"

//! Test parallel access by iterators
void TestParallelFor( int nthread ) {
    typedef tbb::concurrent_vector<int> vector_t;
    vector_t v;
    v.grow_to_at_least(N);  
    tbb::tick_count t0 = tbb::tick_count::now();
    if( Verbose )
        std::printf("Calling parallel_for with %ld threads\n",long(nthread));
    tbb::parallel_for( v.range(10000), AssignElement(v.begin()) );
    tbb::tick_count t1 = tbb::tick_count::now();
    const vector_t& u = v;      
    tbb::parallel_for( u.range(10000), CheckElement(u.begin()) );
    tbb::tick_count t2 = tbb::tick_count::now();
    if( Verbose )
        std::printf("Time for parallel_for: assign time = %8.5f, check time = %8.5f\n",
               (t1-t0).seconds(),(t2-t1).seconds());
    for( long i=0; size_t(i)<v.size(); ++i )
        if( v[i]!=i )
            std::printf("ERROR for v[%ld]\n", i);
}

template<typename Iterator1, typename Iterator2>
void TestIteratorAssignment( Iterator2 j ) {
    Iterator1 i(j);
    ASSERT( i==j, NULL );
    ASSERT( !(i!=j), NULL );
    Iterator1 k;
    k = j;
    ASSERT( k==j, NULL );
    ASSERT( !(k!=j), NULL );
}

template<typename Range1, typename Range2>
void TestRangeAssignment( Range2 r2 ) {
    Range1 r1(r2); r1 = r2;
}

template<typename Iterator, typename T>
void TestIteratorTraits() {
    AssertSameType( static_cast<typename Iterator::difference_type*>(0), static_cast<ptrdiff_t*>(0) ); 
    AssertSameType( static_cast<typename Iterator::value_type*>(0), static_cast<T*>(0) ); 
    AssertSameType( static_cast<typename Iterator::pointer*>(0), static_cast<T**>(0) ); 
    AssertSameType( static_cast<typename Iterator::iterator_category*>(0), static_cast<std::random_access_iterator_tag*>(0) );
    T x;
    typename Iterator::reference xr = x;
    typename Iterator::pointer xp = &x;
    ASSERT( &xr==xp, NULL );
}

template<typename Vector, typename Iterator>
void CheckConstIterator( const Vector& u, int i, const Iterator& cp ) {
    typename Vector::const_reference pref = *cp;
    if( pref.bar()!=i )
        std::printf("ERROR for u[%ld] using const_iterator\n", long(i));
    typename Vector::difference_type delta = cp-u.begin();
    ASSERT( delta==i, NULL );
    if( u[i].bar()!=i )
        std::printf("ERROR for u[%ld] using subscripting\n", long(i));
    ASSERT( u.begin()[i].bar()==i, NULL );
}

template<typename Iterator1, typename Iterator2, typename V> 
void CheckIteratorComparison( V& u ) {
    Iterator1 i = u.begin();
    for( int i_count=0; i_count<100; ++i_count ) {
        Iterator2 j = u.begin();
        for( int j_count=0; j_count<100; ++j_count ) {
            ASSERT( (i==j)==(i_count==j_count), NULL );
            ASSERT( (i!=j)==(i_count!=j_count), NULL );
            ASSERT( (i-j)==(i_count-j_count), NULL );
            ASSERT( (i<j)==(i_count<j_count), NULL );
            ASSERT( (i>j)==(i_count>j_count), NULL );
            ASSERT( (i<=j)==(i_count<=j_count), NULL );
            ASSERT( (i>=j)==(i_count>=j_count), NULL );
            ++j;
        }
        ++i;
    }
}

//! Test sequential iterators for vector type V.
/** Also does timing. */
template<typename T>
void TestSequentialFor() {
    typedef tbb::concurrent_vector<Foo> V;
    V v;
    v.grow_by(N);
    ASSERT(v.grow_by(0) == v.grow_by(0, Foo()), NULL);

    // Check iterator 
    tbb::tick_count t0 = tbb::tick_count::now();
    typename V::iterator p = v.begin();
    ASSERT( !(*p).is_const(), NULL );
    ASSERT( !p->is_const(), NULL );
    for( int i=0; size_t(i)<v.size(); ++i, ++p ) {
        if( (*p).state!=Foo::DefaultInitialized )
            std::printf("ERROR for v[%ld]\n", long(i));
        typename V::reference pref = *p;
        pref.bar() = i;
        typename V::difference_type delta = p-v.begin();
        ASSERT( delta==i, NULL );
        ASSERT( -delta<=0, "difference type not signed?" );
    }
    tbb::tick_count t1 = tbb::tick_count::now();
    
    // Check const_iterator going forwards
    const V& u = v;
    typename V::const_iterator cp = u.begin();
    ASSERT( (*cp).is_const(), NULL );
    ASSERT( cp->is_const(), NULL );
    ASSERT( *cp == v.front(), NULL);
    for( int i=0; size_t(i)<u.size(); ++i, ++cp ) {
        CheckConstIterator(u,i,cp);
    }
    tbb::tick_count t2 = tbb::tick_count::now();
    if( Verbose )
        std::printf("Time for serial for:  assign time = %8.5f, check time = %8.5f\n",
               (t1-t0).seconds(),(t2-t1).seconds());

    // Now go backwards
    cp = u.end();
    for( int i=int(u.size()); i>0; ) {
        --i;
        --cp;
        if( i>0 ) {
            typename V::const_iterator cp_old = cp--;
            int here = (*cp_old).bar();
            ASSERT( here==u[i].bar(), NULL );
            typename V::const_iterator cp_new = cp++;
            int prev = (*cp_new).bar();
            ASSERT( prev==u[i-1].bar(), NULL );
        }
        CheckConstIterator(u,i,cp);
    }

    // Now go forwards and backwards
    ptrdiff_t j = 0;
    cp = u.begin();
    for( size_t i=0; i<u.size(); ++i ) {
        CheckConstIterator(u,int(j),cp);
        typename V::difference_type delta = i*3 % u.size();
        if( 0<=j+delta && size_t(j+delta)<u.size() ) {
            cp += delta;
            j += delta; 
        } 
        delta = i*7 % u.size();
        if( 0<=j-delta && size_t(j-delta)<u.size() ) {
            if( i&1 ) 
                cp -= delta;            // Test operator-=
            else
                cp = cp - delta;        // Test operator-
            j -= delta; 
        } 
    }
    
    for( int i=0; size_t(i)<u.size(); i=(i<50?i+1:i*3) )
        for( int j=-i; size_t(i+j)<u.size(); j=(j<50?j+1:j*5) ) {
            ASSERT( (u.begin()+i)[j].bar()==i+j, NULL );
            ASSERT( (v.begin()+i)[j].bar()==i+j, NULL );
            ASSERT( (i+u.begin())[j].bar()==i+j, NULL );
            ASSERT( (i+v.begin())[j].bar()==i+j, NULL );
        }

    CheckIteratorComparison<typename V::iterator, typename V::iterator>(v);
    CheckIteratorComparison<typename V::iterator, typename V::const_iterator>(v);
    CheckIteratorComparison<typename V::const_iterator, typename V::iterator>(v);
    CheckIteratorComparison<typename V::const_iterator, typename V::const_iterator>(v);

    TestIteratorAssignment<typename V::const_iterator>( u.begin() );
    TestIteratorAssignment<typename V::const_iterator>( v.begin() );
    TestIteratorAssignment<typename V::iterator>( v.begin() );
    // doesn't compile as expected: TestIteratorAssignment<typename V::iterator>( u.begin() );

    TestRangeAssignment<typename V::const_range_type>( u.range() );
    TestRangeAssignment<typename V::const_range_type>( v.range() );
    TestRangeAssignment<typename V::range_type>( v.range() );
    // doesn't compile as expected: TestRangeAssignment<typename V::range_type>( u.range() );

    // Check reverse_iterator 
    typename V::reverse_iterator rp = v.rbegin();
    for( size_t i=v.size(); i>0; --i, ++rp ) {
        typename V::reference pref = *rp;
        ASSERT( size_t(pref.bar())==i-1, NULL );
        ASSERT( rp!=v.rend(), NULL );
    }
    ASSERT( rp==v.rend(), NULL );
    
    // Check const_reverse_iterator 
    typename V::const_reverse_iterator crp = u.rbegin();
    ASSERT( *crp == v.back(), NULL);
    for( size_t i=v.size(); i>0; --i, ++crp ) {
        typename V::const_reference cpref = *crp;
        ASSERT( size_t(cpref.bar())==i-1, NULL );
        ASSERT( crp!=u.rend(), NULL );
    }
    ASSERT( crp==u.rend(), NULL );

    TestIteratorAssignment<typename V::const_reverse_iterator>( u.rbegin() );
    TestIteratorAssignment<typename V::reverse_iterator>( v.rbegin() );

    // test compliance with C++ Standard 2003, clause 23.1.1p9
    {
        tbb::concurrent_vector<int> v1, v2(1, 100);
        v1.assign(1, 100); ASSERT(v1 == v2, NULL);
        ASSERT(v1.size() == 1 && v1[0] == 100, "used integral iterators");
    }

    // cross-allocator tests
#if !defined(_WIN64) || defined(_CPPLIB_VER)
    typedef local_counting_allocator<std::allocator<int>, size_t> allocator1_t;
    typedef tbb::cache_aligned_allocator<void> allocator2_t;
    typedef tbb::concurrent_vector<Foo, allocator1_t> V1;
    typedef tbb::concurrent_vector<Foo, allocator2_t> V2;
    V1 v1( v ); // checking cross-allocator copying
    V2 v2( 10 ); v2 = v1; // checking cross-allocator assignment
    ASSERT( (v1 == v) && !(v2 != v), NULL);
    ASSERT( !(v1 < v) && !(v2 > v), NULL);
    ASSERT( (v1 <= v) && (v2 >= v), NULL);
#endif
}

static const size_t Modulus = 7;

typedef static_counting_allocator<tbb::cache_aligned_allocator<Foo> > MyAllocator;
typedef tbb::concurrent_vector<Foo, MyAllocator> MyVector;

class GrowToAtLeast {
    MyVector& my_vector;
public:
    void operator()( const tbb::blocked_range<size_t>& range ) const {
        for( size_t i=range.begin(); i!=range.end(); ++i ) {
            size_t n = my_vector.size();
            size_t k = i % (2*n+1);
            my_vector.grow_to_at_least(k+1);
            ASSERT( my_vector.size()>=k+1, NULL );
        }
    }
    GrowToAtLeast( MyVector& vector ) : my_vector(vector) {}
};

void TestConcurrentGrowToAtLeast() {
    MyAllocator::init_counters();
    MyVector v(2, Foo(), MyAllocator());
    for( size_t s=1; s<1000; s*=10 ) {
        tbb::parallel_for( tbb::blocked_range<size_t>(0,10000*s,s), GrowToAtLeast(v) );
    }
    v.clear();
    size_t items_allocated = v.get_allocator().items_allocated, items_freed = v.get_allocator().items_freed;
    size_t allocations = v.get_allocator().allocations, frees = v.get_allocator().frees;
    ASSERT( items_allocated == items_freed, NULL); ASSERT( allocations == frees, NULL);
}

//! Test concurrent invocations of method concurrent_vector::grow_by
class GrowBy {
    MyVector& my_vector;
public:
    void operator()( const tbb::blocked_range<int>& range ) const {
        for( int i=range.begin(); i!=range.end(); ++i ) {
            if( i&1 ) {
                Foo& element = my_vector[my_vector.grow_by(1)]; 
                element.bar() = i;
            } else {
                Foo f;
                f.bar() = i;
                size_t k;
                if( i&2 )
                    k = my_vector.push_back( f );
                else
                    k = my_vector.grow_by(1, f);
                ASSERT( my_vector[k].bar()==i, NULL );
            }
        }
    }
    GrowBy( MyVector& vector ) : my_vector(vector) {}
};

//! Test concurrent invocations of method concurrent_vector::grow_by
void TestConcurrentGrowBy( int nthread ) {
    MyAllocator::init_counters();
    {
        int m = 100000; MyAllocator a;
        MyVector v( a );
        tbb::parallel_for( tbb::blocked_range<int>(0,m,1000), GrowBy(v) );
        ASSERT( v.size()==size_t(m), NULL );

        // Verify that v is a permutation of 0..m
        int inversions = 0;
        bool* found = new bool[m];
        memset( found, 0, m );
        for( int i=0; i<m; ++i ) {
            int index = v[i].bar();
            ASSERT( v[i].state == (index&1 ? Foo::DefaultInitialized : Foo::CopyInitialized), NULL);
            ASSERT( !found[index], NULL );
            found[index] = true;
            if( i>0 )
                inversions += v[i].bar()<v[i-1].bar();
        }
        for( int i=0; i<m; ++i ) {
            ASSERT( found[i], NULL );
            ASSERT( nthread>1 || v[i].bar()==i, "sequential execution is wrong" );
        }
        delete[] found;
        if( nthread>1 && inversions<m/10 )
            std::printf("WARNING: not much concurrency in TestConcurrentGrowBy\n");
    }
    size_t items_allocated = MyAllocator::items_allocated, items_freed = MyAllocator::items_freed;
    size_t allocations = MyAllocator::allocations, frees = MyAllocator::frees;
    ASSERT( items_allocated == items_freed, NULL); ASSERT( allocations == frees, NULL);
}

//! Test the assignment operator and swap
void TestAssign() {
    typedef tbb::concurrent_vector<FooWithAssign, local_counting_allocator<std::allocator<FooWithAssign>, size_t > > vector_t;
    local_counting_allocator<std::allocator<FooWithAssign>, size_t > init_alloc;
    init_alloc.allocations = 100;
    for( int dst_size=1; dst_size<=128; NextSize( dst_size ) ) {
        for( int src_size=2; src_size<=128; NextSize( src_size ) ) {
            vector_t u(FooIterator(0), FooIterator(src_size), init_alloc);
            for( int i=0; i<src_size; ++i )
                ASSERT( u[i].bar()==i, NULL );
            vector_t v(dst_size, FooWithAssign(), init_alloc);
            for( int i=0; i<dst_size; ++i ) {
                ASSERT( v[i].state==Foo::CopyInitialized, NULL );
                v[i].bar() = ~i;
            }
            ASSERT( v != u, NULL);
            v.swap(u);
            CheckVector(u, dst_size, src_size);
            u.swap(v);
            // using assignment
            v = u;
            ASSERT( v == u, NULL);
            u.clear();
            ASSERT( u.size()==0, NULL );
            ASSERT( v.size()==size_t(src_size), NULL );
            for( int i=0; i<src_size; ++i )
                ASSERT( v[i].bar()==i, NULL );
            size_t items_allocated = u.get_allocator().items_allocated, items_freed = u.get_allocator().items_freed;
            size_t allocations = u.get_allocator().allocations, frees = u.get_allocator().frees + 100;
            ASSERT( items_allocated == items_freed, NULL); ASSERT( allocations == frees, NULL);
            ASSERT( u.get_allocator().allocations == u.get_allocator().frees + 100, NULL);
        }
    }
}

// Test the comparison operators
#include <string>
void TestComparison() {
    std::string str[3]; str[0] = "abc";
    str[1].assign("cba");
    str[2].assign("abc"); // same as 0th
    tbb::concurrent_vector<char> var[3];
    var[0].assign(str[0].begin(), str[0].end());
    var[1].assign(str[0].rbegin(), str[0].rend());
    var[2].assign(var[1].rbegin(), var[1].rend()); // same as 0th
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            ASSERT( (var[i] == var[j]) == (str[i] == str[j]), NULL );
            ASSERT( (var[i] != var[j]) == (str[i] != str[j]), NULL );
            ASSERT( (var[i] < var[j]) == (str[i] < str[j]), NULL );
            ASSERT( (var[i] > var[j]) == (str[i] > str[j]), NULL );
            ASSERT( (var[i] <= var[j]) == (str[i] <= str[j]), NULL );
            ASSERT( (var[i] >= var[j]) == (str[i] >= str[j]), NULL );
        }
    }
}

//------------------------------------------------------------------------
// Regression test for problem where on oversubscription caused
// concurrent_vector::grow_by to run very slowly (TR#196).
//------------------------------------------------------------------------

#include "tbb/task_scheduler_init.h"
#include <math.h>

typedef unsigned long Number;

static tbb::concurrent_vector<Number> Primes;

class FindPrimes {
    bool is_prime( Number val ) const {
        int limit, factor = 3;
        if( val<5u ) 
            return val==2;
        else {
            limit = long(sqrtf(float(val))+0.5f);
            while( factor<=limit && val % factor )
                ++factor;
            return factor>limit;
        }
    }
public:
    void operator()( const tbb::blocked_range<Number>& r ) const {
        for( Number i=r.begin(); i!=r.end(); ++i ) { 
            if( i%2 && is_prime(i) ) {
                Primes[Primes.grow_by(1)] = i;
            }
        }
    }
};

static double TimeFindPrimes( int nthread ) {
    Primes.clear();     // clear behavior has been changed since 2.0
    tbb::task_scheduler_init init(nthread);
    tbb::tick_count t0 = tbb::tick_count::now();
    tbb::parallel_for( tbb::blocked_range<Number>(0,1000000,500), FindPrimes() );
    tbb::tick_count t1 = tbb::tick_count::now();
    return (t1-t0).seconds();
}

static void TestFindPrimes() {
    // Time fully subscribed run.
    double t2 = TimeFindPrimes( tbb::task_scheduler_init::automatic );

    // Time parallel run that is very likely oversubscribed.  
    double t128 = TimeFindPrimes(128);

    if( Verbose ) 
        std::printf("TestFindPrimes: t2==%g t128=%g k=%g\n", t2, t128, t128/t2);

    // We allow the 128-thread run a little extra time to allow for thread overhead.
    // Theoretically, following test will fail on machine with >128 processors.
    // But that situation is not going to come up in the near future,
    // and the generalization to fix the issue is not worth the trouble.
    //
    // [05.09.2007] Anton M has modified coefficient below from value 1.1 to 1.3 due to
    // changes have been made in clear() behavior since 2.0 [U1] version.
    // Originally, clear() kept segments allocated before which led to inaccurate measurment of t128.
    if( t128 > 1.3*t2 ) {
        std::printf("Warning: grow_by is pathetically slow: t2==%g t128=%g k=%g\n", t2, t128, t128/t2);
    } 
}

//------------------------------------------------------------------------
// Test compatibility with STL sort.
//------------------------------------------------------------------------

#include <algorithm>

void TestSort() {
    for( int n=0; n<100; n=n*3+1 ) {
        tbb::concurrent_vector<int> array(n);
        for( int i=0; i<n; ++i )
            array.at(i) = (i*7)%n;
        std::sort( array.begin(), array.end() );
        for( int i=0; i<n; ++i )
            ASSERT( array[i]==i, NULL );
    }
}

//------------------------------------------------------------------------
// Test exceptions safety (from allocator and items constructors)
//------------------------------------------------------------------------
template<typename MyVector>
class GrowthException {
    MyVector& my_vector;
public:
    static volatile bool my_cancel;
    void operator()( const tbb::blocked_range<int>& range ) const {
        if(!my_cancel) for( int i=range.begin(); i!=range.end(); ++i ) {
            try {
                if( i&1 ) {
                    Foo& element = my_vector[my_vector.grow_by(1)]; 
                    element.bar() = i;
                } else {
                    Foo f;
                    f.bar() = i;
                    size_t k;
                    if( i&2 )
                        k = my_vector.push_back( f );
                    else
                        k = my_vector.grow_by(1, f);
                    ASSERT( my_vector[k].bar()==i, NULL );
                }
            } catch(...) {
                my_cancel = true;
            }
        }
    }
    GrowthException( MyVector& vector ) : my_vector(vector) { my_cancel = false; }
};
template<typename MyVector>
volatile bool GrowthException<MyVector>::my_cancel = false;

void TestExceptions() {
	typedef static_counting_allocator<std::allocator<Foo>, std::size_t> allocator_t;
    typedef tbb::concurrent_vector<Foo, allocator_t> vector_t;

    enum methods {
        zero_method = 0,
        ctor_copy, ctor_size, assign_nt, assign_ir, op_equ, reserve, compact, grow,
        all_methods
    };
    try {
        vector_t src(FooIterator(0), FooIterator(N)); // original data

        for(int t = 0; t < 2; t++) // exception type
        for(int m = zero_method+1; m < all_methods; m++)
        {
            allocator_t::init_counters();
            if(t) MaxFooCount = FooCount + N/2;
            else allocator_t::set_limits(N/2);
            vector_t victim;
            try {
                switch(m) {
                case ctor_copy: {
                        vector_t acopy(src);
                    } break; // auto destruction after exception is checked by ~Foo
                case ctor_size: {
                        vector_t sized(N);
                    } break; // auto destruction after exception is checked by ~Foo
                // Do not test assignment constructor due to reusing of same methods as below 
                case assign_nt: {
                        victim.assign(N, Foo());
                    } break;
                case assign_ir: {
                        victim.assign(FooIterator(0), FooIterator(N));
                    } break;
                case op_equ: {
                        victim.reserve(2); victim = src; // fragmented assignment
                    } break;
                case reserve: {
                        try {
                            victim.reserve(victim.max_size()+1);
                        } catch(std::length_error &) {
                        } catch(...) {
                            KNOWN_ISSUE("ERROR: unrecognized exception - known compiler issue\n");
                        }
                        victim.reserve(N);
                    } break;
                case compact: {
                        if(t) MaxFooCount = 0; else allocator_t::set_limits(); // reset limits
                        victim.reserve(2); victim = src; // fragmented assignment
                        if(t) MaxFooCount = 1; else allocator_t::set_limits(1, false); // block any allocation, check NULL return from allocator
                        victim.compact(); // should start defragmenting first segment
                    } break;
                case grow: {
                        tbb::task_scheduler_init init;
                        tbb::parallel_for( tbb::blocked_range<int>(0, N, 5), GrowthException<vector_t>(victim) );
                        if(GrowthException<vector_t>::my_cancel) throw tbb::bad_last_alloc(); // parallel algorithms don't support exception passing yet
                    } break;
                default:;
                }
                if(!t || m != reserve) ASSERT(false, "should throw an exception");
            } catch(std::bad_alloc &e) {
                allocator_t::set_limits(); MaxFooCount = 0;
                size_t capacity = victim.capacity();
                size_t size = victim.size();
                switch(m) {
                case reserve:
                    if(t) ASSERT(false, NULL);
                case assign_nt:
                case assign_ir:
                    if(!t) {
                        ASSERT(capacity < N/2, "unexpected capacity");
                        ASSERT(size == 0, "unexpected size");
                        break;
                    } else {
                        ASSERT(size == N, "unexpected size");
                        ASSERT(capacity >= N, "unexpected capacity");
                        int i;
                        for(i = 1; true; i++)
                            if(!victim[i].zero_bar()) break;
                            else ASSERT(victim[i].bar() == (m == assign_ir)? i : Foo::initial_value_of_bar, NULL);
                        for(; size_t(i) < size; i++) ASSERT(!victim[i].zero_bar(), NULL);
                        ASSERT(size_t(i) == size, NULL);
                        break;
                    }
                case grow:
                case op_equ:
                    if(!t) ASSERT(capacity > 0 && capacity < N, "unexpected capacity");
                    {// not related to if(!t)
                        vector_t copy_of_victim(victim);
                        ASSERT(copy_of_victim.size() > 0, NULL);
                        for(int i = 0; true; i++) {
                            try {
                                Foo &foo = victim.at(i);
                                int bar = t? foo.zero_bar() : foo.bar();
                                if(m != grow) ASSERT( bar == i || (t && bar == 0), NULL);
                                if(size_t(i) < copy_of_victim.size()) ASSERT( copy_of_victim[i].bar() == bar, NULL);
                            } catch(std::range_error &) { // skip broken segment
                                ASSERT( size_t(i) < size, NULL ); if(m == op_equ) break;
                            } catch(std::out_of_range &){
                                ASSERT( i > 0, NULL ); break;
                            } catch(...) {
                                KNOWN_ISSUE("ERROR: unrecognized exception - known compiler issue\n"); break;
                            }
                        }
                        vector_t copy_of_victim2(10); copy_of_victim2 = victim;
                        ASSERT(copy_of_victim == copy_of_victim2, "assignment doesn't match copying");
                        try {
                            victim = copy_of_victim;
                        } catch(tbb::bad_last_alloc &) { break;
                        } catch(...) {
                            KNOWN_ISSUE("ERROR: unrecognized exception - known compiler issue\n"); break;
                        }
                        ASSERT(t, NULL);
                    } break;
                case compact:
                    ASSERT(capacity > 0, "unexpected capacity");
                    ASSERT(victim == src, "compact() is broken");
                    break;

                default:; // nothing to check here
                }
                if( Verbose ) std::printf("Exception %d: %s\t- ok ()\n", m, e.what());
            }
        }
    } catch(...) {
        ASSERT(false, "unexpected exception");
    }
}

//------------------------------------------------------------------------

//! Test driver
int main( int argc, char* argv[] ) {
    // Test requires at least one thread.
    MinThread = 1;
    ParseCommandLine( argc, argv );
    if( MinThread<1 ) {
        std::printf("ERROR: MinThread=%d, but must be at least 1\n",MinThread); MinThread = 1;
    }
    known_issue_verbose = !Verbose;

    TestIteratorTraits<tbb::concurrent_vector<Foo>::iterator,Foo>();
    TestIteratorTraits<tbb::concurrent_vector<Foo>::const_iterator,const Foo>();
    TestSequentialFor<Foo> ();
    TestResizeAndCopy();
    TestAssign();
    TestCapacity();
    for( int nthread=MinThread; nthread<=MaxThread; ++nthread ) {
        tbb::task_scheduler_init init( nthread );
        TestParallelFor( nthread );
        TestConcurrentGrowToAtLeast();
        TestConcurrentGrowBy( nthread );
    }
    TestComparison();
    TestFindPrimes();
    TestSort();
#if __GLIBC__==2&&__GLIBC_MINOR__==3
    printf("Warning: Exception safety test is skipped due to a known issue.\n");
#else
    TestExceptions();
#endif

    if( Verbose ) 
        std::printf("sizeof(concurrent_vector<int>) == %d\n", (int)sizeof(tbb::concurrent_vector<int>));
    std::printf("done\n");
    return 0;
}
