/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                        PPPP    AAA   L      M   M                           %
%                        P   P  A   A  L      MM MM                           %
%                        PPPP   AAAAA  L      M M M                           %
%                        P      A   A  L      M   M                           %
%                        P      A   A  LLLLL  M   M                           %
%                                                                             %
%                                                                             %
%                          Read/Write Palm Pixmap.                            %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                            Christopher R. Hawks                             %
%                               December 2001                                 %
%                                                                             %
%  Copyright 1999-2004 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%   obtain a copy of the License at                                            %
%                                                                             %
%    http://www.imagemagick.org/script/license.php                            %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Based on pnmtopalm by Bill Janssen and ppmtobmp by Ian Goldberg.
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
#include "magick/exception.h"
#include "magick/image.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/paint.h"
#include "magick/pixel-private.h"
#include "magick/property.h"
#include "magick/quantize.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/module.h"
#include "magick/utility.h"

/*
  Define declarations.
*/
#define PALM_IS_COMPRESSED_FLAG  0x8000
#define PALM_HAS_COLORMAP_FLAG  0x4000
#define PALM_HAS_TRANSPARENCY_FLAG  0x2000
#define PALM_IS_INDIRECT  0x1000
#define PALM_IS_FOR_SCREEN  0x0800
#define PALM_IS_DIRECT_COLOR  0x0400

#define PALM_COMPRESSION_SCANLINE  0x00
#define PALM_COMPRESSION_RLE  0x01
#define PALM_COMPRESSION_NONE  0xFF

