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

#ifndef _TBB_ITT_NOTIFY
#define _TBB_ITT_NOTIFY

// IMPORTANT: To use itt_notify.cpp/itt_notify.h in a library, exactly
// one of the files that #includes itt_notify.h should have
//     #define INSTANTIATE_ITT_NOTIFY 1
// just before the #include of "itt_notify.h".  
// That file is the one that will have the instances of the hooks.
//
// The intent is to allow precise placement of the hooks.  Ideally, they
// should reside on a hot mostly-read cache line.

#if DO_ITT_NOTIFY

namespace tbb {

namespace internal {

//! Initialize support for __itt_notify handlers
/** It is the callers responsibility to ensure that this routine is called only once. 
    Returns true if ITT hooks were installed; false otherwise. */
bool InitializeITT();

//! A pointer to a __itt_notify call.
typedef void (*PointerToITT_Handler)(volatile void*);

//! A pointer to __itt_thr_set_name call.
typedef int (*PointerToITT_Name_Set)(const char*, int);

//! Dummy routine used for first indirect call via ITT_Handler_sync_prepare
void dummy_sync_prepare(volatile void*);

//! Dummy routine used for first indirect call via ITT_Handler_sync_acquired.
void dummy_sync_acquired(volatile void*);

//! Dummy routine used for first indirect call via ITT_Handler_sync_releasing.
void dummy_sync_releasing(volatile void*);

//! Dummy routine used for first indirect call via ITT_Handler_sync_cancel.
void dummy_sync_cancel(volatile void*);

//! Dummy routine used for first indirect call via ITT_Handler_thr_name_set.
int dummy_thr_name_set(const char*, int);

#if INSTANTIATE_ITT_NOTIFY
PointerToITT_Handler ITT_Handler_sync_prepare = &dummy_sync_prepare;
PointerToITT_Handler ITT_Handler_sync_acquired = &dummy_sync_acquired;
PointerToITT_Handler ITT_Handler_sync_releasing = &dummy_sync_releasing;
PointerToITT_Handler ITT_Handler_sync_cancel = &dummy_sync_cancel;
PointerToITT_Name_Set ITT_Handler_thr_name_set = &dummy_thr_name_set;
#else
extern PointerToITT_Handler ITT_Handler_sync_prepare;
extern PointerToITT_Handler ITT_Handler_sync_acquired;
extern PointerToITT_Handler ITT_Handler_sync_releasing;
extern PointerToITT_Handler ITT_Handler_sync_cancel;
extern PointerToITT_Name_Set ITT_Handler_thr_name_set;
#endif

} // namespace internal 

} // namespace tbb

//! Glues two tokens together.
#define ITT_GLUE(x,y) tbb::internal::x##y

//! Call routine itt_notify_(name) if corresponding handler is available.
/** For example, use ITT_NOTIFY(sync_releasing,x) to invoke __itt_notify_sync_releasing(x).
    Ordinarily, preprocessor token gluing games should be avoided.
    But here, it seemed to be the best way to handle the issue. */
#define ITT_NOTIFY(name,pointer) ( ITT_GLUE(ITT_Handler_,name) ? ITT_GLUE(ITT_Handler_,name)(pointer) : (void)0 )

#define ITT_NAME_SET(name,pointer,length) ( ITT_GLUE(ITT_Handler_,name) ? ITT_GLUE(ITT_Handler_,name)(pointer,length) : 0 )

#else

#define ITT_NOTIFY(name,pointer) ((void)0)
 
#define ITT_NAME_SET(name,pointer,length) ((void)0)
 
#endif /* DO_ITT_NOTIFY */

#if DO_ITT_QUIET
#define ITT_QUIET(x) (__itt_thr_mode_set(__itt_thr_prop_quiet,(x)?__itt_thr_state_set:__itt_thr_state_clr))
#else
#define ITT_QUIET(x) ((void)0)
#endif /* DO_ITT_QUIET */

#endif /* _TBB_ITT_NOTIFY */
