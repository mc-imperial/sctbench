/*
  Copyright 1999-2007 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.
  
  You may not use this file except in compliance with the License.
  obtain a copy of the License at
  
    http://www.imagemagick.org/script/license.php
  
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickWand private application programming interface declarations.
*/
#ifndef _MAGICKWAND_STUDIO_H
#define _MAGICKWAND_STUDIO_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifdef __CYGWIN32__
#  ifndef __CYGWIN__
#    define __CYGWIN__ __CYGWIN32__
#  endif
#endif
#if defined(_WIN32) || defined(WIN32)
#  ifndef __WINDOWS__
#    ifdef _WIN32
#      define __WINDOWS__ _WIN32
#    else
#      ifdef WIN32
#        define __WINDOWS__ WIN32
#      endif
#    endif
#  endif
#endif

#if defined(_WIN64) || defined(WIN64)
#  ifndef __WINDOWS__
#    ifdef _WIN64
#      define __WINDOWS__ _WIN64
#    else
#      ifdef WIN64
#        define __WINDOWS__ WIN64
#      endif
#    endif
#  endif
#endif

#if !defined(vms) && !defined(macintosh) && !defined(__WINDOWS__)
# define POSIX
#endif

#define MAGICKWAND_IMPLEMENTATION  1

#if !defined(_MAGICKWAND_CONFIG_H)
# define _MAGICKWAND_CONFIG_H
# if !defined(vms) && !defined(macintosh)
#  include "wand/wand-config.h"
# else
#  include "wand-config.h"
# endif
# if defined(__cplusplus) || defined(c_plusplus)
#  undef inline
# endif
#endif

#if !defined(const)
#  define STDC
#endif

#if defined(__BORLANDC__) && defined(_DLL)
#  pragma message("BCBMagick lib DLL export interface")
#  define _WANDDLL_
#  define _WANDLIB_
#endif

#if defined(__WINDOWS__)
# if defined(_MT) && defined(_DLL) && !defined(_WANDDLL_) && !defined(_LIB)
#  define _WANDDLL_
# endif
# if defined(_WANDDLL_)
#  if defined(_VISUALC_)
#   pragma warning( disable: 4273 )  /* Disable the dll linkage warnings */
#  endif
#  if !defined(_WANDLIB_)
#   define WandExport  __declspec(dllimport)
#   if defined(_VISUALC_)
#    pragma message( "MagickWand lib DLL import interface" )
#   endif
#  else
#   define WandExport  __declspec(dllexport)
#   if defined(_VISUALC_)
#    pragma message( "MagickWand lib DLL export interface" )
#   endif
#  endif
# else
#  define WandExport
#  if defined(_VISUALC_)
#   pragma message( "MagickWand lib static interface" )
#  endif
# endif

# if defined(_DLL) && !defined(_LIB)
#  define ModuleExport  __declspec(dllexport)
#  if defined(_VISUALC_)
#   pragma message( "MagickWand module DLL export interface" )
#  endif
# else
#  define ModuleExport
#  if defined(_VISUALC_)
#   pragma message( "MagickWand module static interface" )
#  endif

# endif
# define WandGlobal  __declspec(thread)
# if defined(_VISUALC_)
#  pragma warning(disable : 4018)
#  pragma warning(disable : 4068)
#  pragma warning(disable : 4244)
#  pragma warning(disable : 4142)
#  pragma warning(disable : 4800)
#  pragma warning(disable : 4786)
#  pragma warning(disable : 4996)
# endif
#else
# define WandExport
# define ModuleExport
# define WandGlobal
#endif

#if defined(__cplusplus) || defined(c_plusplus)
# define storage_class  c_class
#else
# define storage_class  class
#endif

#define WandSignature  0xabacadabUL
#if !defined(MaxTextExtent)
# define MaxTextExtent  4096
#endif

#include <stdarg.h>
#include <stdio.h>
#if defined(__WINDOWS__) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#if !defined(__WINDOWS__)
# include <unistd.h>
#else
# include <direct.h>
# if !defined(HAVE_STRERROR)
#  define HAVE_STRERROR
# endif
#endif

