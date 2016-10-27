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

.PRECIOUS: %.$(OBJ) %.$(DLL).$(OBJ) %.exe

ifeq ($(tbb_strict),1)
  ifeq ($(WARNING_AS_ERROR_KEY),)
    $(error WARNING_AS_ERROR_KEY is empty)
  endif
  # Do not remove line below!
  WARNING_KEY += $(WARNING_AS_ERROR_KEY)
endif

INCLUDES += $(INCLUDE_KEY)$(tbb_root)/src $(INCLUDE_KEY)$(tbb_root)/include

CPLUS_FLAGS += $(WARNING_KEY) $(CXXFLAGS)
LINK_FLAGS += $(LDFLAGS)
LIB_LINK_FLAGS += $(LDFLAGS)
CPLUS_FLAGS_NOSTRICT:=$(subst -strict_ansi,-ansi,$(CPLUS_FLAGS))

LIB_LINK_CMD ?= $(CPLUS) $(PIC_KEY)
ifeq ($(origin LIB_OUTPUT_KEY), undefined)
    LIB_OUTPUT_KEY = $(OUTPUT_KEY)
endif
ifeq ($(origin LIB_LINK_LIBS), undefined)
    LIB_LINK_LIBS = $(LIBDL) $(LIBS)
endif

CONLY ?= $(CPLUS)

# The most generic rules
%.$(OBJ): %.cpp
	$(CPLUS) $(COMPILE_ONLY) $(CPLUS_FLAGS) $(INCLUDES) $<

%.$(OBJ): %.c
	$(CONLY) $(COMPILE_ONLY) $(C_FLAGS) $(INCLUDES) $<

%.$(OBJ): %.asm
	$(ASM) $(ASM_FLAGS) $<

# Rule for generating .E file if needed for visual inspection
%.E: %.cpp
	$(CPLUS) $(CPLUS_FLAGS) $(INCLUDES) $(PREPROC_ONLY) $< >$@

# TODO Rule for generating .asm file if needed for visual inspection
%.asm: %.cpp
	$(CPLUS) /c /Fa $(CPLUS_FLAGS) $(INCLUDES) $<

# TODO Rule for generating .s file if needed for visual inspection
%.s: %.cpp
	$(CPLUS) -S $(CPLUS_FLAGS) $(INCLUDES) $<

# Customizations
ifeq (1,$(TBB_NOSTRICT))
# GNU 3.2.3 headers have a ISO syntax that is rejected by Intel compiler in -strict_ansi mode.
# The Mac uses gcc, so the list is empty for that platform.
# The files below need the -strict_ansi flag downgraded to -ansi to compile

$(KNOWN_NOSTRICT): %.o: %.cpp
	$(CPLUS) $(COMPILE_ONLY) $(CPLUS_FLAGS_NOSTRICT) $(INCLUDES) $<

endif

$(KNOWN_WARNINGS): %.$(OBJ): %.cpp
	$(CPLUS) $(COMPILE_ONLY) $(subst $(WARNING_KEY),,$(CPLUS_FLAGS_NOSTRICT)) $(INCLUDES) $<

tbb_misc.$(OBJ): tbb_misc.cpp version_string.tmp
	$(CPLUS) $(COMPILE_ONLY) $(CPLUS_FLAGS_NOSTRICT) $(INCLUDE_KEY). $(INCLUDES) $<

tbb_misc.E: tbb_misc.cpp version_string.tmp
	$(CPLUS) $(CPLUS_FLAGS_NOSTRICT) $(INCLUDE_KEY). $(INCLUDES) $(PREPROC_ONLY) $< >$@

%.res: %.rc version_string.tmp
	rc /Fo$@ $(INCLUDES) $(filter /D%,$(CPLUS_FLAGS)) $<

version_string.tmp: 
	$(MAKE_VERSIONS)

tbbvars:
	$(MAKE_TBBVARS)

