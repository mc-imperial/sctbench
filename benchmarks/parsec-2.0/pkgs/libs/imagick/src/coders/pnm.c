/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            PPPP   N   N  M   M                              %
%                            P   P  NN  N  MM MM                              %
%                            PPPP   N N N  M M M                              %
%                            P      N  NN  M   M                              %
%                            P      N   N  M   M                              %
%                                                                             %
%                                                                             %
%               Read/Write PBMPlus Portable Anymap Image Format.              %
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
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/property.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/statistic.h"
#include "magick/string_.h"
#include "magick/module.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WritePNMImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s P N M                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsPNM() returns MagickTrue if the image format type, identified by the
%  magick string, is PNM.
%
%  The format of the IsPNM method is:
%
%      MagickBooleanType IsPNM(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: This string is generally the first few bytes of an image file
%      or blob.
%
%    o length: Specifies the length of the magick string.
%
*/
static MagickBooleanType IsPNM(const unsigned char *magick,const size_t length)
{
  if (length < 2)
    return(MagickFalse);
  if ((*magick == (unsigned char) 'P') &&
      ((magick[1] == '1') || (magick[1] == '2') || (magick[1] == '3') ||
       (magick[1] == '4') || (magick[1] == '5') || (magick[1] == '6') ||
       (magick[1] == '7') || (magick[1] == 'F') || (magick[1] == 'f')))
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d P N M I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadPNMImage() reads a Portable Anymap image file and returns it.
%  It allocates the memory necessary for the new Image structure and returns
%  a pointer to the new image.
%
%  The format of the ReadPNMImage method is:
%
%      Image *ReadPNMImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline long ConstrainPixel(Image *image,const long offset,
  const unsigned long extent)
{
  if ((offset < 0) || (offset > (long) extent))
    {
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        CorruptImageError,"InvalidPixel","`%s'",image->filename);
      return(0);
    }
  return(offset);
}

static unsigned long PNMInteger(Image *image,const unsigned int base)
{
  char
    *comment;

  int
    c;

  register char
    *p;

  size_t
    length;

  unsigned long
    value;

  /*
    Skip any leading whitespace.
  */
  length=MaxTextExtent;
  comment=(char *) NULL;
  p=comment;
  do
  {
    c=ReadBlobByte(image);
    if (c == EOF)
      return(0);
    if (c == (int) '#')
      {
        /*
          Read comment.
        */
        if (comment == (char *) NULL)
          comment=AcquireString((char *) NULL);
        p=comment+strlen(comment);
        for ( ; (c != EOF) && (c != (int) '\n'); p++)
        {
          if ((size_t) (p-comment+1) >= length)
            {
              length<<=1;
              comment=(char *) ResizeQuantumMemory(comment,length+MaxTextExtent,
                sizeof(*comment));
              if (comment == (char *) NULL)
                break;
              p=comment+strlen(comment);
            }
          c=ReadBlobByte(image);
          *p=(char) c;
          *(p+1)='\0';
        }
        if (comment == (char *) NULL)
          return(0);
        continue;
      }
  } while (isdigit(c) == MagickFalse);
  if (comment != (char *) NULL)
    {
      (void) SetImageProperty(image,"Comment",comment);
      comment=DestroyString(comment);
    }
  if (base == 2)
    return((unsigned long) (c-(int) '0'));
  /*
    Evaluate number.
  */
  value=0;
  do
  {
    value*=10;
    value+=c-(int) '0';
    c=ReadBlobByte(image);
    if (c == EOF)
      return(value);
  } while (isdigit(c) != MagickFalse);
  return(value);
}

static inline unsigned char PushCharPixel(const unsigned char **pixels)
{
  unsigned char
    pixel;

  pixel=(unsigned char) *(*pixels)++;
  return(pixel);
}

static inline unsigned long PushLongPixel(const unsigned char **pixels)
{
  unsigned long
    pixel;

  pixel=(unsigned long) (*(*pixels)++ << 24);
  pixel|=(unsigned long) (*(*pixels)++ << 16);
  pixel|=(unsigned long) (*(*pixels)++ << 8);
  pixel|=(unsigned long) (*(*pixels)++);
  return(pixel);
}

static inline unsigned short PushShortPixel(const unsigned char **pixels)
{
  unsigned short
    pixel;

  pixel=(unsigned short) (*(*pixels)++ << 8);
  pixel|=(unsigned short) (*(*pixels)++);
  return(pixel);
}

static Image *ReadPNMImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    format;

  const unsigned char
    *p;

  Image
    *image;

  long
    index,
    y;

  MagickBooleanType
    grayscale,
    status;

  Quantum
    *scale;

  QuantumInfo
    quantum_info;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  register long
    i;

  size_t
    length,
    packet_size;

  ssize_t
    count;

  unsigned char
    *pixels;

  unsigned long
    max_value;

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
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Read PNM image.
  */
  count=ReadBlob(image,1,(unsigned char *) &format);
  do
  {
    /*
      Initialize image structure.
    */
    if ((count != 1) || (format != 'P'))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    max_value=1;
    grayscale=MagickFalse;
    GetQuantumInfo(image_info,&quantum_info);
    format=(char) ReadBlobByte(image);
    if (format != '7')
      {
        /*
          PBM, PGM, PPM, and PNM.
        */
        image->columns=PNMInteger(image,10);
        image->rows=PNMInteger(image,10);
        if ((format == 'f') || (format == 'F'))
          {
            char
              scale[MaxTextExtent];

            (void) ReadBlobString(image,scale);
            quantum_info.scale=fabs(atof(scale));
            image->endian=atof(scale) < 0.0 ? LSBEndian : MSBEndian;
          }
        else
          {
            if ((format == '1') || (format == '4'))
              max_value=1;  /* bitmap */
            else
              max_value=PNMInteger(image,10);
          }
        if ((format == '1') || (format == '2') || (format == '4') ||
            (format == '5'))
          {
            image->storage_class=PseudoClass;
            image->colors=(unsigned long) (max_value >= MaxColormapSize ?
              MaxColormapSize : max_value+1);
          }
      }
    else
      {
        char
          keyword[MaxTextExtent],
          value[MaxTextExtent];

        int
          c;

        register char
          *p;

        /*
          PAM.
        */
        for (c=ReadBlobByte(image); c != EOF; c=ReadBlobByte(image))
        {
          while (isspace((int) ((unsigned char) c)) != 0)
            c=ReadBlobByte(image);
          p=keyword;
          do
          {
            if ((size_t) (p-keyword) < (MaxTextExtent/2))
              *p++=c;
            c=ReadBlobByte(image);
          } while (isalnum(c));
          *p='\0';
          if (LocaleCompare(keyword,"endhdr") == 0)
            break;
          while (isspace((int) ((unsigned char) c)) != 0)
            c=ReadBlobByte(image);
          p=value;
          while (isalnum(c) || (c == '_'))
          {
            if ((size_t) (p-value) < (MaxTextExtent/2))
              *p++=c;
            c=ReadBlobByte(image);
          }
          *p='\0';
          /*
            Assign a value to the specified keyword.
          */
          if (LocaleCompare(keyword,"depth") == 0)
            packet_size=(unsigned long) atol(value);
          if (LocaleCompare(keyword,"height") == 0)
            image->rows=(unsigned long) atol(value);
          if (LocaleCompare(keyword,"maxval") == 0)
            max_value=(unsigned long) atol(value);
          if (LocaleCompare(keyword,"TUPLTYPE") == 0)
            {
              if (LocaleCompare(value,"BLACKANDWHITE") == 0)
                grayscale=MagickTrue;
              if (LocaleCompare(value,"BLACKANDWHITE_ALPHA") == 0)
                {
                  grayscale=MagickTrue;
                  image->matte=MagickTrue;
                }
              if (LocaleCompare(value,"GRAYSCALE") == 0)
                grayscale=MagickTrue;
              if (LocaleCompare(value,"GRAYSCALE_ALPHA") == 0)
                {
                  grayscale=MagickTrue;
                  image->matte=MagickTrue;
                }
              if (LocaleCompare(value,"RGB_ALPHA") == 0)
                image->matte=MagickTrue;
              if (LocaleCompare(value,"CMYK") == 0)
                image->colorspace=CMYKColorspace;
              if (LocaleCompare(value,"CMYK_ALPHA") == 0)
                image->matte=MagickTrue;
            }
          if (LocaleCompare(keyword,"width") == 0)
            image->columns=(unsigned long) atol(value);
        }
      }
    if ((image->columns == 0) || (image->rows == 0))
      ThrowReaderException(CorruptImageError,"NegativeOrZeroImageSize");
    if (max_value >= 65536)
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    for (image->depth=1; (1UL << image->depth) < max_value; image->depth++);
    scale=(Quantum *) NULL;
    if (image->storage_class == PseudoClass)
      if (AllocateImageColormap(image,image->colors) == MagickFalse)
        ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    if ((image->storage_class != PseudoClass) ||
        (max_value > (1U*QuantumRange)))
      {
        /*
          Compute pixel scaling table.
        */
        scale=(Quantum *) AcquireQuantumMemory((size_t) max_value+1UL,
          sizeof(*scale));
        if (scale == (Quantum *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        for (i=0; i <= (long) max_value; i++)
          scale[i]=ScaleAnyToQuantum((QuantumAny) i,image->depth);
      }
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    if (SetImageExtent(image,0,0) == MagickFalse)
      {
        InheritException(exception,&image->exception);
        return(DestroyImageList(image));
      }
    /*
      Convert PNM pixels to runlength-encoded MIFF packets.
    */
    switch (format)
    {
      case '1':
      {
        /*
          Convert PBM image to pixel packets.
        */
        for (y=0; y < (long) image->rows; y++)
        {
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          for (x=0; x < (long) image->columns; x++)
          {
            index=PNMInteger(image,2) == 0 ? 1 : 0;
            if ((unsigned long) index >= image->colors)
              {
                (void) ThrowMagickException(&image->exception,GetMagickModule(),
                  CorruptImageError,"InvalidColormapIndex","`%s'",
                  image->filename);
                index=0;
              }
            indexes[x]=(IndexPacket) index;
            *q++=image->colormap[index];
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
        break;
      }
      case '2':
      {
        unsigned long
          intensity;

        /*
          Convert PGM image to pixel packets.
        */
        for (y=0; y < (long) image->rows; y++)
        {
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          for (x=0; x < (long) image->columns; x++)
          {
            intensity=PNMInteger(image,10);
            if (scale != (Quantum *) NULL)
              intensity=(unsigned long) scale[ConstrainPixel(image,(long)
                intensity,max_value)];
            index=(long) intensity;
            if ((unsigned long) index >= image->colors)
              {
                (void) ThrowMagickException(&image->exception,GetMagickModule(),
                  CorruptImageError,"InvalidColormapIndex","`%s'",
                  image->filename);
                index=0;
              }
            indexes[x]=(IndexPacket) index;
            *q++=image->colormap[index];
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
        break;
      }
      case '3':
      {
        MagickPixelPacket
          pixel;

        /*
          Convert PNM image to pixel packets.
        */
        for (y=0; y < (long) image->rows; y++)
        {
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            pixel.red=(MagickRealType) PNMInteger(image,10);
            pixel.green=(MagickRealType) PNMInteger(image,10);
            pixel.blue=(MagickRealType) PNMInteger(image,10);
            if (scale != (Quantum *) NULL)
              {
                pixel.red=(MagickRealType) scale[ConstrainPixel(image,(long)
                  pixel.red,max_value)];
                pixel.green=(MagickRealType) scale[ConstrainPixel(image,(long)
                  pixel.green,max_value)];
                pixel.blue=(MagickRealType) scale[ConstrainPixel(image,(long)
                  pixel.blue,max_value)];
              }
            q->red=(Quantum) pixel.red;
            q->green=(Quantum) pixel.green;
            q->blue=(Quantum) pixel.blue;
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
        break;
      }
      case '4':
      {
        unsigned long
          bit,
          byte;

        /*
          Convert PBM raw image to pixel packets.
        */
        for (y=0; y < (long) image->rows; y++)
        {
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          bit=0;
          byte=0;
          for (x=0; x < (long) image->columns; x++)
          {
            if (bit == 0)
              byte=(unsigned long) ReadBlobByte(image);
            index=(long) ((byte & 0x80) != 0 ? 0x00 : 0x01);
            indexes[x]=(IndexPacket) index;
            *q++=image->colormap[index];
            bit++;
            if (bit == 8)
              bit=0;
            byte<<=1;
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
        if (EOFBlob(image) != MagickFalse)
          ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
            image->filename);
        break;
      }
      case '5':
      {
        /*
          Convert PGM raw image to pixel packets.
        */
        packet_size=(size_t) (image->depth <= 8 ? 1 : 2);
        length=image->columns;
        pixels=(unsigned char *) AcquireQuantumMemory(length,packet_size*
          sizeof(*pixels));
        if (pixels == (unsigned char *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        length*=packet_size*sizeof(*pixels);
        for (y=0; y < (long) image->rows; y++)
        {
          count=ReadBlob(image,length,pixels);
          if (count != (ssize_t) length)
            ThrowReaderException(CorruptImageError,"UnableToReadImageData");
          p=pixels;
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          if (image->depth <= 8)
            for (x=0; x < (long) image->columns; x++)
            {
              index=(long) (*p++);
              if ((unsigned long) index >= image->colors)
                {
                  (void) ThrowMagickException(&image->exception,
                    GetMagickModule(),CorruptImageError,"InvalidColormapIndex",
                    "`%s'",image->filename);
                  index=0;
                }
              indexes[x]=(IndexPacket) index;
              *q++=image->colormap[index];
            }
          else
            for (x=0; x < (long) image->columns; x++)
            {
              index=(long) PushShortPixel(&p);
              if (index >= (long) image->colors)
                {
                  (void) ThrowMagickException(&image->exception,
                    GetMagickModule(),CorruptImageError,"InvalidColormapIndex",
                    "`%s'",image->filename);
                  index=0;
                }
              indexes[x]=(IndexPacket) index;
              *q++=image->colormap[index];
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
        pixels=(unsigned char *) RelinquishMagickMemory(pixels);
        if (EOFBlob(image) != MagickFalse)
          (void) ThrowMagickException(exception,GetMagickModule(),
            CorruptImageError,"UnexpectedEndOfFile","`%s'",image->filename);
        break;
      }
      case '6':
      {
        MagickPixelPacket
          pixel;

        /*
          Convert PNM raster image to pixel packets.
        */
        packet_size=(size_t) (image->depth <= 8 ? 3 : 6);
        if (image->colorspace == CMYKColorspace)
          packet_size+=(size_t) (image->depth <= 8 ? 1 : 2);
        pixels=(unsigned char *) AcquireQuantumMemory(image->columns,
          packet_size*sizeof(*pixels));
        if (pixels == (unsigned char *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        (void) ResetMagickMemory(&pixel,0,sizeof(LongPixelPacket));
        for (y=0; y < (long) image->rows; y++)
        {
          count=ReadBlob(image,packet_size*image->columns,pixels);
          if (count != (ssize_t) (packet_size*image->columns))
            ThrowReaderException(CorruptImageError,"UnableToReadImageData");
          p=pixels;
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          if (image->depth <= 8)
            for (x=0; x < (long) image->columns; x++)
            {
              pixel.red=(MagickRealType) PushCharPixel(&p);
              pixel.green=(MagickRealType) PushCharPixel(&p);
              pixel.blue=(MagickRealType) PushCharPixel(&p);
              if (scale != (Quantum *) NULL)
                {
                  pixel.red=(MagickRealType) scale[ConstrainPixel(image,(long)
                    pixel.red,max_value)];
                  pixel.green=(MagickRealType) scale[ConstrainPixel(image,(long)
                    pixel.green,max_value)];
                  pixel.blue=(MagickRealType) scale[ConstrainPixel(image,(long) 
                    pixel.blue,max_value)];
                }
              q->red=(Quantum) pixel.red;
              q->green=(Quantum) pixel.green;
              q->blue=(Quantum) pixel.blue;
              q++;
            }
          else
            for (x=0; x < (long) image->columns; x++)
            {
              pixel.red=(MagickRealType) PushShortPixel(&p);
              pixel.green=(MagickRealType) PushShortPixel(&p);
              pixel.blue=(MagickRealType) PushShortPixel(&p);
              if (scale != (Quantum *) NULL)
                {
                  pixel.red=(MagickRealType) scale[ConstrainPixel(image,(long)
                    pixel.red,max_value)];
                  pixel.green=(MagickRealType) scale[ConstrainPixel(image,(long)
                    pixel.green,max_value)];
                  pixel.blue=(MagickRealType) scale[ConstrainPixel(image,(long)
                    pixel.blue,max_value)];
                }
              q->red=(Quantum) pixel.red;
              q->green=(Quantum) pixel.green;
              q->blue=(Quantum) pixel.blue;
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
        pixels=(unsigned char *) RelinquishMagickMemory(pixels);
        if (EOFBlob(image) != MagickFalse)
          (void) ThrowMagickException(exception,GetMagickModule(),
            CorruptImageError,"UnexpectedEndOfFile","`%s'",image->filename);
        break;
      }
      case '7':
      {
        IndexPacket
          *indexes;

        unsigned long
          pixel;

        /*
          Convert PNM raster image to pixel packets.
        */
        packet_size=3;
        if (grayscale != MagickFalse)
          packet_size=1;
        if (image->matte != MagickFalse)
          packet_size++;
        if (image->colorspace == CMYKColorspace)
          packet_size++;
        if (image->depth > 8)
          packet_size*=2;
        length=image->columns;
        pixels=(unsigned char *) AcquireQuantumMemory(length,packet_size*
          sizeof(*pixels));
        if (pixels == (unsigned char *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        for (y=0; y < (long) image->rows; y++)
        {
          count=ReadBlob(image,packet_size*image->columns,pixels);
          if (count != (ssize_t) (packet_size*image->columns))
            ThrowReaderException(CorruptImageError,"UnableToReadImageData");
          p=pixels;
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          if (grayscale != MagickFalse)
            {
              if (image->depth <= 8)
                for (x=0; x < (long) image->columns; x++)
                {
                  pixel=PushCharPixel(&p);
                  q->red=ScaleAnyToQuantum(pixel,image->depth);
                  q->green=q->red;
                  q->blue=q->red;
                  if (image->colorspace == CMYKColorspace)
                    {
                      pixel=PushCharPixel(&p);
                      indexes[x]=(IndexPacket) ScaleAnyToQuantum(pixel,
                        image->depth);
                    }
                  if (image->matte != MagickFalse)
                    {
                      pixel=PushCharPixel(&p);
                      q->opacity=(Quantum) QuantumRange-ScaleAnyToQuantum(pixel,
                        image->depth);
                    }
                  q++;
                }
              else
                for (x=0; x < (long) image->columns; x++)
                {
                  pixel=PushShortPixel(&p);
                  q->red=ScaleAnyToQuantum(pixel,image->depth);
                  q->green=q->red;
                  q->blue=q->red;
                  if (image->colorspace == CMYKColorspace)
                    {
                      pixel=PushShortPixel(&p);
                      indexes[x]=(IndexPacket) ScaleAnyToQuantum(pixel,
                        image->depth);
                    }
                  if (image->matte != MagickFalse)
                    {
                      pixel=PushShortPixel(&p);
                      q->opacity=(Quantum) QuantumRange-ScaleAnyToQuantum(pixel,
                        image->depth);
                    }
                  q++;
                }
            }
          else
            if (image->depth <= 8)
              for (x=0; x < (long) image->columns; x++)
              {
                pixel=PushCharPixel(&p);
                q->red=ScaleAnyToQuantum(pixel,image->depth);
                pixel=PushCharPixel(&p);
                q->green=ScaleAnyToQuantum(pixel,image->depth);
                pixel=PushCharPixel(&p);
                q->blue=ScaleAnyToQuantum(pixel,image->depth);
                if (image->colorspace == CMYKColorspace)
                  {
                    pixel=PushCharPixel(&p);
                    indexes[x]=(IndexPacket) ScaleAnyToQuantum(pixel,
                      image->depth);
                  }
                if (image->matte != MagickFalse)
                  {
                    pixel=PushCharPixel(&p);
                    q->opacity=(Quantum) QuantumRange-ScaleAnyToQuantum(pixel,
                      image->depth);
                  }
                q++;
              }
            else
              for (x=0; x < (long) image->columns; x++)
              {
                pixel=PushShortPixel(&p);
                q->red=ScaleAnyToQuantum(pixel,image->depth);
                pixel=PushShortPixel(&p);
                q->green=ScaleAnyToQuantum(pixel,image->depth);
                pixel=PushShortPixel(&p);
                q->blue=ScaleAnyToQuantum(pixel,image->depth);
                if (image->colorspace == CMYKColorspace)
                  {
                    pixel=PushShortPixel(&p);
                    indexes[x]=(IndexPacket) ScaleAnyToQuantum(pixel,
                      image->depth);
                  }
                if (image->matte != MagickFalse)
                  {
                    pixel=PushShortPixel(&p);
                    q->opacity=(Quantum) QuantumRange-ScaleAnyToQuantum(pixel,
                      image->depth);
                  }
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
        pixels=(unsigned char *) RelinquishMagickMemory(pixels);
        if (EOFBlob(image) != MagickFalse)
          (void) ThrowMagickException(exception,GetMagickModule(),
            CorruptImageError,"UnexpectedEndOfFile","`%s'",image->filename);
        break;
      }
      case 'F':
      case 'f':
      {
        QuantumType
          quantum_type;

        ssize_t
          count;

        image->depth=32;
        quantum_info.format=FloatingPointQuantumFormat;
        quantum_info.scale=QuantumRange;
        quantum_type=format == 'f' ? GrayQuantum : RGBQuantum;
        length=(format == 'f' ? 1 : 3)*image->columns*sizeof(float);
        pixels=(unsigned char *) AcquireQuantumMemory(length,sizeof(pixels));
        if (pixels == (unsigned char *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        for (y=(long) image->rows-1; y >= 0; y--)
        {
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          count=ReadBlob(image,length,pixels);
          if ((size_t) count != length)
            break;
          status=ExportQuantumPixels(image,&quantum_info,quantum_type,pixels);
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
        pixels=(unsigned char *) RelinquishMagickMemory(pixels);
        if (EOFBlob(image) != MagickFalse)
          (void) ThrowMagickException(exception,GetMagickModule(),
            CorruptImageError,"UnexpectedEndOfFile","`%s'",image->filename);
        break;
      }
      default:
        ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    }
    if (scale != (Quantum *) NULL)
      scale=(Quantum *) RelinquishMagickMemory(scale);
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    if ((format == '1') || (format == '2') || (format == '3'))
      do
      {
        /*
          Skip to end of line.
        */
        count=ReadBlob(image,1,(unsigned char *) &format);
        if (count == 0)
          break;
        if ((count != 0) && (format == 'P'))
          break;
      } while (format != '\n');
    count=ReadBlob(image,1,(unsigned char *) &format);
    if ((count == 1) && (format == 'P'))
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
  } while ((count == 1) && (format == 'P'));
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r P N M I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterPNMImage() adds properties for the PNM image format to
%  the list of supported formats.  The properties include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterPNMImage method is:
%
%      unsigned long RegisterPNMImage(void)
%
*/
ModuleExport unsigned long RegisterPNMImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("PAM");
  entry->decoder=(DecodeImageHandler *) ReadPNMImage;
  entry->encoder=(EncodeImageHandler *) WritePNMImage;
  entry->description=ConstantString("Common 2-dimensional bitmap format");
  entry->module=ConstantString("PNM");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PBM");
  entry->decoder=(DecodeImageHandler *) ReadPNMImage;
  entry->encoder=(EncodeImageHandler *) WritePNMImage;
  entry->description=ConstantString("Portable bitmap format (black and white)");
  entry->module=ConstantString("PNM");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PFM");
  entry->decoder=(DecodeImageHandler *) ReadPNMImage;
  entry->encoder=(EncodeImageHandler *) WritePNMImage;
  entry->description=ConstantString("Portable float format");
  entry->module=ConstantString("PFM");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PGM");
  entry->decoder=(DecodeImageHandler *) ReadPNMImage;
  entry->encoder=(EncodeImageHandler *) WritePNMImage;
  entry->description=ConstantString("Portable graymap format (gray scale)");
  entry->module=ConstantString("PNM");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PNM");
  entry->decoder=(DecodeImageHandler *) ReadPNMImage;
  entry->encoder=(EncodeImageHandler *) WritePNMImage;
  entry->magick=(IsImageFormatHandler *) IsPNM;
  entry->description=ConstantString("Portable anymap");
  entry->module=ConstantString("PNM");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PPM");
  entry->decoder=(DecodeImageHandler *) ReadPNMImage;
  entry->encoder=(EncodeImageHandler *) WritePNMImage;
  entry->description=ConstantString("Portable pixmap format (color)");
  entry->module=ConstantString("PNM");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r P N M I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterPNMImage() removes format registrations made by the
%  PNM module from the list of supported formats.
%
%  The format of the UnregisterPNMImage method is:
%
%      UnregisterPNMImage(void)
%
*/
ModuleExport void UnregisterPNMImage(void)
{
  (void) UnregisterMagickInfo("PAM");
  (void) UnregisterMagickInfo("PBM");
  (void) UnregisterMagickInfo("PGM");
  (void) UnregisterMagickInfo("PNM");
  (void) UnregisterMagickInfo("PPM");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e P N M I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Procedure WritePNMImage() writes an image to a file in the PNM rasterfile
%  format.
%
%  The format of the WritePNMImage method is:
%
%      MagickBooleanType WritePNMImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
*/

static inline void PopCharPixel(const unsigned char pixel,
  unsigned char **pixels)
{
  *(*pixels)++=(unsigned char) (pixel);
}

static inline void PopShortPixel(const unsigned short pixel,
  unsigned char **pixels)
{
  *(*pixels)++=(unsigned char) ((pixel) >> 8);
  *(*pixels)++=(unsigned char) (pixel);
}

static inline void PopLongPixel(const unsigned long pixel,
  unsigned char **pixels)
{
  *(*pixels)++=(unsigned char) ((pixel) >> 24);
  *(*pixels)++=(unsigned char) ((pixel) >> 16);
  *(*pixels)++=(unsigned char) ((pixel) >> 8);
  *(*pixels)++=(unsigned char) (pixel);
}

static MagickBooleanType WritePNMImage(const ImageInfo *image_info,Image *image)
{
  char
    buffer[MaxTextExtent],
    format,
    magick[MaxTextExtent];

  const char
    *value;

  IndexPacket
    index;

  long
    y;

  MagickBooleanType
    grayscale,
    status;

  MagickOffsetType
    scene;

  QuantumInfo
    quantum_info;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  size_t
    length,
    packet_size;

  unsigned char
    *pixels,
    *q;

  unsigned long
    depth;

  /*
    Open output image file.
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
  scene=0;
  do
  {
    /*
      Write PNM file header.
    */
    GetQuantumInfo(image_info,&quantum_info);
    if (image_info->colorspace == UndefinedColorspace)
      (void) SetImageColorspace(image,RGBColorspace);
    grayscale=IsGrayImage(image,&image->exception);
    depth=image->depth;
    (void) CopyMagickString(magick,image_info->magick,MaxTextExtent);
    switch (magick[1])
    {
      case 'A':
      case 'a':
      {
        format='7';
        break;
      }
      case 'B':
      case 'b':
      {
        format='4';
        if (image->compression == NoCompression)
          format='1';
        break;
      }
      case 'F':
      case 'f':
      {
        format='F';
        if (IsGrayImage(image,&image->exception) != MagickFalse)
          format='f';
        break;
      }
      case 'G':
      case 'g':
      {
        format='5';
        if (image->compression == NoCompression)
          format='2';
        break;
      }
      case 'N':
      case 'n':
      {
        if ((image_info->type != TrueColorType) && (grayscale != MagickFalse))
          {
            format='5';
            if (image->compression == NoCompression)
              format='2';
            if (IsMonochromeImage(image,&image->exception) != MagickFalse)
              {
                format='4';
                if (image->compression == NoCompression)
                  format='1';
              }
          }
      }
      default:
      {
        format='6';
        if (image->compression == NoCompression)
          format='3';
        break;
      }
    }
    (void) FormatMagickString(buffer,MaxTextExtent,"P%c\n",format);
    (void) WriteBlobString(image,buffer);
    value=GetImageProperty(image,"Comment");
    if (value != (const char *) NULL)
      {
        register const char
          *p;

        /*
          Write comments to file.
        */
        (void) WriteBlobByte(image,'#');
        for (p=value; *p != '\0'; p++)
        {
          (void) WriteBlobByte(image,(unsigned char) *p);
          if ((*p == '\r') && (*(p+1) != '\0'))
            (void) WriteBlobByte(image,'#');
          if ((*p == '\n') && (*(p+1) != '\0'))
            (void) WriteBlobByte(image,'#');
        }
        (void) WriteBlobByte(image,'\n');
      }
    if (format != '7')
      {
        (void) FormatMagickString(buffer,MaxTextExtent,"%lu %lu\n",
          image->columns,image->rows);
        (void) WriteBlobString(image,buffer);
      }
    else
      {
        char
          type[MaxTextExtent];

        unsigned long
          extent;

        /*
          PAM header.
        */
        (void) FormatMagickString(buffer,MaxTextExtent,
          "WIDTH %lu\nHEIGHT %lu\n",image->columns,image->rows);
        (void) WriteBlobString(image,buffer);
        packet_size=3;
        (void) CopyMagickString(type,"RGB",MaxTextExtent);
        if (grayscale != MagickFalse)
          {
            packet_size=1;
            (void) CopyMagickString(type,"GRAYSCALE",MaxTextExtent);
            if (IsMonochromeImage(image,&image->exception))
              (void) CopyMagickString(type,"BLACKANDWHITE",MaxTextExtent);
          }
        if (image->matte != MagickFalse)
          {
            packet_size++;
            (void) ConcatenateMagickString(type,"_ALPHA",MaxTextExtent);
          }
        if (depth > 16)
          depth=16;
        extent=(1UL << depth)-1;
        (void) FormatMagickString(buffer,MaxTextExtent,
          "DEPTH %lu\nMAXVAL %lu\n",(unsigned long) packet_size,extent);
        (void) WriteBlobString(image,buffer);
        (void) FormatMagickString(buffer,MaxTextExtent,"TUPLTYPE %s\nENDHDR\n",
          type);
        (void) WriteBlobString(image,buffer);
      }
    /*
      Convert runlength encoded to PNM raster pixels.
    */
    switch (format)
    {
      case '1':
      {
        /*
          Convert image to a PBM image.
        */
        (void) SetImageType(image,BilevelType);
        i=0;
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          for (x=0; x < (long) image->columns; x++)
          {
            (void) FormatMagickString(buffer,MaxTextExtent,"%u ",
              PixelIntensity(p) < ((MagickRealType) QuantumRange/2.0) ?
              0x01 : 0x00);
            (void) WriteBlobString(image,buffer);
            i++;
            if (i == 36)
              {
                (void) WriteBlobByte(image,'\n');
                i=0;
              }
            p++;
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
        if (i != 0)
          (void) WriteBlobByte(image,'\n');
        break;
      }
      case '2':
      {
        /*
          Convert image to a PGM image.
        */
        if (image->depth <= 8)
          (void) WriteBlobString(image,"255\n");
        else
          (void) WriteBlobString(image,"65535\n");
        i=0;
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            index=PixelIntensityToQuantum(p);
            if (image->depth <= 8)
              (void) FormatMagickString(buffer,MaxTextExtent," %u",
                ScaleQuantumToChar(index));
            else
              (void) FormatMagickString(buffer,MaxTextExtent," %u",
                ScaleQuantumToShort(index));
            (void) WriteBlobString(image,buffer);
            i++;
            if (i == 12)
              {
                (void) WriteBlobByte(image,'\n');
                i=0;
              }
            p++;
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
        if (i != 0)
          (void) WriteBlobByte(image,'\n');
        break;
      }
      case '3':
      {
        /*
          Convert image to a PNM image.
        */
        if (image->depth <= 8)
          (void) WriteBlobString(image,"255\n");
        else
          (void) WriteBlobString(image,"65535\n");
        i=0;
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            if (image->depth <= 8)
              (void) FormatMagickString(buffer,MaxTextExtent,"%u %u %u ",
                ScaleQuantumToChar(p->red),ScaleQuantumToChar(p->green),
                ScaleQuantumToChar(p->blue));
            else
              (void) FormatMagickString(buffer,MaxTextExtent,"%u %u %u ",
                ScaleQuantumToShort(p->red),ScaleQuantumToShort(p->green),
                ScaleQuantumToShort(p->blue));
            (void) WriteBlobString(image,buffer);
            i++;
            if (i == 4)
              {
                (void) WriteBlobByte(image,'\n');
                i=0;
              }
            p++;
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
        if (i != 0)
          (void) WriteBlobByte(image,'\n');
        break;
      }
      case '4':
      {
        unsigned long
          bit,
          byte;

        /*
          Convert image to a PBM image.
        */
        (void) SetImageType(image,BilevelType);
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          bit=0;
          byte=0;
          for (x=0; x < (long) image->columns; x++)
          {
            byte<<=1;
            if (PixelIntensity(p) < ((MagickRealType) QuantumRange/2.0))
              byte|=0x01;
            bit++;
            if (bit == 8)
              {
                (void) WriteBlobByte(image,(unsigned char) byte);
                bit=0;
                byte=0;
              }
            p++;
          }
          if (bit != 0)
            (void) WriteBlobByte(image,(unsigned char) (byte << (8-bit)));
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
      case '5':
      {
        /*
          Convert image to a PGM image.
        */
        if (image->depth <= 8)
          (void) WriteBlobString(image,"255\n");
        else
          (void) WriteBlobString(image,"65535\n");
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          if (image->depth <= 8)
            for (x=0; x < (long) image->columns; x++)
            {
              (void) WriteBlobByte(image,ScaleQuantumToChar(
                PixelIntensityToQuantum(p)));
              p++;
            }
          else
            for (x=0; x < (long) image->columns; x++)
            {
              (void) WriteBlobMSBShort(image,ScaleQuantumToShort(
                PixelIntensityToQuantum(p)));
              p++;
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
      case '6':
      {
        /*
          Allocate memory for pixels.
        */
        packet_size=(size_t) (image->depth <= 8 ? 3 : 6);
        length=image->columns;
        pixels=(unsigned char *) AcquireQuantumMemory(length,packet_size*
          sizeof(*pixels));
        if (pixels == (unsigned char *) NULL)
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
        /*
          Convert image to a PNM image.
        */
        if (image->depth <= 8)
          (void) WriteBlobString(image,"255\n");
        else
          (void) WriteBlobString(image,"65535\n");
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          q=pixels;
          if (image->depth <= 8)
            for (x=0; x < (long) image->columns; x++)
            {
              *q++=ScaleQuantumToChar(p->red);
              *q++=ScaleQuantumToChar(p->green);
              *q++=ScaleQuantumToChar(p->blue);
              p++;
            }
          else
            for (x=0; x < (long) image->columns; x++)
            {
              *q++=(unsigned char) (ScaleQuantumToShort(p->red) >> 8);
              *q++=(unsigned char) ScaleQuantumToShort(p->red);
              *q++=(unsigned char) (ScaleQuantumToShort(p->green) >> 8);
              *q++=(unsigned char) ScaleQuantumToShort(p->green);
              *q++=(unsigned char) (ScaleQuantumToShort(p->blue) >> 8);
              *q++=(unsigned char) ScaleQuantumToShort(p->blue);
              p++;
            }
          (void) WriteBlob(image,(size_t) (q-pixels),pixels);
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
        pixels=(unsigned char *) RelinquishMagickMemory(pixels);
        break;
      }
      case '7':
      {
        /*
          Convert image to a PAM.
        */
        packet_size=3;
        if (grayscale != MagickFalse)
          packet_size=1;
        if (image->matte != MagickFalse)
          packet_size++;
        if (depth > 8)
          packet_size*=2;
        length=image->columns;
        pixels=(unsigned char *) AcquireQuantumMemory(length,packet_size*
          sizeof(*pixels));
        if (pixels == (unsigned char *) NULL)
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          q=pixels;
          if (grayscale != MagickFalse)
            {
              if (depth <= 8)
                for (x=0; x < (long) image->columns; x++)
                {
                  unsigned char
                    pixel;

                  pixel=(unsigned char) ScaleQuantumToAny(
                    PixelIntensityToQuantum(p),depth);
                  PopCharPixel(pixel,&q);
                  if (image->matte != MagickFalse)
                    {
                      pixel=(unsigned char) ScaleQuantumToAny((Quantum)
                        (QuantumRange-p->opacity),depth);
                      PopCharPixel(pixel,&q);
                    }
                  p++;
                }
              else
                for (x=0; x < (long) image->columns; x++)
                {
                  unsigned short
                    pixel;

                  pixel=(unsigned short) ScaleQuantumToAny(
                    PixelIntensityToQuantum(p),depth);
                  PopShortPixel(pixel,&q);
                  if (image->matte != MagickFalse)
                    {
                      pixel=(unsigned short) ScaleQuantumToAny((Quantum)
                        (QuantumRange-p->opacity),depth);
                      PopShortPixel(pixel,&q);
                    }
                  p++;
                }
            }
          else
            if (depth <= 8)
              for (x=0; x < (long) image->columns; x++)
              {
                unsigned char
                  pixel;

                pixel=(unsigned char) ScaleQuantumToAny(p->red,depth);
                PopCharPixel(pixel,&q);
                pixel=(unsigned char) ScaleQuantumToAny(p->green,depth);
                PopCharPixel(pixel,&q);
                pixel=(unsigned char) ScaleQuantumToAny(p->blue,depth);
                PopCharPixel(pixel,&q);
                if (image->matte != MagickFalse)
                  {
                    pixel=(unsigned char) ScaleQuantumToAny((Quantum)
                      (QuantumRange-p->opacity),depth);
                    PopCharPixel(pixel,&q);
                  }
                p++;
              }
            else
              for (x=0; x < (long) image->columns; x++)
              {
                unsigned short
                  pixel;

                pixel=(unsigned short) ScaleQuantumToAny(p->red,depth);
                PopShortPixel(pixel,&q);
                pixel=(unsigned short) ScaleQuantumToAny(p->green,depth);
                PopShortPixel(pixel,&q);
                pixel=(unsigned short) ScaleQuantumToAny(p->blue,depth);
                PopShortPixel(pixel,&q);
                if (image->matte != MagickFalse)
                  {
                    pixel=(unsigned short) ScaleQuantumToAny((Quantum)
                      (QuantumRange-p->opacity),depth);
                    PopShortPixel(pixel,&q);
                  }
                p++;
              }
          (void) WriteBlob(image,(size_t) (q-pixels),pixels);
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
        pixels=(unsigned char *) RelinquishMagickMemory(pixels);
        break;
      }
      case 'F':
      case 'f':
      {
        QuantumType
          quantum_type;

        (void) WriteBlobString(image,image->endian != LSBEndian ?
          "1.0\n" : "-1.0\n");
        image->depth=32;
        quantum_info.format=FloatingPointQuantumFormat;
        quantum_info.scale=1.0/QuantumRange;
        quantum_type=format == 'f' ? GrayQuantum : RGBQuantum;
        length=(format == 'f' ? 1 : 3)*image->columns*sizeof(float);
        pixels=(unsigned char *) AcquireQuantumMemory(length,
          sizeof(*pixels));
        if (pixels == (unsigned char *) NULL)
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
        for (y=(long) image->rows-1; y >= 0; y--)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          (void) ImportQuantumPixels(image,&quantum_info,quantum_type,pixels);
          (void) WriteBlob(image,length,pixels);
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
        pixels=(unsigned char *) RelinquishMagickMemory(pixels);
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
  CloseBlob(image);
  return(MagickTrue);
}
