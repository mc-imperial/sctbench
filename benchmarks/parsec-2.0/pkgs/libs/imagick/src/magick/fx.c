/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                                 FFFFF  X   X                                %
%                                 F       X X                                 %
%                                 FFF      X                                  %
%                                 F       X X                                 %
%                                 F      X   X                                %
%                                                                             %
%                                                                             %
%                  ImageMagick Image Special Effects Methods                  %
%                                                                             %
%                               Software Design                               %
%                                 John Cristy                                 %
%                                 October 1996                                %
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
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/annotate.h"
#include "magick/cache.h"
#include "magick/cache-view.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/composite.h"
#include "magick/decorate.h"
#include "magick/draw.h"
#include "magick/effect.h"
#include "magick/enhance.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/fx.h"
#include "magick/fx-private.h"
#include "magick/gem.h"
#include "magick/geometry.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/option.h"
#include "magick/pixel-private.h"
#include "magick/property.h"
#include "magick/quantum.h"
#include "magick/random_.h"
#include "magick/resample.h"
#include "magick/resize.h"
#include "magick/shear.h"
#include "magick/splay-tree.h"
#include "magick/statistic.h"
#include "magick/string_.h"
#include "magick/transform.h"
#include "magick/utility.h"

/*
  Define declarations.
*/
#define LeftShiftOperator 0xf5
#define RightShiftOperator 0xf6
#define LessThanEqualOperator 0xf7
#define GreaterThanEqualOperator 0xf8
#define EqualOperator 0xf9
#define NotEqualOperator 0xfa
#define LogicalAndOperator 0xfb
#define LogicalOrOperator 0xfc

struct _FxInfo
{
  const Image
    *images;

  MagickBooleanType
    matte;

  char
    *expression;

  SplayTreeInfo
    *colors,
    *symbols;

  ResampleFilter
    **resample_filter;

