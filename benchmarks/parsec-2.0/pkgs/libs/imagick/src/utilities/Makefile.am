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
#
#  Makefile for building ImageMagick utilities.

UTILITIES_PGMS = \
	utilities/animate \
	utilities/compare \
	utilities/composite \
	utilities/conjure \
	utilities/convert \
	utilities/display \
	utilities/identify \
	utilities/import \
	utilities/mogrify \
	utilities/montage \
	utilities/stream

utilities_animate_LDADD	= $(LIBMAGICK) $(LIBWAND)
utilities_animate_LDFLAGS	= $(LDFLAGS)
utilities_animate_SOURCES	= utilities/animate.c

utilities_compare_LDADD	= $(LIBMAGICK) $(LIBWAND)
utilities_compare_LDFLAGS	= $(LDFLAGS)
utilities_compare_SOURCES	= utilities/compare.c

utilities_composite_LDADD	= $(LIBMAGICK) $(LIBWAND)
utilities_composite_LDFLAGS	= $(LDFLAGS)
utilities_composite_SOURCES	= utilities/composite.c

utilities_conjure_LDADD	= $(LIBMAGICK) $(LIBWAND)
utilities_conjure_LDFLAGS	= $(LDFLAGS)
utilities_conjure_SOURCES	= utilities/conjure.c

utilities_convert_LDADD	= $(LIBMAGICK) $(LIBWAND)
utilities_convert_LDFLAGS	= $(LDFLAGS)
utilities_convert_SOURCES	= utilities/convert.c

utilities_display_LDADD	= $(LIBMAGICK) $(LIBWAND)
utilities_display_LDFLAGS	= $(LDFLAGS)
utilities_display_SOURCES	= utilities/display.c

utilities_identify_LDADD	= $(LIBMAGICK) $(LIBWAND)
utilities_identify_LDFLAGS	= $(LDFLAGS)
utilities_identify_SOURCES	= utilities/identify.c

utilities_import_LDADD	= $(LIBMAGICK) $(LIBWAND)
utilities_import_LDFLAGS	= $(LDFLAGS)
utilities_import_SOURCES	= utilities/import.c

utilities_mogrify_LDADD	= $(LIBMAGICK) $(LIBWAND)
utilities_mogrify_LDFLAGS	= $(LDFLAGS)
utilities_mogrify_SOURCES	= utilities/mogrify.c

utilities_montage_LDADD	= $(LIBMAGICK) $(LIBWAND)
utilities_montage_LDFLAGS	= $(LDFLAGS)
utilities_montage_SOURCES	= utilities/montage.c

utilities_stream_LDADD	= $(LIBMAGICK) $(LIBWAND)
utilities_stream_LDFLAGS	= $(LDFLAGS)
utilities_stream_SOURCES	= utilities/stream.c

# Tests to run
UTILITIES_TESTS = \
	utilities/tests/addnoise.sh \
	utilities/tests/annotate.sh \
	utilities/tests/blur.sh \
	utilities/tests/border.sh \
	utilities/tests/channel.sh \
	utilities/tests/charcoal.sh \
	utilities/tests/composite.sh \
	utilities/tests/contrast.sh \
	utilities/tests/convolve.sh \
	utilities/tests/crop.sh \
	utilities/tests/despeckle.sh \
	utilities/tests/draw.sh \
	utilities/tests/edge.sh \
	utilities/tests/emboss.sh \
	utilities/tests/equalize.sh \
	utilities/tests/explode.sh \
	utilities/tests/flip.sh \
	utilities/tests/flop.sh \
	utilities/tests/frame.sh \
	utilities/tests/gamma.sh \
	utilities/tests/gaussianblur.sh \
	utilities/tests/gradient.sh \
	utilities/tests/grayscale.sh \
	utilities/tests/implode.sh \
	utilities/tests/level.sh \
	utilities/tests/medianfilter.sh \
	utilities/tests/modulate.sh \
	utilities/tests/monochrome.sh \
	utilities/tests/negate.sh \
	utilities/tests/normalize.sh \
	utilities/tests/oilpaint.sh \
	utilities/tests/ordered-dither2x2.sh \
	utilities/tests/ordered-dither3x3.sh \
	utilities/tests/ordered-dither4x4.sh \
	utilities/tests/plasma.sh \
	utilities/tests/quantize.sh \
	utilities/tests/raise.sh \
	utilities/tests/random-threshold.sh \
	utilities/tests/reducenoise.sh \
	utilities/tests/resize.sh \
	utilities/tests/roll.sh \
	utilities/tests/rotate.sh \
	utilities/tests/scale.sh \
	utilities/tests/segment.sh \
	utilities/tests/shade.sh \
	utilities/tests/sharpen.sh \
	utilities/tests/shave.sh \
	utilities/tests/shear.sh \
	utilities/tests/solarize.sh \
	utilities/tests/spread.sh \
	utilities/tests/swirl.sh \
	utilities/tests/unsharpmask.sh \
	utilities/tests/wave.sh \
	utilities/tests/montage.sh

UTILITIES_MANS = \
	utilities/ImageMagick.1 \
	utilities/animate.1 \
	utilities/compare.1 \
	utilities/composite.1 \
	utilities/conjure.1 \
	utilities/convert.1 \
	utilities/display.1 \
	utilities/identify.1 \
	utilities/import.1 \
	utilities/mogrify.1 \
	utilities/montage.1 \
	utilities/stream.1

UTILITIES_CONFIGURE = \
	utilities/ImageMagick.1.in \
	utilities/animate.1.in \
	utilities/compare.1.in \
	utilities/composite.1.in \
	utilities/conjure.1.in \
	utilities/convert.1.in \
	utilities/display.1.in \
	utilities/identify.1.in \
	utilities/import.1.in \
	utilities/mogrify.1.in \
	utilities/montage.1.in \
	utilities/stream.1.in

UTILITIES_EXTRA_DIST = \
	$(UTILITIES_TESTS) \
	utilities/tests/common.sh

UTILITIES_CLEANFILES = \
	utilities/tests/*_out.miff \
	utilities/tests/*_out.txt \
	utilities/tests/demo.miff \
	utilities/tests/core