/*
 The 256 color system palette for Palm Computing Devices.
*/
static unsigned char
  PalmPalette[256][3] =
  {
    {255, 255,255}, {255, 204,255}, {255, 153,255}, {255, 102,255},
    {255,  51,255}, {255,   0,255}, {255, 255,204}, {255, 204,204},
    {255, 153,204}, {255, 102,204}, {255,  51,204}, {255,   0,204},
    {255, 255,153}, {255, 204,153}, {255, 153,153}, {255, 102,153},
    {255,  51,153}, {255,   0,153}, {204, 255,255}, {204, 204,255},
    {204, 153,255}, {204, 102,255}, {204,  51,255}, {204,   0,255},
    {204, 255,204}, {204, 204,204}, {204, 153,204}, {204, 102,204},
    {204,  51,204}, {204,   0,204}, {204, 255,153}, {204, 204,153},
    {204, 153,153}, {204, 102,153}, {204,  51,153}, {204,   0,153},
    {153, 255,255}, {153, 204,255}, {153, 153,255}, {153, 102,255},
    {153,  51,255}, {153,   0,255}, {153, 255,204}, {153, 204,204},
    {153, 153,204}, {153, 102,204}, {153,  51,204}, {153,   0,204},
    {153, 255,153}, {153, 204,153}, {153, 153,153}, {153, 102,153},
    {153,  51,153}, {153,   0,153}, {102, 255,255}, {102, 204,255},
    {102, 153,255}, {102, 102,255}, {102,  51,255}, {102,   0,255},
    {102, 255,204}, {102, 204,204}, {102, 153,204}, {102, 102,204},
    {102,  51,204}, {102,   0,204}, {102, 255,153}, {102, 204,153},
    {102, 153,153}, {102, 102,153}, {102,  51,153}, {102,   0,153},
    { 51, 255,255}, { 51, 204,255}, { 51, 153,255}, { 51, 102,255},
    { 51,  51,255}, { 51,   0,255}, { 51, 255,204}, { 51, 204,204},
    { 51, 153,204}, { 51, 102,204}, { 51,  51,204}, { 51,   0,204},
    { 51, 255,153}, { 51, 204,153}, { 51, 153,153}, { 51, 102,153},
    { 51,  51,153}, { 51,   0,153}, {  0, 255,255}, {  0, 204,255},
    {  0, 153,255}, {  0, 102,255}, {  0,  51,255}, {  0,   0,255},
    {  0, 255,204}, {  0, 204,204}, {  0, 153,204}, {  0, 102,204},
    {  0,  51,204}, {  0,   0,204}, {  0, 255,153}, {  0, 204,153},
    {  0, 153,153}, {  0, 102,153}, {  0,  51,153}, {  0,   0,153},
    {255, 255,102}, {255, 204,102}, {255, 153,102}, {255, 102,102},
    {255,  51,102}, {255,   0,102}, {255, 255, 51}, {255, 204, 51},
    {255, 153, 51}, {255, 102, 51}, {255,  51, 51}, {255,   0, 51},
    {255, 255,  0}, {255, 204,  0}, {255, 153,  0}, {255, 102,  0},
    {255,  51,  0}, {255,   0,  0}, {204, 255,102}, {204, 204,102},
    {204, 153,102}, {204, 102,102}, {204,  51,102}, {204,   0,102},
    {204, 255, 51}, {204, 204, 51}, {204, 153, 51}, {204, 102, 51},
    {204,  51, 51}, {204,   0, 51}, {204, 255,  0}, {204, 204,  0},
    {204, 153,  0}, {204, 102,  0}, {204,  51,  0}, {204,   0,  0},
    {153, 255,102}, {153, 204,102}, {153, 153,102}, {153, 102,102},
    {153,  51,102}, {153,   0,102}, {153, 255, 51}, {153, 204, 51},
    {153, 153, 51}, {153, 102, 51}, {153,  51, 51}, {153,   0, 51},
    {153, 255,  0}, {153, 204,  0}, {153, 153,  0}, {153, 102,  0},
    {153,  51,  0}, {153,   0,  0}, {102, 255,102}, {102, 204,102},
    {102, 153,102}, {102, 102,102}, {102,  51,102}, {102,   0,102},
    {102, 255, 51}, {102, 204, 51}, {102, 153, 51}, {102, 102, 51},
    {102,  51, 51}, {102,   0, 51}, {102, 255,  0}, {102, 204,  0},
    {102, 153,  0}, {102, 102,  0}, {102,  51,  0}, {102,   0,  0},
    { 51, 255,102}, { 51, 204,102}, { 51, 153,102}, { 51, 102,102},
    { 51,  51,102}, { 51,   0,102}, { 51, 255, 51}, { 51, 204, 51},
    { 51, 153, 51}, { 51, 102, 51}, { 51,  51, 51}, { 51,   0, 51},
    { 51, 255,  0}, { 51, 204,  0}, { 51, 153,  0}, { 51, 102,  0},
    { 51,  51,  0}, { 51,   0,  0}, {  0, 255,102}, {  0, 204,102},
    {  0, 153,102}, {  0, 102,102}, {  0,  51,102}, {  0,   0,102},
    {  0, 255, 51}, {  0, 204, 51}, {  0, 153, 51}, {  0, 102, 51},
    {  0,  51, 51}, {  0,   0, 51}, {  0, 255,  0}, {  0, 204,  0},
    {  0, 153,  0}, {  0, 102,  0}, {  0,  51,  0}, { 17,  17, 17},
    { 34,  34, 34}, { 68,  68, 68}, { 85,  85, 85}, {119, 119,119},
    {136, 136,136}, {170, 170,170}, {187, 187,187}, {221, 221,221},
    {238, 238,238}, {192, 192,192}, {128,   0,  0}, {128,   0,128},
    {  0, 128,  0}, {  0, 128,128}, {  0,   0,  0}, {  0,   0,  0},
    {  0,   0,  0}, {  0,   0,  0}, {  0,   0,  0}, {  0,   0,  0},
    {  0,   0,  0}, {  0,   0,  0}, {  0,   0,  0}, {  0,   0,  0},
    {  0,   0,  0}, {  0,   0,  0}, {  0,   0,  0}, {  0,   0,  0},
    {  0,   0,  0}, {  0,   0,  0}, {  0,   0,  0}, {  0,   0,  0},
    {  0,   0,  0}, {  0,   0,  0}, {  0,   0,  0}, {  0,   0,  0},
    {  0,   0,  0}, {  0,   0,  0}, {  0,   0,  0}, {  0,   0,  0}
  };

