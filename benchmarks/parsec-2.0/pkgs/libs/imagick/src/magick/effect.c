/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                   EEEEE  FFFFF  FFFFF  EEEEE  CCCC  TTTTT                   %
%                   E      F      F      E     C        T                     %
%                   EEE    FFF    FFF    EEE   C        T                     %
%                   E      F      F      E     C        T                     %
%                   EEEEE  F      F      EEEEE  CCCC    T                     %
%                                                                             %
%                                                                             %
%                      ImageMagick Image Effects Methods                      %
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
#include "magick/property.h"
#include "magick/blob.h"
#include "magick/cache-view.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/constitute.h"
#include "magick/decorate.h"
#include "magick/draw.h"
#include "magick/enhance.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/effect.h"
#include "magick/fx.h"
#include "magick/gem.h"
#include "magick/geometry.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/montage.h"
#include "magick/pixel-private.h"
#include "magick/property.h"
#include "magick/quantize.h"
#include "magick/quantum.h"
#include "magick/random_.h"
#include "magick/resize.h"
#include "magick/resource_.h"
#include "magick/segment.h"
#include "magick/shear.h"
#include "magick/signature.h"
#include "magick/string_.h"
#include "magick/transform.h"
#include "magick/threshold.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A d a p t i v e B l u r I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AdaptiveBlurImage() adaptively blurs the image by blurring less
%  intensely near image edges and more intensely far from edges.  We blur the
%  image with a Gaussian operator of the given radius and standard deviation
%  (sigma).  For reasonable results, radius should be larger than sigma.  Use a
%  radius of 0 and AdaptiveBlurImage() selects a suitable radius for you.
%
%  The format of the AdaptiveBlurImage method is:
%
%      Image *AdaptiveBlurImage(const Image *image,const double radius,
%        const double sigma,ExceptionInfo *exception)
%      Image *AdaptiveBlurImageChannel(const Image *image,
%        const ChannelType channel,double radius,const double sigma,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel type.
%
%    o radius: The radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the Laplacian, in pixels.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

MagickExport Image *AdaptiveBlurImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  Image
    *blur_image;

  blur_image=AdaptiveBlurImageChannel(image,DefaultChannels,radius,sigma,
    exception);
  return(blur_image);
}

