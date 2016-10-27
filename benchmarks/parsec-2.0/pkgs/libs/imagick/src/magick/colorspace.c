/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     CCCC   OOO   L       OOO   RRRR   SSSSS  PPPP    AAA    CCCC  EEEEE     %
%    C      O   O  L      O   O  R   R  SS     P   P  A   A  C      E         %
%    C      O   O  L      O   O  RRRR    SSS   PPPP   AAAAA  C      EEE       %
%    C      O   O  L      O   O  R R       SS  P      A   A  C      E         %
%     CCCC   OOO   LLLLL   OOO   R  R   SSSSS  P      A   A   CCCC  EEEEE     %
%                                                                             %
%                                                                             %
%                   ImageMagick Image Colorspace Methods                      %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1992                                   %
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
#include "magick/cache.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/colorspace-private.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/gem.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/pixel-private.h"
#include "magick/quantize.h"
#include "magick/quantum.h"
#include "magick/string_.h"
#include "magick/utility.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     R G B T r a n s f o r m I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RGBTransformImage() converts the reference image from RGB to an alternate
%  colorspace.  The transformation matrices are not the standard ones: the
%  weights are rescaled to normalized the range of the transformed values to
%  be [0..QuantumRange].
%
%  The format of the RGBTransformImage method is:
%
%      MagickBooleanType RGBTransformImage(Image *image,
%        const ColorspaceType colorspace)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o colorspace: the colorspace to transform the image to.
%
*/

static inline void ConvertRGBToXYZ(const Quantum red,const Quantum green,
  const Quantum blue,double *X,double *Y,double *Z)
{
  double
    b,
    g,
    r;

  assert(X != (double *) NULL);
  assert(Y != (double *) NULL);
  assert(Z != (double *) NULL);
  r=QuantumScale*red;
  g=QuantumScale*green;
  b=QuantumScale*blue;
  *X=0.4124240*r+0.3575790*g+0.1804640*b;
  *Y=0.2126560*r+0.7151580*g+0.0721856*b;
  *Z=0.0193324*r+0.1191930*g+0.9504440*b;
}

static inline void ConvertXYZToLab(const double X,const double Y,const double Z,
  double *L,double *a,double *b)
{
  double
    x,
    y,
    z;

  assert(L != (double *) NULL);
  assert(a != (double *) NULL);
  assert(b != (double *) NULL);
  x=X/0.9504559271;
  if (x > (216/24389.0))
    x=pow(x,1.0/3.0);
  else 
    x=(7.787*x)+(16.0/116.0);
  y=Y/1.00000;
  if (y > (216/24389.0))
    y=pow(y,1.0/3.0);
  else
    y=(7.787*y)+(16.0/116.0);
  z=Z/1.0890577508;
  if (z > (216/24389.0))
    z=pow(z,1.0/3.0);
  else
    z=(7.787*z)+(16.0/116.0);
  *L=0.5*((1.160*y)-0.160+1.0);
  *a=0.5*(5.000*(x-y)+1.0);
  *b=0.5*(2.000*(y-z)+1.0);
}

