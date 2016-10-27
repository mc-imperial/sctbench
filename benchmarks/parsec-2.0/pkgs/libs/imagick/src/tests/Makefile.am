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
#  Makefile for ImageMagick tests.

TESTS_CHECK_PGRMS = \
	tests/constitute \
	tests/rwblob \
	tests/rwfile

tests_constitute_SOURCES = tests/constitute.c
tests_constitute_CPPFLAGS = $(AM_CPPFLAGS)
tests_constitute_LDADD = $(LIBMAGICK) $(LIBWAND)

tests_rwblob_SOURCES = tests/rwblob.c
tests_rwblob_CPPFLAGS = $(AM_CPPFLAGS)
tests_rwblob_LDADD = $(LIBMAGICK) $(LIBWAND)

tests_rwfile_SOURCES = tests/rwfile.c
tests_rwfile_CPPFLAGS = $(AM_CPPFLAGS)
tests_rwfile_LDADD = $(LIBMAGICK) $(LIBWAND)

if !HasFPX
TESTS_FPX_XFAIL_TESTS = \
	tests/rwblob_FPX_bilevel.sh \
	tests/rwblob_FPX_gray.sh \
	tests/rwblob_FPX_pallette.sh \
	tests/rwblob_FPX_truecolor.sh \
	tests/rwfile_FPX_bilevel.sh \
	tests/rwfile_FPX_gray.sh \
	tests/rwfile_FPX_pallette.sh \
	tests/rwfile_FPX_truecolor.sh \
	tests/rwfile_FPX_truecolor10.sh \
	tests/rwfile_FPX_truecolor12.sh \
	tests/rwfile_FPX_truecolor16.sh
endif

if !HasJBIG
TESTS_JBIG_XFAIL_TESTS = \
	tests/rwblob_JBIG_bilevel.sh \
	tests/rwblob_JBIG_gray.sh \
	tests/rwblob_JBIG_pallette.sh \
	tests/rwblob_JBIG_truecolor.sh \
	tests/rwfile_JBIG_bilevel.sh \
	tests/rwfile_JBIG_gray.sh \
	tests/rwfile_JBIG_pallette.sh \
	tests/rwfile_JBIG_truecolor.sh \
	tests/rwfile_JBIG_truecolor10.sh \
	tests/rwfile_JBIG_truecolor12.sh \
	tests/rwfile_JBIG_truecolor16.sh
endif

if !HasJPEG
TESTS_JPEG_XFAIL_TESTS = \
	tests/rwblob_JPEG_bilevel.sh \
	tests/rwblob_JPEG_gray.sh \
	tests/rwblob_JPEG_pallette.sh \
	tests/rwblob_JPEG_truecolor.sh \
	tests/rwfile_JPEG_bilevel.sh \
	tests/rwfile_JPEG_gray.sh \
	tests/rwfile_JPEG_pallette.sh \
	tests/rwfile_JPEG_truecolor.sh \
	tests/rwfile_JPEG_truecolor10.sh \
	tests/rwfile_JPEG_truecolor12.sh \
	tests/rwfile_JPEG_truecolor16.sh
endif

if !HasJP2
TESTS_JP2_XFAIL_TESTS = \
	tests/rwblob_JP2_bilevel.sh \
	tests/rwblob_JP2_gray.sh \
	tests/rwblob_JP2_pallette.sh \
	tests/rwblob_JP2_truecolor.sh \
	tests/rwfile_JP2_bilevel.sh \
	tests/rwfile_JP2_gray.sh \
	tests/rwfile_JP2_pallette.sh \
	tests/rwfile_JP2_truecolor.sh \
	tests/rwfile_JP2_truecolor10.sh \
	tests/rwfile_JP2_truecolor12.sh \
	tests/rwfile_JP2_truecolor16.sh
endif

if !HasPNG
TESTS_PNG_XFAIL_TESTS = \
	tests/rwblob_MNG_bilevel.sh \
	tests/rwblob_MNG_gray.sh \
	tests/rwblob_MNG_pallette.sh \
	tests/rwblob_MNG_truecolor.sh \
	tests/rwblob_PNG_bilevel.sh \
	tests/rwblob_PNG_gray.sh \
	tests/rwblob_PNG_pallette.sh \
	tests/rwblob_PNG_truecolor.sh \
	tests/rwfile_MNG_bilevel.sh \
	tests/rwfile_MNG_gray.sh \
	tests/rwfile_MNG_pallette.sh \
	tests/rwfile_MNG_truecolor.sh \
	tests/rwfile_MNG_truecolor10.sh \
	tests/rwfile_MNG_truecolor12.sh \
	tests/rwfile_MNG_truecolor16.sh \
	tests/rwfile_PNG_bilevel.sh \
	tests/rwfile_PNG_gray.sh \
	tests/rwfile_PNG_pallette.sh \
	tests/rwfile_PNG_truecolor.sh \
	tests/rwfile_PNG_truecolor10.sh \
	tests/rwfile_PNG_truecolor12.sh \
	tests/rwfile_PNG_truecolor16.sh
endif

if !HasTIFF
TESTS_TIFF_XFAIL_TESTS = \
	tests/rwblob_EPT_bilevel.sh \
	tests/rwblob_EPT_gray.sh \
	tests/rwblob_EPT_pallette.sh \
	tests/rwblob_EPT_truecolor.sh \
	tests/rwblob_PTIF_bilevel.sh \
	tests/rwblob_PTIF_gray.sh \
	tests/rwblob_PTIF_pallette.sh \
	tests/rwblob_PTIF_truecolor.sh \
	tests/rwblob_TIFF_bilevel.sh \
	tests/rwblob_TIFF_gray.sh \
	tests/rwblob_TIFF_pallette.sh \
	tests/rwblob_TIFF_truecolor.sh \
	tests/rwfile_EPT_bilevel.sh \
	tests/rwfile_EPT_gray.sh \
	tests/rwfile_EPT_pallette.sh \
	tests/rwfile_EPT_truecolor.sh \
	tests/rwfile_EPT_truecolor10.sh \
	tests/rwfile_EPT_truecolor12.sh \
	tests/rwfile_EPT_truecolor16.sh \
	tests/rwfile_PTIF_bilevel.sh \
	tests/rwfile_PTIF_gray.sh \
	tests/rwfile_PTIF_pallette.sh \
	tests/rwfile_PTIF_truecolor.sh \
	tests/rwfile_PTIF_truecolor10.sh \
	tests/rwfile_PTIF_truecolor12.sh \
	tests/rwfile_PTIF_truecolor16.sh \
	tests/rwfile_TIFF_bilevel.sh \
	tests/rwfile_TIFF_gray.sh \
	tests/rwfile_TIFF_pallette.sh \
	tests/rwfile_TIFF_truecolor.sh \
	tests/rwfile_TIFF_truecolor10.sh \
	tests/rwfile_TIFF_truecolor12.sh \
	tests/rwfile_TIFF_truecolor16.sh