#if defined(HAVE_STRINGS_H)
# include <strings.h>
#endif
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <signal.h>
#include <assert.h>

#if defined(HAVE_PREAD) && defined(HAVE_DECL_PREAD) && !HAVE_DECL_PREAD
ssize_t pread(int,void *,size_t,off_t);
#endif

#if defined(HAVE_PWRITE) && defined(HAVE_DECL_PWRITE) && !HAVE_DECL_PWRITE
ssize_t pwrite(int,const void *,size_t,off_t);
#endif

#if defined(HAVE_STRLCPY) && defined(HAVE_DECL_STRLCPY) && !HAVE_DECL_STRLCPY
extern size_t strlcpy(char *,const char *,size_t);
#endif

#if defined(HAVE_VSNPRINTF) && defined(HAVE_DECL_VSNPRINTF) && !HAVE_DECL_VSNPRINTF
extern int vsnprintf(char *,size_t,const char *,va_list);
#endif

#if !defined(wand_attribute)
#  if !defined(__GNUC__)
#    define wand_attribute(x)  /* nothing */
#  else
#    define wand_attribute  __attribute__
#  endif
#endif

#if !defined(wand_unused)
#  if defined(__GNUC__)
#     define wand_unused(x)  wand_unused_ ## x __attribute__((unused))
#  elif defined(__LCLINT__)
#    define wand_unused(x) /*@unused@*/ x
#  else
#    define wand_unused(x) x
#  endif
#endif

#if defined(__WINDOWS__) || defined(POSIX)
# include <sys/types.h>
# include <sys/stat.h>
# if defined(HAVE_FTIME)
# include <sys/timeb.h>
# endif
# if defined(POSIX)
#  if defined(HAVE_SYS_NDIR_H) || defined(HAVE_SYS_DIR_H) || defined(HAVE_NDIR_H)
#   define dirent direct
#   define NAMLEN(dirent) (dirent)->d_namlen
#   if defined(HAVE_SYS_NDIR_H)
#    include <sys/ndir.h>
#   endif
#   if defined(HAVE_SYS_DIR_H)
#    include <sys/dir.h>
#   endif
#   if defined(HAVE_NDIR_H)
#    include <ndir.h>
#   endif
#  else
#   include <dirent.h>
#   define NAMLEN(dirent) strlen((dirent)->d_name)
#  endif
#  include <sys/wait.h>
#  include <pwd.h>
# endif
# if !defined(S_ISDIR)
#  define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
# endif
# if !defined(S_ISREG)
#  define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
# endif
# include "wand/MagickWand.h"
# if !defined(__WINDOWS__)
#  include <sys/time.h>
# if defined(HAVE_SYS_TIMES_H)
#  include <sys/times.h>
# endif
# if defined(HAVE_SYS_RESOURCE_H)
#  include <sys/resource.h>
# endif
#endif
#else
# include <types.h>
# include <stat.h>
# if defined(macintosh)
#  if !defined(DISABLE_SIOUX)
#   include <SIOUX.h>
#   include <console.h>
#  endif
#  include <unix.h>
# endif
# include "wand/MagickWand.h"
#endif

#if defined(S_IRUSR) && defined(S_IWUSR)
# define S_MODE (S_IRUSR | S_IWUSR)
#elif defined (__WINDOWS__)
# define S_MODE (_S_IREAD | _S_IWRITE)
#else
# define S_MODE  0600
#endif

#if defined(__WINDOWS__)
# include "magick/nt-base.h"
#endif
#if defined(macintosh)
# include "magick/mac.h"
#endif
#if defined(vms)
# include "magick/vms.h"
#endif

#undef index
#undef pipe

