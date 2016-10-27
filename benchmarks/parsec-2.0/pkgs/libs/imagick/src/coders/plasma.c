/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                 PPPP   L       AAA   SSSSS  M   M   AAA                     %
%                 P   P  L      A   A  SS     MM MM  A   A                    %
%                 PPPP   L      AAAAA   SSS   M M M  AAAAA                    %
%                 P      L      A   A     SS  M   M  A   A                    %
%                 P      LLLLL  A   A  SSSSS  M   M  A   A                    %
%                                                                             %
%                                                                             %
%                          Read a Plasma Image.                               %
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
#include "magick/constitute.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/random_.h"
#include "magick/signature.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/module.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d P L A S M A I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadPlasmaImage creates a plasma fractal image.  The image is
%  initialized to to the X server color as specified by the filename.
%
%  The format of the ReadPlasmaImage method is:
%
%      Image *ReadPlasmaImage(const ImageInfo *image_info,
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

static inline void PlasmaPixel(Image *image,double x,double y)
{
  register PixelPacket
    *q;

  q=GetImagePixels(image,(long) ceil(x-0.5),(long) ceil(y-0.5),1,1);
  if (q == (PixelPacket *) NULL)
    return;
  q->red=ScaleAnyToQuantum((unsigned long)
    (65535.0*GetRandomValue()+0.5),16UL);
  q->green=ScaleAnyToQuantum((unsigned long)
    (65535.0*GetRandomValue()+0.5),16UL);
  q->blue=ScaleAnyToQuantum((unsigned long)
    (65535.0*GetRandomValue()+0.5),16UL);
  (void) SyncImagePixels(image);
}

static Image *ReadPlasmaImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
#define PlasmaImageTag  "Plasma/Image"

  Image
    *image;

  ImageInfo
    *read_info;

  long
    y;

  MagickBooleanType
    status;

  register long
    x;

  register PixelPacket
    *q;

  register unsigned long
    i;

  SegmentInfo
    segment_info;

  unsigned long
    depth,
    max_depth;

  /*
    Recursively apply plasma to the image.
  */
  read_info=CloneImageInfo(image_info);
  SetImageInfoBlob(read_info,(void *) NULL,0);
  (void) FormatMagickString(read_info->filename,MaxTextExtent,
    "gradient:%s",image_info->filename);
  image=ReadImage(read_info,exception);
  read_info=DestroyImageInfo(read_info);
  if (image == (Image *) NULL)
    return((Image *) NULL);
  image->storage_class=DirectClass;
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      q->opacity=(Quantum) (QuantumRange/2);
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
  }
  segment_info.x1=0;
  segment_info.y1=0;
  segment_info.x2=(double) image->columns-1;
  segment_info.y2=(double) image->rows-1;
  if (LocaleCompare(image_info->filename,"fractal") == 0)
    {
      /*
        Seed pixels before recursion.
      */
      PlasmaPixel(image,segment_info.x1,segment_info.y1);
      PlasmaPixel(image,segment_info.x1,(segment_info.y1+segment_info.y2)/2);
      PlasmaPixel(image,segment_info.x1,segment_info.y2);
      PlasmaPixel(image,(segment_info.x1+segment_info.x2)/2,segment_info.y1);
      PlasmaPixel(image,(segment_info.x1+segment_info.x2)/2,
        (segment_info.y1+segment_info.y2)/2);
      PlasmaPixel(image,(segment_info.x1+segment_info.x2)/2,segment_info.y2);
      PlasmaPixel(image,segment_info.x2,segment_info.y1);
      PlasmaPixel(image,segment_info.x2,(segment_info.y1+segment_info.y2)/2);
      PlasmaPixel(image,segment_info.x2,segment_info.y2);
    }
  i=(unsigned long) MagickMax(image->columns,image->rows)/2;
  for (max_depth=0; i != 0; max_depth++)
    i>>=1;
  for (depth=1; ; depth++)
  {
    if (PlasmaImage(image,&segment_info,0,depth) != MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick((MagickOffsetType) depth,max_depth) != MagickFalse))
      {
        status=image->progress_monitor(PlasmaImageTag,(MagickOffsetType) depth,
          max_depth,image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  (void) SetImageOpacity(image,OpaqueOpacity);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r P L A S M A I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterPLASMAImage() adds attributes for the Plasma image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterPLASMAImage method is:
%
%      unsigned long RegisterPLASMAImage(void)
%
*/
ModuleExport unsigned long RegisterPLASMAImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("PLASMA");
  entry->decoder=(DecodeImageHandler *) ReadPlasmaImage;
  entry->adjoin=MagickFalse;
  entry->description=ConstantString("Plasma fractal image");
  entry->module=ConstantString("PLASMA");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("FRACTAL");
  entry->decoder=(DecodeImageHandler *) ReadPlasmaImage;
  entry->adjoin=MagickFalse;
  entry->description=ConstantString("Plasma fractal image");
  entry->module=ConstantString("PLASMA");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r P L A S M A I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterPLASMAImage() removes format registrations made by the
%  PLASMA module from the list of supported formats.
%
%  The format of the UnregisterPLASMAImage method is:
%
%      UnregisterPLASMAImage(void)
%
*/
ModuleExport void UnregisterPLASMAImage(void)
{
  (void) UnregisterMagickInfo("FRACTAL");
  (void) UnregisterMagickInfo("PLASMA");
}
