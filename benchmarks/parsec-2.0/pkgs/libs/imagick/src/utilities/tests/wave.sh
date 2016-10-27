#!/bin/sh
# Copyright (C) 1999-2005 ImageMagick Studio LLC
#
# This program is covered by multiple licenses, which are described in
# LICENSE. You should have received a copy of LICENSE with this
# package; otherwise see http://www.imagemagick.org/script/license.php.
#
#  Test for '${CONVERT}' utility.
#

set -e # Exit on any error
. ${srcdir}/utilities/tests/common.sh

${CONVERT} ${MODEL_MIFF} -background '#000000FF' \
        -wave 25x150 -label Wave Wave_out.miff
