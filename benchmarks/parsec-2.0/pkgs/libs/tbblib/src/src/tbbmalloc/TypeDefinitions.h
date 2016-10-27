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

#ifndef _itt_shared_malloc_TypeDefinitions_H_
#define _itt_shared_malloc_TypeDefinitions_H_

// Define preprocessor symbols used to determine architecture
#if _WIN32||_WIN64
#   if defined(_M_AMD64)
#       define __ARCH_x86_64 1
#   elif defined(_M_IA64)
#       define __ARCH_ipf 1
#   elif defined(_M_IX86)
#       define __ARCH_x86_32 1
#   else
#       error Unknown processor architecture for Windows
#   endif
#   define USE_WINTHREAD 1
#else /* Assume generic Unix */
#   if __x86_64__
#       define __ARCH_x86_64 1
#   elif __ia64__
#       define __ARCH_ipf 1
#   elif __i386__ || __i386
#       define __ARCH_x86_32 1
#   else
#       define __ARCH_other 1
#   endif
#   define USE_PTHREAD 1
#endif

// Include files containing declarations of intptr_t and uintptr_t
#if _WIN32
#include <stddef.h>
typedef unsigned __int32 uint32_t;
#else
#include <stdint.h>
#endif

//! PROVIDE YOUR OWN Customize.h IF YOU FEEL NECESSARY
#include "Customize.h"

#endif /* _itt_shared_malloc_TypeDefinitions_H_ */
