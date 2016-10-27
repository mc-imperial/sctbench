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

  MagickWand mogrify command-line private methods.
*/
#ifndef _MAGICKWAND_MOGRIFY_PRIVATE_H
#define _MAGICKWAND_MOGRIFY_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define MaxImageStackDepth  32
#define MogrifyImageStack(image,advance,fire) \
  if ((j <= i) && (i < argc)) \
    { \
      if ((image) == (Image *) NULL) \
        status&=MogrifyImageInfo(image_info,(int) (i-j+1),(const char **) \
          argv+j,exception); \
      else \
        if ((fire) != MagickFalse) \
          { \
            status&=MogrifyImages(image_info,(int) (i-j+1),(const char **) \
              argv+j,&(image),exception); \
            if ((advance) != MagickFalse) \
              j=i+1; \
            pend=MagickFalse; \
          } \
    }
#define DegreesToRadians(x)  (MagickPI*(x)/180.0)
#define MagickPI  3.14159265358979323846264338327950288419716939937510
#define QuantumScale  ((MagickRealType) 1.0/(MagickRealType) QuantumRange)
#define QuantumTick(i,span) ((MagickBooleanType) ((((i) & ((i)-1)) == 0) || \
   (((i) & 0xfff) == 0) || \
   ((MagickOffsetType) (i) == ((MagickOffsetType) (span)-1))))
#define RadiansToDegrees(x) (180.0*(x)/MagickPI)

static inline MagickRealType MagickPixelIntensity(
  const MagickPixelPacket *pixel)
{
  MagickRealType
    intensity;

  intensity=0.299*pixel->red+0.587*pixel->green+0.114*pixel->blue;
  return(intensity);
}

static inline Quantum MagickPixelIntensityToQuantum(
  const MagickPixelPacket *pixel)
{
  MagickRealType
    intensity;

  intensity=0.299*pixel->red+0.587*pixel->green+0.114*pixel->blue;
  return((Quantum) (intensity+0.5));
}

static inline MagickRealType PixelIntensity(const PixelPacket *pixel)
{
  MagickRealType
    intensity;

  intensity=(MagickRealType) (0.299*pixel->red+0.587*pixel->green+
    0.114*pixel->blue);
  return(intensity);
}

static inline Quantum PixelIntensityToQuantum(const PixelPacket *pixel)
{
  MagickRealType
    intensity;

  intensity=(MagickRealType) (0.299*pixel->red+0.587*pixel->green+
    0.114*pixel->blue);
#if !defined(UseHDRI)
  return((Quantum) (intensity+0.5));
#else
  return((Quantum) intensity);
#endif
}

static inline void SetMagickPixelPacket(const Image *image,
  const PixelPacket *color,const IndexPacket *index,MagickPixelPacket *pixel)
{
  pixel->red=(MagickRealType) color->red;
  pixel->green=(MagickRealType) color->green;
  pixel->blue=(MagickRealType) color->blue;
  if (image->matte != MagickFalse)
    pixel->opacity=(MagickRealType) color->opacity;
  if (((image->colorspace == CMYKColorspace) ||
       (image->storage_class == PseudoClass)) &&
      (index != (const IndexPacket *) NULL))
    pixel->index=(MagickRealType) *index;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
