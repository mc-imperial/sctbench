#!/bin/sh
# Copyright (C) 1999-2006 ImageMagick Studio LLC
#
# This program is covered by multiple licenses, which are described in
# LICENSE. You should have received a copy of LICENSE with this
# package; otherwise see http://www.imagemagick.org/script/license.php.

. ${srcdir}/tests/common.shi
${RUNENV} ${MEMCHECK} ./rwfile ${SRCDIR}/input_bilevel.miff EPDF