endif

if !HasX11
TESTS_X11_XFAIL_TESTS = \
	tests/rwblob_XWD_bilevel.sh \
	tests/rwblob_XWD_gray.sh \
	tests/rwblob_XWD_pallette.sh \
	tests/rwblob_XWD_truecolor.sh \
	tests/rwfile_XWD_bilevel.sh \
	tests/rwfile_XWD_gray.sh \
	tests/rwfile_XWD_pallette.sh \
	tests/rwfile_XWD_truecolor.sh \
	tests/rwfile_XWD_truecolor10.sh \
	tests/rwfile_XWD_truecolor12.sh \
	tests/rwfile_XWD_truecolor16.sh
endif

# These tests do not currently actually fail so they are not included
# in TESTS_XFAIL_TESTS
if !HasBZLIB
TESTS_BZIP_XFAIL_TESTS = \
	tests/rwfile_MIFF_bilevel_bzip.sh \
	tests/rwfile_MIFF_gray_bzip.sh \
	tests/rwfile_MIFF_pallette_bzip.sh \
	tests/rwfile_MIFF_truecolor_bzip.sh
endif

# These tests do not currently actually fail so they are not included
# in TESTS_XFAIL_TESTS
if !HasZLIB
TESTS_ZIP_XFAIL_TESTS = \
	tests/rwfile_MIFF_bilevel_zip.sh \
	tests/rwfile_MIFF_gray_zip.sh \
	tests/rwfile_MIFF_pallette_zip.sh \
	tests/rwfile_MIFF_truecolor_zip.sh
endif

TESTS_XFAIL_TESTS = \
	$(TESTS_FPX_XFAIL_TESTS) \
	$(TESTS_JBIG_XFAIL_TESTS) \
	$(TESTS_JP2_XFAIL_TESTS) \
	$(TESTS_JPEG_XFAIL_TESTS) \
	$(TESTS_PNG_XFAIL_TESTS) \
	$(TESTS_TIFF_XFAIL_TESTS) \
	$(TESTS_X11_XFAIL_TESTS)

