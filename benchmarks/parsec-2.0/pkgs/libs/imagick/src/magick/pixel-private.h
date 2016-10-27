/*
  Copyright 1999-2007 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.
  
  You may not use this file except in compliance with the License.
  obtain a copy of the License at
  
    http://www.imagemagick.org/script/license.php
  
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore image pixel private methods.
*/
#ifndef _MAGICKCORE_PIXEL_PRIVATE_H
#define _MAGICKCORE_PIXEL_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <magick/exception-private.h>
#include <magick/image.h>
#include <magick/color.h>
#include <magick/image-private.h>
#include <magick/quantum.h>

typedef struct _RealPixelPacket
{
  MagickRealType
    red,
    green,
    blue,
    opacity;
} RealPixelPacket;

static inline MagickPixelPacket *CloneMagickPixelPacket(
  const MagickPixelPacket *pixel)
{
  MagickPixelPacket
    *clone_pixel;

  clone_pixel=(MagickPixelPacket *) AcquireMagickMemory(sizeof(*clone_pixel));
  if (clone_pixel == (MagickPixelPacket *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  *clone_pixel=(*pixel);
  return(clone_pixel);
}

static inline void SetMagickPixelPacket(const Image *image,
  const PixelPacket *color,const IndexPacket *index,MagickPixelPacket *pixel)
{
  pixel->red=(MagickRealType) color->red;
  pixel->green=(MagickRealType) color->green;
  pixel->blue=(MagickRealType) color->blue;
  pixel->opacity=(MagickRealType) color->opacity;
  if (((image->colorspace == CMYKColorspace) ||
       (image->storage_class == PseudoClass)) &&
      (index != (const IndexPacket *) NULL))
    pixel->index=(MagickRealType) *index;
}

static inline void SetPixelPacket(const Image *image,
  const MagickPixelPacket *pixel,PixelPacket *color,IndexPacket *index)
{
  color->red=RoundToQuantum(pixel->red);
  color->green=RoundToQuantum(pixel->green);
  color->blue=RoundToQuantum(pixel->blue);
  color->opacity=RoundToQuantum(pixel->opacity);
  if (((image->colorspace == CMYKColorspace) ||
       (image->storage_class == PseudoClass)) &&
      (index != (const IndexPacket *) NULL))
    *index=RoundToQuantum(pixel->index);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
