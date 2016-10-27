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

// Declarations for simple estimate of the memory being used by a program.
// Not yet implemented for Mac.
// This header is an optional part of the test harness.
// It assumes that "harness_assert.h" has already been included.

#if __linux__
#include <unistd.h>
#elif __APPLE__ || __sun
#include <unistd.h>
#elif _WIN32
#include <windows.h>
#endif
#include <new>
#include <stdexcept>
#include <tbb/atomic.h>

#if __SUNPRO_CC
using std::printf;
#endif

#if defined(_MSC_VER) && defined(_Wp64)
    // Workaround for overzealous compiler warnings in /Wp64 mode
    #pragma warning (push)
    #pragma warning (disable: 4267)
#endif /* _MSC_VER && _Wp64 */


template <typename base_alloc_t, typename count_t = tbb::atomic<size_t> >
class static_counting_allocator : public base_alloc_t
{
public:
    typedef typename base_alloc_t::pointer pointer;
    typedef typename base_alloc_t::const_pointer const_pointer;
    typedef typename base_alloc_t::reference reference;
    typedef typename base_alloc_t::const_reference const_reference;
    typedef typename base_alloc_t::value_type value_type;
    typedef typename base_alloc_t::size_type size_type;
    typedef typename base_alloc_t::difference_type difference_type;
    template<typename U> struct rebind {
        typedef static_counting_allocator<typename base_alloc_t::template rebind<U>::other,count_t> other;
    };

    static size_t max_items;
    static count_t items_allocated;
    static count_t items_freed;
    static count_t allocations;
    static count_t frees;
    static bool verbose, throwing;

    static_counting_allocator() throw() { }

    static_counting_allocator(const static_counting_allocator&) throw() { }

    template<typename U, typename C>
    static_counting_allocator(const static_counting_allocator<U, C>&) throw() { }

    bool operator==(const static_counting_allocator &a) const
    { return true; }

    pointer allocate(const size_type n)
    {
        if(verbose) printf("\t+%d|", int(n));
        if(max_items && items_allocated + n >= max_items) {
            if(verbose) printf("items limit hits!");
            if(throwing) throw std::bad_alloc();
            return NULL;
        }
        allocations++;
        items_allocated += n;
        return base_alloc_t::allocate(n, pointer(0));
    }

    pointer allocate(const size_type n, const void * const)
    {   return allocate(n); }

    void deallocate(const pointer ptr, const size_type n)
    {
        if(verbose) printf("\t-%d|", int(n));
        frees++;
        items_freed += n;
        base_alloc_t::deallocate(ptr, n);
    }

    static void init_counters(bool v = false) {
        verbose = v;
        if(verbose) printf("\n------------------------------------------- Allocations:\n");
        items_allocated = 0;
        items_freed = 0;
        allocations = 0;
        frees = 0;
        max_items = 0;
    }

    static void set_limits(size_type max = 0, bool do_throw = true) {
        max_items = max;
        throwing = do_throw;
    }
};

template <typename base_alloc_t, typename count_t>
size_t static_counting_allocator<base_alloc_t, count_t>::max_items;
template <typename base_alloc_t, typename count_t>
count_t static_counting_allocator<base_alloc_t, count_t>::items_allocated;
template <typename base_alloc_t, typename count_t>
count_t static_counting_allocator<base_alloc_t, count_t>::items_freed;
template <typename base_alloc_t, typename count_t>
count_t static_counting_allocator<base_alloc_t, count_t>::allocations;
template <typename base_alloc_t, typename count_t>
count_t static_counting_allocator<base_alloc_t, count_t>::frees;
template <typename base_alloc_t, typename count_t>
bool static_counting_allocator<base_alloc_t, count_t>::verbose;
template <typename base_alloc_t, typename count_t>
bool static_counting_allocator<base_alloc_t, count_t>::throwing;

template <typename base_alloc_t, typename count_t = tbb::atomic<size_t> >
class local_counting_allocator : public base_alloc_t
{
public:
    typedef typename base_alloc_t::pointer pointer;
    typedef typename base_alloc_t::const_pointer const_pointer;
    typedef typename base_alloc_t::reference reference;
    typedef typename base_alloc_t::const_reference const_reference;
    typedef typename base_alloc_t::value_type value_type;
    typedef typename base_alloc_t::size_type size_type;
    typedef typename base_alloc_t::difference_type difference_type;
    template<typename U> struct rebind {
        typedef local_counting_allocator<typename base_alloc_t::template rebind<U>::other,count_t> other;
    };

    count_t items_allocated;
    count_t items_freed;
    count_t allocations;
    count_t frees;
    size_t max_items;

    local_counting_allocator() throw() {
        items_allocated = 0;
        items_freed = 0;
        allocations = 0;
        frees = 0;
        max_items = 0;
    }

    local_counting_allocator(const local_counting_allocator &a) throw()
        : items_allocated(a.items_allocated)
        , items_freed(a.items_freed)
        , allocations(a.allocations)
        , frees(a.frees)
        , max_items(a.max_items)
    { }

    template<typename U, typename C>
    local_counting_allocator(const static_counting_allocator<U,C> &) throw() {
        items_allocated = static_counting_allocator<U,C>::items_allocated;
        items_freed = static_counting_allocator<U,C>::items_freed;
        allocations = static_counting_allocator<U,C>::allocations;
        frees = static_counting_allocator<U,C>::frees;
        max_items = static_counting_allocator<U,C>::max_items;
    }

    template<typename U, typename C>
    local_counting_allocator(const local_counting_allocator<U,C> &a) throw()
        : items_allocated(a.items_allocated)
        , items_freed(a.items_freed)
        , allocations(a.allocations)
        , frees(a.frees)
        , max_items(a.max_items)
    { }

    bool operator==(const local_counting_allocator &a) const
    { return &a == this; }

    pointer allocate(const size_type n)
    {
        if(max_items && items_allocated + n >= max_items)
            throw std::bad_alloc();
        ++allocations;
        items_allocated += n;
        return base_alloc_t::allocate(n, pointer(0));
    }

    pointer allocate(const size_type n, const void * const)
    { return allocate(n); }

    void deallocate(const pointer ptr, const size_type n)
    {
        ++frees;
        items_freed += n;
        base_alloc_t::deallocate(ptr, n);
    }

    void set_limits(size_type max = 0) {
        max_items = max;
    }
};

#if defined(_MSC_VER) && defined(_Wp64)
    // Workaround for overzealous compiler warnings in /Wp64 mode
    #pragma warning (pop)
#endif /* _MSC_VER && _Wp64 */
