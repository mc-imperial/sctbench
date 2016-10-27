/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%           RRRR    EEEEE   SSSSS   AAA   M   M  PPPP   L      EEEEE          %
%           R   R   E       SS     A   A  MM MM  P   P  L      E              %
%           RRRR    EEE      SSS   AAAAA  M M M  PPPP   L      EEE            %
%           R R     E          SS  A   A  M   M  P      L      E              %
%           R  R    EEEEE   SSSSS  A   A  M   M  P      LLLLL  EEEEE          %
%                                                                             %
%                                                                             %
%                           Pixel Resampling Methods.                         %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                              Anthony Thyssen                                %
%                                August 2007                                  %
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
#include "magick/color-private.h"
#include "magick/cache.h"
#include "magick/draw.h"
#include "magick/exception-private.h"
#include "magick/gem.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/log.h"
#include "magick/memory_.h"
#include "magick/pixel-private.h"
#include "magick/quantum.h"
#include "magick/random_.h"
#include "magick/resample.h"
#include "magick/resize.h"
#include "magick/transform.h"
#include "magick/signature.h"
/*
  Typedef declarations.
*/
#define WLUT_WIDTH 1024
struct _ResampleFilter
{
  Image
    *image;

  ViewInfo
    *view;

  ExceptionInfo
    *exception;

  MagickBooleanType
    debug;

  /* Information about image being resampled */
  long
    image_area;

  InterpolatePixelMethod
    interpolate;

  VirtualPixelMethod
    virtual_pixel;

  FilterTypes
    filter;

  /* processing settings needed */
  MagickBooleanType
    limit_reached,
    do_interpolate,
    average_defined;

  MagickPixelPacket
    average_pixel;

  /* current ellipitical area being resampled around center point */
  double
    A, B, C,
    sqrtA, sqrtC, sqrtU, slope;

  /* LUT of weights for filtered average in elliptical area */
  double
    filter_lut[WLUT_WIDTH];

  unsigned long
    signature;
};

/*
  Forward declarations.
*/
static void
  SetResampleFilter(ResampleFilter *,const FilterTypes,const double);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e R e s a m p l e I n f o                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireResampleFilter() initializes the information resample needs do to a