MagickExport MagickBooleanType RGBTransformImage(Image *image,
  const ColorspaceType colorspace)
{
#define RGBTransformImageTag  "RGBTransform/Image"

  long
    y;

  IndexPacket
    *indexes;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  PrimaryInfo
    primary_info,
    *x_map,
    *y_map,
    *z_map;

  register long
    i,
    x;

  register PixelPacket
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(colorspace != RGBColorspace);
  assert(colorspace != TransparentColorspace);
  assert(colorspace != UndefinedColorspace);
  switch (image->colorspace)
  {
    case GRAYColorspace:
    case Rec601LumaColorspace:
    case Rec709LumaColorspace:
    case RGBColorspace:
    case TransparentColorspace:
      break;
    default:
    {
      (void) SetImageColorspace(image,image->colorspace);
      break;
    }
  }
  image->colorspace=colorspace;
  (void) SetImagePixels(image,0,0,image->columns,1);
  switch (colorspace)
  {
    case CMYColorspace:
    {
      /*
        Convert RGB to CMY colorspace.
      */
      if (image->storage_class == PseudoClass)
        {
          if (SyncImage(image) == MagickFalse)
            return(MagickFalse);
          if (SetImageStorageClass(image,DirectClass) == MagickFalse)
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
          q->red=RoundToQuantum((MagickRealType) QuantumRange-q->red);
          q->green=RoundToQuantum((MagickRealType) QuantumRange-q->green);
          q->blue=RoundToQuantum((MagickRealType) QuantumRange-q->blue);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      return(MagickTrue);
    }
    case CMYKColorspace:
    {
      MagickPixelPacket
        pixel;

      /*
        Convert RGB to CMYK colorspace.
      */
      if (image->storage_class == PseudoClass)
        {
          if (SyncImage(image) == MagickFalse)
            return(MagickFalse);
          if (SetImageStorageClass(image,DirectClass) == MagickFalse)
            return(MagickFalse);
        }
      GetMagickPixelPacket(image,&pixel);
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) image->columns; x++)
        {
          SetMagickPixelPacket(image,q,indexes+x,&pixel);
          ConvertRGBToCMYK(&pixel);
          SetPixelPacket(image,&pixel,q,indexes+x);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      return(MagickTrue);
    }
    case HSBColorspace:
    {
      double
        brightness,
        hue,
        saturation;

      /*
        Transform image from RGB to HSB.
      */
      if (image->storage_class == PseudoClass)
        {
          if (SyncImage(image) == MagickFalse)
            return(MagickFalse);
          if (SetImageStorageClass(image,DirectClass) == MagickFalse)
            return(MagickFalse);
        }
      hue=0.0;
      saturation=0.0;
      brightness=0.0;
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          ConvertRGBToHSB(q->red,q->green,q->blue,&hue,&saturation,&brightness);
          q->red=RoundToQuantum((MagickRealType) QuantumRange*hue);
          q->green=RoundToQuantum((MagickRealType) QuantumRange*saturation);
          q->blue=RoundToQuantum((MagickRealType) QuantumRange*brightness);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      return(MagickTrue);
    }
    case HSLColorspace:
    {
      double
        hue,
        luminosity,
        saturation;

      /*
        Transform image from RGB to HSL.
      */
      if (image->storage_class == PseudoClass)
        {
          if (SyncImage(image) == MagickFalse)
            return(MagickFalse);
          if (SetImageStorageClass(image,DirectClass) == MagickFalse)
            return(MagickFalse);
        }
      hue=0.0;
      saturation=0.0;
      luminosity=0.0;
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          ConvertRGBToHSL(q->red,q->green,q->blue,&hue,&saturation,&luminosity);
          q->red=RoundToQuantum((MagickRealType) QuantumRange*hue);
          q->green=RoundToQuantum((MagickRealType) QuantumRange*saturation);
          q->blue=RoundToQuantum((MagickRealType) QuantumRange*luminosity);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      return(MagickTrue);
    }
    case HWBColorspace:
    {
      double
        blackness,
        hue,
        whiteness;

      /*
        Transform image from RGB to HWB.
      */
      if (image->storage_class == PseudoClass)
        {
          if (SyncImage(image) == MagickFalse)
            return(MagickFalse);
          if (SetImageStorageClass(image,DirectClass) == MagickFalse)
            return(MagickFalse);
        }
      hue=0.0;
      whiteness=0.0;
      blackness=0.0;
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          ConvertRGBToHWB(q->red,q->green,q->blue,&hue,&whiteness,&blackness);
          q->red=RoundToQuantum((MagickRealType) QuantumRange*hue);
          q->green=RoundToQuantum((MagickRealType) QuantumRange*whiteness);
          q->blue=RoundToQuantum((MagickRealType) QuantumRange*blackness);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      return(MagickTrue);
    }
    case LabColorspace:
    {
      double
        a,
        b,
        L,
        X,
        Y,
        Z;

      /*
        Transform image from RGB to Lab.
      */
      if (image->storage_class == PseudoClass)
        {
          if (SyncImage(image) == MagickFalse)
            return(MagickFalse);
          if (SetImageStorageClass(image,DirectClass) == MagickFalse)
            return(MagickFalse);
        }
      L=0.0;
      a=0.0;
      b=0.0;
      X=0.0;
      Y=0.0;
      Z=0.0;
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          ConvertRGBToXYZ(q->red,q->green,q->blue,&X,&Y,&Z);
          ConvertXYZToLab(X,Y,Z,&L,&a,&b);
          q->red=RoundToQuantum((MagickRealType) QuantumRange*L);
          q->green=RoundToQuantum((MagickRealType) QuantumRange*a);
          q->blue=RoundToQuantum((MagickRealType) QuantumRange*b);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      return(MagickTrue);
    }
    case LogColorspace:
    {
#define ReferenceBlack  95.0
#define ReferenceWhite  685.0
#define DisplayGamma  (1.0/1.7)

      const char
        *value;

      double
        black,
        density,
        gamma,
        reference_black,
        reference_white;

      Quantum
        *logmap;

      /*
        Transform RGB to Log colorspace.
      */
      density=2.03728;
      gamma=DisplayGamma;
      value=GetImageProperty(image,"Gamma");
      if (value != (const char *) NULL)
        gamma=1.0/atof(value) != 0.0 ? atof(value) : 1.0;
      reference_black=ReferenceBlack;
      value=GetImageProperty(image,"reference-black");
      if (value != (const char *) NULL)
        reference_black=atof(value);
      reference_white=ReferenceWhite;
      value=GetImageProperty(image,"reference-white");
      if (value != (const char *) NULL)
        reference_white=atof(value);
      logmap=(Quantum *) AcquireQuantumMemory((size_t) MaxMap+1UL,
        sizeof(*logmap));
      if (logmap == (Quantum *) NULL)
        ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
          image->filename);
      black=pow(10.0,(reference_black-reference_white)*(gamma/density)*
        0.002/0.6);
      for (i=0; i <= (long) MaxMap; i++)
        logmap[i]=ScaleMapToQuantum((MagickRealType) (MaxMap*(reference_white+
          log10(black+((double) i/MaxMap)*(1.0-black))/((gamma/density)*
          0.002/0.6))/1024.0+0.5));
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=(long) image->columns; x != 0; x--)
        {
          q->red=logmap[ScaleQuantumToMap(q->red)];
          q->green=logmap[ScaleQuantumToMap(q->green)];
          q->blue=logmap[ScaleQuantumToMap(q->blue)];
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      logmap=(Quantum *) RelinquishMagickMemory(logmap);
      return(y == (long) image->rows ? MagickTrue : MagickFalse);
    }
    default:
      break;
  }
  /*
    Allocate the tables.
  */
  x_map=(PrimaryInfo *) AcquireQuantumMemory((size_t) MaxMap+1UL,
    sizeof(*x_map));
  y_map=(PrimaryInfo *) AcquireQuantumMemory((size_t) MaxMap+1UL,
    sizeof(*y_map));
  z_map=(PrimaryInfo *) AcquireQuantumMemory((size_t) MaxMap+1UL,
    sizeof(*z_map));
  if ((x_map == (PrimaryInfo *) NULL) || (y_map == (PrimaryInfo *) NULL) ||
      (z_map == (PrimaryInfo *) NULL))
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  (void) ResetMagickMemory(&primary_info,0,sizeof(primary_info));
  switch (colorspace)
  {
    case OHTAColorspace:
    {
      /*
        Initialize OHTA tables:

          I1 = 0.33333*R+0.33334*G+0.33333*B
          I2 = 0.50000*R+0.00000*G-0.50000*B
          I3 =-0.25000*R+0.50000*G-0.25000*B

        I and Q, normally -0.5 through 0.5, are normalized to the range 0
        through QuantumRange.
      */
      primary_info.y=(double) (MaxMap+1.0)/2.0;
      primary_info.z=(double) (MaxMap+1.0)/2.0;
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.33333*i;
        y_map[i].x=0.33334*i;
        z_map[i].x=0.33333*i;
        x_map[i].y=0.50000*i;
        y_map[i].y=0.00000*i;
        z_map[i].y=(-0.50000)*i;
        x_map[i].z=(-0.25000)*i;
        y_map[i].z=0.50000*i;
        z_map[i].z=(-0.25000)*i;
      }
      break;
    }
    case Rec601LumaColorspace:
    case GRAYColorspace:
    {
      /*
        Initialize Rec601 luma tables:

          G = 0.29900*R+0.58700*G+0.11400*B
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.29900*i;
        y_map[i].x=0.58700*i;
        z_map[i].x=0.11400*i;
        x_map[i].y=0.29900*i;
        y_map[i].y=0.58700*i;
        z_map[i].y=0.11400*i;
        x_map[i].z=0.29900*i;
        y_map[i].z=0.58700*i;
        z_map[i].z=0.11400*i;
      }
      break;
    }
    case Rec601YCbCrColorspace:
    case YCbCrColorspace:
    {
      /*
        Initialize YCbCr tables (ITU-R BT.601):

          Y =  0.299000*R+0.587000*G+0.114000*B
          Cb= -0.168736*R-0.331264*G+0.500000*B
          Cr=  0.500000*R-0.418688*G-0.081312*B

        Cb and Cr, normally -0.5 through 0.5, are normalized to the range 0
        through QuantumRange.
      */
      primary_info.y=(double) (MaxMap+1.0)/2.0;
      primary_info.z=(double) (MaxMap+1.0)/2.0;
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.299000*i;
        y_map[i].x=0.587000*i;
        z_map[i].x=0.114000*i;
        x_map[i].y=(-0.168730)*i;
        y_map[i].y=(-0.331264)*i;
        z_map[i].y=0.500000*i;
        x_map[i].z=0.500000*i;
        y_map[i].z=(-0.418688)*i;
        z_map[i].z=(-0.081312)*i;
      }
      break;
    }
    case Rec709LumaColorspace:
    {
      /*
        Initialize Rec709 luma tables:

          G = 0.21260*R+0.71520*G+0.07220*B
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.21260*i;
        y_map[i].x=0.71520*i;
        z_map[i].x=0.07220*i;
        x_map[i].y=0.21260*i;
        y_map[i].y=0.71520*i;
        z_map[i].y=0.07220*i;
        x_map[i].z=0.21260*i;
        y_map[i].z=0.71520*i;
        z_map[i].z=0.07220*i;
      }
      break;
    }
    case Rec709YCbCrColorspace:
    {
      /*
        Initialize YCbCr tables (ITU-R BT.709):

          Y =  0.212600*R+0.715200*G+0.072200*B
          Cb= -0.114572*R-0.385428*G+0.500000*B
          Cr=  0.500000*R-0.454153*G-0.045847*B

        Cb and Cr, normally -0.5 through 0.5, are normalized to the range 0
        through QuantumRange.
      */
      primary_info.y=(double) (MaxMap+1.0)/2.0;
      primary_info.z=(double) (MaxMap+1.0)/2.0;
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.212600*i;
        y_map[i].x=0.715200*i;
        z_map[i].x=0.072200*i;
        x_map[i].y=(-0.114572)*i;
        y_map[i].y=(-0.385428)*i;
        z_map[i].y=0.500000*i;
        x_map[i].z=0.500000*i;
        y_map[i].z=(-0.454153)*i;
        z_map[i].z=(-0.045847)*i;
      }
      break;
    }
    case sRGBColorspace:
    {
      double
        v;

      /*
        Linear RGB to nonlinear sRGB (http://www.w3.org/Graphics/Color/sRGB):

          R = 1.0*R+0.0*G+0.0*B
          G = 0.0*R+0.1*G+0.0*B
          B = 0.0*R+0.0*G+1.0*B
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        v=(double) i/MaxMap;
        if (((double) i/MaxMap) <= 0.03928)
          v/=12.92;
        else
          v=(double) MaxMap*pow((((double) i/MaxMap)+0.055)/1.055,2.4);
        x_map[i].x=1.0*v;
        y_map[i].x=0.0*v;
        z_map[i].x=0.0*v;
        x_map[i].y=0.0*v;
        y_map[i].y=1.0*v;
        z_map[i].y=0.0*v;
        x_map[i].z=0.0*v;
        y_map[i].z=0.0*v;
        z_map[i].z=1.0*v;
      }
      break;
    }
    case XYZColorspace:
    {
      /*
        Initialize CIE XYZ tables (ITU-R 709 RGB):

          X = 0.4124240*R+0.3575790*G+0.1804640*B
          Y = 0.2126560*R+0.7151580*G+0.0721856*B
          Z = 0.0193324*R+0.1191930*G+0.9504440*B
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.4124240*i;
        y_map[i].x=0.3575790*i;
        z_map[i].x=0.1804640*i;
        x_map[i].y=0.2126560*i;
        y_map[i].y=0.7151580*i;
        z_map[i].y=0.0721856*i;
        x_map[i].z=0.0193324*i;
        y_map[i].z=0.1191930*i;
        z_map[i].z=0.9504440*i;
      }
      break;
    }
    case YCCColorspace:
    {
      /*
        Initialize YCC tables:

          Y =  0.29900*R+0.58700*G+0.11400*B
          C1= -0.29900*R-0.58700*G+0.88600*B
          C2=  0.70100*R-0.58700*G-0.11400*B

        YCC is scaled by 1.3584.  C1 zero is 156 and C2 is at 137.
      */
      primary_info.y=(double) ScaleQuantumToMap(ScaleCharToQuantum(156));
      primary_info.z=(double) ScaleQuantumToMap(ScaleCharToQuantum(137));
      for (i=0; i <= (long) (0.018*MaxMap); i++)
      {
        x_map[i].x=0.003962014134275617*i;
        y_map[i].x=0.007778268551236748*i;
        z_map[i].x=0.001510600706713781*i;
        x_map[i].y=(-0.002426619775463276)*i;
        y_map[i].y=(-0.004763965913702149)*i;
        z_map[i].y=0.007190585689165425*i;
        x_map[i].z=0.006927257754597858*i;
        y_map[i].z=(-0.005800713697502058)*i;
        z_map[i].z=(-0.0011265440570958)*i;
      }
      for ( ; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.2201118963486454*(1.099*i-0.099);
        y_map[i].x=0.4321260306242638*(1.099*i-0.099);
        z_map[i].x=0.08392226148409894*(1.099*i-0.099);
        x_map[i].y=(-0.1348122097479598)*(1.099*i-0.099);
        y_map[i].y=(-0.2646647729834528)*(1.099*i-0.099);
        z_map[i].y=0.3994769827314126*(1.099*i-0.099);
        x_map[i].z=0.3848476530332144*(1.099*i-0.099);
        y_map[i].z=(-0.3222618720834477)*(1.099*i-0.099);
        z_map[i].z=(-0.06258578094976668)*(1.099*i-0.099);
      }
      break;
    }
    case YIQColorspace:
    {
      /*
        Initialize YIQ tables:

          Y = 0.29900*R+0.58700*G+0.11400*B
          I = 0.59600*R-0.27400*G-0.32200*B
          Q = 0.21100*R-0.52300*G+0.31200*B

        I and Q, normally -0.5 through 0.5, are normalized to the range 0
        through QuantumRange.
      */
      primary_info.y=(double) (MaxMap+1.0)/2.0;
      primary_info.z=(double) (MaxMap+1.0)/2.0;
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.29900*i;
        y_map[i].x=0.58700*i;
        z_map[i].x=0.11400*i;
        x_map[i].y=0.59600*i;
        y_map[i].y=(-0.27400)*i;
        z_map[i].y=(-0.32200)*i;
        x_map[i].z=0.21100*i;
        y_map[i].z=(-0.52300)*i;
        z_map[i].z=0.31200*i;
      }
      break;
    }
    case YPbPrColorspace:
    {
      /*
        Initialize YPbPr tables (ITU-R BT.601):

          Y =  0.299000*R+0.587000*G+0.114000*B
          Pb= -0.168736*R-0.331264*G+0.500000*B
          Pr=  0.500000*R-0.418688*G-0.081312*B

        Pb and Pr, normally -0.5 through 0.5, are normalized to the range 0
        through QuantumRange.
      */
      primary_info.y=(double) (MaxMap+1.0)/2.0;
      primary_info.z=(double) (MaxMap+1.0)/2.0;
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.299000*i;
        y_map[i].x=0.587000*i;
        z_map[i].x=0.114000*i;
        y_map[i].y=(-0.331264)*i;
        x_map[i].y=(-0.168736)*i;
        z_map[i].y=0.500000*i;
        x_map[i].z=0.500000*i;
        y_map[i].z=(-0.418688)*i;
        z_map[i].z=(-0.081312)*i;
      }
      break;
    }
    case YUVColorspace:
    default:
    {
      /*
        Initialize YUV tables:

          Y =  0.29900*R+0.58700*G+0.11400*B
          U = -0.14740*R-0.28950*G+0.43690*B
          V =  0.61500*R-0.51500*G-0.10000*B

        U and V, normally -0.5 through 0.5, are normalized to the range 0
        through QuantumRange.  Note that U = 0.493*(B-Y), V = 0.877*(R-Y).
      */
      primary_info.y=(double) (MaxMap+1.0)/2.0;
      primary_info.z=(double) (MaxMap+1.0)/2.0;
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.29900*i;
        y_map[i].x=0.58700*i;
        z_map[i].x=0.11400*i;
        x_map[i].y=(-0.14740)*i;
        y_map[i].y=(-0.28950)*i;
        z_map[i].y=0.43690*i;
        x_map[i].z=0.61500*i;
        y_map[i].z=(-0.51500)*i;
        z_map[i].z=(-0.10000)*i;
      }
      break;
    }
  }
  /*
    Convert from RGB.
  */
  switch (image->storage_class)
  {
    case DirectClass:
    default:
    {
      /*
        Convert DirectClass image.
      */
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          pixel.red=x_map[ScaleQuantumToMap(q->red)].x+
            y_map[ScaleQuantumToMap(q->green)].x+
            z_map[ScaleQuantumToMap(q->blue)].x+primary_info.x;
          pixel.green=x_map[ScaleQuantumToMap(q->red)].y+
            y_map[ScaleQuantumToMap(q->green)].y+
            z_map[ScaleQuantumToMap(q->blue)].y+primary_info.y;
          pixel.blue=x_map[ScaleQuantumToMap(q->red)].z+
            y_map[ScaleQuantumToMap(q->green)].z+
            z_map[ScaleQuantumToMap(q->blue)].z+primary_info.z;
          q->red=ScaleMapToQuantum(pixel.red);
          q->green=ScaleMapToQuantum(pixel.green);
          q->blue=ScaleMapToQuantum(pixel.blue);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(y,image->rows) != MagickFalse))
          {
            status=image->progress_monitor(RGBTransformImageTag,y,image->rows,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
      break;
    }
    case PseudoClass:
    {
      /*
        Convert PseudoClass image.
      */
      for (i=0; i < (long) image->colors; i++)
      {
        pixel.red=x_map[ScaleQuantumToMap(image->colormap[i].red)].x+
          y_map[ScaleQuantumToMap(image->colormap[i].green)].x+
          z_map[ScaleQuantumToMap(image->colormap[i].blue)].x+primary_info.x;
        pixel.green=x_map[ScaleQuantumToMap(image->colormap[i].red)].y+
          y_map[ScaleQuantumToMap(image->colormap[i].green)].y+
          z_map[ScaleQuantumToMap(image->colormap[i].blue)].y+primary_info.y;
        pixel.blue=x_map[ScaleQuantumToMap(image->colormap[i].red)].z+
          y_map[ScaleQuantumToMap(image->colormap[i].green)].z+
          z_map[ScaleQuantumToMap(image->colormap[i].blue)].z+primary_info.z;
        image->colormap[i].red=ScaleMapToQuantum(pixel.red);
        image->colormap[i].green=ScaleMapToQuantum(pixel.green);
        image->colormap[i].blue=ScaleMapToQuantum(pixel.blue);
      }
      (void) SyncImage(image);
      break;
    }
  }
  /*
    Free resources.
  */
  z_map=(PrimaryInfo *) RelinquishMagickMemory(z_map);
  y_map=(PrimaryInfo *) RelinquishMagickMemory(y_map);
  x_map=(PrimaryInfo *) RelinquishMagickMemory(x_map);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e C o l o r s p a c e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageColorspace() sets the colorspace member of the Image structure.