/*
  Forward declarations.
*/
static MagickBooleanType
  WritePALMImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   F i n d C o l o r                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FindColor() returns the index of the matching entry from PalmPalette for a
%  given PixelPacket.
%
%  The format of the FindColor method is:
%
%      int FindColor(PixelPacket *pixel)
%
%  A description of each parameter follows:
%
%    o int: the index of the matching color or -1 if not found/
%
%    o pixel: a pointer to the PixelPacket to be matched.
%
%
*/
static int FindColor(PixelPacket *pixel)
{
  register long
    i;

  for (i=0; i < 256; i++)
    if (ScaleQuantumToChar(pixel->red) == PalmPalette[i][0] &&
        ScaleQuantumToChar(pixel->green) == PalmPalette[i][1] &&
        ScaleQuantumToChar(pixel->blue) == PalmPalette[i][2])
      return(i);
  return(-1);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d P A L M I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadPALMImage() reads an image of raw bites in LSB order and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadPALMImage method is:
%
%      Image *ReadPALMImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: Specifies a pointer to an ImageInfo structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline long MagickMax(const long x,const long y)
{
  if (x > y)
    return(x);
  return(y);
}

static inline ssize_t MagickMin(const ssize_t x,const ssize_t y)
{
  if (x < y)
    return(x);
  return(y);
}

static Image *ReadPALMImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  Image
    *image;

  IndexPacket
    index;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    transpix;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  ssize_t
    count;

  unsigned char
    *lastrow,
    *one_row,
    *ptr;

  unsigned long
    bytes_per_row,
    flags,
    bits_per_pixel,
    version,
    nextDepthOffset,
    transparentIndex,
    compressionType,
    byte,
    mask,
    redbits,
    greenbits,
    bluebits,
    pad,
    size,
    bit;

  unsigned short
    color16;

  MagickOffsetType
    totalOffset,
    seekNextDepth;

  /*
    Open image file.
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
      (void) DestroyImageList(image);
      return((Image *) NULL);
    }
  totalOffset=0;
  do
  {
    image->columns=ReadBlobMSBShort(image);
    image->rows=ReadBlobMSBShort(image);
    /*
      Copied from coders/pnm.c. TODO other checks ...
    */
    if ((image->columns == 0) || (image->rows == 0))
      ThrowReaderException(CorruptImageError,"NegativeOrZeroImageSize");
    bytes_per_row=ReadBlobMSBShort(image);
    flags=ReadBlobMSBShort(image);
    bits_per_pixel=(unsigned long) ReadBlobByte(image);
    if (bits_per_pixel > 16)
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    version=(unsigned long) ReadBlobByte(image);
    nextDepthOffset=(unsigned long) ReadBlobMSBShort(image);
    transparentIndex=(unsigned long) ReadBlobByte(image);
    compressionType=(unsigned long) ReadBlobByte(image);
    pad=ReadBlobMSBShort(image);
    /*
      Initialize image colormap.
    */
    if ((bits_per_pixel < 16) &&
        !AllocateImageColormap(image,1L << bits_per_pixel))
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    GetMagickPixelPacket(image,&transpix);
    if (bits_per_pixel == 16)  /* Direct Color */
      {
        redbits=(unsigned long) ReadBlobByte(image);  /* # of bits of red */
        greenbits=(unsigned long) ReadBlobByte(image);  /* # of bits of green */
        bluebits=(unsigned long) ReadBlobByte(image);  /* # of bits of blue */
        ReadBlobByte(image);  /* reserved by Palm */
        ReadBlobByte(image);  /* reserved by Palm */
        transpix.red=(MagickRealType) (QuantumRange*ReadBlobByte(image)/31);
        transpix.green=(MagickRealType) (QuantumRange*ReadBlobByte(image)/63);
        transpix.blue=(MagickRealType) (QuantumRange*ReadBlobByte(image)/31);
      }
    if (bits_per_pixel == 8)
      {
        if (flags & PALM_HAS_COLORMAP_FLAG)
          {
            count=(ssize_t) ReadBlobMSBShort(image);
            for (i=0; i < (long) count; i++)
            {
              ReadBlobByte(image);
              image->colormap[255-i].red=
                ScaleCharToQuantum((unsigned char) ReadBlobByte(image));
              image->colormap[255-i].green=
                ScaleCharToQuantum((unsigned char) ReadBlobByte(image));
              image->colormap[255-i].blue=
                ScaleCharToQuantum((unsigned char) ReadBlobByte(image));
          }
        }
      else
        {
          for (i=0; i < (long) (1L << bits_per_pixel); i++)
          {
            image->colormap[255-i].red=ScaleCharToQuantum(PalmPalette[i][0]);
            image->colormap[255-i].green=ScaleCharToQuantum(PalmPalette[i][1]);
            image->colormap[255-i].blue=ScaleCharToQuantum(PalmPalette[i][2]);
          }
        }
      }
    if (flags & PALM_IS_COMPRESSED_FLAG)
      size=ReadBlobMSBShort(image);
    image->storage_class=DirectClass;
    if (bits_per_pixel < 16)
      {
        image->storage_class=PseudoClass;
        image->depth=8;
      }
    if (SetImageExtent(image,0,0) == MagickFalse)
      {
        InheritException(exception,&image->exception);
        return(DestroyImageList(image));
      }
    one_row=(unsigned char *) AcquireQuantumMemory(bytes_per_row,
      sizeof(*one_row));
    if (one_row == (unsigned char *) NULL)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    lastrow=(unsigned char *) NULL;
    if (compressionType == PALM_COMPRESSION_SCANLINE) {
      lastrow=(unsigned char *) AcquireQuantumMemory(bytes_per_row,
        sizeof(*lastrow));
    if (lastrow == (unsigned char *) NULL)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    }
    mask=(1l << bits_per_pixel)-1;
    for (y = 0; y < (long) image->rows; y++)
    {
      if ((flags & PALM_IS_COMPRESSED_FLAG) == 0)
        {
          /* TODO move out of loop! */
          image->compression=NoCompression;
          count=ReadBlob(image,bytes_per_row,one_row);
        }
      else
        {
          if (compressionType == PALM_COMPRESSION_RLE)
            { 
              /* TODO move out of loop! */
              image->compression=RLECompression;
              for (i=0; i < (long) bytes_per_row; )
              {
                count=(ssize_t) ReadBlobByte(image);
                count=MagickMin(count,(ssize_t) bytes_per_row-i);
                byte=(unsigned long) ReadBlobByte(image);
                (void) ResetMagickMemory(one_row+i,(int) byte,(size_t) count);
                i+=count;
              }
          }
        else
          if (compressionType == PALM_COMPRESSION_SCANLINE)
            {  
              /* TODO move out of loop! */
              image->compression=FaxCompression;
              for (i=0; i < (long) bytes_per_row; i+=8)
              {
                count=(ssize_t) ReadBlobByte(image);
                byte=1UL*MagickMin((ssize_t) bytes_per_row-i,8);
                for (bit=0; bit < byte; bit++)
                {
                  if ((y == 0) || (count & (1 << (7 - bit))))
                    one_row[i+bit]=(unsigned char) ReadBlobByte(image);
                  else
                    one_row[i+bit]=lastrow[i+bit];
                }
              }
              (void) CopyMagickMemory(lastrow, one_row, bytes_per_row);
            }
        }
      ptr=one_row;
      q=SetImagePixels(image,0,y,image->columns,1);
      if (q == (PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      if (bits_per_pixel == 16)
        {
          if (image->columns > (2*bytes_per_row))
            ThrowReaderException(CorruptImageError,"CorruptImage");
          for (x=0; x < (long) image->columns; x++)
          {
            color16=(*ptr++ << 8);
            color16|=(*ptr++);
            q->red=(Quantum) ((QuantumRange*((color16 >> 11) & 0x1f))/0x1f);
            q->green=(Quantum) ((QuantumRange*((color16 >> 5) & 0x3f))/0x3f);
            q->blue=(Quantum) ((QuantumRange*((color16 >> 0) & 0x1f))/0x1f);
            q->opacity=OpaqueOpacity;
            q++;
          }
        }
      else
        {
          bit=8-bits_per_pixel;
          for (x=0; x < (long) image->columns; x++)
          {
            if ((size_t) (ptr-one_row) >= bytes_per_row)
              ThrowReaderException(CorruptImageError,"CorruptImage");
            index=(IndexPacket) (mask-(((*ptr) & (mask << bit)) >> bit));
            indexes[x]=index;
            *q++=image->colormap[(long) index];
            if (bit)
              bit-=bits_per_pixel;
            else
              {
                ptr++;
                bit=8-bits_per_pixel;
              }
          }
          if (SyncImagePixels(image) == MagickFalse)
            break;
        }
      }
    if (flags & PALM_HAS_TRANSPARENCY_FLAG)
      {
        if (bits_per_pixel != 16)
          SetMagickPixelPacket(image,image->colormap+(mask-transparentIndex),
            (const IndexPacket *) NULL,&transpix);
        (void) PaintTransparentImage(image,&transpix,(Quantum)
          TransparentOpacity);
      }
    one_row=(unsigned char *) RelinquishMagickMemory(one_row);
    if (compressionType == PALM_COMPRESSION_SCANLINE)
    lastrow=(unsigned char *) RelinquishMagickMemory(lastrow);
    /*
      Proceed to next image. Copied from coders/pnm.c
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    if (nextDepthOffset != 0)
      { 
        /*
          Skip to next image. 
        */
        totalOffset+=(MagickOffsetType) (nextDepthOffset*4);
        if (totalOffset >= (MagickOffsetType) GetBlobSize(image))
          {
            ThrowReaderException(CorruptImageError,"ImproperImageHeader");
          }
        else
          {
            seekNextDepth=SeekBlob(image,totalOffset,SEEK_SET);
          }
        if (seekNextDepth != totalOffset)
          ThrowReaderException(CorruptImageError,"ImproperImageHeader");
        /*
          Allocate next image structure. Copied from coders/pnm.c
        */
        AllocateNextImage(image_info,image);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            (void) DestroyImageList(image);
            return((Image *) NULL);
          }
        image=SyncNextImageInList(image);
        /* TODO Analyze this ...
        if (image->progress_monitor != (MagickProgressMonitor) NULL)
          {
            status=image->progress_monitor(LoadImagesTag,TellBlob(image),
              GetBlobSize(image),image->client_data);
            if (status == MagickFalse)
              break;
          }*/
      }
  } while (nextDepthOffset != 0);
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r P A L M I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterPALMImage() adds properties for the PALM image format to the list of
%  supported formats.  The properties include the image format tag, a method to
%  read and/or write the format, whether the format supports the saving of more
%  than one frame to the same file or blob, whether the format supports native
%  in-memory I/O, and a brief description of the format.
%
%  The format of the RegisterPALMImage method is:
%
%      unsigned long RegisterPALMImage(void)
%
*/
ModuleExport unsigned long RegisterPALMImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("PALM");
  entry->decoder=(DecodeImageHandler *) ReadPALMImage;
  entry->encoder=(EncodeImageHandler *) WritePALMImage;
  entry->seekable_stream=MagickTrue;
  entry->description=ConstantString("Palm pixmap");
  entry->module=ConstantString("PALM");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r P A L M I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterPALMImage() removes format registrations made by the PALM