/*
  Review these platform specific definitions.
*/
#if defined(POSIX)
# define DirectorySeparator  "/"
# define DirectoryListSeparator  ':'
# define EditorOptions  " -title \"Edit Image Comment\" -e vi"
# define Exit  exit
# define IsBasenameSeparator(c)  ((c) == '/')
# define PreferencesDefaults  "~/."
# define ProcessPendingEvents(text)
# define ReadCommandlLine(argc,argv)
# define SetNotifyHandlers
#else
# if defined(vms)
#  define ApplicationDefaults  "decw$system_defaults:"
#  define DirectorySeparator  ""
#  define DirectoryListSeparator  ';'
#  define EditorOptions  ""
#  define Exit  exit
#  define IsBasenameSeparator(c)  (((c) == ']') || ((c) == ':') || ((c) == '/'))
#  define MagickLibPath  "sys$login:"
#  define MagickImageCodersPath  "sys$login:"
#  define MagickImageFiltersPath  "sys$login:"
#  define MagickSharePath  "sys$login:"
#  define PreferencesDefaults  "decw$user_defaults:"
#  define ProcessPendingEvents(text)
#  define ReadCommandlLine(argc,argv)
#  define SetNotifyHandlers
# endif
# if defined(macintosh)
#  define ApplicationDefaults  "/usr/lib/X11/app-defaults/"
#  define DirectorySeparator  ":"
#  define DirectoryListSeparator  ';'
#  define EditorOptions ""
#  define IsBasenameSeparator(c)  ((c) == ':')
#  define MagickLibPath  ""
#  define MagickImageCodersPath  ""
#  define MagickImageFiltersPath  ""
#  define MagickSharePath  ""
#  define PreferencesDefaults  "~/."
#  if defined(DISABLE_SIOUX)
#   define ReadCommandlLine(argc,argv)
#   define SetNotifyHandlers \
     SetFatalErrorHandler(MacFatalErrorHandler); \
     SetErrorHandler(MACErrorHandler); \
     SetWarningHandler(MACWarningHandler)
#  else
#   define ReadCommandlLine(argc,argv) argc=ccommand(argv); puts(MagickVersion);
#   define SetNotifyHandlers \
     SetErrorHandler(MACErrorHandler); \
     SetWarningHandler(MACWarningHandler)
#  endif
# endif
# if defined(__WINDOWS__)
#  define DirectorySeparator  "\\"
#  define DirectoryListSeparator  ';'
#  define EditorOptions ""
#  define IsBasenameSeparator(c)  (((c) == '/') || ((c) == '\\'))
#  define ProcessPendingEvents(text)
#  if !defined(PreferencesDefaults)
#    define PreferencesDefaults  "~\\."
#  endif
#  define ReadCommandlLine(argc,argv)
#  define SetNotifyHandlers \
    SetErrorHandler(NTErrorHandler); \
    SetWarningHandler(NTWarningHandler)
#  undef sleep
#  define sleep(seconds)  Sleep(seconds*1000)
#  if !defined(HAVE_TIFFCONF_H)
#    define HAVE_TIFFCONF_H
#  endif
# endif

#endif

/*
  Define system symbols if not already defined.
*/
#if !defined(STDIN_FILENO)
#define STDIN_FILENO  0x00
#endif

#if !defined(O_BINARY)
#define O_BINARY  0x00
#endif

#if defined(HasLTDL) || (defined(__WINDOWS__) && defined(_DLL) && !defined(_LIB))
#  define SupportWandModules
#endif

#if defined(_WANDMOD_)
# undef BuildWandModules
# define BuildWandModules
#endif

/*
  I/O defines.
*/
#if defined(__WINDOWS__) && !defined(Windows95) && !defined(__BORLANDC__)
#define MagickSeek(file,offset,whence)  _lseeki64(file,offset,whence)
#define MagickTell(file)  _telli64(file)
#else
#define MagickSeek(file,offset,whence)  lseek(file,offset,whence)
#define MagickTell(file) tell(file)
#endif

#define ThrowWandFatalException(severity,tag,context) \
{ \
  ExceptionInfo \
    *exception; \
 \
  exception=AcquireExceptionInfo(); \
  (void) ThrowMagickException(exception,GetMagickModule(),severity,tag, \
    "`%s'",context); \
  CatchException(exception); \
  exception=DestroyExceptionInfo(exception); \
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