%  scaled lookup of a color from an image, using area sampling.
%
%  The algorithm is based on a Elliptical Weighted Average, where the pixels
%  found in a large elliptical area is averaged together according to a
%  weighting (filter) function.  For more details see "Fundamentals of Texture
%  Mapping and Image Warping" a master's thesis by Paul.S.Heckbert, June 17,
%  1989.  Available for free from, http://www.cs.cmu.edu/~ph/
%
%  As EWA resampling (or any sort of resampling) can require a lot of
%  calculations to produce a distorted scaling of the source image for each
%  output pixel, the ResampleFilter structure generated holds that information
%  between individual image resampling.
%
%  This function will make the appropriate OpenCacheView() calls
%  to view the image, calling functions do not need to open a cache view.
%
%  Usage Example...
%      resample_filter=AcquireResampleFilter(image,exception);
%      for (y=0; y < (long) image->rows; y++) {
%        for (x=0; x < (long) image->columns; x++) {
%          X= ....;   Y= ....;
%          ScaleResampleFilter(resample_filter, ... scaling vectors ...);
%          pixel=ResamplePixelColor(resample_filter,X,Y);
%          ... assign resampled pixel value ...
%        }
%      }
%      DestroyResampleFilter(resample_filter);
%
%  The format of the AcquireResampleFilter method is:
%
%     ResampleFilter *AcquireResampleFilter(const Image *image,
%       ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport ResampleFilter *AcquireResampleFilter(const Image *image,
  ExceptionInfo *exception)
{
  register ResampleFilter
    *resample_filter;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);

  resample_filter=(ResampleFilter *) AcquireMagickMemory(
    sizeof(*resample_filter));
  if (resample_filter == (ResampleFilter *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  (void) ResetMagickMemory(resample_filter,0,sizeof(*resample_filter));

  resample_filter->image=ReferenceImage((Image *) image);
  resample_filter->view=OpenCacheView(resample_filter->image);
  resample_filter->exception=exception;

  resample_filter->debug=IsEventLogging();
  resample_filter->signature=MagickSignature;

  resample_filter->image_area = (long) resample_filter->image->columns *
    resample_filter->image->rows;
  resample_filter->average_defined = MagickFalse;

  /* initialise the resampling filter settings */
  SetResampleFilter(resample_filter, resample_filter->image->filter,
    resample_filter->image->blur);
  resample_filter->interpolate = resample_filter->image->interpolate;
  resample_filter->virtual_pixel=GetImageVirtualPixelMethod(image);

  /* init scale to a default of a unit circle */
  ScaleResampleFilter(resample_filter, 1.0, 0.0, 0.0, 1.0);

  return(resample_filter);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y R e s a m p l e I n f o                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyResampleFilter() finalizes and cleans up the resampling
%  resample_filter as returned by AcquireResampleFilter(), freeing any memory
%  or other information as needed.
%
%  The format of the DestroyResampleFilter method is:
%
%      ResampleFilter *DestroyResampleFilter(ResampleFilter *resample_filter)
%
%  A description of each parameter follows:
%
%    o resample_filter: resampling resample_filterrmation structure
%
*/
MagickExport ResampleFilter *DestroyResampleFilter(
  ResampleFilter *resample_filter)
{
  assert(resample_filter != (ResampleFilter *) NULL);
  assert(resample_filter->signature == MagickSignature);
  assert(resample_filter->image != (Image *) NULL);
  if (resample_filter->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      resample_filter->image->filename);

  resample_filter->view=CloseCacheView(resample_filter->view);
  resample_filter->image=DestroyImage(resample_filter->image);
  resample_filter->signature=(~MagickSignature);

  resample_filter=(ResampleFilter *) RelinquishMagickMemory(resample_filter);
  return(resample_filter);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n t e r p o l a t e R e s a m p l e F i l t e r                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InterpolateResampleFilter() applies bi-linear or tri-linear interpolation
%  between a floating point coordinate and the pixels surrounding that
%  coordinate.  No pixel area resampling, or scaling of the result is
%  performed.
%
%  The format of the InterpolateResampleFilter method is:
%
%      MagickPixelPacket InterpolateResampleFilter(ResampleInfo *resample_filter,
%        const InterpolatePixelMethod method,const double x,const double y)
%
%  A description of each parameter follows:
%
%    o resample_filter: The resample filter.
%
%    o method: The pixel clor interpolation method.
%
%    o x,y: A double representing the current (x,y) position of the pixel.
%
*/

static inline double MagickMax(const double x,const double y)
{
  if (x > y)
    return(x);
  return(y);
}

static void BicubicInterpolate(const MagickPixelPacket *pixels,const double dx,
  MagickPixelPacket *pixel)
{
  MagickRealType
    dx2,
    p,
    q,
    r,
    s;

  dx2=dx*dx;
  p=(pixels[3].red-pixels[2].red)-(pixels[0].red-pixels[1].red);
  q=(pixels[0].red-pixels[1].red)-p;
  r=pixels[2].red-pixels[0].red;
  s=pixels[1].red;
  pixel->red=(dx*dx2*p)+(dx2*q)+(dx*r)+s;
  p=(pixels[3].green-pixels[2].green)-(pixels[0].green-pixels[1].green);
  q=(pixels[0].green-pixels[1].green)-p;
  r=pixels[2].green-pixels[0].green;
  s=pixels[1].green;
  pixel->green=(dx*dx2*p)+(dx2*q)+(dx*r)+s;
  p=(pixels[3].blue-pixels[2].blue)-(pixels[0].blue-pixels[1].blue);
  q=(pixels[0].blue-pixels[1].blue)-p;
  r=pixels[2].blue-pixels[0].blue;
  s=pixels[1].blue;
  pixel->blue=(dx*dx2*p)+(dx2*q)+(dx*r)+s;
  p=(pixels[3].opacity-pixels[2].opacity)-(pixels[0].opacity-pixels[1].opacity);
  q=(pixels[0].opacity-pixels[1].opacity)-p;
  r=pixels[2].opacity-pixels[0].opacity;
  s=pixels[1].opacity;
  pixel->opacity=(dx*dx2*p)+(dx2*q)+(dx*r)+s;
  if (pixel->colorspace == CMYKColorspace)
    {
      p=(pixels[3].index-pixels[2].index)-(pixels[0].index-pixels[1].index);
      q=(pixels[0].index-pixels[1].index)-p;
      r=pixels[2].index-pixels[0].index;
      s=pixels[1].index;
      pixel->index=(dx*dx2*p)+(dx2*q)+(dx*r)+s;
    }
}

static inline MagickRealType CubicWeightingFunction(const MagickRealType x)
{
  MagickRealType
    alpha,
    gamma;

  alpha=MagickMax(x+2.0,0.0);
  gamma=1.0*alpha*alpha*alpha;
  alpha=MagickMax(x+1.0,0.0);
  gamma-=4.0*alpha*alpha*alpha;
  alpha=MagickMax(x+0.0,0.0);
  gamma+=6.0*alpha*alpha*alpha;
  alpha=MagickMax(x-1.0,0.0);
  gamma-=4.0*alpha*alpha*alpha;
  return(gamma/6.0);
}

static inline double MeshInterpolate(const PointInfo *delta,const double p,
  const double x,const double y)
{
  return(delta->x*x+delta->y*y+(1.0-delta->x-delta->y)*p);
}

static inline long NearestNeighbor(MagickRealType x)
{
  if (x >= 0.0)
    return((long) (x+0.5));
  return((long) (x-0.5));
}

static MagickPixelPacket InterpolateResampleFilter(
  ResampleFilter *resample_filter,const InterpolatePixelMethod method,
  const double x,const double y)
{
  MagickPixelPacket
    pixel;

  register const IndexPacket
    *indexes;

  register const PixelPacket
    *p;

  register long
    i;

  assert(resample_filter != (ResampleFilter *) NULL);
  assert(resample_filter->signature == MagickSignature);
  GetMagickPixelPacket(resample_filter->image,&pixel);
  switch (method)
  {
    case AverageInterpolatePixel:
    {
      MagickPixelPacket
        pixels[16];

      MagickRealType
        alpha[16],
        gamma;

      p=AcquireCacheViewPixels(resample_filter->view,(long) floor(x)-1,(long)
        floor(y)-1,4,4,resample_filter->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=AcquireCacheViewIndexes(resample_filter->view);
      for (i=0; i < 16L; i++)
      {
        GetMagickPixelPacket(resample_filter->image,pixels+i);
        SetMagickPixelPacket(resample_filter->image,p,indexes+i,pixels+i);
        alpha[i]=1.0;
        if (resample_filter->image->matte != MagickFalse)
          {
            alpha[i]=QuantumScale*((MagickRealType) QuantumRange-p->opacity);
            pixels[i].red*=alpha[i];
            pixels[i].green*=alpha[i];
            pixels[i].blue*=alpha[i];
            if (resample_filter->image->colorspace == CMYKColorspace)
              pixels[i].index*=alpha[i];
          }
        gamma=alpha[i];
        gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
        pixel.red+=gamma*0.0625*pixels[i].red;
        pixel.green+=gamma*0.0625*pixels[i].green;
        pixel.blue+=gamma*0.0625*pixels[i].blue;
        pixel.opacity+=0.0625*pixels[i].opacity;
        if (resample_filter->image->colorspace == CMYKColorspace)
          pixel.index+=gamma*0.0625*pixels[i].index;
        p++;
      }
      break;
    }
    case BicubicInterpolatePixel:
    {
      MagickPixelPacket
        pixels[16],
        u[4];

      MagickRealType
        alpha[16];

      PointInfo
        delta;

      p=AcquireCacheViewPixels(resample_filter->view,(long) floor(x)-1,(long)
        floor(y)-1,4,4,resample_filter->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=AcquireCacheViewIndexes(resample_filter->view);
      for (i=0; i < 16L; i++)
      {
        GetMagickPixelPacket(resample_filter->image,pixels+i);
        SetMagickPixelPacket(resample_filter->image,p,indexes+i,pixels+i);
        alpha[i]=1.0;
        if (resample_filter->image->matte != MagickFalse)
          {
            alpha[i]=QuantumScale*((MagickRealType) QuantumRange-p->opacity);
            pixels[i].red*=alpha[i];
            pixels[i].green*=alpha[i];
            pixels[i].blue*=alpha[i];
            if (resample_filter->image->colorspace == CMYKColorspace)
              pixels[i].index*=alpha[i];
          }
        p++;
      }
      delta.x=x-floor(x);
      for (i=0; i < 4L; i++)
        BicubicInterpolate(pixels+4*i,delta.x,u+i);
      delta.y=y-floor(y);
      BicubicInterpolate(u,delta.y,&pixel);
      break;
    }
    case BilinearInterpolatePixel:
    default:
    {
      MagickPixelPacket
        pixels[16];

      MagickRealType
        alpha[16],
        gamma;

      PointInfo
        delta;

      p=AcquireCacheViewPixels(resample_filter->view,(long) floor(x),(long)
        floor(y),2,2,resample_filter->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=AcquireCacheViewIndexes(resample_filter->view);
      for (i=0; i < 4L; i++)
      {
        GetMagickPixelPacket(resample_filter->image,pixels+i);
        SetMagickPixelPacket(resample_filter->image,p,indexes+i,pixels+i);
        alpha[i]=1.0;
        if (resample_filter->image->matte != MagickFalse)
          {
            alpha[i]=QuantumScale*((MagickRealType) QuantumRange-p->opacity);
            pixels[i].red*=alpha[i];
            pixels[i].green*=alpha[i];
            pixels[i].blue*=alpha[i];
            if (resample_filter->image->colorspace == CMYKColorspace)
              pixels[i].index*=alpha[i];
          }
        p++;
      }
      delta.x=x-floor(x);
      delta.y=y-floor(y);
      gamma=(((1.0-delta.y)*((1.0-delta.x)*alpha[0]+delta.x*alpha[1])+delta.y*
        ((1.0-delta.x)*alpha[2]+delta.x*alpha[3])));
      gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
      pixel.red=gamma*((1.0-delta.y)*((1.0-delta.x)*pixels[0].red+delta.x*
        pixels[1].red)+delta.y*((1.0-delta.x)*pixels[2].red+delta.x*
        pixels[3].red));
      pixel.green=gamma*((1.0-delta.y)*((1.0-delta.x)*pixels[0].green+delta.x*
        pixels[1].green)+delta.y*((1.0-delta.x)*pixels[2].green+
        delta.x*pixels[3].green));
      pixel.blue=gamma*((1.0-delta.y)*((1.0-delta.x)*pixels[0].blue+delta.x*
        pixels[1].blue)+delta.y*((1.0-delta.x)*pixels[2].blue+delta.x*
        pixels[3].blue));
      pixel.opacity=((1.0-delta.y)*((1.0-delta.x)*pixels[0].opacity+delta.x*
        pixels[1].opacity)+delta.y*((1.0-delta.x)*pixels[2].opacity+delta.x*
        pixels[3].opacity));
      if (resample_filter->image->colorspace == CMYKColorspace)
        pixel.index=gamma*((1.0-delta.y)*((1.0-delta.x)*pixels[0].index+delta.x*
          pixels[1].index)+delta.y*((1.0-delta.x)*pixels[2].index+delta.x*
          pixels[3].index));
      break;
    }
    case FilterInterpolatePixel:
    {
      Image
        *excert_image,
        *filter_image;

      MagickPixelPacket
        pixels[1];

      RectangleInfo
        geometry;

      geometry.width=4L;
      geometry.height=4L;
      geometry.x=(long) floor(x)-1L;
      geometry.y=(long) floor(y)-1L;
      excert_image=ExcerptImage(resample_filter->image,&geometry,
        resample_filter->exception);
      if (excert_image == (Image *) NULL)
        break;
      filter_image=ResizeImage(excert_image,1,1,resample_filter->image->filter,
        resample_filter->image->blur,resample_filter->exception);
      excert_image=DestroyImage(excert_image);
      if (filter_image == (Image *) NULL)
        break;
      p=AcquireImagePixels(filter_image,0,0,1,1,resample_filter->exception);
      if (p == (const PixelPacket *) NULL)
        {
          filter_image=DestroyImage(filter_image);
          break;
        }
      indexes=AcquireIndexes(filter_image);
      GetMagickPixelPacket(resample_filter->image,pixels);
      SetMagickPixelPacket(resample_filter->image,p,indexes,&pixel);
      filter_image=DestroyImage(filter_image);
      break;
    }
    case IntegerInterpolatePixel:
    {
      MagickPixelPacket
        pixels[1];

      p=AcquireCacheViewPixels(resample_filter->view,(long) floor(x),(long)
        floor(y),1,1,resample_filter->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=AcquireCacheViewIndexes(resample_filter->view);
      GetMagickPixelPacket(resample_filter->image,pixels);
      SetMagickPixelPacket(resample_filter->image,p,indexes,&pixel);
      break;
    }
    case MeshInterpolatePixel:
    {
      MagickPixelPacket
        pixels[4];

      MagickRealType
        alpha[4],
        gamma;

      PointInfo
        delta,
        luminance;

      p=AcquireCacheViewPixels(resample_filter->view,(long) floor(x),(long)
        floor(y),2,2,resample_filter->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=AcquireCacheViewIndexes(resample_filter->view);
      for (i=0; i < 4L; i++)
      {
        GetMagickPixelPacket(resample_filter->image,pixels+i);
        SetMagickPixelPacket(resample_filter->image,p,indexes+i,pixels+i);
        alpha[i]=1.0;
        if (resample_filter->image->matte != MagickFalse)
          {
            alpha[i]=QuantumScale*((MagickRealType) QuantumRange-p->opacity);
            pixels[i].red*=alpha[i];
            pixels[i].green*=alpha[i];
            pixels[i].blue*=alpha[i];
            if (resample_filter->image->colorspace == CMYKColorspace)
              pixels[i].index*=alpha[i];
          }
        p++;
      }
      delta.x=x-floor(x);
      delta.y=y-floor(y);
      luminance.x=MagickPixelLuminance(pixels+0)-MagickPixelLuminance(pixels+3);
      luminance.y=MagickPixelLuminance(pixels+1)-MagickPixelLuminance(pixels+2);
      if (fabs(luminance.x) < fabs(luminance.y))
        {
          /*
            Diagonal 0-3 NW-SE.
          */
          if (delta.x <= delta.y)
            {
              /*
                Bottom-left triangle  (pixel:2, diagonal: 0-3).
              */
              delta.y=1.0-delta.y;
              gamma=MeshInterpolate(&delta,alpha[2],alpha[3],alpha[0]);
              gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
              pixel.red=gamma*MeshInterpolate(&delta,pixels[2].red,
                pixels[3].red,pixels[0].red);
              pixel.green=gamma*MeshInterpolate(&delta,pixels[2].green,
                pixels[3].green,pixels[0].green);
              pixel.blue=gamma*MeshInterpolate(&delta,pixels[2].blue,
                pixels[3].blue,pixels[0].blue);
              pixel.opacity=gamma*MeshInterpolate(&delta,pixels[2].opacity,
                pixels[3].opacity,pixels[0].opacity);
              if (resample_filter->image->colorspace == CMYKColorspace)
                pixel.index=gamma*MeshInterpolate(&delta,pixels[2].index,
                  pixels[3].index,pixels[0].index);
            }
          else
            {
              /*
                Top-right triangle (pixel:1, diagonal: 0-3).
              */
              delta.x=1.0-delta.x;
              gamma=MeshInterpolate(&delta,alpha[1],alpha[0],alpha[3]);
              gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
              pixel.red=gamma*MeshInterpolate(&delta,pixels[1].red,
                pixels[0].red,pixels[3].red);
              pixel.green=gamma*MeshInterpolate(&delta,pixels[1].green,
                pixels[0].green,pixels[3].green);
              pixel.blue=gamma*MeshInterpolate(&delta,pixels[1].blue,
                pixels[0].blue,pixels[3].blue);
              pixel.opacity=gamma*MeshInterpolate(&delta,pixels[1].opacity,
                pixels[0].opacity,pixels[3].opacity);
              if (resample_filter->image->colorspace == CMYKColorspace)
                pixel.index=gamma*MeshInterpolate(&delta,pixels[1].index,
                  pixels[0].index,pixels[3].index);
            }
        }
      else
        {
          /*
            Diagonal 1-2 NE-SW.
          */
          if (delta.x <= (1.0-delta.y))
            {
              /*
                Top-left triangle (pixel 0, diagonal: 1-2).
              */
              gamma=MeshInterpolate(&delta,alpha[0],alpha[1],alpha[2]);
              gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
              pixel.red=gamma*MeshInterpolate(&delta,pixels[0].red,
                pixels[1].red,pixels[2].red);
              pixel.green=gamma*MeshInterpolate(&delta,pixels[0].green,
                pixels[1].green,pixels[2].green);
              pixel.blue=gamma*MeshInterpolate(&delta,pixels[0].blue,
                pixels[1].blue,pixels[2].blue);
              pixel.opacity=gamma*MeshInterpolate(&delta,pixels[0].opacity,
                pixels[1].opacity,pixels[2].opacity);
              if (resample_filter->image->colorspace == CMYKColorspace)
                pixel.index=gamma*MeshInterpolate(&delta,pixels[0].index,
                  pixels[1].index,pixels[2].index);
            }
          else
            {
              /*
                Bottom-right triangle (pixel: 3, diagonal: 1-2).
              */
              delta.x=1.0-delta.x;
              delta.y=1.0-delta.y;
              gamma=MeshInterpolate(&delta,alpha[3],alpha[2],alpha[1]);
              gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
              pixel.red=gamma*MeshInterpolate(&delta,pixels[3].red,
                pixels[2].red,pixels[1].red);
              pixel.green=gamma*MeshInterpolate(&delta,pixels[3].green,
                pixels[2].green,pixels[1].green);
              pixel.blue=gamma*MeshInterpolate(&delta,pixels[3].blue,
                pixels[2].blue,pixels[1].blue);
              pixel.opacity=gamma*MeshInterpolate(&delta,pixels[3].opacity,
                pixels[2].opacity,pixels[1].opacity);
              if (resample_filter->image->colorspace == CMYKColorspace)
                pixel.index=gamma*MeshInterpolate(&delta,pixels[3].index,
                  pixels[2].index,pixels[1].index);
            }
        }
      break;
    }
    case NearestNeighborInterpolatePixel:
    {
      MagickPixelPacket
        pixels[1];

      p=AcquireCacheViewPixels(resample_filter->view,NearestNeighbor(x),
        NearestNeighbor(y),1,1,resample_filter->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=AcquireCacheViewIndexes(resample_filter->view);
      GetMagickPixelPacket(resample_filter->image,pixels);
      SetMagickPixelPacket(resample_filter->image,p,indexes,&pixel);
      break;
    }
    case SplineInterpolatePixel:
    {
      long
        j,
        n;

      MagickPixelPacket
        pixels[16];

      MagickRealType
        alpha[16],
        dx,
        dy,
        gamma;

      PointInfo
        delta;

      p=AcquireCacheViewPixels(resample_filter->view,(long) floor(x)-1,(long)
        floor(y)-1,4,4,resample_filter->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=AcquireCacheViewIndexes(resample_filter->view);
      n=0;
      delta.x=x-floor(x);
      delta.y=y-floor(y);
      for (i=(-1); i < 3L; i++)
      {
        dy=CubicWeightingFunction((MagickRealType) i-delta.y);
        for (j=(-1); j < 3L; j++)
        {
          GetMagickPixelPacket(resample_filter->image,pixels+n);
          SetMagickPixelPacket(resample_filter->image,p,indexes+n,pixels+n);
          alpha[n]=1.0;
          if (resample_filter->image->matte != MagickFalse)
            {
              alpha[n]=QuantumScale*((MagickRealType) QuantumRange-p->opacity);
              pixels[n].red*=alpha[n];
              pixels[n].green*=alpha[n];
              pixels[n].blue*=alpha[n];
              if (resample_filter->image->colorspace == CMYKColorspace)
                pixels[n].index*=alpha[n];
            }
          dx=CubicWeightingFunction(delta.x-(MagickRealType) j);
          gamma=alpha[n];
          gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
          pixel.red+=gamma*dx*dy*pixels[n].red;
          pixel.green+=gamma*dx*dy*pixels[n].green;
          pixel.blue+=gamma*dx*dy*pixels[n].blue;
          if (resample_filter->image->matte != MagickFalse)
            pixel.opacity+=dx*dy*pixels[n].opacity;
          if (resample_filter->image->colorspace == CMYKColorspace)
            pixel.index+=gamma*dx*dy*pixels[n].index;
          n++;
          p++;
        }
      }
      break;
    }
  }
  return(pixel);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s a m p l e P i x e l C o l o r                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResamplePixelColor() samples the pixel values surrounding the location
%  given using a Elliptical Weighted Average, at the scale previously
%  calculated, and in the most efficent manner posible for the
%  VirtualPixelMethod also previously set.
%
%  The format of the ResamplePixelColor method is:
%
%     MagickPixelPacket ResamplePixelColor(ResampleFilter *resample_filter,
%       const double u0,const double v0 )
%
%  A description of each parameter follows:
%
%    o resample_filter: The Resampling Information Structure
%
%    o u0,v0: A double representing the center of the area to resample,
%             The distortion transformed transformed x,y coordinate.
%
*/
MagickExport MagickPixelPacket ResamplePixelColor(
  ResampleFilter *resample_filter,const double u0,const double v0)
{
  MagickPixelPacket pixel;
  long u,v, uw,v1,v2, hit;
  double u1;
  double U,V,Q,DQ,DDQ;
  double divisor;
  register double weight;
  register PixelPacket *pixels;
  register IndexPacket *indexes;

  assert(resample_filter != (ResampleFilter *) NULL);
  assert(resample_filter->signature == MagickSignature);

  GetMagickPixelPacket(resample_filter->image,&pixel);

  if ( resample_filter->do_interpolate ) {
    pixel = InterpolateResampleFilter(resample_filter,resample_filter->interpolate,
      u0,v0);
    return(pixel);
  }

  /*
    Does resample area Miss the image?
    And area is in virtual areas of solid color - return that color
  */
  hit = 0;
  switch ( resample_filter->virtual_pixel ) {
    case BackgroundVirtualPixelMethod:
    case ConstantVirtualPixelMethod:
    case TransparentVirtualPixelMethod:
    case BlackVirtualPixelMethod:
    case GrayVirtualPixelMethod:
    case WhiteVirtualPixelMethod:
    case MaskVirtualPixelMethod:
      if ( resample_filter->limit_reached
           || u0 + resample_filter->sqrtC < 0.0
           || u0 - resample_filter->sqrtC > (double) resample_filter->image->columns
           || v0 + resample_filter->sqrtA < 0.0
           || v0 - resample_filter->sqrtA > (double) resample_filter->image->rows
           )
        hit++;
      break;

    case UndefinedVirtualPixelMethod:
    case EdgeVirtualPixelMethod:
      if (    ( u0 + resample_filter->sqrtC < 0.0 && v0 + resample_filter->sqrtA < 0.0 )
           || ( u0 + resample_filter->sqrtC < 0.0
                && v0 - resample_filter->sqrtA > (double) resample_filter->image->rows )
           || ( u0 - resample_filter->sqrtC > (double) resample_filter->image->columns
                && v0 + resample_filter->sqrtA < 0.0 )
           || ( u0 - resample_filter->sqrtC > (double) resample_filter->image->columns
                && v0 - resample_filter->sqrtA > (double) resample_filter->image->rows )
           )
        hit++;
      break;

    case DitherVirtualPixelMethod:
      if (    ( u0 + resample_filter->sqrtC < -32.0 && v0 + resample_filter->sqrtA < -32.0 )
           || ( u0 + resample_filter->sqrtC < -32.0
                && v0 - resample_filter->sqrtA > (double) resample_filter->image->rows+32.0 )
           || ( u0 - resample_filter->sqrtC > (double) resample_filter->image->columns+32.0
                && v0 + resample_filter->sqrtA < -32.0 )
           || ( u0 - resample_filter->sqrtC > (double) resample_filter->image->columns+32.0
                && v0 - resample_filter->sqrtA > (double) resample_filter->image->rows+32.0 )
           )
        hit++;
      break;
    case TileVirtualPixelMethod:
    case MirrorVirtualPixelMethod:
    case RandomVirtualPixelMethod:
      /* resampling of area always needed - no VP limits */
      break;
  }
  if ( hit ) {
    /* whole area is a solid color -- just return that color */
    pixel = InterpolateResampleFilter(resample_filter,IntegerInterpolatePixel,u0,v0);
    return(pixel);
  }

  /*
    Scaling limits reached, return 'averaged' result.
  */
  if ( resample_filter->limit_reached ) {
    switch ( resample_filter->virtual_pixel ) {
      /*  This has already been handled above
        case BackgroundVirtualPixelMethod:
        case ConstantVirtualPixelMethod:
        case TransparentVirtualPixelMethod:
        case GrayVirtualPixelMethod,
        case WhiteVirtualPixelMethod
        case MaskVirtualPixelMethod:
        case MaskVirtualPixelMethod:
          pixel = InterpolateResampleFilter(resample_filter->image,resample_filter->view,
                IntegerInterpolatePixel, u0,v0, resample_filter->exception);
          break;
      */
      case UndefinedVirtualPixelMethod:
      case EdgeVirtualPixelMethod:
      case DitherVirtualPixelMethod:
        /* We need an average edge pixel!
           How should I calculate an average edge color?
           Just return an averaged neighbourhood, seems to work well */
        pixel = InterpolateResampleFilter(resample_filter,AverageInterpolatePixel,
          u0,v0);
        break;
      case TileVirtualPixelMethod:
      case MirrorVirtualPixelMethod:
      case RandomVirtualPixelMethod:
      default:
        if ( resample_filter->average_defined == MagickFalse ) {
          Image
            *average_image;

          GetMagickPixelPacket(resample_filter->image,
                (MagickPixelPacket *)&(resample_filter->average_pixel));
          resample_filter->average_defined = MagickTrue;

          /* Try to get an averaged pixel color of whole image */
          average_image=ResizeImage(resample_filter->image,1,1,BoxFilter,1.0,
           resample_filter->exception);
          if (average_image == (Image *) NULL)
            return(resample_filter->average_pixel);
          pixels=(PixelPacket *)AcquireImagePixels(average_image,0,0,1,1,
            resample_filter->exception);
          if (pixels == (const PixelPacket *) NULL) {
            average_image=DestroyImage(average_image);
            return(resample_filter->average_pixel);
          }
          indexes=(IndexPacket *) GetIndexes(average_image);
          SetMagickPixelPacket(resample_filter->image,pixels,indexes,
            &(resample_filter->average_pixel));
          average_image=DestroyImage(average_image);
        }
        pixel = resample_filter->average_pixel;
        break;
    }
    return(pixel);
  }

  /*
    Initialize weighted average data collection
  */
  hit = 0;
  divisor = 0.0;
  pixel.red = pixel.green = pixel.blue = 0.0;
  if (resample_filter->image->matte != MagickFalse) pixel.opacity = 0.0;
  if (resample_filter->image->colorspace == CMYKColorspace) pixel.index = 0.0;

  /*
    Determine the parellelogram bounding box fitted to the ellipse
    centered at u0,v0.  This area is bounding by the lines...
        v = +/- sqrt(A)
        u = -By/2A  +/- sqrt(F/A)
    Which has been pre-calculated above.
  */
  v1 = (long)(v0 - resample_filter->sqrtA);               /* range of scan lines */
  v2 = (long)(v0 + resample_filter->sqrtA + 1);

  u1 = u0 + (v1-v0)*resample_filter->slope - resample_filter->sqrtU; /* start of scanline for v=v1 */
  uw = (long)(2*resample_filter->sqrtU)+1;       /* width of parallelogram */

  /*
    Do weighted resampling of all pixels,  within the scaled ellipse,
    bound by a Parellelogram fitted to the ellipse.
  */
  DDQ = 2*resample_filter->A;
  for( v=v1; v<=v2;  v++, u1+=resample_filter->slope ) {
    u = (long)u1;       /* first pixel in scanline  ( floor(u1) ) */
    U = (double)u-u0;   /* location of that pixel, relative to u0,v0 */
    V = (double)v-v0;

    /* Q = ellipse quotent ( if Q<F then pixel is inside ellipse) */
    Q = U*(resample_filter->A*U + resample_filter->B*V) + resample_filter->C*V*V;
    DQ = resample_filter->A*(2.0*U+1) + resample_filter->B*V;

    /* get the scanline of pixels for this v */
    pixels=(PixelPacket *)AcquireCacheViewPixels(resample_filter->view,u,v,1UL*uw,1,
         resample_filter->exception);
    if (pixels == (const PixelPacket *) NULL)
      return(pixel);
    indexes=(IndexPacket *) AcquireCacheViewIndexes(resample_filter->view);

    /* count up the weighted pixel colors */
    for( u=0; u<uw; u++ ) {
      /* Note that the ellipse has been pre-scaled so F = WLUT_WIDTH */
      if ( Q < (double)WLUT_WIDTH ) {
        weight = resample_filter->filter_lut[(int)Q];
        if (resample_filter->image->matte != MagickFalse) {
          pixel.opacity  += weight*pixels->opacity;
          weight *= QuantumScale*((MagickRealType)
                      (QuantumRange - pixels->opacity));
        }
        pixel.red   += weight*pixels->red;
        pixel.green += weight*pixels->green;
        pixel.blue  += weight*pixels->blue;
        if (resample_filter->image->colorspace == CMYKColorspace)
          pixel.index += weight*(*indexes);

        divisor += weight;
        hit++;
      }
      pixels++;
      indexes++;
      Q += DQ;
      DQ += DDQ;
    }
  }

  /*
    Result sanity check -- this should NOT happen
  */
  if ( hit < 4 ) {
    /* not enough pixels in resampling, resort to direct interpolation */
    pixel = InterpolateResampleFilter(resample_filter,resample_filter->interpolate,
      u0,v0);
    return pixel;
  }

  /*
    Finialize results of resampling
  */
  divisor = 1.0/divisor;
  pixel.red   = (MagickRealType) RoundToQuantum(divisor*pixel.red);
  pixel.green = (MagickRealType) RoundToQuantum(divisor*pixel.green);
  pixel.blue  = (MagickRealType) RoundToQuantum(divisor*pixel.blue);
  if (resample_filter->image->matte != MagickFalse)
    pixel.opacity = (MagickRealType) RoundToQuantum(divisor*pixel.opacity);
  if (resample_filter->image->colorspace == CMYKColorspace)
    pixel.index = (MagickRealType) RoundToQuantum(divisor*pixel.index);
  return pixel;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S c a l e R e s a m p l e F i l t e r                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ScaleResampleFilter() does all the calculations needed to resample an image
%  at a specific scale, defined by two scaling vectors.  This is not using
%  a orthogonal scaling, but two distorted scaling vectors.
%
%  The input vectors are produced by either finding the derivitives of
%  the distortion function, or the partial derivitives from a distortion
%  mapping.
%
%  If   u,v =  DistortEquation(x,y)
%  Then the scaling vectors are the function derivitives...
%      du/dx, du/dy     and    dv/dx, dv/dy
%  If the scaling is only othogonally aligned then...
%      dv/dx =  du/dy  =  0
%  Producing a othogonally scaled circle for the area to be resampled.
%
%  The format of the ScaleResampleFilter method is:
%
%     void ScaleResampleFilter(const ResampleFilter *resample_filter,
%       const double dux,const double duy,const double dvx,const double dvy)
%
%  A description of each parameter follows:
%
%    o resample_filter: The resampling resample_filterrmation defining the
%      image being resampled
%
%    o dux,duy,dvx,dvy:
%         The partial derivitives or scaling vectors for resampling
%         for the X direction:  du/dx, dv/dx
%         and the Y direction:  du/dy, dv/dy
%         This defines the size and angle of the elliptical resampling area
%         The order is the the order you generally would figure out the
%         deritives from the distortion equations.
%
*/
MagickExport void ScaleResampleFilter(ResampleFilter *resample_filter,
  const double dux,const double duy,const double dvx,const double dvy)
{
  double A,B,C,F, area;

  assert(resample_filter != (ResampleFilter *) NULL);
  assert(resample_filter->signature == MagickSignature);

  resample_filter->limit_reached = MagickFalse;
  resample_filter->do_interpolate = MagickFalse;

//printf("dux = %.5lf;  dvx = %.5lf;    duy = %.5lf;  dvy = %.5lf\n",dux,dvx,duy,dvy);

  /* A 'point' filter forces use of interpolation instead of area sampling */
  if ( resample_filter->filter == PointFilter ) {
    resample_filter->do_interpolate = MagickTrue;
    return;
  }

  /* Find Ellipse Coefficents such that
        A*u^2 + B*u*v + C*v^2 = F
     With u,v relative to point around which we are resampling.
  */
#if 0
  /* Direct conversions of derivatives to elliptical coefficients
     No scaling will result in F == 1.0 and a unit circle.
  */
  A = dvx*dvx+dvy*dvy;
  B = (dux*dvx+duy*dvy)*-2.0;
  C = dux*dux+duy*duy;
  F = dux*dvy+duy*dvx;
  F *= F;
#define F_UNITY 1.0
#else
  /* This is Paul Heckbert's recomended "Higher Quality EWA" formula, from page
     60 in his thesis, which adds a unit circle to the elliptical area so are
     to do both Reconstruction and Prefiltering of the pixels in the
     resampling.  It also means it is likely to have at least 4 pixels within
     the area of the ellipse, for weighted averaging.
     No scaling will result in F == 4.0 and a circle of radius 2.0
  */
  A = dvx*dvx+dvy*dvy+1;
  B = (dux*dvx+duy*dvy)*-2.0;
  C = dux*dux+duy*duy+1;
  F = A*C - B*B/4;
#define F_UNITY 4.0
#endif
#if 0
   printf ("F = %lf\n", F);
#endif

  /* Is default elliptical area, too small? Image being magnified?
     Switch to doing pure 'point' interpolation of the pixel.
     That is turn off  EWA Resampling.
  */
  if ( F <= F_UNITY ) {
    resample_filter->do_interpolate = MagickTrue;
    return;
  }


  /* If F is impossibly large, we may as well not bother doing any
   * form of resampling, as you risk an infinite resampled area.
  */
  if ( F > MagickHuge ) {
    resample_filter->limit_reached = MagickTrue;
    return;
  }

#if 0
  /* Figure out the Ellipses Major and Minor Axis, and other info.
     This information currently not needed at this time, but may be
     needed later for better limit determination.
  */
  { double alpha, beta, gamma, Major, Minor;
    alpha = A+C;
    beta  = A-C;
    gamma = sqrt(beta*beta + B*B );

    if ( alpha - gamma <= MagickEpsilon )
      Major = MagickHuge;
    else
      Major = sqrt(2*F/(alpha - gamma));
    Minor = sqrt(2*F/(alpha + gamma));

    /* other information about ellipse include...
    Eccentricity = Major/Minor;
    Ellipse_Area = MagickPI*Major*Minor
    ellipse_angle =  arctan2(B, A-C);

    This is actually used as part of parallelogram bounds
    max_horizontal_cross_section =  2*sqrt(F/A) = sqrt(C-B*B/A);
    max_vertical_cross_section   =  2*sqrt(F/C) = sqrt(A-B*B/C);
    */
  }
#endif

  /* Othogonal bounds of the ellipse */
  resample_filter->sqrtA = sqrt(A)+1.0;     /* Vertical Orthogonal Limit */
  resample_filter->sqrtC = sqrt(C)+1.0;     /* Horizontal Orthogonal Limit */

  /* Horizontally aligned Parallelogram fitted to ellipse */
  resample_filter->sqrtU = sqrt(F/A)+1.0;   /* Parallelogram Width */
  resample_filter->slope = -B/(2*A);        /* Slope of the parallelogram */

  /* The size of the area of the parallelogram we will be sampling */
  area = 4 * resample_filter->sqrtA * resample_filter->sqrtU;

  /* Absolute limit on the area to be resampled
   * This limit needs more work, as it gets too slow for
   * larger images involved with tiled views of the horizon. */
  if ( area > 20.0*resample_filter->image_area ) {
    resample_filter->limit_reached = MagickTrue;
    return;
  }

  /* Scale ellipse formula to directly fit the Filter Lookup Table */
  { register double scale;
    scale = (double)WLUT_WIDTH/F;
    resample_filter->A = A*scale;
    resample_filter->B = B*scale;
    resample_filter->C = C*scale;
    /* ..ple_filter->F = WLUT_WIDTH; -- hardcoded */
  }

  return;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S e t R e s a m p l e F i l t e r                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetResampleFilter() set the filter lookup table based on a specific
%  filter.
%
%  The default Filter, is Gaussian, which is the standard filter used by the
%  original paper on the Elliptical Weighted Everage Algorithm. However other
%  filters can also be used.
%
%  The format of the SetResampleFilter method is:
%
%    void SetResampleFilter(ResampleFilter *resample_filter,
%      const FilterTypes filter,const double blur)
%
%  A description of each parameter follows:
%
%    o resample_filter: resampling resample_filterrmation structure
%
%    o filter: the resize filter for elliptical weighting LUT
%
%    o blur: filter blur factor for elliptical weighting LUT
%
*/
static void SetResampleFilter(ResampleFilter *resample_filter,
  const FilterTypes filter,const double blur)
{
  register int
     Q;

  double
     support_factor;

  /* Remember: During weighted lookup the index Q is the elliptical
     quotent scaled by WLUT_WIDTH.   IE: when Q=1024 at the ellipse edge
     where ellipse radius is about 1.0

     Future: Use the resize filters in "resize.c" to set the lookup table.
     This may require some scaling such as   x = sqrt(Q*PI/2048)
     Note  PI/2  is about 1.7 which was experimentally confirmed.
  */
  assert(resample_filter != (ResampleFilter *) NULL);
  assert(resample_filter->signature == MagickSignature);

  resample_filter->filter = filter;

  support_factor = sqrt(1.0/(double)WLUT_WIDTH)/blur;

  switch( filter ) {
    case PointFilter:
      /* This is equivelent to turning off the EWA algroithm.
         Only Interpolated lookup will be used.  */
      break;
    case BoxFilter:
      for(Q=0; Q<WLUT_WIDTH; Q++)
        resample_filter->filter_lut[Q] = 1.0*Q<WLUT_WIDTH*blur*blur ? 1.0 : 0.0;
      break;
    case TriangleFilter:
      for(Q=0; Q<WLUT_WIDTH; Q++) {
        double r = sqrt((double)Q)*support_factor;
        resample_filter->filter_lut[Q] = r < 1.0 ? 1.0-r : 0.0;
      }
      break;
    case QuadraticFilter:
      /* Testing if a 'resize' function could be used */
      for(Q=0; Q<WLUT_WIDTH; Q++) {
        double r = sqrt((double)Q)*support_factor;
        resample_filter->filter_lut[Q] = (r < .5 ) ? 0.75-r*r : 0.5*(r-1.5)*(r-1.5);
      }
      break;
    case SincFilter:
      /* A windowed Sinc filter scaled to fit ellipse */
      support_factor *= MagickPI;
      resample_filter->filter_lut[0] = 1.0;
      for(Q=1; Q<WLUT_WIDTH; Q++) {
        double r = sqrt((double)Q)*support_factor;
        resample_filter->filter_lut[Q] = sin(r)/r;
      }
      break;
    case GaussianFilter:
    case UndefinedFilter:
    default:
      /*
        Create Normal Gaussian Filter Weighted Lookup Table.
        A normal elliptical lookup would use...
            ALPHA = -4.0*ln(2.0)  ==>  -2.77258872223978123767
        or ALPHA/1024            ==>  -2.70760617406228636491E-3
      */
      support_factor = -2.70760617406228636491E-3/blur/blur;
      for(Q=0; Q<WLUT_WIDTH; Q++)
        resample_filter->filter_lut[Q] = exp((double)Q*support_factor);
      break;
  }
#if 0
  /* Debug output of the filter weighting LUT
     Gnuplot the LUT with hoizontal adjusted to 'r' using...
       plot [0:1][0:1] "lut.dat" using (sqrt($0/1024)):1 with lines
  */
  for(Q=0; Q<WLUT_WIDTH; Q++)
    printf("%lf\n", resample_filter->filter_lut[Q]);
  exit(0);
#endif
  return;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t R e s a m p l e F i l t e r V i r t u a l P i x e l M e t h o d     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetResampleFilterVirtualPixelMethod() changes the virtual pixel method
%  associated with the specified resample filter.
%
%  The format of the SetResampleFilterVirtualPixelMethod method is:
%
%      MagickBooleanType SetResampleFilterVirtualPixelMethod(
%        ResampleFilter *resample_filter,
%        const VirtualPixelMethod virtual_pixel_method)
%
%  A description of each parameter follows:
%
%    o resample_filter: The resample filter.
%
%    o virtual_pixel_method: The virtual pixel method.
%
*/
MagickExport MagickBooleanType SetResampleFilterVirtualPixelMethod(
  ResampleFilter *resample_filter,const VirtualPixelMethod virtual_pixel_method)
{
  assert(resample_filter != (ResampleFilter *) NULL);
  assert(resample_filter->signature == MagickSignature);
  assert(resample_filter->image != (Image *) NULL);
  if (resample_filter->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      resample_filter->image->filename);
  resample_filter->virtual_pixel=virtual_pixel_method;
  (void) SetCacheViewVirtualPixelMethod(resample_filter->view,
    virtual_pixel_method);
  return(MagickTrue);
}
