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

${MONTAGE} null: null: null: null: null: 'tmp:[A-Z]*_out.miff' \
  -geometry '130x194+10+5>' -gravity 'Center' -bordercolor 'green' \
  -border 1x1 -tile '5x' -background '#ffffff' -font ${GENERIC_TTF} \
  -pointsize 18 -fill '#600' -stroke 'none' -compress rle montage_out.miff
${CONVERT} logo: -resize 40% logo_out.miff
${COMPOSITE} 'tmp:logo_out.miff' -gravity north 'tmp:montage_out.miff' demo.miff
