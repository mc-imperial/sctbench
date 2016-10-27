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

# GNU Makefile that builds and runs example.
NAME=seismic

# The C++ compiler
#CXX=g++

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


ifeq ($(UI),x)
EXE=./$(NAME)
CXXFLAGS += -I/usr/X11R6/include
LIBS += -lpthread -L/usr/X11R6/lib$(x64) -lX11
# detect if libXext can be found
ifeq ($(shell sh -c "[ -f /usr/X11R6/lib$(x64)/libXext.so -o -f /usr/lib$(x64)/libXext.so ] && echo 0"),0)
LIBS += -lXext
else  # no libXext
CXXFLAGS += -DX_NOSHMEM
endif # libXext
else # ! X

ifeq ($(UI),mac)
TBBLIBSPATH = /Library/Frameworks/TBB.framework/Libraries
CXXFLAGS += -L$(TBBLIBSPATH) -arch i386 -mmacosx-version-min=10.4 -msse3 -ftree-vectorize
LIBS += -framework OpenGL -framework AGL -framework Carbon
APPRES = $(NAME).app/Contents/Resources
EXE = ./$(NAME).app/Contents/MacOS/$(NAME)
# look in /System and /Developer (for Xcode 3)
PBXCP = $(wildcard /[SD]*/Library/PrivateFrameworks/DevToolsCore.framework/Resources/pbxcp) -resolve-src-symlinks -exclude .DS_Store -exclude CVS -exclude .svn
else # ! Mac
UI=con
EXE=./$(NAME)
endif # Mac
endif # X

all:	release test

resources:
ifeq ($(UI),mac)
	mkdir -p $(APPRES)/English.lproj $(NAME).app/Contents/MacOS
	$(PBXCP) xcode/English.lproj/InfoPlist.strings xcode/English.lproj/main.nib $(APPRES)/English.lproj
	$(PBXCP) xcode/Info.plist $(APPRES)
endif

release: ../../common/gui/$(UI)video.cpp SeismicSimulation.cpp resources
	$(CXX) -O2 -DNDEBUG $(CXXFLAGS) -o $(EXE) $< SeismicSimulation.cpp -ltbb $(LIBS)

debug: ../../common/gui/$(UI)video.cpp SeismicSimulation.cpp resources
	$(CXX) -g -O0 -DTBB_DO_ASSERT $(CXXFLAGS) -o $(EXE) $< SeismicSimulation.cpp -ltbb_debug $(LIBS)

clean:
	rm -f $(EXE) *.o *.d
ifeq ($(UI),mac)
	rm -rf $(NAME).app
endif

test:
ifeq ($(UI),mac)
	export DYLD_LIBRARY_PATH="$(DYLD_LIBRARY_PATH):$(TBBLIBSPATH)"; $(EXE) - 300
else
	$(EXE) - 300
endif
