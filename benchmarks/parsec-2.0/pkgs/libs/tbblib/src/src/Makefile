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

tbb_root?=..
examples_root:=$(tbb_root)/examples
include $(tbb_root)/build/common.inc
.PHONY: all tbb tbbmalloc test debug examples clean info

all: release debug examples

tbb: tbb_release tbb_debug

tbbmalloc: tbbmalloc_release tbbmalloc_debug

test: tbbmalloc_test_release test_release tbbmalloc_test_debug test_debug

test_no_depends: tbbmalloc_test_release_no_depends test_release_no_depends tbbmalloc_test_debug_no_depends test_debug_no_depends
	@echo done

release: tbb_release tbbmalloc_release
release: $(call cross_cfg,tbbmalloc_test_release) $(call cross_cfg,test_release)

debug: tbb_debug tbbmalloc_debug
debug: $(call cross_cfg,tbbmalloc_test_debug) $(call cross_cfg, test_debug)

examples: tbb tbbmalloc examples_debug clean_examples examples_release

clean: clean_release clean_debug clean_examples
	@echo clean done

.PHONY: full
full:
	$(MAKE) -s -i -r --no-print-directory -f Makefile tbb_root=. clean all
ifeq ($(tbb_os),windows)
	$(MAKE) -s -i -r --no-print-directory -f Makefile tbb_root=. compiler=icl clean all native_examples
else
	$(MAKE) -s -i -r --no-print-directory -f Makefile tbb_root=. compiler=icc clean all native_examples
endif
ifeq ($(arch),em64t)
	$(MAKE) -s -i -r --no-print-directory -f Makefile tbb_root=. arch=ia32 clean all
endif
# it doesn't test compiler=icc arch=ia32 on em64t systems due to enviroment settings of icc

native_examples: tbb tbbmalloc
	$(MAKE) -C $(examples_root) -r -f Makefile tbb_root=.. compiler=$(native_compiler) tbb_build_prefix=$(tbb_build_prefix) debug test
	$(MAKE) -C $(examples_root) -r -f Makefile tbb_root=.. compiler=$(native_compiler) tbb_build_prefix=$(tbb_build_prefix) clean release test

../examples/% examples/%::
	$(MAKE) -C $(examples_root) -r -f Makefile tbb_root=.. $(subst examples/,,$(subst ../,,$@))

debug_%:: cfg?=debug
test_% stress_% time_%:: cfg?=release
debug_% test_% stress_% time_%::
	$(MAKE) -C "$(work_dir)_$(cfg)"  -r -f $(tbb_root)/build/Makefile.test cfg=$(cfg) tbb_root=$(tbb_root) $@

clean_%::
ifeq ($(cfg),)
	@$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.test cfg=release tbb_root=$(tbb_root) $@
	@$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.test cfg=debug tbb_root=$(tbb_root) $@
else
	@$(MAKE) -C "$(work_dir)_$(cfg)"  -r -f $(tbb_root)/build/Makefile.test cfg=$(cfg) tbb_root=$(tbb_root) $@
endif

.PHONY: tbb_release tbb_debug test_release test_debug

# do not delete double-space after -C option
tbb_release: mkdir_release
	$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.tbb cfg=release tbb_root=$(tbb_root)

tbb_debug: mkdir_debug
	$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.tbb cfg=debug tbb_root=$(tbb_root)

test_release: $(call cross_cfg,mkdir_release) $(call cross_cfg,tbb_release) test_release_no_depends
test_release_no_depends: 
	-$(MAKE) -C "$(call cross_cfg,$(work_dir)_release)"  -r -f $(tbb_root)/build/Makefile.test cfg=release tbb_root=$(tbb_root) 

test_debug: $(call cross_cfg,mkdir_debug) $(call cross_cfg,tbb_debug) test_debug_no_depends
test_debug_no_depends:
	-$(MAKE) -C "$(call cross_cfg,$(work_dir)_debug)"  -r -f $(tbb_root)/build/Makefile.test cfg=debug tbb_root=$(tbb_root)

.PHONY: tbbmalloc_release tbbmalloc_debug tbbmalloc_test_release tbbmalloc_test_debug

tbbmalloc_release: mkdir_release
	$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=release malloc tbb_root=$(tbb_root)

tbbmalloc_debug: mkdir_debug
	$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=debug malloc tbb_root=$(tbb_root)

tbbmalloc_test_release: $(call cross_cfg,mkdir_release) $(call cross_cfg,tbbmalloc_release) tbbmalloc_test_release_no_depends
tbbmalloc_test_release_no_depends:
	-$(MAKE) -C "$(call cross_cfg,$(work_dir)_release)"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=release malloc_test tbb_root=$(tbb_root)

tbbmalloc_test_debug: $(call cross_cfg,mkdir_debug) $(call cross_cfg,tbbmalloc_debug) tbbmalloc_test_debug_no_depends
tbbmalloc_test_debug_no_depends:
	-$(MAKE) -C "$(call cross_cfg,$(work_dir)_debug)"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=debug malloc_test tbb_root=$(tbb_root)

.PHONY: examples_release examples_debug

examples_release: tbb_release tbbmalloc_release
	$(MAKE) -C $(examples_root) -r -f Makefile tbb_root=.. release test

examples_debug: tbb_debug tbbmalloc_debug
	$(MAKE) -C $(examples_root) -r -f Makefile tbb_root=.. debug test

.PHONY: clean_release clean_debug clean_examples

clean_release:
	$(shell $(RM) $(work_dir)_release$(SLASH)*.* >$(NUL) 2>$(NUL))
	$(shell $(RD) $(work_dir)_release >$(NUL) 2>$(NUL))

clean_debug:
	$(shell $(RM) $(work_dir)_debug$(SLASH)*.* >$(NUL) 2>$(NUL))
	$(shell $(RD) $(work_dir)_debug >$(NUL) 2>$(NUL))

clean_examples:
	$(shell $(MAKE) -s -i -r -C $(examples_root) -f Makefile tbb_root=.. clean >$(NUL) 2>$(NUL))

.PHONY: mkdir_release mkdir_debug

mkdir_release:
	$(shell $(MD) "$(work_dir)_release" >$(NUL) 2>$(NUL))
	$(if $(subst undefined,,$(origin_build_dir)),,cd "$(work_dir)_release" && $(MAKE_TBBVARS) $(tbb_build_prefix)_release)

mkdir_debug:
	$(shell $(MD) "$(work_dir)_debug" >$(NUL) 2>$(NUL))
	$(if $(subst undefined,,$(origin_build_dir)),,cd "$(work_dir)_debug" && $(MAKE_TBBVARS) $(tbb_build_prefix)_debug)

info:
	@echo OS: $(tbb_os)
	@echo arch=$(arch)
	@echo compiler=$(compiler)
	@echo runtime=$(runtime)
	@echo tbb_build_prefix=$(tbb_build_prefix)
