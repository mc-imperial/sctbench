# Copyright 2005-2008 Intel Corporation.  All Rights Reserved.
#
# This file is part of Threading Building Blocks.
#
# Threading Building Blocks is free software; you can redistribute it
# and/or modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation.
#
# Threading Building Blocks is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Threading Building Blocks; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# As a special exception, you may use this file as part of a free software
# library without restriction.  Specifically, if other files instantiate
# templates or use macros or inline functions from this file, or you compile
# this file and link it with other files to produce an executable, this
# file does not by itself cause the resulting executable to be covered by
# the GNU General Public License.  This exception does not however
# invalidate any other reasons why the executable file might be covered by
# the GNU General Public License.

# The original source for this example is
# Copyright (c) 1994-2008 John E. Stone
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

# GNU Makefile that builds and runs example.
NAME:=tachyon

# The C++ compiler
#CXX = g++

# The default dataset
export DATASET = balls
# The default runtime arguments
export ARGS =

# detecting x64
ifeq ($(shell uname -m),x86_64)
x64 ?= 64
endif
# detecting UI ("mac", "x" or "con")
ifeq ($(shell uname),Darwin)
UI ?= mac
else
UI ?= $(shell sh -c "[ -f /usr/X11R6/lib$(x64)/libX11.so -o -f /usr/lib$(x64)/libX11.so ] && echo x || echo con")
endif

MYCXXFLAGS = -DX_FULLSYNC
LIBS ?= -lm

# Platform-specific settings
ifeq ($(UI),x)
EXE=$(NAME).$(VERSION)
MYCXXFLAGS += -I/usr/X11R6/include
ADD_THREADS=1
LIBS += -L/usr/X11R6/lib$(x64) -lX11
# detect if libXext can be found
ifeq ($(shell sh -c "[ -f /usr/X11R6/lib$(x64)/libXext.so -o -f /usr/lib$(x64)/libXext.so ] && echo 0"),0)
LIBS += -lXext
MYCXXFLAGS += -DX_NOSHMPIX
else
MYCXXFLAGS += -DX_NOSHMEM
endif
else

ifeq ($(UI),mac)
TBBLIBSPATH = /Library/Frameworks/TBB.framework/Libraries
CXXFLAGS += -arch i386 -mmacosx-version-min=10.4 -msse3 -ftree-vectorize
LIBS += -framework OpenGL -framework AGL -framework Carbon -L$(TBBLIBSPATH)
RES = $(NAME).$(VERSION).app/Contents/Resources
EXE = $(NAME).$(VERSION).app/Contents/MacOS/$(NAME).$(VERSION)
# look in /System and /Developer (for Xcode 3)
PBXCP = $(wildcard /[SD]*/Library/PrivateFrameworks/DevToolsCore.framework/Resources/pbxcp) -resolve-src-symlinks -exclude .DS_Store -exclude CVS -exclude .svn
else # ! Mac

UI=con
EXE=$(NAME).$(VERSION)
endif # Mac
endif # X

MYCXXFLAGS += $(CXXFLAGS)

all: build run
release: build
debug: build_debug
test: run

build: build_serial build_tbb1d build_tbb
build_debug: build_serial_debug build_tbb1d_debug build_tbb_debug
run: run_serial run_tbb1d run_tbb

serial: build_serial run_serial
serial_debug: build_serial_debug run_serial
tbb: build_tbb run_tbb
tbb_debug: build_tbb_debug run_tbb
tbb1d: build_tbb1d run_tbb1d
tbb1d_debug: build_tbb1d_debug run_tbb1d

build_serial:
	$(MAKE) VERSION=serial build_one
build_serial_debug:
	$(MAKE) VERSION=serial ADD_DEBUG=1 build_one
run_serial:
	$(MAKE) VERSION=serial run_one

build_tbb:
	$(MAKE) VERSION=tbb ADD_TBB=1 build_one
build_tbb_debug:
	$(MAKE) VERSION=tbb ADD_TBB=1 ADD_DEBUG=1 build_one
run_tbb:
	$(MAKE) VERSION=tbb run_one

build_tbb1d:
	$(MAKE) VERSION=tbb1d ADD_TBB=1 build_one
build_tbb1d_debug:
	$(MAKE) VERSION=tbb1d ADD_TBB=1 ADD_DEBUG=1 build_one
run_tbb1d:
	$(MAKE) VERSION=tbb1d run_one


#
# Per-build Makefile rules (for recursive $(MAKE) calls from above)
#

SVERSION = $(VERSION)

ifeq ($(ADD_DEBUG),1)
MYCXXFLAGS += -O0 -g -D_DEBUG
else
MYCXXFLAGS += -O2
endif

ifeq ($(ADD_THREADS),1)
LIBS += -lpthread
endif


ifeq ($(ADD_TBB),1)
MYCXXFLAGS +=
ifeq ($(ADD_DEBUG),1)
MYCXXFLAGS += -DTBB_DO_ASSERT
LIBS += -ltbb_debug
else
LIBS += -ltbb
endif
endif

SOURCE = ../../common/gui/$(UI)video.cpp src/trace.$(SVERSION).cpp src/pthread.cpp src/video.cpp src/api.cpp src/apigeom.cpp src/apitrigeom.cpp src/bndbox.cpp src/box.cpp src/camera.cpp src/coordsys.cpp src/cylinder.cpp src/extvol.cpp src/getargs.cpp src/global.cpp src/grid.cpp src/imageio.cpp src/imap.cpp src/intersect.cpp src/jpeg.cpp src/light.cpp src/objbound.cpp src/parse.cpp src/plane.cpp src/ppm.cpp src/quadric.cpp src/render.cpp src/ring.cpp src/shade.cpp src/sphere.cpp src/texture.cpp src/tgafile.cpp src/trace_rest.cpp src/triangle.cpp src/ui.cpp src/util.cpp src/vector.cpp src/vol.cpp

build_one:	$(EXE)

run_one:
ifeq ($(UI),mac)
	export DYLD_LIBRARY_PATH="$(DYLD_LIBRARY_PATH):$(TBBLIBSPATH)"; ./$(EXE) $(ARGS) dat/$(DATASET).dat
else
	./$(EXE) $(ARGS) dat/$(DATASET).dat
endif

$(EXE): $(SOURCE)
ifeq ($(UI),mac)
	mkdir -p $(RES)/English.lproj $(NAME).$(VERSION).app/Contents/MacOS
	$(PBXCP) xcode/English.lproj/InfoPlist.strings xcode/English.lproj/main.nib $(RES)/English.lproj
	$(PBXCP) xcode/Info.plist $(RES)
endif
	$(CXX) $(MYCXXFLAGS) -o $@ $(SOURCE) $(LIBS)
	rm -f *.o

clean:
	rm -rf $(NAME).* *.o *.d