%  module from the list of supported formats.
%
%  The format of the UnregisterPALMImage method is:
%
%      UnregisterPALMImage(void)
%
*/
ModuleExport void UnregisterPALMImage(void)
{
  (void) UnregisterMagickInfo("PALM");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e P A L M I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WritePALMImage() writes an image of raw bits in LSB order to a file.
%
%  The format of the WritePALMImage method is:
%
%      MagickBooleanType WritePALMImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: Specifies a pointer to an ImageInfo structure.
%
%    o image:  A pointer to a Image structure.
%
*/
static MagickBooleanType WritePALMImage(const ImageInfo *image_info,
  Image *image)
{
  int
    y;

  const char
    *value;

  Image
    *map;

  ExceptionInfo
    exception;

  MagickBooleanType
    status;

  MagickOffsetType
    currentOffset,
    offset;

  MagickSizeType
    cc;

  PixelPacket
    transpix;

  QuantizeInfo
    quantize_info;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *p;

  unsigned char
    bit,
    byte,
    color,
    *lastrow,
    *one_row,
    *ptr,
    version = 0;

  unsigned int
    transparentIndex = 0;

  unsigned long
    count,
    bits_per_pixel,
    bytes_per_row,
    nextDepthOffset;

  unsigned short
    color16,
    flags = 0;


  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&exception);
  if (status == MagickFalse)
    return(status);
  GetExceptionInfo(&exception);
  currentOffset=0;
  transpix.red=0;
  transpix.green=0;
  transpix.blue=0;
  transpix.opacity=0;
  do
  {
    value=GetImageProperty(image, "Comment");
    if (value != (const char *) NULL)
      if (LocaleCompare("COLORMAP",value) == 0)
        flags|=PALM_HAS_COLORMAP_FLAG;
    count = GetNumberColors(image, NULL, &exception);
    for (bits_per_pixel=1;  (1UL << bits_per_pixel) < count;  bits_per_pixel*=2);
    if (image_info->depth > 100)
      bits_per_pixel=image_info->depth-100;
    if (bits_per_pixel < 16)
      (void) SetImageColorspace(image,image->colorspace);
    if (bits_per_pixel < 8)
      {
        (void) SetImageColorspace(image,GRAYColorspace);
        (void) SortColormapByIntensity(image);
      }
    if (bits_per_pixel > 8)
      flags|=PALM_IS_DIRECT_COLOR;
    (void) WriteBlobMSBShort(image,(unsigned short) image->columns);  /* width */
    (void) WriteBlobMSBShort(image,(unsigned short) image->rows );  /* height */
    bytes_per_row=((image->columns+(16/bits_per_pixel-1))/(16/bits_per_pixel))*2;
    (void) WriteBlobMSBShort(image,(unsigned short) bytes_per_row);
    if ((image->compression == RLECompression) ||
        (image->compression == FaxCompression))
      flags|=PALM_IS_COMPRESSED_FLAG;
    (void) WriteBlobMSBShort(image, flags);
    (void) WriteBlobByte(image,(unsigned char) bits_per_pixel);
    if (bits_per_pixel > 1)
      version=1;
    if ((image->compression == RLECompression) ||
        (image->compression == FaxCompression))
      version=2;
    (void) WriteBlobByte(image,version);
    (void) WriteBlobMSBShort(image,0);  /* nextDepthOffset */
    (void) WriteBlobByte(image,(unsigned char) transparentIndex);
    if (image->compression == RLECompression)
      (void) WriteBlobByte(image,PALM_COMPRESSION_RLE);
    else
      if (image->compression == FaxCompression)
        (void) WriteBlobByte(image,PALM_COMPRESSION_SCANLINE);
      else
        (void) WriteBlobByte(image,PALM_COMPRESSION_NONE);
    (void) WriteBlobMSBShort(image,0);  /* reserved */
    offset=16;
    if (bits_per_pixel == 16)
      {
        (void) WriteBlobByte(image,5);  /* # of bits of red */
        (void) WriteBlobByte(image,6);  /* # of bits of green */
        (void) WriteBlobByte(image,5);  /* # of bits of blue */
        (void) WriteBlobByte(image,0);  /* reserved by Palm */
        (void) WriteBlobMSBLong(image,0);  /* no transparent color, YET */
        offset+=8;
      }
    if (bits_per_pixel == 8)
      {
        if (flags & PALM_HAS_COLORMAP_FLAG)  /* Write out colormap */
          {
            GetQuantizeInfo(&quantize_info);
            quantize_info.dither=IsPaletteImage(image,&image->exception);
            quantize_info.number_colors=image->colors;
            (void) QuantizeImage(&quantize_info,image);
            (void) WriteBlobMSBShort(image,(unsigned short) image->colors);
            for (count = 0; count < image->colors; count++)
            {
              (void) WriteBlobByte(image,(unsigned char) count);
              (void) WriteBlobByte(image,ScaleQuantumToChar(
                image->colormap[count].red));
              (void) WriteBlobByte(image,
                ScaleQuantumToChar(image->colormap[count].green));
              (void) WriteBlobByte(image,
                ScaleQuantumToChar(image->colormap[count].blue));
            }
            offset+=2+count*4;
          }
      else  /* Map colors to Palm standard colormap */
        {
          map=ConstituteImage(256,1,"RGB",CharPixel,&PalmPalette,&exception);
          (void) SetImageColorspace(map,map->colorspace);
          (void) MapImage(image,map,image_info->dither);
          for (y=0; y < (long) image->rows; y++)
          {
            p=GetImagePixels(image,0,y,image->columns,1);
            indexes=GetIndexes(image);
            for (x=0; x < (long) image->columns; x++)
              indexes[x]=(IndexPacket) FindColor(&image->colormap[(long) indexes[x]]);
          }
          (void) DestroyImage(map);
        }
      }
    if (flags & PALM_IS_COMPRESSED_FLAG)
      (void) WriteBlobMSBShort(image,0);  /* fill in size later */
    lastrow=(unsigned char *) NULL;
    if (image->compression == FaxCompression)
      lastrow=(unsigned char *) AcquireQuantumMemory(bytes_per_row,
        sizeof(*lastrow));
      /* TODO check whether memory really was acquired? */
    one_row=(unsigned char *) AcquireQuantumMemory(bytes_per_row,
      sizeof(*one_row));
    if (one_row == (unsigned char *) NULL)
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    for (y=0; y < (int) image->rows; y++)
    {
      ptr=one_row;
      (void) ResetMagickMemory(ptr,0,bytes_per_row);
      p=GetImagePixels(image,0,y,image->columns,1);
      if (p == (PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      if (bits_per_pixel == 16)
        {
          for (x=0; x < (int) image->columns; x++)
          {
            color16=(unsigned short) ((((31*(unsigned long) p->red)/
              (unsigned long) QuantumRange) << 11) |
              (((63*(unsigned long) p->green)/(unsigned long) QuantumRange) << 5) |
              ((31*(unsigned long) p->blue)/(unsigned long) QuantumRange));
            if (p->opacity == (Quantum) TransparentOpacity)
              {
                transpix.red=p->red;
                transpix.green=p->green;
                transpix.blue=p->blue;
                transpix.opacity=p->opacity;
                flags|=PALM_HAS_TRANSPARENCY_FLAG;
              }
            *ptr++=(unsigned char) ((color16 >> 8) & 0xff);
            *ptr++=(unsigned char) (color16 & 0xff);
            p++;
          }
        }
      else
        {
          byte=0x00;
          bit=(unsigned char) (8-bits_per_pixel);
          for (x=0; x < (int) image->columns; x++)
          {
            if (bits_per_pixel >= 8)
              color=(unsigned char) indexes[x];
            else
              color=(unsigned char) (indexes[x]*((1 << bits_per_pixel)-1)/
                MagickMax(1L*image->colors-1L,1L));
            byte|=color << bit;
            if (bit != 0)
              bit-=bits_per_pixel;
            else
              {
                *ptr++=byte;
                byte=0x00;
                bit=(unsigned char) (8-bits_per_pixel);
              }
          }
          if ((image->columns % (8/bits_per_pixel)) != 0)
            *ptr++=byte;
        }
      if (image->compression == RLECompression)
        {
          x=0;
          while (x < (long) bytes_per_row)
          {
            byte=one_row[x];
            count=1;
            while ((one_row[++x] == byte) && (count < 255) &&
                   (x < (long) bytes_per_row))
              count++;
            (void) WriteBlobByte(image,(unsigned char) count);
            (void) WriteBlobByte(image,(unsigned char) byte);
          }
        }
      else
        if (image->compression == FaxCompression)
          {
            char
              tmpbuf[8],
              *tptr;
  
            for (x = 0;  x < (long) bytes_per_row;  x += 8)
            {
              tptr = tmpbuf;
              for (bit=0, byte=0; bit < (unsigned char) MagickMin(8,(ssize_t) bytes_per_row-x); bit++)
              {
                if ((y == 0) || (lastrow[x + bit] != one_row[x + bit]))
                  {
                  byte |= (1 << (7 - bit));
                  *tptr++ = (char) one_row[x + bit];
                }
              }
              (void) WriteBlobByte(image, byte);
              (void) WriteBlob(image,tptr-tmpbuf,(unsigned char *) tmpbuf);
            }
            (void) CopyMagickMemory(lastrow,one_row,bytes_per_row);
          }
        else
          (void) WriteBlob(image,bytes_per_row,one_row);
      }
    if (flags & PALM_HAS_TRANSPARENCY_FLAG)
      {
        offset=SeekBlob(image,currentOffset+6,SEEK_SET);
        (void) WriteBlobMSBShort(image,flags);
        offset=SeekBlob(image,currentOffset+12,SEEK_SET);
        (void) WriteBlobByte(image,(unsigned char) transparentIndex);  /* trans index */
      }
    if (bits_per_pixel == 16)
      {
        offset=SeekBlob(image,currentOffset+20,SEEK_SET);
        (void) WriteBlobByte(image,0);  /* reserved by Palm */
        (void) WriteBlobByte(image,(unsigned char) ((31*transpix.red)/QuantumRange));
        (void) WriteBlobByte(image,(unsigned char) ((63*transpix.green)/QuantumRange));
        (void) WriteBlobByte(image,(unsigned char) ((31*transpix.blue)/QuantumRange));
      }
    if (flags & PALM_IS_COMPRESSED_FLAG)  /* fill in size now */
      {
        offset=SeekBlob(image,currentOffset+offset,SEEK_SET);
        (void) WriteBlobMSBShort(image,(unsigned short) (GetBlobSize(image)-currentOffset-offset));
      }
    if (one_row != (unsigned char *) NULL) 
      one_row=(unsigned char *) RelinquishMagickMemory(one_row);
    if (lastrow != (unsigned char *) NULL) 
      lastrow=(unsigned char *) RelinquishMagickMemory(lastrow);
    if (GetNextImageInList(image) == (Image *) NULL)
      break;
    /* padding to 4 byte word */
    for (cc = (GetBlobSize(image))%4; cc > 0; cc--)
    {
      (void) WriteBlobByte(image,0);
    } 
    /* write nextDepthOffset and return to end of image */
    offset=SeekBlob(image,currentOffset+10,SEEK_SET);
    nextDepthOffset=(unsigned long) ((GetBlobSize(image)-currentOffset)/4);
    (void) WriteBlobMSBShort(image,(unsigned short) nextDepthOffset);
    currentOffset=(MagickOffsetType) GetBlobSize(image);
    offset=SeekBlob(image,currentOffset,SEEK_SET);
    image=SyncNextImageInList(image);
  } while (image_info->adjoin != MagickFalse);
  CloseBlob(image);
  (void) DestroyExceptionInfo(&exception);
  return(MagickTrue);
}