MagickExport Image *AdaptiveBlurImageChannel(const Image *image,
  const ChannelType channel,const double radius,const double sigma,
  ExceptionInfo *exception)
{
#define AdaptiveBlurImageTag  "Convolve/Image"

  double
    **kernel;

  Image
    *blur_image,
    *edge_image,
    *gaussian_image;

  IndexPacket
    *indexes,
    *blur_indexes;

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
    gamma,
    normalize;

  register const double
    *k;

  register const PixelPacket
    *p,
    *r;

  register long
    i,
    x;

  register PixelPacket
    *q;

  unsigned long
    width;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  blur_image=CloneImage(image,0,0,MagickTrue,exception);
  if (blur_image == (Image *) NULL)
    return((Image *) NULL);
  if (fabs(sigma) <= MagickEpsilon)
    return(blur_image);
  if (SetImageStorageClass(blur_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&blur_image->exception);
      blur_image=DestroyImage(blur_image);
      return((Image *) NULL);
    }
  /*
    Edge detect the image brighness channel, level, blur, and level again.
  */
  edge_image=EdgeImage(image,radius,exception);
  if (edge_image == (Image *) NULL)
    {
      blur_image=DestroyImage(blur_image);
      return((Image *) NULL);
    }
  (void) LevelImage(edge_image,"20%,95%");
  gaussian_image=GaussianBlurImage(edge_image,radius,sigma,exception);
  if (gaussian_image != (Image *) NULL)
    {
      edge_image=DestroyImage(edge_image);
      edge_image=gaussian_image;
    }
  (void) LevelImage(edge_image,"10%,95%");
  /*
    Create a set of kernels from maximum (radius,sigma) to minimum.
  */
  width=GetOptimalKernelWidth2D(radius,sigma);
  kernel=(double **) AcquireQuantumMemory((size_t) width,sizeof(*kernel));
  if (kernel == (double **) NULL)
    {
      edge_image=DestroyImage(edge_image);
      blur_image=DestroyImage(blur_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  (void) ResetMagickMemory(kernel,0,(size_t) width*sizeof(*kernel));
  for (i=0; i < (long) width; i+=2)
  {
    kernel[i]=(double *) AcquireQuantumMemory((size_t) (width-i),(width-i)*
      sizeof(**kernel));
    if (kernel[i] == (double *) NULL)
      break;
    j=0;
    normalize=0.0;
    for (v=(-((long) (width-i)/2)); v <= (long) ((width-i)/2); v++)
    {
      for (u=(-((long) (width-i)/2)); u <= (long) ((width-i)/2); u++)
      {
        alpha=exp(-((double) u*u+v*v)/(2.0*sigma*sigma));
        kernel[i][j]=(double) (alpha/(2.0*MagickPI*sigma*sigma));
        if (((width-i) < 3) || (u != 0) || (v != 0))
          normalize+=kernel[i][j];
        j++;
      }
    }
    kernel[i][j/2]=(double) ((-2.0)*normalize);
    normalize=0.0;
    for (j=0; j < (long) ((width-i)*(width-i)); j++)
      normalize+=kernel[i][j];
    if (fabs(normalize) <= MagickEpsilon)
      normalize=1.0;
    normalize=1.0/normalize;
    for (j=0; j < (long) ((width-i)*(width-i)); j++)
      kernel[i][j]=(double) (normalize*kernel[i][j]);
  }
  if (i < (long) width)
    {
      for (i-=2; i >= 0; i-=2)
        kernel[i]=(double *) RelinquishMagickMemory(kernel[i]);
      kernel=(double **) RelinquishMagickMemory(kernel);
      edge_image=DestroyImage(edge_image);
      blur_image=DestroyImage(blur_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Adaptively blur image.
  */
  for (y=0; y < (long) blur_image->rows; y++)
  {
    r=AcquireImagePixels(edge_image,0,y,edge_image->columns,1,exception);
    q=GetImagePixels(blur_image,0,y,blur_image->columns,1);
    if ((r == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    blur_indexes=GetIndexes(blur_image);
    for (x=0; x < (long) blur_image->columns; x++)
    {
      GetMagickPixelPacket(image,&pixel);
      gamma=0.0;
      i=(long) (width*QuantumScale*PixelIntensity(r)+0.5);
      if ((i & 0x01) != 0)
        i--;
      p=AcquireImagePixels(image,x-((long) (width-i)/2L),y-(long)
        ((width-i)/2L),width-i,width-i,exception);
      if (p == (const PixelPacket *) NULL)
        break;
      k=kernel[i];
      for (v=0; v < (long) (width-i); v++)
      {
        for (u=0; u < (long) (width-i); u++)
        {
          alpha=1.0;
          if (((channel & OpacityChannel) != 0) &&
              (image->matte != MagickFalse))
            alpha=QuantumScale*((MagickRealType) QuantumRange-p->opacity);
          if ((channel & RedChannel) != 0)
            pixel.red+=(*k)*alpha*p->red;
          if ((channel & GreenChannel) != 0)
            pixel.green+=(*k)*alpha*p->green;
          if ((channel & BlueChannel) != 0)
            pixel.blue+=(*k)*alpha*p->blue;
          if ((channel & OpacityChannel) != 0)
            pixel.opacity+=(*k)*p->opacity;
          if (((channel & IndexChannel) != 0) &&
              (image->colorspace == CMYKColorspace))
            pixel.index+=(*k)*alpha*indexes[x+(width-i)*v+u];
          gamma+=(*k)*alpha;
          k++;
          p++;
        }
      }
      gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
      if ((channel & RedChannel) != 0)
        q->red=RoundToQuantum(gamma*pixel.red+image->bias);
      if ((channel & GreenChannel) != 0)
        q->green=RoundToQuantum(gamma*pixel.green+image->bias);
      if ((channel & BlueChannel) != 0)
        q->blue=RoundToQuantum(gamma*pixel.blue+image->bias);
      if ((channel & OpacityChannel) != 0)
        q->opacity=RoundToQuantum(pixel.opacity+image->bias);
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        blur_indexes[x]=RoundToQuantum(gamma*pixel.index+image->bias);
      q++;
      r++;
    }
    if (SyncImagePixels(blur_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(AdaptiveBlurImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  edge_image=DestroyImage(edge_image);
  for (i=0; i < (long) width;  i+=2)
    kernel[i]=(double *) RelinquishMagickMemory(kernel[i]);
  kernel=(double **) RelinquishMagickMemory(kernel);
  return(blur_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A d a p t i v e S h a r p e n I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AdaptiveSharpenImage() adaptively sharpens the image by sharpening more
%  intensely near image edges and less intensely far from edges. We sharpen the
%  image with a Gaussian operator of the given radius and standard deviation
%  (sigma).  For reasonable results, radius should be larger than sigma.  Use a
%  radius of 0 and AdaptiveSharpenImage() selects a suitable radius for you.
%
%  The format of the AdaptiveSharpenImage method is:
%
%      Image *AdaptiveSharpenImage(const Image *image,const double radius,
%        const double sigma,ExceptionInfo *exception)
%      Image *AdaptiveSharpenImageChannel(const Image *image,
%        const ChannelType channel,double radius,const double sigma,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel type.
%
%    o radius: The radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the Laplacian, in pixels.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

MagickExport Image *AdaptiveSharpenImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  Image
    *sharp_image;

  sharp_image=AdaptiveSharpenImageChannel(image,DefaultChannels,radius,sigma,
    exception);
  return(sharp_image);
}

MagickExport Image *AdaptiveSharpenImageChannel(const Image *image,
  const ChannelType channel,const double radius,const double sigma,
  ExceptionInfo *exception)
{
#define AdaptiveSharpenImageTag  "Convolve/Image"

  double
    **kernel;

  Image
    *blur_image,
    *edge_image,
    *sharp_image;

  IndexPacket
    *indexes,
    *sharp_indexes;

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
    gamma,
    normalize;

  register const double
    *k;

  register const PixelPacket
    *p,
    *r;

  register long
    i,
    x;

  register PixelPacket
    *q;

  unsigned long
    width;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  sharp_image=CloneImage(image,0,0,MagickTrue,exception);
  if (sharp_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(sharp_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&sharp_image->exception);
      sharp_image=DestroyImage(sharp_image);
      return((Image *) NULL);
    }
  /*
    Edge detect the image brighness channel, level, blur, and level again.
  */
  edge_image=EdgeImage(image,radius,exception);
  if (edge_image == (Image *) NULL)
    {
      sharp_image=DestroyImage(sharp_image);
      return((Image *) NULL);
    }
  (void) LevelImage(edge_image,"20%,95%");
  blur_image=GaussianBlurImage(edge_image,radius,sigma,exception);
  if (blur_image != (Image *) NULL)
    {
      edge_image=DestroyImage(edge_image);
      edge_image=blur_image;
    }
  (void) LevelImage(edge_image,"10%,95%");
  /*
    Create a set of kernels from maximum (radius,sigma) to minimum.
  */
  width=GetOptimalKernelWidth2D(radius,sigma);
  kernel=(double **) AcquireQuantumMemory((size_t) width,sizeof(*kernel));
  if (kernel == (double **) NULL)
    {
      edge_image=DestroyImage(edge_image);
      sharp_image=DestroyImage(sharp_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  (void) ResetMagickMemory(kernel,0,(size_t) width*sizeof(*kernel));
  for (i=0; i < (long) width; i+=2)
  {
    kernel[i]=(double *) AcquireQuantumMemory((size_t) (width-i),(width-i)*
      sizeof(**kernel));
    if (kernel[i] == (double *) NULL)
      break;
    j=0;
    normalize=0.0;
    for (v=(-((long) (width-i)/2)); v <= (long) ((width-i)/2); v++)
    {
      for (u=(-((long) (width-i)/2)); u <= (long) ((width-i)/2); u++)
      {
        alpha=exp(-((double) u*u+v*v)/(2.0*sigma*sigma));
        kernel[i][j]=(double) (-alpha/(2.0*MagickPI*sigma*sigma));
        if (((width-i) < 3) || (u != 0) || (v != 0))
          normalize+=kernel[i][j];
        j++;
      }
    }
    kernel[i][j/2]=(double) ((-2.0)*normalize);
    normalize=0.0;
    for (j=0; j < (long) ((width-i)*(width-i)); j++)
      normalize+=kernel[i][j];
    if (fabs(normalize) <= MagickEpsilon)
      normalize=1.0;
    normalize=1.0/normalize;
    for (j=0; j < (long) ((width-i)*(width-i)); j++)
      kernel[i][j]=(double) (normalize*kernel[i][j]);
  }
  if (i < (long) width)
    {
      for (i-=2; i >= 0; i-=2)
        kernel[i]=(double *) RelinquishMagickMemory(kernel[i]);
      kernel=(double **) RelinquishMagickMemory(kernel);
      edge_image=DestroyImage(edge_image);
      sharp_image=DestroyImage(sharp_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Adaptively sharpen image.
  */
  for (y=0; y < (long) sharp_image->rows; y++)
  {
    r=AcquireImagePixels(edge_image,0,y,edge_image->columns,1,exception);
    q=GetImagePixels(sharp_image,0,y,sharp_image->columns,1);
    if ((r == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    sharp_indexes=GetIndexes(sharp_image);
    for (x=0; x < (long) sharp_image->columns; x++)
    {
      GetMagickPixelPacket(image,&pixel);
      gamma=0.0;
      i=(long) (width*QuantumScale*(QuantumRange-PixelIntensity(r))+0.5);
      if ((i & 0x01) != 0)
        i--;
      p=AcquireImagePixels(image,x-((long) (width-i)/2L),y-(long)
        ((width-i)/2L),width-i,width-i,exception);
      if (p == (const PixelPacket *) NULL)
        break;
      k=kernel[i];
      for (v=0; v < (long) (width-i); v++)
      {
        for (u=0; u < (long) (width-i); u++)
        {
          alpha=1.0;
          if (((channel & OpacityChannel) != 0) &&
              (image->matte != MagickFalse))
            alpha=QuantumScale*((MagickRealType) QuantumRange-p->opacity);
          if ((channel & RedChannel) != 0)
            pixel.red+=(*k)*alpha*p->red;
          if ((channel & GreenChannel) != 0)
            pixel.green+=(*k)*alpha*p->green;
          if ((channel & BlueChannel) != 0)
            pixel.blue+=(*k)*alpha*p->blue;
          if ((channel & OpacityChannel) != 0)
            pixel.opacity+=(*k)*p->opacity;
          if (((channel & IndexChannel) != 0) &&
              (image->colorspace == CMYKColorspace))
            pixel.index+=(*k)*alpha*indexes[x+(width-i)*v+u];
          gamma+=(*k)*alpha;
          k++;
          p++;
        }
      }
      gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
      if ((channel & RedChannel) != 0)
        q->red=RoundToQuantum(gamma*pixel.red+image->bias);
      if ((channel & GreenChannel) != 0)
        q->green=RoundToQuantum(gamma*pixel.green+image->bias);
      if ((channel & BlueChannel) != 0)
        q->blue=RoundToQuantum(gamma*pixel.blue+image->bias);
      if ((channel & OpacityChannel) != 0)
        q->opacity=RoundToQuantum(pixel.opacity+image->bias);
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        sharp_indexes[x]=RoundToQuantum(gamma*pixel.index+image->bias);
      q++;
      r++;
    }
    if (SyncImagePixels(sharp_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(AdaptiveSharpenImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  edge_image=DestroyImage(edge_image);
  for (i=0; i < (long) width;  i+=2)
    kernel[i]=(double *) RelinquishMagickMemory(kernel[i]);
  kernel=(double **) RelinquishMagickMemory(kernel);
  return(sharp_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A d d N o i s e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AddNoiseImage() adds random noise to the image.
%
%  The format of the AddNoiseImage method is:
%
%      Image *AddNoiseImage(const Image *image,const NoiseType noise_type,
%        ExceptionInfo *exception)
%      Image *AddNoiseImageChannel(const Image *image,const ChannelType channel,
%        const NoiseType noise_type,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel type.
%
%    o noise_type:  The type of noise: Uniform, Gaussian, Multiplicative,
%      Impulse, Laplacian, or Poisson.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

static Quantum GenerateNoise(const Quantum pixel,const NoiseType noise_type,
  const MagickRealType attenuate)
{
#define NoiseEpsilon  (attenuate*1.0e-5)
#define SigmaUniform  ScaleCharToQuantum((unsigned char) (attenuate*4.0+0.5))
#define SigmaGaussian  ScaleCharToQuantum((unsigned char) (attenuate*4.0+0.5))
#define SigmaImpulse  (attenuate*0.10)
#define SigmaLaplacian ScaleCharToQuantum((unsigned char) (attenuate*10.0+0.5))
#define SigmaMultiplicativeGaussian \
  ScaleCharToQuantum((unsigned char) (attenuate*1.0+0.5))
#define SigmaPoisson  (attenuate*0.05)
#define TauGaussian  ScaleCharToQuantum((unsigned char) (attenuate*20.0+0.5))

  MagickRealType
    alpha,
    beta,
    noise,
    sigma;

  alpha=GetRandomValue();
  if (alpha == 0.0)
    alpha=1.0;
  switch (noise_type)
  {
    case UniformNoise:
    default:
    {
      noise=(MagickRealType) pixel+SigmaUniform*(alpha-0.5);
      break;
    }
    case GaussianNoise:
    {
      MagickRealType
        tau;

      beta=GetRandomValue();
      sigma=sqrt(-2.0*log((double) alpha))*cos((double) (2.0*MagickPI*beta));
      tau=sqrt(-2.0*log((double) alpha))*sin((double) (2.0*MagickPI*beta));
      noise=(MagickRealType) pixel+sqrt((double) pixel)*SigmaGaussian*sigma+
        TauGaussian*tau;
      break;
    }
    case MultiplicativeGaussianNoise:
    {
      if (alpha <= NoiseEpsilon)
        sigma=(MagickRealType) QuantumRange;
      else
        sigma=sqrt(-2.0*log((double) alpha));
      beta=GetRandomValue();
      noise=(MagickRealType) pixel+pixel*SigmaMultiplicativeGaussian*sigma/2.0*
        cos((double) (2.0*MagickPI*beta));
      break;
    }
    case ImpulseNoise:
    {
      if (alpha < (SigmaImpulse/2.0))
        noise=0.0;
       else
         if (alpha >= (1.0-(SigmaImpulse/2.0)))
           noise=(MagickRealType) QuantumRange;
         else
           noise=(MagickRealType) pixel;
      break;
    }
    case LaplacianNoise:
    {
      if (alpha <= 0.5)
        {
          if (alpha <= NoiseEpsilon)
            noise=(MagickRealType) pixel-(MagickRealType) QuantumRange;
          else
            noise=(MagickRealType) pixel+ScaleCharToQuantum((unsigned char)
              (SigmaLaplacian*log((double) (2.0*alpha))+0.5));
          break;
        }
      beta=1.0-alpha;
      if (beta <= (0.5*NoiseEpsilon))
        noise=(MagickRealType) (pixel+QuantumRange);
      else
        noise=(MagickRealType) pixel-ScaleCharToQuantum((unsigned char)
          (SigmaLaplacian*log((double) (2.0*beta))+0.5));
      break;
    }
    case PoissonNoise:
    {
      MagickRealType
        poisson;

      register long
        i;

      poisson=exp(-SigmaPoisson*(double) ScaleQuantumToChar(pixel));
      for (i=0; alpha > poisson; i++)
      {
        beta=GetRandomValue();
        alpha=alpha*beta;
      }
      noise=(MagickRealType) ScaleCharToQuantum((unsigned char)
        (i/SigmaPoisson));
      break;
    }
    case RandomNoise:
    {
      noise=(MagickRealType) QuantumRange*GetRandomValue();
      break;
    }
  }
  return(RoundToQuantum(noise));
}

MagickExport Image *AddNoiseImage(const Image *image,const NoiseType noise_type,
  ExceptionInfo *exception)
{
  Image
    *noise_image;

  noise_image=AddNoiseImageChannel(image,DefaultChannels,noise_type,exception);
  return(noise_image);
}

MagickExport Image *AddNoiseImageChannel(const Image *image,
  const ChannelType channel,const NoiseType noise_type,ExceptionInfo *exception)
{
#define AddNoiseImageTag  "AddNoise/Image"

  const char
    *option;

  Image
    *noise_image;

  long
    y;

  MagickBooleanType
    status;

  MagickRealType
    attenuate;

  register const IndexPacket
    *indexes;

  register const PixelPacket
    *pixels;

  register IndexPacket
    *noise_indexes;

  register long
    x;

  register PixelPacket
    *noise_pixels;

  /*
    Initialize noise image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  noise_image=CloneImage(image,0,0,MagickTrue,exception);
  if (noise_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(noise_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&noise_image->exception);
      noise_image=DestroyImage(noise_image);
      return((Image *) NULL);
    }
  /*
    Add noise in each row.
  */
  attenuate=1.0;
  option=GetImageProperty(image,"attenuate");
  if (option != (char *) NULL)
    attenuate=atof(option);
  for (y=0; y < (long) image->rows; y++)
  {
    pixels=AcquireImagePixels(image,0,y,image->columns,1,exception);
    noise_pixels=GetImagePixels(noise_image,0,y,noise_image->columns,1);
    if ((pixels == (PixelPacket *) NULL) ||
        (noise_pixels == (PixelPacket *) NULL))
      break;
    indexes=AcquireIndexes(image);
    noise_indexes=GetIndexes(noise_image);
    for (x=0; x < (long) image->columns; x++)
    {
      if ((channel & RedChannel) != 0)
        noise_pixels[x].red=GenerateNoise(pixels[x].red,noise_type,attenuate);
      if ((channel & GreenChannel) != 0)
        noise_pixels[x].green=GenerateNoise(pixels[x].green,noise_type,
          attenuate);
      if ((channel & BlueChannel) != 0)
        noise_pixels[x].blue=GenerateNoise(pixels[x].blue,noise_type,attenuate);
      if ((channel & OpacityChannel) != 0)
        noise_pixels[x].opacity=GenerateNoise(pixels[x].opacity,noise_type,
          attenuate);
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        noise_indexes[x]=(IndexPacket) GenerateNoise(indexes[x],noise_type,
          attenuate);
    }
    if (SyncImagePixels(noise_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(AddNoiseImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(noise_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     B l u r I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BlurImage() blurs an image.  We convolve the image with a Gaussian operator
%  of the given radius and standard deviation (sigma).  For reasonable results,
%  the radius should be larger than sigma.  Use a radius of 0 and BlurImage()
%  selects a suitable radius for you.
%
%  BlurImage() differs from GaussianBlurImage() in that it uses a separable
%  kernel which is faster but mathematically equivalent to the non-separable
%  kernel.
%
%  The format of the BlurImage method is:
%
%      Image *BlurImage(const Image *image,const double radius,
%        const double sigma,ExceptionInfo *exception)
%      Image *BlurImageChannel(const Image *image,const ChannelType channel,
%        const double radius,const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel type.
%
%    o radius: The radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

MagickExport Image *BlurImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  Image
    *blur_image;

  blur_image=BlurImageChannel(image,DefaultChannels,radius,sigma,exception);
  return(blur_image);
}

static double *GetBlurKernel(unsigned long width,const MagickRealType sigma)
{
#define KernelRank 3

  double
    *kernel;

  long
    bias;

  MagickRealType
    alpha,
    normalize;

  register long
    i;

  /*
    Generate a 1-D convolution kernel.
  */
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  kernel=(double *) AcquireQuantumMemory((size_t) width,sizeof(*kernel));
  if (kernel == (double *) NULL)
    return(0);
  (void) ResetMagickMemory(kernel,0,(size_t) width*sizeof(*kernel));
  bias=KernelRank*(long) width/2;
  for (i=(-bias); i <= bias; i++)
  {
    alpha=exp((-((double) (i*i))/(double) (2.0*KernelRank*KernelRank*
      sigma*sigma)));
    kernel[(i+bias)/KernelRank]+=(double) (alpha/(MagickSQ2PI*sigma));
  }
  normalize=0.0;
  for (i=0; i < (long) width; i++)
    normalize+=kernel[i];
  for (i=0; i < (long) width; i++)
    kernel[i]/=normalize;
  return(kernel);
}

MagickExport Image *BlurImageChannel(const Image *image,
  const ChannelType channel,const double radius,const double sigma,
  ExceptionInfo *exception)
{
#define BlurImageTag  "Blur/Image"

  double
    *kernel;

  Image
    *blur_image;

  IndexPacket
    *blur_indexes;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MagickOffsetType
    offset;

  MagickRealType
    alpha,
    gamma;

  register const IndexPacket
    *indexes;

  register const PixelPacket
    *pixels;

  register const double
    *k;

  register long
    i,
    x;

  register PixelPacket
    *blur_pixels;

  unsigned long
    width;

  ViewInfo
    *image_view;

  /*
    Initialize blur image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  width=GetOptimalKernelWidth1D(radius,sigma);
  kernel=GetBlurKernel(width,sigma);
  if (kernel == (double *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  blur_image=CloneImage(image,0,0,MagickTrue,exception);
  if (blur_image == (Image *) NULL)
    return((Image *) NULL);
  if (fabs(sigma) <= MagickEpsilon)
    return(blur_image);
  if (SetImageStorageClass(blur_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&blur_image->exception);
      blur_image=DestroyImage(blur_image);
      return((Image *) NULL);
    }
  if (image->debug != MagickFalse)
    {
      char
        format[MaxTextExtent],
        *message;

      (void) LogMagickEvent(TransformEvent,GetMagickModule(),
        "  BlurImage with %ld kernel:",width);
      message=AcquireString("");
      k=kernel;
      for (i=0; i < (long) width; i++)
      {
        *message='\0';
        (void) FormatMagickString(format,MaxTextExtent,"%ld: ",i);
        (void) ConcatenateString(&message,format);
        (void) FormatMagickString(format,MaxTextExtent,"%g ",*k++);
        (void) ConcatenateString(&message,format);
        (void) LogMagickEvent(TransformEvent,GetMagickModule(),"%s",message);
      }
      message=DestroyString(message);
    }
  /*
    Blur rows.
  */
  for (y=0; y < (long) blur_image->rows; y++)
  {
    pixels=AcquireImagePixels(image,-((long) width/2L),y,image->columns+width,1,
      exception);
    blur_pixels=GetImagePixels(blur_image,0,y,blur_image->columns,1);
    if ((pixels == (const PixelPacket *) NULL) ||
        (blur_pixels == (PixelPacket *) NULL))
      break;
    indexes=AcquireIndexes(image);
    blur_indexes=GetIndexes(blur_image);
    #pragma omp parallel for private(alpha, gamma, i, k, pixel)
    for (x=0; x < (long) blur_image->columns; x++)
    {
      GetMagickPixelPacket(image,&pixel);
      gamma=0.0;
      k=kernel;
      for (i=0; i < (long) width; i++)
      {
        alpha=1.0;
        if (((channel & OpacityChannel) != 0) &&
            (image->matte != MagickFalse))
          alpha=((MagickRealType) QuantumRange-pixels[x+i].opacity)/
            (MagickRealType) QuantumRange;
        if ((channel & RedChannel) != 0)
          pixel.red+=(*k)*alpha*pixels[x+i].red;
        if ((channel & GreenChannel) != 0)
          pixel.green+=(*k)*alpha*pixels[x+i].green;
        if ((channel & BlueChannel) != 0)
          pixel.blue+=(*k)*alpha*pixels[x+i].blue;
        if ((channel & OpacityChannel) != 0)
          pixel.opacity+=(*k)*pixels[x+i].opacity;
        if (((channel & IndexChannel) != 0) &&
            (image->colorspace == CMYKColorspace))
          pixel.index+=(*k)*alpha*indexes[x+i];
        gamma+=(*k)*alpha;
        k++;
      }
      gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
      if ((channel & RedChannel) != 0)
        blur_pixels[x].red=RoundToQuantum(gamma*pixel.red+image->bias);
      if ((channel & GreenChannel) != 0)
        blur_pixels[x].green=RoundToQuantum(gamma*pixel.green+image->bias);
      if ((channel & BlueChannel) != 0)
        blur_pixels[x].blue=RoundToQuantum(gamma*pixel.blue+image->bias);
      if ((channel & OpacityChannel) != 0)
        blur_pixels[x].opacity=RoundToQuantum(pixel.opacity+image->bias);
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        blur_indexes[x]=RoundToQuantum(gamma*pixel.index+image->bias);
    }
    if (SyncImagePixels(blur_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows+image->columns) != MagickFalse))
      {
        status=image->progress_monitor(BlurImageTag,y,image->rows+
          image->columns,image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  /*
    Blur columns.
  */
  image_view=OpenCacheView(blur_image);
  for (x=0; x < (long) blur_image->columns; x++)
  {
    pixels=AcquireCacheViewPixels(image_view,x,-((long) width/2L),1,
      image->rows+width,exception);
    blur_pixels=GetImagePixels(blur_image,x,0,1,blur_image->rows);
    if ((pixels == (const PixelPacket *) NULL) ||
        (blur_pixels == (PixelPacket *) NULL))
      break;
    indexes=AcquireCacheViewIndexes(image_view);
    blur_indexes=GetIndexes(blur_image);
    #pragma omp parallel for private(alpha, gamma, i, k, pixel)
    for (y=0; y < (long) blur_image->rows; y++)
    {
      GetMagickPixelPacket(image,&pixel);
      gamma=0.0;
      k=kernel;
      for (i=0; i < (long) width; i++)
      {
        alpha=1.0;
        if (((channel & OpacityChannel) != 0) &&
            (image->matte != MagickFalse))
          alpha=((MagickRealType) QuantumRange-pixels[y+i].opacity)/
            (MagickRealType)QuantumRange;
        if ((channel & RedChannel) != 0)
          pixel.red+=(*k)*alpha*pixels[y+i].red;
        if ((channel & GreenChannel) != 0)
          pixel.green+=(*k)*alpha*pixels[y+i].green;
        if ((channel & BlueChannel) != 0)
          pixel.blue+=(*k)*alpha*pixels[y+i].blue;
        if ((channel & OpacityChannel) != 0)
          pixel.opacity+=(*k)*pixels[y+i].opacity;
        if (((channel & IndexChannel) != 0) &&
            (image->colorspace == CMYKColorspace))
          pixel.index+=(*k)*alpha*indexes[y+i];
        gamma+=(*k)*alpha;
        k++;
      }
      gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
      if ((channel & RedChannel) != 0)
        blur_pixels[y].red=RoundToQuantum(gamma*pixel.red+image->bias);
      if ((channel & GreenChannel) != 0)
        blur_pixels[y].green=RoundToQuantum(gamma*pixel.green+image->bias);
      if ((channel & BlueChannel) != 0)
        blur_pixels[y].blue=RoundToQuantum(gamma*pixel.blue+image->bias);
      if ((channel & OpacityChannel) != 0)
        blur_pixels[y].opacity=RoundToQuantum(pixel.opacity+image->bias);
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        blur_indexes[x]=RoundToQuantum(gamma*pixel.index+image->bias);
    }
    if (SyncImagePixels(blur_image) == MagickFalse)
      break;
    offset=(MagickOffsetType) image->rows+x;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(offset,image->rows+image->columns) != MagickFalse))
      {
        status=image->progress_monitor(BlurImageTag,offset,image->rows+
          image->columns,image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  image_view=CloseCacheView(image_view);
  kernel=(double *) RelinquishMagickMemory(kernel);
  return(blur_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     D e s p e c k l e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DespeckleImage() reduces the speckle noise in an image while perserving the
%  edges of the original image.
%
%  The format of the DespeckleImage method is:
%
%      Image *DespeckleImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

static void Hull(const long x_offset,const long y_offset,
  const unsigned long columns,const unsigned long rows,Quantum *f,Quantum *g,
  const int polarity)
{
  long
    y;

  MagickRealType
    v;

  register long
    x;

  register Quantum
    *p,
    *q,
    *r,
    *s;

  assert(f != (Quantum *) NULL);
  assert(g != (Quantum *) NULL);
  p=f+(columns+2);
  q=g+(columns+2);
  r=p+(y_offset*((long) columns+2)+x_offset);
  for (y=0; y < (long) rows; y++)
  {
    p++;
    q++;
    r++;
    if (polarity > 0)
      for (x=(long) columns; x != 0; x--)
      {
        v=(MagickRealType) (*p);
        if ((MagickRealType) *r >= (v+(MagickRealType) ScaleCharToQuantum(2)))
          v+=ScaleCharToQuantum(1);
        *q=(Quantum) v;
        p++;
        q++;
        r++;
      }
    else
      for (x=(long) columns; x != 0; x--)
      {
        v=(MagickRealType) (*p);
        if ((MagickRealType) *r <= (v-(MagickRealType) ScaleCharToQuantum(2)))
          v-=(long) ScaleCharToQuantum(1);
        *q=(Quantum) v;
        p++;
        q++;
        r++;
      }
    p++;
    q++;
    r++;
  }
  p=f+(columns+2);
  q=g+(columns+2);
  r=q+(y_offset*((long) columns+2)+x_offset);
  s=q-(y_offset*((long) columns+2)+x_offset);
  for (y=0; y < (long) rows; y++)
  {
    p++;
    q++;
    r++;
    s++;
    if (polarity > 0)
      for (x=(long) columns; x != 0; x--)
      {
        v=(MagickRealType) (*q);
        if (((MagickRealType) *s >=
             (v+(MagickRealType) ScaleCharToQuantum(2))) &&
            ((MagickRealType) *r > v))
          v+=ScaleCharToQuantum(1);
        *p=(Quantum) v;
        p++;
        q++;
        r++;
        s++;
      }
    else
      for (x=(long) columns; x != 0; x--)
      {
        v=(MagickRealType) (*q);
        if (((MagickRealType) *s <=
             (v-(MagickRealType) ScaleCharToQuantum(2))) &&
            ((MagickRealType) *r < v))
          v-=(MagickRealType) ScaleCharToQuantum(1);
        *p=(Quantum) v;
        p++;
        q++;
        r++;
        s++;
      }
    p++;
    q++;
    r++;
    s++;
  }
}

MagickExport Image *DespeckleImage(const Image *image,ExceptionInfo *exception)
{
#define DespeckleImageTag  "Despeckle/Image"

  Image
    *despeckle_image;

  int
    layer;

  long
    j,
    y;

  MagickBooleanType
    status;

  Quantum
    *buffer,
    *pixels;

  register const PixelPacket
    *p;

  register long
    i,
    x;

  register PixelPacket
    *q;

  size_t
    length;

  static const int
    X[4]= {0, 1, 1,-1},
    Y[4]= {1, 0, 1, 1};

  /*
    Allocate despeckled image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  despeckle_image=CloneImage(image,0,0,MagickTrue,exception);
  if (despeckle_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(despeckle_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&despeckle_image->exception);
      despeckle_image=DestroyImage(despeckle_image);
      return((Image *) NULL);
    }
  /*
    Allocate image buffers.
  */
  length=(size_t) image->columns+2UL;
  pixels=(Quantum *) AcquireQuantumMemory(length,(image->rows+2UL)*
    sizeof(*pixels));
  buffer=(Quantum *) AcquireQuantumMemory(length,(image->rows+2UL)*
    sizeof(*pixels));
  if ((buffer == (Quantum *) NULL) || (pixels == (Quantum *) NULL))
    {
      despeckle_image=DestroyImage(despeckle_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  length*=(image->rows+2);
  /*
    Reduce speckle in the image.
  */
  for (layer=0; layer <= 3; layer++)
  {
    (void) ResetMagickMemory(pixels,0,length);
    j=(long) image->columns+2;
    for (y=0; y < (long) image->rows; y++)
    {
      p=AcquireImagePixels(image,0,y,image->columns,1,exception);
      if (p == (const PixelPacket *) NULL)
        break;
      j++;
      for (x=0; x < (long) image->columns; x++)
      {
        switch (layer)
        {
          case 0: pixels[j]=p->red; break;
          case 1: pixels[j]=p->green; break;
          case 2: pixels[j]=p->blue; break;
          case 3: pixels[j]=p->opacity; break;
          default: break;
        }
        p++;
        j++;
      }
      j++;
    }
    (void) ResetMagickMemory(buffer,0,length);
    for (i=0; i < 4; i++)
    {
      Hull(X[i],Y[i],image->columns,image->rows,pixels,buffer,1);
      Hull(-X[i],-Y[i],image->columns,image->rows,pixels,buffer,1);
      Hull(-X[i],-Y[i],image->columns,image->rows,pixels,buffer,-1);
      Hull(X[i],Y[i],image->columns,image->rows,pixels,buffer,-1);
    }
    j=(long) image->columns+2;
    for (y=0; y < (long) image->rows; y++)
    {
      q=GetImagePixels(despeckle_image,0,y,despeckle_image->columns,1);
      if (q == (PixelPacket *) NULL)
        break;
      j++;
      for (x=0; x < (long) image->columns; x++)
      {
        switch (layer)
        {
          case 0: q->red=pixels[j]; break;
          case 1: q->green=pixels[j]; break;
          case 2: q->blue=pixels[j]; break;
          case 3: q->opacity=pixels[j]; break;
          default: break;
        }
        q++;
        j++;
      }
      if (SyncImagePixels(despeckle_image) == MagickFalse)
        break;
      j++;
    }
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(layer,3) != MagickFalse))
      {
        status=image->progress_monitor(DespeckleImageTag,layer,3,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  /*
    Free resources.
  */
  buffer=(Quantum *) RelinquishMagickMemory(buffer);
  pixels=(Quantum *) RelinquishMagickMemory(pixels);
  return(despeckle_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     E d g e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EdgeImage() finds edges in an image.  Radius defines the radius of the
%  convolution filter.  Use a radius of 0 and EdgeImage() selects a suitable
%  radius for you.
%
%  The format of the EdgeImage method is:
%
%      Image *EdgeImage(const Image *image,const double radius,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o radius: the radius of the pixel neighborhood.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *EdgeImage(const Image *image,const double radius,
  ExceptionInfo *exception)
{
  Image
    *edge_image;

  double
    *kernel;

  register long
    i;

  unsigned long
    width;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  width=GetOptimalKernelWidth1D(radius,0.5);
  kernel=(double *) AcquireQuantumMemory((size_t) width,width*sizeof(*kernel));
  if (kernel == (double *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  for (i=0; i < (long) (width*width); i++)
    kernel[i]=(-1.0);
  kernel[i/2]=(double) (width*width-1.0);
  edge_image=ConvolveImage(image,width,kernel,exception);
  kernel=(double *) RelinquishMagickMemory(kernel);
  return(edge_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     E m b o s s I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EmbossImage() returns a grayscale image with a three-dimensional effect.
%  We convolve the image with a Gaussian operator of the given radius and
%  standard deviation (sigma).  For reasonable results, radius should be
%  larger than sigma.  Use a radius of 0 and Emboss() selects a suitable
%  radius for you.
%
%  The format of the EmbossImage method is:
%
%      Image *EmbossImage(const Image *image,const double radius,
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
MagickExport Image *EmbossImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  double
    *kernel;

  Image
    *emboss_image;

  long
    j;

  MagickRealType
    alpha;

  register long
    i,
    u,
    v;

  unsigned long
    width;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  width=GetOptimalKernelWidth2D(radius,sigma);
  kernel=(double *) AcquireQuantumMemory((size_t) width,width*sizeof(*kernel));
  if (kernel == (double *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  i=0;
  j=(long) width/2;
  for (v=(-((long) width/2)); v <= (long) (width/2); v++)
  {
    for (u=(-((long) width/2)); u <= (long) (width/2); u++)
    {
      alpha=exp(-((double) u*u+v*v)/(2.0*sigma*sigma));
      kernel[i]=(double) (((u < 0) || (v < 0) ? -8.0 : 8.0)*alpha/
        (2.0*MagickPI*sigma*sigma));
      if (u != j)
        kernel[i]=0.0;
      i++;
    }
    j--;
  }
  emboss_image=ConvolveImage(image,width,kernel,exception);
  if (emboss_image != (Image *) NULL)
    (void) EqualizeImage(emboss_image);
  kernel=(double *) RelinquishMagickMemory(kernel);
  return(emboss_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     G a u s s i a n B l u r I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GaussianBlurImage() blurs an image.  We convolve the image with a
%  Gaussian operator of the given radius and standard deviation (sigma).
%  For reasonable results, the radius should be larger than sigma.  Use a
%  radius of 0 and GaussianBlurImage() selects a suitable radius for you
%
%  The format of the GaussianBlurImage method is:
%
%      Image *GaussianBlurImage(const Image *image,onst double radius,
%        const double sigma,ExceptionInfo *exception)
%      Image *GaussianBlurImageChannel(const Image *image,
%        const ChannelType channel,const double radius,const double sigma,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o radius: the radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o channel: The channel type.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

MagickExport Image *GaussianBlurImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  Image
    *blur_image;

  blur_image=GaussianBlurImageChannel(image,DefaultChannels,radius,sigma,
    exception);
  return(blur_image);
}

MagickExport Image *GaussianBlurImageChannel(const Image *image,
  const ChannelType channel,const double radius,const double sigma,
  ExceptionInfo *exception)
{
  double
    *kernel;

  Image
    *blur_image;

  MagickRealType
    alpha;

  register long
    i,
    u,
    v;

  unsigned long
    width;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  width=GetOptimalKernelWidth2D(radius,sigma);
  kernel=(double *) AcquireQuantumMemory((size_t) width,width*sizeof(*kernel));
  if (kernel == (double *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  i=0;
  for (v=(-((long) width/2)); v <= (long) (width/2); v++)
  {
    for (u=(-((long) width/2)); u <= (long) (width/2); u++)
    {
      alpha=exp(-((double) u*u+v*v)/(2.0*sigma*sigma));
      kernel[i]=(double) (alpha/(2.0*MagickPI*sigma*sigma));
      i++;
    }
  }
  blur_image=ConvolveImageChannel(image,channel,width,kernel,exception);
  kernel=(double *) RelinquishMagickMemory(kernel);
  return(blur_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M e d i a n F i l t e r I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MedianFilterImage() applies a digital filter that improves the quality
%  of a noisy image.  Each pixel is replaced by the median in a set of
%  neighboring pixels as defined by radius.
%
%  The algorithm was contributed by Mike Edmonds and implements an insertion
%  sort for selecting median color-channel values.  For more on this algorithm
%  see "Skip Lists: A probabilistic Alternative to Balanced Trees" by William
%  Pugh in the June 1990 of Communications of the ACM.
%
%  The format of the MedianFilterImage method is:
%
%      Image *MedianFilterImage(const Image *image,const double radius,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o radius: The radius of the pixel neighborhood.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

typedef struct _MedianListNode
{
  unsigned long
    next[9],
    count,
    signature;
} MedianListNode;

typedef struct _MedianSkipList
{
  long
    level;

  MedianListNode
    nodes[65537];
} MedianSkipList;

typedef struct _MedianPixelList
{
  unsigned long
    center,
    seed,
    signature;

  MedianSkipList
    lists[5];
} MedianPixelList;

static void AddNodeMedianList(MedianPixelList *pixel_list,int channel,
  unsigned long color)
{
  register long
    level;

  register MedianSkipList
    *list;

  unsigned long
    search,
    update[9];

  /*
    Initialize the node.
  */
  list=pixel_list->lists+channel;
  list->nodes[color].signature=pixel_list->signature;
  list->nodes[color].count=1;
  /*
    Determine where it belongs in the list.
  */
  search=65536UL;
  for (level=list->level; level >= 0; level--)
  {
    while (list->nodes[search].next[level] < color)
      search=list->nodes[search].next[level];
    update[level]=search;
  }
  /*
    Generate a pseudo-random level for this node.
  */
  for (level=0; ; level++)
  {
    pixel_list->seed=(pixel_list->seed*42893621L)+1L;
    if ((pixel_list->seed & 0x300) != 0x300)
      break;
  }
  if (level > 8)
    level=8;
  if (level > (list->level+2))
    level=list->level+2;
  /*
    If we're raising the list's level, link back to the root node.
  */
  while (level > list->level)
  {
    list->level++;
    update[list->level]=65536UL;
  }
  /*
    Link the node into the skip-list.
  */
  do
  {
    list->nodes[color].next[level]=list->nodes[update[level]].next[level];
    list->nodes[update[level]].next[level]=color;
  }
  while (level-- > 0);
}

static MagickPixelPacket GetMedianList(MedianPixelList *pixel_list)
{
  MagickPixelPacket
    pixel;

  register long
    channel;

  register MedianSkipList
    *list;

  unsigned long
    center,
    color,
    count;

  unsigned short
    channels[5];

  /*
    Find the median value for each of the color.
  */
  center=pixel_list->center;
  for (channel=0; channel < 5; channel++)
  {
    list=pixel_list->lists+channel;
    color=65536UL;
    count=0;
    do
    {
      color=list->nodes[color].next[0];
      count+=list->nodes[color].count;
    }
    while (count <= center);
    channels[channel]=(unsigned short) color;
  }
  GetMagickPixelPacket((const Image *) NULL,&pixel);
  pixel.red=(MagickRealType) ScaleShortToQuantum(channels[0]);
  pixel.green=(MagickRealType) ScaleShortToQuantum(channels[1]);
  pixel.blue=(MagickRealType) ScaleShortToQuantum(channels[2]);
  pixel.opacity=(MagickRealType) ScaleShortToQuantum(channels[3]);
  pixel.index=(MagickRealType) ScaleShortToQuantum(channels[4]);
  return(pixel);
}

static void InitializeMedianList(MedianPixelList *pixel_list,
  unsigned long width)
{
  pixel_list->center=width*width/2;
  pixel_list->signature=MagickSignature;
  (void) ResetMagickMemory((void *) pixel_list->lists,0,
    5*sizeof(*pixel_list->lists));
}

static inline void InsertMedianList(const Image *image,const PixelPacket *pixel,
  const IndexPacket *indexes,MedianPixelList *pixel_list)
{
  unsigned long
    signature;

  unsigned short
    index;

  index=ScaleQuantumToShort(pixel->red);
  signature=pixel_list->lists[0].nodes[index].signature;
  if (signature == pixel_list->signature)
    pixel_list->lists[0].nodes[index].count++;
  else
    AddNodeMedianList(pixel_list,0,index);
  index=ScaleQuantumToShort(pixel->green);
  signature=pixel_list->lists[1].nodes[index].signature;
  if (signature == pixel_list->signature)
    pixel_list->lists[1].nodes[index].count++;
  else
    AddNodeMedianList(pixel_list,1,index);
  index=ScaleQuantumToShort(pixel->blue);
  signature=pixel_list->lists[2].nodes[index].signature;
  if (signature == pixel_list->signature)
    pixel_list->lists[2].nodes[index].count++;
  else
    AddNodeMedianList(pixel_list,2,index);
  index=ScaleQuantumToShort(pixel->opacity);
  signature=pixel_list->lists[3].nodes[index].signature;
  if (signature == pixel_list->signature)
    pixel_list->lists[3].nodes[index].count++;
  else
    AddNodeMedianList(pixel_list,3,index);
  if (image->colorspace == CMYKColorspace)
    index=ScaleQuantumToShort(*indexes);
  signature=pixel_list->lists[4].nodes[index].signature;
  if (signature == pixel_list->signature)
    pixel_list->lists[4].nodes[index].count++;
  else
    AddNodeMedianList(pixel_list,4,index);
}

static void ResetMedianList(MedianPixelList *pixel_list)
{
  int
    level;

  register long
    channel;

  register MedianListNode
    *root;

  register MedianSkipList
    *list;

  /*
    Reset the skip-list.
  */
  for (channel=0; channel < 5; channel++)
  {
    list=pixel_list->lists+channel;
    root=list->nodes+65536UL;
    list->level=0;
    for (level=0; level < 9; level++)
      root->next[level]=65536UL;
  }
  pixel_list->seed=pixel_list->signature++;
}

MagickExport Image *MedianFilterImage(const Image *image,const double radius,
  ExceptionInfo *exception)
{
#define MedianFilterImageTag  "MedianFilter/Image"

  Image
    *median_image;

  long
    x,
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MedianPixelList
    *skiplist;

  register const PixelPacket
    *p,
    *r;

  register IndexPacket
    *indexes,
    *median_indexes,
    *s;

  register long
    u,
    v;

  register PixelPacket
    *q;

  unsigned long
    width;

  /*
    Initialize median image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  width=GetOptimalKernelWidth2D(radius,0.5);
  if ((image->columns < width) || (image->rows < width))
    ThrowImageException(OptionError,"ImageSmallerThanKernelRadius");
  median_image=CloneImage(image,0,0,MagickTrue,exception);
  if (median_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(median_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&median_image->exception);
      median_image=DestroyImage(median_image);
      return((Image *) NULL);
    }
  /*
    Allocate skip-lists.
  */
  skiplist=(MedianPixelList *) AcquireMagickMemory(sizeof(*skiplist));
  if (skiplist == (MedianPixelList *) NULL)
    {
      median_image=DestroyImage(median_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Median filter each image row.
  */
  InitializeMedianList(skiplist,width);
  for (y=0; y < (long) median_image->rows; y++)
  {
    p=AcquireImagePixels(image,-((long) width/2L),y-(long) (width/2L),
      image->columns+width,width,exception);
    q=GetImagePixels(median_image,0,y,median_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    median_indexes=GetIndexes(median_image);
    for (x=0; x < (long) median_image->columns; x++)
    {
      r=p;
      s=indexes+x;
      ResetMedianList(skiplist);
      for (v=0; v < (long) width; v++)
      {
        for (u=0; u < (long) width; u++)
          InsertMedianList(image,r+u,s+u,skiplist);
        r+=image->columns+width;
        s+=image->columns+width;
      }
      pixel=GetMedianList(skiplist);
      SetPixelPacket(median_image,&pixel,q,median_indexes+x);
      p++;
      q++;
    }
    if (SyncImagePixels(median_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(MedianFilterImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  skiplist=(MedianPixelList *) RelinquishMagickMemory(skiplist);
  return(median_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M o t i o n B l u r I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MotionBlurImage() simulates motion blur.  We convolve the image with a
%  Gaussian operator of the given radius and standard deviation (sigma).
%  For reasonable results, radius should be larger than sigma.  Use a
%  radius of 0 and MotionBlurImage() selects a suitable radius for you.
%  Angle gives the angle of the blurring motion.
%
%  Andrew Protano contributed this effect.
%
%  The format of the MotionBlurImage method is:
%
%    Image *MotionBlurImage(const Image *image,const double radius,
%      const double sigma,const double angle,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o radius: The radius of the Gaussian, in pixels, not counting
%     the center pixel.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
%    o angle: Apply the effect along this angle.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

static double *GetMotionBlurKernel(unsigned long width,
  const MagickRealType sigma)
{
#define KernelRank 3

  double
    *kernel;

  long
    bias;

  MagickRealType
    alpha,
    normalize;

  register long
    i;

  /*
    Generate a 1-D convolution kernel.
  */
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  kernel=(double *) AcquireQuantumMemory((size_t) width,sizeof(*kernel));
  if (kernel == (double *) NULL)
    return(0);
  (void) ResetMagickMemory(kernel,0,(size_t) width*sizeof(*kernel));
  bias=(long) (KernelRank*width);
  for (i=0; i < (long) bias; i++)
  {
    alpha=exp((-((double) (i*i))/(double) (2.0*KernelRank*KernelRank*
      sigma*sigma)));
    kernel[i/KernelRank]+=(double) alpha/(MagickSQ2PI*sigma);
  }
  normalize=0.0;
  for (i=0; i < (long) width; i++)
    normalize+=kernel[i];
  for (i=0; i < (long) width; i++)
    kernel[i]/=normalize;
  return(kernel);
}

MagickExport Image *MotionBlurImage(const Image *image,const double radius,
  const double sigma,const double angle,ExceptionInfo *exception)
{
  double
    *kernel;

  Image
    *blur_image;

  long
    u,
    v,
    y;

  MagickBooleanType
    status;

  MagickRealType
    alpha,
    gamma;

  PointInfo
    *offsets;

  MagickPixelPacket
    pixel;

  register const PixelPacket
    *p;

  register double
    *k;

  register IndexPacket
    *blur_indexes,
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  unsigned long
    width;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  width=GetOptimalKernelWidth1D(radius,sigma);
  kernel=GetMotionBlurKernel(width,sigma);
  if (kernel == (double *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  offsets=(PointInfo *) AcquireQuantumMemory(width,sizeof(*offsets));
  if (offsets == (PointInfo *) NULL)
    {
      kernel=(double *) RelinquishMagickMemory(kernel);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Allocate blur image.
  */
  blur_image=CloneImage(image,image->columns,image->rows,MagickTrue,exception);
  if (blur_image == (Image *) NULL)
    {
      kernel=(double *) RelinquishMagickMemory(kernel);
      offsets=(PointInfo *) RelinquishMagickMemory(offsets);
      return((Image *) NULL);
    }
  if (SetImageStorageClass(blur_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&blur_image->exception);
      blur_image=DestroyImage(blur_image);
      return((Image *) NULL);
    }
  x=(long) (width*sin(DegreesToRadians(angle))+0.5);
  y=(long) (width*cos(DegreesToRadians(angle))+0.5);
  for (i=0; i < (long) width; i++)
  {
    offsets[i].x=(MagickRealType) (i*y)/hypot((double) x,(double) y);
    offsets[i].y=(MagickRealType) (i*x)/hypot((double) x,(double) y);
  }
  alpha=1.0;
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(blur_image,0,y,blur_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    blur_indexes=GetIndexes(blur_image);
    for (x=0; x < (long) image->columns; x++)
    {
      GetMagickPixelPacket(image,&pixel);
      gamma=0.0;
      k=kernel;
      for (i=0; i < (long) width; i++)
      {
        u=(long) (x+offsets[i].x+0.5);
        v=(long) (y+offsets[i].y+0.5);
        p=AcquireImagePixels(image,u,v,1,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        if (image->matte != MagickFalse)
          alpha=QuantumScale*((MagickRealType) QuantumRange-p->opacity);
        pixel.red+=(*k)*alpha*p->red;
        pixel.green+=(*k)*alpha*p->green;
        pixel.blue+=(*k)*alpha*p->blue;
        pixel.opacity+=(*k)*p->opacity;
        if (image->colorspace == CMYKColorspace)
          pixel.index+=(*k)*alpha*(*indexes);
        gamma+=(*k)*alpha;
        k++;
      }
      gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
      q->red=RoundToQuantum(gamma*pixel.red);
      q->green=RoundToQuantum(gamma*pixel.green);
      q->blue=RoundToQuantum(gamma*pixel.blue);
      q->opacity=RoundToQuantum(pixel.opacity);
      if (image->colorspace == CMYKColorspace)
        blur_indexes[x]=(IndexPacket) RoundToQuantum(gamma*pixel.index);
      q++;
    }
    if (SyncImagePixels(blur_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(BlurImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  kernel=(double *) RelinquishMagickMemory(kernel);
  offsets=(PointInfo *) RelinquishMagickMemory(offsets);
  return(blur_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     P r e v i e w I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PreviewImage() tiles 9 thumbnails of the specified image with an image
%  processing operation applied with varying parameters.  This may be helpful
%  pin-pointing an appropriate parameter for a particular image processing
%  operation.
%
%  The format of the PreviewImages method is:
%
%      Image *PreviewImages(const Image *image,const PreviewType preview,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o preview: The image processing operation.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *PreviewImage(const Image *image,const PreviewType preview,
  ExceptionInfo *exception)
{
#define NumberTiles  9
#define PreviewImageTag  "Preview/Image"
#define DefaultPreviewGeometry  "204x204+10+10"

  char
    factor[MaxTextExtent],
    label[MaxTextExtent];

  double
    degrees,
    gamma,
    percentage,
    radius,
    sigma,
    threshold;

  Image
    *images,
    *montage_image,
    *preview_image,
    *thumbnail;

  ImageInfo
    *preview_info;

  long
    y;

  MagickBooleanType
    status;

  MontageInfo
    *montage_info;

  QuantizeInfo
    quantize_info;

  RectangleInfo
    geometry;

  register long
    i,
    x;

  unsigned long
    colors;

  /*
    Open output image file.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  colors=2;
  degrees=0.0;
  gamma=(-0.2f);
  preview_info=AcquireImageInfo();
  SetGeometry(image,&geometry);
  (void) ParseMetaGeometry(DefaultPreviewGeometry,&geometry.x,&geometry.y,
    &geometry.width,&geometry.height);
  images=NewImageList();
  percentage=12.5;
  GetQuantizeInfo(&quantize_info);
  radius=0.0;
  sigma=1.0;
  threshold=0.0;
  x=0;
  y=0;
  for (i=0; i < NumberTiles; i++)
  {
    thumbnail=ThumbnailImage(image,geometry.width,geometry.height,exception);
    if (thumbnail == (Image *) NULL)
      break;
    (void) SetImageProgressMonitor(thumbnail,(MagickProgressMonitor) NULL,
      (void *) NULL);
    (void) SetImageProperty(thumbnail,"label",DefaultTileLabel);
    if (i == (NumberTiles/2))
      {
        (void) QueryColorDatabase("#dfdfdf",&thumbnail->matte_color,exception);
        AppendImageToList(&images,thumbnail);
        continue;
      }
    switch (preview)
    {
      case RotatePreview:
      {
        degrees+=45.0;
        preview_image=RotateImage(thumbnail,degrees,exception);
        (void) FormatMagickString(label,MaxTextExtent,"rotate %g",degrees);
        break;
      }
      case ShearPreview:
      {
        degrees+=5.0;
        preview_image=ShearImage(thumbnail,degrees,degrees,exception);
        (void) FormatMagickString(label,MaxTextExtent,"shear %gx%g",
          degrees,2.0*degrees);
        break;
      }
      case RollPreview:
      {
        x=(long) ((i+1)*thumbnail->columns)/NumberTiles;
        y=(long) ((i+1)*thumbnail->rows)/NumberTiles;
        preview_image=RollImage(thumbnail,x,y,exception);
        (void) FormatMagickString(label,MaxTextExtent,"roll %ldx%ld",x,y);
        break;
      }
      case HuePreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) FormatMagickString(factor,MaxTextExtent,"100,100,%g",
          2.0*percentage);
        (void) ModulateImage(preview_image,factor);
        (void) FormatMagickString(label,MaxTextExtent,"modulate %s",factor);
        break;
      }
      case SaturationPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) FormatMagickString(factor,MaxTextExtent,"100,%g",2.0*percentage);
        (void) ModulateImage(preview_image,factor);
        (void) FormatMagickString(label,MaxTextExtent,"modulate %s",factor);
        break;
      }
      case BrightnessPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) FormatMagickString(factor,MaxTextExtent,"%g",2.0*percentage);
        (void) ModulateImage(preview_image,factor);
        (void) FormatMagickString(label,MaxTextExtent,"modulate %s",factor);
        break;
      }
      case GammaPreview:
      default:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        gamma+=0.4f;
        (void) GammaImageChannel(preview_image,DefaultChannels,gamma);
        (void) FormatMagickString(label,MaxTextExtent,"gamma %g",gamma);
        break;
      }
      case SpiffPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image != (Image *) NULL)
          for (x=0; x < i; x++)
            (void) ContrastImage(preview_image,MagickTrue);
        (void) FormatMagickString(label,MaxTextExtent,"contrast (%ld)",i+1);
        break;
      }
      case DullPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        for (x=0; x < i; x++)
          (void) ContrastImage(preview_image,MagickFalse);
        (void) FormatMagickString(label,MaxTextExtent,"+contrast (%ld)",i+1);
        break;
      }
      case GrayscalePreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        colors<<=1;
        quantize_info.number_colors=colors;
        quantize_info.colorspace=GRAYColorspace;
        (void) QuantizeImage(&quantize_info,preview_image);
        (void) FormatMagickString(label,MaxTextExtent,
          "-colorspace gray -colors %ld",colors);
        break;
      }
      case QuantizePreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        colors<<=1;
        quantize_info.number_colors=colors;
        (void) QuantizeImage(&quantize_info,preview_image);
        (void) FormatMagickString(label,MaxTextExtent,"colors %ld",colors);
        break;
      }
      case DespecklePreview:
      {
        for (x=0; x < (i-1); x++)
        {
          preview_image=DespeckleImage(thumbnail,exception);
          if (preview_image == (Image *) NULL)
            break;
          thumbnail=DestroyImage(thumbnail);
          thumbnail=preview_image;
        }
        preview_image=DespeckleImage(thumbnail,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) FormatMagickString(label,MaxTextExtent,"despeckle (%ld)",i+1);
        break;
      }
      case ReduceNoisePreview:
      {
        preview_image=ReduceNoiseImage(thumbnail,radius,exception);
        (void) FormatMagickString(label,MaxTextExtent,"noise %g",radius);
        break;
      }
      case AddNoisePreview:
      {
        switch ((int) i)
        {
          case 0:
          {
            (void) CopyMagickString(factor,"uniform",MaxTextExtent);
            break;
          }
          case 1:
          {
            (void) CopyMagickString(factor,"gaussian",MaxTextExtent);
            break;
          }
          case 2:
          {
            (void) CopyMagickString(factor,"multiplicative",MaxTextExtent);
            break;
          }
          case 3:
          {
            (void) CopyMagickString(factor,"impulse",MaxTextExtent);
            break;
          }
          case 4:
          {
            (void) CopyMagickString(factor,"laplacian",MaxTextExtent);
            break;
          }
          case 5:
          {
            (void) CopyMagickString(factor,"Poisson",MaxTextExtent);
            break;
          }
          default:
          {
            (void) CopyMagickString(thumbnail->magick,"NULL",MaxTextExtent);
            break;
          }
        }
        preview_image=ReduceNoiseImage(thumbnail,(double) i,exception);
        (void) FormatMagickString(label,MaxTextExtent,"+noise %s",factor);
        break;
      }
      case SharpenPreview:
      {
        preview_image=SharpenImage(thumbnail,radius,sigma,exception);
        (void) FormatMagickString(label,MaxTextExtent,"sharpen %gx%g",radius,
          sigma);
        break;
      }
      case BlurPreview:
      {
        preview_image=BlurImage(thumbnail,radius,sigma,exception);
        (void) FormatMagickString(label,MaxTextExtent,"blur %gx%g",radius,
          sigma);
        break;
      }
      case ThresholdPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) BilevelImage(thumbnail,
          (double) (percentage*((MagickRealType) QuantumRange+1.0))/100.0);
        (void) FormatMagickString(label,MaxTextExtent,"threshold %g",
          (double) (percentage*((MagickRealType) QuantumRange+1.0))/100.0);
        break;
      }
      case EdgeDetectPreview:
      {
        preview_image=EdgeImage(thumbnail,radius,exception);
        (void) FormatMagickString(label,MaxTextExtent,"edge %g",radius);
        break;
      }
      case SpreadPreview:
      {
        preview_image=SpreadImage(thumbnail,radius,exception);
        (void) FormatMagickString(label,MaxTextExtent,"spread %g",radius+0.5);
        break;
      }
      case SolarizePreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) SolarizeImage(preview_image,(double) QuantumRange*
          percentage/100.0);
        (void) FormatMagickString(label,MaxTextExtent,"solarize %g",
          (QuantumRange*percentage)/100.0);
        break;
      }
      case ShadePreview:
      {
        degrees+=10.0;
        preview_image=ShadeImage(thumbnail,MagickTrue,degrees,degrees,
          exception);
        (void) FormatMagickString(label,MaxTextExtent,"shade %gx%g",degrees,
          degrees);
        break;
      }
      case RaisePreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        geometry.width=(unsigned long) (2*i+2);
        geometry.height=(unsigned long) (2*i+2);
        geometry.x=i/2;
        geometry.y=i/2;
        (void) RaiseImage(preview_image,&geometry,MagickTrue);
        (void) FormatMagickString(label,MaxTextExtent,"raise %lux%lu%+ld%+ld",
          geometry.width,geometry.height,geometry.x,geometry.y);
        break;
      }
      case SegmentPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        threshold+=0.4f;
        (void) SegmentImage(preview_image,RGBColorspace,MagickFalse,threshold,
          threshold);
        (void) FormatMagickString(label,MaxTextExtent,"segment %gx%g",
          threshold,threshold);
        break;
      }
      case SwirlPreview:
      {
        preview_image=SwirlImage(thumbnail,degrees,exception);
        (void) FormatMagickString(label,MaxTextExtent,"swirl %g",degrees);
        degrees+=45.0;
        break;
      }
      case ImplodePreview:
      {
        degrees+=0.1f;
        preview_image=ImplodeImage(thumbnail,degrees,exception);
        (void) FormatMagickString(label,MaxTextExtent,"implode %g",degrees);
        break;
      }
      case WavePreview:
      {
        degrees+=5.0f;
        preview_image=WaveImage(thumbnail,0.5*degrees,2.0*degrees,exception);
        (void) FormatMagickString(label,MaxTextExtent,"wave %gx%g",0.5*degrees,
          2.0*degrees);
        break;
      }
      case OilPaintPreview:
      {
        preview_image=OilPaintImage(thumbnail,(double) radius,exception);
        (void) FormatMagickString(label,MaxTextExtent,"paint %g",radius);
        break;
      }
      case CharcoalDrawingPreview:
      {
        preview_image=CharcoalImage(thumbnail,(double) radius,(double) sigma,
          exception);
        (void) FormatMagickString(label,MaxTextExtent,"charcoal %gx%g",radius,
          sigma);
        break;
      }
      case JPEGPreview:
      {
        char
          filename[MaxTextExtent];

        int
          file;

        MagickBooleanType
          status;

        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        preview_info->quality=(unsigned long) percentage;
        (void) FormatMagickString(factor,MaxTextExtent,"%lu",
          preview_info->quality);
        file=AcquireUniqueFileResource(filename);
        if (file != -1)
          file=close(file)-1;
        (void) FormatMagickString(preview_image->filename,MaxTextExtent,
          "jpeg:%s",filename);
        status=WriteImage(preview_info,preview_image);
        if (status != MagickFalse)
          {
            Image
              *quality_image;

            (void) CopyMagickString(preview_info->filename,
              preview_image->filename,MaxTextExtent);
            quality_image=ReadImage(preview_info,exception);
            if (quality_image != (Image *) NULL)
              {
                preview_image=DestroyImage(preview_image);
                preview_image=quality_image;
              }
          }
        (void) RelinquishUniqueFileResource(preview_image->filename);
        if ((GetBlobSize(preview_image)/1024) >= 1024)
          (void) FormatMagickString(label,MaxTextExtent,"quality %s\n%gmb ",
            factor,(double) ((MagickOffsetType) GetBlobSize(preview_image))/
            1024.0/1024.0);
        else
          if (GetBlobSize(preview_image) >= 1024)
            (void) FormatMagickString(label,MaxTextExtent,"quality %s\n%gkb ",
              factor,(double) ((MagickOffsetType) GetBlobSize(preview_image))/
              1024.0);
          else
            (void) FormatMagickString(label,MaxTextExtent,"quality %s\n%lub ",
              factor,(unsigned long) GetBlobSize(thumbnail));
        break;
      }
    }
    thumbnail=DestroyImage(thumbnail);
    percentage+=12.5;
    radius+=0.5;
    sigma+=0.25;
    if (preview_image == (Image *) NULL)
      break;
    (void) DeleteImageProperty(preview_image,"label");
    (void) SetImageProperty(preview_image,"label",label);
    AppendImageToList(&images,preview_image);
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(i,NumberTiles) != MagickFalse))
      {
        status=image->progress_monitor(PreviewImageTag,i,NumberTiles,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  if (images == (Image *) NULL)
    {
      preview_info=DestroyImageInfo(preview_info);
      return((Image *) NULL);
    }
  /*
    Create the montage.
  */
  montage_info=CloneMontageInfo(preview_info,(MontageInfo *) NULL);
  (void) CopyMagickString(montage_info->filename,image->filename,MaxTextExtent);
  montage_info->shadow=MagickTrue;
  (void) CloneString(&montage_info->tile,"3x3");
  (void) CloneString(&montage_info->geometry,DefaultPreviewGeometry);
  (void) CloneString(&montage_info->frame,DefaultTileFrame);
  montage_image=MontageImages(images,montage_info,exception);
  montage_info=DestroyMontageInfo(montage_info);
  images=DestroyImageList(images);
  if (montage_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  if (montage_image->montage != (char *) NULL)
    {
      /*
        Free image directory.
      */
      montage_image->montage=(char *)
        RelinquishMagickMemory(montage_image->montage);
      if (image->directory != (char *) NULL)
        montage_image->directory=(char *)
          RelinquishMagickMemory(montage_image->directory);
    }
  preview_info=DestroyImageInfo(preview_info);
  return(montage_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     R a d i a l B l u r I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RadialBlurImage() applies a radial blur to the image.
%
%  Andrew Protano contributed this effect.
%
%  The format of the RadialBlurImage method is:
%
%    Image *RadialBlurImage(const Image *image,const double angle,
%      ExceptionInfo *exception)
%    Image *RadialBlurImageChannel(const Image *image,const ChannelType channel,
%      const double angle,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel type.
%
%    o angle: The angle of the radial blur.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

MagickExport Image *RadialBlurImage(const Image *image,const double angle,
  ExceptionInfo *exception)
{
  Image
    *blur_image;

  blur_image=RadialBlurImageChannel(image,DefaultChannels,angle,exception);
  return(blur_image);
}

MagickExport Image *RadialBlurImageChannel(const Image *image,
  const ChannelType channel,const double angle,ExceptionInfo *exception)
{
  Image
    *blur_image;

  long
    y;

  PointInfo
    blur_center,
    center;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MagickRealType
    alpha,
    blur_radius,
    *cos_theta,
    gamma,
    normalize,
    offset,
    radius,
    *sin_theta,
    theta;

  register const PixelPacket
    *p;

  register IndexPacket
    *blur_indexes,
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  unsigned long
    n,
    step;

  /*
    Allocate blur image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  blur_image=CloneImage(image,0,0,MagickTrue,exception);
  if (blur_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(blur_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&blur_image->exception);
      blur_image=DestroyImage(blur_image);
      return((Image *) NULL);
    }
  blur_center.x=(double) image->columns/2.0;
  blur_center.y=(double) image->rows/2.0;
  blur_radius=hypot(blur_center.x,blur_center.y);
  n=(unsigned long) fabs(4.0*DegreesToRadians(angle)*sqrt((double) blur_radius)+
    2UL);
  theta=DegreesToRadians(angle)/(MagickRealType) (n-1);
  cos_theta=(MagickRealType *) AcquireQuantumMemory((size_t) n,
    sizeof(*cos_theta));
  sin_theta=(MagickRealType *) AcquireQuantumMemory((size_t) n,
    sizeof(*sin_theta));
  if ((cos_theta == (MagickRealType *) NULL) ||
      (sin_theta == (MagickRealType *) NULL))
    {
      blur_image=DestroyImage(blur_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  offset=theta*(MagickRealType) (n-1)/2.0;
  for (i=0; i < (long) n; i++)
  {
    cos_theta[i]=cos((double) (theta*i-offset));
    sin_theta[i]=sin((double) (theta*i-offset));
  }
  /*
    Radial blur image.
  */
  alpha=1.0;
  for (y=0; y < (long) blur_image->rows; y++)
  {
    q=GetImagePixels(blur_image,0,y,blur_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    blur_indexes=GetIndexes(blur_image);
    for (x=0; x < (long) blur_image->columns; x++)
    {
      center.x=(double) x-blur_center.x;
      center.y=(double) y-blur_center.y;
      radius=hypot((double) center.x,center.y);
      if (radius == 0)
        step=1;
      else
        {
          step=(unsigned long) (blur_radius/radius);
          if (step == 0)
            step=1;
          else
            if (step >= n)
              step=n-1;
        }
      GetMagickPixelPacket(image,&pixel);
      gamma=0.0;
      normalize=0.0;
      for (i=0; i < (long) n; i+=step)
      {
        p=AcquireImagePixels(image,(long) (blur_center.x+center.x*cos_theta[i]-
          center.y*sin_theta[i]+0.5),(long) (blur_center.y+center.x*
          sin_theta[i]+center.y*cos_theta[i]+0.5),1,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        if (((channel & OpacityChannel) != 0) &&
            (image->matte != MagickFalse))
          alpha=QuantumScale*((MagickRealType) QuantumRange-p->opacity);
        if ((channel & RedChannel) != 0)
          pixel.red+=alpha*p->red;
        if ((channel & GreenChannel) != 0)
          pixel.green+=alpha*p->green;
        if ((channel & BlueChannel) != 0)
          pixel.blue+=alpha*p->blue;
        if ((channel & OpacityChannel) != 0)
          pixel.opacity+=p->opacity;
        if (((channel & IndexChannel) != 0) &&
            (image->colorspace == CMYKColorspace))
          pixel.index+=alpha*(*indexes);
        gamma+=alpha;
        normalize+=1.0;
      }
      gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
      normalize=1.0/
        (fabs((double) normalize) <= MagickEpsilon ? 1.0 : normalize);
      if ((channel & RedChannel) != 0)
        q->red=RoundToQuantum(gamma*pixel.red);
      if ((channel & GreenChannel) != 0)
        q->green=RoundToQuantum(gamma*pixel.green);
      if ((channel & BlueChannel) != 0)
        q->blue=RoundToQuantum(gamma*pixel.blue);
      if ((channel & OpacityChannel) != 0)
        q->opacity=RoundToQuantum(normalize*pixel.opacity);
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        blur_indexes[x]=(IndexPacket) RoundToQuantum(gamma*pixel.index);
      q++;
    }
    if (SyncImagePixels(blur_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(BlurImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  cos_theta=(MagickRealType *) RelinquishMagickMemory(cos_theta);
  sin_theta=(MagickRealType *) RelinquishMagickMemory(sin_theta);
  return(blur_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     R e d u c e N o i s e I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReduceNoiseImage() smooths the contours of an image while still preserving
%  edge information.  The algorithm works by replacing each pixel with its
%  neighbor closest in value.  A neighbor is defined by radius.  Use a radius
%  of 0 and ReduceNoise() selects a suitable radius for you.
%
%  The format of the ReduceNoiseImage method is:
%
%      Image *ReduceNoiseImage(const Image *image,const double radius,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o radius: The radius of the pixel neighborhood.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

static MagickPixelPacket GetNonpeakMedianList(MedianPixelList *pixel_list)
{
  MagickPixelPacket
    pixel;

  register MedianSkipList
    *list;

  register long
    channel;

  unsigned long
    center,
    color,
    count,
    previous,
    next;

  unsigned short
    channels[5];

  /*
    Finds the median value for each of the color.
  */
  center=pixel_list->center;
  for (channel=0; channel < 5; channel++)
  {
    list=pixel_list->lists+channel;
    color=65536UL;
    next=list->nodes[color].next[0];
    count=0;
    do
    {
      previous=color;
      color=next;
      next=list->nodes[color].next[0];
      count+=list->nodes[color].count;
    }
    while (count <= center);
    if ((previous == 65536UL) && (next != 65536UL))
      color=next;
    else
      if ((previous != 65536UL) && (next == 65536UL))
        color=previous;
    channels[channel]=(unsigned short) color;
  }
  GetMagickPixelPacket((const Image *) NULL,&pixel);
  pixel.red=(MagickRealType) ScaleShortToQuantum(channels[0]);
  pixel.green=(MagickRealType) ScaleShortToQuantum(channels[1]);
  pixel.blue=(MagickRealType) ScaleShortToQuantum(channels[2]);
  pixel.opacity=(MagickRealType) ScaleShortToQuantum(channels[3]);
  pixel.index=(MagickRealType) ScaleShortToQuantum(channels[4]);
  return(pixel);
}

MagickExport Image *ReduceNoiseImage(const Image *image,const double radius,
  ExceptionInfo *exception)
{
#define ReduceNoiseImageTag  "ReduceNoise/Image"

  Image
    *noise_image;

  long
    x,
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MedianPixelList
    *skiplist;

  register const PixelPacket
    *p,
    *r;

  register IndexPacket
    *indexes,
    *noise_indexes,
    *s;

  register long
    u,
    v;

  register PixelPacket
    *q;

  unsigned long
    width;

  /*
    Initialize noised image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  width=GetOptimalKernelWidth2D(radius,0.5);
  if ((image->columns < width) || (image->rows < width))
    ThrowImageException(OptionError,"ImageSmallerThanKernelRadius");
  noise_image=CloneImage(image,0,0,MagickTrue,exception);
  if (noise_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(noise_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&noise_image->exception);
      noise_image=DestroyImage(noise_image);
      return((Image *) NULL);
    }
  /*
    Allocate skip-lists.
  */
  skiplist=(MedianPixelList *) AcquireMagickMemory(sizeof(*skiplist));
  if (skiplist == (MedianPixelList *) NULL)
    {
      noise_image=DestroyImage(noise_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Median filter each image row.
  */
  InitializeMedianList(skiplist,width);
  for (y=0; y < (long) noise_image->rows; y++)
  {
    p=AcquireImagePixels(image,-((long) width/2L),y-(long) (width/2L),
      image->columns+width,width,exception);
    q=GetImagePixels(noise_image,0,y,noise_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    noise_indexes=GetIndexes(noise_image);
    for (x=0; x < (long) noise_image->columns; x++)
    {
      r=p;
      s=indexes+x;
      ResetMedianList(skiplist);
      for (v=0; v < (long) width; v++)
      {
        for (u=0; u < (long) width; u++)
          InsertMedianList(image,r+u,s+u,skiplist);
        r+=image->columns+width;
        s+=image->columns+width;
      }
      pixel=GetNonpeakMedianList(skiplist);
      SetPixelPacket(noise_image,&pixel,q,noise_indexes+x);
      p++;
      q++;
    }
    if (SyncImagePixels(noise_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ReduceNoiseImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  skiplist=(MedianPixelList *) RelinquishMagickMemory(skiplist);
  return(noise_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S h a d e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ShadeImage() shines a distant light on an image to create a
%  three-dimensional effect. You control the positioning of the light with
%  azimuth and elevation; azimuth is measured in degrees off the x axis
%  and elevation is measured in pixels above the Z axis.
%
%  The format of the ShadeImage method is:
%
%      Image *ShadeImage(const Image *image,const MagickBooleanType gray,
%        const double azimuth,const double elevation,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o gray: A value other than zero shades the intensity of each pixel.
%
%    o azimuth, elevation:  Define the light source direction.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *ShadeImage(const Image *image,const MagickBooleanType gray,
  const double azimuth,const double elevation,ExceptionInfo *exception)
{
#define ShadeImageTag  "Shade/Image"

  Image
    *shade_image;

  long
    y;

  MagickBooleanType
    status;

  MagickRealType
    distance,
    normal_distance,
    shade;

  PrimaryInfo
    light,
    normal;

  register const PixelPacket
    *p,
    *s0,
    *s1,
    *s2;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Initialize shaded image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  shade_image=CloneImage(image,0,0,MagickTrue,exception);
  if (shade_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(shade_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&shade_image->exception);
      shade_image=DestroyImage(shade_image);
      return((Image *) NULL);
    }
  /*
    Compute the light vector.
  */
  light.x=(double) QuantumRange*cos(DegreesToRadians(azimuth))*
    cos(DegreesToRadians(elevation));
  light.y=(double) QuantumRange*sin(DegreesToRadians(azimuth))*
    cos(DegreesToRadians(elevation));
  light.z=(double) QuantumRange*sin(DegreesToRadians(elevation));
  normal.z=2.0*(double) QuantumRange;  /* constant Z of surface normal */
  /*
    Shade image.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,-1,y-1,image->columns+2,3,exception);
    q=GetImagePixels(shade_image,0,y,shade_image->columns,1);
    if ((p == (PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    /*
      Shade this row of pixels.
    */
    s0=p+1;
    s1=s0+image->columns+2;
    s2=s1+image->columns+2;
    for (x=0; x < (long) image->columns; x++)
    {
      /*
        Determine the surface normal and compute shading.
      */
      normal.x=(double) (PixelIntensity(s0-1)+PixelIntensity(s1-1)+
        PixelIntensity(s2-1)-PixelIntensity(s0+1)-PixelIntensity(s1+1)-
        PixelIntensity(s2+1));
      normal.y=(double) (PixelIntensity(s2-1)+PixelIntensity(s2)+
        PixelIntensity(s2+1)-PixelIntensity(s0-1)-PixelIntensity(s0)-
        PixelIntensity(s0+1));
      if ((normal.x == 0.0) && (normal.y == 0.0))
        shade=light.z;
      else
        {
          shade=0.0;
          distance=normal.x*light.x+normal.y*light.y+normal.z*light.z;
          if (distance > MagickEpsilon)
            {
              normal_distance=
                normal.x*normal.x+normal.y*normal.y+normal.z*normal.z;
              if (normal_distance > (MagickEpsilon*MagickEpsilon))
                shade=distance/sqrt((double) normal_distance);
            }
        }
      if (gray != MagickFalse)
        {
          q->red=(Quantum) shade;
          q->green=(Quantum) shade;
          q->blue=(Quantum) shade;
        }
      else
        {
          q->red=RoundToQuantum(QuantumScale*shade*s1->red);
          q->green=RoundToQuantum(QuantumScale*shade*s1->green);
          q->blue=RoundToQuantum(QuantumScale*shade*s1->blue);
        }
      q->opacity=s1->opacity;
      s0++;
      s1++;
      s2++;
      q++;
    }
    if (SyncImagePixels(shade_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ShadeImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(shade_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S h a r p e n I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SharpenImage() sharpens the image.  We convolve the image with a Gaussian
%  operator of the given radius and standard deviation (sigma).  For
%  reasonable results, radius should be larger than sigma.  Use a radius of 0
%  and SharpenImage() selects a suitable radius for you.
%
%  Using a separable kernel would be faster, but the negative weights cancel
%  out on the corners of the kernel producing often undesirable ringing in the
%  filtered result; this can be avoided by using a 2D gaussian shaped image
%  sharpening kernel instead.
%
%  The format of the SharpenImage method is:
%
%    Image *SharpenImage(const Image *image,const double radius,
%      const double sigma,ExceptionInfo *exception)
%    Image *SharpenImageChannel(const Image *image,const ChannelType channel,
%      const double radius,const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel type.
%
%    o radius: The radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the Laplacian, in pixels.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

MagickExport Image *SharpenImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  Image
    *sharp_image;

  sharp_image=SharpenImageChannel(image,DefaultChannels,radius,sigma,exception);
  return(sharp_image);
}

MagickExport Image *SharpenImageChannel(const Image *image,
  const ChannelType channel,const double radius,const double sigma,
  ExceptionInfo *exception)
{
  double
    *kernel;

  Image
    *sharp_image;

  MagickRealType
    alpha,
    normalize;

  register long
    i,
    u,
    v;

  unsigned long
    width;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  width=GetOptimalKernelWidth2D(radius,sigma);
  kernel=(double *) AcquireQuantumMemory((size_t) width*width,sizeof(*kernel));
  if (kernel == (double *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  i=0;
  normalize=0.0;
  for (v=(-((long) width/2)); v <= (long) (width/2); v++)
  {
    for (u=(-((long) width/2)); u <= (long) (width/2); u++)
    {
      alpha=exp(-((double) u*u+v*v)/(2.0*sigma*sigma));
      kernel[i]=(double) (-alpha/(2.0*MagickPI*sigma*sigma));
      if ((width < 3) || (u != 0) || (v != 0))
        normalize+=kernel[i];
      i++;
    }
  }
  kernel[i/2]=(double) ((-2.0)*normalize);
  sharp_image=ConvolveImageChannel(image,channel,width,kernel,exception);
  kernel=(double *) RelinquishMagickMemory(kernel);
  return(sharp_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S p r e a d I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SpreadImage() is a special effects method that randomly displaces each
%  pixel in a block defined by the radius parameter.
%
%  The format of the SpreadImage method is:
%
%      Image *SpreadImage(const Image *image,const double radius,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o radius:  Choose a random pixel in a neighborhood of this extent.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *SpreadImage(const Image *image,const double radius,
  ExceptionInfo *exception)
{
#define SpreadImageTag  "Spread/Image"

  Image
    *spread_image;

  long
    x_distance,
    y,
    y_distance;

  MagickBooleanType
    status;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  unsigned long
    width;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if ((image->columns < 3) || (image->rows < 3))
    return((Image *) NULL);
  /*
    Initialize spread image attributes.
  */
  spread_image=CloneImage(image,0,0,MagickTrue,exception);
  if (spread_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(spread_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&spread_image->exception);
      spread_image=DestroyImage(spread_image);
      return((Image *) NULL);
    }
  /*
    Convolve each row.
  */
  width=2*((unsigned long) radius)+1;
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,-((long) width/2L),y-(long) (width/2L),
      image->columns+width,width,exception);
    q=GetImagePixels(spread_image,0,y,spread_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      x_distance=(long) ((MagickRealType) width*GetRandomValue());
      y_distance=(long) ((MagickRealType) width*GetRandomValue());
      *q++=(*(p+(image->columns+width)*y_distance+x+x_distance));
    }
    if (SyncImagePixels(spread_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SpreadImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(spread_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     U n s h a r p M a s k I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnsharpMaskImage() sharpens one or more image channels.  We convolve the
%  image with a Gaussian operator of the given radius and standard deviation
%  (sigma).  For reasonable results, radius should be larger than sigma.  Use a
%  radius of 0 and UnsharpMaskImage() selects a suitable radius for you.
%
%  The format of the UnsharpMaskImage method is:
%
%    Image *UnsharpMaskImage(const Image *image,const double radius,
%      const double sigma,const double amount,const double threshold,
%      ExceptionInfo *exception)
%    Image *UnsharpMaskImageChannel(const Image *image,
%      const ChannelType channel,const double radius,const double sigma,
%      const double amount,const double threshold,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel type.
%
%    o radius: The radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
%    o amount: The percentage of the difference between the original and the
%      blur image that is added back into the original.
%
%    o threshold: The threshold in pixels needed to apply the diffence amount.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

MagickExport Image *UnsharpMaskImage(const Image *image,const double radius,
  const double sigma,const double amount,const double threshold,
  ExceptionInfo *exception)
{
  Image
    *sharp_image;

  sharp_image=UnsharpMaskImageChannel(image,DefaultChannels,radius,sigma,amount,
    threshold,exception);
  return(sharp_image);
}

MagickExport Image *UnsharpMaskImageChannel(const Image *image,
  const ChannelType channel,const double radius,const double sigma,
  const double amount,const double threshold,ExceptionInfo *exception)
{
#define SharpenImageTag  "Sharpen/Image"

  Image
    *unsharp_image;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MagickRealType
    quantum_threshold;

  register const IndexPacket
    *indexes;

  register const PixelPacket
    *pixels;

  register IndexPacket
    *unsharp_indexes;

  register long
    x;

  register PixelPacket
    *unsharp_pixels;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  unsharp_image=BlurImageChannel(image,channel,radius,sigma,exception);
  if (unsharp_image == (Image *) NULL)
    return((Image *) NULL);
  quantum_threshold=(MagickRealType) QuantumRange*threshold;
  for (y=0; y < (long) image->rows; y++)
  {
    pixels=AcquireImagePixels(image,0,y,image->columns,1,exception);
    unsharp_pixels=GetImagePixels(unsharp_image,0,y,unsharp_image->columns,1);
    if ((pixels == (const PixelPacket *) NULL) ||
        (unsharp_pixels == (PixelPacket *) NULL))
      break;
    indexes=AcquireIndexes(image);
    unsharp_indexes=GetIndexes(unsharp_image);
    #pragma omp parallel for private(pixel)
    for (x=0; x < (long) image->columns; x++)
    {
      if ((channel & RedChannel) != 0)
        {
          pixel.red=pixels[x].red-(MagickRealType) unsharp_pixels[x].red;
          if (fabs(2.0*pixel.red) < quantum_threshold)
            pixel.red=(MagickRealType) pixels[x].red;
          else
            pixel.red=(MagickRealType) pixels[x].red+(pixel.red*amount);
          unsharp_pixels[x].red=RoundToQuantum(pixel.red);
        }
      if ((channel & GreenChannel) != 0)
        {
          pixel.green=pixels[x].green-(MagickRealType) unsharp_pixels[x].green;
          if (fabs(2.0*pixel.green) < quantum_threshold)
            pixel.green=(MagickRealType) pixels[x].green;
          else
            pixel.green=(MagickRealType) pixels[x].green+(pixel.green*amount);
          unsharp_pixels[x].green=RoundToQuantum(pixel.green);
        }
      if ((channel & BlueChannel) != 0)
        {
          pixel.blue=pixels[x].blue-(MagickRealType) unsharp_pixels[x].blue;
          if (fabs(2.0*pixel.blue) < quantum_threshold)
            pixel.blue=(MagickRealType) pixels[x].blue;
          else
            pixel.blue=(MagickRealType) pixels[x].blue+(pixel.blue*amount);
          unsharp_pixels[x].blue=RoundToQuantum(pixel.blue);
        }
      if ((channel & OpacityChannel) != 0)
        {
          pixel.opacity=pixels[x].opacity-(MagickRealType)
            unsharp_pixels[x].opacity;
          if (fabs(2.0*pixel.opacity) < quantum_threshold)
            pixel.opacity=(MagickRealType) pixels[x].opacity;
          else
            pixel.opacity=pixels[x].opacity+(pixel.opacity*amount);
          unsharp_pixels[x].opacity=RoundToQuantum(pixel.opacity);
        }
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        {
          pixel.index=unsharp_indexes[x]-(MagickRealType) indexes[x];
          if (fabs(2.0*pixel.index) < quantum_threshold)
            pixel.index=(MagickRealType) unsharp_indexes[x];
          else
            pixel.index=(MagickRealType) unsharp_indexes[x]+
              (pixel.index*amount);
          unsharp_indexes[x]=RoundToQuantum(pixel.index);
        }
    }
    if (SyncImagePixels(unsharp_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SharpenImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(unsharp_image);
}
