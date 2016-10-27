#  Copyright 1999-2007 ImageMagick Studio LLC, a non-profit organization
#  dedicated to making software imaging solutions freely available.
#
#  You may not use this file except in compliance with the License.  You may
#  obtain a copy of the License at
#
#    http://www.imagemagick.org/script/license.php
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

AC_PREREQ(2.60)
AC_INIT([ImageMagick],[6.3.6],[http://www.imagemagick.org],[ImageMagick])
AC_CONFIG_SRCDIR([magick/MagickCore.h])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_LIBOBJ_DIR([ltdl])
AC_CONFIG_HEADERS([magick/magick-config.h wand/wand-config.h])
AC_CONFIG_FILES( \
    config/delegates.xml \
    config/configure.xml \
    config/ImageMagick.rdf \
    config/type-ghostscript.xml \
    config/type.xml \
    config/type-solaris.xml \
    config/type-windows.xml \
    ImageMagick.spec \
    magick/ImageMagick.pc \
    magick/Magick-config \
    magick/version.h \
    magick.sh \
    Magick++/bin/Magick++-config \
    Magick++/lib/ImageMagick++.pc \
    Makefile \
    PerlMagick/Magick.pm \
    PerlMagick/Makefile.PL \
    utilities/animate.1 \
    utilities/compare.1 \
    utilities/composite.1 \
    utilities/conjure.1 \
    utilities/convert.1 \
    utilities/display.1 \
    utilities/identify.1 \
    utilities/ImageMagick.1 \
    utilities/import.1 \
    utilities/mogrify.1 \
    utilities/montage.1 \
    utilities/stream.1 \
    wand/Wand-config \
    wand/Wand.pc )

#
# Save initial user-tunable values
#
USER_LIBS=$LIBS
for var in CC CFLAGS CPPFLAGS CXX CXXCPP LDFLAGS LIBS ; do
  eval isset=\${$var+set}
    if test "$isset" = 'set'; then
        eval val=$`echo $var`
        DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS}'${var}=${val}' "
    fi
done
AC_SUBST(DISTCHECK_CONFIG_FLAGS)

CONFIGURE_ARGS=`echo "$0 $@" | sed -e 's/  */ /g'`
AC_SUBST(CONFIGURE_ARGS)

# Source file containing package/library versioning information.
. ${srcdir}/version.sh

echo "configuring ${PACKAGE_NAME} ${PACKAGE_VERSION}${PACKAGE_VERSION_ADDENDUM}"
AC_CANONICAL_TARGET([])

# Substitute library versioning
AC_SUBST(LIBRARY_CURRENT)dnl
AC_SUBST(LIBRARY_REVISION)dnl
AC_SUBST(LIBRARY_AGE)dnl
AC_SUBST(PACKAGE_NAME)dnl
AC_SUBST(PACKAGE_VERSION)dnl
AC_SUBST(PACKAGE_LIB_VERSION)dnl
AC_SUBST(PACKAGE_LIB_VERSION_NUMBER)dnl
AC_SUBST(PACKAGE_RELEASE_DATE)dnl
AC_SUBST(PACKAGE_VERSION_ADDENDUM)dnl

# Ensure that make can run correctly
AM_SANITY_CHECK

AM_INIT_AUTOMAKE([foreign subdir-objects dist-zip dist-bzip2 tar-ustar -Wall])
AC_SUBST([CONFIG_STATUS_DEPENDENCIES],['$(top_srcdir)/ChangeLog $(top_srcdir)/version.sh'])

PERLMAINCC=$CC

MAGICK_CFLAGS=''
MAGICK_CPPFLAGS=$CPPFLAGS_USER
MAGICK_PCFLAGS=$CPPFLAGS_USER
MAGICK_LDFLAGS=''
MAGICK_LIBS=''

#
# Evaluate shell variable equivalents to Makefile directory variables
#
if test "x$prefix" = xNONE; then
    prefix=$ac_default_prefix
fi
# Let make expand exec_prefix.
if test "x$exec_prefix" = xNONE; then
    exec_prefix='${prefix}'
fi

#
eval "eval PREFIX_DIR=${prefix}"
AC_SUBST(PREFIX_DIR)
eval "eval EXEC_PREFIX_DIR=${exec_prefix}"
AC_SUBST(EXEC_PREFIX_DIR)
eval "eval BIN_DIR=$bindir"
AC_SUBST(BIN_DIR)
eval "eval SBIN_DIR=$sbindir"
AC_SUBST(SBIN_DIR)
eval "eval LIBEXEC_DIR=$libexecdir"
AC_SUBST(LIBEXEC_DIR)
eval "eval DATA_DIR=$datadir"
AC_SUBST(DATA_DIR)
eval "eval SYSCONF_DIR=$sysconfdir"
AC_SUBST(SYSCONF_DIR)
eval "eval SHAREDSTATE_DIR=$sharedstatedir"
AC_SUBST(SHAREDSTATE_DIR)
eval "eval LOCALSTATE_DIR=$localstatedir"
AC_SUBST(LOCALSTATE_DIR)
eval "eval LIB_DIR=$libdir"
AC_SUBST(LIB_DIR)
eval "eval INCLUDE_DIR=$includedir"
AC_SUBST(INCLUDE_DIR)
eval "eval PERSISTINCLUDE_DIR=$oldincludedir"
AC_SUBST(PERSISTINCLUDE_DIR)
eval "eval INFO_DIR=$infodir"
AC_SUBST(INFO_DIR)
eval "eval MAN_DIR=$mandir"
AC_SUBST(MAN_DIR)

# Get full paths to source and build directories
srcdirfull=`cd $srcdir && pwd`
builddir=`pwd`

WinPathScript="${srcdirfull}/winpath.sh"
AC_SUBST(WinPathScript)

#
# Compute variables useful for running uninstalled software.
#
MAGICK_CODER_MODULE_PATH="${builddir}/coders"
MAGICK_CONFIGURE_SRC_PATH="${srcdirfull}/config"
MAGICK_CONFIGURE_BUILD_PATH="${builddir}/config"
MAGICK_FILTER_MODULE_PATH="${builddir}/filters"
DIRSEP=':'
case "${host_os}" in
    mingw* )
        MAGICK_CODER_MODULE_PATH=`$WinPathScript "${MAGICK_CODER_MODULE_PATH}" 0`
        MAGICK_CONFIGURE_SRC_PATH=`$WinPathScript "${MAGICK_CONFIGURE_SRC_PATH}" 0`
        MAGICK_CONFIGURE_BUILD_PATH=`$WinPathScript "${MAGICK_CONFIGURE_BUILD_PATH}" 0`
        MAGICK_FILTER_MODULE_PATH=`$WinPathScript "${MAGICK_FILTER_MODULE_PATH}" 0`
        DIRSEP=';'
    ;;
esac
AC_SUBST(MAGICK_CODER_MODULE_PATH)
AC_SUBST(MAGICK_CONFIGURE_SRC_PATH)
AC_SUBST(MAGICK_CONFIGURE_BUILD_PATH)
AC_SUBST(MAGICK_FILTER_MODULE_PATH)
AC_SUBST(DIRSEP)

# Check for programs
AC_PROG_CC
AC_PROG_CPP
AC_PROG_LD
AC_SUBST(LD)
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_COMPILE_WARNINGS
AC_PROG_LN_S
AM_WITH_DMALLOC

#
# Enable OS features.
#
AC_GNU_SOURCE

#
# Tests for Windows
#
AC_EXEEXT
AC_OBJEXT

GDI32_LIBS=''
StaticCplusPlusLibraries='no'
native_win32_build='no'
cygwin_build='no'
case "${host_os}" in
    cygwin* )
        StaticCplusPlusLibraries='yes'
        cygwin_build='yes'
        GDI32_LIBS='-lgdi32'
    ;;
    mingw* )
        StaticCplusPlusLibraries='yes'
        native_win32_build='yes'
        GDI32_LIBS='-lgdi32'
    ;;
esac
if test "${GDI32_LIBS}x" != 'x'; then
    AC_DEFINE(HasWINGDI32,1,Define to use the Windows GDI32 library)
fi
AC_SUBST(GDI32_LIBS)
AM_CONDITIONAL(HasWINGDI32, test "${GDI32_LIBS}x" != 'x' )
AM_CONDITIONAL(WIN32_NATIVE_BUILD, test "${native_win32_build}" = 'yes' )
AM_CONDITIONAL(CYGWIN_BUILD, test "${cygwin_build}" = 'yes' )

#
# Compiler flags tweaks
#
if test "${GCC}" != "yes"; then
    case "${host}" in
        *-*-hpux* )
            # aCC: HP ANSI C++ B3910B A.03.34
            CFLAGS="${CFLAGS} -Wp,-H30000"
            if test -n "${CXXFLAGS}"; then
                CXXFLAGS='-AA'
            else
                CXXFLAGS="${CXXFLAGS} -AA"
            fi
        ;;
        *-dec-osf5.* )
            # Compaq alphaev68-dec-osf5.1 compiler
            if test -n "${CXXFLAGS}"; then
                CXXFLAGS='-std strict_ansi -noimplicit_include'
            else
                CXXFLAGS="${CXXFLAGS} -std strict_ansi -noimplicit_include"
            fi
  esac
fi

dnl Platform-specific stuff
case "$host" in
*darwin*)
  dnl OS X universal binary support, requires --disable-dependency-tracking
  AC_ARG_ENABLE(osx-universal-binaries,
        AC_HELP_STRING([--enable-osx-universal-binaries],
           [build universal binaries on OS X [[default=no]]]),
        [build_osxuniversal="${enableval}"], [build_osxuniversal=no])
  
  if test "${build_osxuniversal}" != no ; then
    if test "$enable_dependency_tracking" != no ; then
      AC_MSG_ERROR([--enable-osx-universal-binary requires --disable-dependency-tracking.
Please re-run configure with these options:
  --disable-dependency-tracking --enable-osx-universal-binary
        ])
    fi
    CFLAGS="$CFLAGS -isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch ppc -arch i386"
    CXXFLAGS="$CXXFLAGS -isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch ppc -arch i386"
    LDFLAGS="$LDFLAGS -Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk -arch ppc -arch i386"
  fi
  
esac

# Enable support for threads
AC_ARG_WITH([threads],
    [AC_HELP_STRING([--without-threads],
                    [disable threads support])],
    [with_threads=$withval],
    [with_threads='yes'])

have_threads=no
if test "$with_threads" != 'no'; then
    ACX_PTHREAD()
    if test "$acx_pthread_ok" = yes; then
        have_threads=yes
        DEF_THREAD="$PTHREAD_CFLAGS"
        CFLAGS="$CFLAGS $DEF_THREAD"
        CXXFLAGS="$CXXFLAGS $DEF_THREAD"
        if test "$CC" != "$PTHREAD_CC"; then
            AC_MSG_WARN([Replacing compiler $CC with compiler $PTHREAD_CC to support pthreads.])
            CC="$PTHREAD_CC"
        fi
    fi
fi

# Enable support for OpenMP
AC_OPENMP([C])
CFLAGS="$CFLAGS $OPENMP_CFLAGS"
LDFLAGS="$LDFLAGS $OPENMP_CFLAGS"
MAGICK_PCFLAGS="$MAGICK_PCFLAGS $OPENMP_CFLAGS"

########
#
# Check for large file support
#
########
AC_SYS_LARGEFILE
AC_FUNC_FSEEKO
LFS_CPPFLAGS=''
if test "$enable_largefile" != no; then
    if test "$ac_cv_sys_file_offset_bits" != 'no'; then
        LFS_CPPFLAGS="$LFS_CPPFLAGS -D_FILE_OFFSET_BITS=$ac_cv_sys_file_offset_bits"
    else
        AC_MSG_CHECKING([for native large file support])
        AC_RUN_IFELSE([#include <unistd.h>
          main () { 
          exit(!(sizeof(off_t) == 8));
        }],
        [ac_cv_sys_file_offset_bits=64; AC_DEFINE(_FILE_OFFSET_BITS,64)
         AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])])
    fi
    if test "$ac_cv_sys_large_files" != 'no'; then
        LFS_CPPFLAGS="$LFS_CPPFLAGS -D_LARGE_FILES=1"
    fi
    if  test "$ac_cv_sys_largefile_source" != 'no'; then
        LFS_CPPFLAGS="$LFS_CPPFLAGS -D_LARGEFILE_SOURCE=1"
    fi
fi
AC_SUBST(LFS_CPPFLAGS)

#
# Configure libtool & libltdl
#

# Configure libltdl
#AC_CONFIG_SUBDIRS(ltdl)
AC_LIBLTDL_CONVENIENCE(ltdl)
AC_LIB_LTDL
# Substitute INCLTDL and LIBLTDL in the Makefiles
#AC_SUBST(INCLTDL)
#AC_SUBST(LIBLTDL)

# Configure libtool
AC_ENABLE_SHARED
AC_ENABLE_STATIC
AC_LIBTOOL_TAGS([CXX])
AC_LIBTOOL_WIN32_DLL
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)
#AC_OUTPUT

# Check to see if building shared libraries
libtool_build_shared_libs='no'
if test "$enable_shared" = 'yes'; then
    libtool_build_shared_libs='yes'
fi

# Check to see if building static libraries
libtool_build_static_libs='no'
if test "$enable_static" = 'yes'; then
    libtool_build_static_libs='yes'
fi

# MinGW and Cygwin can't build C++ DLLs which support exceptions.
if test "${StaticCplusPlusLibraries}" = 'yes'; then
    LTCXXLIBOPTS='--static'
    AC_SUBST(LTCXXLIBOPTS)
fi

#
# Enable support for building loadable modules
#
AC_ARG_WITH([modules],
    [AC_HELP_STRING([--with-modules],
                    [support dynamically loadable modules])],
    [with_modules=$withval],
    [with_modules=$libtool_build_shared_libs])

# Only allow building loadable modules if we are building shared libraries
if test "$with_modules" != 'no' ; then
    if test "$libtool_build_shared_libs" = 'no'; then
        AC_MSG_WARN([Modules may only be built if building shared libraries is enabled.])
        with_modules='no'
    fi
fi
if test "$with_modules" != 'no'; then
    AC_DEFINE(BuildMagickModules,1,Define if coders and filters are to be built as modules.)
fi
AM_CONDITIONAL(WITH_MODULES, test "$with_modules" != 'no')

# Enable building/use of libltdl if we are building shared libraries regardless
# of whether modules are built or not.
with_ltdl='no'
if test "$libtool_build_shared_libs" != 'no'; then
    with_ltdl='yes'
fi

AM_CONDITIONAL(WITH_LTDL, test "$with_ltdl" != 'no')
if test "$with_ltdl" != 'no'; then
    AC_DEFINE(HasLTDL,1,Define if using libltdl to support dynamically loadable modules)

    # Set DLLDFLAGS
    if test X"$enable_shared" = Xyes; then
        DLLDFLAGS=-export-dynamic
        AC_SUBST(DLLDFLAGS)
    fi