%
%  The format of the SetImageColorspace method is:
%
%      MagickBooleanType SetImageColorspace(Image *image,
%        const ColorspaceType colorspace)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o colorspace: The colorspace.
%
*/
MagickExport MagickBooleanType SetImageColorspace(Image *image,
  const ColorspaceType colorspace)
{
  MagickBooleanType
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (colorspace == UndefinedColorspace)
    {
      image->colorspace=UndefinedColorspace;
      return(MagickTrue);
    }
  if (image->colorspace == colorspace)
    return(MagickTrue);
  if ((colorspace == RGBColorspace) || (colorspace == TransparentColorspace))
    return(TransformRGBImage(image,image->colorspace));
  status=MagickTrue;
  if ((image->colorspace != RGBColorspace) &&
      (image->colorspace != TransparentColorspace) &&
      (image->colorspace != GRAYColorspace))
    status=TransformRGBImage(image,image->colorspace);
  if (RGBTransformImage(image,colorspace) == MagickFalse)
    status=MagickFalse;
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     T r a n s f o r m R G B I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TransformRGBImage() converts the reference image from an alternate
%  colorspace to RGB.  The transformation matrices are not the standard ones:
%  the weights are rescaled to normalize the range of the transformed values to
%  be [0..QuantumRange].
%
%  The format of the TransformRGBImage method is:
%
%      MagickBooleanType TransformRGBImage(Image *image,
%        const ColorspaceType colorspace)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o colorspace: the colorspace to transform the image to.
%
*/

static inline void ConvertLabToXYZ(const double L,const double a,const double b,
  double *X,double *Y,double *Z)
{
  double
    x,
    y,
    z;

  assert(X != (double *) NULL);
  assert(Y != (double *) NULL);
  assert(Z != (double *) NULL);
  y=((2.0*L-1.0)+0.160)/1.160;
  x=(2.0*a-1.0)/5.000+y;
  z=y-(2.0*b-1.0)/2.000;
  if ((x*x*x) > (216.0/24389.0))
    x=x*x*x;
  else
    x=(x-16.0/116.0)/7.787;
  if ((y*y*y) > (216.0/24389.0))
    y=y*y*y;
  else
    y=(y-16.0/116.0)/7.787;
  if ((z*z*z) > (216.0/24389.0))
    z=z*z*z;
  else
    z=(z-16.0/116.0)/7.787;
  *X=0.9504559271*x;
  *Y=1.0000000000*y;
  *Z=1.0890577508*z;
}

static inline unsigned short RoundToYCC(const MagickRealType value)
{
  if (value <= 0.0)
    return(0UL);
  if (value >= 351.0)
    return(351);
  return((unsigned short) (value+0.5));
}

static inline void ConvertXYZToRGB(const double x,const double y,const double z,
  Quantum *red,Quantum *green,Quantum *blue)
{
  double
    b,
    g,
    r;

  /*
    Convert XYZ to RGB colorspace.
  */
  assert(red != (Quantum *) NULL);
  assert(green != (Quantum *) NULL);
  assert(blue != (Quantum *) NULL);
  r=3.2407100*x-1.5372600*y-0.4985710*z;
  g=(-0.9692580*x+1.8759900*y+0.0415557*z);
  b=0.0556352*x-0.2039960*y+1.0570700*z;
  *red=RoundToQuantum((MagickRealType) QuantumRange*r);
  *green=RoundToQuantum((MagickRealType) QuantumRange*g);
  *blue=RoundToQuantum((MagickRealType) QuantumRange*b);
}

static inline void ConvertCMYKToRGB(MagickPixelPacket *pixel)
{
  pixel->red=(MagickRealType) QuantumRange-(QuantumScale*pixel->red*
    (QuantumRange-pixel->index)+pixel->index);
  pixel->green=(MagickRealType) QuantumRange-(QuantumScale*pixel->green*
    (QuantumRange-pixel->index)+pixel->index);
  pixel->blue=(MagickRealType) QuantumRange-(QuantumScale*pixel->blue*
    (QuantumRange-pixel->index)+pixel->index);
}

MagickExport MagickBooleanType TransformRGBImage(Image *image,
  const ColorspaceType colorspace)
{
#define D50X  (0.9642)
#define D50Y  (1.0)
#define D50Z  (0.8249)
#define TransformRGBImageTag  "Transform/Image"

#if !defined(UseHDRI)
  static const unsigned char
    YCCMap[351] =  /* Photo CD information beyond 100% white, Gamma 2.2 */
    {
        0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,
       14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,
       28,  29,  30,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,
       43,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  56,  57,  58,
       59,  60,  61,  62,  63,  64,  66,  67,  68,  69,  70,  71,  72,  73,
       74,  76,  77,  78,  79,  80,  81,  82,  83,  84,  86,  87,  88,  89,
       90,  91,  92,  93,  94,  95,  97,  98,  99, 100, 101, 102, 103, 104,
      105, 106, 107, 108, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
      120, 121, 122, 123, 124, 125, 126, 127, 129, 130, 131, 132, 133, 134,
      135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148,
      149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162,
      163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176,
      176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
      190, 191, 192, 193, 193, 194, 195, 196, 197, 198, 199, 200, 201, 201,
      202, 203, 204, 205, 206, 207, 207, 208, 209, 210, 211, 211, 212, 213,
      214, 215, 215, 216, 217, 218, 218, 219, 220, 221, 221, 222, 223, 224,
      224, 225, 226, 226, 227, 228, 228, 229, 230, 230, 231, 232, 232, 233,
      234, 234, 235, 236, 236, 237, 237, 238, 238, 239, 240, 240, 241, 241,
      242, 242, 243, 243, 244, 244, 245, 245, 245, 246, 246, 247, 247, 247,
      248, 248, 248, 249, 249, 249, 249, 250, 250, 250, 250, 251, 251, 251,
      251, 251, 252, 252, 252, 252, 252, 253, 253, 253, 253, 253, 253, 253,
      253, 253, 253, 253, 253, 253, 254, 254, 254, 254, 254, 254, 254, 254,
      254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255
    };
#endif

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  PrimaryInfo
    *y_map,
    *x_map,
    *z_map;

  register long
    x;

  register long
    i;

  register PixelPacket
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  switch (colorspace)
  {
    case GRAYColorspace:
    case Rec601LumaColorspace:
    case Rec709LumaColorspace:
    case RGBColorspace:
    case TransparentColorspace:
    case UndefinedColorspace:
      return(MagickTrue);
    default:
      break;
  }
  switch (colorspace)
  {
    case CMYColorspace:
    {
      /*
        Transform image from CMY to RGB.
      */
      if (image->storage_class == PseudoClass)
        {
          if (SyncImage(image) == MagickFalse)
            return(MagickFalse);
          if (SetImageStorageClass(image,DirectClass) == MagickFalse)
            return(MagickFalse);
        }
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          q->red=RoundToQuantum((MagickRealType) QuantumRange-q->red);
          q->green=RoundToQuantum((MagickRealType) QuantumRange-q->green);
          q->blue=RoundToQuantum((MagickRealType) QuantumRange-q->blue);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      image->colorspace=RGBColorspace;
      return(y == (long) image->rows ? MagickTrue : MagickFalse);
    }
    case CMYKColorspace:
    {
      IndexPacket
        *indexes;

      MagickPixelPacket
        pixel;

      /*
        Transform image from CMYK to RGB.
      */
      if (image->storage_class == PseudoClass)
        {
          if (SyncImage(image) == MagickFalse)
            return(MagickFalse);
          if (SetImageStorageClass(image,DirectClass) == MagickFalse)
            return(MagickFalse);
        }
      GetMagickPixelPacket(image,&pixel);
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) image->columns; x++)
        {
          SetMagickPixelPacket(image,q,indexes+x,&pixel);
          ConvertCMYKToRGB(&pixel);
          SetPixelPacket(image,&pixel,q,indexes+x);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      image->colorspace=RGBColorspace;
      (void) SetImagePixels(image,0,0,image->columns,1);
      return(y == (long) image->rows ? MagickTrue : MagickFalse);
    }
    case HSBColorspace:
    {
      double
        brightness,
        hue,
        saturation;

      /*
        Transform image from HSB to RGB.
      */
      if (image->storage_class == PseudoClass)
        {
          if (SyncImage(image) == MagickFalse)
            return(MagickFalse);
          if (SetImageStorageClass(image,DirectClass) == MagickFalse)
            return(MagickFalse);
        }
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          hue=(double) (QuantumScale*q->red);
          saturation=(double) (QuantumScale*q->green);
          brightness=(double) (QuantumScale*q->blue);
          ConvertHSBToRGB(hue,saturation,brightness,&q->red,&q->green,&q->blue);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      image->colorspace=RGBColorspace;
      return(y == (long) image->rows ? MagickTrue : MagickFalse);
    }
    case HSLColorspace:
    {
      double
        hue,
        luminosity,
        saturation;

      /*
        Transform image from HSL to RGB.
      */
      if (image->storage_class == PseudoClass)
        {
          if (SyncImage(image) == MagickFalse)
            return(MagickFalse);
          if (SetImageStorageClass(image,DirectClass) == MagickFalse)
            return(MagickFalse);
        }
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          hue=(double) (QuantumScale*q->red);
          saturation=(double) (QuantumScale*q->green);
          luminosity=(double) (QuantumScale*q->blue);
          ConvertHSLToRGB(hue,saturation,luminosity,&q->red,&q->green,&q->blue);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      image->colorspace=RGBColorspace;
      return(y == (long) image->rows ? MagickTrue : MagickFalse);
    }
    case HWBColorspace:
    {
      double
        blackness,
        hue,
        whiteness;

      /*
        Transform image from HWB to RGB.
      */
      if (image->storage_class == PseudoClass)
        {
          if (SyncImage(image) == MagickFalse)
            return(MagickFalse);
          if (SetImageStorageClass(image,DirectClass) == MagickFalse)
            return(MagickFalse);
        }
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          hue=(double) (QuantumScale*q->red);
          whiteness=(double) (QuantumScale*q->green);
          blackness=(double) (QuantumScale*q->blue);
          ConvertHWBToRGB(hue,whiteness,blackness,&q->red,&q->green,&q->blue);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      image->colorspace=RGBColorspace;
      return(y == (long) image->rows ? MagickTrue : MagickFalse);
    }
    case LabColorspace:
    {
      double
        a,
        b,
        L,
        X,
        Y,
        Z;

      /*
        Transform image from Lab to RGB.
      */
      if (image->storage_class == PseudoClass)
        {
          if (SyncImage(image) == MagickFalse)
            return(MagickFalse);
          if (SetImageStorageClass(image,DirectClass) == MagickFalse)
            return(MagickFalse);
        }
      L=0.0;
      a=0.0;
      b=0.0;
      X=0.0;
      Y=0.0;
      Z=0.0;
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          L=QuantumScale*q->red;
          a=QuantumScale*q->green;
          b=QuantumScale*q->blue;
          ConvertLabToXYZ(L,a,b,&X,&Y,&Z);
          ConvertXYZToRGB(X,Y,Z,&q->red,&q->green,&q->blue);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      image->colorspace=RGBColorspace;
      return(y == (long) image->rows ? MagickTrue : MagickFalse);
    }
    case LogColorspace:
    {
      const char
        *value;

      double
        black,
        density,
        gamma,
        reference_black,
        reference_white;

      Quantum
        *logmap;

      /*
        Transform Log to RGB colorspace.
      */
      density=2.03728;
      gamma=DisplayGamma;
      value=GetImageProperty(image,"Gamma");
      if (value != (const char *) NULL)
        gamma=1.0/atof(value) != 0.0 ? atof(value) : 1.0;
      reference_black=ReferenceBlack;
      value=GetImageProperty(image,"reference-black");
      if (value != (const char *) NULL)
        reference_black=atof(value);
      reference_white=ReferenceWhite;
      value=GetImageProperty(image,"reference-white");
      if (value != (const char *) NULL)
        reference_white=atof(value);
      logmap=(Quantum *) AcquireQuantumMemory((size_t) MaxMap+1UL,
        sizeof(*logmap));
      if (logmap == (Quantum *) NULL)
        ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
          image->filename);
      black=pow(10.0,(reference_black-reference_white)*(gamma/density)*
        0.002/0.6);
      for (i=0; i <= (long) (reference_black*MaxMap/1024.0); i++)
        logmap[i]=(Quantum) 0;
      for ( ; i < (long) (reference_white*MaxMap/1024.0); i++)
        logmap[i]=RoundToQuantum((MagickRealType) QuantumRange/(1.0-black)*
          (pow(10.0,(1024.0*i/MaxMap-reference_white)*
          (gamma/density)*0.002/0.6)-black));
      for ( ; i <= (long) MaxMap; i++)
        logmap[i]=(Quantum) QuantumRange;
      if (SetImageStorageClass(image,DirectClass) == MagickFalse)
        return(MagickFalse);
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=(long) image->columns; x != 0; x--)
        {
          q->red=logmap[ScaleQuantumToMap(q->red)];
          q->green=logmap[ScaleQuantumToMap(q->green)];
          q->blue=logmap[ScaleQuantumToMap(q->blue)];
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      logmap=(Quantum *) RelinquishMagickMemory(logmap);
      image->colorspace=RGBColorspace;
      return(y == (long) image->rows ? MagickTrue : MagickFalse);
    }
    default:
      break;
  }
  /*
    Allocate the tables.
  */
  x_map=(PrimaryInfo *) AcquireQuantumMemory((size_t) MaxMap+1UL,
    sizeof(*x_map));
  y_map=(PrimaryInfo *) AcquireQuantumMemory((size_t) MaxMap+1UL,
    sizeof(*y_map));
  z_map=(PrimaryInfo *) AcquireQuantumMemory((size_t) MaxMap+1UL,
    sizeof(*z_map));
  if ((x_map == (PrimaryInfo *) NULL) || (y_map == (PrimaryInfo *) NULL) ||
      (z_map == (PrimaryInfo *) NULL))
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  switch (colorspace)
  {
    case OHTAColorspace:
    {
      /*
        Initialize OHTA tables:

          R = I1+1.00000*I2-0.66668*I3
          G = I1+0.00000*I2+1.33333*I3
          B = I1-1.00000*I2-0.66668*I3

        I and Q, normally -0.5 through 0.5, must be normalized to the range 0
        through QuantumRange.
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=(double) i;
        y_map[i].x=0.500000*(2.000000*i-MaxMap);
        z_map[i].x=(-0.333340)*(2.000000*i-MaxMap);
        x_map[i].y=(double) i;
        y_map[i].y=0.000000;
        z_map[i].y=0.666665*(2.000000*i-MaxMap);
        x_map[i].z=(double) i;
        y_map[i].z=(-0.500000)*(2.000000*i-MaxMap);
        z_map[i].z=(-0.333340)*(2.000000*i-MaxMap);
      }
      break;
    }
    case Rec601YCbCrColorspace:
    case YCbCrColorspace:
    {
      /*
        Initialize YCbCr tables:

          R = Y            +1.402000*Cr
          G = Y-0.344136*Cb-0.714136*Cr
          B = Y+1.772000*Cb

        Cb and Cr, normally -0.5 through 0.5, must be normalized to the range 0
        through QuantumRange.
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=(double) i;
        y_map[i].x=0.000000;
        z_map[i].x=(1.402000*0.500000)*(2.000000*i-MaxMap);
        x_map[i].y=(double) i;
        y_map[i].y=(-0.344136*0.500000)*(2.000000*i-MaxMap);
        z_map[i].y=(-0.714136*0.500000)*(2.000000*i-MaxMap);
        x_map[i].z=(double) i;
        y_map[i].z=(1.772000*0.500000)*(2.000000*i-MaxMap);
        z_map[i].z=0.000000;
      }
      break;
    }
    case Rec709YCbCrColorspace:
    {
      /*
        Initialize YCbCr tables:

          R = Y            +1.574800*Cr
          G = Y-0.187324*Cb-0.468124*Cr
          B = Y+1.855600*Cb

        Cb and Cr, normally -0.5 through 0.5, must be normalized to the range 0
        through QuantumRange.
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=(double) i;
        y_map[i].x=0.000000;
        z_map[i].x=(1.574800*0.50000)*(2.00000*i-MaxMap);
        x_map[i].y=(double) i;
        y_map[i].y=(-0.187324*0.50000)*(2.00000*i-MaxMap);
        z_map[i].y=(-0.468124*0.50000)*(2.00000*i-MaxMap);
        x_map[i].z=(double) i;
        y_map[i].z=(1.855600*0.50000)*(2.00000*i-MaxMap);
        z_map[i].z=0.00000;
      }
      break;
    }
    case sRGBColorspace:
    {
      /*
        Nonlinear sRGB to linear RGB.

          R = 1.0*R+0.0*G+0.0*B
          G = 0.0*R+1.0*G+0.0*B
          B = 0.0*R+0.0*G+1.0*B
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=1.0*i;
        y_map[i].x=0.0*i;
        z_map[i].x=0.0*i;
        x_map[i].y=0.0*i;
        y_map[i].y=1.0*i;
        z_map[i].y=0.0*i;
        x_map[i].z=0.0*i;
        y_map[i].z=0.0*i;
        z_map[i].z=1.0*i;
      }
      break;
    }
    case XYZColorspace:
    {
      /*
        Initialize CIE XYZ tables (ITU R-709 RGB):

          R =  3.2407100*X-1.5372600*Y-0.4985710*Z
          G = -0.9692580*X+1.8759900*Y+0.0415557*Z
          B =  0.0556352*X-0.2039960*Y+1.0570700*Z
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=3.2407100*i;
        x_map[i].y=(-0.9692580)*i;
        x_map[i].z=0.0556352*i;
        y_map[i].x=(-1.5372600)*i;
        y_map[i].y=1.8759900*i;
        y_map[i].z=(-0.2039960)*i;
        z_map[i].x=(-0.4985710)*i;
        z_map[i].y=0.0415557*i;
        z_map[i].z=1.0570700*i;
      }
      break;
    }
    case YCCColorspace:
    {
      /*
        Initialize YCC tables:

          R = Y            +1.340762*C2
          G = Y-0.317038*C1-0.682243*C2
          B = Y+1.632639*C1

        YCC is scaled by 1.3584.  C1 zero is 156 and C2 is at 137.
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=1.3584000*i;
        y_map[i].x=0.0000000;
        z_map[i].x=1.8215000*(i-(double) ScaleQuantumToMap(
          ScaleCharToQuantum(137)));
        x_map[i].y=1.3584000*i;
        y_map[i].y=(-0.4302726)*(i-(double) ScaleQuantumToMap(
          ScaleCharToQuantum(156)));
        z_map[i].y=(-0.9271435)*(i-(double) ScaleQuantumToMap(
          ScaleCharToQuantum(137)));
        x_map[i].z=1.3584000*i;
        y_map[i].z=2.2179000*(i-(double) ScaleQuantumToMap(
          ScaleCharToQuantum(156)));
        z_map[i].z=0.0000000;
      }
      break;
    }
    case YIQColorspace:
    {
      /*
        Initialize YIQ tables:

          R = Y+0.95620*I+0.62140*Q
          G = Y-0.27270*I-0.64680*Q
          B = Y-1.10370*I+1.70060*Q

        I and Q, normally -0.5 through 0.5, must be normalized to the range 0
        through QuantumRange.
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=(double) i;
        y_map[i].x=0.47810*(2.00000*i-MaxMap);
        z_map[i].x=0.31070*(2.00000*i-MaxMap);
        x_map[i].y=(double) i;
        y_map[i].y=(-0.13635)*(2.00000*i-MaxMap);
        z_map[i].y=(-0.32340)*(2.00000*i-MaxMap);
        x_map[i].z=(double) i;
        y_map[i].z=(-0.55185)*(2.00000*i-MaxMap);
        z_map[i].z=0.85030*(2.00000*i-MaxMap);
      }
      break;
    }
    case YPbPrColorspace:
    {
      /*
        Initialize YPbPr tables:

          R = Y            +1.402000*C2
          G = Y-0.344136*C1+0.714136*C2
          B = Y+1.772000*C1

        Pb and Pr, normally -0.5 through 0.5, must be normalized to the range 0
        through QuantumRange.
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=(double) i;
        y_map[i].x=0.000000;
        z_map[i].x=0.701000*(2.00000*i-MaxMap);
        x_map[i].y=(double) i;
        y_map[i].y=(-0.172068)*(2.00000*i-MaxMap);
        z_map[i].y=0.357068*(2.00000*i-MaxMap);
        x_map[i].z=(double) i;
        y_map[i].z=0.88600*(2.00000*i-MaxMap);
        z_map[i].z=0.00000;
      }
      break;
    }
    case YUVColorspace:
    default:
    {
      /*
        Initialize YUV tables:

          R = Y          +1.13980*V
          G = Y-0.39380*U-0.58050*V
          B = Y+2.02790*U

        U and V, normally -0.5 through 0.5, must be normalized to the range 0
        through QuantumRange.
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=(double) i;
        y_map[i].x=0.00000;
        z_map[i].x=0.56990*(2.0000*i-MaxMap);
        x_map[i].y=(double) i;
        y_map[i].y=(-0.19690)*(2.00000*i-MaxMap);
        z_map[i].y=(-0.29025)*(2.00000*i-MaxMap);
        x_map[i].z=(double) i;
        y_map[i].z=1.01395*(2.00000*i-MaxMap);
        z_map[i].z=0.00000;
      }
      break;
    }
  }
  /*
    Convert to RGB.
  */
  switch (image->storage_class)
  {
    case DirectClass:
    default:
    {
      /*
        Convert DirectClass image.
      */
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          pixel.red=x_map[ScaleQuantumToMap(q->red)].x+
            y_map[ScaleQuantumToMap(q->green)].x+
            z_map[ScaleQuantumToMap(q->blue)].x;
          pixel.green=x_map[ScaleQuantumToMap(q->red)].y+
            y_map[ScaleQuantumToMap(q->green)].y+
            z_map[ScaleQuantumToMap(q->blue)].y;
          pixel.blue=x_map[ScaleQuantumToMap(q->red)].z+
            y_map[ScaleQuantumToMap(q->green)].z+
            z_map[ScaleQuantumToMap(q->blue)].z;
          switch (colorspace)
          {
#if !defined(UseHDRI)
            case YCCColorspace:
            {
              q->red=ScaleCharToQuantum(YCCMap[RoundToYCC(
                255.0*QuantumScale*pixel.red)]);
              q->green=ScaleCharToQuantum(YCCMap[RoundToYCC(
                255.0*QuantumScale*pixel.green)]);
              q->blue=ScaleCharToQuantum(YCCMap[RoundToYCC(
                255.0*QuantumScale*pixel.blue)]);
              break;
            }
#endif
            case sRGBColorspace:
            {
              if ((QuantumScale*pixel.red) <= 0.0031308)
                pixel.red*=12.92f;
              else
                pixel.red=(MagickRealType) QuantumRange*(1.055*
                  pow(QuantumScale*pixel.red,(1.0/2.4))-0.055);
              if ((QuantumScale*pixel.green) <= 0.0031308)
                pixel.green*=12.92f;
              else
                pixel.green=(MagickRealType) QuantumRange*(1.055*
                  pow(QuantumScale*pixel.green,(1.0/2.4))-0.055);
              if ((QuantumScale*pixel.blue) <= 0.0031308)
                pixel.blue*=12.92f;
              else
                pixel.blue=(MagickRealType) QuantumRange*(1.055*
                  pow(QuantumScale*pixel.blue,(1.0/2.4))-0.055);
            }
            default:
            {
              q->red=ScaleMapToQuantum((MagickRealType) MaxMap*
                QuantumScale*pixel.red);
              q->green=ScaleMapToQuantum((MagickRealType) MaxMap*
                QuantumScale*pixel.green);
              q->blue=ScaleMapToQuantum((MagickRealType) MaxMap*
                QuantumScale*pixel.blue);
              break;
            }
          }
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(y,image->rows) != MagickFalse))
          {
            status=image->progress_monitor(TransformRGBImageTag,y,image->rows,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
      break;
    }
    case PseudoClass:
    {
      /*
        Convert PseudoClass image.
      */
      for (i=0; i < (long) image->colors; i++)
      {
        pixel.red=x_map[ScaleQuantumToMap(image->colormap[i].red)].x+
          y_map[ScaleQuantumToMap(image->colormap[i].green)].x+
          z_map[ScaleQuantumToMap(image->colormap[i].blue)].x;
        pixel.green=x_map[ScaleQuantumToMap(image->colormap[i].red)].y+
          y_map[ScaleQuantumToMap(image->colormap[i].green)].y+
          z_map[ScaleQuantumToMap(image->colormap[i].blue)].y;
        pixel.blue=x_map[ScaleQuantumToMap(image->colormap[i].red)].z+
          y_map[ScaleQuantumToMap(image->colormap[i].green)].z+
          z_map[ScaleQuantumToMap(image->colormap[i].blue)].z;
        switch (colorspace)
        {
#if !defined(UseHDRI)
          case YCCColorspace:
          {
            image->colormap[i].red=ScaleCharToQuantum(YCCMap[RoundToYCC(
              255.0*QuantumScale*pixel.red)]);
            image->colormap[i].green=ScaleCharToQuantum(YCCMap[RoundToYCC(
              255.0*QuantumScale*pixel.green)]);
            image->colormap[i].blue=ScaleCharToQuantum(YCCMap[RoundToYCC(
              255.0*QuantumScale*pixel.blue)]);
            break;
          }
#endif
          case sRGBColorspace:
          {
            if ((QuantumScale*pixel.red) <= 0.0031308)
              pixel.red*=12.92f;
            else
              pixel.red=(MagickRealType) QuantumRange*(1.055*pow(QuantumScale*
                pixel.red,(1.0/2.4))-0.055);
            if ((QuantumScale*pixel.green) <= 0.0031308)
              pixel.green*=12.92f;
            else
              pixel.green=(MagickRealType) QuantumRange*(1.055*pow(QuantumScale*
                pixel.green,(1.0/2.4))-0.055);
            if ((QuantumScale*pixel.blue) <= 0.0031308)
              pixel.blue*=12.92f;
            else
              pixel.blue=(MagickRealType) QuantumRange*(1.055*pow(QuantumScale*
                pixel.blue,(1.0/2.4))-0.055);
          }
          default:
          {
            image->colormap[i].red=ScaleMapToQuantum((MagickRealType) MaxMap*
              QuantumScale*pixel.red);
            image->colormap[i].green=ScaleMapToQuantum((MagickRealType) MaxMap*
              QuantumScale*pixel.green);
            image->colormap[i].blue=ScaleMapToQuantum((MagickRealType) MaxMap*
              QuantumScale*pixel.blue);
            break;
          }
        }
      }
      (void) SyncImage(image);
      break;
    }
  }
  image->colorspace=RGBColorspace;
  /*
    Free resources.
  */
  z_map=(PrimaryInfo *) RelinquishMagickMemory(z_map);
  y_map=(PrimaryInfo *) RelinquishMagickMemory(y_map);
  x_map=(PrimaryInfo *) RelinquishMagickMemory(x_map);
  return(MagickTrue);
}