  ExceptionInfo
    *exception;
};

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A c q u i r e F x I n f o                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireFxInfo() allocates the FxInfo structure.
%
%  The format of the AcquireFxInfo method is:
%
%      FxInfo *AcquireFxInfo(Image *image,const char *expression)
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o expression: The expression.
%
*/
MagickExport FxInfo *AcquireFxInfo(const Image *image,const char *expression)
{
  char
    fx_op[2];

  FxInfo
    *fx_info;

  register long
    i;

  fx_info=(FxInfo *) AcquireMagickMemory(sizeof(*fx_info));
  if (fx_info == (FxInfo *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  (void) ResetMagickMemory(fx_info,0,sizeof(*fx_info));
  fx_info->exception=AcquireExceptionInfo();
  fx_info->images=image;
  fx_info->matte=image->matte;
  fx_info->colors=NewSplayTree(CompareSplayTreeString,RelinquishMagickMemory,
    RelinquishMagickMemory);
  fx_info->symbols=NewSplayTree(CompareSplayTreeString,RelinquishMagickMemory,
    RelinquishMagickMemory);
  fx_info->resample_filter=(ResampleFilter **) AcquireQuantumMemory(
    GetImageListLength(fx_info->images),sizeof(*fx_info->resample_filter));
  if (fx_info->resample_filter == (ResampleFilter **) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  for (i=0; i < (long) GetImageListLength(fx_info->images); i++)
    fx_info->resample_filter[i]=AcquireResampleFilter(GetImageFromList(
      fx_info->images,i),fx_info->exception);
  if (*expression != '@')
    fx_info->expression=AcquireString(expression);
  else
    fx_info->expression=FileToString(expression+1,~0,fx_info->exception);
  (void) SubstituteString(&fx_info->expression," ","");
  fx_op[1]='\0';
  *fx_op=(char) LeftShiftOperator;
  (void) SubstituteString(&fx_info->expression,"<<",fx_op);
  *fx_op=(char) RightShiftOperator;
  (void) SubstituteString(&fx_info->expression,">>",fx_op);
  *fx_op=(char) LessThanEqualOperator;
  (void) SubstituteString(&fx_info->expression,"<=",fx_op);
  *fx_op=(char) GreaterThanEqualOperator;
  (void) SubstituteString(&fx_info->expression,">=",fx_op);
  *fx_op=(char) EqualOperator;
  (void) SubstituteString(&fx_info->expression,"==",fx_op);
  *fx_op=(char) NotEqualOperator;
  (void) SubstituteString(&fx_info->expression,"!=",fx_op);
  *fx_op=(char) LogicalAndOperator;
  (void) SubstituteString(&fx_info->expression,"&&",fx_op);
  *fx_op=(char) LogicalOrOperator;
  (void) SubstituteString(&fx_info->expression,"||",fx_op);
  return(fx_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C h a r c o a l I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CharcoalImage() creates a new image that is a copy of an existing one with
%  the edge highlighted.  It allocates the memory necessary for the new Image
%  structure and returns a pointer to the new image.
%
%  The format of the CharcoalImage method is:
%
%      Image *CharcoalImage(const Image *image,const double radius,
%        const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o radius: the radius of the pixel neighborhood.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *CharcoalImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  Image
    *charcoal_image,
    *clone_image,
    *edge_image;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  clone_image=CloneImage(image,0,0,MagickTrue,exception);
  if (clone_image == (Image *) NULL)
    return((Image *) NULL);
  (void) SetImageType(clone_image,GrayscaleType);
  edge_image=EdgeImage(clone_image,radius,exception);
  clone_image=DestroyImage(clone_image);
  if (edge_image == (Image *) NULL)
    return((Image *) NULL);
  charcoal_image=BlurImage(edge_image,radius,sigma,exception);
  edge_image=DestroyImage(edge_image);
  if (charcoal_image == (Image *) NULL)
    return((Image *) NULL);
  (void) NormalizeImage(charcoal_image);
  (void) NegateImage(charcoal_image,MagickFalse);
  (void) SetImageType(charcoal_image,GrayscaleType);
  return(charcoal_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o l o r i z e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ColorizeImage() blends the fill color with each pixel in the image.
%  A percentage blend is specified with opacity.  Control the application
%  of different color components by specifying a different percentage for
%  each component (e.g. 90/100/10 is 90% red, 100% green, and 10% blue).
%
%  The format of the ColorizeImage method is:
%
%      Image *ColorizeImage(const Image *image,const char *opacity,
%        const PixelPacket colorize,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o opacity:  A character string indicating the level of opacity as a
%      percentage.
%
%    o colorize: A color value.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *ColorizeImage(const Image *image,const char *opacity,
  const PixelPacket colorize,ExceptionInfo *exception)
{
#define ColorizeImageTag  "Colorize/Image"

  GeometryInfo
    geometry_info;

  Image
    *colorize_image;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MagickStatusType
    flags;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Allocate colorized image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  colorize_image=CloneImage(image,0,0,MagickTrue,exception);
  if (colorize_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(colorize_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&colorize_image->exception);
      colorize_image=DestroyImage(colorize_image);
      return((Image *) NULL);
    }
  if (opacity == (const char *) NULL)
    return(colorize_image);
  /*
    Determine RGB values of the pen color.
  */
  flags=ParseGeometry(opacity,&geometry_info);
  pixel.red=geometry_info.rho;
  if ((flags & SigmaValue) != 0)
    pixel.green=geometry_info.sigma;
  else
    pixel.green=pixel.red;
  if ((flags & XiValue) != 0)
    pixel.blue=geometry_info.xi;
  else
    pixel.blue=pixel.red;
  if ((flags & PsiValue) != 0)
    pixel.opacity=geometry_info.psi;
  else
    pixel.opacity=(MagickRealType) OpaqueOpacity;
  /*
    Colorize DirectClass image.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=GetImagePixels(colorize_image,0,y,colorize_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      q->red=(Quantum) ((p->red*(100.0-pixel.red)+
        colorize.red*pixel.red)/100.0);
      q->green=(Quantum) ((p->green*(100.0-pixel.green)+
        colorize.green*pixel.green)/100.0);
      q->blue=(Quantum) ((p->blue*(100.0-pixel.blue)+
        colorize.blue*pixel.blue)/100.0);
      q->opacity=(Quantum) ((p->opacity*(100.0-pixel.opacity)+
        colorize.opacity*pixel.opacity)/100.0);
      p++;
      q++;
    }
    if (SyncImagePixels(colorize_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ColorizeImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(colorize_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o n v o l v e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConvolveImage() applies a custom convolution kernel to the image.
%
%  The format of the ConvolveImage method is:
%
%      Image *ConvolveImage(const Image *image,const unsigned long order,
%        const double *kernel,ExceptionInfo *exception)
%      Image *ConvolveImageChannel(const Image *image,const ChannelType channel,
%        const unsigned long order,const double *kernel,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel type.
%
%    o order: The number of columns and rows in the filter kernel.
%
%    o kernel: An array of double representing the convolution kernel.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

MagickExport Image *ConvolveImage(const Image *image,const unsigned long order,
  const double *kernel,ExceptionInfo *exception)
{
  Image
    *convolve_image;

  convolve_image=ConvolveImageChannel(image,DefaultChannels,order,kernel,
    exception);
  return(convolve_image);
}

MagickExport Image *ConvolveImageChannel(const Image *image,
  const ChannelType channel,const unsigned long order,const double *kernel,
  ExceptionInfo *exception)
{
#define ConvolveImageTag  "Convolve/Image"

  Image
    *convolve_image;

  long
    j,
    u,
    v,
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MagickRealType
    alpha,
    gamma;

  register const double
    *k;
  register const IndexPacket
    *indexes;

  register const PixelPacket
    *pixels;

  register IndexPacket
    *convolve_indexes;

  register long
    x;

  register PixelPacket
    *convolve_pixels;

  unsigned long
    width;

  /*
    Initialize convolve image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  width=order;
  if ((width % 2) == 0)
    ThrowImageException(OptionError,"KernelWidthMustBeAnOddNumber");
  convolve_image=CloneImage(image,0,0,MagickTrue,exception);
  if (convolve_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(convolve_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&convolve_image->exception);
      convolve_image=DestroyImage(convolve_image);
      return((Image *) NULL);
    }
  /*
    Convolve image.
  */
  if (image->debug != MagickFalse)
    {
      char
        format[MaxTextExtent],
        *message;

      (void) LogMagickEvent(TransformEvent,GetMagickModule(),
        "  ConvolveImage with %ldx%ld kernel:",width,width);
      message=AcquireString("");
      k=kernel;
      for (v=0; v < (long) width; v++)
      {
        *message='\0';
        (void) FormatMagickString(format,MaxTextExtent,"%ld: ",v);
        (void) ConcatenateString(&message,format);
        for (u=0; u < (long) width; u++)
        {
          (void) FormatMagickString(format,MaxTextExtent,"%+f ",*k++);
          (void) ConcatenateString(&message,format);
        }
        (void) LogMagickEvent(TransformEvent,GetMagickModule(),"%s",message);
      }
      message=DestroyString(message);
    }
  for (y=0; y < (long) convolve_image->rows; y++)
  {
    pixels=AcquireImagePixels(image,-((long) width/2L),y-(long) (width/2L),
      image->columns+width,width,exception);
    convolve_pixels=GetImagePixels(convolve_image,0,y,convolve_image->columns,
      1);
    if ((pixels == (const PixelPacket *) NULL) ||
        (convolve_pixels == (PixelPacket *) NULL))
      break;
    indexes=AcquireIndexes(image);
    convolve_indexes=GetIndexes(convolve_image);
    #pragma omp parallel for private(alpha, gamma, j, k, pixel, u, v)
    for (x=0; x < (long) convolve_image->columns; x++)
    {
      GetMagickPixelPacket(image,&pixel);
      gamma=0.0;
      k=kernel;
      j=0;
      for (v=0; v < (long) width; v++)
      {
        for (u=0; u < (long) width; u++)
        {
          alpha=1.0;
          if (((channel & OpacityChannel) != 0) &&
              (image->matte != MagickFalse))
            alpha=((MagickRealType) QuantumRange-pixels[x+u+j].opacity)/
              (MagickRealType) QuantumRange;
          if ((channel & RedChannel) != 0)
            pixel.red+=(*k)*alpha*pixels[x+u+j].red;
          if ((channel & GreenChannel) != 0)
            pixel.green+=(*k)*alpha*pixels[x+u+j].green;
          if ((channel & BlueChannel) != 0)
            pixel.blue+=(*k)*alpha*pixels[x+u+j].blue;
          if ((channel & OpacityChannel) != 0)
            pixel.opacity+=(*k)*pixels[x+u+j].opacity;
          if (((channel & IndexChannel) != 0) &&
              (image->colorspace == CMYKColorspace))
            pixel.index+=(*k)*alpha*indexes[x+u+j];
          gamma+=(*k)*alpha;
          k++;
        }
        j+=image->columns+width;
      }
      gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
      if ((channel & RedChannel) != 0)
        convolve_pixels[x].red=RoundToQuantum(gamma*pixel.red+image->bias);
      if ((channel & GreenChannel) != 0)
        convolve_pixels[x].green=RoundToQuantum(gamma*pixel.green+image->bias);
      if ((channel & BlueChannel) != 0)
        convolve_pixels[x].blue=RoundToQuantum(gamma*pixel.blue+image->bias);
      if ((channel & OpacityChannel) != 0)
        convolve_pixels[x].opacity=RoundToQuantum(pixel.opacity+image->bias);
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        convolve_indexes[x]=RoundToQuantum(gamma*pixel.index+image->bias);
    }
    if (SyncImagePixels(convolve_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ConvolveImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(convolve_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y F x I n f o                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyFxInfo() deallocates memory associated with an FxInfo structure.
%
%  The format of the DestroyFxInfo method is:
%
%      ImageInfo *DestroyFxInfo(ImageInfo *fx_info)
%
%  A description of each parameter follows:
%
%    o fx_info: The fx info.
%
*/
MagickExport FxInfo *DestroyFxInfo(FxInfo *fx_info)
{
  register long
    i;

  fx_info->exception=DestroyExceptionInfo(fx_info->exception);
  fx_info->expression=DestroyString(fx_info->expression);
  fx_info->symbols=DestroySplayTree(fx_info->symbols);
  fx_info->colors=DestroySplayTree(fx_info->colors);
  for (i=0; i < (long) GetImageListLength(fx_info->images); i++)
    fx_info->resample_filter[i]=DestroyResampleFilter(
      fx_info->resample_filter[i]);
  fx_info->resample_filter=(ResampleFilter **) RelinquishMagickMemory(
    fx_info->resample_filter);
  fx_info=(FxInfo *) RelinquishMagickMemory(fx_info);
  return(fx_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     E v a l u a t e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EvaluateImage() applies a value to the image with an arithmetic, relational,
%  or logical operator to an image. Use these operations to lighten or darken
%  an image, to increase or decrease contrast in an image, or to produce the
%  "negative" of an image.
%
%  The format of the EvaluateImageChannel method is:
%
%      MagickBooleanType EvaluateImage(Image *image,
%        const MagickEvaluateOperator op,const double value,
%        ExceptionInfo *exception)
%      MagickBooleanType EvaluateImageChannel(Image *image,
%        const ChannelType channel,const MagickEvaluateOperator op,
%        const double value,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel.
%
%    o op: A channel op.
%
%    o value: A value value.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

static inline double MagickMax(const double x,const double y)
{
  if (x > y)
    return(x);
  return(y);
}

static inline double MagickMin(const double x,const double y)
{
  if (x < y)
    return(x);
  return(y);
}

static inline Quantum ApplyEvaluateOperator(Quantum pixel,
  const MagickEvaluateOperator op,const MagickRealType value)
{
  double
    result;

  result=0.0;
  switch(op)
  {
    case UndefinedEvaluateOperator:
      break;
    case AddEvaluateOperator:
    {
      result=pixel+value;
      break;
    }
    case AndEvaluateOperator:
    {
      result=(MagickRealType) ((unsigned long) pixel & (unsigned long)
        (value+0.5));
      break;
    }
    case DivideEvaluateOperator:
    {
      result=pixel/(value == 0.0 ? 1.0 : value);
      break;
    }
    case LeftShiftEvaluateOperator:
    {
      result=(MagickRealType) ((unsigned long) pixel << (unsigned long)
        (value+0.5));
      break;
    }
    case MaxEvaluateOperator:
    {
      result=MagickMax((double) pixel,value);
      break;
    }
    case MinEvaluateOperator:
    {
      result=MagickMin((double) pixel,value);
      break;
    }
    case MultiplyEvaluateOperator:
    {
      result=pixel*value;
      break;
    }
    case OrEvaluateOperator:
    {
      result=(MagickRealType) ((unsigned long) pixel | (unsigned long)
        (value+0.5));
      break;
    }
    case RightShiftEvaluateOperator:
    {
      result=(MagickRealType) ((unsigned long) pixel >> (unsigned long)
        (value+0.5));
      break;
    }
    case SetEvaluateOperator:
    {
      result=value;
      break;
    }
    case SubtractEvaluateOperator:
    {
      result=pixel-value;
      break;
    }
    case XorEvaluateOperator:
    {
      result=(MagickRealType) ((unsigned long) pixel ^ (unsigned long)
        (value+0.5));
      break;
    }
  }
  return(RoundToQuantum(result));
}

MagickExport MagickBooleanType EvaluateImage(Image *image,
  const MagickEvaluateOperator op,const double value,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  status=EvaluateImageChannel(image,AllChannels,op,value,exception);
  return(status);
}

MagickExport MagickBooleanType EvaluateImageChannel(Image *image,
  const ChannelType channel,const MagickEvaluateOperator op,const double value,
  ExceptionInfo *exception)
{
#define EvaluateImageTag  "Constant/Image "

  IndexPacket
    *indexes;

  long
    y;

  MagickBooleanType
    status;

  register long
    x;

  register PixelPacket
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if (SetImageStorageClass(image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&image->exception);
      return(MagickFalse);
    }
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      if ((channel & RedChannel) != 0)
        q->red=ApplyEvaluateOperator(q->red,op,value);
      if ((channel & GreenChannel) != 0)
        q->green=ApplyEvaluateOperator(q->green,op,value);
      if ((channel & BlueChannel) != 0)
        q->blue=ApplyEvaluateOperator(q->blue,op,value);
      if ((channel & OpacityChannel) != 0)
        {
          if (image->matte == MagickFalse)
            q->opacity=ApplyEvaluateOperator(q->opacity,op,value);
          else
            q->opacity=(Quantum) QuantumRange-ApplyEvaluateOperator(
              (Quantum) QuantumRange-q->opacity,op,value);
        }
      if (((channel & IndexChannel) != 0) && (indexes != (IndexPacket *) NULL))
        indexes[x]=(IndexPacket) ApplyEvaluateOperator(indexes[x],op,value);
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(EvaluateImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(y == (long) image->rows ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     F x E v a l u a t e C h a n n e l E x p r e s s i o n                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FxEvaluateChannelExpression() evaluates an expression and returns the
%  results.
%
%  The format of the FxEvaluateExpression method is:
%
%      MagickRealType FxEvaluateChannelExpression(FxInfo *fx_info,
%        const ChannelType channel,const long x,const long y,
%        MagickRealType *alpha,Exceptioninfo *exception)
%      MagickRealType FxEvaluateExpression(FxInfo *fx_info,
%        MagickRealType *alpha,Exceptioninfo *exception)
%
%  A description of each parameter follows:
%
%    o fx_info: The fx info.
%
%    o channel: The channel.
%
%    o x,y: the pixel position.
%
%    o alpha: the result.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

static MagickRealType
  FxEvaluateSubexpression(FxInfo *,const ChannelType,const long,const long,
    const char *,MagickRealType *,ExceptionInfo *);

static inline MagickRealType FxMax(FxInfo *fx_info,const ChannelType channel,
  const long x,const long y,const char *expression,ExceptionInfo *exception)
{
  MagickRealType
    alpha,
    beta;

  alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression,&beta,exception);
  return((MagickRealType) MagickMax((double) alpha,(double) beta));
}

static inline MagickRealType FxMin(FxInfo *fx_info,ChannelType channel,
  const long x,const long y,const char *expression,ExceptionInfo *exception)
{
  MagickRealType
    alpha,
    beta;

  alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression,&beta,exception);
  return((MagickRealType) MagickMin((double) alpha,(double) beta));
}

static inline const char *FxSubexpression(const char *expression,
  ExceptionInfo *exception)
{
  const char
    *subexpression;

  register long
    level;

  level=0;
  subexpression=expression;
  while ((*subexpression != '\0') &&
         ((level != 1) || (strchr(")",(int) *subexpression) == (char *) NULL)))
  {
    if (strchr("(",(int) *subexpression) != (char *) NULL)
      level++;
    else
      if (strchr(")",(int) *subexpression) != (char *) NULL)
        level--;
    subexpression++;
  }
  if (*subexpression == '\0')
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
      "UnbalancedParenthesis","`%s'",expression);
  return(subexpression);
}

static MagickRealType FxGetSymbol(FxInfo *fx_info,const ChannelType channel,
  const long x,const long y,const char *expression,ExceptionInfo *exception)
{
  char
    *q,
    subexpression[MaxTextExtent],
    symbol[MaxTextExtent];

  const char
    *p,
    *value;

  Image
    *image;

  MagickPixelPacket
    pixel;

  MagickRealType
    alpha,
    beta;

  PointInfo
    point;

  register long
    i;

  unsigned long
    level;

  p=expression;
  i=GetImageIndexInList(fx_info->images);
  level=0;
  point.x=(double) x;
  point.y=(double) y;
  if (isalpha((int) *(p+1)) == 0)
    {
      if (strchr("suv",(int) *p) != (char *) NULL)
        {
          switch (*p)
          {
            case 's':
            default:
            {
              i=GetImageIndexInList(fx_info->images);
              break;
            }
            case 'u': i=0; break;
            case 'v': i=1; break;
          }
          p++;
          if (*p == '[')
            {
              level++;
              q=subexpression;
              for (p++; *p != '\0'; )
              {
                if (*p == '[')
                  level++;
                else
                  if (*p == ']')
                    {
                      level--;
                      if (level == 0)
                        break;
                    }
                *q++=(*p++);
              }
              *q='\0';
              alpha=FxEvaluateSubexpression(fx_info,channel,x,y,subexpression,
                &beta,exception);
              i=(long) (alpha+0.5);
              p++;
            }
          if (*p == '.')
            p++;
        }
      if (*p == 'p')
        {
          p++;
          if (*p == '{')
            {
              level++;
              q=subexpression;
              for (p++; *p != '\0'; )
              {
                if (*p == '{')
                  level++;
                else
                  if (*p == '}')
                    {
                      level--;
                      if (level == 0)
                        break;
                    }
                *q++=(*p++);
              }
              *q='\0';
              alpha=FxEvaluateSubexpression(fx_info,channel,x,y,subexpression,
                &beta,exception);
              point.x=alpha;
              point.y=beta;
              p++;
            }
          else
            if (*p == '[')
              {
                level++;
                q=subexpression;
                for (p++; *p != '\0'; )
                {
                  if (*p == '[')
                    level++;
                  else
                    if (*p == ']')
                      {
                        level--;
                        if (level == 0)
                          break;
                      }
                  *q++=(*p++);
                }
                *q='\0';
                alpha=FxEvaluateSubexpression(fx_info,channel,x,y,subexpression,
                  &beta,exception);
                point.x+=alpha;
                point.y+=beta;
                p++;
              }
          if (*p == '.')
            p++;
        }
    }
  image=GetImageFromList(fx_info->images,i);
  if (image == (Image *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "NoSuchImage","`%s'",expression);
      return(0.0);
    }
  pixel=ResamplePixelColor(fx_info->resample_filter[i],point.x,point.y);
  if ((strlen(p) > 2) &&
      (LocaleCompare(p,"intensity") != 0) &&
      (LocaleCompare(p,"hue") != 0) &&
      (LocaleCompare(p,"saturation") != 0) &&
      (LocaleCompare(p,"luminosity") != 0))
    {
      char
        name[MaxTextExtent];

      GetPathComponent(p,BasePath,name);
      if (strlen(name) > 2)
        {
          MagickPixelPacket
            *color;

          color=(MagickPixelPacket *) GetValueFromSplayTree(fx_info->colors,
            name);
          if (color != (MagickPixelPacket *) NULL)
            {
              pixel=(*color);
              p+=strlen(name);
            }
          else
            if (QueryMagickColor(name,&pixel,fx_info->exception) != MagickFalse)
              {
                (void) AddValueToSplayTree(fx_info->colors,ConstantString(name),
                  CloneMagickPixelPacket(&pixel));
                p+=strlen(name);
              }
        }
    }
  (void) CopyMagickString(symbol,p,MaxTextExtent);
  StripString(symbol);
  if (*symbol == '\0')
    {
      switch (channel)
      {
        case RedChannel: return(QuantumScale*pixel.red);
        case GreenChannel: return(QuantumScale*pixel.green);
        case BlueChannel: return(QuantumScale*pixel.blue);
        case OpacityChannel:
        {
          if (pixel.matte == MagickFalse)
            {
              fx_info->matte=MagickFalse;
              return(QuantumScale*pixel.opacity);
            }
          return((MagickRealType) (QuantumScale*(QuantumRange-pixel.opacity)));
        }
        case IndexChannel:
        {
          if (image->colorspace != CMYKColorspace)
            {
              (void) ThrowMagickException(exception,GetMagickModule(),
                OptionError,"ColorSeparatedImageRequired","`%s'",
                image->filename);
              return(0.0);
            }
          return(QuantumScale*pixel.index);
        }
        default:
          break;
      }
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "UnableToParseExpression","`%s'",p);
      return(0.0);
    }
  switch (*symbol)
  {
    case 'A':
    case 'a':
    {
      if (LocaleCompare(symbol,"a") == 0)
        {
          if (pixel.matte == MagickFalse)
            {
              fx_info->matte=MagickFalse;
              return(QuantumScale*pixel.opacity);
            }
          return((MagickRealType) (QuantumScale*(QuantumRange-pixel.opacity)));
        }
      break;
    }
    case 'B':
    case 'b':
    {
      if (LocaleCompare(symbol,"b") == 0)
        return(QuantumScale*pixel.blue);
      break;
    }
    case 'C':
    case 'c':
    {
      if (LocaleCompare(symbol,"c") == 0)
        return(QuantumScale*pixel.red);
      break;
    }
    case 'G':
    case 'g':
    {
      if (LocaleCompare(symbol,"g") == 0)
        return(QuantumScale*pixel.green);
      break;
    }
    case 'K':
    case 'k':
    {
      if (LocaleCompare(symbol,"k") == 0)
        {
          if (image->colorspace != CMYKColorspace)
            {
              (void) ThrowMagickException(exception,GetMagickModule(),
                OptionError,"ColorSeparatedImageRequired","`%s'",
                image->filename);
              return(0.0);
            }
          return(QuantumScale*pixel.index);
        }
      break;
    }
    case 'H':
    case 'h':
    {
      if (LocaleCompare(symbol,"h") == 0)
        return((MagickRealType) image->rows);
      if (LocaleCompare(symbol,"hue") == 0)
        {
          double
            hue,
            luminosity,
            saturation;

          ConvertRGBToHSB(RoundToQuantum(pixel.red),RoundToQuantum(pixel.green),
            RoundToQuantum(pixel.blue),&hue,&saturation,&luminosity);
          return(hue);
        }
      break;
    }
    case 'I':
    case 'i':
    {
      if (LocaleCompare(symbol,"image.resolution.x") == 0)
        return(image->x_resolution);
      if (LocaleCompare(symbol,"image.resolution.y") == 0)
        return(image->y_resolution);
      if (LocaleCompare(symbol,"intensity") == 0)
        return(QuantumScale*MagickPixelIntensity(&pixel));
      if (LocaleCompare(symbol,"i") == 0)
        return((MagickRealType) x);
      break;
    }
    case 'J':
    case 'j':
    {
      if (LocaleCompare(symbol,"j") == 0)
        return((MagickRealType) y);
      break;
    }
    case 'L':
    case 'l':
    {
      if (LocaleCompare(symbol,"luminosity") == 0)
        {
          double
            hue,
            luminosity,
            saturation;

          ConvertRGBToHSB(RoundToQuantum(pixel.red),RoundToQuantum(pixel.green),
            RoundToQuantum(pixel.blue),&hue,&saturation,&luminosity);
          return(luminosity);
        }
      break;
    }
    case 'M':
    case 'm':
    {
      if (LocaleCompare(symbol,"m") == 0)
        return(QuantumScale*pixel.blue);
      break;
    }
    case 'N':
    case 'n':
    {
      if (LocaleCompare(symbol,"n") == 0)
        return((MagickRealType) GetImageListLength(fx_info->images));
      break;
    }
    case 'O':
    case 'o':
    {
      if (LocaleCompare(symbol,"o") == 0)
        return(QuantumScale*pixel.opacity);
      break;
    }
    case 'R':
    case 'r':
    {
      if (LocaleCompare(symbol,"r") == 0)
        return(QuantumScale*pixel.red);
      break;
    }
    case 'S':
    case 's':
    {
      if (LocaleCompare(symbol,"saturation") == 0)
        {
          double
            hue,
            luminosity,
            saturation;

          ConvertRGBToHSB(RoundToQuantum(pixel.red),RoundToQuantum(pixel.green),
            RoundToQuantum(pixel.blue),&hue,&saturation,&luminosity);
          return(saturation);
        }
      break;
    }
    case 'T':
    case 't':
    {
      if (LocaleCompare(symbol,"t") == 0)
        return((MagickRealType) fx_info->images->scene);
      break;
    }
    case 'W':
    case 'w':
    {
      if (LocaleCompare(symbol,"w") == 0)
        return((MagickRealType) image->columns);
      break;
    }
    case 'Y':
    case 'y':
    {
      if (LocaleCompare(symbol,"y") == 0)
        return(QuantumScale*pixel.green);
      break;
    }
    case 'Z':
    case 'z':
    {
      if (LocaleCompare(symbol,"z") == 0)
        {
          MagickRealType
            depth;

          depth=(MagickRealType) GetImageChannelDepth(image,channel,
            fx_info->exception);
          return(depth);
        }
      break;
    }
    default:
      break;
  }
  value=(const char *) GetValueFromSplayTree(fx_info->symbols,symbol);
  if (value != (const char *) NULL)
    return((MagickRealType) atof(value));
  (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
    "UnableToParseExpression","`%s'",symbol);
  return(0.0);
}

static const char *FxOperatorPrecedence(const char *expression,
  ExceptionInfo *exception)
{
  typedef enum
  {
    UndefinedPrecedence,
    NullPrecedence,
    BitwiseComplementPrecedence,
    ExponentPrecedence,
    MultiplyPrecedence,
    AdditionPrecedence,
    ShiftPrecedence,
    RelationalPrecedence,
    EquivalencyPrecedence,
    BitwiseAndPrecedence,
    BitwiseOrPrecedence,
    LogicalAndPrecedence,
    LogicalOrPrecedence,
    TernaryPrecedence,
    AssignmentPrecedence,
    CommaPrecedence,
    SeparatorPrecedence
  } FxPrecedence;

  FxPrecedence
    precedence,
    target;

  register const char
    *subexpression;

  register int
    c;

  unsigned long
    level;

  c=0;
  level=0;
  subexpression=(const char *) NULL;
  target=NullPrecedence;
  while (*expression != '\0')
  {
    precedence=UndefinedPrecedence;
    if ((isspace((int) ((char) *expression)) != 0) || (c == (int) '@'))
      {
        expression++;
        continue;
      }
    if (LocaleNCompare(expression,"atan2",5) == 0)
      {
        expression+=5;
        continue;
      }
    if ((c == (int) '{') || (c == (int) '['))
      level++;
    else
      if ((c == (int) '}') || (c == (int) ']'))
        level--;
    if (level == 0)
      switch ((unsigned char) *expression)
      {
        case '~':
        case '!':
        {
          precedence=BitwiseComplementPrecedence;
          break;
        }
        case '^':
        {
          precedence=ExponentPrecedence;
          break;
        }
        default:
        {
          if (((c != 0) && ((isdigit((int) ((char) c)) != 0) ||
               (strchr(")",c) != (char *) NULL))) &&
              (((islower((int) ((char) *expression)) != 0) ||
               (strchr("(",(int) *expression) != (char *) NULL)) ||
               ((isdigit((int) ((char) c)) == 0) &&
                (isdigit((int) ((char) *expression)) != 0))) &&
              (strchr("xy",(int) *expression) == (char *) NULL))
            precedence=MultiplyPrecedence;
          break;
        }
        case '*':
        case '/':
        case '%':
        {
          precedence=MultiplyPrecedence;
          break;
        }
        case '+':
        case '-':
        {
          if ((strchr("(+-/*%:&^|<>~,",c) == (char *) NULL) ||
              (isalpha(c) != 0))
            precedence=AdditionPrecedence;
          break;
        }
        case LeftShiftOperator:
        case RightShiftOperator:
        {
          precedence=ShiftPrecedence;
          break;
        }
        case '<':
        case LessThanEqualOperator:
        case GreaterThanEqualOperator:
        case '>':
        {
          precedence=RelationalPrecedence;
          break;
        }
        case EqualOperator:
        case NotEqualOperator:
        {
          precedence=EquivalencyPrecedence;
          break;
        }
        case '&':
        {
          precedence=BitwiseAndPrecedence;
          break;
        }
        case '|':
        {
          precedence=BitwiseOrPrecedence;
          break;
        }
        case LogicalAndOperator:
        {
          precedence=LogicalAndPrecedence;
          break;
        }
        case LogicalOrOperator:
        {
          precedence=LogicalOrPrecedence;
          break;
        }
        case ':':
        case '?':
        {
          precedence=TernaryPrecedence;
          break;
        }
        case '=':
        {
          precedence=AssignmentPrecedence;
          break;
        }
        case ',':
        {
          precedence=CommaPrecedence;
          break;
        }
        case ';':
        {
          precedence=SeparatorPrecedence;
          break;
        }
      }
    if ((precedence == BitwiseComplementPrecedence) ||
        (precedence == TernaryPrecedence) ||
        (precedence == AssignmentPrecedence))
      {
        if (precedence > target)
          {
            /*
              Right-to-left associativity.
            */
            target=precedence;
            subexpression=expression;
          }
      }
    else
      if (precedence >= target)
        {
          /*
            Left-to-right associativity.
          */
          target=precedence;
          subexpression=expression;
        }
    if (strchr("(",(int) *expression) != (char *) NULL)
      expression=FxSubexpression(expression,exception);
    c=(int) (*expression++);
  }
  return(subexpression);
}

static MagickRealType FxEvaluateSubexpression(FxInfo *fx_info,
  const ChannelType channel,const long x,const long y,const char *expression,
  MagickRealType *beta,ExceptionInfo *exception)
{
  char
    *q,
    subexpression[MaxTextExtent];

  MagickRealType
    alpha,
    gamma;

  register const char
    *p;

  *beta=0.0;
  if (exception->severity != UndefinedException)
    return(0.0);
  while (isspace((int) *expression) != 0)
    expression++;
  if (*expression == '\0')
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "MissingExpression","`%s'",expression);
      return(0.0);
    }
  p=FxOperatorPrecedence(expression,exception);
  if (p != (const char *) NULL)
    {
      (void) CopyMagickString(subexpression,expression,(size_t)
        (p-expression+1));
      alpha=FxEvaluateSubexpression(fx_info,channel,x,y,subexpression,beta,
        exception);
      switch ((unsigned char) *p)
      {
        case '~':
        {
          *beta=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          *beta=(MagickRealType) (~(unsigned long) *beta);
          return(*beta);
        }
        case '!':
        {
          *beta=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          return(*beta == 0.0 ? 1.0 : 0.0);
        }
        case '^':
        {
          *beta=pow((double) alpha,(double) FxEvaluateSubexpression(fx_info,
            channel,x,y,++p,beta,exception));
          return(*beta);
        }
        case '*':
        {
          *beta=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          return(alpha*(*beta));
        }
        case '/':
        {
          *beta=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          if (*beta == 0.0)
            {
              if (exception->severity == UndefinedException)
                (void) ThrowMagickException(exception,GetMagickModule(),
                  OptionError,"DivideByZero","`%s'",expression);
              return(0.0);
            }
          return(alpha/(*beta));
        }
        case '%':
        {
          *beta=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          *beta=fabs(floor(((double) *beta)+0.5));
          if (*beta == 0.0)
            {
              (void) ThrowMagickException(exception,GetMagickModule(),
                OptionError,"DivideByZero","`%s'",expression);
              return(0.0);
            }
          return(fmod((double) alpha,(double) *beta));
        }
        case '+':
        {
          *beta=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          return(alpha+(*beta));
        }
        case '-':
        {
          *beta=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          return(alpha-(*beta));
        }
        case LeftShiftOperator:
        {
          gamma=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          *beta=(MagickRealType) ((unsigned long) (alpha+0.5) << (unsigned long)
            (gamma+0.5));
          return(*beta);
        }
        case RightShiftOperator:
        {
          gamma=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          *beta=(MagickRealType) ((unsigned long) (alpha+0.5) >> (unsigned long)
            (gamma+0.5));
          return(*beta);
        }
        case '<':
        {
          *beta=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          return(alpha < *beta ? 1.0 : 0.0);
        }
        case LessThanEqualOperator:
        {
          *beta=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          return(alpha <= *beta ? 1.0 : 0.0);
        }
        case '>':
        {
          *beta=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          return(alpha > *beta ? 1.0 : 0.0);
        }
        case GreaterThanEqualOperator:
        {
          *beta=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          return(alpha >= *beta ? 1.0 : 0.0);
        }
        case EqualOperator:
        {
          *beta=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          return(alpha == *beta ? 1.0 : 0.0);
        }
        case NotEqualOperator:
        {
          *beta=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          return(alpha != *beta ? 1.0 : 0.0);
        }
        case '&':
        {
          gamma=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          *beta=(MagickRealType) ((unsigned long) (alpha+0.5) & (unsigned long)
            (gamma+0.5));
          return(*beta);
        }
        case '|':
        {
          gamma=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          *beta=(MagickRealType) ((unsigned long) (alpha+0.5) | (unsigned long)
            (gamma+0.5));
          return(*beta);
        }
        case LogicalAndOperator:
        {
          gamma=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          *beta=(alpha > 0.0) && (gamma > 0.0) ? 1.0 : 0.0;
          return(*beta);
        }
        case LogicalOrOperator:
        {
          gamma=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          *beta=(alpha > 0.0) || (gamma > 0.0) ? 1.0 : 0.0;
          return(*beta);
        }
        case '?':
        {
          MagickRealType
            gamma;

          (void) CopyMagickString(subexpression,++p,MaxTextExtent);
          q=subexpression;
          p=StringToken(":",&q);
          if (q == (char *) NULL)
            {
              (void) ThrowMagickException(exception,GetMagickModule(),
                OptionError,"UnableToParseExpression","`%s'",subexpression);
              return(0.0);
            }
          if (fabs((double) alpha) > MagickEpsilon)
            gamma=FxEvaluateSubexpression(fx_info,channel,x,y,p,beta,exception);
          else
            gamma=FxEvaluateSubexpression(fx_info,channel,x,y,q,beta,exception);
          return(gamma);
        }
        case '=':
        {
          char
            numeric[MaxTextExtent];

          q=subexpression;
          while (isalpha((int) ((unsigned char) *q)) != 0)
            q++;
          if (*q != '\0')
            {
              (void) ThrowMagickException(exception,GetMagickModule(),
                OptionError,"UnableToParseExpression","`%s'",subexpression);
              return(0.0);
            }
          ClearMagickException(exception);
          *beta=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          (void) FormatMagickString(numeric,MaxTextExtent,"%g",(double) *beta);
          (void) DeleteNodeFromSplayTree(fx_info->symbols,subexpression);
          (void) AddValueToSplayTree(fx_info->symbols,ConstantString(
            subexpression),ConstantString(numeric));
          return(*beta);
        }
        case ',':
        {
          *beta=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          return(alpha);
        }
        case ';':
        {
          *beta=FxEvaluateSubexpression(fx_info,channel,x,y,++p,beta,exception);
          return(*beta);
        }
        default:
        {
          gamma=alpha*FxEvaluateSubexpression(fx_info,channel,x,y,p,beta,
            exception);
          return(gamma);
        }
      }
    }
  if (strchr("(",(int) *expression) != (char *) NULL)
    {
      (void) CopyMagickString(subexpression,expression+1,MaxTextExtent);
      subexpression[strlen(subexpression)-1]='\0';
      gamma=FxEvaluateSubexpression(fx_info,channel,x,y,subexpression,beta,
        exception);
      return(gamma);
    }
  switch (*expression)
  {
    case '+':
    {
      gamma=FxEvaluateSubexpression(fx_info,channel,x,y,expression+1,beta,
        exception);
      return(1.0*gamma);
    }
    case '-':
    {
      gamma=FxEvaluateSubexpression(fx_info,channel,x,y,expression+1,beta,
        exception);
      return(-1.0*gamma);
    }
    case '~':
    {
      gamma=FxEvaluateSubexpression(fx_info,channel,x,y,expression+1,beta,
        exception);
      return((MagickRealType) (~(unsigned long) (gamma+0.5)));
    }
    case 'A':
    case 'a':
    {
      if (LocaleNCompare(expression,"abs",3) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+3,beta,
            exception);
          return((MagickRealType) fabs((double) alpha));
        }
      if (LocaleNCompare(expression,"acos",4) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+4,beta,
            exception);
          return((MagickRealType) acos((double) alpha));
        }
      if (LocaleNCompare(expression,"asin",4) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+4,beta,
            exception);
          return((MagickRealType) asin((double) alpha));
        }
      if (LocaleNCompare(expression,"alt",3) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+3,beta,
            exception);
          return(((long) alpha) & 0x01 ? -1.0 : 1.0);
        }
      if (LocaleNCompare(expression,"atan2",5) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+5,beta,
            exception);
          return((MagickRealType) atan2((double) alpha,(double) *beta));
        }
      if (LocaleNCompare(expression,"atan",4) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+4,beta,
            exception);
          return((MagickRealType) atan((double) alpha));
        }
      if (LocaleCompare(expression,"a") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'B':
    case 'b':
    {
      if (LocaleCompare(expression,"b") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'C':
    case 'c':
    {
      if (LocaleNCompare(expression,"ceil",4) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+4,beta,
            exception);
          return((MagickRealType) ceil((double) alpha));
        }
      if (LocaleNCompare(expression,"cos",3) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+3,beta,
            exception);
          return((MagickRealType) cos((double) alpha));
        }
      if (LocaleCompare(expression,"c") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'D':
    case 'd':
    {
      if (LocaleNCompare(expression,"debug",5) == 0)
        {
          const char
            *type;

          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+5,beta,
            exception);
          if (fx_info->images->colorspace == CMYKColorspace)
            switch (channel)
            {
              case RedChannel: type="cyan"; break;
              case GreenChannel: type="magenta"; break;
              case BlueChannel: type="yellow"; break;
              case OpacityChannel: type="opacity"; break;
              case IndexChannel: type="black"; break;
              default: type="unknown"; break;
            }
          else
            switch (channel)
            {
              case RedChannel: type="red"; break;
              case GreenChannel: type="green"; break;
              case BlueChannel: type="blue"; break;
              case OpacityChannel: type="opacity"; break;
              default: type="unknown"; break;
            }
          (void) CopyMagickString(subexpression,expression+6,MaxTextExtent);
          if (strlen(subexpression) > 1)
            subexpression[strlen(subexpression)-1]='\0';
          (void) fprintf(stderr,"%s[%ld,%ld].%s: %s=%g\n",
            fx_info->images->filename,y,x,type,subexpression,(double) alpha);
          return(0.0);
        }
      break;
    }
    case 'E':
    case 'e':
    {
      if (LocaleNCompare(expression,"exp",3) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+3,beta,
            exception);
          return((MagickRealType) exp((double) alpha));
        }
      if (LocaleCompare(expression,"e") == 0)
        return((MagickRealType) 2.7182818284590452354);
      break;
    }
    case 'F':
    case 'f':
    {
      if (LocaleNCompare(expression,"floor",5) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+5,beta,
            exception);
          return((MagickRealType) floor((double) alpha));
        }
      break;
    }
    case 'G':
    case 'g':
    {
      if (LocaleCompare(expression,"g") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'H':
    case 'h':
    {
      if (LocaleCompare(expression,"h") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      if (LocaleCompare(expression,"hue") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      if (LocaleNCompare(expression,"hypot",5) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+5,beta,
            exception);
          return((MagickRealType) hypot((double) alpha,(double) *beta));
        }
      break;
    }
    case 'K':
    case 'k':
    {
      if (LocaleCompare(expression,"k") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'I':
    case 'i':
    {
      if (LocaleCompare(expression,"intensity") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      if (LocaleNCompare(expression,"int",3) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+3,beta,
            exception);
          return((MagickRealType) floor(alpha+0.5));
        }
      if (LocaleCompare(expression,"i") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'J':
    case 'j':
    {
      if (LocaleCompare(expression,"j") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'L':
    case 'l':
    {
      if (LocaleNCompare(expression,"ln",2) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+2,beta,
            exception);
          return((MagickRealType) log((double) alpha));
        }
      if (LocaleNCompare(expression,"log",3) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+3,beta,
            exception);
          return((MagickRealType) log10((double) alpha));
        }
      if (LocaleCompare(expression,"luminosity") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'M':
    case 'm':
    {
      if (LocaleCompare(expression,"MaxRGB") == 0)
        return((MagickRealType) QuantumRange);
      if (LocaleNCompare(expression,"max",3) == 0)
        return(FxMax(fx_info,channel,x,y,expression+3,exception));
      if (LocaleNCompare(expression,"min",3) == 0)
        return(FxMin(fx_info,channel,x,y,expression+3,exception));
      if (LocaleNCompare(expression,"mod",3) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+3,beta,
            exception);
          return((MagickRealType) fmod((double) alpha,(double) *beta));
        }
      if (LocaleCompare(expression,"m") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'N':
    case 'n':
    {
      if (LocaleCompare(expression,"n") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'O':
    case 'o':
    {
      if (LocaleCompare(expression,"Opaque") == 0)
        return(1.0);
      if (LocaleCompare(expression,"o") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'P':
    case 'p':
    {
      if (LocaleCompare(expression,"pi") == 0)
        return((MagickRealType) MagickPI);
      if (LocaleNCompare(expression,"pow",3) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+3,beta,
            exception);
          return((MagickRealType) pow((double) alpha,(double) *beta));
        }
      if (LocaleCompare(expression,"p") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'Q':
    case 'q':
    {
      if (LocaleCompare(expression,"QuantumRange") == 0)
        return((MagickRealType) QuantumRange);
      if (LocaleCompare(expression,"QuantumScale") == 0)
        return((MagickRealType) QuantumScale);
      break;
    }
    case 'R':
    case 'r':
    {
      if (LocaleNCompare(expression,"rand",4) == 0)
        return((MagickRealType) GetRandomValue());
      if (LocaleCompare(expression,"r") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'S':
    case 's':
    {
      if (LocaleCompare(expression,"saturation") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      if (LocaleNCompare(expression,"sign",4) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+4,beta,
            exception);
          return(alpha < 0.0 ? -1.0 : 1.0);
        }
      if (LocaleNCompare(expression,"sin",3) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+3,beta,
            exception);
          return((MagickRealType) sin((double) alpha));
        }
      if (LocaleNCompare(expression,"sqrt",4) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+4,beta,
            exception);
          return((MagickRealType) sqrt((double) alpha));
        }
      if (LocaleCompare(expression,"s") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'T':
    case 't':
    {
      if (LocaleNCompare(expression,"tan",3) == 0)
        {
          alpha=FxEvaluateSubexpression(fx_info,channel,x,y,expression+3,beta,
            exception);
          return((MagickRealType) tan((double) alpha));
        }
      if (LocaleCompare(expression,"Transparent") == 0)
        return(0.0);
      if (LocaleCompare(expression,"t") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'U':
    case 'u':
    {
      if (LocaleCompare(expression,"u") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'V':
    case 'v':
    {
      if (LocaleCompare(expression,"v") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'W':
    case 'w':
    {
      if (LocaleCompare(expression,"w") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'Y':
    case 'y':
    {
      if (LocaleCompare(expression,"y") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    case 'Z':
    case 'z':
    {
      if (LocaleCompare(expression,"z") == 0)
        return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
      break;
    }
    default:
      break;
  }
  q=(char *) expression;
  alpha=strtod(expression,&q);
  if (q == expression)
    return(FxGetSymbol(fx_info,channel,x,y,expression,exception));
  return(alpha);
}

MagickExport MagickBooleanType FxEvaluateExpression(FxInfo *fx_info,
  MagickRealType *alpha,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  status=FxEvaluateChannelExpression(fx_info,GrayChannel,0,0,alpha,
    exception);
  return(status);
}

MagickExport MagickBooleanType FxEvaluateChannelExpression(FxInfo *fx_info,
  const ChannelType channel,const long x,const long y,MagickRealType *alpha,
  ExceptionInfo *exception)
{
  MagickRealType
    beta;

  *alpha=FxEvaluateSubexpression(fx_info,channel,x,y,fx_info->expression,&beta,
    exception);
  return(exception->severity == OptionError ? MagickFalse : MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     F x I m a g e                                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FxImage() applies a mathematical expression to the specified image.
%
%  The format of the FxImage method is:
%
%      Image *FxImage(const Image *image,const char *expression,
%        ExceptionInfo *exception)
%      Image *FxImageChannel(const Image *image,const ChannelType channel,
%        const char *expression,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel.
%
%    o expression: A mathematical expression.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

MagickExport Image *FxImage(const Image *image,const char *expression,
  ExceptionInfo *exception)
{
  Image
    *fx_image;

  fx_image=FxImageChannel(image,GrayChannel,expression,exception);
  return(fx_image);
}

MagickExport Image *FxImageChannel(const Image *image,const ChannelType channel,
  const char *expression,ExceptionInfo *exception)
{
#define FxImageTag  "Fx/Image"

  FxInfo
    *fx_info;

  Image
    *fx_image;

  IndexPacket
    *indexes;

  long
    y;

  MagickBooleanType
    status;

  MagickRealType
    alpha;

  register long
    x;

  register PixelPacket
    *pixels;

  /*
    Fx image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  fx_image=CloneImage(image,image->columns,image->rows,MagickTrue,exception);
  if (fx_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(fx_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&fx_image->exception);
      fx_image=DestroyImage(fx_image);
      return((Image *) NULL);
    }
  fx_info=AcquireFxInfo(image,expression);
  status=FxEvaluateExpression(fx_info,&alpha,exception);
  if (status == MagickFalse)
    {
      fx_info=DestroyFxInfo(fx_info);
      return((Image *) NULL);
    }
  for (y=0; y < (long) fx_image->rows; y++)
  {
    pixels=GetImagePixels(fx_image,0,y,fx_image->columns,1);
    if (pixels == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(fx_image);
    for (x=0; x < (long) fx_image->columns; x++)
    {
      if ((channel & RedChannel) != 0)
        {
          status=FxEvaluateChannelExpression(fx_info,RedChannel,x,y,&alpha,
            exception);
          pixels[x].red=RoundToQuantum((MagickRealType) QuantumRange*alpha);
        }
      if ((channel & GreenChannel) != 0)
        {
          status=FxEvaluateChannelExpression(fx_info,GreenChannel,x,y,&alpha,
            exception);
          pixels[x].green=RoundToQuantum((MagickRealType) QuantumRange*alpha);
        }
      if ((channel & BlueChannel) != 0)
        {
          status=FxEvaluateChannelExpression(fx_info,BlueChannel,x,y,&alpha,
            exception);
          pixels[x].blue=RoundToQuantum((MagickRealType) QuantumRange*alpha);
        }
      if ((channel & OpacityChannel) != 0)
        {
          status=FxEvaluateChannelExpression(fx_info,OpacityChannel,x,y,&alpha,
            exception);
          if (image->matte == MagickFalse)
            pixels[x].opacity=RoundToQuantum((MagickRealType) QuantumRange*
              alpha);
          else
            pixels[x].opacity=RoundToQuantum((MagickRealType) (QuantumRange-
              QuantumRange*alpha));
        }
      if (((channel & IndexChannel) != 0) &&
          (fx_image->colorspace == CMYKColorspace))
        {
          status=FxEvaluateChannelExpression(fx_info,IndexChannel,x,y,&alpha,
            exception);
          indexes[x]=(IndexPacket) RoundToQuantum((MagickRealType) QuantumRange*
            alpha);
        }
    }
    if (SyncImagePixels(fx_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(FxImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  fx_image->matte=fx_info->matte;
  fx_info=DestroyFxInfo(fx_info);
  return(fx_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     I m p l o d e I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImplodeImage() creates a new image that is a copy of an existing
%  one with the image pixels "implode" by the specified percentage.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ImplodeImage method is:
%
%      Image *ImplodeImage(const Image *image,const double amount,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o implode_image: Method ImplodeImage returns a pointer to the image
%      after it is implode.  A null image is returned if there is a memory
%      shortage.
%
%    o image: The image.
%
%    o amount:  Define the extent of the implosion.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *ImplodeImage(const Image *image,const double amount,
  ExceptionInfo *exception)
{
#define ImplodeImageTag  "Implode/Image"

  Image
    *implode_image;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MagickRealType
    distance,
    radius;

  PointInfo
    center,
    delta,
    scale;

  register const IndexPacket
    *indexes;

  register const PixelPacket
    *p;

  register IndexPacket
    *implode_indexes;

  register long
    x;

  register PixelPacket
    *q;

  ResampleFilter
    *resample_filter;

  ViewInfo
    *image_view,
    *implode_view;

  /*
    Initialize implode image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  implode_image=CloneImage(image,0,0,MagickTrue,exception);
  if (implode_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(implode_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&implode_image->exception);
      implode_image=DestroyImage(implode_image);
      return((Image *) NULL);
    }
  if (implode_image->background_color.opacity != OpaqueOpacity)
    implode_image->matte=MagickTrue;
  /*
    Compute scaling factor.
  */
  scale.x=1.0;
  scale.y=1.0;
  center.x=0.5*image->columns;
  center.y=0.5*image->rows;
  radius=center.x;
  if (image->columns > image->rows)
    scale.y=(double) image->columns/(double) image->rows;
  else
    if (image->columns < image->rows)
      {
        scale.x=(double) image->rows/(double) image->columns;
        radius=center.y;
      }
  /*
    Implode image.
  */
  GetMagickPixelPacket(implode_image,&pixel);
  resample_filter=AcquireResampleFilter(image,exception);
  image_view=OpenCacheView(image);
  implode_view=OpenCacheView(implode_image);
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetCacheViewPixels(implode_view,0,y,implode_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    implode_indexes=GetCacheViewIndexes(implode_view);
    delta.y=scale.y*(double) (y-center.y);
    for (x=0; x < (long) image->columns; x++)
    {
      /*
        Determine if the pixel is within an ellipse.
      */
      delta.x=scale.x*(double) (x-center.x);
      distance=delta.x*delta.x+delta.y*delta.y;
      if (distance >= (radius*radius))
        {
          p=AcquireCacheViewPixels(image_view,x,y,1,1,exception);
          if (p == (const PixelPacket *) NULL)
            break;
          indexes=AcquireCacheViewIndexes(image_view);
          SetMagickPixelPacket(image,p,indexes,&pixel);
        }
      else
        {
          double
            factor;

          /*
            Implode the pixel.
          */
          factor=1.0;
          if (distance > 0.0)
            factor=pow(sin((double) (MagickPI*sqrt((double) distance)/
              radius/2)),-amount);
          pixel=ResamplePixelColor(resample_filter,(double) (factor*delta.x/
            scale.x+center.x),(double) (factor*delta.y/scale.y+center.y));
        }
      SetPixelPacket(implode_image,&pixel,q,implode_indexes+x);
      q++;
    }
    if (SyncCacheView(implode_view) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ImplodeImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  resample_filter=DestroyResampleFilter(resample_filter);
  implode_view=CloseCacheView(implode_view);
  image_view=CloseCacheView(image_view);
  return(implode_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M o r p h I m a g e s                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  The MorphImages() method requires a minimum of two images.  The first
%  image is transformed into the second by a number of intervening images
%  as specified by frames.
%
%  The format of the MorphImage method is:
%
%      Image *MorphImages(const Image *image,const unsigned long number_frames,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o number_frames:  Define the number of in-between image to generate.
%      The more in-between frames, the smoother the morph.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *MorphImages(const Image *image,
  const unsigned long number_frames,ExceptionInfo *exception)
{
#define MorphImageTag  "Morph/Image"

  Image
    *morph_image,
    *morph_images;

  long
    y;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  MagickRealType
    alpha,
    beta;

  register const Image
    *next;

  register const PixelPacket
    *p;

  register long
    i,
    x;

  register PixelPacket
    *q;

  /*
    Clone first frame in sequence.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  morph_images=CloneImage(image,0,0,MagickTrue,exception);
  if (morph_images == (Image *) NULL)
    return((Image *) NULL);
  if (GetNextImageInList(image) == (Image *) NULL)
    {
      /*
        Morph single image.
      */
      for (i=1; i < (long) number_frames; i++)
      {
        morph_image=CloneImage(image,0,0,MagickTrue,exception);
        if (morph_image == (Image *) NULL)
          {
            morph_images=DestroyImageList(morph_images);
            return((Image *) NULL);
          }
        AppendImageToList(&morph_images,morph_image);
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(i,number_frames) != MagickFalse))
          {
            status=image->progress_monitor(MorphImageTag,i,number_frames,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
      return(GetFirstImageInList(morph_images));
    }
  /*
    Morph image sequence.
  */
  scene=0;
  next=image;
  for ( ; GetNextImageInList(next) != (Image *) NULL; next=GetNextImageInList(next))
  {
    for (i=0; i < (long) number_frames; i++)
    {
      beta=(MagickRealType) (i+1.0)/(MagickRealType) (number_frames+1.0);
      alpha=1.0-beta;
      morph_image=ZoomImage(next,(unsigned long) (alpha*next->columns+beta*
        GetNextImageInList(next)->columns+0.5),(unsigned long) (alpha*
        next->rows+beta*GetNextImageInList(next)->rows+0.5),exception);
      if (morph_image == (Image *) NULL)
        {
          morph_images=DestroyImageList(morph_images);
          return((Image *) NULL);
        }
      if (SetImageStorageClass(morph_image,DirectClass) == MagickFalse)
        {
          InheritException(exception,&morph_image->exception);
          morph_image=DestroyImage(morph_image);
          return((Image *) NULL);
        }
      AppendImageToList(&morph_images,morph_image);
      morph_images=GetLastImageInList(morph_images);
      morph_image=ZoomImage(GetNextImageInList(next),morph_images->columns,
        morph_images->rows,exception);
      if (morph_image == (Image *) NULL)
        {
          morph_images=DestroyImageList(morph_images);
          return((Image *) NULL);
        }
      for (y=0; y < (long) morph_images->rows; y++)
      {
        p=AcquireImagePixels(morph_image,0,y,morph_image->columns,1,exception);
        q=GetImagePixels(morph_images,0,y,morph_images->columns,1);
        if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
          break;
        for (x=0; x < (long) morph_images->columns; x++)
        {
          q->red=RoundToQuantum(alpha*q->red+beta*p->red);
          q->green=RoundToQuantum(alpha*q->green+beta*p->green);
          q->blue=RoundToQuantum(alpha*q->blue+beta*p->blue);
          q->opacity=RoundToQuantum(alpha*q->opacity+beta*p->opacity);
          p++;
          q++;
        }
        if (SyncImagePixels(morph_images) == MagickFalse)
          break;
      }
      morph_image=DestroyImage(morph_image);
    }
    if (i < (long) number_frames)
      break;
    /*
      Clone last frame in sequence.
    */
    morph_image=CloneImage(GetNextImageInList(next),0,0,MagickTrue,exception);
    if (morph_image == (Image *) NULL)
      {
        morph_images=DestroyImageList(morph_images);
        return((Image *) NULL);
      }
    AppendImageToList(&morph_images,morph_image);
    morph_images=GetLastImageInList(morph_images);
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(scene,GetImageListLength(image)) != MagickFalse))
      {
        status=image->progress_monitor(MorphImageTag,scene,
          GetImageListLength(image),image->client_data);
        if (status == MagickFalse)
          break;
      }
    scene++;
  }
  if (GetNextImageInList(next) != (Image *) NULL)
    {
      morph_images=DestroyImageList(morph_images);
      return((Image *) NULL);
    }
  return(GetFirstImageInList(morph_images));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     O i l P a i n t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OilPaintImage() applies a special effect filter that simulates an oil
%  painting.  Each pixel is replaced by the most frequent color occurring
%  in a circular region defined by radius.
%
%  The format of the OilPaintImage method is:
%
%      Image *OilPaintImage(const Image *image,const double radius,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o radius: The radius of the circular neighborhood.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *OilPaintImage(const Image *image,const double radius,
  ExceptionInfo *exception)
{
#define OilPaintImageTag  "OilPaint/Image"

  Image
    *paint_image;

  long
    k,
    y;

  MagickBooleanType
    status;

  register const PixelPacket
    *pixels;

  register long
    i,
    u,
    v,
    x;

  register PixelPacket
    *paint_pixels;

  unsigned long
    count,
    *histogram,
    width;

  /*
    Initialize painted image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  width=GetOptimalKernelWidth2D(radius,0.5);
  if ((image->columns < width) || (image->rows < width))
    ThrowImageException(OptionError,"ImageSmallerThanRadius");
  paint_image=CloneImage(image,0,0,MagickTrue,exception);
  if (paint_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(paint_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&paint_image->exception);
      paint_image=DestroyImage(paint_image);
      return((Image *) NULL);
    }
  /*
    Allocate histogram and scanline.
  */
  histogram=(unsigned long *) AcquireQuantumMemory(256UL,sizeof(*histogram));
  if (histogram == (unsigned long *) NULL)
    {
      paint_image=DestroyImage(paint_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Paint each row of the image.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    pixels=AcquireImagePixels(image,-((long) width/2L),y-(long) (width/2L),
      image->columns+width,width,exception);
    paint_pixels=GetImagePixels(paint_image,0,y,paint_image->columns,1);
    if ((pixels == (const PixelPacket *) NULL) ||
        (paint_pixels == (PixelPacket *) NULL))
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      /*
        Assign most frequent color.
      */
      i=0;
      count=0;
      (void) ResetMagickMemory(histogram,0,256*sizeof(*histogram));
      for (v=0; v < (long) width; v++)
      {
        for (u=0; u < (long) width; u++)
        {
          k=(long) ScaleQuantumToChar(PixelIntensityToQuantum(pixels+(x+u+i)));
          histogram[k]++;
          if (histogram[k] > count)
            {
              paint_pixels[x]=pixels[x+u+i];
              count=histogram[k];
            }
        }
        i+=image->columns+width;
      }
    }
    if (SyncImagePixels(paint_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(OilPaintImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  histogram=(unsigned long *) RelinquishMagickMemory(histogram);
  return(paint_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P o l a r o i d I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PolaroidImage() simulates a Polaroid picture.
%
%  The format of the AnnotateImage method is:
%
%      Image *PolaroidImage(const Image *image,const DrawInfo *draw_info,
%        const double angle,ExceptionInfo exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o draw_info: The draw info.
%
%    o angle: Apply the effect along this angle.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *PolaroidImage(const Image *image,const DrawInfo *draw_info,
  const double angle,ExceptionInfo *exception)
{
  const char
    *value;

  long
    quantum;

  Image
    *bend_image,
    *caption_image,
    *flop_image,
    *picture_image,
    *polaroid_image,
    *rotate_image,
    *trim_image;

  unsigned long
    height;

  /*
    Simulate a Polaroid picture.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  quantum=(long) MagickMax(MagickMax((double) image->columns,(double)
    image->rows)/25.0,10.0);
  height=image->rows+2*quantum;
  caption_image=(Image *) NULL;
  value=GetImageProperty(image,"Caption");
  if (value != (const char *) NULL)
    {
      char
        *caption,
        geometry[MaxTextExtent];

      DrawInfo
        *annotate_info;

      long
        count;

      MagickBooleanType
        status;

      TypeMetric
        metrics;

      /*
        Generate caption image.
      */
      caption_image=CloneImage(image,image->columns,1,MagickTrue,exception);
      if (caption_image == (Image *) NULL)
        return((Image *) NULL);
      annotate_info=CloneDrawInfo((const ImageInfo *) NULL,draw_info);
      caption=InterpretImageProperties((ImageInfo *) NULL,caption_image,value);
      (void) CloneString(&annotate_info->text,caption);
      count=FormatMagickCaption(caption_image,annotate_info,caption,&metrics);
      status=SetImageExtent(caption_image,image->columns,(unsigned long)
        ((count+1)*(metrics.ascent-metrics.descent)+0.5));
      if (status == MagickFalse)
        caption_image=DestroyImage(caption_image);
      else
        {
          caption_image->background_color=image->border_color;
          (void) SetImageBackgroundColor(caption_image);
          (void) CloneString(&annotate_info->text,caption);
          (void) FormatMagickString(geometry,MaxTextExtent,"+0+%g",
            metrics.ascent);
          if (annotate_info->gravity == UndefinedGravity)
            (void) CloneString(&annotate_info->geometry,AcquireString(
              geometry));
          (void) AnnotateImage(caption_image,annotate_info);
          height+=caption_image->rows;
        }
      annotate_info=DestroyDrawInfo(annotate_info);
      caption=DestroyString(caption);
    }
  picture_image=CloneImage(image,image->columns+2*quantum,height,MagickTrue,
    exception);
  if (picture_image == (Image *) NULL)
    {
      if (caption_image != (Image *) NULL)
        caption_image=DestroyImage(caption_image);
      return((Image *) NULL);
    }
  picture_image->background_color=image->border_color;
  (void) SetImageBackgroundColor(picture_image);
  (void) CompositeImage(picture_image,OverCompositeOp,image,quantum,quantum);
  if (caption_image != (Image *) NULL)
    {
      (void) CompositeImage(picture_image,OverCompositeOp,caption_image,
        quantum,(long) (image->rows+3*quantum/2));
      caption_image=DestroyImage(caption_image);
    }
  (void) QueryColorDatabase("none",&picture_image->background_color,exception);
  (void) SetImageOpacity(picture_image,OpaqueOpacity);
  rotate_image=RotateImage(picture_image,90.0,exception);
  picture_image=DestroyImage(picture_image);
  if (rotate_image == (Image *) NULL)
    return((Image *) NULL);
  picture_image=rotate_image;
  bend_image=WaveImage(picture_image,0.01*picture_image->rows,
       2.0*picture_image->columns,exception);
  picture_image=DestroyImage(picture_image);
  if (bend_image == (Image *) NULL)
    return((Image *) NULL);
  InheritException(&bend_image->exception,exception);
  picture_image=bend_image;
  rotate_image=RotateImage(picture_image,-90.0,exception);
  picture_image=DestroyImage(picture_image);
  if (rotate_image == (Image *) NULL)
    return((Image *) NULL);
  picture_image=rotate_image;
  picture_image->background_color=image->background_color;
  polaroid_image=ShadowImage(picture_image,80.0,2.0,quantum/3,quantum/3,
    exception);
  if (polaroid_image == (Image *) NULL)
    {
      picture_image=DestroyImage(picture_image);
      return(picture_image);
    }
  flop_image=FlopImage(polaroid_image,exception);
  polaroid_image=DestroyImage(polaroid_image);
  if (flop_image == (Image *) NULL)
    {
      picture_image=DestroyImage(picture_image);
      return(picture_image);
    }
  polaroid_image=flop_image;
  (void) CompositeImage(polaroid_image,OverCompositeOp,picture_image,
    (long) (-0.01*picture_image->columns/2.0),0L);
  picture_image=DestroyImage(picture_image);
  (void) QueryColorDatabase("none",&polaroid_image->background_color,exception);
  rotate_image=RotateImage(polaroid_image,angle,exception);
  polaroid_image=DestroyImage(polaroid_image);
  if (rotate_image == (Image *) NULL)
    return((Image *) NULL);
  polaroid_image=rotate_image;
  trim_image=TrimImage(polaroid_image,exception);
  polaroid_image=DestroyImage(polaroid_image);
  if (trim_image == (Image *) NULL)
    return((Image *) NULL);
  polaroid_image=trim_image;
  return(polaroid_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     R e c o l o r I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RecolorImage() translate, scale, shear, or rotate image colors.  Although
%  you can use variable sized matrices, typically you use a 5 x 5 for an RGBA
%  image and a 6x6 for CMYKA.  Populate the last row with normalized values to
%  translate.
%
%  The format of the RecolorImage method is:
%
%      Image *RecolorImage(const Image *image,const unsigned long order,
%        const double *color_matrix,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o order: The number of columns and rows in the recolor matrix.
%
%    o color_matrix: An array of double representing the recolor matrix.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *RecolorImage(const Image *image,const unsigned long order,
  const double *color_matrix,ExceptionInfo *exception)
{
#define RecolorImageTag  "Recolor/Image"

  Image
    *recolor_image;

  IndexPacket
    *indexes,
    *recolor_indexes;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel,
    recolor_pixel;

  register const double
    *k;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Initialize image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  recolor_image=CloneImage(image,0,0,MagickTrue,exception);
  if (recolor_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(recolor_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&recolor_image->exception);
      recolor_image=DestroyImage(recolor_image);
      return((Image *) NULL);
    }
  if (image->debug != MagickFalse)
    {
      char
        format[MaxTextExtent],
        *message;

      long
        u,
        v;

      (void) LogMagickEvent(TransformEvent,GetMagickModule(),
        "  Recolor image with %ldx%ld color matrix:",order,order);
      message=AcquireString("");
      k=color_matrix;
      for (v=0; v < (long) order; v++)
      {
        *message='\0';
        (void) FormatMagickString(format,MaxTextExtent,"%ld: ",v);
        (void) ConcatenateString(&message,format);
        for (u=0; u < (long) order; u++)
        {
          (void) FormatMagickString(format,MaxTextExtent,"%+f ",*k++);
          (void) ConcatenateString(&message,format);
        }
        (void) LogMagickEvent(TransformEvent,GetMagickModule(),"%s",message);
      }
      message=DestroyString(message);
    }
  /*
    Recolor image.
  */
  GetMagickPixelPacket(image,&pixel);
  GetMagickPixelPacket(image,&recolor_pixel);
  k=color_matrix;
  for (y=0; y < (long) recolor_image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=GetImagePixels(recolor_image,0,y,recolor_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    recolor_indexes=GetIndexes(recolor_image);
    for (x=0; x < (long) recolor_image->columns; x++)
    {
      SetMagickPixelPacket(image,p,indexes,&pixel);
      SetMagickPixelPacket(image,p,indexes,&recolor_pixel);
      switch (order)
      {
        case 0:
          break;
        case 1:
        {
          recolor_pixel.red=k[0]*pixel.red;
          break;
        }
        case 2:
        {
          recolor_pixel.red=k[0]*pixel.red+k[1]*pixel.green;
          recolor_pixel.green=k[2]*pixel.red+k[3]*pixel.green;
          break;
        }
        case 3:
        {
          recolor_pixel.red=k[0]*pixel.red+k[1]*pixel.green+k[2]*pixel.blue;
          recolor_pixel.green=k[3]*pixel.red+k[4]*pixel.green+k[5]*pixel.blue;
          recolor_pixel.blue=k[6]*pixel.red+k[7]*pixel.green+k[8]*pixel.blue;
          break;
        }
        case 4:
        {
          recolor_pixel.red=k[0]*pixel.red+k[1]*pixel.green+k[2]*pixel.blue+
            k[12]*QuantumRange;
          recolor_pixel.green=k[4]*pixel.red+k[5]*pixel.green+k[6]*pixel.blue+
            k[13]*QuantumRange;
          recolor_pixel.blue=k[8]*pixel.red+k[9]*pixel.green+k[10]*pixel.blue+
            k[14]*QuantumRange;
          break;
        }
        case 5:
        {
          recolor_pixel.red=k[0]*pixel.red+k[1]*pixel.green+k[2]*pixel.blue+
            k[3]*(QuantumRange-pixel.opacity)+k[20]*QuantumRange;
          recolor_pixel.green=k[5]*pixel.red+k[6]*pixel.green+k[7]*pixel.blue+
            k[8]*(QuantumRange-pixel.opacity)+k[21]*QuantumRange;
          recolor_pixel.blue=k[10]*pixel.red+k[11]*pixel.green+k[12]*pixel.blue+
            k[13]*(QuantumRange-pixel.opacity)+k[22]*QuantumRange;
          recolor_pixel.opacity=(MagickRealType) QuantumRange-k[15]*pixel.red+
            k[16]*pixel.green+k[17]*pixel.blue+k[18]*(QuantumRange-
            pixel.opacity)+k[23]*QuantumRange;
          break;
        }
        default:
        {
          recolor_pixel.red=k[0]*pixel.red+k[1]*pixel.green+k[2]*pixel.blue+
            k[3]*pixel.index+k[4]*((Quantum) QuantumRange-pixel.opacity)+
            k[30]*QuantumRange;
          recolor_pixel.green=k[6]*pixel.red+k[7]*pixel.green+k[8]*pixel.blue+
            k[9]*pixel.index+k[10]*((Quantum) QuantumRange-pixel.opacity)+
            k[31]*QuantumRange;
          recolor_pixel.blue=k[12]*pixel.red+k[13]*pixel.green+k[14]*pixel.blue+
            k[15]*pixel.index+k[16]*((Quantum) QuantumRange-pixel.opacity)+
            k[32]*QuantumRange;
          if (image->colorspace == CMYKColorspace)
            recolor_pixel.index=k[18]*pixel.red+k[19]*pixel.green+k[20]*
              pixel.blue+k[21]*pixel.index+k[22]*((Quantum) QuantumRange-
              pixel.opacity)+k[33]*QuantumRange;
          recolor_pixel.opacity=(MagickRealType) QuantumRange-k[24]*pixel.red+
            k[25]*pixel.green+k[26]*pixel.blue+k[27]*pixel.index+k[28]*
            (QuantumRange-pixel.opacity)+k[34]*QuantumRange;
          break;
        }
      }
      q->red=RoundToQuantum(recolor_pixel.red);
      q->green=RoundToQuantum(recolor_pixel.green);
      q->blue=RoundToQuantum(recolor_pixel.blue);
      q->opacity=RoundToQuantum(recolor_pixel.opacity);
      if (image->colorspace == CMYKColorspace)
        recolor_indexes[x]=RoundToQuantum(recolor_pixel.index);
      p++;
      q++;
    }
    if (SyncImagePixels(recolor_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(RecolorImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(recolor_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S e p i a T o n e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSepiaToneImage() applies a special effect to the image, similar to the
%  effect achieved in a photo darkroom by sepia toning.  Threshold ranges from
%  0 to QuantumRange and is a measure of the extent of the sepia toning.  A
%  threshold of 80% is a good starting point for a reasonable tone.
%
%  The format of the SepiaToneImage method is:
%
%      Image *SepiaToneImage(const Image *image,const double threshold,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o threshold: The tone threshold.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *SepiaToneImage(const Image *image,const double threshold,
  ExceptionInfo *exception)
{
#define SepiaToneImageTag  "SepiaTone/Image"

  Image
    *sepia_image;

  long
    y;

  MagickBooleanType
    status;

  MagickRealType
    intensity,
    tone;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Initialize sepia-toned image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  sepia_image=CloneImage(image,image->columns,image->rows,MagickTrue,exception);
  if (sepia_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(sepia_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&sepia_image->exception);
      sepia_image=DestroyImage(sepia_image);
      return((Image *) NULL);
    }
  /*
    Tone each row of the image.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=SetImagePixels(sepia_image,0,y,sepia_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      intensity=(MagickRealType) PixelIntensityToQuantum(p);
      tone=intensity > threshold ? (MagickRealType) QuantumRange :
        intensity+(MagickRealType) QuantumRange-threshold;
      q->red=RoundToQuantum(tone);
      tone=intensity > (7.0*threshold/6.0) ? (MagickRealType) QuantumRange :
        intensity+(MagickRealType) QuantumRange-7.0*threshold/6.0;
      q->green=RoundToQuantum(tone);
      tone=intensity < (threshold/6.0) ? 0 : intensity-threshold/6.0;
      q->blue=RoundToQuantum(tone);
      tone=threshold/7.0;
      if ((MagickRealType) q->green < tone)
        q->green=RoundToQuantum(tone);
      if ((MagickRealType) q->blue < tone)
        q->blue=RoundToQuantum(tone);
      p++;
      q++;
    }
    if (SyncImagePixels(sepia_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SepiaToneImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  (void) NormalizeImage(sepia_image);
  (void) ContrastImage(sepia_image,MagickTrue);
  return(sepia_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S h a d o w I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ShadowImage() simulates a shadow from the specified image and returns it.
%
%  The format of the ShadowImage method is:
%
%      Image *ShadowImage(const Image *image,const double opacity,
%        const double sigma,const long x_offset,const long y_offset,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o opacity: percentage transparency.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
%    o x_offset: the shadow x-offset.
%
%    o y_offset: the shadow y-offset.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *ShadowImage(const Image *image,const double opacity,
  const double sigma,const long x_offset,const long y_offset,
  ExceptionInfo *exception)
{
  Image
    *border_image,
    *clone_image,
    *shadow_image;

  long
    x;

  RectangleInfo
    border_info;

  register long
    y;

  register PixelPacket
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  clone_image=CloneImage(image,0,0,MagickTrue,exception);
  if (clone_image == (Image *) NULL)
    return((Image *) NULL);
  (void) SetImageVirtualPixelMethod(clone_image,EdgeVirtualPixelMethod);
  border_info.width=(unsigned long) (2.0*sigma+0.5);
  border_info.height=(unsigned long) (2.0*sigma+0.5);
  border_info.x=0;
  border_info.y=0;
  (void) QueryColorDatabase("none",&clone_image->border_color,exception);
  border_image=BorderImage(clone_image,&border_info,exception);
  clone_image=DestroyImage(clone_image);
  if (border_image == (Image *) NULL)
    return((Image *) NULL);
  if (border_image->matte == MagickFalse)
    (void) SetImageOpacity(border_image,OpaqueOpacity);
  for (y=0; y < (long) border_image->rows; y++)
  {
    q=GetImagePixels(border_image,0,y,border_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < (long) border_image->columns; x++)
    {
      q->red=border_image->background_color.red;
      q->green=border_image->background_color.green;
      q->blue=border_image->background_color.blue;
      if (border_image->matte == MagickFalse)
        q->opacity=border_image->background_color.opacity;
      else
        q->opacity=RoundToQuantum((MagickRealType) QuantumRange-(QuantumRange-
          q->opacity)*opacity/100.0);
      q++;
    }
    if (SyncImagePixels(border_image) == MagickFalse)
      break;
  }
  shadow_image=BlurImageChannel(border_image,AlphaChannel,0.0,sigma,exception);
  border_image=DestroyImage(border_image);
  if (shadow_image == (Image *) NULL)
    return((Image *) NULL);
  if (shadow_image->page.width == 0)
    shadow_image->page.width=shadow_image->columns;
  if (shadow_image->page.height == 0)
    shadow_image->page.height=shadow_image->rows;
  shadow_image->page.width+=x_offset-(long) border_info.width;
  shadow_image->page.height+=y_offset-(long) border_info.height;
  shadow_image->page.x+=x_offset-(long) border_info.width;
  shadow_image->page.y+=y_offset-(long) border_info.height;
  return(shadow_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S k e t c h I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SketchImage() simulates a pencil sketch.  We convolve the image with a
%  Gaussian operator of the given radius and standard deviation (sigma).  For
%  reasonable results, radius should be larger than sigma.  Use a radius of 0
%  and SketchImage() selects a suitable radius for you.  Angle gives the angle
%  of the sketch.
%
%  The format of the SketchImage method is:
%
%    Image *SketchImage(const Image *image,const double radius,
%      const double sigma,const double angle,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o radius: The radius of the Gaussian, in pixels, not counting
%      the center pixel.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
%    o angle: Apply the effect along this angle.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *SketchImage(const Image *image,const double radius,
  const double sigma,const double angle,ExceptionInfo *exception)
{
  Image
    *blend_image,
    *blur_image,
    *dodge_image,
    *random_image,
    *sketch_image;

  long
    y;

  MagickPixelPacket
    pixel;

  register long
    x;

  register IndexPacket
    *indexes;

  register PixelPacket
    *q;

  /*
    Sketch image.
  */
  random_image=CloneImage(image,image->columns << 1,image->rows << 1,
    MagickTrue,exception);
  if (random_image == (Image *) NULL)
    return((Image *) NULL);
  GetMagickPixelPacket(random_image,&pixel);
  for (y=0; y < (long) random_image->rows; y++)
  {
    q=SetImagePixels(random_image,0,y,random_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(random_image);
    for (x=0; x < (long) random_image->columns; x++)
    {
      pixel.red=(MagickRealType) (QuantumRange*GetRandomValue());
      pixel.green=pixel.red;
      pixel.blue=pixel.red;
      if (image->colorspace == CMYKColorspace)
        pixel.index=pixel.red;
      SetPixelPacket(random_image,&pixel,q,indexes+x);
      q++;
    }
    if (SyncImagePixels(random_image) == MagickFalse)
      break;
  }
  blur_image=MotionBlurImage(random_image,radius,sigma,angle,exception);
  random_image=DestroyImage(random_image);
  if (blur_image == (Image *) NULL)
    return((Image *) NULL);
  dodge_image=EdgeImage(blur_image,radius,exception);
  blur_image=DestroyImage(blur_image);
  if (dodge_image == (Image *) NULL)
    return((Image *) NULL);
  (void) NormalizeImage(dodge_image);
  (void) NegateImage(dodge_image,MagickFalse);
  (void) TransformImage(&dodge_image,(char *) NULL,"50%");
  sketch_image=CloneImage(image,0,0,MagickTrue,exception);
  if (sketch_image == (Image *) NULL)
    {
      dodge_image=DestroyImage(dodge_image);
      return((Image *) NULL);
    }
  (void) CompositeImage(sketch_image,ColorDodgeCompositeOp,dodge_image,0,0);
  dodge_image=DestroyImage(dodge_image);
  blend_image=CloneImage(image,0,0,MagickTrue,exception);
  if (blend_image == (Image *) NULL)
    {
      sketch_image=DestroyImage(sketch_image);
      return((Image *) NULL);
    }
  blend_image->geometry=AcquireString("20x80");
  (void) CompositeImage(sketch_image,BlendCompositeOp,blend_image,0,0);
  blend_image=DestroyImage(blend_image);
  return(sketch_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S o l a r i z e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SolarizeImage() applies a special effect to the image, similar to the effect
%  achieved in a photo darkroom by selectively exposing areas of photo
%  sensitive paper to light.  Threshold ranges from 0 to QuantumRange and is a
%  measure of the extent of the solarization.
%
%  The format of the SolarizeImage method is:
%
%      MagickBooleanType SolarizeImage(Image *image,const double threshold)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o threshold:  Define the extent of the solarization.
%
*/
MagickExport MagickBooleanType SolarizeImage(Image *image,
  const double threshold)
{
#define SolarizeImageTag  "Solarize/Image"

  long
    y;

  MagickBooleanType
    status;

  register long
    i,
    x;

  register PixelPacket
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->storage_class == PseudoClass)
    {
      /*
        Solarize colormap.
      */
      for (i=0; i < (long) image->colors; i++)
      {
        if ((MagickRealType) image->colormap[i].red > threshold)
          image->colormap[i].red=(Quantum) QuantumRange-image->colormap[i].red;
        if ((MagickRealType) image->colormap[i].green > threshold)
          image->colormap[i].green=(Quantum) QuantumRange-
            image->colormap[i].green;
        if ((MagickRealType) image->colormap[i].blue > threshold)
          image->colormap[i].blue=(Quantum) QuantumRange-
            image->colormap[i].blue;
      }
    }
  /*
    Solarize image.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      if ((MagickRealType) q->red > threshold)
        q->red=(Quantum) QuantumRange-q->red;
      if ((MagickRealType) q->green > threshold)
        q->green=(Quantum) QuantumRange-q->green;
      if ((MagickRealType) q->blue > threshold)
        q->blue=(Quantum) QuantumRange-q->blue;
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SolarizeImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S t e g a n o I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SteganoImage() hides a digital watermark within the image.  Recover
%  the hidden watermark later to prove that the authenticity of an image.
%  Offset defines the start position within the image to hide the watermark.
%
%  The format of the SteganoImage method is:
%
%      Image *SteganoImage(const Image *image,Image *watermark,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o watermark: The watermark image.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *SteganoImage(const Image *image,const Image *watermark,
  ExceptionInfo *exception)
{
#define GetBit(alpha,i) ((((unsigned long) (alpha) >> (unsigned long) \
  (i)) & 0x01) != 0)
#define SetBit(alpha,i,set) (alpha)=(Quantum) ((set) ? (unsigned long) (alpha) \
  | (1UL << (unsigned long) (i)) : (unsigned long) (alpha) & \
  ~(1UL << (unsigned long) (i)))
#define SteganoImageTag  "Stegano/Image"

  Image
    *stegano_image;

  int
    c;

  long
    i,
    j,
    k,
    y;

  MagickBooleanType
    status;

  PixelPacket
    pixel;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Initialize steganographic image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(watermark != (const Image *) NULL);
  assert(watermark->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  stegano_image=CloneImage(image,0,0,MagickTrue,exception);
  if (stegano_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(stegano_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&stegano_image->exception);
      stegano_image=DestroyImage(stegano_image);
      return((Image *) NULL);
    }
  stegano_image->depth=QuantumDepth;
  /*
    Hide watermark in low-order bits of image.
  */
  c=0;
  i=0;
  j=0;
  k=image->offset;
  for (i=QuantumDepth-1; (i >= 0) && (j < QuantumDepth); i--)
  {
    for (y=0; (y < (long) watermark->rows) && (j < QuantumDepth); y++)
    {
      for (x=0; (x < (long) watermark->columns) && (j < QuantumDepth); x++)
      {
        pixel=AcquireOnePixel(watermark,x,y,exception);
        q=GetImagePixels(stegano_image,k % (long) stegano_image->columns,
          k/(long) stegano_image->columns,1,1);
        if (q == (PixelPacket *) NULL)
          break;
        switch (c)
        {
          case 0:
          {
            SetBit(q->red,j,GetBit(PixelIntensityToQuantum(&pixel),i));
            break;
          }
          case 1:
          {
            SetBit(q->green,j,GetBit(PixelIntensityToQuantum(&pixel),i));
            break;
          }
          case 2:
          {
            SetBit(q->blue,j,GetBit(PixelIntensityToQuantum(&pixel),i));
            break;
          }
        }
        if (SyncImagePixels(stegano_image) == MagickFalse)
          break;
        c++;
        if (c == 3)
          c=0;
        k++;
        if (k == (long) (stegano_image->columns*stegano_image->columns))
          k=0;
        if (k == image->offset)
          j++;
      }
    }
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(QuantumDepth-i,QuantumDepth) != MagickFalse))
      {
        status=image->progress_monitor(SteganoImageTag,QuantumDepth-i,
          QuantumDepth,image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  if (stegano_image->storage_class == PseudoClass)
    (void) SyncImage(stegano_image);
  return(stegano_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S t e r e o I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StereoImage() combines two images and produces a single image that is the
%  composite of a left and right image of a stereo pair.  Special red-green
%  stereo glasses are required to view this effect.
%
%  The format of the StereoImage method is:
%
%      Image *StereoImage(const Image *image,const Image *offset_image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o offset_image: Another image.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *StereoImage(const Image *image,const Image *offset_image,
  ExceptionInfo *exception)
{
#define StereoImageTag  "Stereo/Image"

  Image
    *stereo_image;

  long
    y;

  MagickBooleanType
    status;

  register const PixelPacket
    *p,
    *q;

  register long
    x;

  register PixelPacket
    *r;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  assert(offset_image != (const Image *) NULL);
  if ((image->columns != offset_image->columns) ||
      (image->rows != offset_image->rows))
    ThrowImageException(ImageError,"LeftAndRightImageSizesDiffer");
  /*
    Initialize stereo image attributes.
  */
  stereo_image=CloneImage(image,0,0,MagickTrue,exception);
  if (stereo_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(stereo_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&stereo_image->exception);
      stereo_image=DestroyImage(stereo_image);
      return((Image *) NULL);
    }
  /*
    Copy left image to red channel and right image to blue channel.
  */
  for (y=0; y < (long) stereo_image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=AcquireImagePixels(offset_image,0,y,offset_image->columns,1,exception);
    r=GetImagePixels(stereo_image,0,y,stereo_image->columns,1);
    if ((p == (PixelPacket *) NULL) || (q == (PixelPacket *) NULL) ||
        (r == (PixelPacket *) NULL))
      break;
    for (x=0; x < (long) stereo_image->columns; x++)
    {
      r->red=p->red;
      r->green=q->green;
      r->blue=q->blue;
      r->opacity=(Quantum) ((p->opacity+q->opacity)/2);
      p++;
      q++;
      r++;
    }
    if (SyncImagePixels(stereo_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(StereoImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(stereo_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S w i r l I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SwirlImage() swirls the pixels about the center of the image, where
%  degrees indicates the sweep of the arc through which each pixel is moved.
%  You get a more dramatic effect as the degrees move from 1 to 360.
%
%  The format of the SwirlImage method is:
%
%      Image *SwirlImage(const Image *image,double degrees,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o degrees: Define the tightness of the swirling effect.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *SwirlImage(const Image *image,double degrees,
  ExceptionInfo *exception)
{
#define SwirlImageTag  "Swirl/Image"

  Image
    *swirl_image;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MagickRealType
    cosine,
    distance,
    factor,
    radius,
    sine;

  PointInfo
    center,
    delta,
    scale;

  register const IndexPacket
    *indexes;

  register const PixelPacket
    *p;

  register IndexPacket
    *swirl_indexes;

  register PixelPacket
    *q;

  register long
    x;

  ResampleFilter
    *resample_filter;

  ViewInfo
    *image_view,
    *swirl_view;

  /*
    Initialize swirl image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  swirl_image=CloneImage(image,0,0,MagickTrue,exception);
  if (swirl_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(swirl_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&swirl_image->exception);
      swirl_image=DestroyImage(swirl_image);
      return((Image *) NULL);
    }
  if (swirl_image->background_color.opacity != OpaqueOpacity)
    swirl_image->matte=MagickTrue;
  /*
    Compute scaling factor.
  */
  center.x=(double) image->columns/2.0;
  center.y=(double) image->rows/2.0;
  radius=MagickMax(center.x,center.y);
  scale.x=1.0;
  scale.y=1.0;
  if (image->columns > image->rows)
    scale.y=(double) image->columns/(double) image->rows;
  else
    if (image->columns < image->rows)
      scale.x=(double) image->rows/(double) image->columns;
  degrees=(double) DegreesToRadians(degrees);
  /*
    Swirl image.
  */
  GetMagickPixelPacket(swirl_image,&pixel);
  resample_filter=AcquireResampleFilter(image,exception);
  image_view=OpenCacheView(image);
  swirl_view=OpenCacheView(swirl_image);
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetCacheViewPixels(swirl_view,0,y,swirl_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    swirl_indexes=GetCacheViewIndexes(swirl_view);
    delta.y=scale.y*(double) (y-center.y);
    for (x=0; x < (long) image->columns; x++)
    {
      /*
        Determine if the pixel is within an ellipse.
      */
      delta.x=scale.x*(double) (x-center.x);
      distance=delta.x*delta.x+delta.y*delta.y;
      if (distance >= (radius*radius))
        {
          p=AcquireCacheViewPixels(image_view,x,y,1,1,exception);
          if (p == (const PixelPacket *) NULL)
            break;
          indexes=AcquireCacheViewIndexes(image_view);
          SetMagickPixelPacket(image,p,indexes,&pixel);
        }
      else
        {
          /*
            Swirl the pixel.
          */
          factor=1.0-sqrt((double) distance)/radius;
          sine=sin((double) (degrees*factor*factor));
          cosine=cos((double) (degrees*factor*factor));
          pixel=ResamplePixelColor(resample_filter,(double) ((cosine*delta.x-
            sine*delta.y)/scale.x+center.x),(double) ((sine*delta.x+cosine*
            delta.y)/scale.y+center.y));
        }
      SetPixelPacket(swirl_image,&pixel,q,swirl_indexes+x);
      q++;
    }
    if (SyncCacheView(swirl_view) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SwirlImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  resample_filter=DestroyResampleFilter(resample_filter);
  swirl_view=CloseCacheView(swirl_view);
  image_view=CloseCacheView(image_view);
  return(swirl_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     T i n t I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TintImage() applies a color vector to each pixel in the image.  The length
%  of the vector is 0 for black and white and at its maximum for the midtones.
%  The vector weighting function is f(x)=(1-(4.0*((x-0.5)*(x-0.5))))
%
%  The format of the TintImage method is:
%
%      Image *TintImage(const Image *image,const char *opacity,
%        const PixelPacket tint,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o opacity: A color value used for tinting.
%
%    o tint: A color value used for tinting.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *TintImage(const Image *image,const char *opacity,
  const PixelPacket tint,ExceptionInfo *exception)
{
#define TintImageTag  "Tint/Image"

  GeometryInfo
    geometry_info;

  Image
    *tint_image;

  long
    y;

  MagickBooleanType
    status;

  MagickStatusType
    flags;

  MagickPixelPacket
    color_vector,
    pixel;

  MagickRealType
    weight;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Allocate tint image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  tint_image=CloneImage(image,0,0,MagickTrue,exception);
  if (tint_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(tint_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&tint_image->exception);
      tint_image=DestroyImage(tint_image);
      return((Image *) NULL);
    }
  if (opacity == (const char *) NULL)
    return(tint_image);
  /*
    Determine RGB values of the color.
  */
  flags=ParseGeometry(opacity,&geometry_info);
  pixel.red=geometry_info.rho;
  if ((flags & SigmaValue) != 0)
    pixel.green=geometry_info.sigma;
  else
    pixel.green=pixel.red;
  if ((flags & XiValue) != 0)
    pixel.blue=geometry_info.xi;
  else
    pixel.blue=pixel.red;
  if ((flags & PsiValue) != 0)
    pixel.opacity=geometry_info.psi;
  else
    pixel.opacity=(MagickRealType) OpaqueOpacity;
  color_vector.red=(MagickRealType) (pixel.red*
    tint.red/100.0-PixelIntensity(&tint));
  color_vector.green=(MagickRealType) (pixel.green*
    tint.green/100.0-PixelIntensity(&tint));
  color_vector.blue=(MagickRealType) (pixel.blue*
    tint.blue/100.0-PixelIntensity(&tint));
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=GetImagePixels(tint_image,0,y,tint_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      weight=QuantumScale*p->red-0.5;
      pixel.red=(MagickRealType)
        p->red+color_vector.red*(1.0-(4.0*(weight*weight)));
      q->red=RoundToQuantum(pixel.red);
      weight=QuantumScale*p->green-0.5;
      pixel.green=(MagickRealType) p->green+color_vector.green*
        (1.0-(4.0*(weight*weight)));
      q->green=RoundToQuantum(pixel.green);
      weight=QuantumScale*p->blue-0.5;
      pixel.blue=(MagickRealType) p->blue+color_vector.blue*(1.0-
        (4.0*(weight*weight)));
      q->blue=RoundToQuantum(pixel.blue);
      q->opacity=p->opacity;
      p++;
      q++;
    }
    if (SyncImagePixels(tint_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(TintImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(tint_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     V i g n e t t e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  VignetteImage() softens the edges of the image in vignette style.
%
%  The format of the VignetteImage method is:
%
%      Image *VignetteImage(const Image *image,const double radius,
%        const double sigma,const long x,const long y,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o radius: the radius of the pixel neighborhood.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
%    o x, y:  Define the x and y ellipse offset.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *VignetteImage(const Image *image,const double radius,
  const double sigma,const long x,const long y,ExceptionInfo *exception)
{
  char
    ellipse[MaxTextExtent];

  DrawInfo
    *draw_info;

  Image
    *canvas_image,
    *blur_image,
    *oval_image,
    *vignette_image;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  canvas_image=CloneImage(image,0,0,MagickTrue,exception);
  if (canvas_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(canvas_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&canvas_image->exception);
      canvas_image=DestroyImage(canvas_image);
      return((Image *) NULL);
    }
  canvas_image->matte=MagickTrue;
  oval_image=CloneImage(canvas_image,canvas_image->columns,
    canvas_image->rows,MagickTrue,exception);
  if (oval_image == (Image *) NULL)
    {
      canvas_image=DestroyImage(canvas_image);
      return((Image *) NULL);
    }
  (void) QueryColorDatabase("black",&oval_image->background_color,exception);
  (void) SetImageBackgroundColor(oval_image);
  draw_info=CloneDrawInfo((const ImageInfo *) NULL,(const DrawInfo *) NULL);
  (void) QueryColorDatabase("white",&draw_info->fill,exception);
  (void) QueryColorDatabase("white",&draw_info->stroke,exception);
  (void) FormatMagickString(ellipse,MaxTextExtent,
    "ellipse %g,%g,%g,%g,0.0,360.0",image->columns/2.0,image->rows/2.0,
    image->columns/2.0-x,image->rows/2.0-y);
  draw_info->primitive=AcquireString(ellipse);
  (void) DrawImage(oval_image,draw_info);
  draw_info=DestroyDrawInfo(draw_info);
  blur_image=BlurImage(oval_image,radius,sigma,exception);
  oval_image=DestroyImage(oval_image);
  if (blur_image == (Image *) NULL)
    {
      canvas_image=DestroyImage(canvas_image);
      return((Image *) NULL);
    }
  blur_image->matte=MagickFalse;
  (void) CompositeImage(canvas_image,CopyOpacityCompositeOp,blur_image,0,0);
  blur_image=DestroyImage(blur_image);
  vignette_image=FlattenImages(canvas_image,exception);
  canvas_image=DestroyImage(canvas_image);
  return(vignette_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     W a v e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WaveImage() creates a "ripple" effect in the image by shifting the pixels
%  vertically along a sine wave whose amplitude and wavelength is specified
%  by the given parameters.
%
%  The format of the WaveImage method is:
%
%      Image *WaveImage(const Image *image,const double amplitude,
%        const double wave_length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o amplitude, wave_length:  Define the amplitude and wave length of the
%      sine wave.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *WaveImage(const Image *image,const double amplitude,
  const double wave_length,ExceptionInfo *exception)
{
#define WaveImageTag  "Wave/Image"

  Image
    *wave_image;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MagickRealType
    *sine_map;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  ResampleFilter
    *resample_filter;

  ViewInfo
    *wave_view;

  /*
    Initialize wave image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  wave_image=CloneImage(image,image->columns,(unsigned long) (image->rows+2.0*
    fabs(amplitude)),MagickTrue,exception);
  if (wave_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(wave_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&wave_image->exception);
      wave_image=DestroyImage(wave_image);
      return((Image *) NULL);
    }
  if (wave_image->background_color.opacity != OpaqueOpacity)
    wave_image->matte=MagickTrue;
  /*
    Allocate sine map.
  */
  sine_map=(MagickRealType *) AcquireQuantumMemory((size_t) wave_image->columns,
    sizeof(*sine_map));
  if (sine_map == (MagickRealType *) NULL)
    {
      wave_image=DestroyImage(wave_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  for (x=0; x < (long) wave_image->columns; x++)
    sine_map[x]=fabs(amplitude)+amplitude*sin((2*MagickPI*x)/wave_length);
  /*
    Wave image.
  */
  GetMagickPixelPacket(wave_image,&pixel);
  resample_filter=AcquireResampleFilter(image,exception);
  (void) SetResampleFilterVirtualPixelMethod(resample_filter,
    BackgroundVirtualPixelMethod);
  wave_view=OpenCacheView(wave_image);
  for (y=0; y < (long) wave_image->rows; y++)
  {
    q=SetCacheView(wave_view,0,y,wave_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetCacheViewIndexes(wave_view);
    for (x=0; x < (long) wave_image->columns; x++)
    {
      pixel=ResamplePixelColor(resample_filter,(double) x,(double) (y-
        sine_map[x]));
      SetPixelPacket(wave_image,&pixel,q,indexes+x);
      q++;
    }
    if (SyncCacheView(wave_view) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(WaveImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  resample_filter=DestroyResampleFilter(resample_filter);
  wave_view=CloseCacheView(wave_view);
  sine_map=(MagickRealType *) RelinquishMagickMemory(sine_map);
  return(wave_image);
}