fi

# Enable build using delegate libraries built in subdirectories rather than installed
# delegate libraries (bzlib fpx jp2 jbig jpeg lcms png tiff ttf wmf xml zlib)
AC_ARG_ENABLE([delegate-build],
    [AC_HELP_STRING([--enable-delegate-build],
                    [look for delegate libraries in build directory])],
    [enable_delegate_build=$enableval],
    [enable_delegate_build='no'])

AC_ARG_ENABLE([deprecated],
    [AC_HELP_STRING([--disable-deprecated],
                    [exclude deprecated methods in MagickCore and MagickWand API's])],
    [enable_deprecated=$enableval],
    [enable_deprecated='no'])

if test "$enable_deprecated" = 'yes'; then
  AC_DEFINE(ExcludeMagickDeprecated,1,[exclude deprecated methods in MagickCore API])
else
  DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --disable-deprecate "
fi

# Build a version of ImageMagick which operates uninstalled.
# Used to build distributions located via MAGICK_HOME / executable path
AC_ARG_ENABLE([installed],
    [AC_HELP_STRING([--disable-installed],
                    [Formally install ImageMagick under PREFIX])],
    [enable_installed=$enableval],
    [enable_installed='yes'])

if test "$enable_installed" = 'yes'; then
    AC_DEFINE(UseInstalledMagick,1,[ImageMagick is formally installed under prefix])
else
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --disable-installed "
fi

# Build an embeddable version of ImageMagick.
AC_ARG_ENABLE([embeddable],
    [AC_HELP_STRING([--enable-embeddable],
                    [enable self-contained, embeddable, zero-configuration ImageMagick])],
    [enable_embeddable=$enableval],
    [enable_embeddable='no'])

if test "$enable_embeddable" = 'yes'; then
    AC_DEFINE(UseEmbeddableMagick,1,[Build self-contained, embeddable, zero-configuration ImageMagick (experimental)])
fi

# Build a high dynamic range version of ImageMagick.
AC_ARG_ENABLE([hdri],
    [AC_HELP_STRING([--enable-hdri],
                    [accurately represent the wide range of intensity levels found in real scenes])],
    [enable_hdri=$enableval],
    [enable_hdri='no'])

MAGICK_HDRI=""
if test "$enable_hdri" = 'yes'; then
    MAGICK_HDRI="HDRI"
    AC_DEFINE(UseHDRI,1,[accurately represent the wide range of intensity levels  in real scenes])
fi
AC_SUBST(MAGICK_HDRI)dnl

# Enable verbose output from libtool (enable libtool's default)
AC_ARG_ENABLE([libtool-verbose],
    [AC_HELP_STRING([--enable-libtool-verbose],
                    [enable verbose libtool output])],
    [enable_libtool_verbose=$enableval],
    [enable_libtool_verbose='no'])

if test "$enable_libtool_verbose" = 'no'; then
    LIBTOOL="$LIBTOOL --silent"
fi 

# Build a version of ImageMagick with assert statements.
AC_ARG_ENABLE([assert],
    [AC_HELP_STRING([--disable-assert],
                    [disable assert() statements in build])],
    [enable_assert=$enableval],
    [enable_assert='yes'])

if test "$enable_assert" = 'no'; then
    AC_DEFINE(NDEBUG,1,[Turn off assert statements])
fi

# Add configure option --enable-maintainer-mode which enables dependency
# checking and generation useful to package maintainers.  This is made an
# option to avoid confusing end users.
AM_MAINTAINER_MODE


# Enable ccmalloc memory debugging support
AC_ARG_ENABLE([ccmalloc],
    [AC_HELP_STRING([--enable-ccmalloc],
                    [enable 'ccmalloc' memory debug support])],
    [enable_ccmalloc=$enableval],
    [enable_ccmalloc='no'])

# Enable Electric Fence memory debugging support
AC_ARG_ENABLE([efence],
    [AC_HELP_STRING([--enable-efence],
                    [enable 'efence' memory debug support])],
    [enable_efence=$enableval],
    [enable_efence='no'])

# Enable prof-based profiling support
AC_ARG_ENABLE([prof],
    [AC_HELP_STRING([--enable-prof],
                    [enable 'prof' profiling support])],
    [enable_prof=$enableval],
    [enable_prof='no'])

# Enable gprof-based profiling support
AC_ARG_ENABLE([gprof],
    [AC_HELP_STRING([--enable-gprof],
                    [enable 'gprof' profiling support])],
    [enable_gprof=$enableval],
    [enable_gprof='no'])

# Enable gcov-based profiling support
AC_ARG_ENABLE([gcov],
    [AC_HELP_STRING([--enable-gcov],
                    [enable 'gcov' profiling support])],
    [enable_gcov=$enableval],
    [enable_gcov='no'])

enable_profiling='no'
if test "$enable_prof" = 'yes' || test "$enable_gprof" = 'yes' || test "$enable_gcov" = 'yes'; then
    enable_profiling='yes'
    if test "$libtool_build_shared_libs" = 'yes'; then
        echo "Warning: Can not profile code using shared libraries"
    fi
fi

# Magick API method prefix
AC_ARG_WITH([method-prefix],
    [AC_HELP_STRING([--with-method-prefix=PREFIX],
                    [prefix MagickCore API methods])],
    [with_method_prefix=$enableval],
    [with_method_prefix=''])

if test "$with_method_prefix" != ''; then
    AC_DEFINE_UNQUOTED(MagickMethodPrefix,$with_method_prefix,[Magick API method prefix])
fi

# Number of bits in a Quantum
AC_ARG_WITH([quantum-depth],
    [AC_HELP_STRING([--with-quantum-depth=DEPTH],
                    [number of bits in a pixel quantum (default 16)])],
    [with_quantum_depth=$withval],
    [with_quantum_depth=16])

if test "$with_quantum_depth" != '8'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-quantum-depth=$with_quantum_depth "
fi

case "${with_quantum_depth}" in
    8 )
    ;;
    16 )
    ;;
    32 )
    ;;
    64 )
    ;;
    * )
        AC_MSG_ERROR("Pixel quantum depth must have value of 8, 16, or 32")
    ;;
esac
QuantumDepth="$with_quantum_depth"
AC_DEFINE_UNQUOTED(QuantumDepth,$QuantumDepth,[Number of bits in a pixel Quantum (8/16/32/64)])
AC_SUBST(QuantumDepth)dnl

# Set pixel cache threshold
AC_ARG_WITH([cache],
    [AC_HELP_STRING([--with-cache=THRESHOLD],
                    [set pixel cache threshhold in MB (default available memory)])],
    [with_cache=$withval],
    [with_cache=''])

if test "$with_cache" != ''; then
    AC_DEFINE_UNQUOTED(PixelCacheThreshold,$with_cache,[Pixel cache threshold in MB (defaults to available memory)])
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-cache=$with_cache "
fi

# Disable/Enable support for full delegate paths
AC_ARG_WITH([frozenpaths],
    [AC_HELP_STRING([--with-frozenpaths],
                    [freeze delegate paths])],
    [with_frozenpaths=$withval],
    [with_frozenpaths='no'])

# Enable build/install of Magick++
AC_ARG_WITH([magick-plus-plus],
    [AC_HELP_STRING([--without-magick-plus-plus],
                    [disable build/install of Magick++])],
    [with_magick_plus_plus=$withval],
    [with_magick_plus_plus='yes'])

# Disable build/install of PerlMagick.
AC_ARG_WITH([perl],
    [AC_HELP_STRING([--without-perl],
                    [no not build or install of PerlMagick])],
    [with_perl=$withval],
    [with_perl='yes'])

