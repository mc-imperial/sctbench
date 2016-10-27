/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                        TTTTT  IIIII  FFFFF  FFFFF                           %
%                          T      I    F      F                               %
%                          T      I    FFF    FFF                             %
%                          T      I    F      F                               %
%                          T    IIIII  F      F                               %
%                                                                             %
%                                                                             %
%                        Read/Write TIFF Image Format.                        %
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
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/constitute.h"
#include "magick/enhance.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/option.h"
#include "magick/property.h"
#include "magick/quantum.h"
#include "magick/profile.h"
#include "magick/resize.h"
#include "magick/splay-tree.h"
#include "magick/static.h"
#include "magick/statistic.h"
#include "magick/string_.h"
#include "magick/module.h"
#include "magick/utility.h"
#include "magick/module.h"
#if defined(HasTIFF)
# if defined(HAVE_TIFFCONF_H)
#  include "tiffconf.h"
#endif
# include "tiffio.h"
# if !defined(COMPRESSION_ADOBE_DEFLATE)
#  define COMPRESSION_ADOBE_DEFLATE  8
# endif

/*
  Global declarations.
*/
static ExceptionInfo
  *tiff_exception;
#endif

/*
  Forward declarations.
*/
#if defined(HasTIFF)
static MagickBooleanType
  WritePTIFImage(const ImageInfo *,Image *),
  WriteTIFFImage(const ImageInfo *,Image *);
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s T I F F                                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsTIFF() returns MagickTrue if the image format type, identified by the magick
%  string, is TIFF.
%
%  The format of the IsTIFF method is:
%
%      MagickBooleanType IsTIFF(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: This string is generally the first few bytes of an image file
%      or blob.
%
%    o length: Specifies the length of the magick string.
%
%
*/
static MagickBooleanType IsTIFF(const unsigned char *magick,const size_t length)
{
  if (length < 4)
    return(MagickFalse);
  if (memcmp(magick,"\115\115\000\052",4) == 0)
    return(MagickTrue);
  if (memcmp(magick,"\111\111\052\000",4) == 0)
    return(MagickTrue);
#if defined(TIFF_VERSION_BIG)
  if (length < 8)
    return(MagickFalse);
  if (memcmp(magick,"\115\115\000\053\000\010\000\000",8) == 0)
    return(MagickTrue);
  if (memcmp(magick,"\111\111\053\000\010\000\000\000",8) == 0)
    return(MagickTrue);
#endif
  return(MagickFalse);
}

#if defined(HasTIFF)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d T I F F I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadTIFFImage() reads a Tagged image file and returns it.  It allocates the
%  memory necessary for the new Image structure and returns a pointer to the
%  new image.
%
%  The format of the ReadTIFFImage method is:
%
%      Image *ReadTIFFImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline size_t MagickMax(const size_t x,const size_t y)
{
  if (x > y)
    return(x);
  return(y);
}

static inline long MagickMin(const long x,const long y)
{
  if (x < y)
    return(x);
  return(y);
}

static MagickBooleanType ReadProfile(Image *image,const char *name,
  unsigned char *datum,long length)
{
  MagickBooleanType
    status;

  register long
    i;

  StringInfo
    *profile;

  if (length < 4)
    return(MagickFalse);
  i=0;
  if ((LocaleCompare(name,"icc") != 0) && (LocaleCompare(name,"xmp") != 0))
    {
      for (i=0; i < (length-4); i+=2)
        if (LocaleNCompare((char *) (datum+i),"8BIM",4) == 0)
          break;
      if (i == length)
        length-=i;
      else
        i=0;
      if (length < 4)
        return(MagickFalse);
    }
  profile=AcquireStringInfo((size_t) length);
  SetStringInfoDatum(profile,datum+i);
  status=SetImageProfile(image,name,profile);
  profile=DestroyStringInfo(profile);
  if (status == MagickFalse)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  return(MagickTrue);
}

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int TIFFCloseBlob(thandle_t image)
{
  CloseBlob((Image *) image);
  return(0);
}

static void TIFFErrors(const char *module,const char *format,va_list error)
{
  char
    message[MaxTextExtent];

#if defined(HAVE_VSNPRINTF)
  (void) vsnprintf(message,MaxTextExtent,format,error);
#else
  (void) vsprintf(message,format,error);
#endif
  (void) ConcatenateMagickString(message,".",MaxTextExtent);
  (void) ThrowMagickException(tiff_exception,GetMagickModule(),CoderWarning,
    message,"`%s'",module);
}

static int TIFFMapBlob(thandle_t image,tdata_t *base,toff_t *size)
{
  *base=(tdata_t *) GetBlobStreamData((Image *) image);
  if (*base != (tdata_t *) NULL)
    *size=(toff_t) GetBlobSize((Image *) image);
  if (*base != (tdata_t *) NULL)
    return(1);
  return(0);
}

static tsize_t TIFFReadBlob(thandle_t image,tdata_t data,tsize_t size)
{
  tsize_t
    count;

  count=(tsize_t) ReadBlob((Image *) image,(size_t) size,
    (unsigned char *) data);
  return(count);
}

static int32 TIFFReadPixels(TIFF *tiff,unsigned long bits_per_sample,
  tsample_t sample,long row,tdata_t scanline)
{
  int32
    status;

  (void) bits_per_sample;
  status=TIFFReadScanline(tiff,scanline,(uint32) row,sample);
  return(status);
}

static toff_t TIFFSeekBlob(thandle_t image,toff_t offset,int whence)
{
  return((toff_t) SeekBlob((Image *) image,(MagickOffsetType) offset,whence));
}

static toff_t TIFFGetBlobSize(thandle_t image)
{
  return((toff_t) GetBlobSize((Image *) image));
}

static void TIFFUnmapBlob(thandle_t image,tdata_t base,toff_t size)
{
  (void) image;
  (void) base;
  (void) size;
}

static void TIFFWarnings(const char *module,const char *format,va_list warning)
{
  char
    message[MaxTextExtent];

#if defined(HAVE_VSNPRINTF)
  (void) vsnprintf(message,MaxTextExtent,format,warning);
#else
  (void) vsprintf(message,format,warning);
#endif
  (void) ConcatenateMagickString(message,".",MaxTextExtent);
  (void) ThrowMagickException(tiff_exception,GetMagickModule(),CoderWarning,
    message,"`%s'",module);
}

static tsize_t TIFFWriteBlob(thandle_t image,tdata_t data,tsize_t size)
{
  tsize_t
    count;

  count=(tsize_t) WriteBlob((Image *) image,(size_t) size,
    (unsigned char *) data);
  return(count);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

static Image *ReadTIFFImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  typedef enum
  {
    ReadSingleSampleMethod,
    ReadRGBAMethod,
    ReadCMYKAMethod,
    ReadStripMethod,
    ReadTileMethod,
    ReadGenericMethod
  } TIFFMethodType;

  char
    *text;

  const char
    *option;

  float
    *chromaticity,
    x_position,
    y_position,
    x_resolution,
    y_resolution;

  Image
    *image;

  long
    y;

  MagickBooleanType
    associated_alpha,
    debug,
    status;

  MagickSizeType
    number_pixels;

  QuantumInfo
    quantum_info;

  QuantumType
    quantum_type;

  register long
    i,
    x;

  register PixelPacket
    *q;

  TIFF
    *tiff;

  TIFFMethodType
    method;

  uint16
    compress_tag,
    bits_per_sample,
    endian,
    extra_samples,
    interlace,
    max_sample_value,
    min_sample_value,
    orientation,
    pages,
    photometric,
    *sample_info,
    sample_format,
    samples_per_pixel,
    units,
    value;

  uint32
    height,
    length,
    *pixels,
    rows_per_strip,
    width;

  unsigned char
    *profile,
    *scanline;

  unsigned long
    lsb_first;

  /*
    Open image.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=AllocateImage(image_info);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  tiff_exception=exception;
  (void) TIFFSetErrorHandler(TIFFErrors);
  (void) TIFFSetWarningHandler(TIFFWarnings);
  tiff=TIFFClientOpen(image->filename,"r",(thandle_t) image,TIFFReadBlob,
    TIFFWriteBlob,TIFFSeekBlob,TIFFCloseBlob,TIFFGetBlobSize,TIFFMapBlob,
    TIFFUnmapBlob);
  if (tiff == (TIFF *) NULL)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  debug=IsEventLogging();
  if (image_info->number_scenes != 0)
    {
      /*
        Generate blank images for subimage specification (e.g. image.tif[4].
      */
      for (i=0; i < (long) image_info->scene; i++)
      {
        (void) TIFFReadDirectory(tiff);
        AllocateNextImage(image_info,image);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            image=DestroyImageList(image);
            return((Image *) NULL);
          }
        image=SyncNextImageInList(image);
      }
    }
  do
  {
    GetQuantumInfo(image_info,&quantum_info);
    if (image_info->verbose != MagickFalse)
      TIFFPrintDirectory(tiff,stdout,MagickFalse);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_COMPRESSION,&compress_tag);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_ORIENTATION,&orientation);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_IMAGEWIDTH,&width);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_IMAGELENGTH,&height);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_FILLORDER,&endian);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_PLANARCONFIG,&interlace);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_BITSPERSAMPLE,&bits_per_sample);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_SAMPLEFORMAT,&sample_format);
    if (sample_format == SAMPLEFORMAT_UINT)
      quantum_info.format=UnsignedQuantumFormat;
    if (sample_format == SAMPLEFORMAT_INT)
      quantum_info.format=SignedQuantumFormat;
    if (sample_format == SAMPLEFORMAT_IEEEFP)
      quantum_info.format=FloatingPointQuantumFormat;
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_MINSAMPLEVALUE,&min_sample_value);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_MAXSAMPLEVALUE,&max_sample_value);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_PHOTOMETRIC,&photometric);
    switch (photometric)
    {
      case PHOTOMETRIC_MINISBLACK:
      {
        quantum_info.min_is_white=MagickFalse;
        break;
      }
      case PHOTOMETRIC_MINISWHITE:
      {
        quantum_info.min_is_white=MagickTrue;
        break;
      }
      default:
        break;
    }
    if (image->debug != MagickFalse)
      {
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Geometry: %ux%u",
          (unsigned int) width,(unsigned int) height);
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Interlace: %u",
          interlace);
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Bits per sample: %u",bits_per_sample);
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Min sample value: %u",min_sample_value);
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Max sample value: %u",max_sample_value);
        switch (photometric)
        {
          case PHOTOMETRIC_MINISBLACK:
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "Photometric: MINISBLACK");
            quantum_info.min_is_white=MagickFalse;
            break;
          }
          case PHOTOMETRIC_MINISWHITE:
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "Photometric: MINISWHITE");
            quantum_info.min_is_white=MagickTrue;
            break;
          }
          case PHOTOMETRIC_PALETTE:
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "Photometric: PALETTE");
            break;
          }
          case PHOTOMETRIC_RGB:
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "Photometric: RGB");
            break;
          }
          case PHOTOMETRIC_CIELAB:
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "Photometric: CIELAB");
            break;
          }
          case PHOTOMETRIC_SEPARATED:
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "Photometric: SEPARATED");
            break;
          }
          default:
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "Photometric interpretation: %u",photometric);
            break;
          }
        }
      }
    lsb_first=1;
    image->endian=MSBEndian;
    if ((int) (*(char *) &lsb_first) != 0)
      image->endian=LSBEndian;
    if (photometric == PHOTOMETRIC_SEPARATED)
      image->colorspace=CMYKColorspace;
    if (photometric == PHOTOMETRIC_CIELAB)
      image->colorspace=LabColorspace;
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_SAMPLESPERPIXEL,
      &samples_per_pixel);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_RESOLUTIONUNIT,&units);
    x_resolution=(float) image->x_resolution;
    y_resolution=(float) image->y_resolution;
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_XRESOLUTION,&x_resolution);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_YRESOLUTION,&y_resolution);
    image->x_resolution=x_resolution;
    image->y_resolution=y_resolution;
    x_position=(float) image->page.x/x_resolution;
    y_position=(float) image->page.y/y_resolution;
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_XPOSITION,&x_position);
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_YPOSITION,&y_position);
    image->page.x=(long) (x_position*x_resolution+0.5);
    image->page.y=(long) (y_position*y_resolution+0.5);
    image->orientation=(OrientationType) orientation;
    chromaticity=(float *) NULL;
    (void) TIFFGetField(tiff,TIFFTAG_WHITEPOINT,&chromaticity);
    if (chromaticity != (float *) NULL)
      {
        image->chromaticity.white_point.x=chromaticity[0];
        image->chromaticity.white_point.y=chromaticity[1];
      }
    chromaticity=(float *) NULL;
    (void) TIFFGetField(tiff,TIFFTAG_PRIMARYCHROMATICITIES,&chromaticity);
    if (chromaticity != (float *) NULL)
      {
        image->chromaticity.red_primary.x=chromaticity[0];
        image->chromaticity.red_primary.y=chromaticity[1];
        image->chromaticity.green_primary.x=chromaticity[2];
        image->chromaticity.green_primary.y=chromaticity[3];
        image->chromaticity.blue_primary.x=chromaticity[4];
        image->chromaticity.blue_primary.y=chromaticity[5];
      }
    length=0;