TESTS_TESTS = \
	tests/constitute_char_bgro.sh \
	tests/constitute_char_bgrp.sh \
	tests/constitute_char_bgr.sh \
	tests/constitute_char_cmyk.sh \
	tests/constitute_char_cmy.sh \
	tests/constitute_char_i.sh \
	tests/constitute_char_prgb.sh \
	tests/constitute_char_rgba.sh \
	tests/constitute_char_rgbo.sh \
	tests/constitute_char_rgb.sh \
	tests/constitute_double_bgro.sh \
	tests/constitute_double_bgrp.sh \
	tests/constitute_double_bgr.sh \
	tests/constitute_double_cmyk.sh \
	tests/constitute_double_cmy.sh \
	tests/constitute_double_i.sh \
	tests/constitute_double_prgb.sh \
	tests/constitute_double_rgba.sh \
	tests/constitute_double_rgbo.sh \
	tests/constitute_double_rgb.sh \
	tests/constitute_float_bgro.sh \
	tests/constitute_float_bgrp.sh \
	tests/constitute_float_bgr.sh \
	tests/constitute_float_cmyk.sh \
	tests/constitute_float_cmy.sh \
	tests/constitute_float_i.sh \
	tests/constitute_float_prgb.sh \
	tests/constitute_float_rgba.sh \
	tests/constitute_float_rgbo.sh \
	tests/constitute_float_rgb.sh \
	tests/constitute_integer_bgro.sh \
	tests/constitute_integer_bgrp.sh \
	tests/constitute_integer_bgr.sh \
	tests/constitute_integer_cmyk.sh \
	tests/constitute_integer_cmy.sh \
	tests/constitute_integer_i.sh \
	tests/constitute_integer_prgb.sh \
	tests/constitute_integer_rgba.sh \
	tests/constitute_integer_rgbo.sh \
	tests/constitute_integer_rgb.sh \
	tests/constitute_long_bgro.sh \
	tests/constitute_long_bgrp.sh \
	tests/constitute_long_bgr.sh \
	tests/constitute_long_cmyk.sh \
	tests/constitute_long_cmy.sh \
	tests/constitute_long_i.sh \
	tests/constitute_long_prgb.sh \
	tests/constitute_long_rgba.sh \
	tests/constitute_long_rgbo.sh \
	tests/constitute_long_rgb.sh \
	tests/constitute_short_bgro.sh \
	tests/constitute_short_bgrp.sh \
	tests/constitute_short_bgr.sh \
	tests/constitute_short_cmyk.sh \
	tests/constitute_short_cmy.sh \
	tests/constitute_short_i.sh \
	tests/constitute_short_prgb.sh \
	tests/constitute_short_rgba.sh \
	tests/constitute_short_rgbo.sh \
	tests/constitute_short_rgb.sh \
	tests/rwblob_AVS_bilevel.sh \
	tests/rwblob_AVS_gray.sh \
	tests/rwblob_AVS_pallette.sh \
	tests/rwblob_AVS_truecolor.sh \
	tests/rwblob_BMP_bilevel.sh \
	tests/rwblob_BMP_gray.sh \
	tests/rwblob_BMP_pallette.sh \
	tests/rwblob_BMP_truecolor.sh \
	tests/rwblob_CMYK_truecolor_70x46.sh \
	tests/rwblob_DIB_bilevel.sh \
	tests/rwblob_DIB_gray.sh \
	tests/rwblob_DIB_pallette.sh \
	tests/rwblob_DIB_truecolor.sh \
	tests/rwblob_DPX_bilevel.sh \
	tests/rwblob_DPX_gray.sh \
	tests/rwblob_DPX_pallette.sh \
	tests/rwblob_DPX_truecolor.sh \
	tests/rwblob_EPDF_bilevel.sh \
	tests/rwblob_EPDF_gray.sh \
	tests/rwblob_EPDF_pallette.sh \
	tests/rwblob_EPDF_truecolor.sh \
	tests/rwblob_EPT_bilevel.sh \
	tests/rwblob_EPT_gray.sh \
	tests/rwblob_EPT_pallette.sh \
	tests/rwblob_EPT_truecolor.sh \
	tests/rwblob_FAX_bilevel.sh \
	tests/rwblob_FAX_gray.sh \
	tests/rwblob_FAX_pallette.sh \
	tests/rwblob_FAX_truecolor.sh \
	tests/rwblob_FITS_bilevel.sh \
	tests/rwblob_FITS_gray.sh \
	tests/rwblob_FPX_bilevel.sh \
	tests/rwblob_FPX_gray.sh \
	tests/rwblob_FPX_pallette.sh \
	tests/rwblob_FPX_truecolor.sh \
	tests/rwblob_G3_bilevel.sh \
	tests/rwblob_G3_gray.sh \
	tests/rwblob_G3_pallette.sh \
	tests/rwblob_G3_truecolor.sh \
	tests/rwblob_GIF87_bilevel.sh \
	tests/rwblob_GIF87_gray.sh \
	tests/rwblob_GIF87_pallette.sh \
	tests/rwblob_GIF87_truecolor.sh \
	tests/rwblob_GIF_bilevel.sh \
	tests/rwblob_GIF_gray.sh \
	tests/rwblob_GIF_pallette.sh \
	tests/rwblob_GIF_truecolor.sh \
	tests/rwblob_GRAY_truecolor_70x46.sh \
	tests/rwblob_JBIG_bilevel.sh \
	tests/rwblob_JBIG_gray.sh \
	tests/rwblob_JBIG_pallette.sh \
	tests/rwblob_JBIG_truecolor.sh \
	tests/rwblob_JP2_bilevel.sh \
	tests/rwblob_JP2_gray.sh \
	tests/rwblob_JP2_pallette.sh \
	tests/rwblob_JP2_truecolor.sh \
	tests/rwblob_JPEG_bilevel.sh \
	tests/rwblob_JPEG_gray.sh \
	tests/rwblob_JPEG_pallette.sh \
	tests/rwblob_JPEG_truecolor.sh \
	tests/rwblob_MIFF_bilevel.sh \
	tests/rwblob_MIFF_gray.sh \
	tests/rwblob_MIFF_pallette.sh \
	tests/rwblob_MIFF_truecolor.sh \
	tests/rwblob_MNG_bilevel.sh \
	tests/rwblob_MNG_gray.sh \
	tests/rwblob_MNG_pallette.sh \
	tests/rwblob_MNG_truecolor.sh \
	tests/rwblob_MONO_truecolor_70x46.sh \
	tests/rwblob_MTV_bilevel.sh \
	tests/rwblob_MTV_gray.sh \
	tests/rwblob_MTV_pallette.sh \
	tests/rwblob_MTV_truecolor.sh \
	tests/rwblob_PAL_truecolor_70x46.sh \
	tests/rwblob_PAM_bilevel.sh \
	tests/rwblob_PAM_gray.sh \
	tests/rwblob_PAM_pallette.sh \
	tests/rwblob_PAM_truecolor.sh \
	tests/rwblob_PBM_bilevel.sh \
	tests/rwblob_PBM_gray.sh \
	tests/rwblob_PBM_pallette.sh \
	tests/rwblob_PBM_truecolor.sh \
	tests/rwblob_PCD_bilevel.sh \
	tests/rwblob_PCD_gray.sh \
	tests/rwblob_PCD_pallette.sh \
	tests/rwblob_PCDS_bilevel.sh \
	tests/rwblob_PCDS_gray.sh \
	tests/rwblob_PCDS_pallette.sh \
	tests/rwblob_PCDS_truecolor.sh \
	tests/rwblob_PCD_truecolor.sh \
	tests/rwblob_PCX_bilevel.sh \
	tests/rwblob_PCX_gray.sh \
	tests/rwblob_PCX_pallette.sh \
	tests/rwblob_PCX_truecolor.sh \
	tests/rwblob_PGM_bilevel.sh \
	tests/rwblob_PGM_gray.sh \
	tests/rwblob_PGM_pallette.sh \
	tests/rwblob_PGM_truecolor.sh \
	tests/rwblob_PICON_bilevel.sh \
	tests/rwblob_PICON_gray.sh \
	tests/rwblob_PICON_pallette.sh \
	tests/rwblob_PICON_truecolor.sh \
	tests/rwblob_PICT_bilevel.sh \
	tests/rwblob_PICT_gray.sh \
	tests/rwblob_PICT_pallette.sh \
	tests/rwblob_PICT_truecolor.sh \
	tests/rwblob_PNG_bilevel.sh \
	tests/rwblob_PNG_gray.sh \
	tests/rwblob_PNG_pallette.sh \
	tests/rwblob_PNG_truecolor.sh \
	tests/rwblob_PNM_bilevel.sh \
	tests/rwblob_PNM_gray.sh \
	tests/rwblob_PNM_pallette.sh \
	tests/rwblob_PNM_truecolor.sh \
	tests/rwblob_PPM_bilevel.sh \
	tests/rwblob_PPM_gray.sh \
	tests/rwblob_PPM_pallette.sh \
	tests/rwblob_PPM_truecolor.sh \
	tests/rwblob_PSD_bilevel.sh \
	tests/rwblob_PSD_gray.sh \
	tests/rwblob_PSD_pallette.sh \
	tests/rwblob_PSD_truecolor.sh \
	tests/rwblob_PTIF_bilevel.sh \
	tests/rwblob_PTIF_gray.sh \
	tests/rwblob_PTIF_pallette.sh \
	tests/rwblob_PTIF_truecolor.sh \
	tests/rwblob_RAS_bilevel.sh \
	tests/rwblob_RAS_gray.sh \
	tests/rwblob_RAS_pallette.sh \
	tests/rwblob_RAS_truecolor.sh \
	tests/rwblob_RGBA_truecolor_70x46.sh \
	tests/rwblob_RGB_truecolor_70x46.sh \
	tests/rwblob_SGI_bilevel.sh \
	tests/rwblob_SGI_gray.sh \
	tests/rwblob_SGI_pallette.sh \
	tests/rwblob_SGI_truecolor.sh \
	tests/rwblob_SUN_bilevel.sh \
	tests/rwblob_SUN_gray.sh \
	tests/rwblob_SUN_pallette.sh \
	tests/rwblob_SUN_truecolor.sh \
	tests/rwblob_TGA_bilevel.sh \
	tests/rwblob_TGA_gray.sh \
	tests/rwblob_TGA_pallette.sh \
	tests/rwblob_TGA_truecolor.sh \
	tests/rwblob_TIFF_bilevel.sh \
	tests/rwblob_TIFF_gray.sh \
	tests/rwblob_TIFF_pallette.sh \
	tests/rwblob_TIFF_truecolor.sh \
	tests/rwblob_UYVY_truecolor_70x46.sh \
	tests/rwblob_VDA_bilevel.sh \
	tests/rwblob_VDA_gray.sh \
	tests/rwblob_VDA_pallette.sh \
	tests/rwblob_VDA_truecolor.sh \
	tests/rwblob_VICAR_bilevel.sh \
	tests/rwblob_VICAR_gray.sh \
	tests/rwblob_VICAR_pallette.sh \
	tests/rwblob_VICAR_truecolor.sh \
	tests/rwblob_VIFF_bilevel.sh \
	tests/rwblob_VIFF_gray.sh \
	tests/rwblob_VIFF_pallette.sh \
	tests/rwblob_VIFF_truecolor.sh \
	tests/rwblob_VST_bilevel.sh \
	tests/rwblob_VST_gray.sh \
	tests/rwblob_VST_pallette.sh \
	tests/rwblob_VST_truecolor.sh \
	tests/rwblob_WBMP_bilevel.sh \
	tests/rwblob_WBMP_gray.sh \
	tests/rwblob_WBMP_pallette.sh \
	tests/rwblob_WBMP_truecolor.sh \
	tests/rwblob_XBM_bilevel.sh \
	tests/rwblob_XBM_gray.sh \
	tests/rwblob_XBM_pallette.sh \
	tests/rwblob_XBM_truecolor.sh \
	tests/rwblob_XPM_bilevel.sh \
	tests/rwblob_XPM_gray.sh \
	tests/rwblob_XPM_pallette.sh \
	tests/rwblob_XPM_truecolor.sh \
	tests/rwblob_XWD_bilevel.sh \
	tests/rwblob_XWD_gray.sh \
	tests/rwblob_XWD_pallette.sh \
	tests/rwblob_XWD_truecolor.sh \
	tests/rwblob_YUV_truecolor_70x46.sh \
	tests/rwfile_AVS_bilevel.sh \
	tests/rwfile_AVS_gray.sh \
	tests/rwfile_AVS_pallette.sh \
	tests/rwfile_AVS_truecolor10.sh \
	tests/rwfile_AVS_truecolor12.sh \
	tests/rwfile_AVS_truecolor16.sh \
	tests/rwfile_AVS_truecolor.sh \
	tests/rwfile_BMP_bilevel.sh \
	tests/rwfile_BMP_gray.sh \
	tests/rwfile_BMP_pallette.sh \
	tests/rwfile_BMP_truecolor10.sh \
	tests/rwfile_BMP_truecolor12.sh \
	tests/rwfile_BMP_truecolor16.sh \
	tests/rwfile_BMP_truecolor.sh \
	tests/rwfile_CMYK_truecolor_70x46.sh \
	tests/rwfile_DIB_bilevel.sh \
	tests/rwfile_DIB_gray.sh \
	tests/rwfile_DIB_pallette.sh \
	tests/rwfile_DIB_truecolor10.sh \
	tests/rwfile_DIB_truecolor12.sh \
	tests/rwfile_DIB_truecolor16.sh \
	tests/rwfile_DIB_truecolor.sh \
	tests/rwfile_DPX_bilevel.sh \
	tests/rwfile_DPX_gray.sh \
	tests/rwfile_DPX_pallette.sh \
	tests/rwfile_DPX_truecolor10.sh \
	tests/rwfile_DPX_truecolor12.sh \
	tests/rwfile_DPX_truecolor16.sh \
	tests/rwfile_DPX_truecolor.sh \
	tests/rwfile_EPDF_bilevel.sh \
	tests/rwfile_EPDF_gray.sh \
	tests/rwfile_EPDF_pallette.sh \
	tests/rwfile_EPDF_truecolor10.sh \
	tests/rwfile_EPDF_truecolor12.sh \
	tests/rwfile_EPDF_truecolor16.sh \
	tests/rwfile_EPDF_truecolor.sh \
	tests/rwfile_EPI_bilevel.sh \
	tests/rwfile_EPI_gray.sh \
	tests/rwfile_EPI_pallette.sh \
	tests/rwfile_EPI_truecolor10.sh \
	tests/rwfile_EPI_truecolor12.sh \
	tests/rwfile_EPI_truecolor16.sh \
	tests/rwfile_EPI_truecolor.sh \
	tests/rwfile_EPS_bilevel.sh \
	tests/rwfile_EPSF_bilevel.sh \
	tests/rwfile_EPSF_gray.sh \
	tests/rwfile_EPSF_pallette.sh \
	tests/rwfile_EPSF_truecolor10.sh \
	tests/rwfile_EPSF_truecolor12.sh \
	tests/rwfile_EPSF_truecolor16.sh \
	tests/rwfile_EPSF_truecolor.sh \
	tests/rwfile_EPS_gray.sh \
	tests/rwfile_EPSI_bilevel.sh \
	tests/rwfile_EPSI_gray.sh \
	tests/rwfile_EPSI_pallette.sh \
	tests/rwfile_EPSI_truecolor10.sh \
	tests/rwfile_EPSI_truecolor12.sh \
	tests/rwfile_EPSI_truecolor16.sh \
	tests/rwfile_EPSI_truecolor.sh \
	tests/rwfile_EPS_pallette.sh \
	tests/rwfile_EPS_truecolor10.sh \
	tests/rwfile_EPS_truecolor12.sh \
	tests/rwfile_EPS_truecolor16.sh \
	tests/rwfile_EPS_truecolor.sh \
	tests/rwfile_EPT_bilevel.sh \
	tests/rwfile_EPT_gray.sh \
	tests/rwfile_EPT_pallette.sh \
	tests/rwfile_EPT_truecolor10.sh \
	tests/rwfile_EPT_truecolor12.sh \
	tests/rwfile_EPT_truecolor16.sh \
	tests/rwfile_EPT_truecolor.sh \
	tests/rwfile_FAX_bilevel.sh \
	tests/rwfile_FAX_gray.sh \
	tests/rwfile_FAX_pallette.sh \
	tests/rwfile_FAX_truecolor10.sh \
	tests/rwfile_FAX_truecolor12.sh \
	tests/rwfile_FAX_truecolor16.sh \
	tests/rwfile_FAX_truecolor.sh \
	tests/rwfile_FITS_bilevel.sh \
	tests/rwfile_FITS_gray.sh \
	tests/rwfile_FPX_bilevel.sh \
	tests/rwfile_FPX_gray.sh \
	tests/rwfile_FPX_pallette.sh \
	tests/rwfile_FPX_truecolor10.sh \
	tests/rwfile_FPX_truecolor12.sh \
	tests/rwfile_FPX_truecolor16.sh \
	tests/rwfile_FPX_truecolor.sh \
	tests/rwfile_GIF_bilevel.sh \
	tests/rwfile_GIF_gray.sh \
	tests/rwfile_GIF_pallette.sh \
	tests/rwfile_GIF_truecolor10.sh \
	tests/rwfile_GIF_truecolor12.sh \
	tests/rwfile_GIF_truecolor16.sh \
	tests/rwfile_GIF_truecolor.sh \
	tests/rwfile_GRAY_bilevel.sh \
	tests/rwfile_GRAY_gray.sh \
	tests/rwfile_GRAY_pallette.sh \
	tests/rwfile_GRAY_truecolor10.sh \
	tests/rwfile_GRAY_truecolor12.sh \
	tests/rwfile_GRAY_truecolor16.sh \
	tests/rwfile_GRAY_truecolor_70x46.sh \
	tests/rwfile_GRAY_truecolor.sh \
	tests/rwfile_ICB_bilevel.sh \
	tests/rwfile_ICB_gray.sh \
	tests/rwfile_ICB_pallette.sh \
	tests/rwfile_ICB_truecolor10.sh \
	tests/rwfile_ICB_truecolor12.sh \
	tests/rwfile_ICB_truecolor16.sh \
	tests/rwfile_ICB_truecolor.sh \
	tests/rwfile_JBIG_bilevel.sh \
	tests/rwfile_JBIG_gray.sh \
	tests/rwfile_JBIG_pallette.sh \
	tests/rwfile_JBIG_truecolor10.sh \
	tests/rwfile_JBIG_truecolor12.sh \
	tests/rwfile_JBIG_truecolor16.sh \
	tests/rwfile_JBIG_truecolor.sh \
	tests/rwfile_JP2_bilevel.sh \
	tests/rwfile_JP2_gray.sh \
	tests/rwfile_JP2_pallette.sh \
	tests/rwfile_JP2_truecolor10.sh \
	tests/rwfile_JP2_truecolor12.sh \
	tests/rwfile_JP2_truecolor16.sh \
	tests/rwfile_JP2_truecolor.sh \
	tests/rwfile_JPEG_bilevel.sh \
	tests/rwfile_JPEG_gray.sh \
	tests/rwfile_JPEG_pallette.sh \
	tests/rwfile_JPEG_truecolor10.sh \
	tests/rwfile_JPEG_truecolor12.sh \
	tests/rwfile_JPEG_truecolor16.sh \
	tests/rwfile_JPEG_truecolor.sh \
	tests/rwfile_MIFF_bilevel_bzip.sh \
	tests/rwfile_MIFF_bilevel_none.sh \
	tests/rwfile_MIFF_bilevel_rle.sh \
	tests/rwfile_MIFF_bilevel.sh \
	tests/rwfile_MIFF_bilevel_zip.sh \
	tests/rwfile_MIFF_gray_bzip.sh \
	tests/rwfile_MIFF_gray_none.sh \
	tests/rwfile_MIFF_gray_rle.sh \
	tests/rwfile_MIFF_gray.sh \
	tests/rwfile_MIFF_gray_zip.sh \
	tests/rwfile_MIFF_pallette_bzip.sh \
	tests/rwfile_MIFF_pallette_none.sh \
	tests/rwfile_MIFF_pallette_rle.sh \
	tests/rwfile_MIFF_pallette.sh \
	tests/rwfile_MIFF_pallette_zip.sh \
	tests/rwfile_MIFF_truecolor10.sh \
	tests/rwfile_MIFF_truecolor12.sh \
	tests/rwfile_MIFF_truecolor16.sh \
	tests/rwfile_MIFF_truecolor_bzip.sh \
	tests/rwfile_MIFF_truecolor_none.sh \
	tests/rwfile_MIFF_truecolor_rle.sh \
	tests/rwfile_MIFF_truecolor.sh \
	tests/rwfile_MIFF_truecolor_zip.sh \
	tests/rwfile_MNG_bilevel.sh \
	tests/rwfile_MNG_gray.sh \
	tests/rwfile_MNG_pallette.sh \
	tests/rwfile_MNG_truecolor10.sh \
	tests/rwfile_MNG_truecolor12.sh \
	tests/rwfile_MNG_truecolor16.sh \
	tests/rwfile_MNG_truecolor.sh \
	tests/rwfile_MONO_truecolor_70x46.sh \
	tests/rwfile_MTV_bilevel.sh \
	tests/rwfile_MTV_gray.sh \
	tests/rwfile_MTV_pallette.sh \
	tests/rwfile_MTV_truecolor10.sh \
	tests/rwfile_MTV_truecolor12.sh \
	tests/rwfile_MTV_truecolor16.sh \
	tests/rwfile_MTV_truecolor.sh \
	tests/rwfile_PAL_truecolor_70x46.sh \
	tests/rwfile_PAM_bilevel.sh \
	tests/rwfile_PAM_gray.sh \
	tests/rwfile_PAM_pallette.sh \
	tests/rwfile_PAM_truecolor10.sh \
	tests/rwfile_PAM_truecolor12.sh \
	tests/rwfile_PAM_truecolor16.sh \
	tests/rwfile_PAM_truecolor.sh \
	tests/rwfile_PPM_bilevel.sh \
	tests/rwfile_PPM_gray.sh \
	tests/rwfile_PBM_bilevel.sh \
	tests/rwfile_PBM_gray.sh \
	tests/rwfile_PBM_pallette.sh \
	tests/rwfile_PBM_truecolor10.sh \
	tests/rwfile_PBM_truecolor12.sh \
	tests/rwfile_PBM_truecolor16.sh \
	tests/rwfile_PBM_truecolor.sh \
	tests/rwfile_PCD_bilevel.sh \
	tests/rwfile_PCD_gray.sh \
	tests/rwfile_PCD_pallette.sh \
	tests/rwfile_PCDS_bilevel.sh \
	tests/rwfile_PCDS_gray.sh \
	tests/rwfile_PCDS_pallette.sh \
	tests/rwfile_PCDS_truecolor10.sh \
	tests/rwfile_PCDS_truecolor12.sh \
	tests/rwfile_PCDS_truecolor16.sh \
	tests/rwfile_PCDS_truecolor.sh \
	tests/rwfile_PCD_truecolor10.sh \
	tests/rwfile_PCD_truecolor12.sh \
	tests/rwfile_PCD_truecolor16.sh \
	tests/rwfile_PCD_truecolor.sh \
	tests/rwfile_PCX_bilevel.sh \
	tests/rwfile_PCX_gray.sh \
	tests/rwfile_PCX_pallette.sh \
	tests/rwfile_PCX_truecolor10.sh \
	tests/rwfile_PCX_truecolor12.sh \
	tests/rwfile_PCX_truecolor16.sh \
	tests/rwfile_PCX_truecolor.sh \
	tests/rwfile_PGM_bilevel.sh \
	tests/rwfile_PGM_gray.sh \
	tests/rwfile_PGM_pallette.sh \
	tests/rwfile_PGM_truecolor10.sh \
	tests/rwfile_PGM_truecolor12.sh \
	tests/rwfile_PGM_truecolor16.sh \
	tests/rwfile_PGM_truecolor.sh \
	tests/rwfile_PICON_bilevel.sh \
	tests/rwfile_PICON_gray.sh \
	tests/rwfile_PICON_pallette.sh \
	tests/rwfile_PICON_truecolor10.sh \
	tests/rwfile_PICON_truecolor12.sh \
	tests/rwfile_PICON_truecolor16.sh \
	tests/rwfile_PICON_truecolor.sh \
	tests/rwfile_PICT_bilevel.sh \
	tests/rwfile_PICT_gray.sh \
	tests/rwfile_PICT_pallette.sh \
	tests/rwfile_PICT_truecolor10.sh \
	tests/rwfile_PICT_truecolor12.sh \
	tests/rwfile_PICT_truecolor16.sh \
	tests/rwfile_PICT_truecolor.sh \
	tests/rwfile_PNG_bilevel.sh \
	tests/rwfile_PNG_gray.sh \
	tests/rwfile_PNG_pallette.sh \
	tests/rwfile_PNG_truecolor10.sh \
	tests/rwfile_PNG_truecolor12.sh \
	tests/rwfile_PNG_truecolor16.sh \
	tests/rwfile_PNG_truecolor.sh \
	tests/rwfile_PNM_bilevel.sh \
	tests/rwfile_PNM_gray.sh \
	tests/rwfile_PNM_pallette.sh \
	tests/rwfile_PNM_truecolor10.sh \
	tests/rwfile_PNM_truecolor12.sh \
	tests/rwfile_PNM_truecolor16.sh \
	tests/rwfile_PNM_truecolor.sh \
	tests/rwfile_PPM_bilevel.sh \
	tests/rwfile_PPM_gray.sh \
	tests/rwfile_PPM_pallette.sh \
	tests/rwfile_PPM_truecolor10.sh \
	tests/rwfile_PPM_truecolor12.sh \
	tests/rwfile_PPM_truecolor16.sh \
	tests/rwfile_PPM_truecolor.sh \
	tests/rwfile_PS_bilevel.sh \
	tests/rwfile_PSD_bilevel.sh \
	tests/rwfile_PSD_gray.sh \
	tests/rwfile_PSD_pallette.sh \
	tests/rwfile_PSD_truecolor10.sh \
	tests/rwfile_PSD_truecolor12.sh \
	tests/rwfile_PSD_truecolor16.sh \
	tests/rwfile_PSD_truecolor.sh \
	tests/rwfile_PS_gray.sh \
	tests/rwfile_PS_pallette.sh \
	tests/rwfile_PS_truecolor10.sh \
	tests/rwfile_PS_truecolor12.sh \
	tests/rwfile_PS_truecolor16.sh \
	tests/rwfile_PS_truecolor.sh \
	tests/rwfile_PTIF_bilevel.sh \
	tests/rwfile_PTIF_gray.sh \
	tests/rwfile_PTIF_pallette.sh \
	tests/rwfile_PTIF_truecolor10.sh \
	tests/rwfile_PTIF_truecolor12.sh \
	tests/rwfile_PTIF_truecolor16.sh \
	tests/rwfile_PTIF_truecolor.sh \
	tests/rwfile_RAS_bilevel.sh \
	tests/rwfile_RAS_gray.sh \
	tests/rwfile_RAS_pallette.sh \
	tests/rwfile_RAS_truecolor10.sh \
	tests/rwfile_RAS_truecolor12.sh \
	tests/rwfile_RAS_truecolor16.sh \
	tests/rwfile_RAS_truecolor.sh \
	tests/rwfile_RGBA_truecolor_70x46.sh \
	tests/rwfile_RGB_truecolor_70x46.sh \
	tests/rwfile_SGI_bilevel.sh \
	tests/rwfile_SGI_gray.sh \
	tests/rwfile_SGI_pallette.sh \
	tests/rwfile_SGI_truecolor10.sh \
	tests/rwfile_SGI_truecolor12.sh \
	tests/rwfile_SGI_truecolor16.sh \
	tests/rwfile_SGI_truecolor.sh \
	tests/rwfile_SUN_bilevel.sh \
	tests/rwfile_SUN_gray.sh \
	tests/rwfile_SUN_pallette.sh \
	tests/rwfile_SUN_truecolor10.sh \
	tests/rwfile_SUN_truecolor12.sh \
	tests/rwfile_SUN_truecolor16.sh \
	tests/rwfile_SUN_truecolor.sh \
	tests/rwfile_TGA_bilevel.sh \
	tests/rwfile_TGA_gray.sh \
	tests/rwfile_TGA_pallette.sh \
	tests/rwfile_TGA_truecolor10.sh \
	tests/rwfile_TGA_truecolor12.sh \
	tests/rwfile_TGA_truecolor16.sh \
	tests/rwfile_TGA_truecolor.sh \
	tests/rwfile_TIFF_bilevel.sh \
	tests/rwfile_TIFF_gray.sh \
	tests/rwfile_TIFF_pallette.sh \
	tests/rwfile_TIFF_truecolor10.sh \
	tests/rwfile_TIFF_truecolor12.sh \
	tests/rwfile_TIFF_truecolor16.sh \
	tests/rwfile_TIFF_truecolor.sh \
	tests/rwfile_UYVY_truecolor_70x46.sh \
	tests/rwfile_VDA_bilevel.sh \
	tests/rwfile_VDA_gray.sh \
	tests/rwfile_VDA_pallette.sh \
	tests/rwfile_VDA_truecolor10.sh \
	tests/rwfile_VDA_truecolor12.sh \
	tests/rwfile_VDA_truecolor16.sh \
	tests/rwfile_VDA_truecolor.sh \
	tests/rwfile_VICAR_bilevel.sh \
	tests/rwfile_VICAR_gray.sh \
	tests/rwfile_VICAR_pallette.sh \
	tests/rwfile_VICAR_truecolor10.sh \
	tests/rwfile_VICAR_truecolor12.sh \
	tests/rwfile_VICAR_truecolor16.sh \
	tests/rwfile_VICAR_truecolor.sh \
	tests/rwfile_VIFF_bilevel.sh \
	tests/rwfile_VIFF_gray.sh \
	tests/rwfile_VIFF_pallette.sh \
	tests/rwfile_VIFF_truecolor10.sh \
	tests/rwfile_VIFF_truecolor12.sh \
	tests/rwfile_VIFF_truecolor16.sh \
	tests/rwfile_VIFF_truecolor.sh \
	tests/rwfile_VST_bilevel.sh \
	tests/rwfile_VST_gray.sh \
	tests/rwfile_VST_pallette.sh \
	tests/rwfile_VST_truecolor10.sh \
	tests/rwfile_VST_truecolor12.sh \
	tests/rwfile_VST_truecolor16.sh \
	tests/rwfile_VST_truecolor.sh \
	tests/rwfile_WBMP_bilevel.sh \
	tests/rwfile_WBMP_gray.sh \
	tests/rwfile_WBMP_pallette.sh \
	tests/rwfile_WBMP_truecolor10.sh \
	tests/rwfile_WBMP_truecolor12.sh \
	tests/rwfile_WBMP_truecolor16.sh \
	tests/rwfile_WBMP_truecolor.sh \
	tests/rwfile_XBM_bilevel.sh \
	tests/rwfile_XBM_gray.sh \
	tests/rwfile_XBM_pallette.sh \
	tests/rwfile_XBM_truecolor10.sh \
	tests/rwfile_XBM_truecolor12.sh \
	tests/rwfile_XBM_truecolor16.sh \
	tests/rwfile_XBM_truecolor.sh \
	tests/rwfile_XPM_bilevel.sh \
	tests/rwfile_XPM_gray.sh \
	tests/rwfile_XPM_pallette.sh \
	tests/rwfile_XPM_truecolor10.sh \
	tests/rwfile_XPM_truecolor12.sh \
	tests/rwfile_XPM_truecolor16.sh \
	tests/rwfile_XPM_truecolor.sh \
	tests/rwfile_XWD_bilevel.sh \
	tests/rwfile_XWD_gray.sh \
	tests/rwfile_XWD_pallette.sh \
	tests/rwfile_XWD_truecolor10.sh \
	tests/rwfile_XWD_truecolor12.sh \
	tests/rwfile_XWD_truecolor16.sh \
	tests/rwfile_XWD_truecolor.sh \
	tests/rwfile_YUV_truecolor_70x46.sh

