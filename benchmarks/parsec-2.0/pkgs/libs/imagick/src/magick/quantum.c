/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                QQQ   U   U   AAA   N   N  TTTTT  U   U  M   M               %
%               Q   Q  U   U  A   A  NN  N    T    U   U  MM MM               %
%               Q   Q  U   U  AAAAA  N N N    T    U   U  M M M               %
%               Q  QQ  U   U  A   A  N  NN    T    U   U  M   M               %
%                QQQQ   UUU   A   A  N   N    T     UUU   M   M               %
%                                                                             %
%                   Methods to Import/Export Quantum Pixels                   %
%                                                                             %
%                             Software Design                                 %
%                               John Cristy                                   %
%                               October 1998                                  %
%                                                                             %
%                                                                             %
%  Copyright 1999-2007 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    http://www.imagemagick.org/script/license.php                            %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/property.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/color-private.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/cache.h"
#include "magick/constitute.h"
#include "magick/delegate.h"
#include "magick/geometry.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/option.h"
#include "magick/pixel.h"
#include "magick/pixel-private.h"
#include "magick/quantum.h"
#include "magick/quantum-private.h"
#include "magick/resource_.h"
#include "magick/semaphore.h"
#include "magick/statistic.h"
#include "magick/stream.h"
#include "magick/string_.h"
#include "magick/utility.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e Q u a n t u m I n f o                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireQuantumInfo() allocates the QuantumInfo structure.
%
%  The format of the AcquireQuantumInfo method is:
%
%      QuantumInfo *AcquireQuantumInfo(const ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
*/
MagickExport QuantumInfo *AcquireQuantumInfo(const ImageInfo *image_info)
{
  QuantumInfo
    *quantum_info;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  quantum_info=(QuantumInfo *) AcquireMagickMemory(sizeof(*quantum_info));
  if (quantum_info == (QuantumInfo *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  (void) ResetMagickMemory(quantum_info,0,sizeof(*quantum_info));
  GetQuantumInfo(image_info,quantum_info);
  return(quantum_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y Q u a n t u m I n f o                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyQuantumInfo() deallocates memory associated with the QuantumInfo
%  structure.
%
%  The format of the DestroyQuantumInfo method is:
%
%      QuantumInfo *DestroyQuantumInfo(QuantumInfo *quantum_info)
%
%  A description of each parameter follows:
%
%    o quantum_info: The quantum_info info.
%
*/
MagickExport QuantumInfo *DestroyQuantumInfo(QuantumInfo *quantum_info)
{
  assert(quantum_info != (QuantumInfo *) NULL);
  assert(quantum_info->signature == MagickSignature);
  if (quantum_info->semaphore != (SemaphoreInfo *) NULL)
    quantum_info->semaphore=DestroySemaphoreInfo(quantum_info->semaphore);
  quantum_info->signature=(~MagickSignature);
  quantum_info=(QuantumInfo *) RelinquishMagickMemory(quantum_info);
  return(quantum_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x p o r t Q u a n t u m P i x e l s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExportQuantumPixels() transfers one or more pixel components from a user
%  supplied buffer into the image pixel cache of an image.  The pixels are
%  expected in network byte order.  It returns MagickTrue if the pixels are
%  successfully transferred, otherwise MagickFalse.
%
%  The format of the ExportQuantumPixels method is:
%
%      MagickBooleanType ExportQuantumPixels(Image *image,
%        const QuantumInfo *quantum_info,const QuantumType quantum_type,
%        const unsigned char *pixels)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o quantum_info: The quantum info.
%
%    o quantum_type: Declare which pixel components to transfer (red, green,
%      blue, opacity, RGB, or RGBA).
%
%    o pixels:  The pixel components are transferred from this buffer.
%
*/
MagickExport MagickBooleanType ExportQuantumPixels(Image *image,
  const QuantumInfo *quantum_info,const QuantumType quantum_type,
  const unsigned char *pixels)
{
  const unsigned char
    *p;

  long
    bit;

  MagickSizeType
    number_pixels;

  QuantumState
    quantum_state;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(quantum_info != (QuantumInfo *) NULL);
  assert(quantum_info->signature == MagickSignature);
  assert(pixels != (const unsigned char *) NULL);
  number_pixels=GetPixelCacheArea(image);
  x=0;
  p=pixels;
  q=GetPixels(image);
  indexes=GetIndexes(image);
  InitializeQuantumState(quantum_info,image->endian,&quantum_state);
  switch (quantum_type)
  {
    case IndexQuantum:
    {
      if (image->storage_class != PseudoClass)
        ThrowBinaryException(ImageError,"ColormappedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 1:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-7); x+=8)
          {
            for (bit=0; bit < 8; bit++)
            {
              pixel=(unsigned char)
                (((*p) & (1 << (7-bit))) != 0 ? 0x01 : 0x00);
              indexes[x+bit]=PushColormapIndex(image,pixel);
              *q=image->colormap[(long) indexes[x+bit]];
              q++;
            }
            p++;
          }
          for (bit=0; bit < (long) (number_pixels % 8); bit++)
          {
            pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ? 0x01 : 0x00);
            indexes[x+bit]=PushColormapIndex(image,pixel);
            *q=image->colormap[(long) indexes[x+bit]];
            q++;
          }
          break;
        }
        case 2:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-3); x+=4)
          {
            pixel=(unsigned char) ((*p >> 6) & 0x03);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[(long) indexes[x]];
            q++;
            pixel=(unsigned char) ((*p >> 4) & 0x03);
            indexes[x+1]=PushColormapIndex(image,pixel);
            *q=image->colormap[(long) indexes[x+1]];
            q++;
            pixel=(unsigned char) ((*p >> 2) & 0x03);
            indexes[x+2]=PushColormapIndex(image,pixel);
            *q=image->colormap[(long) indexes[x+2]];
            q++;
            pixel=(unsigned char) ((*p) & 0x03);
            indexes[x+3]=PushColormapIndex(image,pixel);
            *q=image->colormap[(long) indexes[x+3]];
            p++;
            q++;
          }
          for (bit=0; bit < (long) (number_pixels % 4); bit++)
          {
            pixel=(unsigned char) ((*p >> (2*(3-bit))) & 0x03);
            indexes[x+bit]=PushColormapIndex(image,pixel);
            *q=image->colormap[(long) indexes[x+bit]];
            q++;
          }
          break;
        }
        case 4:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-1); x+=2)
          {
            pixel=(unsigned char) ((*p >> 4) & 0xf);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[(long) indexes[x]];
            q++;
            pixel=(unsigned char) ((*p) & 0xf);
            indexes[x+1]=PushColormapIndex(image,pixel);
            *q=image->colormap[(long) indexes[x+1]];
            p++;
            q++;
          }
          for (bit=0; bit < (long) (number_pixels % 2); bit++)
          {
            pixel=(unsigned char) ((*p++ >> 4) & 0xf);
            indexes[x+bit]=PushColormapIndex(image,pixel);
            *q=image->colormap[(long) indexes[x+bit]];
            q++;
          }
          break;
        }
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushCharPixel(&p);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[(long) indexes[x]];
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 12:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) (number_pixels-1); x+=2)
          {
            pixel=((((*(p+1) >> 4) & 0xf) << 8) | (*p));
            indexes[x]=PushColormapIndex(image,1UL*ScaleAnyToQuantum(pixel,
              image->depth));
            *q=image->colormap[(long) indexes[x]];
            q++;
            pixel=(((*(p+1) & 0xf) << 8) | (*(p+2)));
            indexes[x+1]=PushColormapIndex(image,1UL*ScaleAnyToQuantum(pixel,
              image->depth));
            *q=image->colormap[(long) indexes[x+1]];
            p+=3;
            q++;
          }
          for (bit=0; bit < (long) (number_pixels % 2); bit++)
          {
            pixel=(((*(p+1) >> 4) & 0xf) | (*p));
            indexes[x+bit]=PushColormapIndex(image,1UL*ScaleAnyToQuantum(pixel,
              image->depth));
            *q=image->colormap[(long) indexes[x+bit]];
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushShortPixel(&quantum_state,&p);
            indexes[x]=PushColormapIndex(image,1UL*ScaleShortToQuantum(pixel));
            *q=image->colormap[(long) indexes[x]];
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushFloatPixel(&quantum_state,&p);
                indexes[x]=PushColormapIndex(image,1UL*RoundToQuantum(pixel));
                *q=image->colormap[(long) indexes[x]];
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushLongPixel(&quantum_state,&p);
            indexes[x]=PushColormapIndex(image,1UL*ScaleLongToQuantum(pixel));
            *q=image->colormap[(long) indexes[x]];
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register double
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushDoublePixel(&quantum_state,&p);
                indexes[x]=PushColormapIndex(image,1UL*RoundToQuantum(pixel));
                *q=image->colormap[(long) indexes[x]];
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
        }
        default:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            indexes[x]=PushColormapIndex(image,1UL*ScaleAnyToQuantum(pixel,
              image->depth));
            *q=image->colormap[(long) indexes[x]];
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
      }
      break;
    }
    case IndexAlphaQuantum:
    {
      if (image->storage_class != PseudoClass)
        ThrowBinaryException(ImageError,"ColormappedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 1:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-3); x+=4)
          {
            for (bit=0; bit < 8; bit+=2)
            {
              pixel=(unsigned char)
                (((*p) & (1 << (7-bit))) != 0 ? 0x00 : 0x01);
              indexes[x+bit/2]=(IndexPacket) (pixel == 0 ? 0 : 1);
              q->red=(Quantum) (pixel == 0 ? 0 : QuantumRange);
              q->green=q->red;
              q->blue=q->red;
              q->opacity=(Quantum) (((*p) & (1UL << (unsigned char) (6-bit)))
                == 0 ? TransparentOpacity : OpaqueOpacity);
              q++;
            }
            p++;
          }
          for (bit=0; bit < (long) (number_pixels % 4); bit+=2)
          {
            pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ? 0x00 : 0x01);
            indexes[x+bit/2]=(IndexPacket) (pixel == 0 ? 0 : 1);
            q->red=(Quantum) (pixel == 0 ? 0 : QuantumRange);
            q->green=q->red;
            q->blue=q->red;
            q->opacity=(Quantum) (((*p) & (1UL << (unsigned char) (6-bit))) ==
              0 ? TransparentOpacity : OpaqueOpacity);
            q++;
          }
          break;
        }
        case 2:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-1); x+=2)
          {
            pixel=(unsigned char) ((*p >> 6) & 0x03);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[(long) indexes[x]];
            q->opacity=(Quantum) ((MagickRealType) QuantumRange*((int)
              (*p >> 4) & 0x03)/4);
            q++;
            pixel=(unsigned char) ((*p >> 2) & 0x03);
            indexes[x+2]=PushColormapIndex(image,pixel);
            *q=image->colormap[(long) indexes[x+2]];
            q->opacity=(Quantum) ((MagickRealType) QuantumRange*((int)
              (*p) & 0x03)/4);
            p++;
            q++;
          }
          break;
        }
        case 4:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=(unsigned char) ((*p >> 4) & 0xf);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[(long) indexes[x]];
            q->opacity=(Quantum) ((MagickRealType) QuantumRange-
              ((MagickRealType) QuantumRange*((int) (*p) & 0xf)/15));
            p++;
            q++;
          }
          break;
        }
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushCharPixel(&p);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[(long) indexes[x]];
            pixel=PushCharPixel(&p);
            q->opacity=(Quantum) ((MagickRealType) QuantumRange-
              ScaleCharToQuantum(pixel));
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 12:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=((((*(p+1) >> 4) & 0xf) << 8) | (*p));
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[(long) indexes[x]];
            pixel=(((*(p+1) & 0xf) << 8) | (*(p+2)));
            q->opacity=(Quantum) ((unsigned long) QuantumRange*pixel/1024);
            p+=3;
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushShortPixel(&quantum_state,&p);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[(long) indexes[x]];
            pixel=PushShortPixel(&quantum_state,&p);
            q->opacity=(Quantum) QuantumRange-ScaleShortToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushFloatPixel(&quantum_state,&p);
                indexes[x]=PushColormapIndex(image,1UL*RoundToQuantum(pixel));
                *q=image->colormap[(long) indexes[x]];
                pixel=PushFloatPixel(&quantum_state,&p);
                q->opacity=(Quantum) QuantumRange-RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushLongPixel(&quantum_state,&p);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[(long) indexes[x]];
            pixel=PushLongPixel(&quantum_state,&p);
            q->opacity=(Quantum) QuantumRange-ScaleLongToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register double
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushDoublePixel(&quantum_state,&p);
                indexes[x]=PushColormapIndex(image,1UL*RoundToQuantum(pixel));
                *q=image->colormap[(long) indexes[x]];
                pixel=PushDoublePixel(&quantum_state,&p);
                q->opacity=(Quantum) QuantumRange-RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
        }
        default:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            indexes[x]=PushColormapIndex(image,1UL*ScaleAnyToQuantum(pixel,
              image->depth));
            *q=image->colormap[(long) indexes[x]];
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->opacity=(Quantum) QuantumRange-ScaleAnyToQuantum(pixel,image->depth);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
      }
      break;
    }
    case GrayQuantum:
    {
      switch (image->depth)
      {
        case 1:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-7); x+=8)
          {
            for (bit=0; bit < 8; bit++)
            {
              if (quantum_info->min_is_white == MagickFalse)
                pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ? 255 : 0);
              else
                pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ? 0 : 255);
              q->red=(Quantum) (pixel == 0 ? 0 : QuantumRange);
              q->green=q->red;
              q->blue=q->red;
              q++;
            }
            p++;
          }
          for (bit=0; bit < (long) (number_pixels % 8); bit++)
          {
            if (quantum_info->min_is_white == MagickFalse)
              pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ? 255 : 0);
            else
              pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ? 0 : 255);
            q->red=(Quantum) (pixel == 0 ? 0 : QuantumRange);
            q->green=q->red;
            q->blue=q->red;
            q++;
          }
          break;
        }
        case 2:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-3); x+=4)
          {
            pixel=(unsigned char) ((*p >> 6) & 0x03);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            q->green=q->red;
            q->blue=q->red;
            q++;
            pixel=(unsigned char) ((*p >> 4) & 0x03);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            q->green=q->red;
            q->blue=q->red;
            q++;
            pixel=(unsigned char) ((*p >> 2) & 0x03);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            q->green=q->red;
            q->blue=q->red;
            q++;
            pixel=(unsigned char) ((*p) & 0x03);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            q->green=q->red;
            q->blue=q->red;
            p++;
            q++;
          }
          for (bit=0; bit < (long) (number_pixels % 4); bit++)
          {
            pixel=(unsigned char) ((*p >> (2*(3-bit))) & 0x03);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            q->green=q->red;
            q->blue=q->red;
            q++;
          }
          break;
        }
        case 4:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-1); x+=2)
          {
            pixel=(unsigned char) ((*p >> 4) & 0xf);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            q->green=q->red;
            q->blue=q->red;
            q++;
            pixel=(unsigned char) ((*p) & 0xf);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            q->green=q->red;
            q->blue=q->red;
            p++;
            q++;
          }
          for (bit=0; bit < (long) (number_pixels % 2); bit++)
          {
            pixel=(unsigned char) ((*p++ >> 4) & 0xf);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            q->green=q->red;
            q->blue=q->red;
            q++;
          }
          break;
        }
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushCharPixel(&p);
            q->red=ScaleCharToQuantum(pixel);
            q->green=q->red;
            q->blue=q->red;
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 10:
        {
          register unsigned long
            pixel;

          if (quantum_info->pack == MagickFalse)
            {
              if (image->endian == MSBEndian)
                {
                  for (x=0; x < (long) number_pixels/3; x++)
                  {
                    pixel=PushLongPixel(&quantum_state,&p);
                    q->red=ScaleAnyToQuantum((pixel >> 0) & 0x3ff,image->depth);
                    q->green=q->red;
                    q->blue=q->red;
                    q++;
                    q->red=ScaleAnyToQuantum((pixel >> 10) & 0x3ff,
                      image->depth);
                    q->green=q->red;
                    q->blue=q->red;
                    q++;
                    q->red=ScaleAnyToQuantum((pixel >> 20) & 0x3ff,
                      image->depth);
                    q->green=q->red;
                    q->blue=q->red;
                    p+=quantum_info->pad*sizeof(pixel);
                    q++;
                  }
                  break;
                }
              for (x=0; x < (long) number_pixels/3; x++)
              {
                pixel=PushLongPixel(&quantum_state,&p);
                q->red=ScaleAnyToQuantum((pixel >> 22) & 0x3ff,image->depth);
                q->green=q->red;
                q->blue=q->red;
                q++;
                q->red=ScaleAnyToQuantum((pixel >> 12) & 0x3ff,image->depth);
                q->green=q->red;
                q->blue=q->red;
                q++;
                q->red=ScaleAnyToQuantum((pixel >> 2) & 0x3ff,image->depth);
                q->green=q->red;
                q->blue=q->red;
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            q->green=q->red;
            q->blue=q->red;
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 12:
        {
          register unsigned long
            pixel;

          if (quantum_info->pack == MagickFalse)
            {
              for (x=0; x < (long) (number_pixels-1); x+=2)
              {
                pixel=PushShortPixel(&quantum_state,&p);
                q->red=ScaleAnyToQuantum(pixel >> 4,image->depth);
                q->green=q->red;
                q->blue=q->red;
                q++;
                pixel=PushShortPixel(&quantum_state,&p);
                q->red=ScaleAnyToQuantum(pixel >> 4,image->depth);
                q->green=q->red;
                q->blue=q->red;
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              for (bit=0; bit < (long) (number_pixels % 2); bit++)
              {
                pixel=PushShortPixel(&quantum_state,&p);
                q->red=ScaleAnyToQuantum(pixel >> 4,image->depth);
                q->green=q->red;
                q->blue=q->red;
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            q->green=q->red;
            q->blue=q->red;
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushShortPixel(&quantum_state,&p);
            q->red=ScaleShortToQuantum(pixel);
            q->green=q->red;
            q->blue=q->red;
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushFloatPixel(&quantum_state,&p);
                q->red=RoundToQuantum(pixel);
                q->green=q->red;
                q->blue=q->red;
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushLongPixel(&quantum_state,&p);
            q->red=ScaleLongToQuantum(pixel);
            q->green=q->red;
            q->blue=q->red;
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushDoublePixel(&quantum_state,&p);
                q->red=RoundToQuantum(pixel);
                q->green=q->red;
                q->blue=q->red;
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
        }
        default:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            q->green=q->red;
            q->blue=q->red;
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
      }
      break;
    }
    case GrayAlphaQuantum:
    {
      switch (image->depth)
      {
        case 1:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-3); x+=4)
          {
            for (bit=0; bit < 8; bit+=2)
            {
              pixel=(unsigned char)
                (((*p) & (1 << (7-bit))) != 0 ? 0x00 : 0x01);
              q->red=(Quantum) (pixel == 0 ? 0 : QuantumRange);
              q->green=q->red;
              q->blue=q->red;
              q->opacity=(Quantum) (((*p) & (1UL << (unsigned char) (6-bit)))
                == 0 ? TransparentOpacity : OpaqueOpacity);
              q++;
            }
            p++;
          }
          for (bit=0; bit < (long) (number_pixels % 4); bit+=2)
          {
            pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ? 0x00 : 0x01);
            q->red=(Quantum) (pixel == 0 ? 0 : QuantumRange);
            q->green=q->red;
            q->blue=q->red;
            q->opacity=(Quantum) (((*p) & (1UL << (unsigned char) (6-bit))) == 0
              ? TransparentOpacity : OpaqueOpacity);
            q++;
          }
          break;
        }
        case 2:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-1); x+=2)
          {
            pixel=(unsigned char) ((*p >> 6) & 0x03);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            q->green=q->red;
            q->blue=q->red;
            q->opacity=(Quantum) ((MagickRealType) QuantumRange*((int)
              (*p >> 4) & 0x03)/4);
            q++;
            pixel=(unsigned char) ((*p >> 2) & 0x03);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            q->green=q->red;
            q->blue=q->red;
            q->opacity=(Quantum) ((MagickRealType) QuantumRange*((int)
              (*p) & 0x03)/4);
            p++;
            q++;
          }
          break;
        }
        case 4:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=(unsigned char) ((*p >> 4) & 0xf);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            q->green=q->red;
            q->blue=q->red;
            q->opacity=(Quantum) ((MagickRealType) QuantumRange-(QuantumRange*
              ((*p) & 0xf)/15));
            p++;
            q++;
          }
          break;
        }
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushCharPixel(&p);
            q->red=ScaleCharToQuantum(pixel);
            q->green=q->red;
            q->blue=q->red;
            pixel=PushCharPixel(&p);
            q->opacity=(Quantum) ((MagickRealType) QuantumRange-
              ScaleCharToQuantum(pixel));
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 10:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            q->green=q->red;
            q->blue=q->red;
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->opacity=ScaleAnyToQuantum(pixel,image->depth);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 12:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            q->green=q->red;
            q->blue=q->red;
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->opacity=ScaleAnyToQuantum(pixel,image->depth);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushShortPixel(&quantum_state,&p);
            q->red=ScaleShortToQuantum(pixel);
            q->green=q->red;
            q->blue=q->red;
            pixel=PushShortPixel(&quantum_state,&p);
            q->opacity=(Quantum) QuantumRange-ScaleShortToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushFloatPixel(&quantum_state,&p);
                q->red=RoundToQuantum(pixel);
                q->green=q->red;
                q->blue=q->red;
                pixel=PushFloatPixel(&quantum_state,&p);
                q->opacity=(Quantum) QuantumRange-RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushLongPixel(&quantum_state,&p);
            q->red=ScaleLongToQuantum(pixel);
            q->green=q->red;
            q->blue=q->red;
            pixel=PushLongPixel(&quantum_state,&p);
            q->opacity=(Quantum) QuantumRange-ScaleLongToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushDoublePixel(&quantum_state,&p);
                q->red=RoundToQuantum(pixel);
                q->green=q->red;
                q->blue=q->red;
                pixel=PushDoublePixel(&quantum_state,&p);
                q->opacity=(Quantum) QuantumRange-RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
        }
        default:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            q->green=q->red;
            q->blue=q->red;
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->opacity=(Quantum) QuantumRange-ScaleAnyToQuantum(pixel,image->depth);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
      }
      break;
    }
    case RedQuantum:
    case CyanQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushCharPixel(&p);
            q->red=ScaleCharToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushShortPixel(&quantum_state,&p);
            q->red=ScaleShortToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushFloatPixel(&quantum_state,&p);
                q->red=RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushLongPixel(&quantum_state,&p);
            q->red=ScaleLongToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushDoublePixel(&quantum_state,&p);
                q->red=RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
        }
        default:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
      }
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushCharPixel(&p);
            q->green=ScaleCharToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushShortPixel(&quantum_state,&p);
            q->green=ScaleShortToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushFloatPixel(&quantum_state,&p);
                q->green=RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushLongPixel(&quantum_state,&p);
            q->green=ScaleLongToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushDoublePixel(&quantum_state,&p);
                q->green=RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
        }
        default:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->green=ScaleAnyToQuantum(pixel,image->depth);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
      }
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushCharPixel(&p);
            q->blue=ScaleCharToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushShortPixel(&quantum_state,&p);
            q->blue=ScaleShortToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushFloatPixel(&quantum_state,&p);
                q->blue=RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushLongPixel(&quantum_state,&p);
            q->blue=ScaleLongToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushDoublePixel(&quantum_state,&p);
                q->blue=RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
        }
        default:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->blue=ScaleAnyToQuantum(pixel,image->depth);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
      }
      break;
    }
    case AlphaQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushCharPixel(&p);
            q->opacity=(Quantum) QuantumRange-ScaleCharToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushShortPixel(&quantum_state,&p);
            q->opacity=(Quantum) QuantumRange-ScaleShortToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushFloatPixel(&quantum_state,&p);
                q->opacity=(Quantum) QuantumRange-RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushLongPixel(&quantum_state,&p);
            q->opacity=(Quantum) QuantumRange-ScaleLongToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushDoublePixel(&quantum_state,&p);
                q->opacity=(Quantum) QuantumRange-RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
        }
        default:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->opacity=(Quantum) QuantumRange-ScaleAnyToQuantum(pixel,image->depth);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
      }
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace != CMYKColorspace)
        ThrowBinaryException(ImageError,"ColorSeparatedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushCharPixel(&p);
            indexes[x]=ScaleCharToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushShortPixel(&quantum_state,&p);
            indexes[x]=ScaleShortToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushFloatPixel(&quantum_state,&p);
                indexes[x]=RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushLongPixel(&quantum_state,&p);
            indexes[x]=ScaleLongToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushDoublePixel(&quantum_state,&p);
                indexes[x]=RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
        }
        default:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            indexes[x]=ScaleAnyToQuantum(pixel,image->depth);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
      }
      break;
    }
    case RGBQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushCharPixel(&p);
            q->red=ScaleCharToQuantum(pixel);
            pixel=PushCharPixel(&p);
            q->green=ScaleCharToQuantum(pixel);
            pixel=PushCharPixel(&p);
            q->blue=ScaleCharToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 10:
        {
          register unsigned long
            pixel;

          if (quantum_info->pack == MagickFalse)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushLongPixel(&quantum_state,&p);
                q->red=ScaleAnyToQuantum((pixel >> 22) & 0x3ff,image->depth);
                q->green=ScaleAnyToQuantum((pixel >> 12) & 0x3ff,image->depth);
                q->blue=ScaleAnyToQuantum((pixel >> 2) & 0x3ff,image->depth);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          if (quantum_info->quantum == 32UL)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushQuantumLongPixel(&quantum_state,image->depth,&p);
                q->red=ScaleAnyToQuantum(pixel,image->depth);
                pixel=PushQuantumLongPixel(&quantum_state,image->depth,&p);
                q->green=ScaleAnyToQuantum(pixel,image->depth);
                pixel=PushQuantumLongPixel(&quantum_state,image->depth,&p);
                q->blue=ScaleAnyToQuantum(pixel,image->depth);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->green=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->blue=ScaleAnyToQuantum(pixel,image->depth);
            q++;
          }
          break;
        }
        case 12:
        {
          register unsigned long
            pixel;

          if (quantum_info->pack == MagickFalse)
            {
              for (x=0; x < (long) (3*number_pixels-1); x+=2)
              {
                pixel=PushShortPixel(&quantum_state,&p);
                switch (x % 3)
                {
                  default:
                  case 0:
                  {
                    q->red=ScaleAnyToQuantum(pixel >> 4,image->depth);
                    break;
                  }
                  case 1:
                  {
                    q->green=ScaleAnyToQuantum(pixel >> 4,image->depth);
                    break;
                  }
                  case 2:
                  {
                    q->blue=ScaleAnyToQuantum(pixel >> 4,image->depth);
                    q++;
                    break;
                  }
                }
                pixel=PushShortPixel(&quantum_state,&p);
                switch ((x+1) % 3)
                {
                  default:
                  case 0:
                  {
                    q->red=ScaleAnyToQuantum(pixel >> 4,image->depth);
                    break;
                  }
                  case 1:
                  {
                    q->green=ScaleAnyToQuantum(pixel >> 4,image->depth);
                    break;
                  }
                  case 2:
                  {
                    q->blue=ScaleAnyToQuantum(pixel >> 4,image->depth);
                    q++;
                    break;
                  }
                }
                p+=quantum_info->pad*sizeof(pixel);
              }
              for (bit=0; bit < (long) (3*number_pixels % 2); bit++)
              {
                pixel=PushShortPixel(&quantum_state,&p);
                switch ((x+bit) % 3)
                {
                  default:
                  case 0:
                  {
                    q->red=ScaleAnyToQuantum(pixel >> 4,image->depth);
                    break;
                  }
                  case 1:
                  {
                    q->green=ScaleAnyToQuantum(pixel >> 4,image->depth);
                    break;
                  }
                  case 2:
                  {
                    q->blue=ScaleAnyToQuantum(pixel >> 4,image->depth);
                    q++;
                    break;
                  }
                }
                p+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          if (quantum_info->quantum == 32UL)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushQuantumLongPixel(&quantum_state,image->depth,&p);
                q->red=ScaleAnyToQuantum(pixel,image->depth);
                pixel=PushQuantumLongPixel(&quantum_state,image->depth,&p);
                q->green=ScaleAnyToQuantum(pixel,image->depth);
                pixel=PushQuantumLongPixel(&quantum_state,image->depth,&p);
                q->blue=ScaleAnyToQuantum(pixel,image->depth);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->green=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->blue=ScaleAnyToQuantum(pixel,image->depth);
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushShortPixel(&quantum_state,&p);
            q->red=ScaleShortToQuantum(pixel);
            pixel=PushShortPixel(&quantum_state,&p);
            q->green=ScaleShortToQuantum(pixel);
            pixel=PushShortPixel(&quantum_state,&p);
            q->blue=ScaleShortToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushFloatPixel(&quantum_state,&p);
                q->red=RoundToQuantum(pixel);
                pixel=PushFloatPixel(&quantum_state,&p);
                q->green=RoundToQuantum(pixel);
                pixel=PushFloatPixel(&quantum_state,&p);
                q->blue=RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushLongPixel(&quantum_state,&p);
            q->red=ScaleLongToQuantum(pixel);
            pixel=PushLongPixel(&quantum_state,&p);
            q->green=ScaleLongToQuantum(pixel);
            pixel=PushLongPixel(&quantum_state,&p);
            q->blue=ScaleLongToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register double
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushDoublePixel(&quantum_state,&p);
                q->red=RoundToQuantum(pixel);
                pixel=PushDoublePixel(&quantum_state,&p);
                q->green=RoundToQuantum(pixel);
                pixel=PushDoublePixel(&quantum_state,&p);
                q->blue=RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
        }
        default:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->green=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->blue=ScaleAnyToQuantum(pixel,image->depth);
            q++;
          }
          break;
        }
      }
      break;
    }
    case RGBAQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushCharPixel(&p);
            q->red=ScaleCharToQuantum(pixel);
            pixel=PushCharPixel(&p);
            q->green=ScaleCharToQuantum(pixel);
            pixel=PushCharPixel(&p);
            q->blue=ScaleCharToQuantum(pixel);
            pixel=PushCharPixel(&p);
            q->opacity=(Quantum) QuantumRange-ScaleCharToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 10:
        {
          register unsigned long
            pixel;

          if (quantum_info->pack == MagickFalse)
            {
              long
                n;

              register long
                i;

              unsigned long
                quantum;

              n=0;
              pixel=0;
              quantum=0;
              for (x=0; x < (long) number_pixels; x++)
              {
                for (i=0; i < 4; i++)
                {
                  switch (n % 3)
                  {
                    case 0:
                    {
                      pixel=PushLongPixel(&quantum_state,&p);
                      quantum=(unsigned long) (ScaleShortToQuantum(
                        (unsigned short) (((pixel >> 22) & 0x3ff) << 6)));
                      break;
                    }
                    case 1:
                    {
                      quantum=(unsigned long) (ScaleShortToQuantum(
                        (unsigned short) (((pixel >> 12) & 0x3ff) << 6)));
                      break;
                    }
                    case 2:
                    {
                      quantum=(unsigned long) (ScaleShortToQuantum(
                        (unsigned short) (((pixel >> 2) & 0x3ff) << 6)));
                      break;
                    }
                  }
                  switch (i)
                  {
                    case 0: q->red=(Quantum) (quantum); break;
                    case 1: q->green=(Quantum) (quantum); break;
                    case 2: q->blue=(Quantum) (quantum); break;
                    case 3: q->opacity=(Quantum) (QuantumRange-quantum); break;
                  }
                  n++;
                }
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->red=ScaleShortToQuantum((unsigned short) (pixel << 6));
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->green=ScaleShortToQuantum((unsigned short) (pixel << 6));
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->blue=ScaleShortToQuantum((unsigned short) (pixel << 6));
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->opacity=(Quantum) (QuantumRange-ScaleShortToQuantum(
              (unsigned short) (pixel << 6)));
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushShortPixel(&quantum_state,&p);
            q->red=ScaleShortToQuantum(pixel);
            pixel=PushShortPixel(&quantum_state,&p);
            q->green=ScaleShortToQuantum(pixel);
            pixel=PushShortPixel(&quantum_state,&p);
            q->blue=ScaleShortToQuantum(pixel);
            pixel=PushShortPixel(&quantum_state,&p);
            q->opacity=(Quantum) QuantumRange-ScaleShortToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushFloatPixel(&quantum_state,&p);
                q->red=RoundToQuantum(pixel);
                pixel=PushFloatPixel(&quantum_state,&p);
                q->green=RoundToQuantum(pixel);
                pixel=PushFloatPixel(&quantum_state,&p);
                q->blue=RoundToQuantum(pixel);
                pixel=PushFloatPixel(&quantum_state,&p);
                q->opacity=(Quantum) QuantumRange-RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushLongPixel(&quantum_state,&p);
            q->red=ScaleLongToQuantum(pixel);
            pixel=PushLongPixel(&quantum_state,&p);
            q->green=ScaleLongToQuantum(pixel);
            pixel=PushLongPixel(&quantum_state,&p);
            q->blue=ScaleLongToQuantum(pixel);
            pixel=PushLongPixel(&quantum_state,&p);
            q->opacity=(Quantum) QuantumRange-ScaleLongToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register double
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushDoublePixel(&quantum_state,&p);
                q->red=RoundToQuantum(pixel);
                pixel=PushDoublePixel(&quantum_state,&p);
                q->green=RoundToQuantum(pixel);
                pixel=PushDoublePixel(&quantum_state,&p);
                q->blue=RoundToQuantum(pixel);
                pixel=PushDoublePixel(&quantum_state,&p);
                q->opacity=(Quantum) QuantumRange-RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
        }
        default:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->green=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->blue=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->opacity=(Quantum) QuantumRange-ScaleAnyToQuantum(pixel,image->depth);
            q++;
          }
          break;
        }
      }
      break;
    }
    case RGBOQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushCharPixel(&p);
            q->red=ScaleCharToQuantum(pixel);
            pixel=PushCharPixel(&p);
            q->green=ScaleCharToQuantum(pixel);
            pixel=PushCharPixel(&p);
            q->blue=ScaleCharToQuantum(pixel);
            pixel=PushCharPixel(&p);
            q->opacity=ScaleCharToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushShortPixel(&quantum_state,&p);
            q->red=ScaleShortToQuantum(pixel);
            pixel=PushShortPixel(&quantum_state,&p);
            q->green=ScaleShortToQuantum(pixel);
            pixel=PushShortPixel(&quantum_state,&p);
            q->blue=ScaleShortToQuantum(pixel);
            pixel=PushShortPixel(&quantum_state,&p);
            q->opacity=ScaleShortToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushFloatPixel(&quantum_state,&p);
                q->red=RoundToQuantum(pixel);
                pixel=PushFloatPixel(&quantum_state,&p);
                q->green=RoundToQuantum(pixel);
                pixel=PushFloatPixel(&quantum_state,&p);
                q->blue=RoundToQuantum(pixel);
                pixel=PushFloatPixel(&quantum_state,&p);
                q->opacity=RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushLongPixel(&quantum_state,&p);
            q->red=ScaleLongToQuantum(pixel);
            pixel=PushLongPixel(&quantum_state,&p);
            q->green=ScaleLongToQuantum(pixel);
            pixel=PushLongPixel(&quantum_state,&p);
            q->blue=ScaleLongToQuantum(pixel);
            pixel=PushLongPixel(&quantum_state,&p);
            q->opacity=ScaleLongToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register double
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushDoublePixel(&quantum_state,&p);
                q->red=RoundToQuantum(pixel);
                pixel=PushDoublePixel(&quantum_state,&p);
                q->green=RoundToQuantum(pixel);
                pixel=PushDoublePixel(&quantum_state,&p);
                q->blue=RoundToQuantum(pixel);
                pixel=PushDoublePixel(&quantum_state,&p);
                q->opacity=RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
        }
        default:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->green=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->blue=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->opacity=ScaleAnyToQuantum(pixel,image->depth);
            q++;
          }
          break;
        }
      }
      break;
    }
    case CMYKQuantum:
    {
      if (image->colorspace != CMYKColorspace)
        ThrowBinaryException(ImageError,"ColorSeparatedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushCharPixel(&p);
            q->red=ScaleCharToQuantum(pixel);
            pixel=PushCharPixel(&p);
            q->green=ScaleCharToQuantum(pixel);
            pixel=PushCharPixel(&p);
            q->blue=ScaleCharToQuantum(pixel);
            pixel=PushCharPixel(&p);
            indexes[x]=ScaleCharToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushShortPixel(&quantum_state,&p);
            q->red=ScaleShortToQuantum(pixel);
            pixel=PushShortPixel(&quantum_state,&p);
            q->green=ScaleShortToQuantum(pixel);
            pixel=PushShortPixel(&quantum_state,&p);
            q->blue=ScaleShortToQuantum(pixel);
            pixel=PushShortPixel(&quantum_state,&p);
            indexes[x]=ScaleShortToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushFloatPixel(&quantum_state,&p);
                q->red=RoundToQuantum(pixel);
                pixel=PushFloatPixel(&quantum_state,&p);
                q->green=RoundToQuantum(pixel);
                pixel=PushFloatPixel(&quantum_state,&p);
                q->blue=RoundToQuantum(pixel);
                pixel=PushFloatPixel(&quantum_state,&p);
                indexes[x]=(IndexPacket) RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushLongPixel(&quantum_state,&p);
            q->red=ScaleLongToQuantum(pixel);
            pixel=PushLongPixel(&quantum_state,&p);
            q->green=ScaleLongToQuantum(pixel);
            pixel=PushLongPixel(&quantum_state,&p);
            q->blue=ScaleLongToQuantum(pixel);
            pixel=PushLongPixel(&quantum_state,&p);
            indexes[x]=ScaleLongToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register double
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushDoublePixel(&quantum_state,&p);
                q->red=RoundToQuantum(pixel);
                pixel=PushDoublePixel(&quantum_state,&p);
                q->green=RoundToQuantum(pixel);
                pixel=PushDoublePixel(&quantum_state,&p);
                q->blue=RoundToQuantum(pixel);
                pixel=PushDoublePixel(&quantum_state,&p);
                indexes[x]=(IndexPacket) RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
        }
        default:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->green=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->blue=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            indexes[x]=ScaleAnyToQuantum(pixel,image->depth);
            q++;
          }
          break;
        }
      }
      break;
    }
    case CMYKAQuantum:
    {
      if (image->colorspace != CMYKColorspace)
        ThrowBinaryException(ImageError,"ColorSeparatedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushCharPixel(&p);
            q->red=ScaleCharToQuantum(pixel);
            pixel=PushCharPixel(&p);
            q->green=ScaleCharToQuantum(pixel);
            pixel=PushCharPixel(&p);
            q->blue=ScaleCharToQuantum(pixel);
            pixel=PushCharPixel(&p);
            indexes[x]=ScaleCharToQuantum(pixel);
            pixel=PushCharPixel(&p);
            q->opacity=(Quantum) QuantumRange-ScaleCharToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushShortPixel(&quantum_state,&p);
            q->red=ScaleShortToQuantum(pixel);
            pixel=PushShortPixel(&quantum_state,&p);
            q->green=ScaleShortToQuantum(pixel);
            pixel=PushShortPixel(&quantum_state,&p);
            q->blue=ScaleShortToQuantum(pixel);
            pixel=PushShortPixel(&quantum_state,&p);
            indexes[x]=ScaleShortToQuantum(pixel);
            pixel=PushShortPixel(&quantum_state,&p);
            q->opacity=(Quantum) QuantumRange-ScaleShortToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register float
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushFloatPixel(&quantum_state,&p);
                q->red=RoundToQuantum(pixel);
                pixel=PushFloatPixel(&quantum_state,&p);
                q->green=RoundToQuantum(pixel);
                pixel=PushFloatPixel(&quantum_state,&p);
                q->blue=RoundToQuantum(pixel);
                pixel=PushFloatPixel(&quantum_state,&p);
                indexes[x]=(IndexPacket) RoundToQuantum(pixel);
                pixel=PushFloatPixel(&quantum_state,&p);
                q->opacity=(Quantum) QuantumRange-RoundToQuantum(pixel);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushLongPixel(&quantum_state,&p);
            q->red=ScaleLongToQuantum(pixel);
            pixel=PushLongPixel(&quantum_state,&p);
            q->green=ScaleLongToQuantum(pixel);
            pixel=PushLongPixel(&quantum_state,&p);
            q->blue=ScaleLongToQuantum(pixel);
            pixel=PushLongPixel(&quantum_state,&p);
            indexes[x]=ScaleLongToQuantum(pixel);
            pixel=PushLongPixel(&quantum_state,&p);
            q->opacity=(Quantum) QuantumRange-ScaleLongToQuantum(pixel);
            p+=quantum_info->pad*sizeof(pixel);
            q++;
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register double
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=PushDoublePixel(&quantum_state,&p);
                q->red=RoundToQuantum(pixel);
                pixel=PushDoublePixel(&quantum_state,&p);
                q->green=RoundToQuantum(pixel);
                pixel=PushDoublePixel(&quantum_state,&p);
                q->blue=RoundToQuantum(pixel);
                pixel=PushDoublePixel(&quantum_state,&p);
                indexes[x]=(IndexPacket) RoundToQuantum(pixel);
                pixel=PushDoublePixel(&quantum_state,&p);
                q->opacity=(Quantum) QuantumRange-RoundToQuantum(pixel);
                pixel=PushDoublePixel(&quantum_state,&p);
                p+=quantum_info->pad*sizeof(pixel);
                q++;
              }
              break;
            }
        }
        default:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->red=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->green=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->blue=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            indexes[x]=ScaleAnyToQuantum(pixel,image->depth);
            pixel=PushQuantumPixel(&quantum_state,image->depth,&p);
            q->opacity=(Quantum) QuantumRange-ScaleAnyToQuantum(pixel,
              image->depth);
            q++;
          }
          break;
        }
      }
      break;
    }
    default:
      break;
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t Q u a n t u m I n f o                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetQuantumInfo() initializes the QuantumInfo structure to default values.
%
%  The format of the GetQuantumInfo method is:
%
%      GetQuantumInfo(const ImageInfo *image_info,QuantumInfo *quantum_info)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o quantum_info: The quantum info.
%
*/
MagickExport void GetQuantumInfo(const ImageInfo *image_info,
  QuantumInfo *quantum_info)
{
  const char
    *option;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(quantum_info != (QuantumInfo *) NULL);
  (void) ResetMagickMemory(quantum_info,0,sizeof(*quantum_info));
  quantum_info->quantum=8;
  option=GetImageOption(image_info,"quantum:format");
  if (option != (char *) NULL)
    quantum_info->format=(QuantumFormatType) ParseMagickOption(
      MagickQuantumFormatOptions,MagickFalse,option);
  quantum_info->minimum=0.0;
  option=GetImageOption(image_info,"quantum:minimum");
  if (option != (char *) NULL)
    quantum_info->minimum=atof(option);
  quantum_info->maximum=1.0;
  option=GetImageOption(image_info,"quantum:maximum");
  if (option != (char *) NULL)
    quantum_info->maximum=atof(option);
  if ((quantum_info->minimum == 0.0) && (quantum_info->maximum == 0.0))
    quantum_info->scale=0.0;
  else
    if (quantum_info->minimum == quantum_info->maximum)
      {
        quantum_info->scale=(MagickRealType) QuantumRange/quantum_info->minimum;
        quantum_info->minimum=0.0;
      }
    else
      quantum_info->scale=(MagickRealType) QuantumRange/(quantum_info->maximum-
        quantum_info->minimum);
  option=GetImageOption(image_info,"quantum:scale");
  if (option != (char *) NULL)
    quantum_info->scale=atof(option);
  option=GetImageOption(image_info,"quantum:polarity");
  if (option != (char *) NULL)
    quantum_info->min_is_white=LocaleCompare(option,"min-is-white") == 0 ?
      MagickTrue : MagickFalse;
  quantum_info->pad=0;
  quantum_info->pack=MagickTrue;
  quantum_info->signature=MagickSignature;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I m p o r t Q u a n t u m P i x e l s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImportQuantumPixels() transfers one or more pixel components from the image
%  pixel cache to a user supplied buffer.  The pixels are returned in network
%  byte order.  MagickTrue is returned if the pixels are successfully
%  transferred, otherwise MagickFalse.
%
%  The format of the ImportQuantumPixels method is:
%
%      MagickBooleanType ImportQuantumPixels(Image *image,
%        const QuantumInfo *quantum_info,const QuantumType quantum_type,
%        unsigned char *pixels)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o quantum_info: The quantum info.
%
%    o quantum_type: Declare which pixel components to transfer (RGB, RGBA,
%      etc).
%
%    o pixels:  The components are transferred to this buffer.
%
*/
MagickExport MagickBooleanType ImportQuantumPixels(Image *image,
  const QuantumInfo *quantum_info,const QuantumType quantum_type,
  unsigned char *pixels)
{
  long
    bit;

  MagickSizeType
    number_pixels;

  QuantumState
    quantum_state;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *p;

  unsigned char
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(quantum_info != (QuantumInfo *) NULL);
  assert(quantum_info->signature == MagickSignature);
  assert(pixels != (unsigned char *) NULL);
  number_pixels=GetPixelCacheArea(image);
  x=0;
  p=GetPixels(image);
  indexes=GetIndexes(image);
  q=pixels;
  InitializeQuantumState(quantum_info,image->endian,&quantum_state);
  switch (quantum_type)
  {
    case IndexQuantum:
    {
      if (image->storage_class != PseudoClass)
        ThrowBinaryException(ImageError,"ColormappedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 1:
        {
          register unsigned char
            pixel;

          for (x=((long) number_pixels-7); x > 0; x-=8)
          {
            pixel=(unsigned char) *indexes++;
            *q=((pixel & 0x01) << 7);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 6);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 5);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 4);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 3);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 2);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 1);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 0);
            q++;
          }
          if ((number_pixels % 8) != 0)
            {
              *q='\0';
              for (bit=7; bit >= (long) (8-(number_pixels % 8)); bit--)
              {
                pixel=(unsigned char) *indexes++;
                *q|=((pixel & 0x01) << (unsigned char) bit);
              }
              q++;
            }
          break;
        }
        case 2:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-3); x+=4)
          {
            pixel=(unsigned char) *indexes++;
            *q=((pixel & 0x03) << 6);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x03) << 4);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x03) << 2);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x03) << 0);
            q++;
          }
          if ((number_pixels % 4) != 0)
            {
              *q='\0';
              for (i=3; i >= (4-((long) number_pixels % 4)); i--)
              {
                pixel=(unsigned char) *indexes++;
                *q|=((pixel & 0x03) << ((unsigned char) i*2));
              }
              q++;
            }
          break;
        }
        case 4:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) (number_pixels-1) ; x+=2)
          {
            pixel=(unsigned char) *indexes++;
            *q=((pixel & 0xf) << 4);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0xf) << 0);
            q++;
          }
          if ((number_pixels % 2) != 0)
            {
              pixel=(unsigned char) *indexes++;
              *q=((pixel & 0xf) << 4);
              q++;
            }
          break;
        }
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopCharPixel((unsigned char) indexes[x],&q);
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopShortPixel(&quantum_state,(unsigned short) indexes[x],&q);
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 32:
        {
          register float
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=(float)  indexes[x];
                PopFloatPixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            PopLongPixel(&quantum_state,(unsigned long) indexes[x],&q);
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 64:
        {
          register double
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=(double) indexes[x];
                PopDoublePixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
        }
        default:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              indexes[x],image->depth),&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
      }
      break;
    }
    case IndexAlphaQuantum:
    {
      if (image->storage_class != PseudoClass)
        ThrowBinaryException(ImageError,"ColormappedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 1:
        {
          register unsigned char
            pixel;

          for (x=((long) number_pixels-3); x > 0; x-=4)
          {
            pixel=(unsigned char) *indexes++;
            *q=((pixel & 0x01) << 7);
            pixel=(unsigned char) (p->opacity == (Quantum) TransparentOpacity ?
              1 : 0);
            *q|=((pixel & 0x01) << 6);
            p++;
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 5);
            pixel=(unsigned char) (p->opacity == (Quantum) TransparentOpacity ?
              1 : 0);
            *q|=((pixel & 0x01) << 4);
            p++;
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 3);
            pixel=(unsigned char) (p->opacity == (Quantum) TransparentOpacity ?
              1 : 0);
            *q|=((pixel & 0x01) << 2);
            p++;
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 1);
            pixel=(unsigned char) (p->opacity == (Quantum) TransparentOpacity ?
              1 : 0);
            *q|=((pixel & 0x01) << 0);
            p++;
            q++;
          }
          if ((number_pixels % 4) != 0)
            {
              *q='\0';
              for (bit=3; bit >= (long) (4-(number_pixels % 4)); bit-=2)
              {
                pixel=(unsigned char) *indexes++;
                *q|=((pixel & 0x01) << (unsigned char) bit);
                pixel=(unsigned char) (p->opacity == (Quantum) TransparentOpacity ? 1 : 0);
                *q|=((pixel & 0x01) << (unsigned char) (bit-1));
                p++;
              }
              q++;
            }
          break;
        }
        case 2:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=(unsigned char) *indexes++;
            *q=((pixel & 0x03) << 6);
            pixel=(unsigned char) (4*QuantumScale*p->opacity+0.5);
            *q|=((pixel & 0x03) << 4);
            p++;
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x03) << 2);
            pixel=(unsigned char) (4*QuantumScale*p->opacity+0.5);
            *q|=((pixel & 0x03) << 0);
            p++;
            q++;
          }
          break;
        }
        case 4:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels ; x++)
          {
            pixel=(unsigned char) *indexes++;
            *q=((pixel & 0xf) << 4);
            pixel=(unsigned char) (16*QuantumScale*((Quantum)
              ((MagickRealType) QuantumRange-p->opacity))+0.5);
            *q|=((pixel & 0xf) << 0);
            p++;
            q++;
          }
          break;
        }
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopCharPixel((unsigned char) indexes[x],&q);
            pixel=ScaleQuantumToChar((Quantum) ((MagickRealType)
              QuantumRange-p->opacity));
            PopCharPixel(pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopShortPixel(&quantum_state,(unsigned short) indexes[x],&q);
            pixel=ScaleQuantumToShort((Quantum) ((MagickRealType)
              QuantumRange-p->opacity));
            PopShortPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register float
                  pixel;

                pixel=(float)  indexes[x];
                PopFloatPixel(&quantum_state,pixel,&q);
                pixel=(float)  ((MagickRealType) QuantumRange-p->opacity);
                PopFloatPixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            PopLongPixel(&quantum_state,(unsigned long) indexes[x],&q);
            pixel=ScaleQuantumToLong((Quantum) ((MagickRealType)
              QuantumRange-p->opacity));
            PopLongPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register double
                  pixel;

                pixel=(double) indexes[x];
                PopDoublePixel(&quantum_state,pixel,&q);
                pixel=(double) ((MagickRealType) QuantumRange-p->opacity);
                PopDoublePixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
        }
        default:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              indexes[x],image->depth),&q);
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              (Quantum) ((MagickRealType) QuantumRange-p->opacity),
              image->depth),&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
      }
      break;
    }
    case GrayQuantum:
    {
      switch (image->depth)
      {
        case 1:
        {
          for (x=((long) number_pixels-7); x > 0; x-=8)
          {
            *q='\0';
            if (quantum_info->min_is_white == MagickFalse)
              *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x01 :
                0x00) << 7;
            else
              *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x00 :
                0x01) << 7;
            p++;
            if (quantum_info->min_is_white == MagickFalse)
              *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x01 :
                0x00) << 6;
            else
              *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x00 :
                0x01) << 6;
            p++;
            if (quantum_info->min_is_white == MagickFalse)
              *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x01 :
                0x00) << 5;
            else
              *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x00 :
                0x01) << 5;
            p++;
            if (quantum_info->min_is_white == MagickFalse)
              *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x01 :
                0x00) << 4;
            else
              *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x00 :
                0x01) << 4;
            p++;
            if (quantum_info->min_is_white == MagickFalse)
              *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x01 :
                0x00) << 3;
            else
              *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x00 :
                0x01) << 3;
            p++;
            if (quantum_info->min_is_white == MagickFalse)
              *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x01 :
                0x00) << 2;
            else
              *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x00 :
                0x01) << 2;
            p++;
            if (quantum_info->min_is_white == MagickFalse)
              *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x01 :
                0x00) << 1;
            else
              *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x00 :
                0x01) << 1;
            p++;
            if (quantum_info->min_is_white == MagickFalse)
              *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x01 :
                0x00) << 0;
            else
              *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x00 :
                0x01) << 0;
            p++;
            q++;
          }
          if ((number_pixels % 8) != 0)
            {
              *q='\0';
              for (bit=7; bit >= (long) (8-(number_pixels % 8)); bit--)
              {
                if (quantum_info->min_is_white == MagickFalse)
                  *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x01 :
                    0x00) << bit;
                else
                  *q|=(PixelIntensity(p) > (1.0*QuantumRange/2.0) ? 0x00 :
                    0x01) << bit;
                p++;
              }
              q++;
            }
          break;
        }
        case 2:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-3); x+=4)
          {
            *q='\0';
            pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            *q|=((pixel & 0x03) << 6);
            p++;
            pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            *q|=((pixel & 0x03) << 4);
            p++;
            pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            *q|=((pixel & 0x03) << 2);
            p++;
            pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            *q|=((pixel & 0x03));
            p++;
            q++;
          }
          if ((number_pixels % 4) != 0)
            {
              *q='\0';
              for (i=3; i >= (4-((long) number_pixels % 4)); i--)
              {
                pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
                *q|=(pixel << ((unsigned char) i*2));
                p++;
              }
              q++;
            }
          break;
        }
        case 4:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) (number_pixels-1) ; x+=2)
          {
            pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            *q=((pixel & 0xf) << 4);
            p++;
            pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            *q|=((pixel & 0xf) << 0);
            p++;
            q++;
          }
          if ((number_pixels % 2) != 0)
            {
              pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
              *q=((pixel & 0xf) << 4);
              p++;
              q++;
            }
          break;
        }
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            PopCharPixel(pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 10:
        {
          register unsigned short
            pixel;

          if (quantum_info->pack == MagickFalse)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=ScaleQuantumToShort(PixelIntensityToQuantum(p));
                PopShortPixel(&quantum_state,(unsigned short) ScaleQuantumToAny(
                  (Quantum) pixel,image->depth),&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              PixelIntensityToQuantum(p),image->depth),&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 12:
        {
          register unsigned short
            pixel;

          if (quantum_info->pack == MagickFalse)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=ScaleQuantumToShort(PixelIntensityToQuantum(p));
                PopShortPixel(&quantum_state,(unsigned short) (pixel >> 4),&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              PixelIntensityToQuantum(p),image->depth),&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(PixelIntensityToQuantum(p));
            PopShortPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register float
                  pixel;

                pixel=(float) PixelIntensityToQuantum(p);
                PopFloatPixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(PixelIntensityToQuantum(p));
            PopLongPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register double
                  pixel;

                pixel=(double) PixelIntensityToQuantum(p);
                PopDoublePixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
        }
        default:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              PixelIntensityToQuantum(p),image->depth),&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
      }
      break;
    }
    case GrayAlphaQuantum:
    {
      switch (image->depth)
      {
        case 1:
        {
          register unsigned char
            pixel;

          for (x=((long) number_pixels-3); x > 0; x-=4)
          {
            pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            *q=(unsigned char) (((int) pixel != 0 ? 0x00 : 0x01) << 7);
            pixel=(unsigned char) (p->opacity == OpaqueOpacity ? 0x00 : 0x01);
            *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 6);
            p++;
            pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 5);
            pixel=(unsigned char) (p->opacity == OpaqueOpacity ? 0x00 : 0x01);
            *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 4);
            p++;
            pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 3);
            pixel=(unsigned char) (p->opacity == OpaqueOpacity ? 0x00 : 0x01);
            *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 2);
            p++;
            pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 1);
            pixel=(unsigned char) (p->opacity == OpaqueOpacity ? 0x00 : 0x01);
            *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 0);
            p++;
            q++;
          }
          if ((number_pixels % 4) != 0)
            {
              *q='\0';
              for (bit=3; bit >= (long) (4-(number_pixels % 4)); bit-=2)
              {
                pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
                *q|=(((int) pixel != 0 ? 0x00 : 0x01) << (unsigned char) bit);
                pixel=(unsigned char) (p->opacity == OpaqueOpacity ?
                  0x00 : 0x01);
                *q|=(((int) pixel != 0 ? 0x00 : 0x01) <<
                  (unsigned char) (bit-1));
                p++;
              }
              q++;
            }
          break;
        }
        case 2:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            *q=((pixel & 0x03) << 6);
            pixel=(unsigned char) (4*QuantumScale*p->opacity+0.5);
            *q|=((pixel & 0x03) << 4);
            p++;
            pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            *q|=((pixel & 0x03) << 2);
            pixel=(unsigned char) (4*QuantumScale*p->opacity+0.5);
            *q|=((pixel & 0x03) << 0);
            p++;
            q++;
          }
          break;
        }
        case 4:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels ; x++)
          {
            pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            *q=((pixel & 0xf) << 4);
            pixel=(unsigned char) (16*QuantumScale*((Quantum)
              ((MagickRealType) QuantumRange-p->opacity))+0.5);
            *q|=((pixel & 0xf) << 0);
            p++;
            q++;
          }
          break;
        }
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            PopCharPixel(pixel,&q);
            pixel=ScaleQuantumToChar((Quantum) ((MagickRealType)
              QuantumRange-p->opacity));
            PopCharPixel(pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(PixelIntensityToQuantum(p));
            PopShortPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToShort((Quantum) ((MagickRealType) QuantumRange-
              p->opacity));
            PopShortPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register float
                  pixel;

                pixel=(float) PixelIntensityToQuantum(p);
                PopFloatPixel(&quantum_state,pixel,&q);
                pixel=(float) ((MagickRealType) QuantumRange-p->opacity);
                PopFloatPixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(PixelIntensityToQuantum(p));
            PopLongPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToLong((Quantum) ((MagickRealType) QuantumRange-
              p->opacity));
            PopLongPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register double
                  pixel;

                pixel=(double) PixelIntensityToQuantum(p);
                PopDoublePixel(&quantum_state,pixel,&q);
                pixel=(double) ((MagickRealType) QuantumRange-p->opacity);
                PopDoublePixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
        }
        default:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              PixelIntensityToQuantum(p),image->depth),&q);
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              (Quantum) ((MagickRealType) QuantumRange-p->opacity),
              image->depth),&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
      }
      break;
    }
    case RedQuantum:
    case CyanQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(p->red);
            PopCharPixel(pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(p->red);
            PopShortPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register float
                  pixel;

                pixel=(float) p->red;
                PopFloatPixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(p->red);
            PopLongPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register double
                  pixel;

                pixel=(double) p->red;
                PopDoublePixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
        }
        default:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->red,image->depth),&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
      }
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(p->green);
            PopCharPixel(pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(p->green);
            PopShortPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register float
                  pixel;

                pixel=(float) p->green;
                PopFloatPixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(p->green);
            PopLongPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register double
                  pixel;

                pixel=(double) p->green;
                PopDoublePixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
        }
        default:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->green,image->depth),&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
      }
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(p->blue);
            PopCharPixel(pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(p->blue);
            PopShortPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register float
                  pixel;

                pixel=(float) p->blue;
                PopFloatPixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(p->blue);
            PopLongPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register double
                  pixel;

                pixel=(double) p->blue;
                PopDoublePixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
        }
        default:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->blue,image->depth),&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
      }
      break;
    }
    case AlphaQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar((Quantum) ((MagickRealType) QuantumRange-
              p->opacity));
            PopCharPixel(pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort((Quantum) ((MagickRealType) QuantumRange-
              p->opacity));
            PopShortPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register float
                  pixel;

                pixel=(float) ((MagickRealType) QuantumRange-p->opacity);
                PopFloatPixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong((Quantum) ((MagickRealType) QuantumRange-
              p->opacity));
            PopLongPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register double
                  pixel;

                pixel=(double) ((MagickRealType) QuantumRange-p->opacity);
                PopDoublePixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
        }
        default:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              (Quantum) ((MagickRealType) QuantumRange-p->opacity),
              image->depth),&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
      }
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace != CMYKColorspace)
        ThrowBinaryException(ImageError,"ColorSeparatedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(indexes[x]);
            PopCharPixel(pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(indexes[x]);
            PopShortPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register float
                  pixel;

                pixel=(float) indexes[x];
                PopFloatPixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(indexes[x]);
            PopLongPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register double
                  pixel;

                pixel=(double) indexes[x];
                PopDoublePixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
        }
        default:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              (Quantum) indexes[x],image->depth),&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
      }
      break;
    }
    case RGBQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(p->red);
            PopCharPixel(pixel,&q);
            pixel=ScaleQuantumToChar(p->green);
            PopCharPixel(pixel,&q);
            pixel=ScaleQuantumToChar(p->blue);
            PopCharPixel(pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 10:
        {
          register unsigned long
            pixel;

          if (quantum_info->pack == MagickFalse)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=ScaleQuantumToAny(p->red,image->depth) << 22 |
                  ScaleQuantumToAny(p->green,image->depth) <<  12 |
                  ScaleQuantumToAny(p->blue,image->depth) << 2;
                PopLongPixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          if (quantum_info->quantum == 32UL)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=ScaleQuantumToAny(p->red,image->depth);
                PopQuantumLongPixel(&quantum_state,image->depth,pixel,&q);
                pixel=ScaleQuantumToAny(p->green,image->depth);
                PopQuantumLongPixel(&quantum_state,image->depth,pixel,&q);
                pixel=ScaleQuantumToAny(p->blue,image->depth);
                PopQuantumLongPixel(&quantum_state,image->depth,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToAny(p->red,image->depth);
            PopQuantumPixel(&quantum_state,image->depth,pixel,&q);
            pixel=ScaleQuantumToAny(p->green,image->depth);
            PopQuantumPixel(&quantum_state,image->depth,pixel,&q);
            pixel=ScaleQuantumToAny(p->blue,image->depth);
            PopQuantumPixel(&quantum_state,image->depth,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 12:
        {
          register unsigned long
            pixel;

          if (quantum_info->pack == MagickFalse)
            {
              for (x=0; x < (long) (3*number_pixels-1); x+=2)
              {
                switch (x % 3)
                {
                  default:
                  case 0:
                  {
                    pixel=ScaleQuantumToAny(p->red,image->depth);
                    break;
                  }
                  case 1:
                  {
                    pixel=ScaleQuantumToAny(p->green,image->depth);
                    break;
                  }
                  case 2:
                  {
                    pixel=ScaleQuantumToAny(p->blue,image->depth);
                    p++;
                    break;
                  }
                }
                PopShortPixel(&quantum_state,(unsigned short) (pixel << 4),&q);
                switch ((x+1) % 3)
                {
                  default:
                  case 0:
                  {
                    pixel=ScaleQuantumToAny(p->red,image->depth);
                    break;
                  }
                  case 1:
                  {
                    pixel=ScaleQuantumToAny(p->green,image->depth);
                    break;
                  }
                  case 2:
                  {
                    pixel=ScaleQuantumToAny(p->blue,image->depth);
                    p++;
                    break;
                  }
                }
                PopShortPixel(&quantum_state,(unsigned short) (pixel << 4),&q);
                q+=quantum_info->pad*sizeof(pixel);
              }
              for (bit=0; bit < (long) (3*number_pixels % 2); bit++)
              {
                switch ((x+bit) % 3)
                {
                  default:
                  case 0:
                  {
                    pixel=ScaleQuantumToAny(p->red,image->depth);
                    break;
                  }
                  case 1:
                  {
                    pixel=ScaleQuantumToAny(p->green,image->depth);
                    break;
                  }
                  case 2:
                  {
                    pixel=ScaleQuantumToAny(p->blue,image->depth);
                    p++;
                    break;
                  }
                }
                PopShortPixel(&quantum_state,(unsigned short) (pixel << 4),&q);
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          if (quantum_info->quantum == 32UL)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=ScaleQuantumToAny(p->red,image->depth);
                PopQuantumLongPixel(&quantum_state,image->depth,pixel,&q);
                pixel=ScaleQuantumToAny(p->green,image->depth);
                PopQuantumLongPixel(&quantum_state,image->depth,pixel,&q);
                pixel=ScaleQuantumToAny(p->blue,image->depth);
                PopQuantumLongPixel(&quantum_state,image->depth,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToAny(p->red,image->depth);
            PopQuantumPixel(&quantum_state,image->depth,pixel,&q);
            pixel=ScaleQuantumToAny(p->green,image->depth);
            PopQuantumPixel(&quantum_state,image->depth,pixel,&q);
            pixel=ScaleQuantumToAny(p->blue,image->depth);
            PopQuantumPixel(&quantum_state,image->depth,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(p->red);
            PopShortPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToShort(p->green);
            PopShortPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToShort(p->blue);
            PopShortPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register float
                  pixel;

                pixel=(float) p->red;
                PopFloatPixel(&quantum_state,pixel,&q);
                pixel=(float) p->green;
                PopFloatPixel(&quantum_state,pixel,&q);
                pixel=(float) p->blue;
                PopFloatPixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(p->red);
            PopLongPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToLong(p->green);
            PopLongPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToLong(p->blue);
            PopLongPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register double
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=(double) p->red;
                PopDoublePixel(&quantum_state,pixel,&q);
                pixel=(double) p->green;
                PopDoublePixel(&quantum_state,pixel,&q);
                pixel=(double) p->blue;
                PopDoublePixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
        }
        default:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->red,image->depth),&q);
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->green,image->depth),&q);
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->blue,image->depth),&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
      }
      break;
    }
    case RGBAQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(p->red);
            PopCharPixel(pixel,&q);
            pixel=ScaleQuantumToChar(p->green);
            PopCharPixel(pixel,&q);
            pixel=ScaleQuantumToChar(p->blue);
            PopCharPixel(pixel,&q);
            pixel=ScaleQuantumToChar((Quantum) ((MagickRealType) QuantumRange-
              p->opacity));
            PopCharPixel(pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(p->red);
            PopShortPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToShort(p->green);
            PopShortPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToShort(p->blue);
            PopShortPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToShort((Quantum) ((MagickRealType) QuantumRange-
              p->opacity));
            PopShortPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register float
                  pixel;

                pixel=(float) p->red;
                PopFloatPixel(&quantum_state,pixel,&q);
                pixel=(float) p->green;
                PopFloatPixel(&quantum_state,pixel,&q);
                pixel=(float) p->blue;
                PopFloatPixel(&quantum_state,pixel,&q);
                pixel=(float) ((MagickRealType) QuantumRange-p->opacity);
                PopFloatPixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(p->red);
            PopLongPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToLong(p->green);
            PopLongPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToLong(p->blue);
            PopLongPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToLong((Quantum) ((MagickRealType) QuantumRange-
              p->opacity));
            PopLongPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register double
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=(double) p->red;
                PopDoublePixel(&quantum_state,pixel,&q);
                pixel=(double) p->green;
                PopDoublePixel(&quantum_state,pixel,&q);
                pixel=(double) p->blue;
                PopDoublePixel(&quantum_state,pixel,&q);
                pixel=(double) ((MagickRealType) QuantumRange-p->opacity);
                PopDoublePixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
        }
        default:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->red,image->depth),&q);
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->green,image->depth),&q);
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->blue,image->depth),&q);
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              (Quantum) ((MagickRealType) QuantumRange-p->opacity),
              image->depth),&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
      }
      break;
    }
    case RGBOQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(p->red);
            PopCharPixel(pixel,&q);
            pixel=ScaleQuantumToChar(p->green);
            PopCharPixel(pixel,&q);
            pixel=ScaleQuantumToChar(p->blue);
            PopCharPixel(pixel,&q);
            pixel=ScaleQuantumToChar(p->opacity);
            PopCharPixel(pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(p->red);
            PopShortPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToShort(p->green);
            PopShortPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToShort(p->blue);
            PopShortPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToShort(p->opacity);
            PopShortPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register float
                  pixel;

                pixel=(float) p->red;
                PopFloatPixel(&quantum_state,pixel,&q);
                pixel=(float) p->green;
                PopFloatPixel(&quantum_state,pixel,&q);
                pixel=(float) p->blue;
                PopFloatPixel(&quantum_state,pixel,&q);
                pixel=(float) p->opacity;
                PopFloatPixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(p->red);
            PopLongPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToLong(p->green);
            PopLongPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToLong(p->blue);
            PopLongPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToLong(p->opacity);
            PopLongPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register double
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=(double) p->red;
                PopDoublePixel(&quantum_state,pixel,&q);
                pixel=(double) p->green;
                PopDoublePixel(&quantum_state,pixel,&q);
                pixel=(double) p->blue;
                PopDoublePixel(&quantum_state,pixel,&q);
                pixel=(double) p->opacity;
                PopDoublePixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
        }
        default:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->red,image->depth),&q);
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->green,image->depth),&q);
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->blue,image->depth),&q);
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->opacity,image->depth),&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
      }
      break;
    }
    case CMYKQuantum:
    {
      if (image->colorspace != CMYKColorspace)
        ThrowBinaryException(ImageError,"ColorSeparatedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(p->red);
            PopCharPixel(pixel,&q);
            pixel=ScaleQuantumToChar(p->green);
            PopCharPixel(pixel,&q);
            pixel=ScaleQuantumToChar(p->blue);
            PopCharPixel(pixel,&q);
            pixel=ScaleQuantumToChar(indexes[x]);
            PopCharPixel(pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(p->red);
            PopShortPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToShort(p->green);
            PopShortPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToShort(p->blue);
            PopShortPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToShort(indexes[x]);
            PopShortPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register float
                  pixel;

                pixel=(float) p->red;
                PopFloatPixel(&quantum_state,pixel,&q);
                pixel=(float) p->green;
                PopFloatPixel(&quantum_state,pixel,&q);
                pixel=(float) p->blue;
                PopFloatPixel(&quantum_state,pixel,&q);
                pixel=(float) indexes[x];
                PopFloatPixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(p->red);
            PopLongPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToLong(p->green);
            PopLongPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToLong(p->blue);
            PopLongPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToLong(indexes[x]);
            PopLongPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register double
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=(double) p->red;
                PopDoublePixel(&quantum_state,pixel,&q);
                pixel=(double) p->green;
                PopDoublePixel(&quantum_state,pixel,&q);
                pixel=(double) p->blue;
                PopDoublePixel(&quantum_state,pixel,&q);
                pixel=(double) indexes[x];
                PopDoublePixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
        }
        default:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->red,image->depth),&q);
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->green,image->depth),&q);
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->blue,image->depth),&q);
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              indexes[x],image->depth),&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
      }
      break;
    }
    case CMYKAQuantum:
    {
      if (image->colorspace != CMYKColorspace)
        ThrowBinaryException(ImageError,"ColorSeparatedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(p->red);
            PopCharPixel(pixel,&q);
            pixel=ScaleQuantumToChar(p->green);
            PopCharPixel(pixel,&q);
            pixel=ScaleQuantumToChar(p->blue);
            PopCharPixel(pixel,&q);
            pixel=ScaleQuantumToChar(indexes[x]);
            PopCharPixel(pixel,&q);
            pixel=ScaleQuantumToChar((Quantum) ((MagickRealType) QuantumRange-
              p->opacity));
            PopCharPixel(pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(p->red);
            PopShortPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToShort(p->green);
            PopShortPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToShort(p->blue);
            PopShortPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToShort(indexes[x]);
            PopShortPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToShort((Quantum) ((MagickRealType) QuantumRange-
              p->opacity));
            PopShortPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                register float
                  pixel;

                pixel=(float) p->red;
                PopFloatPixel(&quantum_state,pixel,&q);
                pixel=(float) p->green;
                PopFloatPixel(&quantum_state,pixel,&q);
                pixel=(float) p->blue;
                PopFloatPixel(&quantum_state,pixel,&q);
                pixel=(float) p->opacity;
                PopFloatPixel(&quantum_state,pixel,&q);
                pixel=(float) ((MagickRealType) QuantumRange-p->opacity);
                PopFloatPixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(p->red);
            PopLongPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToLong(p->green);
            PopLongPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToLong(p->blue);
            PopLongPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToLong(indexes[x]);
            PopLongPixel(&quantum_state,pixel,&q);
            pixel=ScaleQuantumToLong((Quantum) ((MagickRealType) QuantumRange-
              p->opacity));
            PopLongPixel(&quantum_state,pixel,&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
        case 64:
        {
          if (quantum_info->format == FloatingPointQuantumFormat)
            {
              register double
                pixel;

              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=(double) p->red;
                PopDoublePixel(&quantum_state,pixel,&q);
                pixel=(double) p->green;
                PopDoublePixel(&quantum_state,pixel,&q);
                pixel=(double) p->blue;
                PopDoublePixel(&quantum_state,pixel,&q);
                pixel=(double) indexes[x];
                PopDoublePixel(&quantum_state,pixel,&q);
                pixel=(double) ((MagickRealType) QuantumRange-p->opacity);
                PopDoublePixel(&quantum_state,pixel,&q);
                p++;
                q+=quantum_info->pad*sizeof(pixel);
              }
              break;
            }
        }
        default:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->red,image->depth),&q);
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->green,image->depth),&q);
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->blue,image->depth),&q);
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              indexes[x],image->depth),&q);
            PopQuantumPixel(&quantum_state,image->depth,ScaleQuantumToAny(
              p->opacity,image->depth),&q);
            p++;
            q+=quantum_info->pad*sizeof(pixel);
          }
          break;
        }
      }
      break;
    }
    default:
      break;
  }
  return(MagickTrue);
}
