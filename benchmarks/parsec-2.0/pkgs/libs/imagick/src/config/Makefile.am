#  Copyright 1999-2005 ImageMagick Studio LLC, a non-profit organization
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
#  Makefile for Magick ImageMagick configuration files.

# Where architecture-independent configuration files get installed
# (share/ImageMagick-version)
configsharedir = $(MagickShareConfigPath)
configshare_DATA = \
	config/coder.xml \
	config/log.xml \
	config/magic.xml \
	config/mime.xml \
	config/sRGB.icm \
	config/thresholds.xml

# Where architecture-dependent configuration files get installed
# (lib/ImageMagick-version)
configlibdir =  $(MagickLibConfigPath)
configlib_DATA = \
	config/colors.xml \
	config/configure.xml \
	config/delegates.xml \
	config/english.xml \
	config/francais.xml \
	config/locale.xml \
	config/type.xml \
	config/type-ghostscript.xml \
	config/type-solaris.xml \
	config/type-windows.xml

CONFIG_EXTRA_DIST = \
	config/coder.xml \
	config/colors.xml \
	config/delegates.xml.in \
	config/english.xml \
	config/francais.xml \
	config/ImageMagick.rdf.in \
	config/locale.xml \
	config/log.xml \
	config/magic.xml \
	config/mime.xml \
	config/sRGB.icm \
	config/thresholds.xml \
	config/type-ghostscript.xml.in \
	config/type-solaris.xml.in \
	config/type-windows.xml.in \
	config/type.xml.in