TESTS_EXTRA_DIST = \
	tests/common.shi \
	tests/input_bilevel.miff \
	tests/input_gray.miff \
	tests/input_pallette.miff \
	tests/input_truecolor_70x46.miff \
	tests/input_truecolor.miff \
	tests/input_truecolor10.pam \
	tests/input_truecolor12.pam \
	tests/input_truecolor16.pam \
  $(TESTS_TESTS)

TESTS_CLEANFILES = \
	tests/*out*.*

CHECK_TYPES = bilevel gray pallette truecolor

CHECK_DEEP_TYPES = truecolor10 truecolor12 truecolor16

# Blob formats to test which don't require that size be specified
CHECK_BLOB_FORMATS = \
	AVS \
	BMP \
	DIB \
	DPX \
	EPDF \
	EPT \
	FAX \
	FPX \
	G3 \
	FITS \
	GIF \
	GIF87 \
	JBIG \
	JPEG \
	JP2 \
	MIFF \
	MNG \
	MTV \
	PAM \
	PBM \
	PCD \
	PCDS \
	PCX \
	PGM \
	PICON \
	PICT \
	PNG \
	PNM \
	PPM \
	PSD \
	PTIF \
	RAS \
	SGI \
	SUN \
	TGA \
	TIFF \
	VDA \
	VICAR \
	VIFF \
	VST \
	WBMP \
	XBM \
	XPM \
	XWD

# File formats to test which don't require that size be specified
CHECK_FILE_FORMATS = \
	AVS \
	BMP \
	DIB \
	DPX \
	EPDF \
	EPSF \
	EPSI \
	EPI \
	EPS \
	EPT \
	FAX \
	FITS \
	FPX \
	GIF \
	GRAY \
	ICB \
	JBIG \
	JPEG \
	JP2 \
	MIFF \
	MNG \
	MTV \
	PAM \
	PNM \
	PBM \
	PCD \
	PCDS \
	PCX \
	PGM \
	PICON \
	PICT \
	PNG \
	PNM \
	PPM \
	PS \
	PSD \
	PTIF \
	RAS \
	SGI \
	SUN \
	TGA \
	TIFF \
	VDA \
	VICAR \
	VIFF \
	VST \
	WBMP \
	XBM \
	XPM \
	XWD

# Blob formats to test where size must be specified
CHECK_SIZED_BLOB_FORMATS = \
	CMYK \
	GRAY \
	MONO \
	PAL \
	RGB \
	RGBA \
	UYVY \
	YUV

# File formats to test where size must be specified
CHECK_SIZED_FILE_FORMATS = \
	CMYK \
	GRAY \
	MONO \
	PAL \
	RGB \
	RGBA \
	UYVY \
	YUV

CHECK_CONSTITUTE_MAPS = \
	bgr \
	bgro \
	bgrp \
	rgb \
	rgba \
	rgbo \
	prgb \
	cmy \
	cmyk \
	i

CHECK_CONSTITUTE_STORAGETYPES = \
	char \
	short \
	integer \
	long \
	float \
	double

CHECK_MIFF_FILE_COMPRESS = \
	none \
	bzip \
	zip \
	rle

generate-tests-scripts:
	for storage_type in $(CHECK_CONSTITUTE_STORAGETYPES) ; \
	do \
	  for map in $(CHECK_CONSTITUTE_MAPS) ; \
	  do \
	    sh_file="$(srcdir)/tests/constitute_$${storage_type}_$${map}.sh" ; \
	    echo "#!/bin/sh" > "$${sh_file}" ; \
	    echo "# Copyright (C) 1999-2007 ImageMagick Studio LLC" >> "$${sh_file}" ; \
	    echo "#" >> "$${sh_file}" ; \
	    echo "# This program is covered by multiple licenses, which are described in" >> "$${sh_file}" ; \
	    echo "# LICENSE. You should have received a copy of LICENSE with this" >> "$${sh_file}" ; \
	    echo "# package; otherwise see http://www.imagemagick.org/script/license.php." >> "$${sh_file}" ; \
	    echo "" >> $${sh_file} ; \
	    echo ". \$${srcdir}/tests/common.shi" >> "$${sh_file}" ; \
	    echo "\$${RUNENV} \$${MEMCHECK} ./constitute -storagetype $${storage_type} \$${SRCDIR}/input_truecolor.miff $${map}" >> "$$sh_file" ; \
            chmod +x "$$sh_file" ; \
	  done ; \
	done
	for format in $(CHECK_BLOB_FORMATS) ; \
	do \
	  for type in $(CHECK_TYPES) ; \
	  do \
	    sh_file="$(srcdir)/tests/rwblob_$${format}_$${type}.sh" ; \
	    echo "#!/bin/sh" > "$${sh_file}" ; \
	    echo "# Copyright (C) 1999-2007 ImageMagick Studio LLC" >> "$${sh_file}" ; \
	    echo "#" >> "$${sh_file}" ; \
	    echo "# This program is covered by multiple licenses, which are described in" >> "$${sh_file}" ; \
	    echo "# LICENSE. You should have received a copy of LICENSE with this" >> "$${sh_file}" ; \
	    echo "# package; otherwise see http://www.imagemagick.org/script/license.php." >> "$${sh_file}" ; \
	    echo "" >> $${sh_file} ; \
	    echo ". \$${srcdir}/tests/common.shi" >> "$${sh_file}" ; \
	    echo "\$${RUNENV} \$${MEMCHECK} ./rwblob \$${SRCDIR}/input_$${type}.miff $$format" >> "$$sh_file" ; \
            chmod +x "$$sh_file" ; \
	  done ; \
	done
	for format in $(CHECK_FILE_FORMATS) ; \
	do \
	  for type in  $(CHECK_TYPES) ; \
	  do \
	    sh_file="$(srcdir)/tests/rwfile_$${format}_$${type}.sh" ; \
	    echo "#!/bin/sh" > "$${sh_file}" ; \
	    echo "# Copyright (C) 1999-2007 ImageMagick Studio LLC" >> "$${sh_file}" ; \
	    echo "#" >> "$${sh_file}" ; \
	    echo "# This program is covered by multiple licenses, which are described in" >> "$${sh_file}" ; \
	    echo "# LICENSE. You should have received a copy of LICENSE with this" >> "$${sh_file}" ; \
	    echo "# package; otherwise see http://www.imagemagick.org/script/license.php." >> "$${sh_file}" ; \
	    echo "" >> $${sh_file} ; \
	    echo ". \$${srcdir}/tests/common.shi" >> "$${sh_file}" ; \
	    echo "\$${RUNENV} \$${MEMCHECK} ./rwfile \$${SRCDIR}/input_$${type}.miff $$format" >> "$$sh_file" ; \
            chmod +x "$$sh_file" ; \
	  done ; \
	done
	for format in $(CHECK_SIZED_BLOB_FORMATS) ; \
	do \
	  for type in truecolor_70x46 ; \
	  do \
	    sh_file="$(srcdir)/tests/rwblob_$${format}_$${type}.sh" ; \
	    echo "#!/bin/sh" > "$${sh_file}" ; \
	    echo "# Copyright (C) 1999-2007 ImageMagick Studio LLC" >> "$${sh_file}" ; \
	    echo "#" >> "$${sh_file}" ; \
	    echo "# This program is covered by multiple licenses, which are described in" >> "$${sh_file}" ; \
	    echo "# LICENSE. You should have received a copy of LICENSE with this" >> "$${sh_file}" ; \
	    echo "# package; otherwise see http://www.imagemagick.org/script/license.php." >> "$${sh_file}" ; \
	    echo "" >> $${sh_file} ; \
	    echo ". \$${srcdir}/tests/common.shi" >> "$${sh_file}" ; \
	    echo "\$${RUNENV} \$${MEMCHECK} ./rwblob -size 70x46 \$${SRCDIR}/input_$${type}.miff $$format" >> "$$sh_file" ; \
            chmod +x "$$sh_file" ; \
	  done ; \
	done
	for format in $(CHECK_SIZED_FILE_FORMATS) ; \
	do \
	  for type in truecolor_70x46 ; \
	  do \
	    sh_file="$(srcdir)/tests/rwfile_$${format}_$${type}.sh" ; \
	    echo "#!/bin/sh" > "$${sh_file}" ; \
	    echo "# Copyright (C) 1999-2007 ImageMagick Studio LLC" >> "$${sh_file}" ; \
	    echo "#" >> "$${sh_file}" ; \
	    echo "# This program is covered by multiple licenses, which are described in" >> "$${sh_file}" ; \
	    echo "# LICENSE. You should have received a copy of LICENSE with this" >> "$${sh_file}" ; \
	    echo "# package; otherwise see http://www.imagemagick.org/script/license.php." >> "$${sh_file}" ; \
	    echo "" >> $${sh_file} ; \
	    echo ". \$${srcdir}/tests/common.shi" >> "$${sh_file}" ; \
	    echo "\$${RUNENV} \$${MEMCHECK} ./rwfile -size 70x46 \$${SRCDIR}/input_$${type}.miff $$format" >> "$$sh_file" ; \
            chmod +x "$$sh_file" ; \
	  done ; \
	done
	for compress in $(CHECK_MIFF_FILE_COMPRESS) ; \
	do \
	  for type in $(CHECK_TYPES) ; \
	  do \
	    sh_file="$(srcdir)/tests/rwfile_MIFF_$${type}_$${compress}.sh" ; \
	    echo "#!/bin/sh" > "$${sh_file}" ; \
	    echo "# Copyright (C) 1999-2007 ImageMagick Studio LLC" >> "$${sh_file}" ; \
	    echo "#" >> "$${sh_file}" ; \
	    echo "# This program is covered by multiple licenses, which are described in" >> "$${sh_file}" ; \
	    echo "# LICENSE. You should have received a copy of LICENSE with this" >> "$${sh_file}" ; \
	    echo "# package; otherwise see http://www.imagemagick.org/script/license.php." >> "$${sh_file}" ; \
	    echo "" >> $${sh_file} ; \
	    echo ". \$${srcdir}/tests/common.shi" >> "$${sh_file}" ; \
	    echo "\$${RUNENV} \$${MEMCHECK} ./rwfile -compress $$compress \$${SRCDIR}/input_$${type}.miff MIFF" >> "$$sh_file" ; \
            chmod +x "$$sh_file" ; \
	  done ; \
	done
	for format in $(CHECK_FILE_FORMATS) ; \
	do \
	  for type in  $(CHECK_DEEP_TYPES) ; \
	  do \
	    sh_file="$(srcdir)/tests/rwfile_$${format}_$${type}.sh" ; \
	    echo "#!/bin/sh" > "$${sh_file}" ; \
	    echo "# Copyright (C) 1999-2007 ImageMagick Studio LLC" >> "$${sh_file}" ; \
	    echo "#" >> "$${sh_file}" ; \
	    echo "# This program is covered by multiple licenses, which are described in" >> "$${sh_file}" ; \
	    echo "# LICENSE. You should have received a copy of LICENSE with this" >> "$${sh_file}" ; \
	    echo "# package; otherwise see http://www.imagemagick.org/script/license.php." >> "$${sh_file}" ; \
	    echo "" >> $${sh_file} ; \
	    echo ". \$${srcdir}/tests/common.shi" >> "$${sh_file}" ; \
	    echo "\$${RUNENV} \$${MEMCHECK} ./rwfile \$${SRCDIR}/input_$${type}.pam $$format" >> "$$sh_file" ; \
            chmod +x "$$sh_file" ; \
	  done ; \
	done