# Options to pass when configuring PerlMagick
AC_ARG_WITH([perl-options],
    [AC_HELP_STRING([--with-perl-options=OPTIONS],
                    [options to pass on command-line when generating PerlMagick's build file])])

PERL_MAKE_OPTIONS=$with_perl_options
AC_SUBST(PERL_MAKE_OPTIONS)


# Disable BZLIB (bzip2 library)
AC_ARG_WITH([bzlib],
    [AC_HELP_STRING([--without-bzlib],
                    [disable BZLIB support])],
    [with_bzlib=$withval],
    [with_bzlib='yes'])

if test "$with_bzlib" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-bzlib=$with_bzlib "
fi

# Disable Display Postscript.
AC_ARG_WITH([dps],
    [AC_HELP_STRING([--without-dps],
                    [disable Display Postscript support])],
    [with_dps=$withval],
    [with_dps='yes'])

if test "$with_dps" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-dps=$with_dps "
fi

# Disable DJVU.
AC_ARG_WITH([djvu],
    [AC_HELP_STRING([--without-djvu],
                    [disable DjVu support])],
    [with_djvu=$withval],
    [with_djvu='yes'])

if test "$with_djvu" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-djvu=$with_djvu "
fi

# Enable FlashPIX.
AC_ARG_WITH([fpx],
    [AC_HELP_STRING([--with-fpx],
                    [enable FlashPIX support])],
    [with_fpx=$withval],
    [with_fpx='no'])

if test "$with_fpx" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-fpx=$with_fpx "
fi

# Enable Ghostscript library support.
AC_ARG_WITH([gslib],
    [AC_HELP_STRING([--without-gslib],
                    [disable Ghostscript library support])],
    [with_gslib=$withval],
    [with_gslib='yes'])

if test "$with_gslib" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-gslib=$with_gslib "
fi

# Disable JBIG.
AC_ARG_WITH([jbig],
    [AC_HELP_STRING([--without-jbig],
                    [disable JBIG support])],
    [with_jbig=$withval],
    [with_jbig='yes'])

if test "$with_jbig" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-jbig=$with_jbig "
fi

# Disable JPEG.
AC_ARG_WITH([jpeg],
    [AC_HELP_STRING([--without-jpeg],
                    [disable JPEG support])],
    [with_jpeg=$withval],
    [with_jpeg='yes'])

if test "$with_jpeg" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-jpeg=$with_jpeg "
fi 

# Disable JPEG Version 2.
AC_ARG_WITH([jp2],
    [AC_HELP_STRING([--without-jp2],
                    [disable JPEG-2000 support])],
    [with_jp2=$withval],
    [with_jp2='yes'])

if test "$with_jp2" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-jp2=$with_jp2 "
fi 

# Disable TIFF.
AC_ARG_WITH([tiff],
    [AC_HELP_STRING([--without-tiff],
                    [disable TIFF support])],
    [with_tiff=$withval],
    [with_tiff='yes'])

if test "$with_tiff" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-tiff=$with_tiff "
fi

# Disable WMF.
AC_ARG_WITH([wmf],
    [AC_HELP_STRING([--without-wmf],
                    [disable WMF support])],
    [with_wmf=$withval],
    [with_wmf=$with_modules])

if test "$with_wmf" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-wmf=$with_wmf "
fi

# Set default font search path
AC_ARG_WITH([fontpath],
    [AC_HELP_STRING([--with-fontpath=DIR],
                    [prepend to default font search path])],
    [with_fontpath=$withval],
    [with_fontpath=''])

if test "$with_fontpath" != "yes" && test -z "$with_fontpath"; then
    with_fontpath=''
else
    AC_DEFINE_UNQUOTED(MAGICK_FONT_PATH,"$with_fontpath",Define to prepend to default font search path.)
fi
if test "$with_fontpath=" != ''; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-fontpath=$with_fontpath "    
fi 

# Set Ghostscript font directory
AC_ARG_WITH([gs-font-dir],
    [AC_HELP_STRING([--with-gs-font-dir=DIR],
                    [Ghostscript font directory])],
    [with_gs_font_dir=$withval],
    [with_gs_font_dir='default'])

if test "$with_gs_font_dir" != 'default'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-gs-font-dir=$with_gs_font_dir "
fi  

# Set Windows font directory
AC_ARG_WITH([windows-font-dir],
    [AC_HELP_STRING([--with-windows-font-dir=DIR],
                    [MS Windows font directory])],
    [with_windows_font_dir=$withval],
    [with_windows_font_dir=''])

if test "x$with_windows_font_dir" = "xno"; then
    with_windows_font_dir=
fi
if test "$with_windows_font_dir" != ''; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-windows-font-dir=$with_windows_font_dir "
fi

AC_ARG_WITH([zlib],
    [AC_HELP_STRING([--without-zlib],
                    [disable ZLIB support])],
    [with_zlib=$withval],
    [with_zlib='yes'])

if test "$with_zlib" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-zlib=$with_zlib "
fi

#
# Specify path to shared libstdc++ if not in normal location
#
AC_ARG_WITH([libstdc],
    [AC_HELP_STRING([--with-libstdc=DIR],
                    [ use libstdc++ in DIR (for GNU C++)])],
    [with_libstdc=$withval],
    [with_libstdc=''])

if test "$with_libstdc" != ''; then
    if test -d "$with_libstdc"; then
        LIBSTDCLDFLAGS="-L$with_libstdc"
    fi
fi
AC_SUBST(LIBSTDCLDFLAGS)

# Does gcc required -traditional?
AC_PROG_GCC_TRADITIONAL

########
#
# Set defines required to build DLLs and modules using MinGW
#
########
# These options are set for multi-thread DLL module build
#  libMagick:           _DLL _MAGICKMOD_ _MAGICKLIB_
#  module:              _DLL
#  executable/Magick++: _DLL _MAGICKMOD_
MODULE_EXTRA_CPPFLAGS=''
LIBRARY_EXTRA_CPPFLAGS=''
if test "${native_win32_build}" = 'yes'; then
    if test "${libtool_build_shared_libs}" = 'yes'; then
        CPPFLAGS="$CPPFLAGS -D_DLL"
        MAGICK_CPPFLAGS="$MAGICK_CPPFLAGS -D_DLL"
        MAGICK_PCFLAGS="$MAGICK_PCFLAGS -D_DLL"
        LIBRARY_EXTRA_CPPFLAGS="$LIBRARY_EXTRA_CPPFLAGS -D_MAGICKLIB_"
        if test "$with_modules" = 'yes'; then
            LIBRARY_EXTRA_CPPFLAGS="$LIBRARY_EXTRA_CPPFLAGS -D_MAGICKMOD_"
        else
            MODULE_EXTRA_CPPFLAGS="$MODULE_EXTRA_CPPFLAGS -D_MAGICKLIB_"
        fi
    else
        CPPFLAGS="$CPPFLAGS -D_LIB"
        MAGICK_CPPFLAGS="$MAGICK_CPPFLAGS -D_LIB"
        MAGICK_PCFLAGS="$MAGICK_PCFLAGS -D_LIB"
    fi
    if test "$with_threads" = 'yes'; then
         CPPFLAGS="$CPPFLAGS -D_MT"
         MAGICK_CPPFLAGS="$MAGICK_CPPFLAGS -D_MT"
         MAGICK_PCFLAGS="$MAGICK_PCFLAGS -D_MT"
    fi
fi
AC_SUBST(MODULE_EXTRA_CPPFLAGS)
AC_SUBST(LIBRARY_EXTRA_CPPFLAGS)

# Check standard headers
AC_HEADER_STDC
if ! test x"$ac_cv_header_stdc" = x"yes"; then
    AC_MSG_WARN([configure has detected that you do not have the ANSI standard C
    header files.  Compilation cannot proceed.  Please install the ANSI C
    headers and rerun this script.]);
fi
AC_HEADER_DIRENT

# Check additional headers
AC_CHECK_HEADERS(errno.h fcntl.h inttypes.h limits.h locale.h machine/param.h mach-o/dyld.h stdarg.h string.h strings.h sys/resource.h sys/time.h sys/timeb.h sys/times.h sys/types.h sys/wait.h unistd.h)

########
#
# Checks for typedefs, structures, and compiler characteristics.
#
########

# If the C compiler does not fully support the ANSI C qualifier const,
# define const to be empty.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_VOLATILE
AC_C_STRINGIZE
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_STAT
AC_HEADER_TIME
AC_STRUCT_TM

# If the C compiler supports the keyword inline, do nothing. Otherwise
# define inline to __inline__ or __inline if it accepts one of those,
# otherwise define inline to be empty.
AC_C_INLINE

# If words are stored with the most significant byte first (like
# Motorola and SPARC CPUs), define `WORDS_BIGENDIAN'.
AC_C_BIGENDIAN

# If the C compiler supports a working long double type with more range
# or precision than the double type, define HAVE_LONG_DOUBLE.
AC_C_LONG_DOUBLE

# If the C type char is unsigned, define __CHAR_UNSIGNED__, unless the
# C compiler predefines it.
AC_C_CHAR_UNSIGNED

# Obtain size of an 'short' and define as SIZEOF_SHORT
AC_CHECK_SIZEOF(short)

# Obtain size of an 'unsigned short' and define as SIZEOF_UNSIGNED_SHORT
AC_CHECK_SIZEOF(unsigned short)

# Obtain size of an 'int' and define as SIZEOF_INT
AC_CHECK_SIZEOF(int)

# Obtain size of an 'unsigned int' and define as SIZEOF_UNSIGNED_INT
AC_CHECK_SIZEOF(unsigned int)

# Obtain size of a 'long' and define as SIZEOF_LONG
AC_CHECK_SIZEOF(long)

# Obtain size of a 'unsigned long' and define as SIZEOF_UNSIGNED_LONG
AC_CHECK_SIZEOF(unsigned long)

# Obtain size of off_t and define as SIZEOF_OFF_T
AC_CHECK_SIZEOF(off_t)

# Obtain size of a 'size_t' and define as SIZEOF_SIZE_T
AC_CHECK_SIZEOF(size_t)

# Obtain size of a 'ssize_t' and define as SIZEOF_SSIZE_T
AC_CHECK_SIZEOF(ssize_t)

# Obtain size of a 'double' and define as SIZEOF_DOUBLE
AC_CHECK_SIZEOF(double)

# Obtain size of a 'long long' and define as SIZEOF_LONG_LONG.  If
# 'long long' is not supported then the value defined is zero.
AC_CHECK_SIZEOF(long long)

# Obtain size of a 'unsigned long long' and define as
# SIZEOF_UNSIGNED_LONG_LONG.  If 'unsigned long long' is not
# supported then the value defined is zero.
AC_CHECK_SIZEOF(unsigned long long)

# Obtain size of a 'unsigned double' and define as SIZEOF_LONG_DOUBLE.  If
# 'long double' is not supported then the value defined is zero.
AC_CHECK_SIZEOF(long double)

# Obtain size of an 'void *' and define as SIZEOF_VOID
AC_CHECK_SIZEOF(void *)

# If the system does not provide a 'mode_t' type, use 'int' instead
AC_CHECK_TYPE(mode_t, unsigned int)

# If the system does not provide a 'off_t' type, use 'long' instead
AC_CHECK_TYPE(off_t, long)

# If the system does not provide a 'pid_t' type, use 'int' instead
AC_CHECK_TYPE(pid_t, int)

# If the system does not provide a 'size_t', use 'unsigned long' instead.
AC_CHECK_TYPE(size_t, unsigned long)

# If the system does not provide a 'ssize_t', use 'long' instead.
AC_CHECK_TYPE(ssize_t, long)

AC_MSG_CHECKING([whether our compiler supports __func__])
AC_TRY_COMPILE([],
 [{ const char *func = __func__; return(func != 0 ? 0 : 1); }],
 AC_MSG_RESULT([yes]),
 AC_MSG_RESULT([no])
 AC_MSG_CHECKING([whether our compiler supports __FUNCTION__])
 AC_TRY_COMPILE([],
   [{ const char *func = __FUNCTION__; return(func != 0 ? 0 : 1); }],
   AC_MSG_RESULT([yes])
   AC_DEFINE(__func__, __FUNCTION__,
     [Define to appropriate substitue if compiler does not have __func__]),
   AC_MSG_RESULT([no])
   AC_DEFINE(__func__, __FILE__,
     [Define to appropriate substitue if compiler does not have __func__])))

########
#
# Check for functions
#
########
MAGICK_FUNC_MMAP_FILEIO
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_MMAP
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_SETVBUF_REVERSED
AC_TYPE_SIGNAL
AC_FUNC_STRTOD
AC_FUNC_VPRINTF

AC_CHECK_FUNCS([_exit atexit clock_gettime execvp floor fork ftime ftruncate getcwd getexecname getdtablesize getpagesize gettimeofday lstat memset mkstemp munmap _NSGetExecutablePath pclose _pclose poll popen _popen pow pread pwrite raise rand_r select seekdir setlocale sqrt setvbuf stat strcspn strdup strpbrk strspn strstr strtol symlink sysconf sigemptyset sigaction strerror strlcat strlcpy strcasecmp strncasecmp telldir tempnam times usleep vsprintf vsnprintf waitpid])

########
#       
# Check for function prototypes
#
########

AC_CHECK_DECLS([pread, pwrite],[],[],[
#include <unistd.h>])

AC_CHECK_DECLS([strlcpy],[],[],[
#include <strings.h>])

AC_CHECK_DECLS([vsnprintf],[],[],[
#include <stdio.h>
#include <stdarg.h>])

########
#
# C++ Support Tests (For Magick++)
#
########
have_magick_plus_plus='no'
if test "$with_magick_plus_plus" = 'yes'; then
    OLIBS="$LIBS"
    LIBS=''
    AC_LANG_PUSH(C++)

    # Full set of headers used ...
    # algorithm cctype cerrno cmath cstdio cstdlib cstring ctime exception
    # functional iomanip iosfwd iostream iterator list string strstream utility
    AC_LANG([C++])
    AC_PROG_CXX
    AC_CXX_HAVE_BOOL
    AC_CXX_HAVE_NAMESPACES
    AC_CXX_HAVE_STD_NAMESPACE
    AC_CXX_HAVE_STD_LIBS
    AC_CXX_HAVE_LSTRING
    AC_OPENMP([C++])
    CXXFLAGS="$CXXFLAGS $OPENMP_CXXFLAGS" 
    AC_LANG_POP

    AC_MSG_CHECKING([whether C++ compiler is sufficient for Magick++])
    if \
        test $ac_cv_cxx_have_bool = 'yes' && \
        test $ac_cv_cxx_have_lstring = 'yes' && \
        test $ac_cv_cxx_have_namespaces = 'yes' && \
        test $ac_cv_cxx_have_std_libs = 'yes' && \
        test $ac_cv_cxx_have_std_namespace = 'yes'; then
        have_magick_plus_plus='yes'
    else
        have_magick_plus_plus='no (failed tests)'
    fi
    AC_MSG_RESULT([$have_magick_plus_plus])
    LIBS="$OLIBS"
fi
AM_CONDITIONAL(WITH_MAGICK_PLUS_PLUS, test "$have_magick_plus_plus" = 'yes')

# Only check for delegate libraries in subdirectories if requested.
if test "$enable_delegate_build" != 'no'; then
    # Check for delegate sub-directories and add -I & -L options as required.
    # This presumes that delegates are installed as detailed in the ImageMagick
    # README. If delegates are installed in a standard location where the
    # compiler will automatically find them then these options should not be
    # required.

    #
    # Most delegates have includes in the same directory as the library, but not all ...
    #
    # Includes
    for dir in bzlib fpx jp2 jp2/src/libjasper/include jbig/libjbig jpeg lcms/include magick png tiff/libtiff ttf/include wand wmf/include xml/include zlib; do
        if test -d "$builddir/$dir"; then
            CPPFLAGS="$CPPFLAGS -I$builddir/$dir"
        else
            if test -d "$srcdirfull/$dir"; then
                CPPFLAGS="$CPPFLAGS -I$srcdirfull/$dir"
            fi
        fi
    done

    # Libraries
    for dir in bzlib fpx jp2 jp2/src/libjasper jbig/libjbig jpeg lcms/src magick png tiff/libtiff ttf/objs wand wmf/src xml zlib; do
    if test -d "$builddir/$dir/.libs"; then
        LDFLAGS="$LDFLAGS -L$builddir/$dir/.libs"
    else
        if test -d "$srcdirfull/$dir/.libs"; then
            LDFLAGS="$LDFLAGS -L$srcdirfull/$dir/.libs"
        fi
    fi
    if test -d "$builddir/$dir"; then
        LDFLAGS="$LDFLAGS -L$builddir/$dir"
    else
        if test -d "$srcdirfull/$dir"; then
            LDFLAGS="$LDFLAGS -L$srcdirfull/$dir"
        fi
    fi
  done
fi

# Assume that delegate headers reside under same directory as ImageMagick
# installation prefix.
MAGICK_CPPFLAGS="-I$INCLUDE_DIR $MAGICK_CPPFLAGS"

#
# Find the X11 RGB database
#
AC_CACHE_CHECK(for X11 configure files,im_cv_x_configure,
[# Look for the header file in a standard set of common directories.
# Check X11 before X11Rn because it is often a symlink to the current release.
    for ac_dir in          \
    /lib/usr/lib/X11       \
    /usr/X11/lib           \
    /usr/X11R4/lib         \
    /usr/X11R5/lib         \
    /usr/X11R6/lib         \
    /usr/X386/lib          \
    /usr/XFree86/lib/X11   \
    /usr/athena/lib        \
    /usr/lib               \
    /usr/lib/X11           \
    /usr/lib/X11R4         \
    /usr/lib/X11R5         \
    /usr/lib/X11R6         \
    /usr/local/X11/lib     \
    /usr/local/X11R4/lib   \
    /usr/local/X11R5/lib   \
    /usr/local/X11R6/lib   \
    /usr/local/lib         \
    /usr/local/lib/X11     \
    /usr/local/lib/X11R4   \
    /usr/local/lib/X11R5   \
    /usr/local/lib/X11R6   \
    /usr/local/x11r5/lib   \
    /usr/lpp/Xamples/lib   \
    /usr/openwin/lib       \
    /usr/openwin/share/lib \
    /usr/unsupported/lib   \
    /usr/x386/lib          \
    ; do
    if test -f "$ac_dir/X11/rgb.txt"; then
      im_cv_x_configure="$ac_dir/X11/"
      break
    elif test -f "$ac_dir/rgb.txt"; then
      im_cv_x_configure="$ac_dir/"
      break
    fi

  done])
X11ConfigurePath="$im_cv_x_configure"
if test "$native_win32_build" = 'yes'; then
    X11ConfigurePath=`$WinPathScript "$X11ConfigurePath=" 1`
fi
AC_DEFINE_UNQUOTED(X11ConfigurePath,"X11ConfigurePath",Location of X11 configure files)

#
# Find OpenMP library
#
GOMP_LIBS=''
if test "$enable_openmp" != 'no'; then
  AC_CHECK_LIB(gomp,GOMP_parallel_start,GOMP_LIBS="-lgomp",,)
  LIBS="$GOMP_LIBS $LIBS"
fi
AC_SUBST(GOMP_LIBS)

#
# Find Posix threads library
#
THREAD_LIBS=''
if test "$with_threads" != 'no' && test "$have_threads" = 'yes'; then

    if test "x$PTHREAD_LIBS" = "x"; then
    case "${host_cpu}-${host_os}" in
      *-freebsd*)
        MAGICK_CHECK_PTHREAD_LIB(c_r,PTHREAD_LIBS=-lc_r) ;;
    esac
    fi

    for lib in pthread pthreads; do
        if test "x$PTHREAD_LIBS" = "x"; then
            MAGICK_CHECK_PTHREAD_LIB([$lib],[PTHREAD_LIBS=-l$lib])
        fi
    done

    THREAD_LIBS="$PTHREAD_LIBS"
    LIBS="$LIBS $THREAD_LIBS"
fi
AC_SUBST(THREAD_LIBS)

#
# Add support for ccmalloc memory debugging library if requested
#
have_ccmalloc='no'
CCMALLOC_LIBS=''
if test "$enable_ccmalloc" = 'yes'; then
    AC_PATH_PROG(CCMALLOCDelegate,ccmalloc,)
    if test -n "$CCMALLOCDelegate"; then
        eval `grep PREFIX= $CCMALLOCDelegate | sed -e 's/PREFIX/CCMALLOC_PREFIX/'`
        OLIBS="$LIBS"
        # Assume that gcc is used with ccmalloc.
        LIBS="$LIBS $CCMALLOC_PREFIX/lib/ccmalloc-gcc.o"
       AC_CHECK_LIB(ccmalloc,ccmalloc_malloc,CCMALLOC_LIBS="$CCMALLOC_PREFIX/lib/ccmalloc-gcc.o -lccmalloc -ldl",,-ldl)
        if test -n "$CCMALLOC_LIBS"; then
            LIBS="$OLIBS"
            LIBS="$LIBS $CCMALLOC_LIBS"
            have_ccmalloc='yes'
        else
            LIBS="$OLIBS"
        fi
    fi
fi

#
# Add support for efence memory debugging library if requested
#
if test "$enable_efence" = 'yes'; then
    EFENCE_LIBS='-lefence'
    LIBS="$EFENCE_LIBS $LIBS"
fi

#
# Find math library
#
MATH_LIBS=''
AC_CHECK_LIB(m,sqrt,MATH_LIBS="-lm",,)
LIBS="$MATH_LIBS $LIBS"
AC_SUBST(MATH_LIBS)

#
# Check for ZLIB
#
have_zlib='no'
ZLIB_LIBS=''
dnl PNG requires zlib so enable zlib check if PNG is requested
if test "$with_zlib" != 'no' || test "$with_png" != 'no'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for ZLIB support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    ZLIB_LIBS=''
    failed=0;
    passed=0;
    AC_CHECK_HEADER(zconf.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_HEADER(zlib.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_LIB(z,compress,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_CHECK_LIB(z,uncompress,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_CHECK_LIB(z,deflate,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_CHECK_LIB(z,inflate,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_CHECK_LIB(z,gzseek,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_CHECK_LIB(z,gztell,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_MSG_CHECKING([if ZLIB package is complete])
    if test $passed -gt 0; then
        if test $failed -gt 0; then
            AC_MSG_RESULT([no -- some components failed test])
            have_zlib='no (failed tests)'
        else
            ZLIB_LIBS='-lz'
            LIBS="$ZLIB_LIBS $LIBS"
            AC_DEFINE(HasZLIB,1,Define if you have zlib compression library)
            AC_MSG_RESULT([yes])
            have_zlib='yes'
        fi
    else
        AC_MSG_RESULT([no])
    fi
fi
AM_CONDITIONAL(HasZLIB, test "$have_zlib" = 'yes')
AC_SUBST(ZLIB_LIBS)

#
# Check for BZLIB
#
have_bzlib='no'
if test "$with_bzlib" != 'no'; then
    BZLIB_LIBS=''
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for BZLIB support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    failed=0;
    passed=0;
    AC_CHECK_HEADER(bzlib.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_LIB(bz2,BZ2_bzCompress,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_CHECK_LIB(bz2,BZ2_bzDecompress,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_MSG_CHECKING([if BZLIB package is complete])
    if test $passed -gt 0; then
        if test $failed -gt 0; then
            AC_MSG_RESULT([no -- some components failed test])
            have_bzlib='no (failed tests)'
        else
            BZLIB_LIBS='-lbz2'
            LIBS="$BZLIB_LIBS $LIBS"
            AC_DEFINE(HasBZLIB,1,Define if you have the bzip2 library)
            AC_MSG_RESULT([yes])
            have_bzlib='yes'
        fi
    else
        AC_MSG_RESULT([no])
    fi
fi
AM_CONDITIONAL(HasBZLIB, test "$have_bzlib" = 'yes')
AC_SUBST(BZLIB_LIBS)

#
# Find the X11 include and library directories.
#
IPC_LIBS=''
X11_LIBS=''
XEXT_LIBS=''
XT_LIBS=''
AC_PATH_XTRA
if test "$no_x" != 'yes'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for X11 support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    LDFLAGS="$LDFLAGS $X_LIBS"
    X11_LIBS="$X_PRE_LIBS -lX11 $X_EXTRA_LIBS"
    LIBS="$X11_LIBS $LIBS"
    CPPFLAGS="$CPPFLAGS $X_CFLAGS"

    AC_DEFINE(HasX11,1,Define if you have X11 library)dnl

    #
    # Check for X11 shared memory extension
    #
    # shmctl is required to support the shared memory extension
    AC_CHECK_FUNC([shmctl],[have_shmctl='yes'],[])
    if test "$have_shmctl" != 'yes'; then
        PERSIST_LIBS=$LIBS
        LIBS="$LIBS -lcygipc"
        AC_TRY_LINK_FUNC([shmctl],[have_shmctl='yes'; IPC_LIBS='-lcygipc'],[])
        LIBS=$PERSIST_LIBS
    fi

    if test "$have_shmctl" = 'yes'; then
        AC_CHECK_LIB([Xext],[XShmAttach],[XEXT_LIBS='-lXext' ; AC_DEFINE(HasSharedMemory,1,X11 server supports shared memory extension)],[],[])
    fi

    #
    # Check for X11 shape extension
    #
    AC_CHECK_LIB([Xext],[XShapeCombineMask],[XEXT_LIBS='-lXext' ; AC_DEFINE(HasShape,1,X11 server supports shape extension)],[],[])
    AC_CHECK_LIB(Xt,XtSetEventDispatcher,XT_LIBS='-lXt',,)
    LIBS="$XEXT_LIBS $XT_LIBS $LIBS"
fi
if test "$no_x" != 'yes'; then
  have_x='yes'
else
  have_x='no'
fi
AM_CONDITIONAL(HasX11, test "$have_x" = 'yes')
AC_SUBST(X11_LIBS)
AC_SUBST(XEXT_LIBS)

#
# If profiling, then check for -ldl and dlopen (required for Solaris & gcc)
#
LIB_DL=''
if test "$enable_profiling" = 'yes'; then
    AC_CHECK_LIB(dl,dlopen,LIB_DL='-ldl',,)
    LIBS="$LIB_DL $LIBS"
fi
AC_SUBST(LIB_DL)

#
# Check for Display Postscript
#
have_dps='no'
DPS_LIBS=''
if test "$with_dps" != 'no' && test "$with_x" != 'no'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for Display Postscript support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    failed=0;
    passed=0;
    PERSIST_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS -I${ac_x_includes}/X11"
    AC_CHECK_HEADER(DPS/dpsXclient.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    # DPS issues:
    # XFree86-4.x needs -lXt to provide XtMalloc for -ldps.
    # Cygwin doesn't deliver -lXt as a DLL, which prevents a DLL build.
    # Adobe DPS (as delivered on Solaris) doesn't require -lXt.
    # ImageMagick itself doesn't use -lXt.
    have_libdps='no'
    LIBDPS_XT=''
    AC_CHECK_LIB(dps,DPSInitialize,have_libdps='yes',have_libdps='no',)
    if test "$have_libdps" != 'yes'; then
        # Unset cache variable so we can try again.
        unset ac_cv_lib_dps_DPSInitialize
        AC_CHECK_LIB(dps,DPSInitialize,have_libdps='yes',have_libdps='no',-lXt)
        if test "$have_libdps" = 'yes'; then
            LIBDPS_XT='-lXt'
        fi
    fi
    if test "$have_libdps" = 'yes'; then
        passed=`expr $passed + 1`
    else
        failed=`expr $failed + 1`
    fi
    AC_CHECK_LIB(dpstk,XDPSPixelsPerPoint,passed=`expr $passed + 1`,failed=`expr $failed + 1`,-ldps $LIBDPS_XT)
    AC_MSG_CHECKING([if DPS package is complete])
    if test $passed -gt 0; then
        if test $failed -gt 0; then
            AC_MSG_RESULT([no -- some components failed test])
            have_dps='no (failed tests)'
            CPPFLAGS="$PERSIST_CPPFLAGS"
        else
            DPS_LIBS="-ldpstk -ldps ${LIBDPS_XT}"
            LIBS="$DPS_LIBS $LIBS"
            AC_DEFINE(HasDPS,1,Define if you have Display Postscript)
            AC_MSG_RESULT([yes])
            have_dps='yes'
        fi
    else
        AC_MSG_RESULT([no])
        CPPFLAGS=$PERSIST_CPPFLAGS
    fi
fi
AM_CONDITIONAL(HasDPS, test "$have_dps" = 'yes')
AC_SUBST(DPS_LIBS)

 #
# Check for DJVU
#
have_djvu='no'
DJVU_LIBS=''
if test "$with_djvu" != 'no'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for DJVU support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    failed=0;
    passed=0;
    AC_CHECK_HEADER(libdjvu/ddjvuapi.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_CHECK_LIB(djvulibre,ddjvu_context_create,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_MSG_CHECKING([if DJVU package is complete])
    if test $passed -gt 0; then
        if test $failed -gt 0; then
            AC_MSG_RESULT([no -- some components failed test])
            have_djvu='no (failed tests)'
        else
            DJVU_LIBS='-ldjvulibre'
            LIBS="$DJVU_LIBS $LIBS"
            AC_DEFINE(HasDJVU,1,Define if you have DJVU library)
            AC_MSG_RESULT([yes])
            have_djvu='yes'
        fi
    else
        AC_MSG_RESULT([no])
    fi
fi
AM_CONDITIONAL(HasDJVU, test "$have_djvu" = 'yes')
AC_SUBST(DJVU_LIBS)

#
# Check for FlashPIX
#
have_fpx='no'
FPX_LIBS=''
if test "$with_fpx" != 'no'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for FlashPIX support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    failed=0;
    passed=0;
    AC_LANG_PUSH(C++)
    AC_CHECK_HEADER(fpxlib.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_LIB(fpx,FPX_OpenImageByFilename,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_LANG_POP
    AC_MSG_CHECKING([if FlashPIX package is complete])
    if test $passed -gt 0; then
        if test $failed -gt 0; then
            AC_MSG_RESULT([no -- some components failed test])
            have_fpx='no (failed tests)'
        else
            FPX_LIBS='-lfpx'
            AC_DEFINE(HasFPX,1,Define if you have FlashPIX library)
            AC_MSG_RESULT([yes])
            have_fpx='yes'
            PERLMAINCC="$CXX"
        fi
    else
        AC_MSG_RESULT([no])
    fi
fi
AM_CONDITIONAL(HasFPX, test "$have_fpx" = 'yes')
AC_SUBST(FPX_LIBS)

dnl ===========================================================================

AC_ARG_WITH([fontconfig],
    [AC_HELP_STRING([--without-fontconfig],
                    [disable fontconfig support])],
        [with_fontconfig=$withval],
        [with_fontconfig=$have_x])

if test "$with_fontconfig" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-fontconfig=$with_fontconfig "
fi

have_fontconfig='no'
FONTCONFIG_CFLAGS=""
FONTCONFIG_LIBS=""
FONTCONFIG_PKG=""
if test "x$with_fontconfig" = "xyes"; then
    PKG_CHECK_MODULES(FONTCONFIG,[fontconfig >= 2.1.0], have_fontconfig=yes, have_fontconfig=no)
fi

if test "$have_fontconfig" = 'yes'; then
    AC_DEFINE(HasFONTCONFIG,1,Define if you have FONTCONFIG library)
    if test "$with_modules" = 'no'; then
    CPPFLAGS="$FONTCONFIG_CFLAGS $CPPFLAGS"
    fi
fi

AM_CONDITIONAL(HasFONTCONFIG,test "$have_fontconfig" = 'yes')
AC_SUBST(FONTCONFIG_CFLAGS)
AC_SUBST(FONTCONFIG_LIBS)

dnl ===========================================================================

AC_ARG_WITH([freetype],
    [AC_HELP_STRING([--without-freetype],
                    [disable Freetype support])],
    [with_freetype=$withval],
    [with_freetype='yes'])


if test "$with_freetype" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-freetype=$with_freetype "
fi

have_freetype='no'
FREETYPE_LIBS=''
if test "$with_freetype" != 'no'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for FreeType 2.0 support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    failed=0;
    passed=0;
    PERSIST_LDFLAGS="$LDFLAGS"
    PERSIST_CPPFLAGS="$CPPFLAGS"
    if test "$enable_delegate_build" != 'no' && test -d "$builddir/freetype/include"; then
        :
    else
        freetype_config=''
        AC_CHECK_PROGS(freetype_config,freetype-config,)dnl
        if test -n "$freetype_config"; then
            freetype_cflags=`$freetype_config --cflags`
            freetype_libs=`$freetype_config --libs`
            LDFLAGS="$LDFLAGS $freetype_libs"
            CPPFLAGS="$freetype_cflags $CPPFLAGS"
        fi
    fi

dnl First see if there is a library
    if test "$FREETYPE_LIBS" = ''; then
        AC_CHECK_LIB(freetype,FT_Init_FreeType,FREETYPE_LIBS='-lfreetype',,)
        if test "$FREETYPE_LIBS" != ''; then
            passed=`expr $passed + 1`
        else
            failed=`expr $failed + 1`
            LDFLAGS="$PERSIST_LDFLAGS"
        fi
    fi

dnl Now test for the headers
    AC_CHECK_HEADER([ft2build.h],[FT2BUILD_H='#include <ft2build.h>'],[ft2build=''],[])
    AC_CHECK_HEADER(freetype/freetype.h,[have_freetype_h='yes'],[have_freetype_h='no'],[$FT2BUILD_H])
    if test "$ac_cv_header_ft2build_h" = 'yes' || test "$have_freetype_h" = 'yes'; then
        passed=`expr $passed + 1`
    else
        failed=`expr $failed + 1`
        CPPFLAGS="$PERSIST_CPPFLAGS"
    fi

    AC_MSG_CHECKING([if FreeType package is complete])
    if test $passed -gt 0; then
        if test $failed -gt 0; then
            FREETYPE_LIBS=''
            AC_MSG_RESULT([no -- some components failed test])
            have_freetype='no (failed tests)'
        else
            LIBS="$FREETYPE_LIBS $LIBS"
            AC_DEFINE(HasFREETYPE,1,Define if you have FreeType (TrueType font) library)
            if test "$ac_cv_header_ft2build_h" = 'yes'; then
                AC_DEFINE([HAVE_FT2BUILD_H],[1],[Define to 1 if you have the <ft2build.h> header file.])
            fi
            AC_MSG_RESULT([yes])
            have_freetype='yes'
        fi
    else
        AC_MSG_RESULT([no])
    fi
fi
AM_CONDITIONAL(HasFREETYPE,test "$have_freetype" = 'yes')
AC_SUBST(FREETYPE_LIBS)

dnl ===========================================================================

AC_ARG_WITH(lcms,
    [AC_HELP_STRING([--without-lcms],
                    [disable LCMS support])],
    [with_lcms=$withval],
    [with_lcms='yes'])

if test "$with_lcms" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-lcms=$with_lcms "
fi


dnl ===========================================================================

AC_ARG_WITH(png,
    [AC_HELP_STRING([--without-png],
                    [disable PNG support])],
    [with_png=$withval],
    [with_png='yes'])

if test "$with_png" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-png=$with_png "
fi

have_png='no'
PNG_LIBS=''
if test "$with_png" != 'no'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for PNG support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    failed=0;
    passed=0;
    AC_CHECK_HEADER(png.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_CHECK_LIB(png,png_get_io_ptr,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_MSG_CHECKING([if PNG package is complete])
    if test $passed -gt 0; then
        if test $failed -gt 0; then
            AC_MSG_RESULT([no -- some components failed test])
            have_png='no (failed tests)'
        else
            PNG_LIBS='-lpng'
            LIBS="$PNG_LIBS $LIBS"
            AC_DEFINE(HasPNG,1,Define if you have PNG library)
            AC_MSG_RESULT([yes])
            have_png='yes'
        fi
    else
        AC_MSG_RESULT([no])
    fi
fi
AM_CONDITIONAL(HasPNG,test "$have_png" = 'yes')
AC_SUBST(PNG_LIBS)

#
# Check for JPEG
#
have_jpeg='no'
JPEG_LIBS=''
if test "$with_jpeg" != 'no'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for JPEG support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    failed=0;
    passed=0;
    AC_CHECK_HEADER(jconfig.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_HEADER(jerror.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_HEADER(jmorecfg.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_HEADER(jpeglib.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_LIB(jpeg,jpeg_read_header,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)

# Test for compatible JPEG library
if test "$ac_cv_jpeg_version_ok" != 'yes'; then
AC_CACHE_CHECK(for JPEG library is version 6b or later, ac_cv_jpeg_version_ok,
[AC_TRY_COMPILE(
#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
,
changequote(<<, >>)dnl
<<
#if JPEG_LIB_VERSION < 62
#error IJG JPEG library must be version 6b or newer!
#endif
return 0;
>>,
changequote([, ])dnl
ac_cv_jpeg_version_ok='yes'; passed=`expr $passed + 1`,
ac_cv_jpeg_version_ok='no'; failed=`expr $failed + 1`)])
fi
    AC_MSG_CHECKING([if JPEG package is complete])
    if test $passed -gt 0; then
        if test $failed -gt 0; then
            AC_MSG_RESULT([no -- some components failed test])
            have_jpeg='no (failed tests)'
        else
            JPEG_LIBS='-ljpeg'
            LIBS="$JPEG_LIBS $LIBS"
            AC_DEFINE(HasJPEG,1,Define if you have JPEG library)
            AC_MSG_RESULT([yes])
            have_jpeg='yes'
        fi
    else
        AC_MSG_RESULT([no])
    fi
fi
AM_CONDITIONAL(HasJPEG, test "$have_jpeg" = 'yes')
AC_SUBST(JPEG_LIBS)

#
# Check for JPEG Version 2
#
have_jp2='no'
JP2_LIBS=''
if test "$with_jp2" != 'no'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for JPEG Version 2 support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    failed=0;
    passed=0;
    AC_CHECK_HEADER(jasper/jasper.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_LIB(jasper,jas_stream_fopen,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_MSG_CHECKING([if JPEG version 2 support package is complete])
    if test $passed -gt 0; then
        if test $failed -gt 0; then
            AC_MSG_RESULT([no -- some components failed test])
            have_jp2='no (failed tests)'
        else
            JP2_LIBS='-ljasper'
            LIBS="$JP2_LIBS $LIBS"
            AC_DEFINE(HasJP2,1,Define if you have JPEG version 2 "Jasper" library)
            AC_MSG_RESULT([yes])
            have_jp2='yes'
        fi
    else
        AC_MSG_RESULT([no])
    fi
fi
AM_CONDITIONAL(HasJP2, test "$have_jp2" = 'yes')
AC_SUBST(JP2_LIBS)

#
# Check for Ghostscript library or framework.
#
# Test for iapi.h & test for gsapi_new_instance in -lgs
# or -framework Ghostscript
have_gslib='no'
GS_LIBS=''
if test "$with_gslib" != 'no'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for Ghostscript support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    framework=0;
    failed=0;
    passed=0;
    AC_CHECK_HEADER(ghostscript/iapi.h,passed=`expr $passed + 1`,
        failed=`expr $failed + 1`,)
    AC_CHECK_HEADER(ghostscript/ierrors.h,passed=`expr $passed + 1`,
        failed=`expr $failed + 1`,)
    AC_CHECK_FRAMEWORK(Ghostscript,gsapi_new_instance,framework=`expr $framework + 1`,
    AC_CHECK_LIB(gs,gsapi_new_instance,passed=`expr $passed + 1`,failed=`expr $failed + 1`,),)
    AC_MSG_CHECKING([if Ghostscript package is complete])
    if test $passed -gt 0; then
        if test $failed -gt 0; then
            AC_MSG_RESULT([no -- some components failed test])
            have_gslib='no (failed tests)'
        else
            if test $framework -gt 0; then
                GS_LIBS='-framework Ghostscript'
                AC_MSG_RESULT([yes, using framework.])
            else
                AC_MSG_RESULT([yes, using library.])
                GS_LIBS='-lgs'
            fi
            LIBS="$GS_LIBS $LIBS"
            AC_DEFINE(HasGS,1,Define if you have Ghostscript library or framework)
            have_gslib='yes'
        fi
    else
        AC_MSG_RESULT([no])
    fi
fi
AM_CONDITIONAL(HasGS, test "$have_gslib" = 'yes')
AC_SUBST(GS_LIBS)

dnl ===========================================================================

AC_ARG_WITH(gvc,
    [AC_HELP_STRING([--without-gvc],
                    [disable GVC support])],
    [with_gvc=$withval],
    [with_gvc='yes'])

if test "$with_gvc" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-gvc=$with_gvc "
fi

GVC_CFLAGS=""
GVC_LIBS=""
GVC_PKG=""
if test "x$with_gvc" = "xyes"; then
    PKG_CHECK_MODULES(GVC,[libgvc >= 2.9.0], have_gvc=yes, have_gvc=no)
fi

if test "$have_gvc" = 'yes'; then
    AC_DEFINE(HasGVC,1,Define if you have GVC library)
    if test "$with_modules" = 'no'; then
        CPPFLAGS="$GVC_CFLAGS $CPPFLAGS"
    fi
fi

AM_CONDITIONAL(HasGVC, test "$have_gvc" = 'yes')
AC_SUBST(GVC_CFLAGS)
AC_SUBST(GVC_LIBS)

#
# Check for LCMS
#
have_lcms='no'
LCMS_LIBS=''
if test "$with_lcms" != 'no'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for LCMS support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    failed=0;
    passed=0;
    have_lcms_header='no'
    AC_CHECK_HEADER(lcms.h,have_lcms_header='yes',,)
    if test "$have_lcms_header" = 'yes'; then
        passed=`expr $passed + 1`
        AC_DEFINE(HAVE_LCMS_H,1,Define if you have the <lcms.h> header file.)
    else
        AC_CHECK_HEADER(lcms/lcms.h,have_lcms_header='yes',,)
        if test "$have_lcms_header" = 'yes'; then
            passed=`expr $passed + 1`
            AC_DEFINE(HAVE_LCMS_LCMS_H,1,Define if you have the <lcms/lcms.h> header file.)
        else
            failed=`expr $failed + 1`
        fi
    fi
    AC_CHECK_LIB(lcms,cmsOpenProfileFromMem,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_MSG_CHECKING([if LCMS package is complete])
    if test $passed -gt 0; then
        if test $failed -gt 0; then
            AC_MSG_RESULT([no -- some components failed test])
            have_lcms='no (failed tests)'
        else
            LCMS_LIBS='-llcms'
            LIBS="$LCMS_LIBS $LIBS"
            AC_DEFINE(HasLCMS,1,Define if you have LCMS library)
            AC_MSG_RESULT([yes])
            have_lcms='yes'
        fi
    else
        AC_MSG_RESULT([no])
    fi
fi
AM_CONDITIONAL(HasLCMS, test "$have_lcms" = 'yes')
AC_SUBST(LCMS_LIBS)

AC_ARG_WITH([openexr],
    [AC_HELP_STRING([--without-openexr],
                    [disable OpenEXR support])],
    [with_openexr=$withval],
    [with_openexr='yes'])

if test "$with_openexr" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-openexr=$with_openexr "
fi

have_openexr='no'
OPENEXR_CFLAGS=""
OPENEXR_LIBS=""
OPENEXR_PKG=""
if test "x$with_openexr" = "xyes"; then
    PKG_CHECK_MODULES(OPENEXR,[OpenEXR >= 1.0.6], have_openexr=yes, have_openexr=no)
fi

if test "$have_openexr" = 'yes'; then
    AC_DEFINE(HasOPENEXR,1,Define if you have OPENEXR library)
    if test "$with_modules" = 'no'; then
        CFLAGS="$OPENEXR_CFLAGS $CFLAGS"
    fi
fi

AM_CONDITIONAL(HasOPENEXR, test "$have_openexr" = 'yes')
AC_SUBST(OPENEXR_CFLAGS)
AC_SUBST(OPENEXR_LIBS)

AC_ARG_WITH([rsvg],
    [AC_HELP_STRING([--without-rsvg],
                    [disable RSVG support])],
    [with_rsvg=$withval],
    [with_rsvg=$have_x])

if test "$with_rsvg" != 'yes'; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-rsvg=$with_rsvg "
fi

have_rsvg='no'
RSVG_CFLAGS=""
RSVG_LIBS=""
RSVG_PKG=""
if test "x$with_rsvg" = "xyes"; then
    PKG_CHECK_MODULES(RSVG,[librsvg-2.0 >= 2.9.0], have_rsvg=yes, have_rsvg=no)
fi

if test "$have_rsvg" = 'yes'; then
    AC_DEFINE(HasRSVG,1,Define if you have RSVG library)
    if test "$with_modules" = 'no'; then
        CPPFLAGS="$RSVG_CFLAGS $CPPFLAGS"
    fi
fi

AM_CONDITIONAL(HasRSVG, test "$have_rsvg" = 'yes')
AC_SUBST(RSVG_CFLAGS)
AC_SUBST(RSVG_LIBS)

#
# Check for TIFF
#
have_tiff='no'
TIFF_LIBS=''
if test "$with_tiff" != 'no'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for TIFF support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    failed=0;
    passed=0;
    AC_CHECK_HEADER(tiff.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_HEADER(tiffio.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_LIB(tiff,TIFFOpen,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_CHECK_LIB(tiff,TIFFClientOpen,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_CHECK_LIB(tiff,TIFFIsByteSwapped,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_CHECK_LIB(tiff,TIFFReadRGBATile,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_CHECK_LIB(tiff,TIFFReadRGBAStrip,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_MSG_CHECKING([if TIFF package is complete])
    if test $passed -gt 0; then
        if test $failed -gt 0; then
            AC_MSG_RESULT([no -- some components failed test])
            have_tiff='no (failed tests)'
        else
            TIFF_LIBS='-ltiff'
            LIBS="$TIFF_LIBS $LIBS"
            AC_DEFINE(HasTIFF,1,Define if you have TIFF library)
            AC_MSG_RESULT([yes])
            have_tiff='yes'
            AC_CHECK_HEADERS(tiffconf.h)
            AC_CHECK_FUNCS([TIFFMergeFieldInfo TIFFSetTagExtender])
        fi
    else
        AC_MSG_RESULT([no])
    fi
fi
AM_CONDITIONAL(HasTIFF, test "$have_tiff" = 'yes')
AC_SUBST(TIFF_LIBS)

#
# Check for JBIG
#
have_jbig='no'
JBIG_LIBS=''
if test "$with_jbig" != 'no'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for JBIG support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    failed=0;
    passed=0;
    AC_CHECK_HEADER(jbig.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_LIB(jbig,jbg_dec_init,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_MSG_CHECKING([if JBIG package is complete])
    if test $passed -gt 0; then
        if test $failed -gt 0; then
            AC_MSG_RESULT([no -- some components failed test])
            have_jbig='no (failed tests)'
        else
            JBIG_LIBS='-ljbig'
            LIBS="$JBIG_LIBS $LIBS"
            AC_DEFINE(HasJBIG,1,Define if you have JBIG library)
            AC_MSG_RESULT([yes])
            have_jbig='yes'
        fi
    else
        AC_MSG_RESULT([no])
    fi
fi
AM_CONDITIONAL(HasJBIG, test "$have_jbig" = 'yes')
AC_SUBST(JBIG_LIBS)

dnl ===========================================================================

AC_ARG_WITH([xml],
    [AC_HELP_STRING([--without-xml],
                    [disable XML support])],
    [with_xml=$withval],
    [with_xml=$have_x])

if test "$with_xml" != 'yes' ; then
    DISTCHECK_CONFIG_FLAGS="${DISTCHECK_CONFIG_FLAGS} --with-xml=$with_xml "
fi

have_xml='no'
XML_LIBS=''
if test "$with_xml" != 'no'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for XML support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    PERSIST_LDFLAGS=$LDFLAGS
    PERSIST_CPPFLAGS=$CPPFLAGS
    xml2_config=''
    AC_CHECK_PROGS(xml2_config,xml2-config,)dnl
    if test -n "$xml2_config"; then
        # Debian installs libxml headers under /usr/include/libxml2/libxml with
        # the shared library installed under /usr/lib, whereas the package
        # installs itself under $prefix/libxml and $prefix/lib.
        xml2_prefix=`xml2-config --prefix`
        if test -d "${xml2_prefix}/include/libxml2"; then
            CPPFLAGS="$CPPFLAGS -I${xml2_prefix}/include/libxml2"
        fi
        if test "${xml2_prefix}" != '/usr'; then
            LDFLAGS="$LDFLAGS -L${xml2_prefix}/lib"
        fi
    fi
    failed=0;
    passed=0;
    AC_CHECK_HEADER(libxml/parser.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_LIB(xml2,xmlParseExternalEntity,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_MSG_CHECKING([if XML package is complete])
    if test $passed -gt 0; then
        if test $failed -gt 0; then
            AC_MSG_RESULT([no -- some components failed test])
            have_xml='no (failed tests)'
            LDFLAGS="$PERSIST_LDFLAGS"
            CPPFLAGS="$PERSIST_CPPFLAGS"
        else
            XML_LIBS='-lxml2'
            LIBS="$XML_LIBS $LIBS"
            AC_DEFINE(HasXML,1,Define if you have XML library)
            AC_MSG_RESULT([yes])
            have_xml='yes'
        fi
    else
        AC_MSG_RESULT([no])
    fi
fi
AM_CONDITIONAL(HasXML,test "$have_xml" = 'yes')
AC_SUBST(XML_LIBS)

#
# Check for WMF
#

have_wmf='no'
WMF_LIBS=''
WMF_LIBS_DEPS=''
OLIBS="$LIBS"
if test "$with_wmf" != 'no'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for WMF support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    have_libwmf='no'
    have_libwmflite='no'
    have_libwmf_ipa_h='no'

    AC_CHECK_HEADER([libwmf/ipa.h],[have_libwmf_ipa_h='yes'],,[$FT2BUILD_H])
    if test "$have_libwmf_ipa_h" = 'yes'; then
        AC_CHECK_LIB(wmflite,wmf_lite_create,have_libwmflite='yes',,)
        if test "$have_libwmflite" = 'yes'; then
            AC_DEFINE(HasWMFlite,1,Define if you have wmflite library)
            WMF_LIBS='-lwmflite'
            LIBS="$WMF_LIBS $LIBS"
            have_wmf='yes'
        else
            WMF_LIBS_DEPS=''
            WMF_CONFIG_LIBS=`libwmf-config --libs`
            for lib in xml2 expat freetype jpeg png z; do
                testlib="-l${lib}"
                echo "$WMF_CONFIG_LIBS" | grep -- "$testlib" > /dev/null && WMF_LIBS_DEPS="$WMF_LIBS_DEPS $testlib"
            done
            AC_CHECK_LIB(wmf,wmf_api_create,have_libwmf='yes',,$WMF_LIBS_DEPS)
            if test "$have_libwmf" = 'yes'; then
                AC_DEFINE(HasWMF,1,Define if you have wmf library)
                WMF_LIBS='-lwmf'
                LIBS="$WMF_LIBS $LIBS"
                have_wmf='yes'
            else
                AC_MSG_RESULT([no -- some components failed test])
                have_wmf='no (failed tests)'
                have_wmflite='no (failed tests)'
                LIBS="$OLIBS"
                WMF_LIBS=''
            fi
        fi
    fi
    AC_MSG_CHECKING([if WMF package is complete])
    if test "$have_wmf" = 'yes'; then
        AC_MSG_RESULT([yes])
    else
        AC_MSG_RESULT([no])
    fi
fi
AM_CONDITIONAL(HasWMF, test "$have_wmf" = 'yes')
AC_SUBST(WMF_LIBS)
AC_SUBST(WMF_LIBS_DEPS)

# Substitute compiler name to build/link PerlMagick
#
AC_SUBST(PERLMAINCC)

#
# Configure install Paths
#

# Subdirectory under lib to place ImageMagick lib files
MagickLibSubdir="${PACKAGE_NAME}-${PACKAGE_VERSION}"
AC_DEFINE_UNQUOTED(MagickLibSubdir,"$MagickLibSubdir",Subdirectory of lib where ImageMagick architecture dependent files are installed)

# Path to ImageMagick bin directory
MagickBinPath="${BIN_DIR}"
MagickBinPathDefine="${MagickBinPath}/"
if test "$native_win32_build" = 'yes'; then
    MagickBinPathDefine=`$WinPathScript "$MagickBinPathDefine" 1`
fi
AC_DEFINE_UNQUOTED(MagickBinPath,"$MagickBinPathDefine",Directory where executables are installed.)
AC_SUBST(MagickBinPath)

# Path to ImageMagick lib
MagickLibPath="${LIB_DIR}/${MagickLibSubdir}"
MagickLibPathDefine="${MagickLibPath}/"
if test "$native_win32_build" = 'yes'; then
    MagickLibPathDefine=`$WinPathScript "$MagickLibPathDefine" 1`
fi
AC_DEFINE_UNQUOTED(MagickLibPath,"$MagickLibPathDefine",Directory where architecture-dependent files live.)
AC_SUBST(MagickLibPath)

# Subdirectory under lib to place ImageMagick configuration files
MagickLibConfigSubDir="${MagickLibSubdir}/config"
AC_DEFINE_UNQUOTED(MagickLibConfigSubDir,"$MagickLibConfigSubDir",Subdirectory of lib where architecture-dependent configuration files live.)
MagickLibConfigPath="${LIB_DIR}/${MagickLibConfigSubDir}"
MagickLibConfigPathDefine="${MagickLibConfigPath}/"
if test "$native_win32_build" = 'yes'; then
    MagickLibConfigPathDefine=`$WinPathScript "$MagickLibConfigPathDefine" 1`
fi
AC_DEFINE_UNQUOTED(MagickLibConfigPath,"$MagickLibConfigPathDefine",Directory where architecture-dependent configuration files live.)
AC_SUBST(MagickLibConfigPath)

#
# Subdirectory under lib to place ImageMagick coder module files
MagickImageCoderModulesSubdir="${MagickLibSubdir}/modules-Q${QuantumDepth}/coders"
AC_DEFINE_UNQUOTED(MagickImageCoderModulesSubdir,"$MagickImageCoderModulesSubdir",Subdirectory of lib where coder modules are installed)
MagickImageCodersPath="${LIB_DIR}/${MagickImageCoderModulesSubdir}"
MagickImageCodersPathDefine="${MagickImageCodersPath}/"
if test "$native_win32_build" = 'yes'; then
    MagickImageCodersPathDefine=`$WinPathScript "$MagickImageCodersPathDefine" 1`
fi
AC_DEFINE_UNQUOTED(MagickImageCodersPath,"$MagickImageCodersPathDefine",Location of coder modules)
AC_SUBST(MagickImageCodersPath)

#
# Subdirectory under lib to place ImageMagick filter module files
MagickImageFilterModulesSubdir="${MagickLibSubdir}/modules-Q${QuantumDepth}/filters"
AC_DEFINE_UNQUOTED(MagickImageFilterModulesSubdir,"$MagickImageFilterModulesSubdir",Subdirectory of lib where filter modules are installed)
MagickImageFiltersPath="${LIB_DIR}/${MagickImageFilterModulesSubdir}"
MagickImageFiltersPathDefine="${MagickImageFiltersPath}/"
if test "$native_win32_build" = 'yes'; then
    MagickImageFiltersPathDefine=`$WinPathScript "$MagickImageFiltersPathDefine" 1`
fi
AC_DEFINE_UNQUOTED(MagickImageFiltersPath,"$MagickImageFiltersPathDefine",Location of filter modules)
AC_SUBST(MagickImageFiltersPath)

#
# Path to ImageMagick document files
MagickDocumentSubdir="${PACKAGE_NAME}-${PACKAGE_VERSION}"
MagickDocumentPath="${DATA_DIR}/doc/${MagickDocumentSubdir}"
MagickDocumentPathDefine="${MagickDocumentPath}/"
if test "$native_win32_build" = 'yes'; then
    MagickDocumentPathDefine=`$WinPathScript "$MagickDocumentPathDefine" 1`
fi
AC_DEFINE_UNQUOTED(MagickDocumentPath,"$MagickDocumentPathDefine",Directory where ImageMagick documents live.)
AC_SUBST(MagickDocumentPath)

#
# Path to ImageMagick share files
MagickShareSubdir="${PACKAGE_NAME}-${PACKAGE_VERSION}"
MagickSharePath="${DATA_DIR}/${MagickShareSubdir}"
MagickSharePathDefine="${MagickSharePath}/"
if test "$native_win32_build" = 'yes'; then
    MagickSharePathDefine=`$WinPathScript "$MagickSharePathDefine" 1`
fi
AC_DEFINE_UNQUOTED(MagickSharePath,"$MagickSharePathDefine",Directory where architecture-independent files live.)
AC_SUBST(MagickSharePath)

# Subdirectory under share to place ImageMagick configuration files
MagickShareConfigSubDir="${MagickLibSubdir}/config"
AC_DEFINE_UNQUOTED(MagickShareConfigSubDir,"$MagickShareConfigSubDir",Subdirectory of lib where architecture-independent configuration files live.)
MagickShareConfigPath="${DATA_DIR}/${MagickShareConfigSubDir}"
MagickShareConfigPathDefine="${MagickShareConfigPath}/"
if test "$native_win32_build" = 'yes'; then
    MagickShareConfigPathDefine=`$WinPathScript "$MagickShareConfigPathDefine" 1`
fi
AC_DEFINE_UNQUOTED(MagickShareConfigPath,"$MagickShareConfigPathDefine",Directory where architecture-independent configuration files live.)
AC_SUBST(MagickShareConfigPath)

#
# program_transform_name is formed for use in a Makefile, so create a
# modified version for use in a shell script.
configure_transform_name=`echo ${program_transform_name} | sed 's,\\$\\$,$,'`

# Default delegate definitions
AC_MSG_RESULT([-------------------------------------------------------------])
AC_MSG_RESULT([Checking for ImageMagick delegate programs])
AC_MSG_RESULT([-------------------------------------------------------------])
AutotraceDecodeDelegateDefault='autotrace'
BZIPDelegateDefault='bzip2'
BrowseDelegateDefault='htmlview'
CGMDecodeDelegateDefault='ralcgm'
CatDelegateDefault='cat'
DNGDecodeDelegateDefault='ufraw-batch'
GVCDecodeDelegateDefault='dot'
DVIDecodeDelegateDefault='dvips'
EchoDelegateDefault='echo'
EditorDelegateDefault='xterm'
FIGDecodeDelegateDefault='fig2dev'
ConvertDelegateDefault=`echo convert | sed ${configure_transform_name}`
DisplayDelegateDefault=`echo display | sed ${configure_transform_name}`
MogrifyDelegateDefault=`echo mogrify | sed ${configure_transform_name}`
GnuplotDecodeDelegateDefault='gnuplot'
HDRDecodeDelegateDefault='ra_pfm'
HPGLDecodeDelegateDefault='hp2xx'
HTMLDecodeDelegateDefault='html2ps'
ILBMDecodeDelegateDefault='ilbmtoppm'
ILBMEncodeDelegateDefault='ppmtoilbm'
LPDelegateDefault='lp'
LPRDelegateDefault='lpr'
LZWDecodeDelegateDefault='uncompress'
LZWEncodeDelegateDefault='compress'
LaunchDelegateDefault='gimp'
MANDelegateDefault='groff'
MPEGDecodeDelegateDefault='mpeg2decode'
MPEGEncodeDelegateDefault='mpeg2encode'
MVDelegateDefault='mv'
PCLDelegateDefault='pcl6'
PGPDecodeDelegateDefault='pgpv'
POVDelegateDefault='povray'
if test "$native_win32_build" = 'yes'; then
    PSDelegateDefault='gswin32c'
elif test $framework -gt 0; then
    PSDelegateDefault='gsc'
else
    PSDelegateDefault='gs'
fi
RLEEncodeDelegateDefault='rawtorle'
RMDelegateDefault='rm'
SCANDecodeDelegateDefault='scanimage'
TXTDelegateDefault='enscript'
WMFDecodeDelegateDefault='wmf2svg'
WWWDecodeDelegateDefault='wget'
ZipDelegateDefault='gzip'

# Search for delegates
AC_PATH_PROG(AutotraceDecodeDelegate, "$AutotraceDecodeDelegateDefault", "$AutotraceDecodeDelegateDefault")
AC_PATH_PROG(BZIPDelegate, "$BZIPDelegateDefault", "$BZIPDelegateDefault")
AC_PATH_PROG(BrowseDelegate, "$BrowseDelegateDefault" mozilla firefox netscape, "$BrowseDelegateDefault")
AC_PATH_PROG(CGMDecodeDelegate, "$CGMDecodeDelegateDefault", "$CGMDecodeDelegateDefault")
AC_PATH_PROG(CatDelegate, "$CatDelegateDefault", "$CatDelegateDefault")
AC_PATH_PROG(DNGDecodeDelegate, "$DNGDecodeDelegateDefault", "$DNGDecodeDelegateDefault")
AC_PATH_PROG(GVCDecodeDelegate, "$GVCDecodeDelegateDefault", "$GVCDecodeDelegateDefault")
AC_PATH_PROG(DVIDecodeDelegate, "$DVIDecodeDelegateDefault", "$DVIDecodeDelegateDefault")
AC_PATH_PROG(EchoDelegate, "$EchoDelegateDefault", "$EchoDelegateDefault")
AC_PATH_PROG(EditorDelegate, "$EditorDelegateDefault", "$EditorDelegateDefault")
AC_PATH_PROG(FIGDecodeDelegate, "$FIGDecodeDelegateDefault", "$FIGDecodeDelegateDefault")
AC_PATH_PROG(ConvertDelegate, "$ConvertDelegateDefault", "$ConvertDelegateDefault")
AC_PATH_PROG(DisplayDelegate, "$DisplayDelegateDefault", "$DisplayDelegateDefault")
AC_PATH_PROG(MogrifyDelegate, "$MogrifyDelegateDefault", "$MogrifyDelegateDefault")
AC_PATH_PROG(GnuplotDecodeDelegate, "$GnuplotDecodeDelegateDefault", "$GnuplotDecodeDelegateDefault")
AC_PATH_PROG(HDRDecodeDelegate, "$HDRDecodeDelegateDefault", "$HDRDecodeDelegateDefault")
AC_PATH_PROG(HPGLDecodeDelegate, "$HPGLDecodeDelegateDefault", "$HPGLDecodeDelegateDefault")
AC_PATH_PROG(HTMLDecodeDelegate, "$HTMLDecodeDelegateDefault", "$HTMLDecodeDelegateDefault")
AC_PATH_PROG(ILBMDecodeDelegate, "$ILBMDecodeDelegateDefault", "$ILBMDecodeDelegateDefault")
AC_PATH_PROG(ILBMEncodeDelegate, "$ILBMEncodeDelegateDefault", "$ILBMEncodeDelegateDefault")
AC_PATH_PROG(LPDelegate, "$LPDelegateDefault", no)
AC_PATH_PROG(LPRDelegate, "$LPRDelegateDefault", "$LPRDelegateDefault")
AC_PATH_PROG(LZWDecodeDelegate, "$LZWDecodeDelegateDefault", "$LZWDecodeDelegateDefault")
AC_PATH_PROG(LZWEncodeDelegate, "$LZWEncodeDelegateDefault", "$LZWEncodeDelegateDefault")
AC_PATH_PROG(LaunchDelegate, "$LaunchDelegateDefault", "$LaunchDelegateDefault")
AC_PATH_PROG(MANDelegate, "$MANDelegateDefault", "$MANDelegateDefault")
AC_PATH_PROG(MPEGDecodeDelegate, "$MPEGDecodeDelegateDefault", "$MPEGDecodeDelegateDefault")
AC_PATH_PROG(MPEGEncodeDelegate, "$MPEGEncodeDelegateDefault", "$MPEGEncodeDelegateDefault")
AC_PATH_PROG(MVDelegate, "$MVDelegateDefault", "$MVDelegateDefault")
AC_PATH_PROG(PCLDelegate, "$PCLDelegateDefault", "$PCLDelegateDefault")
AC_PATH_PROG(PGPDecodeDelegate, "$PGPDecodeDelegateDefault", "$PGPDecodeDelegateDefault")
AC_PATH_PROG(POVDelegate, "$POVDelegateDefault", "$POVDelegateDefault")
AC_PATH_PROGS(PSDelegate, gsx gsc "$PSDelegateDefault", "$PSDelegateDefault")
AC_PATH_PROG(RLEEncodeDelegate, "$RLEEncodeDelegateDefault", "$RLEEncodeDelegateDefault")
AC_PATH_PROG(RMDelegate, "$RMDelegateDefault", "$RMDelegateDefault")
AC_PATH_PROG(SCANDecodeDelegate, "$SCANDecodeDelegateDefault", "$SCANDecodeDelegateDefault")
AC_PATH_PROG(TXTDelegate, "$TXTDelegateDefault", "$TXTDelegateDefault")
AC_PATH_PROG(WMFDecodeDelegate, "$WMFDecodeDelegateDefault", "$WMFDecodeDelegateDefault")
AC_PATH_PROG(WWWDecodeDelegate, "$WWWDecodeDelegateDefault", "$WWWDecodeDelegateDefault")
AC_PATH_PROG(ZipDelegate, "$ZipDelegateDefault", "$ZipDelegateDefault")

# Prefer lpr to lp; lp needs options tacked on.
if test "$LPRDelegate" != no; then
    PrintDelegate="$LPRDelegate"
else
    PrintDelegate="$LPDelegate -c -s"
fi
AC_SUBST(PrintDelegate)

# Installed ImageMagick utiltity paths
ConvertDelegate="${BIN_DIR}/${ConvertDelegateDefault}"
DisplayDelegate="${BIN_DIR}/${DisplayDelegateDefault}"
MogrifyDelegate="${BIN_DIR}/${MogrifyDelegateDefault}"

# Set delegate booleans
have_fig2dev='no'    ; if test "$FIGDecodeDelegate" != "$FIGDecodeDelegateDefault" ; then have_fig2dev='yes'; fi
have_gs='no'         ; if test "$PSDelegate" != "$PSDelegateDefault"; then have_gs='yes'; fi
have_hp2xx='no'      ; if test "$HPGLDecodeDelegate" !=  "$HPGLDecodeDelegateDefault" ; then have_hp2xx='yes'; fi
have_ilbmtoppm='no'  ; if test "$ILBMDecodeDelegate" != "$ILBMDecodeDelegateDefault" ; then have_ilbmtoppm='yes'; fi
have_pcl='no'        ; if test "$PCLDelegate" != "$PCLDelegateDefault"; then have_pcl='yes'; fi
have_ppmtoilbm='no'  ; if test "$ILBMEncodeDelegate" != "$ILBMEncodeDelegateDefault" ; then have_ppmtoilbm='yes'; fi
have_mpeg2decode='no'; if test "$MPEGDecodeDelegate" != "$MPEGDecodeDelegateDefault" ; then have_mpeg2decode='yes'; fi
have_mpeg2encode='no'; if test "$MPEGEncodeDelegate" != "$MPEGEncodeDelegateDefault" ; then have_mpeg2encode='yes'; fi
have_ra_pfm='no'     ; if test "$HDRDecodeDelegate" != "$HDRDecodeDelegateDefault" ; then have_ra_pfm='yes'; fi
have_ralcgm='no'     ; if test "$CGMDecodeDelegate" != "$CGMDecodeDelegateDefault" ; then have_ralcgm='yes'; fi

# Test for optional txt2html utility and define automake conditional HasTXT2HTML if found.
AC_PATH_PROG(TXT2HTML, txt2html, txt2html)
have_txt2html='no' ; if test "$TXT2HTML" != 'txt2html'; then have_txt2html='yes' ; fi
AM_CONDITIONAL(HasTXT2HTML, test "$have_txt2html" = 'yes')

#
# Test for font directories
#
type_include_files=''

# Windows
windows_font_dir=''
if test "$with_windows_font_dir" != "yes" && test -n "$with_windows_font_dir"; then
   windows_font_dir="${with_windows_font_dir}/"
fi
if test -n "$windows_font_dir"; then
    if test -f '/usr/X11R6/lib/X11/fonts/truetype/arial.ttf'; then
     windows_font_dir='/usr/X11R6/lib/X11/fonts/truetype/'
    fi
fi
if test -n "$windows_font_dir"; then
  type_include_files="$type_include_files "'<include file="type-windows.xml" />'
fi
AC_SUBST(windows_font_dir)

# Adobe Postscript fonts on various systems
case $host_os in
  solaris*) type_include_files="$type_include_files "'<include file="type-solaris.xml" />';;
esac

# Ghostscript
AC_MSG_CHECKING([for Ghostscript fonts directory])
ghostscript_font_dir=''
if test "${with_gs_font_dir}" != 'default'; then
  ghostscript_font_dir="${with_gs_font_dir}/"

else
    if test "${native_win32_build}" = 'yes'; then
        for font_dir in "c:\\Program Files\\gs\\fonts\\" "c:\\gs\\fonts\\"
        do
          if test -f "${font_dir}a010013l.pfb"
          then
            ghostscript_font_dir="$font_dir"
            break 1
          fi
        done
        if test "${PSDelegate}" != 'gswin32c'; then
            ghostscript_font_dir=`echo "${PSDelegate}" | sed -e 's:/gs/.*:/gs:;s:^/::;s/./&:/;s:/:\\\\:g'`"\\fonts\\"
        fi
    else
        # Red Hat Linux puts Ghostscript fonts in /usr/share/fonts/default/Type1
        for font_dir in "${prefix}/share/ghostscript/fonts/" '/usr/share/fonts/default/Type1/' '/usr/share/ghostscript/fonts/' '/usr/share/fonts/ghostscript/' '/sw/share/ghostscript/fonts' '/System/Library/Frameworks/Ghostscript.framework/Resources/fonts'; do
            if test -f "${font_dir}a010013l.pfb"; then
                ghostscript_font_dir="${font_dir}"
                break 1
            fi
        done

        if test "${ghostscript_font_dir}x" = 'x'; then
            if test "$PSDelegate" != "$PSDelegateDefault"; then
                ghostscript_font_dir=`echo "$PSDelegate" | sed -e 's:/'$(basename $(dirname $PSDelegate))'/'$(basename $PSDelegate)':/share/ghostscript/fonts:'`"/"
            fi
        fi
    fi
fi
if test "${ghostscript_font_dir}x" != 'x'; then
    type_include_files="${type_include_files} "'<include file="type-ghostscript.xml" />'
    AC_MSG_RESULT([$ghostscript_font_dir])
else
    AC_MSG_RESULT([not found!]);
fi
AC_SUBST(ghostscript_font_dir)
if test "${native_win32_build}" = 'yes'; then
    PSDelegate=`$WinPathScript "$PSDelegate" 1`
fi

AC_SUBST(type_include_files)

#
# Handle case where user doesn't want frozen paths
#
if test "$with_frozenpaths" != 'yes'; then
    # Re-set delegate definitions to default (no paths)
    AutotraceDecodeDelegate="$AutotraceDecodeDelegateDefault"
    BZIPDelegate="$BZIPDelegateDefault"
    BrowseDelegate="$BrowseDelegateDefault"
    CGMDecodeDelegate="$CGMDecodeDelegateDefault"
    CatDelegate="$CatDelegateDefault"
    ConvertDelegate="$ConvertDelegateDefault"
    GVCDecodeDelegate="$GVCDecodeDelegateDefault"
    DVIDecodeDelegate="$DVIDecodeDelegateDefault"
    EchoDelegate="$EchoDelegateDefault"
    EditorDelegate="$EditorDelegateDefault"
    FIGDecodeDelegate="$FIGDecodeDelegateDefault"
    GnuplotDecodeDelegate="$GnuplotDecodeDelegateDefault"
    HPGLDecodeDelegate="$HPGLDecodeDelegateDefault"
    HTMLDecodeDelegate="$HTMLDecodeDelegateDefault"
    ILBMDecodeDelegate="$ILBMDecodeDelegateDefault"
    ILBMEncodeDelegate="$ILBMEncodeDelegateDefault"
    LPDelegate="$LPDelegateDefault"
    LZWDecodeDelegate="$LZWDecodeDelegateDefault"
    LZWEncodeDelegate="$LZWEncodeDelegateDefault"
    LaunchDelegate="$LaunchDelegateDefault"
    MANDelegate="$MANDelegateDefault"
    MPEGDecodeDelegate="$MPEGDecodeDelegateDefault"
    MPEGEncodeDelegate="$MPEGEncodeDelegateDefault"
    MVDelegate="$MVDelegateDefault"
    MogrifyDelegate="$MogrifyDelegateDefault"
    PCLDelegate="$PCLDelegateDefault"
    PGPDecodeDelegate="$PGPDecodeDelegateDefault"
    POVDelegate="$POVDelegateDefault"
    PSDelegate="$PSDelegateDefault"
    HDRDecodeDelegate="$HDRDecodeDelegateDefault"
    RLEEncodeDelegate="$RLEEncodeDelegateDefault"
    RMDelegate="$RMDelegateDefault"
    SCANDecodeDelegate="$SCANDecodeDelegateDefault"
    ShowImageDelegate="$ShowImageDelegateDefault"
    TXTDelegate="$TXTDelegateDefault"
    WMFDecodeDelegate="$WMFDecodeDelegateDefault"
    WWWDecodeDelegate="$WWWDecodeDelegateDefault"
    ZipDelegate="$ZipDelegateDefault"
fi

# Delegate substitutions
AC_SUBST(AutotraceDecodeDelegate)
AC_SUBST(BZIPDelegate)
AC_SUBST(BrowseDelegate)
AC_SUBST(CGMDecodeDelegate)
AC_SUBST(CatDelegate)
AC_SUBST(ConvertDelegate)
AC_SUBST(GVCDecodeDelegate)
AC_SUBST(DVIDecodeDelegate)
AC_SUBST(EchoDelegate)
AC_SUBST(EditorDelegate)
AC_SUBST(FIGDecodeDelegate)
AC_SUBST(GnuplotDecodeDelegate)
AC_SUBST(HDRDecodeDelegate)
AC_SUBST(HPGLDecodeDelegate)
AC_SUBST(HTMLDecodeDelegate)
AC_SUBST(ILBMDecodeDelegate)
AC_SUBST(ILBMEncodeDelegate)
AC_SUBST(LPDelegate)
AC_SUBST(LZWDecodeDelegate)
AC_SUBST(LZWEncodeDelegate)
AC_SUBST(LaunchDelegate)
AC_SUBST(MANDelegate)
AC_SUBST(MPEGDecodeDelegate)
AC_SUBST(MPEGEncodeDelegate)
AC_SUBST(MVDelegate)
AC_SUBST(MogrifyDelegate)
AC_SUBST(PCLDelegate)
AC_SUBST(PGPDecodeDelegate)
AC_SUBST(POVDelegate)
AC_SUBST(PSDelegate)
AC_SUBST(RLEEncodeDelegate)
AC_SUBST(RMDelegate)
AC_SUBST(SCANDecodeDelegate)
AC_SUBST(ShowImageDelegate)
AC_SUBST(TXTDelegate)
AC_SUBST(WMFDecodeDelegate)
AC_SUBST(WWWDecodeDelegate)
AC_SUBST(ZipDelegate)

#
# RPM support.
#
RPM=''
AC_CHECK_PROGS(TAR,gnutar gtar tar)
AC_CHECK_PROGS(PERL,perl)
AC_CHECK_PROGS(RPM,rpmbuild rpm)
AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)
AC_SUBST(RPM)
AM_CONDITIONAL(HAS_RPM, test "x$RPM" != "x" )

#
# GhostPCL related configuration.
#
PCLColorDevice=ppmraw
PCLCMYKDevice=bmpsep8
PCLMonoDevice=pbmraw
if test -z "$PCLVersion"; then
    PCLVersion='unknown'
fi
if test $have_pcl = 'yes'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for PCL support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    # PCLColorDevice
    AC_MSG_CHECKING([for pcl color device])
    if $PCLDelegate -dBATCH -sDEVICE=$PCLColorDevice -sOutputFile=/dev/null < /dev/null 2> /dev/null; then
        :
    else
        PCLColorDevice=ppmraw
    fi
    AC_MSG_RESULT([$PCLColorDevice])

    # PCLCMYKDevice
    AC_MSG_CHECKING([for pcl CMYK device])
    if $PCLDelegate -dBATCH -sDEVICE=$PCLColorDevice -sOutputFile=/dev/null < /dev/null 2> /dev/null; then
        :
    else
        PCLCMYKDevice=$PCLColorDevice
    fi
    AC_MSG_RESULT([$PCLCMYKDevice])

    # PCLMonoDevice
    AC_MSG_CHECKING([for pcl mono device])
    if $PCLDelegate -dBATCH -sDEVICE=$PCLMonoDevice -sOutputFile=/dev/null < /dev/null 2> /dev/null; then
        :
    else
        PCLMonoDevice=$PCLColorDevice
    fi
    AC_MSG_RESULT([$PCLMonoDevice])
fi

AC_SUBST(PCLMonoDevice)
AC_SUBST(PCLColorDevice)
AC_SUBST(PCLCMYKDevice)
AC_SUBST(PCLVersion)

#
# Ghostscript related configuration.
#
if test "$have_png" = 'yes'; then
    GSAlphaDevice=pngalpha
else
    GSAlphaDevice=pnmraw
fi
GSColorDevice=pnmraw
GSCMYKDevice=pam
GSMonoDevice=pnmraw
GSPDFDevice=pdfwrite
GSPSDevice=pswrite
GSEPSDevice=epswrite
GSVersion='unknown'
if test $have_gs = 'yes'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for Ghostscript support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_CHECKING([for Ghostscript version])
    if GSVersion=`$PSDelegate --version`; then
        :
    else
        GSVersion=`$PSDelegate --help | sed -e '1q' | awk '{ print $3 }'`
    fi
    AC_MSG_RESULT([$GSVersion])

    # GSAlphaDevice
    AC_MSG_CHECKING([for gs alpha device])
    if $PSDelegate -q -dBATCH -sDEVICE=$GSAlphaDevice -sOutputFile=/dev/null < /dev/null 2> /dev/null; then
        :
    else
        GSAlphaDevice=pnmraw
    fi
    AC_MSG_RESULT([$GSAlphaDevice])

    # GSColorDevice
    AC_MSG_CHECKING([for gs color device])
    if $PSDelegate -q -dBATCH -sDEVICE=$GSColorDevice -sOutputFile=/dev/null < /dev/null 2> /dev/null; then
        :
    else
        GSColorDevice=pnmraw
    fi
    AC_MSG_RESULT([$GSColorDevice])

    # GSCMYKDevice
    AC_MSG_CHECKING([for gs CMYK device])
    if $PSDelegate -q -dBATCH -sDEVICE=$GSCMYKDevice -sOutputFile=/dev/null < /dev/null 2> /dev/null; then
        :
    else
        GSCMYKDevice=bmpsep8
    fi
    AC_MSG_RESULT([$GSCMYKDevice])

    # GSMonoDevice
    AC_MSG_CHECKING([for gs mono device])
    if $PSDelegate -q -dBATCH -sDEVICE=$GSMonoDevice -sOutputFile=/dev/null < /dev/null 2> /dev/null; then
        :
    else
        GSMonoDevice=$GSColorDevice
    fi
    AC_MSG_RESULT([$GSMonoDevice])

    # GSPDFDevice
    AC_MSG_CHECKING([for gs PDF writing device])
    if $PSDelegate -q -dBATCH -sDEVICE=$GSPDFDevice -sOutputFile=/dev/null < /dev/null 2> /dev/null; then
        :
    else
        GSPDFDevice=nodevice
    fi
        AC_MSG_RESULT([$GSPDFDevice])

    # GSPSDevice
    AC_MSG_CHECKING([for gs PS writing device])
    if $PSDelegate -q -dBATCH -sDEVICE=$GSPSDevice -sOutputFile=/dev/null < /dev/null 2> /dev/null; then
        :
    else
        GSPSDevice=nodevice
    fi
    AC_MSG_RESULT([$GSPSDevice])

    # GSEPSDevice
    AC_MSG_CHECKING([for gs EPS writing device])
    if $PSDelegate -q -dBATCH -sDEVICE=$GSEPSDevice -sOutputFile=/dev/null < /dev/null 2> /dev/null; then
        :
    else
        GSEPSDevice=nodevice
    fi
    AC_MSG_RESULT([$GSEPSDevice])
fi

AC_SUBST(GSAlphaDevice)
AC_SUBST(GSCMYKDevice)
AC_SUBST(GSColorDevice)
AC_SUBST(GSEPSDevice)
AC_SUBST(GSMonoDevice)
AC_SUBST(GSPDFDevice)
AC_SUBST(GSPSDevice)
AC_SUBST(GSVersion)

#
# PerlMagick-related configuration
#

# Look for PERL if PerlMagick requested
# If name/path of desired PERL interpreter is specified, look for that one first
have_perl='no'
if test "$with_perl" != 'no'; then
    AC_MSG_RESULT([-------------------------------------------------------------])
    AC_MSG_RESULT([Checking for Perl support])
    AC_MSG_RESULT([-------------------------------------------------------------])
    if test "$with_perl" != 'yes'; then
        AC_CACHE_CHECK(for perl,ac_cv_path_PERL,ac_cv_path_PERL="$with_perl");
        PERL=$ac_cv_path_PERL
        AC_SUBST(PERL)dnl
        have_perl="$ac_cv_path_PERL"
    else
        AC_PATH_PROGS(PERL,perl perl5,)dnl
        if test "$ac_cv_path_PERL"; then
            have_perl="$ac_cv_path_PERL"
        fi
    fi
fi

with_perl_static='no'
with_perl_dynamic='no'
if test "$have_perl" != 'no'; then
    if test "$with_perl" != 'no' && test "$libtool_build_shared_libs" = 'no'; then
        with_perl_static='yes'
    fi
    if test "$with_perl" != 'no' && test "$libtool_build_shared_libs" = 'yes'; then
        with_perl_dynamic='yes'
    fi
    # Is PERL's MakeMaker new enough to support DESTDIR?
    AC_PROG_PERL_VERSION(5.8.1,[PERL_SUPPORTS_DESTDIR='yes'],[PERL_SUPPORTS_DESTDIR='no'])
fi
AM_CONDITIONAL(WITH_PERL, test "$have_perl" != 'no')
AM_CONDITIONAL(WITH_PERL_STATIC, test $with_perl_static = 'yes')
AM_CONDITIONAL(WITH_PERL_DYNAMIC, test $with_perl_dynamic = 'yes')
AC_SUBST(PERL_SUPPORTS_DESTDIR)

# Determine path to pick up Magick library from for use with building PerlMagick
MAGICKLIBDIR="${LIB_DIR}"
MAGICKLIB="-L${MAGICKLIBDIR} -lMagick"
if test $with_perl_static = 'yes'; then
    # Find out where libtool hides its uninstalled libraries (as libtool_objdir)
    libtool_objdir=$objdir

    # Linker search path to library, followed by -lMagick
    MAGICKLIBDIR="${builddir}/magick/${libtool_objdir}"
    MAGICKLIB="-L${MAGICKLIBDIR} -lMagick"
fi
AC_SUBST(MAGICKLIB)
AC_SUBST(MAGICKLIBDIR)

# Create a simple string containing format names for all delegate libraries
DELEGATES=''
if test "$have_bzlib"      = 'yes'; then DELEGATES="$DELEGATES bzlib"; fi
if test "$have_dps"        = 'yes'; then DELEGATES="$DELEGATES dps"; fi
if test "$have_djvu"       = 'yes'; then DELEGATES="$DELEGATES djvu"; fi
if test "$have_fontconfig" = 'yes'; then DELEGATES="$DELEGATES fontconfig"; fi
if test "$have_fpx"        = 'yes'; then DELEGATES="$DELEGATES fpx"; fi
if test "$have_freetype"   = 'yes'; then DELEGATES="$DELEGATES freetype"; fi
if test "$have_gslib"      = 'yes'; then DELEGATES="$DELEGATES gs"; fi
if test "$have_gvc"        = 'yes'; then DELEGATES="$DELEGATES gvc"; fi
if test "$have_jbig"       = 'yes'; then DELEGATES="$DELEGATES jbig"; fi
if test "$have_jng"        = 'yes'; then DELEGATES="$DELEGATES jng"; fi
if test "$have_jpeg"       = 'yes'; then DELEGATES="$DELEGATES jpeg"; fi
if test "$have_jp2"        = 'yes'; then DELEGATES="$DELEGATES jp2"; fi
if test "$have_lcms"       = 'yes'; then DELEGATES="$DELEGATES lcms"; fi
if test "$have_openexr"    = 'yes'; then DELEGATES="$DELEGATES openexr"; fi
if test "$have_png"        = 'yes'; then DELEGATES="$DELEGATES png"; fi
if test "$have_rsvg"       = 'yes'; then DELEGATES="$DELEGATES rsvg"; fi
if test "$have_tiff"       = 'yes'; then DELEGATES="$DELEGATES tiff"; fi
if test "$have_wmf"        = 'yes'; then DELEGATES="$DELEGATES wmf"; fi
if test "$have_x"          = 'yes'; then DELEGATES="$DELEGATES x11"; fi
if test "$have_xml"        = 'yes'; then DELEGATES="$DELEGATES xml"; fi
if test "$have_wmf"        = 'yes'; then DELEGATES="$DELEGATES wmf"; fi
if test "$have_zlib"       = 'yes'; then DELEGATES="$DELEGATES zlib"; fi

AC_SUBST(DELEGATES)

#
# Handle special compiler flags
#

# Add '-p' if prof source profiling support enabled
if test "$enable_prof" = 'yes'; then
    CFLAGS="-p $CFLAGS"
    CXXFLAGS="-p $CXXFLAGS"
    LDFLAGS="-p $LDFLAGS"
fi

# Add '-pg' if gprof source profiling support enabled
if test "$enable_gprof" = 'yes'; then
    CFLAGS="-pg $CFLAGS"
    CXXFLAGS="-pg $CXXFLAGS"
    LDFLAGS="-pg $LDFLAGS"
fi

# Add '-ftest-coverage -fprofile-arcs' if gcov source profiling support enabled
# This is a gcc-specific feature
if test "$enable_gcov" = 'yes'; then
    AC_CHECK_LIB(gcov,_gcov_init)
    AC_CHECK_LIB(gcov,__gcov_init)
    case "$target_os" in
        darwin*)
            OSX_GCOV_LDFLAG="-Wl,-single_module"
        ;;
        *)
            OSX_GCOV_LDFLAG=""
        ;;
    esac
    AC_SUBST(OSX_GCOV_LDFLAG)
    CFLAGS="-ftest-coverage -fprofile-arcs  $CFLAGS"
    CXXFLAGS="-ftest-coverage -fprofile-arcs  $CXXFLAGS"
    LDFLAGS="-ftest-coverage -fprofile-arcs $LDFLAGS"
fi

#
# Build library dependency list for libMagick
#
if test "$with_modules" != 'no'; then
    MAGICK_DEP_LIBS="$LCMS_LIBS $TIFF_LIBS $FREETYPE_LIBS $JPEG_LIBS $FONTCONFIG_LIBS $XEXT_LIBS $IPC_LIBS $X11_LIBS $XT_LIBS $BZLIB_LIBS $ZLIB_LIBS $GDI32_LIBS $MATH_LIBS $CCMALLOC_LIBS $EFENCE_LIBS $GOMP_LIBS $THREAD_LIBS"
else
    MAGICK_DEP_LIBS="$JBIG_LIBS $LCMS_LIBS $TIFF_LIBS $FREETYPE_LIBS $JP2_LIBS $JPEG_LIBS $GS_LIBS $PNG_LIBS $DJVU_LIBS $FPX_LIBS $FONTCONFIG_LIBS $WMF_LIBS $DPS_LIBS $XEXT_LIBS $XT_LIBS $IPC_LIBS $X11_LIBS $BZLIB_LIBS $OPENEXR_LIBS $RSVG_LIBS $XML_LIBS $GVC_LIBS $ZLIB_LIBS $GDI32_LIBS $MATH_LIBS $CCMALLOC_LIBS $EFENCE_LIBS $GOMP_LIBS $THREAD_LIBS"
fi
AC_SUBST(MAGICK_DEP_LIBS)

#
# Remove extraneous spaces from output variables (asthetic)
#
X_CFLAGS=`echo $X_CFLAGS | sed -e 's/  */ /g'`
X_PRE_LIBS=`echo $X_PRE_LIBS | sed -e 's/  */ /g'`
X_LIBS=`echo $X_LIBS | sed -e 's/  */ /g'`
X_EXTRA_LIBS=`echo $X_EXTRA_LIBS | sed -e 's/  */ /g'`

CC=`echo $CC | sed -e 's/  */ /g'`
CFLAGS=`echo $CFLAGS | sed -e 's/  */ /g'`
CPPFLAGS=`echo $CPPFLAGS | sed -e 's/  */ /g'`
CXXFLAGS=`echo $CXXFLAGS | sed -e 's/  */ /g'`
DELEGATES=`echo $DELEGATES | sed -e 's/^ //g'`
DISTCHECK_CONFIG_FLAGS=`echo $DISTCHECK_CONFIG_FLAGS | sed -e 's/  */ /g'`
LDFLAGS=`echo $LDFLAGS | sed -e 's/  */ /g'`
TESTED_LIBS=`echo $LIBS | sed -e 's/  */ /g'`
MAGICK_DEP_LIBS=`echo $MAGICK_DEP_LIBS | sed -e 's/  */ /g'`
#LIBS=`echo $LIBS | sed -e 's/  */ /g'`

# Pass only user-provided LIBS as "global" libraries
LIBS=$USER_LIBS

#AC_SUBST(CPPFLAGS)
AC_SUBST(X_CFLAGS)
#AC_SUBST(LDFLAGS)
#AC_SUBST(X_PRE_LIBS)
#AC_SUBST(X_LIBS)
#AC_SUBST(X_EXTRA_LIBS)

MAGICK_CFLAGS=$CFLAGS
MAGICK_CXXFLAGS="$CXXFLAGS"
MAGICK_CPPFLAGS=`echo $MAGICK_CPPFLAGS | sed -e 's/  */ /g'`
MAGICK_PCFLAGS=`echo $MAGICK_PCFLAGS | sed -e 's/  */ /g'`
MAGICK_LDFLAGS="-L$LIB_DIR $LDFLAGS"
MAGICK_LIBS="-lMagick $MAGICK_DEP_LIBS"

AC_SUBST(MAGICK_CFLAGS)
AC_SUBST(MAGICK_CXXFLAGS)
AC_SUBST(MAGICK_CPPFLAGS)
AC_SUBST(MAGICK_PCFLAGS)
AC_SUBST(MAGICK_LDFLAGS)
AC_SUBST(MAGICK_LIBS)

# Set configured scripts to executable.
AC_CONFIG_COMMANDS([Magick++-config.in],[chmod +x Magick++/bin/Magick++-config])
AC_CONFIG_COMMANDS([Magick-config.in],[chmod +x magick/Magick-config])
AC_CONFIG_COMMANDS([Wand-config.in],[chmod +x wand/Wand-config])
AC_CONFIG_COMMANDS([magick.sh.in],[chmod +x magick.sh])

AC_MSG_RESULT([-------------------------------------------------------------])
AC_MSG_RESULT([Update ImageMagick configuration])
AC_MSG_RESULT([-------------------------------------------------------------])
AC_OUTPUT

rm -f magick-version

if test ${ghostscript_font_dir}x" != 'x'; then
    result_ghostscript_font_dir=$ghostscript_font_dir
fi

if test ${windows_font_dir}x" != 'x'; then
    result_windows_font_dir=${windows_font_dir}
fi

AC_MSG_RESULT([
ImageMagick is configured as follows. Please verify that this configuration
matches your expectations.

Host system type : $host

                  Option                        Value
-------------------------------------------------------------------------------
Shared libraries  --enable-shared=$enable_shared		$libtool_build_shared_libs
Static libraries  --enable-static=$enable_static		$libtool_build_static_libs
Module support    --with-modules=$with_modules		$with_modules
GNU ld            --with-gnu-ld=$with_gnu_ld		$lt_cv_prog_gnu_ld
Quantum depth     --with-quantum-depth=$with_quantum_depth	$with_quantum_depth
High Dynamic Range Imagery
                  --enable-hdri=$enable_hdri		$enable_hdri

Delegate Configuration:
BZLIB             --with-bzlib=$with_bzlib		$have_bzlib
DJVU              --with-djvu=$with_djvu		$have_djvu
DPS               --with-dps=$with_dps		$have_dps
FlashPIX          --with-fpx=$with_fpx		$have_fpx
FontConfig        --with-fontconfig=$with_fontconfig		$have_fontconfig
FreeType          --with-freetype=$with_freetype		$have_freetype
GhostPCL          None				$PCLDelegate ($PCLVersion)
Ghostscript       None				$PSDelegate ($GSVersion)
result_ghostscript_font_dir='none'
Ghostscript fonts --with-gs-font-dir=$with_gs_font_dir	$result_ghostscript_font_dir
Ghostscript lib   --with-gslib=$with_gslib		$have_gslib
Graphviz          --with-gvc=$with_gvc		$have_gvc
JBIG              --with-jbig=$with_jbig		$have_jbig
JPEG v1           --with-jpeg=$with_jpeg		$have_jpeg
JPEG-2000         --with-jp2=$with_jp2		$have_jp2
LCMS              --with-lcms=$with_lcms		$have_lcms
Magick++          --with-magick-plus-plus=$with_magick_plus_plus	$have_magick_plus_plus
OpenEXR           --with-openexr=$with_openexr		$have_openexr
PERL              --with-perl=$with_perl		$have_perl
PNG               --with-png=$with_png		$have_png
RSVG              --with-rsvg=$with_rsvg		$have_rsvg
TIFF              --with-tiff=$with_tiff		$have_tiff
result_windows_font_dir='none'
Windows fonts     --with-windows-font-dir=$with_windows_font_dir	$result_windows_font_dir
WMF               --with-wmf=$with_wmf		$have_wmf
X11               --with-x=$with_x			$have_x
XML               --with-xml=$with_xml		$have_xml
ZLIB              --with-zlib=$with_zlib		$have_zlib

X11 Configuration:
      X_CFLAGS        = $X_CFLAGS
      X_PRE_LIBS      = $X_PRE_LIBS
      X_LIBS          = $X_LIBS
      X_EXTRA_LIBS    = $X_EXTRA_LIBS

Options used to compile and link:
  PREFIX          = $PREFIX_DIR
  EXEC-PREFIX     = $EXEC_PREFIX_DIR
  VERSION         = $PACKAGE_VERSION
  CC              = $CC
  CFLAGS          = $CFLAGS
  MAGICK_CFLAGS   = $MAGICK_CFLAGS
  CPPFLAGS        = $MAGICK_CPPFLAGS
  PCFLAGS         = $MAGICK_PCFLAGS
  DEFS            = $DEFS
  LDFLAGS         = $LDFLAGS
  MAGICK_LDFLAGS  = $MAGICK_LDFLAGS
  LIBS            = $MAGICK_LIBS
  CXX             = $CXX
  CXXFLAGS        = $CXXFLAGS
])