#if defined(TIFFTAG_ICCPROFILE)
    if (TIFFGetField(tiff,TIFFTAG_ICCPROFILE,&length,&profile) == 1)
      (void) ReadProfile(image,"icc",profile,(long) length);
#endif
#if defined(TIFFTAG_PHOTOSHOP)
    if (TIFFGetField(tiff,TIFFTAG_PHOTOSHOP,&length,&profile) == 1)
      (void) ReadProfile(image,"8bim",profile,(long) length);
#endif
#if defined(TIFFTAG_RICHTIFFIPTC)
    if (TIFFGetField(tiff,TIFFTAG_RICHTIFFIPTC,&length,&profile) == 1)
      {
        if (TIFFIsByteSwapped(tiff) != 0)
          TIFFSwabArrayOfLong((uint32 *) profile,(unsigned long) length);
        (void) ReadProfile(image,"iptc",profile,4L*length);
      }
#endif
#if defined(TIFFTAG_XMLPACKET)
    if (TIFFGetField(tiff,TIFFTAG_XMLPACKET,&length,&profile) == 1)
      (void) ReadProfile(image,"xmp",profile,(long) length);
#endif
    if (TIFFGetField(tiff,37724,&length,&profile) == 1)
      (void) ReadProfile(image,"tiff:37724",profile,(long) length);
    /*
      Allocate memory for the image and pixel buffer.
    */
    switch (compress_tag)
    {
      case COMPRESSION_NONE: image->compression=NoCompression; break;
      case COMPRESSION_CCITTFAX3: image->compression=FaxCompression; break;
      case COMPRESSION_CCITTFAX4: image->compression=Group4Compression; break;
      case COMPRESSION_JPEG:
      {
         image->compression=JPEGCompression;
#if defined(JPEG_SUPPORT)
         {
           char
             sampling_factor[MaxTextExtent];

           uint16
             horizontal,
             vertical;

           (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_YCBCRSUBSAMPLING,
             &horizontal,&vertical);
           (void) FormatMagickString(sampling_factor,MaxTextExtent,"%dx%d",
             horizontal,vertical);
           (void) SetImageProperty(image,"jpeg:sampling-factor",
             sampling_factor);
           (void) LogMagickEvent(CoderEvent,GetMagickModule(),
             "Sampling Factors: %s",sampling_factor);
         }
#endif
        break;
      }
      case COMPRESSION_OJPEG: image->compression=JPEGCompression; break;
      case COMPRESSION_LZW: image->compression=LZWCompression; break;
      case COMPRESSION_DEFLATE: image->compression=ZipCompression; break;
      case COMPRESSION_ADOBE_DEFLATE: image->compression=ZipCompression; break;
      default: image->compression=RLECompression; break;
    }
    image->columns=width;
    image->rows=height;
    image->depth=(unsigned long) bits_per_sample;
    if (image->debug != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Image depth: %lu",
        image->depth);
    associated_alpha=MagickFalse;
    extra_samples=0;
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_EXTRASAMPLES,&extra_samples,
      &sample_info);
    if (extra_samples == 0)
      {
        if ((samples_per_pixel == 4) && (photometric == PHOTOMETRIC_RGB))
          image->matte=MagickTrue;
      }
    else
      for (i=0; i < (long) extra_samples; i++)
      {
        if (samples_per_pixel > 3)
          {
            image->matte=MagickTrue;
            associated_alpha=MagickFalse;
          }
        if (sample_info[i] == EXTRASAMPLE_UNASSALPHA)
          {
            image->matte=MagickTrue;
            associated_alpha=MagickFalse;
          }
        if (sample_info[i] == EXTRASAMPLE_ASSOCALPHA)
          {
            image->matte=MagickTrue;
            associated_alpha=MagickTrue;
          }
      }
    option=GetImageOption(image_info,"tiff:alpha");
    if (option != (const char *) NULL)
      associated_alpha=LocaleCompare(option,"associate") ? MagickTrue :
        MagickFalse;
    if ((samples_per_pixel <= 2) && (TIFFIsTiled(tiff) == MagickFalse) &&
        (photometric == PHOTOMETRIC_PALETTE))
      {
        if (AllocateImageColormap(image,1UL << bits_per_sample) == MagickFalse)
          {
            TIFFClose(tiff);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
      }
    if (units == RESUNIT_INCH)
      image->units=PixelsPerInchResolution;
    if (units == RESUNIT_CENTIMETER)
      image->units=PixelsPerCentimeterResolution;
    value=(unsigned short) image->scene;
    (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_PAGENUMBER,&value,&pages);
    image->scene=value;
    if (TIFFGetField(tiff,TIFFTAG_ARTIST,&text) == 1)
      (void) SetImageProperty(image,"tiff:artist",text);
    if (TIFFGetField(tiff,TIFFTAG_DATETIME,&text) == 1)
      (void) SetImageProperty(image,"tiff:timestamp",text);
    if (TIFFGetField(tiff,TIFFTAG_SOFTWARE,&text) == 1)
      (void) SetImageProperty(image,"tiff:software",text);
    if (TIFFGetField(tiff,TIFFTAG_HOSTCOMPUTER,&text) == 1)
      (void) SetImageProperty(image,"tiff:hostcomputer",text);
    if (TIFFGetField(tiff,TIFFTAG_DOCUMENTNAME,&text) == 1)
      (void) SetImageProperty(image,"tiff:document",text);
    if (TIFFGetField(tiff,TIFFTAG_MAKE,&text) == 1)
      (void) SetImageProperty(image,"tiff:make",text);
    if (TIFFGetField(tiff,TIFFTAG_MODEL,&text) == 1)
      (void) SetImageProperty(image,"tiff:model",text);
    if (TIFFGetField(tiff,33432,&text) == 1)
      (void) SetImageProperty(image,"tiff:copyright",text);
    if (TIFFGetField(tiff,TIFFTAG_PAGENAME,&text) == 1)
      (void) SetImageProperty(image,"label",text);
    if (TIFFGetField(tiff,TIFFTAG_IMAGEDESCRIPTION,&text) == 1)
      (void) SetImageProperty(image,"comment",text);
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    if (SetImageExtent(image,0,0) == MagickFalse)
      {
        InheritException(exception,&image->exception);
        return(DestroyImageList(image));
      }
    method=ReadGenericMethod;
    if (TIFFGetField(tiff,TIFFTAG_ROWSPERSTRIP,&rows_per_strip) != 0)
      {
        char
          value[MaxTextExtent];

        method=ReadStripMethod;
        (void) FormatMagickString(value,MaxTextExtent,"%u",
          (unsigned int) rows_per_strip);
        (void) SetImageProperty(image,"tiff:rows-per-strip",value);
      }
    if ((samples_per_pixel >= 2) && (interlace == PLANARCONFIG_CONTIG))
      method=ReadRGBAMethod;
    if ((samples_per_pixel >= 2) && (interlace == PLANARCONFIG_SEPARATE))
      method=ReadCMYKAMethod;
    if ((photometric != PHOTOMETRIC_RGB) &&
        (photometric != PHOTOMETRIC_SEPARATED))
      method=ReadGenericMethod;
    if (image->storage_class == PseudoClass)
      method=ReadSingleSampleMethod;
    if ((photometric == PHOTOMETRIC_MINISBLACK) ||
        (photometric == PHOTOMETRIC_MINISWHITE))
      method=ReadSingleSampleMethod;
    if (TIFFIsTiled(tiff) != MagickFalse)
      method=ReadTileMethod;
    switch (method)
    {
      case ReadSingleSampleMethod:
      {
        size_t
          length,
          packet_size;

        /*
          Convert TIFF image to PseudoClass MIFF image.
        */
        packet_size=(size_t) bits_per_sample/8;
        if (image->matte != MagickFalse)
          packet_size*=2;
        length=MagickMax((size_t) TIFFScanlineSize(tiff),packet_size*
          samples_per_pixel*width);
        scanline=(unsigned char *) AcquireQuantumMemory(length,
          sizeof(*scanline));
        if (scanline == (unsigned char *) NULL)
          {
            TIFFClose(tiff);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
        (void) ResetMagickMemory(scanline,0,length);
        if (image->storage_class == PseudoClass)
          {
            unsigned long
              range;

            uint16
              *blue_colormap,
              *green_colormap,
              *red_colormap;

            /*
              Initialize colormap.
            */
            (void) TIFFGetField(tiff,TIFFTAG_COLORMAP,&red_colormap,
              &green_colormap,&blue_colormap);
            range=255;  /* might be old style 8-bit colormap */
            for (i=0; i < (long) image->colors; i++)
              if ((red_colormap[i] >= 256) || (green_colormap[i] >= 256) ||
                  (blue_colormap[i] >= 256))
                {
                  range=65535;
                  break;
                }
            for (i=0; i < (long) image->colors; i++)
            {
              image->colormap[i].red=(Quantum) 
                (((double) QuantumRange*red_colormap[i])/range+0.5);
              image->colormap[i].green=(Quantum) 
                (((double) QuantumRange*green_colormap[i])/range+0.5);
              image->colormap[i].blue=(Quantum) 
                (((double) QuantumRange*blue_colormap[i])/range+0.5);
            }
          }
        quantum_type=IndexQuantum;
        quantum_info.pad=(size_t) MagickMax((size_t) samples_per_pixel-1,0);
        if (image->matte != MagickFalse)
          {
            if (image->storage_class != PseudoClass)
              {
                quantum_type=samples_per_pixel == 1 ? AlphaQuantum :
                  GrayAlphaQuantum;
                quantum_info.pad=(size_t) MagickMax((size_t) samples_per_pixel-
                  2,0);
              }
            else
              {
                quantum_type=IndexAlphaQuantum;
                quantum_info.pad=(size_t) MagickMax((size_t) samples_per_pixel-
                  2,0);
              }
          }
        else
          if (image->storage_class != PseudoClass)
            {
              quantum_type=GrayQuantum;
              quantum_info.pad=(size_t) MagickMax((size_t) samples_per_pixel-
                1,0);
            }
        for (y=0; y < (long) image->rows; y++)
        {
          int
            status;

          status=TIFFReadPixels(tiff,bits_per_sample,0,y,(char *) scanline);
          if (status == -1)
            break;
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          /*
            Transfer image scanline.
          */
          (void) ExportQuantumPixels(image,&quantum_info,quantum_type,scanline);
          if (SyncImagePixels(image) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (QuantumTick(y,image->rows) != MagickFalse))
              {
                status=image->progress_monitor(LoadImageTag,y,image->rows,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
        }
        scanline=(unsigned char *) RelinquishMagickMemory(scanline);
        break;
      }
      case ReadRGBAMethod:
      {
        /*
          Convert TIFF image to DirectClass MIFF image.
        */
        scanline=(unsigned char *) AcquireQuantumMemory((size_t)
          TIFFScanlineSize(tiff),sizeof(*scanline));
        if (scanline == (unsigned char *) NULL)
          {
            TIFFClose(tiff);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
        quantum_info.pad=(size_t) MagickMax((size_t) samples_per_pixel-3,0);
        quantum_type=RGBQuantum;
        if (image->matte != MagickFalse)
          {
            quantum_type=RGBAQuantum;
            quantum_info.pad=(size_t) MagickMax((size_t) samples_per_pixel-4,0);
          }
        if (image->colorspace == CMYKColorspace)
          {
            quantum_info.pad=(size_t) MagickMax((size_t) samples_per_pixel-4,0);
            quantum_type=CMYKQuantum;
            if (image->matte != MagickFalse)
              {
                quantum_type=CMYKAQuantum;
                quantum_info.pad=(size_t) MagickMax((size_t) samples_per_pixel-
                5,0);
              }
          }
        for (y=0; y < (long) image->rows; y++)
        {
          int
            status;

          status=TIFFReadPixels(tiff,bits_per_sample,0,y,(char *) scanline);
          if (status == -1)
            break;
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          (void) ExportQuantumPixels(image,&quantum_info,quantum_type,scanline);
          if (SyncImagePixels(image) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (QuantumTick(y,image->rows) != MagickFalse))
              {
                status=image->progress_monitor(LoadImageTag,y,image->rows,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
        }
        scanline=(unsigned char *) RelinquishMagickMemory(scanline);
        break;
      }
      case ReadCMYKAMethod:
      {
        /*
          Convert TIFF image to DirectClass MIFF image.
        */
        scanline=(unsigned char *) AcquireQuantumMemory((size_t)
          TIFFScanlineSize(tiff),sizeof(*scanline));
        if (scanline == (unsigned char *) NULL)
          {
            TIFFClose(tiff);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
        for (y=0; y < (long) image->rows; y++)
        {
          for (i=0; i < (long) samples_per_pixel; i++)
          {
            int
              status;

            status=TIFFReadPixels(tiff,bits_per_sample,(tsample_t) i,y,(char *)
              scanline);
            if (status == -1)
              break;
            q=GetImagePixels(image,0,y,image->columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            if (image->colorspace != CMYKColorspace)
              switch (i)
              {
                case 0: quantum_type=RedQuantum; break;
                case 1: quantum_type=GreenQuantum; break;
                case 2: quantum_type=BlueQuantum; break;
                case 3: quantum_type=AlphaQuantum; break;
                default: quantum_type=UndefinedQuantum; break;
              }
           else
              switch (i)
              {
                case 0: quantum_type=CyanQuantum; break;
                case 1: quantum_type=MagentaQuantum; break;
                case 2: quantum_type=YellowQuantum; break;
                case 3: quantum_type=BlackQuantum; break;
                case 4: quantum_type=AlphaQuantum; break;
                default: quantum_type=UndefinedQuantum; break;
              }
            (void) ExportQuantumPixels(image,&quantum_info,quantum_type,
              scanline);
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          if (image->previous == (Image *) NULL)
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (QuantumTick(y,image->rows) != MagickFalse))
              {
                status=image->progress_monitor(LoadImageTag,y,image->rows,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
        }
        scanline=(unsigned char *) RelinquishMagickMemory(scanline);
        break;
      }
      case ReadStripMethod:
      {
        register uint32
          *p;

        /*
          Convert stripped TIFF image to DirectClass MIFF image.
        */
        number_pixels=(MagickSizeType) image->columns*rows_per_strip;
        if ((number_pixels*sizeof(uint32)) != (MagickSizeType) ((size_t)
            (number_pixels*sizeof(uint32))))
          {
            TIFFClose(tiff);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
        pixels=(uint32 *) AcquireQuantumMemory((size_t) number_pixels,
          sizeof(*pixels));
        if (pixels == (uint32 *) NULL)
          {
            TIFFClose(tiff);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
        /*
          Convert image to DirectClass pixel packets.
        */
        i=0;
        p=(uint32 *) NULL;
        for (y=0; y < (long) image->rows; y++)
        {
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          if (i == 0)
            {
              if (TIFFReadRGBAStrip(tiff,(tstrip_t) y,pixels) == 0)
                break;
              i=(long) MagickMin((long) rows_per_strip,(long) image->rows-y);
            }
          i--;
          p=pixels+image->columns*i;
          for (x=0; x < (long) image->columns; x++)
          {
            q->red=ScaleCharToQuantum((unsigned char) TIFFGetR(*p));
            q->green=ScaleCharToQuantum((unsigned char) TIFFGetG(*p));
            q->blue=ScaleCharToQuantum((unsigned char) TIFFGetB(*p));
            if (image->matte != MagickFalse)
              q->opacity=ScaleCharToQuantum((unsigned char) TIFFGetA(*p));
            p++;
            q++;
          }
          if (SyncImagePixels(image) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (QuantumTick(y,image->rows) != MagickFalse))
              {
                status=image->progress_monitor(LoadImageTag,y,image->rows,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
        }
        pixels=(uint32 *) RelinquishMagickMemory(pixels);
        break;
      }
      case ReadTileMethod:
      {
        register uint32
          *p;

        uint32
          *tile_pixels,
          columns,
          rows;

        unsigned long
          number_pixels;

        /*
          Convert tiled TIFF image to DirectClass MIFF image.
        */
        if ((TIFFGetField(tiff,TIFFTAG_TILEWIDTH,&columns) == 0) ||
            (TIFFGetField(tiff,TIFFTAG_TILELENGTH,&rows) == 0))
          {
            TIFFClose(tiff);
            ThrowReaderException(CoderError,"ImageIsNotTiled");
          }
        image->extract_info.width=columns;
        image->extract_info.height=rows;
        number_pixels=columns*rows;
        tile_pixels=(uint32 *) AcquireQuantumMemory((size_t) columns*rows,
          sizeof(*tile_pixels));
        if (tile_pixels == (uint32 *) NULL)
          {
            TIFFClose(tiff);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
        for (y=0; y < (long) image->rows; y+=rows)
        {
          PixelPacket
            *tile;

          unsigned long
            columns_remaining,
            rows_remaining;

          rows_remaining=image->rows-y;
          if ((long) (y+rows) < (long) image->rows)
            rows_remaining=rows;
          tile=SetImagePixels(image,0,y,image->columns,rows_remaining);
          if (tile == (PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x+=columns)
          {
            unsigned long
              column,
              row;

            if (TIFFReadRGBATile(tiff,(uint32) x,(uint32) y,tile_pixels) == 0)
              break;
            columns_remaining=image->columns-x;
            if ((long) (x+columns) < (long) image->columns)
              columns_remaining=columns;
            p=tile_pixels+(rows-rows_remaining)*columns;
            q=tile+(image->columns*(rows_remaining-1)+x);
            for (row=rows_remaining; row > 0; row--)
            {
              if (image->matte != MagickFalse)
                for (column=columns_remaining; column > 0; column--)
                {
                  q->red=ScaleCharToQuantum((unsigned char) TIFFGetR(*p));
                  q->green=ScaleCharToQuantum((unsigned char) TIFFGetG(*p));
                  q->blue=ScaleCharToQuantum((unsigned char) TIFFGetB(*p));
                  q->opacity=ScaleCharToQuantum((unsigned char) TIFFGetA(*p));
                  q++;
                  p++;
                }
              else
                for (column=columns_remaining; column > 0; column--)
                {
                  q->red=ScaleCharToQuantum((unsigned char) TIFFGetR(*p));
                  q->green=ScaleCharToQuantum((unsigned char) TIFFGetG(*p));
                  q->blue=ScaleCharToQuantum((unsigned char) TIFFGetB(*p));
                  q++;
                  p++;
                }
              p+=columns-columns_remaining;
              q-=(image->columns+columns_remaining);
            }
          }
          if (SyncImagePixels(image) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (QuantumTick(y,image->rows) != MagickFalse))
              {
                status=image->progress_monitor(LoadImageTag,y,image->rows,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
        }
        tile_pixels=(uint32 *) RelinquishMagickMemory(tile_pixels);
        break;
      }
      case ReadGenericMethod:
      default:
      {
        register uint32
          *p;

        /*
          Convert TIFF image to DirectClass MIFF image.
        */
        number_pixels=(MagickSizeType) image->columns*image->rows;
        if ((number_pixels*sizeof(uint32)) != (MagickSizeType) ((size_t)
            (number_pixels*sizeof(uint32))))
          {
            TIFFClose(tiff);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
        pixels=(uint32 *) AcquireQuantumMemory((size_t) number_pixels,
          sizeof(*pixels));
        if (pixels == (uint32 *) NULL)
          {
            TIFFClose(tiff);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
        (void) TIFFReadRGBAImage(tiff,(uint32) image->columns,
          (uint32) image->rows,pixels,0);
        /*
          Convert image to DirectClass pixel packets.
        */
        p=pixels+number_pixels-1;
        for (y=0; y < (long) image->rows; y++)
        {
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          q+=image->columns-1;
          for (x=0; x < (long) image->columns; x++)
          {
            q->red=ScaleCharToQuantum((unsigned char) TIFFGetR(*p));
            q->green=ScaleCharToQuantum((unsigned char) TIFFGetG(*p));
            q->blue=ScaleCharToQuantum((unsigned char) TIFFGetB(*p));
            if (image->matte != MagickFalse)
              q->opacity=ScaleCharToQuantum((unsigned char) TIFFGetA(*p));
            p--;
            q--;
          }
          if (SyncImagePixels(image) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (QuantumTick(y,image->rows) != MagickFalse))
              {
                status=image->progress_monitor(LoadImageTag,y,image->rows,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
        }
        pixels=(uint32 *) RelinquishMagickMemory(pixels);
        break;
      }
    }
    if ((image->matte != MagickFalse) && (associated_alpha != MagickFalse))
      {
        MagickRealType
          gamma;

        /*
          Disassociate alpha.
        */
        for (y=0; y < (long) image->rows; y++)
        {
          q=GetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            gamma=QuantumScale*((MagickRealType) QuantumRange-q->opacity);
            gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
            q->red=RoundToQuantum(gamma*q->red);
            q->green=RoundToQuantum(gamma*q->green);
            q->blue=RoundToQuantum(gamma*q->blue);
            q++;
          }
          if (SyncImagePixels(image) == MagickFalse)
            break;
        }
      }
    image->endian=MSBEndian;
    if (endian == FILLORDER_LSB2MSB)
      image->endian=LSBEndian;
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    status=TIFFReadDirectory(tiff) != 0 ? MagickTrue : MagickFalse;
    if (status == MagickTrue)
      {
        /*
          Allocate next image structure.
        */
        AllocateNextImage(image_info,image);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            image=DestroyImageList(image);
            return((Image *) NULL);
          }
        image=SyncNextImageInList(image);
        if (image->progress_monitor != (MagickProgressMonitor) NULL)
          {
            status=image->progress_monitor(LoadImagesTag,(MagickOffsetType)
              image->scene-1,image->scene,image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
  } while (status == MagickTrue);
  TIFFClose(tiff);
  return(GetFirstImageInList(image));
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r T I F F I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterTIFFImage() adds properties for the TIFF image format to
%  the list of supported formats.  The properties include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterTIFFImage method is:
%
%      unsigned long RegisterTIFFImage(void)
%
*/
ModuleExport unsigned long RegisterTIFFImage(void)
{
#define TIFFDescription  "Tagged Image File Format"

  char
    version[MaxTextExtent];

  MagickInfo
    *entry;

  *version='\0';
#if defined(TIFF_VERSION)
  (void) FormatMagickString(version,MaxTextExtent,"%d",TIFF_VERSION);
#endif
#if defined(HasTIFF)
  {
    const char
      *p;

    register long int
      i;

    p=TIFFGetVersion();
    for (i=0; (i < MaxTextExtent-1) && (*p != 0) && (*p != '\n'); i++)
      version[i]=(*p++);
    version[i]='\0';
  }
#endif

  entry=SetMagickInfo("PTIF");
#if defined(HasTIFF)
  entry->decoder=(DecodeImageHandler *) ReadTIFFImage;
  entry->encoder=(EncodeImageHandler *) WritePTIFImage;
#endif
  entry->adjoin=MagickFalse;
  entry->endian_support=MagickTrue;
  entry->seekable_stream=MagickTrue;
  entry->thread_support=NoThreadSupport;
  entry->description=ConstantString("Pyramid encoded TIFF");
  entry->module=ConstantString("TIFF");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("TIF");
#if defined(HasTIFF)
  entry->decoder=(DecodeImageHandler *) ReadTIFFImage;
  entry->encoder=(EncodeImageHandler *) WriteTIFFImage;
#endif
  entry->endian_support=MagickTrue;
  entry->seekable_stream=MagickTrue;
  entry->stealth=MagickTrue;
  entry->thread_support=NoThreadSupport;
  entry->description=ConstantString(TIFFDescription);
  if (*version != '\0')
    entry->version=ConstantString(version);
  entry->module=ConstantString("TIFF");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("TIFF");
#if defined(HasTIFF)
  entry->decoder=(DecodeImageHandler *) ReadTIFFImage;
  entry->encoder=(EncodeImageHandler *) WriteTIFFImage;
#endif
  entry->magick=(IsImageFormatHandler *) IsTIFF;
  entry->endian_support=MagickTrue;
  entry->seekable_stream=MagickTrue;
  entry->thread_support=NoThreadSupport;
  entry->description=ConstantString(TIFFDescription);
  if (*version != '\0')
    entry->version=ConstantString(version);
  entry->module=ConstantString("TIFF");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("TIFF64");
#if defined(TIFF_VERSION_BIG)
  entry->decoder=(DecodeImageHandler *) ReadTIFFImage;
  entry->encoder=(EncodeImageHandler *) WritePTIFImage;
#endif
  entry->adjoin=MagickFalse;
  entry->endian_support=MagickTrue;
  entry->seekable_stream=MagickTrue;
  entry->thread_support=NoThreadSupport;
  entry->description=ConstantString("Tagged Image File Format (64-bit)");
  if (*version != '\0')
    entry->version=ConstantString(version);
  entry->module=ConstantString("TIFF");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r T I F F I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterTIFFImage() removes format registrations made by the TIFF module
%  from the list of supported formats.
%
%  The format of the UnregisterTIFFImage method is:
%
%      UnregisterTIFFImage(void)
%
*/
ModuleExport void UnregisterTIFFImage(void)
{
  (void) UnregisterMagickInfo("PTIF");
  (void) UnregisterMagickInfo("TIF");
  (void) UnregisterMagickInfo("TIFF");
  (void) UnregisterMagickInfo("TIFF64");
}

#if defined(HasTIFF)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e P T I F I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WritePTIFImage() writes an image in the pyrimid-encoded Tagged image file
%  format.
%
%  The format of the WritePTIFImage method is:
%
%      MagickBooleanType WritePTIFImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WritePTIFImage(const ImageInfo *image_info,
  Image *image)
{
  Image
    *pyramid_image;

  ImageInfo
    *write_info;

  MagickBooleanType
    status;

  /*
    Create pyramid-encoded TIFF image.
  */
  pyramid_image=CloneImage(image,0,0,MagickTrue,&image->exception);
  if (pyramid_image == (Image *) NULL)
    return(MagickFalse);
  do
  {
    pyramid_image->next=ResizeImage(image,pyramid_image->columns/2,
      pyramid_image->rows/2,LanczosFilter,1.0,&image->exception);
    if (GetNextImageInList(pyramid_image) == (Image *) NULL)
      {
        pyramid_image=DestroyImageList(pyramid_image);
        return(MagickFalse);
      }
    pyramid_image->next->previous=pyramid_image;
    pyramid_image=GetNextImageInList(pyramid_image);
  } while ((pyramid_image->columns > 64) && (pyramid_image->rows > 64));
  pyramid_image=GetFirstImageInList(pyramid_image);
  /*
    Write pyramid-encoded TIFF image.
  */
  write_info=CloneImageInfo(image_info);
  write_info->adjoin=MagickTrue;
  status=WriteTIFFImage(write_info,pyramid_image);
  pyramid_image=DestroyImageList(pyramid_image);
  write_info=DestroyImageInfo(write_info);
  return(status);
}
#endif

#if defined(HasTIFF)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e T I F F I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteTIFFImage() writes an image in the Tagged image file format.
%
%  The format of the WriteTIFFImage method is:
%
%      MagickBooleanType WriteTIFFImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o image:  The image.
%
*/

typedef struct _TIFFInfo
{
  unsigned char
    *scanline,
    *scanlines,
    *pixels;
} TIFFInfo;

static void DestroyTIFFInfo(TIFFInfo *tiff_info)
{
  assert(tiff_info != (TIFFInfo *) NULL);
  if (tiff_info->scanline != (unsigned char *) NULL)
    tiff_info->scanline=(unsigned char *) RelinquishMagickMemory(
      tiff_info->scanline);
  if (tiff_info->scanlines != (unsigned char *) NULL)
    tiff_info->scanlines=(unsigned char *) RelinquishMagickMemory(
      tiff_info->scanlines);
  if (tiff_info->pixels != (unsigned char *) NULL)
    tiff_info->pixels=(unsigned char *) RelinquishMagickMemory(
      tiff_info->pixels);
}

static MagickBooleanType GetTIFFInfo(Image *image,TIFF *tiff,
  TIFFInfo *tiff_info)
{
  assert(tiff_info != (TIFFInfo *) NULL);
  (void) ResetMagickMemory(tiff_info,0,sizeof(*tiff_info));
  tiff_info->scanline=(unsigned char *) AcquireQuantumMemory((size_t)
    TIFFScanlineSize(tiff),sizeof(*tiff_info->scanline));
  if (tiff_info->scanline == (unsigned char *) NULL)
    {
      DestroyTIFFInfo(tiff_info);
      return(MagickFalse);
    }
  if (TIFFIsTiled(tiff) != 0)
    {
      tiff_info->scanlines=(unsigned char *) AcquireQuantumMemory((size_t)
        image->extract_info.height*TIFFScanlineSize(tiff),
        sizeof(*tiff_info->scanlines));
      tiff_info->pixels=(unsigned char *) AcquireQuantumMemory((size_t)
        TIFFTileSize(tiff),sizeof(*tiff_info->scanlines));
      if ((tiff_info->scanlines == (unsigned char *) NULL) ||
          (tiff_info->pixels == (unsigned char *) NULL))
        {
          DestroyTIFFInfo(tiff_info);
          return(MagickFalse);
        }
    }
  return(MagickTrue);
}

static int32 TIFFWritePixels(TIFF *tiff,TIFFInfo *tiff_info,long row,
  tsample_t sample,Image *image)
{
  int32
    status;

  long
    bytes_per_pixel,
    j,
    k,
    l;

  register long
    i;

  register unsigned char
    *p,
    *q;

  unsigned long
    number_tiles,
    tile_width;

  if (TIFFIsTiled(tiff) == 0)
    return(TIFFWriteScanline(tiff,tiff_info->scanline,(uint32) row,sample));
  /*
    Fill scanlines to tile height.
  */
  i=(long) (row % image->extract_info.height)*TIFFScanlineSize(tiff);
  (void) CopyMagickMemory(tiff_info->scanlines+i,(char *) tiff_info->scanline,
    (size_t) TIFFScanlineSize(tiff));
  if (((unsigned long) (row % image->extract_info.height) !=
      (image->extract_info.height-1)) && (row != (long) (image->rows-1)))
    return(0);
  /*
    Write tile to TIFF image.
  */
  status=0;
  bytes_per_pixel=TIFFTileSize(tiff)/(long)
    (image->extract_info.height*image->extract_info.width);
  number_tiles=image->columns/image->extract_info.width;
  for (i=0; i < (long) number_tiles; i++)
  {
    tile_width=(i == (long) (number_tiles-1)) ?
      image->columns-(i*image->extract_info.width) : image->extract_info.width;
    for (j=0; j < (long) ((row % image->extract_info.height)+1); j++)
      for (k=0; k < (long) tile_width; k++)
      {
        if (bytes_per_pixel == 0)
          {
             p=tiff_info->scanlines+(j*TIFFScanlineSize(tiff)+
               (i*image->extract_info.width+k)/8);
             q=tiff_info->pixels+(j*TIFFTileRowSize(tiff)+k/8);
             *q++=(*p++);
             continue;
          }
        p=tiff_info->scanlines+(j*TIFFScanlineSize(tiff)+
          (i*image->extract_info.width+k)*bytes_per_pixel);
        q=tiff_info->pixels+(j*TIFFTileRowSize(tiff)+k*bytes_per_pixel);
        for (l=0; l < bytes_per_pixel; l++)
          *q++=(*p++);
      }
    status=TIFFWriteTile(tiff,tiff_info->pixels,(uint32) (i*
      image->extract_info.width),(uint32) ((row/image->extract_info.height)*
      image->extract_info.height),0,sample);
    if (status < 0)
      break;
  }
  return(status);
}

static MagickBooleanType WriteTIFFImage(const ImageInfo *image_info,
  Image *image)
{
#if !defined(TIFFDefaultStripSize)
#define TIFFDefaultStripSize(tiff,request)  ((8*1024)/TIFFScanlineSize(tiff))
#endif

  const char
    *mode,
    *option;

  const char
    *value;

  CompressionType
    compression;

  long
    y;

  MagickBooleanType
    debug,
    status;

  MagickOffsetType
    scene;

  QuantumInfo
    quantum_info;

  register const PixelPacket
    *p;

  register long
    i;

  TIFF
    *tiff;

  TIFFInfo
    tiff_info;

  uint16
    bits_per_sample,
    compress_tag,
    endian,
    photometric;

  unsigned long
    lsb_first,
    rows_per_strip;

  /*
    Open TIFF file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == MagickFalse)
    return(status);
  tiff_exception=(&image->exception);
  (void) TIFFSetErrorHandler((TIFFErrorHandler) TIFFErrors);
  (void) TIFFSetWarningHandler((TIFFErrorHandler) TIFFWarnings);
  switch (image_info->endian)
  {
    case LSBEndian: mode="wl"; break;
    case MSBEndian: mode="wb"; break;
    default: mode="w"; break;
  }
#if defined(TIFF_VERSION_BIG)
  if (LocaleCompare(image_info->magick,"TIFF64") == 0)
    switch (image_info->endian)
    {
      case LSBEndian: mode="wl8"; break;
      case MSBEndian: mode="wb8"; break;
      default: mode="w8"; break;
    }
#endif
  tiff=TIFFClientOpen(image->filename,mode,(thandle_t) image,TIFFReadBlob,
    TIFFWriteBlob,TIFFSeekBlob,TIFFCloseBlob,TIFFGetBlobSize,TIFFMapBlob,
    TIFFUnmapBlob);
  if (tiff == (TIFF *) NULL)
    return(MagickFalse);
  scene=0;
  debug=IsEventLogging();
  do
  {
    /*
      Initialize TIFF fields.
    */
    GetQuantumInfo(image_info,&quantum_info);
    if (LocaleCompare(image_info->magick,"PTIF") == 0)
      if (GetPreviousImageInList(image) != (Image *) NULL)
        (void) TIFFSetField(tiff,TIFFTAG_SUBFILETYPE,FILETYPE_REDUCEDIMAGE);
    (void) TIFFSetField(tiff,TIFFTAG_IMAGELENGTH,(uint32) image->rows);
    (void) TIFFSetField(tiff,TIFFTAG_IMAGEWIDTH,(uint32) image->columns);
    compression=image->compression;
    switch (image->compression)
    {
      case FaxCompression:
      case Group4Compression:
      {
        if (IsMonochromeImage(image,&image->exception) == MagickFalse)
          compression=RLECompression;
        if (IsOpaqueImage(image,&image->exception) == MagickFalse)
          compression=RLECompression;
#if !defined(CCITT_SUPPORT)
        compression=RLECompression;
#endif
        break;
      }
      case JPEGCompression:
      {
#if !defined(HasJPEG) || !defined(YCBCR_SUPPORT)
        compression=RLECompression;
#else
        (void) SetImageStorageClass(image,DirectClass);
        (void) SetImageDepth(image,8);
#endif
        break;
      }
      case LZWCompression:
      {
#if !defined(LZW_SUPPORT)
        compression=RLECompression;
#endif
        break;
      }
      case ZipCompression:
      {
#if !defined(ZIP_SUPPORT)
        compression=RLECompression;
#endif
        break;
      }
      default:
        break;
    }
#if !defined(PACKBITS_SUPPORT)
    if (compression == RLECompression)
      compression=NoCompression;
#endif
    switch (compression)
    {
      case FaxCompression:
      {
        compress_tag=COMPRESSION_CCITTFAX3;
        break;
      }
      case Group4Compression:
      {
        compress_tag=COMPRESSION_CCITTFAX4;
        break;
      }
      case JPEGCompression:
      {
        compress_tag=COMPRESSION_JPEG;
        break;
      }
      case LZWCompression:
      {
        compress_tag=COMPRESSION_LZW;
        break;
      }
      case RLECompression:
      {
        compress_tag=COMPRESSION_PACKBITS;
        break;
      }
      case ZipCompression:
      {
        compress_tag=COMPRESSION_ADOBE_DEFLATE;
        break;
      }
      case NoCompression:
      default:
      {
        compress_tag=COMPRESSION_NONE;
        break;
      }
    }
    (void) TIFFSetField(tiff,TIFFTAG_COMPRESSION,compress_tag);
    if (((image_info->colorspace == UndefinedColorspace) &&
         (image->colorspace == CMYKColorspace)) ||
         (image_info->colorspace == CMYKColorspace))
      {
        photometric=PHOTOMETRIC_SEPARATED;
        (void) TIFFSetField(tiff,TIFFTAG_SAMPLESPERPIXEL,4);
        (void) TIFFSetField(tiff,TIFFTAG_INKSET,INKSET_CMYK);
      }
    else
      {
        /*
          Full color TIFF raster.
        */
        if (image->colorspace == LabColorspace)
          photometric=PHOTOMETRIC_CIELAB;
        else
          if (image->colorspace == YCbCrColorspace)
            {
              photometric=PHOTOMETRIC_YCBCR;
              (void) TIFFSetField(tiff,TIFFTAG_YCBCRSUBSAMPLING,1,1);
              (void) SetImageDepth(image,8);
            }
          else
            {
              if (image_info->colorspace == UndefinedColorspace)
                (void) SetImageColorspace(image,RGBColorspace);
              photometric=PHOTOMETRIC_RGB;
            }
        (void) TIFFSetField(tiff,TIFFTAG_SAMPLESPERPIXEL,3);
        if (image_info->type != TrueColorType)
          {
            if ((image_info->type != PaletteType) &&
                (IsGrayImage(image,&image->exception) != MagickFalse))
              {
                photometric=(uint16) (quantum_info.min_is_white != MagickFalse ?
                  PHOTOMETRIC_MINISWHITE : PHOTOMETRIC_MINISBLACK);
                (void) TIFFSetField(tiff,TIFFTAG_SAMPLESPERPIXEL,1);
                if ((image_info->depth == 0) &&
                    (IsMonochromeImage(image,&image->exception) != MagickFalse))
                  image->depth=1;
              }
            else
              if (image->storage_class == PseudoClass)
                {
                  /*
                    Colormapped TIFF raster.
                  */
                  (void) TIFFSetField(tiff,TIFFTAG_SAMPLESPERPIXEL,1);
                  photometric=PHOTOMETRIC_PALETTE;
                }
          }
      }
    if (image_info->extract != (char *) NULL)
      {
        (void) TIFFSetField(tiff,TIFFTAG_TILEWIDTH,image->extract_info.width);
        (void) TIFFSetField(tiff,TIFFTAG_TILELENGTH,image->extract_info.height);
        if (image->depth != 1)
          image->depth=GetImageQuantumDepth(image,MagickTrue);
      }
    (void) TIFFSetField(tiff,TIFFTAG_BITSPERSAMPLE,image->depth);
    if (image->matte != MagickFalse)
      {
        uint16
          extra_samples,
          sample_info[1],
          samples_per_pixel;

        /*
          TIFF has a matte channel.
        */
        extra_samples=1;
        sample_info[0]=EXTRASAMPLE_UNASSALPHA;
        (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_SAMPLESPERPIXEL,
          &samples_per_pixel);
        (void) TIFFSetField(tiff,TIFFTAG_SAMPLESPERPIXEL,samples_per_pixel+1);
        (void) TIFFSetField(tiff,TIFFTAG_EXTRASAMPLES,extra_samples,
          &sample_info);
      }
    (void) TIFFSetField(tiff,TIFFTAG_PHOTOMETRIC,photometric);
    switch (quantum_info.format)
    {
      case FloatingPointQuantumFormat:
      {
        (void) TIFFSetField(tiff,TIFFTAG_SAMPLEFORMAT,SAMPLEFORMAT_IEEEFP);
        (void) TIFFSetField(tiff,TIFFTAG_SMINSAMPLEVALUE,quantum_info.minimum);
        (void) TIFFSetField(tiff,TIFFTAG_SMAXSAMPLEVALUE,quantum_info.maximum);
        break;
      }
      case SignedQuantumFormat:
      {
        (void) TIFFSetField(tiff,TIFFTAG_SAMPLEFORMAT,SAMPLEFORMAT_INT);
        break;
      }
      case UnsignedQuantumFormat:
      {
        (void) TIFFSetField(tiff,TIFFTAG_SAMPLEFORMAT,SAMPLEFORMAT_UINT);
        break;
      }
      default:
        break;
    }
    switch (image->endian)
    {
      case LSBEndian:
      {
        endian=FILLORDER_LSB2MSB;
        break;
      }
      case MSBEndian:
      {
        endian=FILLORDER_MSB2LSB;
        break;
      }
      case UndefinedEndian:
      default:
      {
        (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_FILLORDER,&endian);
        break;
      }
    }
    lsb_first=1;
    image->endian=MSBEndian;
    if ((int) (*(char *) &lsb_first) != 0)
      image->endian=LSBEndian;
    (void) TIFFSetField(tiff,TIFFTAG_FILLORDER,endian);
    (void) TIFFSetField(tiff,TIFFTAG_ORIENTATION,ORIENTATION_TOPLEFT);
    (void) TIFFSetField(tiff,TIFFTAG_PLANARCONFIG,PLANARCONFIG_CONTIG);
    if (photometric == PHOTOMETRIC_RGB)
      if ((image_info->interlace == PlaneInterlace) ||
          (image_info->interlace == PartitionInterlace))
        (void) TIFFSetField(tiff,TIFFTAG_PLANARCONFIG,PLANARCONFIG_SEPARATE);
    rows_per_strip=1;
    if (TIFFScanlineSize(tiff) != 0)
      rows_per_strip=(unsigned long) MagickMax((size_t)
        TIFFDefaultStripSize(tiff,-1),1);
    option=GetImageOption(image_info,"tiff:rows-per-strip");
    if (option != (const char *) NULL)
      rows_per_strip=(unsigned long) strtol(option,(char **) NULL,10);
    switch (compress_tag)
    {
      case COMPRESSION_JPEG:
      {
#if defined(JPEG_SUPPORT)
        const char
          *sampling_factor;

        GeometryInfo
          geometry_info;

        MagickStatusType
          flags;

        (void) TIFFSetField(tiff,TIFFTAG_ROWSPERSTRIP,
          rows_per_strip+(16-(rows_per_strip % 16)));
        (void) TIFFSetField(tiff,TIFFTAG_JPEGQUALITY,image_info->quality ==
          UndefinedCompressionQuality ? image->quality : image_info->quality);
        (void) TIFFSetField(tiff,TIFFTAG_JPEGCOLORMODE,JPEGCOLORMODE_RAW);
        if (image->colorspace == RGBColorspace)
          {
            (void) TIFFSetField(tiff,TIFFTAG_JPEGCOLORMODE,JPEGCOLORMODE_RGB);
            sampling_factor=(const char *) NULL;
            value=GetImageProperty(image,"jpeg:sampling-factor");
            if (value != (char *) NULL)
              {
                sampling_factor=value;
                if (image->debug != MagickFalse)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "  Input sampling-factors=%s",sampling_factor);
              }
            if (image_info->sampling_factor != (char *) NULL)
              sampling_factor=image_info->sampling_factor;
            if (sampling_factor != (const char *) NULL)
              {
                flags=ParseGeometry(sampling_factor,&geometry_info);
                if ((flags & SigmaValue) == 0)
                  geometry_info.sigma=geometry_info.rho;
                (void) TIFFSetField(tiff,TIFFTAG_YCBCRSUBSAMPLING,(uint16)
                  geometry_info.rho,(uint16) geometry_info.sigma);
              }
          }
        if (bits_per_sample == 12)
          (void) TIFFSetField(tiff,TIFFTAG_JPEGTABLESMODE,JPEGTABLESMODE_QUANT);
#endif
        break;
      }
      case COMPRESSION_ADOBE_DEFLATE:
      {
        (void) TIFFSetField(tiff,TIFFTAG_ROWSPERSTRIP,image->rows);
        (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_BITSPERSAMPLE,
          &bits_per_sample);
        if (((photometric == PHOTOMETRIC_RGB) ||
             (photometric == PHOTOMETRIC_MINISBLACK)) &&
            ((bits_per_sample == 8) || (bits_per_sample == 16)))
          (void) TIFFSetField(tiff,TIFFTAG_PREDICTOR,2);
        (void) TIFFSetField(tiff,TIFFTAG_ZIPQUALITY,image_info->quality ==
          UndefinedCompressionQuality ? 7 : MagickMin(1L*image_info->quality/10,
          9));
        break;
      }
      case COMPRESSION_CCITTFAX3:
      {
        /*
          Byte-aligned EOL.
        */
        (void) TIFFSetField(tiff,TIFFTAG_GROUP3OPTIONS,4);
        (void) TIFFSetField(tiff,TIFFTAG_ROWSPERSTRIP,image->rows);
        break;
      }
      case COMPRESSION_CCITTFAX4:
      {
        (void) TIFFSetField(tiff,TIFFTAG_ROWSPERSTRIP,image->rows);
        break;
      }
      case COMPRESSION_LZW:
      {
        (void) TIFFSetField(tiff,TIFFTAG_ROWSPERSTRIP,rows_per_strip);
        (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_BITSPERSAMPLE,
          &bits_per_sample);
        if (((photometric == PHOTOMETRIC_RGB) ||
             (photometric == PHOTOMETRIC_MINISBLACK)) &&
            ((bits_per_sample == 8) || (bits_per_sample == 16)))
          (void) TIFFSetField(tiff,TIFFTAG_PREDICTOR,2);
        break;
      }
      default:
      {
        (void) TIFFSetField(tiff,TIFFTAG_ROWSPERSTRIP,rows_per_strip);
        break;
      }
    }
    if ((image->x_resolution != 0.0) && (image->y_resolution != 0.0))
      {
        unsigned short
          units;

        /*
          Set image resolution.
        */
        units=RESUNIT_NONE;
        if (image->units == PixelsPerInchResolution)
          units=RESUNIT_INCH;
        if (image->units == PixelsPerCentimeterResolution)
          units=RESUNIT_CENTIMETER;
        (void) TIFFSetField(tiff,TIFFTAG_RESOLUTIONUNIT,(uint16) units);
        (void) TIFFSetField(tiff,TIFFTAG_XRESOLUTION,image->x_resolution);
        (void) TIFFSetField(tiff,TIFFTAG_YRESOLUTION,image->y_resolution);
        if ((image->page.x != 0) || (image->page.y != 0))
          {
            /*
              Set image position.
            */
            (void) TIFFSetField(tiff,TIFFTAG_XPOSITION,(float) image->page.x/
              image->x_resolution);
            (void) TIFFSetField(tiff,TIFFTAG_YPOSITION,(float) image->page.y/
              image->y_resolution);
          }
      }
    if (image->chromaticity.white_point.x != 0.0)
      {
        float
          chromaticity[6];

        /*
          Set image chromaticity.
        */
        chromaticity[0]=(float) image->chromaticity.red_primary.x;
        chromaticity[1]=(float) image->chromaticity.red_primary.y;
        chromaticity[2]=(float) image->chromaticity.green_primary.x;
        chromaticity[3]=(float) image->chromaticity.green_primary.y;
        chromaticity[4]=(float) image->chromaticity.blue_primary.x;
        chromaticity[5]=(float) image->chromaticity.blue_primary.y;
        (void) TIFFSetField(tiff,TIFFTAG_PRIMARYCHROMATICITIES,chromaticity);
        chromaticity[0]=(float) image->chromaticity.white_point.x;
        chromaticity[1]=(float) image->chromaticity.white_point.y;
        (void) TIFFSetField(tiff,TIFFTAG_WHITEPOINT,chromaticity);
      }
    if (image->profiles != (void *) NULL)
      {
        const char
          *name;

        const StringInfo
          *profile;

        ResetImageProfileIterator(image);
        for (name=GetNextImageProfile(image); name != (const char *) NULL; )
        {
          profile=GetImageProfile(image,name);
#if defined(TIFFTAG_XMLPACKET)
          if (LocaleCompare(name,"xmp") == 0)
            (void) TIFFSetField(tiff,TIFFTAG_XMLPACKET,(uint32)
              GetStringInfoLength(profile),GetStringInfoDatum(profile));
#endif
#if defined(TIFFTAG_ICCPROFILE)
          if (LocaleCompare(name,"icc") == 0)
            (void) TIFFSetField(tiff,TIFFTAG_ICCPROFILE,(uint32)
              GetStringInfoLength(profile),GetStringInfoDatum(profile));
#endif
          if (LocaleCompare(name,"iptc") == 0)
            {
              size_t
                length;

              StringInfo
                *iptc_profile;

              iptc_profile=CloneStringInfo(profile);
              length=GetStringInfoLength(profile)+4-(GetStringInfoLength(
                profile) & 0x03);
              SetStringInfoLength(iptc_profile,length);
              if (TIFFIsByteSwapped(tiff))
                TIFFSwabArrayOfLong((uint32 *) GetStringInfoDatum(iptc_profile),
                  (unsigned long) (length/4));
              (void) TIFFSetField(tiff,TIFFTAG_RICHTIFFIPTC,
                (uint32) GetStringInfoLength(iptc_profile)/4,
                GetStringInfoDatum(iptc_profile));
              iptc_profile=DestroyStringInfo(iptc_profile);
            }
          if (LocaleCompare(name,"8bim") == 0)
            {
#if defined(TIFFTAG_PHOTOSHOP)
              uint32
                length;

              length=(uint32) (GetStringInfoLength(profile)+
                (GetStringInfoLength(profile) & 0x01));
              (void) TIFFSetField(tiff,TIFFTAG_PHOTOSHOP,length,
                GetStringInfoDatum(profile));
#endif
            }
          if (LocaleCompare(name,"tiff:37724") == 0)
            (void) TIFFSetField(tiff,37724,(uint32)
              GetStringInfoLength(profile),GetStringInfoDatum(profile));
          name=GetNextImageProfile(image);
        }
      }
    if ((image_info->adjoin != MagickFalse) && (GetImageListLength(image) > 1))
      {
        (void) TIFFSetField(tiff,TIFFTAG_SUBFILETYPE,FILETYPE_PAGE);
        if (image->scene != 0)
          (void) TIFFSetField(tiff,TIFFTAG_PAGENUMBER,(uint16) image->scene,
            GetImageListLength(image));
      }
    if (image->orientation != UndefinedOrientation)
      (void) TIFFSetField(tiff,TIFFTAG_ORIENTATION,(uint16) image->orientation);
    value=GetImageProperty(image,"tiff:hostcomputer");
    if (value != (const char *) NULL)
      (void) TIFFSetField(tiff,TIFFTAG_HOSTCOMPUTER,value);
    value=GetImageProperty(image,"tiff:artist");
    if (value != (const char *) NULL)
      (void) TIFFSetField(tiff,TIFFTAG_ARTIST,value);
    value=GetImageProperty(image,"tiff:timestamp");
    if (value != (const char *) NULL)
      (void) TIFFSetField(tiff,TIFFTAG_DATETIME,value);
    value=GetImageProperty(image,"tiff:make");
    if (value != (const char *) NULL)
      (void) TIFFSetField(tiff,TIFFTAG_MAKE,value);
    value=GetImageProperty(image,"tiff:model");
    if (value != (const char *) NULL)
      (void) TIFFSetField(tiff,TIFFTAG_MODEL,value);
    (void) TIFFSetField(tiff,TIFFTAG_SOFTWARE,
      GetMagickVersion((unsigned long *) NULL));
    (void) TIFFSetField(tiff,TIFFTAG_DOCUMENTNAME,image->filename);
    value=GetImageProperty(image,"tiff:copyright");
    if (value != (const char *) NULL)
      (void) TIFFSetField(tiff,33432,value);
    value=GetImageProperty(image,"Kodak-33423");
    if (value != (const char *) NULL)
      (void) TIFFSetField(tiff,33423,value);
    value=GetImageProperty(image,"Kodak-36867");
    if (value != (const char *) NULL)
      (void) TIFFSetField(tiff,36867,value);
    value=GetImageProperty(image,"label");
    if (value != (const char *) NULL)
      (void) TIFFSetField(tiff,TIFFTAG_PAGENAME,value);
    value=GetImageProperty(image,"comment");
    if (value != (const char *) NULL)
      (void) TIFFSetField(tiff,TIFFTAG_IMAGEDESCRIPTION,value);
    /*
      Write image scanlines.
    */
    if (GetTIFFInfo(image,tiff,&tiff_info) == MagickFalse)
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    switch (photometric)
    {
      case PHOTOMETRIC_CIELAB:
      case PHOTOMETRIC_YCBCR:
      case PHOTOMETRIC_RGB:
      {
        /*
          RGB TIFF image.
        */
        switch (image_info->interlace)
        {
          case NoInterlace:
          default:
          {
            for (y=0; y < (long) image->rows; y++)
            {
              p=AcquireImagePixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              if (image->matte == MagickFalse)
                (void) ImportQuantumPixels(image,&quantum_info,RGBQuantum,
                  tiff_info.scanline);
              else
                (void) ImportQuantumPixels(image,&quantum_info,RGBAQuantum,
                  tiff_info.scanline);
              if (TIFFWritePixels(tiff,&tiff_info,y,0,image) == -1)
                break;
              if (image->previous == (Image *) NULL)
                if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                    (QuantumTick(y,image->rows) != MagickFalse))
                  {
                    status=image->progress_monitor(SaveImageTag,y,image->rows,
                      image->client_data);
                    if (status == MagickFalse)
                      break;
                  }
            }
            break;
          }
          case PlaneInterlace:
          case PartitionInterlace:
          {
            /*
              Plane interlacing:  RRRRRR...GGGGGG...BBBBBB...
            */
            for (y=0; y < (long) image->rows; y++)
            {
              p=AcquireImagePixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              (void) ImportQuantumPixels(image,&quantum_info,RedQuantum,
                tiff_info.scanline);
              if (TIFFWritePixels(tiff,&tiff_info,y,0,image) == -1)
                break;
            }
            if (image->progress_monitor != (MagickProgressMonitor) NULL)
              {
                status=image->progress_monitor(LoadImageTag,100,400,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
            for (y=0; y < (long) image->rows; y++)
            {
              p=AcquireImagePixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              (void) ImportQuantumPixels(image,&quantum_info,GreenQuantum,
                tiff_info.scanline);
              if (TIFFWritePixels(tiff,&tiff_info,y,1,image) == -1)
                break;
            }
            if (image->progress_monitor != (MagickProgressMonitor) NULL)
              {
                status=image->progress_monitor(LoadImageTag,200,400,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
            for (y=0; y < (long) image->rows; y++)
            {
              p=AcquireImagePixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              (void) ImportQuantumPixels(image,&quantum_info,BlueQuantum,
                tiff_info.scanline);
              if (TIFFWritePixels(tiff,&tiff_info,y,2,image) == -1)
                break;
            }
            if (image->progress_monitor != (MagickProgressMonitor) NULL)
              {
                status=image->progress_monitor(LoadImageTag,300,400,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
            if (image->matte != MagickFalse)
              for (y=0; y < (long) image->rows; y++)
              {
                p=AcquireImagePixels(image,0,y,image->columns,1,
                  &image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                (void) ImportQuantumPixels(image,&quantum_info,AlphaQuantum,
                  tiff_info.scanline);
                if (TIFFWritePixels(tiff,&tiff_info,y,3,image) == -1)
                  break;
              }
            if (image->progress_monitor != (MagickProgressMonitor) NULL)
              {
                status=image->progress_monitor(LoadImageTag,400,400,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
            break;
          }
        }
        break;
      }
      case PHOTOMETRIC_SEPARATED:
      {
        /*
          CMYK TIFF image.
        */
        if (image->colorspace != CMYKColorspace)
          (void) SetImageColorspace(image,CMYKColorspace);
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          if (image->matte == MagickFalse)
            (void) ImportQuantumPixels(image,&quantum_info,CMYKQuantum,
              tiff_info.scanline);
          else
            (void) ImportQuantumPixels(image,&quantum_info,CMYKAQuantum,
              tiff_info.scanline);
          if (TIFFWritePixels(tiff,&tiff_info,y,0,image) == -1)
            break;
          if (image->previous == (Image *) NULL)
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (QuantumTick(y,image->rows) != MagickFalse))
              {
                status=image->progress_monitor(SaveImageTag,y,image->rows,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
        }
        break;
      }
      case PHOTOMETRIC_PALETTE:
      {
        uint16
          *blue,
          *green,
          *red;

        /*
          Colormapped TIFF image.
        */
        red=(uint16 *) AcquireQuantumMemory(65536,sizeof(*red));
        green=(uint16 *) AcquireQuantumMemory(65536,sizeof(*green));
        blue=(uint16 *) AcquireQuantumMemory(65536,sizeof(*blue));
        if ((red == (uint16 *) NULL) || (green == (uint16 *) NULL) ||
            (blue == (uint16 *) NULL))
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
        /*
          Initialize TIFF colormap.
        */
        (void) ResetMagickMemory(red,0,65536*sizeof(*red));
        (void) ResetMagickMemory(green,0,65536*sizeof(*green));
        (void) ResetMagickMemory(blue,0,65536*sizeof(*blue));
        for (i=0; i < (long) image->colors; i++)
        {
          red[i]=ScaleQuantumToShort(image->colormap[i].red);
          green[i]=ScaleQuantumToShort(image->colormap[i].green);
          blue[i]=ScaleQuantumToShort(image->colormap[i].blue);
        }
        (void) TIFFSetField(tiff,TIFFTAG_COLORMAP,red,green,blue);
        red=(uint16 *) RelinquishMagickMemory(red);
        green=(uint16 *) RelinquishMagickMemory(green);
        blue=(uint16 *) RelinquishMagickMemory(blue);
      }
      default:
      {
        /*
          Convert PseudoClass packets to contiguous grayscale scanlines.
        */
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          if (image->matte != MagickFalse)
             {
               if (photometric != PHOTOMETRIC_PALETTE)
                 (void) ImportQuantumPixels(image,&quantum_info,
                   GrayAlphaQuantum,tiff_info.scanline);
               else
                 (void) ImportQuantumPixels(image,&quantum_info,
                   IndexAlphaQuantum,tiff_info.scanline);
             }
           else
             if (photometric != PHOTOMETRIC_PALETTE)
               (void) ImportQuantumPixels(image,&quantum_info,GrayQuantum,
                 tiff_info.scanline);
             else
               (void) ImportQuantumPixels(image,&quantum_info,IndexQuantum,
                 tiff_info.scanline);
          if (TIFFWritePixels(tiff,&tiff_info,y,0,image) == -1)
            break;
          if (image->previous == (Image *) NULL)
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (QuantumTick(y,image->rows) != MagickFalse))
              {
                status=image->progress_monitor(SaveImageTag,y,image->rows,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
        }
        break;
      }
    }
    DestroyTIFFInfo(&tiff_info);
    if (image_info->verbose == MagickTrue)
      TIFFPrintDirectory(tiff,stdout,MagickFalse);
    (void) TIFFWriteDirectory(tiff);
    image->endian=MSBEndian;
    if (endian == FILLORDER_LSB2MSB)
      image->endian=LSBEndian;
    if (GetNextImageInList(image) == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        status=image->progress_monitor(SaveImagesTag,scene,
          GetImageListLength(image),image->client_data);
        if (status == MagickFalse)
          break;
      }
    scene++;
  } while (image_info->adjoin != MagickFalse);
  TIFFClose(tiff);
  return(MagickTrue);
}
#endif
