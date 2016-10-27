/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                     Y   Y   YYYC  BBBB    YYYC  RRRR                        %
%                      Y Y   C      B   B  C      R   R                       %
%                       Y    C      BBBB   C      RRRR                        %
%                       Y    C      B   B  C      R  R                        %
%                       Y     YYYC  BBBB    YYYC  R   R                       %
%                                                                             %
%                                                                             %
%                     Read/Write RAW YCbCr Image Format.                      %
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
#include "magick/colorspace.h"
#include "magick/constitute.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/quantum.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/statistic.h"
#include "magick/string_.h"
#include "magick/module.h"
#include "magick/utility.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteYCBCRImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d Y C b C r I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadYCBCRImage() reads an image of raw Y, Cb, and Cr samples and returns
%  it.  It allocates the memory necessary for the new Image structure and
%  returns a pointer to the new image.
%
%  The format of the ReadYCBCRImage method is:
%
%      Image *ReadYCBCRImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadYCBCRImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  Image
    *image;

  long
    y;

  MagickBooleanType
    status;

  MagickOffsetType
    offset;

  QuantumInfo
    quantum_info;

  register long
    i;

  register PixelPacket
    *q;

  ssize_t
    count;

  size_t
    packet_size;

  unsigned char
    *pixels;

  unsigned long
    width;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=AllocateImage(image_info);
  if ((image->columns == 0) || (image->rows == 0))
    ThrowReaderException(OptionError,"MustSpecifyImageSize");
  if (image_info->interlace != PartitionInterlace)
    {
      /*
        Open image file.
      */
      status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
      if (status == MagickFalse)
        {
          image=DestroyImageList(image);
          return((Image *) NULL);
        }
      for (i=0; i < image->offset; i++)
        (void) ReadBlobByte(image);
    }
  /*
    Allocate memory for a pixels.
  */
  packet_size=(size_t) ((3*image->depth+7)/8);
  if (LocaleCompare(image_info->magick,"YCbCrA") == 0)
    {
      packet_size+=(image->depth+7)/8;
      image->matte=MagickTrue;
    }
  pixels=(unsigned char *) AcquireQuantumMemory(image->extract_info.width,
    packet_size*sizeof(*pixels));
  if (pixels == (unsigned char *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  if (image_info->number_scenes != 0)
    while (image->scene < image_info->scene)
    {
      /*
        Skip to next image.
      */
      image->scene++;
      for (y=0; y < (long) image->rows; y++)
        count=ReadBlob(image,packet_size*image->extract_info.width,pixels);
    }
  offset=(MagickOffsetType) (packet_size*image->extract_info.x);
  do
  {
    /*
      Convert raster image to pixel packets.
    */
    GetQuantumInfo(image_info,&quantum_info);
    image->colorspace=YCbCrColorspace;
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    if (SetImageExtent(image,0,0) == MagickFalse)
      {
        InheritException(exception,&image->exception);
        return(DestroyImageList(image));
      }
    switch (image_info->interlace)
    {
      case NoInterlace:
      default:
      {
        /*
          No interlacing:  YCbCrYCbCrYCbCrYCbCrYCbCrYCbCr...
        */
        for (y=0; y < image->extract_info.y; y++)
          count=ReadBlob(image,packet_size*image->extract_info.width,pixels);
        for (y=0; y < (long) image->rows; y++)
        {
          if ((y > 0) || (GetPreviousImageInList(image) == (Image *) NULL))
            count=ReadBlob(image,packet_size*image->extract_info.width,
              pixels);
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          if (image->matte == MagickFalse)
            (void) ExportQuantumPixels(image,&quantum_info,RGBQuantum,
              pixels+offset);
          else
            (void) ExportQuantumPixels(image,&quantum_info,RGBAQuantum,
              pixels+offset);
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
        width=image->extract_info.height-image->rows-image->extract_info.y;
        for (i=0; i < (long) width; i++)
          count=ReadBlob(image,packet_size*image->extract_info.width,pixels);
        break;
      }
      case LineInterlace:
      {
        /*
          Line interlacing:  YYY...CbCbCb...CrCrCr...YYY...CbCbCb...CrCrCr...
        */
        packet_size=(size_t) ((image->depth+7)/8);
        for (y=0; y < image->extract_info.y; y++)
          count=ReadBlob(image,packet_size*image->extract_info.width,pixels);
        for (y=0; y < (long) image->rows; y++)
        {
          if ((y > 0) || (GetPreviousImageInList(image) == (Image *) NULL))
            count=ReadBlob(image,packet_size*image->extract_info.width,
              pixels);
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          (void) ExportQuantumPixels(image,&quantum_info,RedQuantum,
            pixels+offset);
          count=ReadBlob(image,packet_size*image->extract_info.width,pixels);
          (void) ExportQuantumPixels(image,&quantum_info,GreenQuantum,
            pixels+offset);
          count=ReadBlob(image,packet_size*image->extract_info.width,pixels);
          (void) ExportQuantumPixels(image,&quantum_info,BlueQuantum,
            pixels+offset);
          if (image->matte != MagickFalse)
            {
              count=ReadBlob(image,packet_size*image->extract_info.width,
                pixels);
              (void) ExportQuantumPixels(image,&quantum_info,AlphaQuantum,
                pixels+offset);
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
        width=image->extract_info.height-image->rows-image->extract_info.y;
        for (i=0; i < (long) width; i++)
          count=ReadBlob(image,packet_size*image->extract_info.width,pixels);
        break;
      }
      case PlaneInterlace:
      case PartitionInterlace:
      {
        unsigned long
          span;

        /*
          Plane interlacing:  YYYYYY...CbCbCbCbCbCb...CrCrCrCrCrCr...
        */
        if (image_info->interlace == PartitionInterlace)
          {
            AppendImageFormat("Y",image->filename);
            status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
            if (status == MagickFalse)
              {
                image=DestroyImageList(image);
                return((Image *) NULL);
              }
          }
        packet_size=(size_t) ((image->depth+7)/8);
        for (y=0; y < image->extract_info.y; y++)
          count=ReadBlob(image,packet_size*image->extract_info.width,pixels);
        i=0;
        span=image->rows*(image->matte != MagickFalse ? 5 : 4);
        for (y=0; y < (long) image->rows; y++)
        {
          if ((y > 0) || (GetPreviousImageInList(image) == (Image *) NULL))
            count=ReadBlob(image,packet_size*image->extract_info.width,
              pixels);
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          (void) ExportQuantumPixels(image,&quantum_info,RedQuantum,
            pixels+offset);
          if (SyncImagePixels(image) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (QuantumTick(i,span) != MagickFalse))
              {
                status=image->progress_monitor(LoadImageTag,i,span,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
          i++;
        }
        width=image->extract_info.height-image->rows-image->extract_info.y;
        for (i=0; i < (long) width; i++)
          count=ReadBlob(image,packet_size*image->extract_info.width,pixels);
        if (image_info->interlace == PartitionInterlace)
          {
            CloseBlob(image);
            AppendImageFormat("Cb",image->filename);
            status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
            if (status == MagickFalse)
              {
                image=DestroyImageList(image);
                return((Image *) NULL);
              }
          }
        for (y=0; y < image->extract_info.y; y++)
          count=ReadBlob(image,packet_size*image->extract_info.width,pixels);
        for (y=0; y < (long) image->rows; y++)
        {
          count=ReadBlob(image,packet_size*image->extract_info.width,pixels);
          q=GetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          (void) ExportQuantumPixels(image,&quantum_info,GreenQuantum,
            pixels+offset);
          if (SyncImagePixels(image) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (QuantumTick(i,span) != MagickFalse))
              {
                status=image->progress_monitor(LoadImageTag,i,span,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
          i++;
        }
        width=image->extract_info.height-image->rows-image->extract_info.y;
        for (i=0; i < (long) width; i++)
          count=ReadBlob(image,packet_size*image->extract_info.width,pixels);
        if (image_info->interlace == PartitionInterlace)
          {
            CloseBlob(image);
            AppendImageFormat("Cr",image->filename);
            status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
            if (status == MagickFalse)
              {
                image=DestroyImageList(image);
                return((Image *) NULL);
              }
          }
        for (y=0; y < image->extract_info.y; y++)
          count=ReadBlob(image,packet_size*image->extract_info.width,pixels);
        for (y=0; y < (long) image->rows; y++)
        {
          count=ReadBlob(image,packet_size*image->extract_info.width,pixels);
          q=GetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          (void) ExportQuantumPixels(image,&quantum_info,BlueQuantum,
            pixels+offset);
          if (SyncImagePixels(image) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (QuantumTick(i,span) != MagickFalse))
              {
                status=image->progress_monitor(LoadImageTag,i,span,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
          i++;
        }
        width=image->extract_info.height-image->rows-image->extract_info.y;
        for (i=0; i < (long) width; i++)
          count=ReadBlob(image,packet_size*image->extract_info.width,pixels);
        if (image->matte != MagickFalse)
          {
            /*
              Read matte channel.
            */
            if (image_info->interlace == PartitionInterlace)
              {
                CloseBlob(image);
                AppendImageFormat("A",image->filename);
                status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
                if (status == MagickFalse)
                  {
                    image=DestroyImageList(image);
                    return((Image *) NULL);
                  }
              }
            for (y=0; y < image->extract_info.y; y++)
              count=ReadBlob(image,packet_size*image->extract_info.width,
                pixels);
            for (y=0; y < (long) image->rows; y++)
            {
              count=ReadBlob(image,packet_size*image->extract_info.width,
                pixels);
              q=GetImagePixels(image,0,y,image->columns,1);
              if (q == (PixelPacket *) NULL)
                break;
              (void) ExportQuantumPixels(image,&quantum_info,AlphaQuantum,
                pixels+offset);
              if (SyncImagePixels(image) == MagickFalse)
                break;
              if (image->previous == (Image *) NULL)
                if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                    (QuantumTick(i,span) != MagickFalse))
                  {
                    status=image->progress_monitor(LoadImageTag,i,span,
                      image->client_data);
                    if (status == MagickFalse)
                      break;
                  }
              i++;
            }
            width=image->extract_info.height-image->rows-image->extract_info.y;
            for (i=0; i < (long) width; i++)
              count=ReadBlob(image,packet_size*image->extract_info.width,
                pixels);
          }
        if (image_info->interlace == PartitionInterlace)
          (void) CopyMagickString(image->filename,image_info->filename,
            MaxTextExtent);
        break;
      }
    }
    if (EOFBlob(image) != MagickFalse)
      {
        ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
          image->filename);
        break;
      }
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    if (image_info->interlace == PartitionInterlace)
      break;
    count=ReadBlob(image,packet_size*image->extract_info.width,pixels);
    if (count != 0)
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
            status=image->progress_monitor(LoadImagesTag,TellBlob(image),
              GetBlobSize(image),image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
  } while (count != 0);
  pixels=(unsigned char *) RelinquishMagickMemory(pixels);
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r Y C b C r I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterYCBCRImage() adds attributes for the YCbCr image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterYCBCRImage method is:
%
%      unsigned long RegisterYCBCRImage(void)
%
*/
ModuleExport unsigned long RegisterYCBCRImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("YCbCr");
  entry->decoder=(DecodeImageHandler *) ReadYCBCRImage;
  entry->encoder=(EncodeImageHandler *) WriteYCBCRImage;
  entry->raw=MagickTrue;
  entry->description=ConstantString("Raw Y, Cb, and Cr samples");
  entry->module=ConstantString("YCbCr");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("YCbCrA");
  entry->decoder=(DecodeImageHandler *) ReadYCBCRImage;
  entry->encoder=(EncodeImageHandler *) WriteYCBCRImage;
  entry->raw=MagickTrue;
  entry->description=ConstantString("Raw Y, Cb, Cr, and opacity samples");
  entry->module=ConstantString("YCbCr");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r Y C b C r I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterYCBCRImage() removes format registrations made by the
%  YCbCr module from the list of supported formats.
%
%  The format of the UnregisterYCBCRImage method is:
%
%      UnregisterYCBCRImage(void)
%
*/
ModuleExport void UnregisterYCBCRImage(void)
{
  (void) UnregisterMagickInfo("YCbCr");
  (void) UnregisterMagickInfo("YCbCrA");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e Y C b C r I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteYCBCRImage() writes an image to a file in the raw Y, Cb, and Cr
%  image format.
%
%  The format of the WriteYCBCRImage method is:
%
%      MagickBooleanType WriteYCBCRImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteYCBCRImage(const ImageInfo *image_info,
  Image *image)
{
  long
    y;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  QuantumInfo
    quantum_info;

  register const PixelPacket
    *p;

  size_t
    packet_size;

  unsigned char
    *pixels;

  /*
    Allocate memory for pixels.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  packet_size=(size_t) ((3*image->depth+7)/8);
  if (LocaleCompare(image_info->magick,"YCbCrA") == 0)
    packet_size+=(image->depth+7)/8;
  pixels=(unsigned char *) AcquireQuantumMemory(image->columns,packet_size*
    sizeof(*pixels));
  if (pixels == (unsigned char *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  if (image_info->interlace != PartitionInterlace)
    {
      /*
        Open output image file.
      */
      status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
      if (status == MagickFalse)
        return(status);
    }
  scene=0;
  do
  {
    /*
      Convert to YCbCr pixels.
    */
    GetQuantumInfo(image_info,&quantum_info);
    if (image_info->colorspace == UndefinedColorspace)
      (void) SetImageColorspace(image,YCbCrColorspace);
    if (LocaleCompare(image_info->magick,"YCbCrA") == 0)
      if (image->matte == MagickFalse)
        (void) SetImageOpacity(image,OpaqueOpacity);
    switch (image_info->interlace)
    {
      case NoInterlace:
      default:
      {
        /*
          No interlacing:  YCbCrYCbCrYCbCrYCbCrYCbCrYCbCr...
        */
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          if (LocaleCompare(image_info->magick,"YCbCrA") != 0)
            {
              (void) ImportQuantumPixels(image,&quantum_info,RGBQuantum,pixels);
              (void) WriteBlob(image,packet_size*image->columns,pixels);
            }
          else
            {
              (void) ImportQuantumPixels(image,&quantum_info,RGBAQuantum,
                pixels);
              (void) WriteBlob(image,packet_size*image->columns,pixels);
            }
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
      case LineInterlace:
      {
        /*
          Line interlacing:  YYY...CbCbCb...CrCrCr...YYY...CbCbCb...CrCrCr...
        */
        packet_size=(image->depth+7)/8;
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          (void) ImportQuantumPixels(image,&quantum_info,RedQuantum,pixels);
          (void) WriteBlob(image,packet_size*image->columns,pixels);
          (void) ImportQuantumPixels(image,&quantum_info,GreenQuantum,pixels);
          (void) WriteBlob(image,packet_size*image->columns,pixels);
          (void) ImportQuantumPixels(image,&quantum_info,BlueQuantum,pixels);
          (void) WriteBlob(image,packet_size*image->columns,pixels);
          if (LocaleCompare(image_info->magick,"YCbCrA") == 0)
            {
              (void) ImportQuantumPixels(image,&quantum_info,AlphaQuantum,
                pixels);
              (void) WriteBlob(image,packet_size*image->columns,pixels);
            }
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
          Plane interlacing:  YYYYYY...CbCbCbCbCbCb...CrCrCrCrCrCr...
        */
        packet_size=(image->depth+7)/8;
        if (image_info->interlace == PartitionInterlace)
          {
            AppendImageFormat("Y",image->filename);
            status=
              OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
            if (status == MagickFalse)
              return(status);
          }
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          (void) ImportQuantumPixels(image,&quantum_info,RedQuantum,pixels);
          (void) WriteBlob(image,packet_size*image->columns,pixels);
        }
        if (image_info->interlace == PartitionInterlace)
          {
            CloseBlob(image);
            AppendImageFormat("Cb",image->filename);
            status=OpenBlob(image_info,image,WriteBinaryBlobMode,
              &image->exception);
            if (status == MagickFalse)
              return(status);
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
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          (void) ImportQuantumPixels(image,&quantum_info,GreenQuantum,pixels);
          (void) WriteBlob(image,packet_size*image->columns,pixels);
        }
        if (image_info->interlace == PartitionInterlace)
          {
            CloseBlob(image);
            AppendImageFormat("Cr",image->filename);
            status=OpenBlob(image_info,image,WriteBinaryBlobMode,
              &image->exception);
            if (status == MagickFalse)
              return(status);
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
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          (void) ImportQuantumPixels(image,&quantum_info,BlueQuantum,pixels);
          (void) WriteBlob(image,packet_size*image->columns,pixels);
        }
        if (LocaleCompare(image_info->magick,"YCbCrA") == 0)
          {
            if (image->progress_monitor != (MagickProgressMonitor) NULL)
              {
                status=image->progress_monitor(LoadImageTag,300,400,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
            if (image_info->interlace == PartitionInterlace)
              {
                CloseBlob(image);
                AppendImageFormat("A",image->filename);
                status=OpenBlob(image_info,image,WriteBinaryBlobMode,
                  &image->exception);
                if (status == MagickFalse)
                  return(status);
              }
            for (y=0; y < (long) image->rows; y++)
            {
              p=AcquireImagePixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              (void) ImportQuantumPixels(image,&quantum_info,AlphaQuantum,
                pixels);
              (void) WriteBlob(image,packet_size*image->columns,pixels);
            }
          }
        if (image_info->interlace == PartitionInterlace)
          (void) CopyMagickString(image->filename,image_info->filename,
            MaxTextExtent);
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
  pixels=(unsigned char *) RelinquishMagickMemory(pixels);
  CloseBlob(image);
  return(MagickTrue);
}
