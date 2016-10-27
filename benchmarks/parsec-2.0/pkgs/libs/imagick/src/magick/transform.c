/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%       TTTTT  RRRR    AAA   N   N  SSSSS  FFFFF   OOO   RRRR   M   M         %
%         T    R   R  A   A  NN  N  SS     F      O   O  R   R  MM MM         %
%         T    RRRR   AAAAA  N N N   SSS   FFF    O   O  RRRR   M M M         %
%         T    R R    A   A  N  NN     SS  F      O   O  R R    M   M         %
%         T    R  R   A   A  N   N  SSSSS  F       OOO   R  R   M   M         %
%                                                                             %
%                                                                             %
%                   ImageMagick Image Transform Methods                       %
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
#include "magick/cache-view.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/composite.h"
#include "magick/effect.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/memory_.h"
#include "magick/list.h"
#include "magick/monitor.h"
#include "magick/pixel-private.h"
#include "magick/resource_.h"
#include "magick/resize.h"
#include "magick/statistic.h"
#include "magick/string_.h"
#include "magick/transform.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C h o p I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Chop() removes a region of an image and collapses the image to occupy the
%  removed portion.
%
%  The format of the ChopImage method is:
%
%      Image *ChopImage(const Image *image,const RectangleInfo *chop_info)
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o chop_info: Define the region of the image to chop.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *ChopImage(const Image *image,const RectangleInfo *chop_info,
  ExceptionInfo *exception)
{
#define ChopImageTag  "Chop/Image"

  Image
    *chop_image;

  long
    j,
    y;

  MagickBooleanType
    status;

  RectangleInfo
    extent;

  register const PixelPacket
    *p;

  register IndexPacket
    *chop_indexes,
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  /*
    Check chop geometry.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  assert(chop_info != (RectangleInfo *) NULL);
  if (((chop_info->x+(long) chop_info->width) < 0) ||
      ((chop_info->y+(long) chop_info->height) < 0) ||
      (chop_info->x > (long) image->columns) ||
      (chop_info->y > (long) image->rows))
    ThrowImageException(OptionWarning,"GeometryDoesNotContainImage");
  extent=(*chop_info);
  if ((extent.x+(long) extent.width) > (long) image->columns)
    extent.width=(unsigned long) ((long) image->columns-extent.x);
  if ((extent.y+(long) extent.height) > (long) image->rows)
    extent.height=(unsigned long) ((long) image->rows-extent.y);
  if (extent.x < 0)
    {
      extent.width-=(unsigned long) (-extent.x);
      extent.x=0;
    }
  if (extent.y < 0)
    {
      extent.height-=(unsigned long) (-extent.y);
      extent.y=0;
    }
  /*
    Initialize chop image attributes.
  */
  chop_image=CloneImage(image,image->columns-extent.width,image->rows-
    extent.height,MagickTrue,exception);
  if (chop_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Extract chop image.
  */
  i=0;
  j=0;
  for (y=0; y < (long) extent.y; y++)
  {
    p=AcquireImagePixels(image,0,i++,image->columns,1,exception);
    q=SetImagePixels(chop_image,0,j++,chop_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    chop_indexes=GetIndexes(chop_image);
    for (x=0; x < (long) image->columns; x++)
    {
      if ((x < extent.x) || (x >= (long) (extent.x+extent.width)))
        {
          if ((indexes != (IndexPacket *) NULL) &&
              (chop_indexes != (IndexPacket *) NULL))
            *chop_indexes++=indexes[x];
          *q=(*p);
          q++;
        }
      p++;
    }
    if (SyncImagePixels(chop_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(j,chop_image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ChopImageTag,j,chop_image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  /*
    Extract chop image.
  */
  i+=extent.height;
  for (y=0; y < (long) (image->rows-(extent.y+extent.height)); y++)
  {
    p=AcquireImagePixels(image,0,i++,image->columns,1,exception);
    q=SetImagePixels(chop_image,0,j++,chop_image->columns,1);
    if ((p == (PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    chop_indexes=GetIndexes(chop_image);
    for (x=0; x < (long) image->columns; x++)
    {
      if ((x < extent.x) || (x >= (long) (extent.x+extent.width)))
        {
          if ((indexes != (IndexPacket *) NULL) &&
              (chop_indexes != (IndexPacket *) NULL))
            *chop_indexes++=indexes[x];
          *q=(*p);
          q++;
        }
      p++;
    }
    if (SyncImagePixels(chop_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(j,chop_image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ChopImageTag,j,chop_image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(chop_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     C o n s o l i d a t e C M Y K I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConsolidateCMYKImage() consolidates separate C, M, Y, and K planes into a
%  single image.
%
%  The format of the ConsolidateCMYKImage method is:
%
%      Image *ConsolidateCMYKImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image sequence.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *ConsolidateCMYKImages(const Image *images,
  ExceptionInfo *exception)
{
  Image
    *cmyk_image,
    *cmyk_images;

  long
    y;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  /*
    Consolidate separate C, M, Y, and K planes into a single image.
  */
  assert(images != (Image *) NULL);
  assert(images->signature == MagickSignature);
  if (images->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  cmyk_images=NewImageList();
  for (i=0; i < (long) GetImageListLength(images); i+=4)
  {
    cmyk_image=CloneImage(images,images->columns,images->rows,MagickTrue,
      exception);
    if (cmyk_image == (Image *) NULL)
      break;
    if (SetImageStorageClass(cmyk_image,DirectClass) == MagickFalse)
      break;
    cmyk_image->colorspace=CMYKColorspace;
    for (y=0; y < (long) images->rows; y++)
    {
      p=AcquireImagePixels(images,0,y,images->columns,1,exception);
      q=GetImagePixels(cmyk_image,0,y,cmyk_image->columns,1);
      if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
        break;
      for (x=0; x < (long) images->columns; x++)
      {
        q->red=(Quantum) (QuantumRange-PixelIntensityToQuantum(p));
        p++;
        q++;
      }
      if (SyncImagePixels(cmyk_image) == MagickFalse)
        break;
    }
    images=GetNextImageInList(images);
    if (images == (Image *) NULL)
      break;
    for (y=0; y < (long) images->rows; y++)
    {
      p=AcquireImagePixels(images,0,y,images->columns,1,exception);
      q=GetImagePixels(cmyk_image,0,y,cmyk_image->columns,1);
      if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
        break;
      for (x=0; x < (long) images->columns; x++)
      {
        q->green=(Quantum) (QuantumRange-PixelIntensityToQuantum(p));
        p++;
        q++;
      }
      if (SyncImagePixels(cmyk_image) == MagickFalse)
        break;
    }
    images=GetNextImageInList(images);
    if (images == (Image *) NULL)
      break;
    for (y=0; y < (long) images->rows; y++)
    {
      p=AcquireImagePixels(images,0,y,images->columns,1,exception);
      q=GetImagePixels(cmyk_image,0,y,cmyk_image->columns,1);
      if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
        break;
      for (x=0; x < (long) images->columns; x++)
      {
        q->blue=(Quantum) (QuantumRange-PixelIntensityToQuantum(p));
        p++;
        q++;
      }
      if (SyncImagePixels(cmyk_image) == MagickFalse)
        break;
    }
    images=GetNextImageInList(images);
    if (images == (Image *) NULL)
      break;
    for (y=0; y < (long) images->rows; y++)
    {
      p=AcquireImagePixels(images,0,y,images->columns,1,exception);
      q=GetImagePixels(cmyk_image,0,y,cmyk_image->columns,1);
      if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
        break;
      indexes=GetIndexes(cmyk_image);
      for (x=0; x < (long) images->columns; x++)
      {
        indexes[x]=(IndexPacket) (QuantumRange-PixelIntensityToQuantum(p));
        p++;
      }
      if (SyncImagePixels(cmyk_image) == MagickFalse)
        break;
    }
    AppendImageToList(&cmyk_images,cmyk_image);
    images=GetNextImageInList(images);
    if (images == (Image *) NULL)
      break;
  }
  return(cmyk_images);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C r o p I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CropImage() extracts a region of the image starting at the offset
%  defined by geometry.
%
%  The format of the CropImage method is:
%
%      Image *CropImage(const Image *image,const RectangleInfo *geometry,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o geometry: Define the region of the image to crop with members
%      x, y, width, and height.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *CropImage(const Image *image,const RectangleInfo *geometry,
  ExceptionInfo *exception)
{
#define CropImageTag  "Crop/Image"

  Image
    *crop_image;

  long
    y;

  MagickBooleanType
    status;

  RectangleInfo
    bounding_box,
    page;

  register const IndexPacket
    *indexes;

  register const PixelPacket
    *p;

  register IndexPacket
    *crop_indexes;

  register PixelPacket
    *q;

  ViewInfo
    *crop_view,
    *image_view;

  /*
    Check crop geometry.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(geometry != (const RectangleInfo *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  bounding_box=image->page;
  if ((bounding_box.width == 0) || (bounding_box.height == 0))
    {
      bounding_box.width=image->columns;
      bounding_box.height=image->rows;
    }
  page=(*geometry);
  if (page.width == 0)
    page.width=bounding_box.width;
  if (page.height == 0)
    page.height=bounding_box.height;
  if (((bounding_box.x-page.x) >= (long) page.width) ||
      ((bounding_box.y-page.y) >= (long) page.height) ||
      ((page.x-bounding_box.x) > (long) image->columns) ||
      ((page.y-bounding_box.y) > (long) image->rows))
    {
      /*
        Crop missed the image on the virtual canvas
        Give warnign and return a 'missed image'
        Special case, for 'background' disposed GIF animation frames.
      */
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "GeometryDoesNotContainImage","`%s'",image->filename);
      crop_image=CloneImage(image,1,1,MagickTrue,exception);
      if (crop_image == (Image *) NULL)
        return((Image *) NULL);
      crop_image->background_color.opacity=(Quantum) TransparentOpacity;
      (void) SetImageBackgroundColor(crop_image);
      crop_image->page=bounding_box;
      crop_image->page.x=(-1);
      crop_image->page.y=(-1);
      if ( crop_image->dispose == BackgroundDispose )
        crop_image->dispose = NoneDispose;
      return(crop_image);
    }
  if ((page.x < 0) && (bounding_box.x >= 0))
    {
      page.width+=page.x-bounding_box.x;
      page.x=0;
    }
  else
    {
      page.width-=bounding_box.x-page.x;
      page.x-=bounding_box.x;
      if (page.x < 0)
        page.x=0;
    }
  if ((page.y < 0) && (bounding_box.y >= 0))
    {
      page.height+=page.y-bounding_box.y;
      page.y=0;
    }
  else
    {
      page.height-=bounding_box.y-page.y;
      page.y-=bounding_box.y;
      if (page.y < 0)
        page.y=0;
    }
  if ((unsigned long) (page.x+page.width) > image->columns)
    page.width=image->columns-page.x;
  if (geometry->width != 0)
    if (page.width > geometry->width)
      page.width=geometry->width;
  if ((unsigned long) (page.y+page.height) > image->rows)
    page.height=image->rows-page.y;
  if (geometry->height != 0)
    if (page.height > geometry->height)
      page.height=geometry->height;
  bounding_box.x+=page.x;
  bounding_box.y+=page.y;
  /*
    Initialize crop image attributes.
  */
  crop_image=CloneImage(image,page.width,page.height,MagickTrue,exception);
  if (crop_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Extract crop image.
  */
  crop_image->page.width=image->page.width;
  crop_image->page.height=image->page.height;
  if (((long) (bounding_box.x+bounding_box.width) > (long) image->page.width) ||
      ((long) (bounding_box.y+bounding_box.height) > (long) image->page.height))
    {
      crop_image->page.width=bounding_box.width;
      crop_image->page.height=bounding_box.height;
    }
  crop_image->page.x=bounding_box.x;
  crop_image->page.y=bounding_box.y;
  image_view=OpenCacheView(image);
  crop_view=OpenCacheView(crop_image);
  for (y=0; y < (long) crop_image->rows; y++)
  {
    p=AcquireCacheViewPixels(image_view,page.x,page.y+y,crop_image->columns,1,
      exception);
    q=SetCacheView(crop_view,0,y,crop_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    (void) CopyMagickMemory(q,p,(size_t) crop_image->columns*sizeof(*q));
    indexes=AcquireCacheViewIndexes(image_view);
    crop_indexes=GetCacheViewIndexes(crop_view);
    if ((indexes != (IndexPacket *) NULL) &&
        (crop_indexes != (IndexPacket *) NULL))
      (void) CopyMagickMemory(crop_indexes,indexes,(size_t) crop_image->columns*
        sizeof(*crop_indexes));
    if (SyncCacheView(crop_view) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(CropImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  crop_view=CloseCacheView(crop_view);
  image_view=CloseCacheView(image_view);
  return(crop_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x c e r p t I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExcerptImage() returns a excerpt of the image as defined by the geometry.
%
%  The format of the ExcerptImage method is:
%
%      Image *ExcerptImage(const Image *image,const RectangleInfo *geometry,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o geometry: Define the region of the image to extend with members
%      x, y, width, and height.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *ExcerptImage(const Image *image,
  const RectangleInfo *geometry,ExceptionInfo *exception)
{
#define ExcerptImageTag  "Flip/Image"

  Image
    *excerpt_image;

  long
    y;

  MagickBooleanType
    status;

  register IndexPacket
    *excerpt_indexes,
    *indexes;

  register const PixelPacket
    *p;

  register PixelPacket
    *q;

  /*
    Allocate excerpt image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(geometry != (const RectangleInfo *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  excerpt_image=CloneImage(image,geometry->width,geometry->height,MagickTrue,
    exception);
  if (excerpt_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Excerpt each row.
  */
  for (y=0; y < (long) excerpt_image->rows; y++)
  {
    p=AcquireImagePixels(image,geometry->x,geometry->y+y,geometry->width,1,
      exception);
    q=GetImagePixels(excerpt_image,0,y,excerpt_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    (void) CopyMagickMemory(q,p,(size_t) excerpt_image->columns*sizeof(*q));
    indexes=GetIndexes(image);
    excerpt_indexes=GetIndexes(excerpt_image);
    if ((indexes != (IndexPacket *) NULL) &&
        (excerpt_indexes != (IndexPacket *) NULL))
      (void) CopyMagickMemory(excerpt_indexes,indexes,(size_t)
        excerpt_image->columns*sizeof(*excerpt_indexes));
    if (SyncImagePixels(excerpt_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,excerpt_image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ExcerptImageTag,y,excerpt_image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(excerpt_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x t e n t I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExtentImage() extends the image as defined by the geometry, gravity, and
%  image background color.  Set the (x,y) offset of the geometry to move the 
%  original image relative to the extended image.
%
%  The format of the ExtentImage method is:
%
%      Image *ExtentImage(const Image *image,const RectangleInfo *geometry,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o geometry: Define the region of the image to extend with members
%      x, y, width, and height.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *ExtentImage(const Image *image,
  const RectangleInfo *geometry,ExceptionInfo *exception)
{
  Image
    *extent_image;

  /*
    Allocate extent image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(geometry != (const RectangleInfo *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  extent_image=CloneImage(image,geometry->width,geometry->height,MagickTrue,
    exception);
  if (extent_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(extent_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&extent_image->exception);
      extent_image=DestroyImage(extent_image);
      return((Image *) NULL);
    }
  if (image->background_color.opacity != OpaqueOpacity)
    extent_image->matte=MagickTrue;
  (void) SetImageBackgroundColor(extent_image);
  (void) CompositeImage(extent_image,image->compose,image,-geometry->x,
    -geometry->y);
  return(extent_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     F l a t t e n I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FlattenImage() composites all images from the current image pointer to the
%  end of the image list and returns a single flattened image.
%
%  The format of the FlattenImage method is:
%
%      Image *FlattenImage(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image sequence.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *FlattenImages(Image *image,ExceptionInfo *exception)
{
#define FlattenImageTag  "Flatten/Image"

  Image
    *flatten_image;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  RectangleInfo
    page;

  unsigned long
    number_images;

  /*
    Determine flatten bounding box.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  page=image->page;
  if (page.width == 0)
    page.width=image->columns;
  if (page.height == 0)
    page.height=image->rows;
  page.x=0;
  page.y=0;
  /*
    Allocate flatten image.
  */
  flatten_image=CloneImage(image,page.width,page.height,MagickTrue,exception);
  if (flatten_image == (Image *) NULL)
    return((Image *) NULL);
  (void) SetImageBackgroundColor(flatten_image);
  flatten_image->page=page;
  /*
    Flatten the images.
  */
  number_images=GetImageListLength(image)-GetImageIndexInList(image);
  for (scene=0; image != (Image *) NULL; scene++)
  {
    (void) CompositeImage(flatten_image,image->compose,image,image->page.x,
      image->page.y);
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(scene,number_images) != MagickFalse))
      {
        status=image->progress_monitor(FlattenImageTag,scene,number_images,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
    image=GetNextImageInList(image);
  }
  return(flatten_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   F l i p I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FlipImage() creates a vertical mirror image by reflecting the pixels
%  around the central x-axis.
%
%  The format of the FlipImage method is:
%
%      Image *FlipImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *FlipImage(const Image *image,ExceptionInfo *exception)
{
#define FlipImageTag  "Flip/Image"

  Image
    *flip_image;

  long
    y;

  MagickBooleanType
    status;

  register const PixelPacket
    *p;

  register IndexPacket
    *flip_indexes,
    *indexes;

  register PixelPacket
    *q;

  /*
    Initialize flip image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  flip_image=CloneImage(image,image->columns,image->rows,MagickTrue,exception);
  if (flip_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Flip each row.
  */
  for (y=0; y < (long) flip_image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=GetImagePixels(flip_image,0,(long) (flip_image->rows-y-1),
      flip_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    (void) CopyMagickMemory(q,p,(size_t) flip_image->columns*sizeof(*q));
    indexes=GetIndexes(image);
    flip_indexes=GetIndexes(flip_image);
    if ((indexes != (IndexPacket *) NULL) &&
        (flip_indexes != (IndexPacket *) NULL))
      (void) CopyMagickMemory(flip_indexes,indexes,(size_t) flip_image->columns*
        sizeof(*flip_indexes));
    if (SyncImagePixels(flip_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,flip_image->rows) != MagickFalse))
      {
        status=image->progress_monitor(FlipImageTag,y,flip_image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(flip_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   F l o p I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FlopImage() creates a horizontal mirror image by reflecting the pixels
%  around the central y-axis.
%
%  The format of the FlopImage method is:
%
%      Image *FlopImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *FlopImage(const Image *image,ExceptionInfo *exception)
{
#define FlopImageTag  "Flop/Image"

  Image
    *flop_image;

  long
    y;

  MagickBooleanType
    status;

  register IndexPacket
    *flop_indexes,
    *indexes;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Initialize flop image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  flop_image=CloneImage(image,image->columns,image->rows,MagickTrue,exception);
  if (flop_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Flop each row.
  */
  for (y=0; y < (long) flop_image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=SetImagePixels(flop_image,0,y,flop_image->columns,1);
    if ((p == (PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    flop_indexes=GetIndexes(flop_image);
    q+=flop_image->columns;
    for (x=0; x < (long) flop_image->columns; x++)
    {
      if ((indexes != (IndexPacket *) NULL) &&
          (flop_indexes != (IndexPacket *) NULL))
        flop_indexes[flop_image->columns-x-1]=indexes[x];
      q--;
      *q=(*p);
      p++;
    }
    if (SyncImagePixels(flop_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,flop_image->rows) != MagickFalse))
      {
        status=image->progress_monitor(FlopImageTag,y,flop_image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(flop_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     M o s a i c I m a g e s                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MosaicImages() inlays an image sequence to form a single coherent picture.
%  It returns a single image with each image in the sequence composited at
%  the location defined by the page member of the image structure.
%
%  The format of the MosaicImage method is:
%
%      Image *MosaicImages(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *MosaicImages(Image *image,ExceptionInfo *exception)
{
#define MosaicImageTag  "Mosaic/Image"

  Image
    *mosaic_image;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  RectangleInfo
    page;

  register const Image
    *next;

  unsigned long
    number_images;

  /*
    Determine mosaic bounding box.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  (void) ResetMagickMemory(&page,0,sizeof(page));
  page.width=image->columns;
  page.height=image->rows;
  for (next=image; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    page.x=next->page.x;
    page.y=next->page.y;
    if ((next->columns+page.x) > page.width)
      page.width=next->columns+page.x;
    if (next->page.width > page.width)
      page.width=next->page.width;
    if ((next->rows+page.y) > page.height)
      page.height=next->rows+page.y;
    if (next->page.height > page.height)
      page.height=next->page.height;
  }
  page.x=0;
  page.y=0;
  /*
    Allocate mosaic image.
  */
  mosaic_image=CloneImage(image,page.width,page.height,MagickTrue,exception);
  if (mosaic_image == (Image *) NULL)
    return((Image *) NULL);
  (void) SetImageBackgroundColor(mosaic_image);
  mosaic_image->page=page;
  /*
    Mosaic the images.
  */
  number_images=GetImageListLength(image);
  for (scene=0; scene < (long) number_images; scene++)
  {
    (void) CompositeImage(mosaic_image,image->compose,image,image->page.x,
      image->page.y);
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(scene,number_images) != MagickFalse))
      {
        status=image->progress_monitor(MosaicImageTag,scene,number_images,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
    image=GetNextImageInList(image);
  }
  return(mosaic_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R o l l I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RollImage() offsets an image as defined by x_offset and y_offset.
%
%  The format of the RollImage method is:
%
%      Image *RollImage(const Image *image,const long x_offset,
%        const long y_offset,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o x_offset: The number of columns to roll in the horizontal direction.
%
%    o y_offset: The number of rows to roll in the vertical direction.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *RollImage(const Image *image,const long x_offset,
  const long y_offset,ExceptionInfo *exception)
{
#define RollImageTag  "Roll/Image"

  Image
    *roll_image;

  long
    y;

  MagickBooleanType
    status;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes,
    *roll_indexes;

  register long
    x;

  register PixelPacket
    *q;

  RectangleInfo
    offset;

  /*
    Initialize roll image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  roll_image=CloneImage(image,image->columns,image->rows,MagickTrue,exception);
  if (roll_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Roll image.
  */
  offset.x=x_offset;
  offset.y=y_offset;
  while (offset.x < 0)
    offset.x+=image->columns;
  while (offset.x >= (long) image->columns)
    offset.x-=image->columns;
  while (offset.y < 0)
    offset.y+=image->rows;
  while (offset.y >= (long) image->rows)
    offset.y-=image->rows;
  for (y=0; y < (long) image->rows; y++)
  {
    /*
      Transfer scanline.
    */
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    if (p == (const PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      q=SetImagePixels(roll_image,(long) (offset.x+x) % image->columns,
        (long) (offset.y+y) % image->rows,1,1);
      if (q == (PixelPacket *) NULL)
        break;
      roll_indexes=GetIndexes(roll_image);
      if ((indexes != (IndexPacket *) NULL) &&
          (roll_indexes != (IndexPacket *) NULL))
        *roll_indexes=indexes[x];
      *q=(*p);
      if (SyncImagePixels(roll_image) == MagickFalse)
        break;
      p++;
    }
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(RollImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(roll_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S h a v e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ShaveImage() shaves pixels from the image edges.  It allocates the memory
%  necessary for the new Image structure and returns a pointer to the new
%  image.
%
%  The format of the ShaveImage method is:
%
%      Image *ShaveImage(const Image *image,const RectangleInfo *shave_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o shave_image: Method ShaveImage returns a pointer to the shaved
%      image.  A null image is returned if there is a memory shortage or
%      if the image width or height is zero.
%
%    o image: The image.
%
%    o shave_info: Specifies a pointer to a RectangleInfo which defines the
%      region of the image to crop.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *ShaveImage(const Image *image,
  const RectangleInfo *shave_info,ExceptionInfo *exception)
{
  Image
    *shave_image;

  RectangleInfo
    geometry;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (((2*shave_info->width) >= image->columns) ||
      ((2*shave_info->height) >= image->rows))
    ThrowImageException(OptionWarning,"GeometryDoesNotContainImage");
  SetGeometry(image,&geometry);
  geometry.width-=2*shave_info->width;
  geometry.height-=2*shave_info->height;
  geometry.x=(long) shave_info->width+image->page.x;
  geometry.y=(long) shave_info->height+image->page.y;
  shave_image=CropImage(image,&geometry,exception);
  if (shave_image == (Image *) NULL)
    return((Image *) NULL);
  shave_image->page.width-=2*shave_info->width;
  shave_image->page.height-=2*shave_info->height;
  shave_image->page.x-=shave_info->width;
  shave_image->page.y-=shave_info->height;
  return(shave_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S p l i c e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SpliceImage() splices a solid color into the image as defined by the
%  geometry.
%
%  The format of the SpliceImage method is:
%
%      Image *SpliceImage(const Image *image,const RectangleInfo *geometry,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o geometry: Define the region of the image to splice with members
%      x, y, width, and height.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *SpliceImage(const Image *image,
  const RectangleInfo *geometry,ExceptionInfo *exception)
{
#define SpliceImageTag  "Splice/Image"

  Image
    *splice_image;

  long
    y;

  MagickBooleanType
    status;

  RectangleInfo
    splice_geometry;

  register const PixelPacket
    *p;

  register IndexPacket
    *splice_indexes,
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  /*
    Allocate splice image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(geometry != (const RectangleInfo *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if ((geometry->x < 0) || (geometry->x > (long) image->columns) ||
      (geometry->y < 0) || (geometry->y > (long) image->rows))
     ThrowImageException(OptionWarning,"GeometryDoesNotContainImage");
  splice_geometry=(*geometry);
  splice_image=CloneImage(image,image->columns+splice_geometry.width,
    image->rows+splice_geometry.height,MagickTrue,exception);
  if (splice_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(splice_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&splice_image->exception);
      splice_image=DestroyImage(splice_image);
      return((Image *) NULL);
    }
  if (image->background_color.opacity != OpaqueOpacity)
    splice_image->matte=MagickTrue;
  /*
    Respect image geometry.
  */
  switch (image->gravity)
  {
    default:
    case UndefinedGravity:
    case NorthWestGravity:
      break;
    case NorthGravity:
    {
      splice_geometry.x+=splice_geometry.width/2;
      break;
    }
    case NorthEastGravity:
    {
      splice_geometry.x+=splice_geometry.width;
      break;
    }
    case WestGravity:
    {
      splice_geometry.y+=splice_geometry.width/2;
      break;
    }
    case StaticGravity:
    case CenterGravity:
    {
      splice_geometry.x+=splice_geometry.width/2;
      splice_geometry.y+=splice_geometry.height/2;
      break;
    }
    case EastGravity:
    {
      splice_geometry.x+=splice_geometry.width;
      splice_geometry.y+=splice_geometry.height/2;
      break;
    }
    case SouthWestGravity:
    {
      splice_geometry.y+=splice_geometry.height;
      break;
    }
    case SouthGravity:
    {
      splice_geometry.x+=splice_geometry.width/2;
      splice_geometry.y+=splice_geometry.height;
      break;
    }
    case SouthEastGravity:
    {
      splice_geometry.x+=splice_geometry.width;
      splice_geometry.y+=splice_geometry.height;
      break;
    }
  }
  /*
    Splice image.
  */
  i=0;
  for (y=0; y < (long) splice_geometry.y; y++)
  {
    p=AcquireImagePixels(image,0,i++,image->columns,1,exception);
    q=SetImagePixels(splice_image,0,y,splice_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    splice_indexes=GetIndexes(splice_image);
    for (x=0; x < splice_geometry.x; x++)
    {
      *q++=(*p++);
      if (splice_image->colorspace == CMYKColorspace)
        splice_indexes[x]=(*indexes++);
    }
    for ( ; x < (long) (splice_geometry.x+splice_geometry.width); x++)
    {
      *q++=image->background_color;
      if (splice_image->colorspace == CMYKColorspace)
        splice_indexes[x]=(IndexPacket) 0;
    }
    for ( ; x < (long) splice_image->columns; x++)
    {
      *q++=(*p++);
      if (splice_image->colorspace == CMYKColorspace)
        splice_indexes[x]=(*indexes++);
    }
    if (SyncImagePixels(splice_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,splice_image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SpliceImageTag,y,splice_image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  for ( ; y < (long) (splice_geometry.y+splice_geometry.height); y++)
  {
    q=SetImagePixels(splice_image,0,y,splice_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    splice_indexes=GetIndexes(splice_image);
    for (x=0; x < (long) splice_image->columns; x++)
    {
      *q++=image->background_color;
      if (splice_image->colorspace == CMYKColorspace)
        splice_indexes[x]=(IndexPacket) 0;
    }
    if (SyncImagePixels(splice_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,splice_image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SpliceImageTag,y,splice_image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  for ( ; y < (long) splice_image->rows; y++)
  {
    p=AcquireImagePixels(image,0,i++,image->columns,1,exception);
    q=SetImagePixels(splice_image,0,y,splice_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    splice_indexes=GetIndexes(splice_image);
    for (x=0; x < splice_geometry.x; x++)
    {
      *q++=(*p++);
      if (splice_image->colorspace == CMYKColorspace)
        splice_indexes[x]=(*indexes++);
    }
    for ( ; x < (long) (splice_geometry.x+splice_geometry.width); x++)
    {
      *q++=image->background_color;
      if (splice_image->colorspace == CMYKColorspace)
        splice_indexes[x]=(IndexPacket) 0;
    }
    for ( ; x < (long) splice_image->columns; x++)
    {
      *q++=(*p++);
      if (splice_image->colorspace == CMYKColorspace)
        splice_indexes[x]=(*indexes++);
    }
    if (SyncImagePixels(splice_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,splice_image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SpliceImageTag,y,splice_image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(splice_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r a n s f o r m I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TransformImage() is a convenience method that behaves like ResizeImage() or
%  CropImage() but accepts scaling and/or cropping information as a region
%  geometry specification.  If the operation fails, the original image handle
%  is returned.
%
%  The format of the TransformImage method is:
%
%      MagickBooleanType TransformImage(Image **image,const char *crop_geometry,
%        const char *image_geometry)
%
%  A description of each parameter follows:
%
%    o image: The image The transformed image is returned as this parameter.
%
%    o crop_geometry: A crop geometry string.  This geometry defines a
%      subregion of the image to crop.
%
%    o image_geometry: An image geometry string.  This geometry defines the
%      final size of the image.
%
*/
MagickExport MagickBooleanType TransformImage(Image **image,
  const char *crop_geometry,const char *image_geometry)
{
  Image
    *resize_image,
    *transform_image;

  MagickStatusType
    flags;

  RectangleInfo
    geometry;

  assert(image != (Image **) NULL);
  assert((*image)->signature == MagickSignature);
  if ((*image)->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),(*image)->filename);
  transform_image=(*image);
  if (crop_geometry != (const char *) NULL)
    {
      Image
        *crop_image;

      RectangleInfo
        geometry;

      /*
        Crop image to a user specified size.
      */
      crop_image=NewImageList();
      flags=ParseGravityGeometry(transform_image,crop_geometry,&geometry);
      if (((geometry.width == 0) && (geometry.height == 0)) ||
          ((flags & XValue) != 0) || ((flags & YValue) != 0))
        {
          crop_image=CropImage(transform_image,&geometry,&(*image)->exception);
          if ((crop_image != (Image *) NULL) && ((flags & AspectValue) != 0))
            {
              crop_image->page.width=geometry.width;
              crop_image->page.height=geometry.height;
              crop_image->page.x-=geometry.x;
              crop_image->page.y-=geometry.y;
            }
        }
      else
        if ((transform_image->columns > geometry.width) ||
            (transform_image->rows > geometry.height))
          {
            Image
              *next;

            long
              y;

            register long
              x;

            unsigned long
              height,
              width;

            /*
              Crop repeatedly to create uniform scenes.
            */
            if (transform_image->page.width == 0)
              transform_image->page.width=transform_image->columns;
            if (transform_image->page.height == 0)
              transform_image->page.height=transform_image->rows;
            width=geometry.width;
            if (width == 0)
              width=transform_image->page.width;
            height=geometry.height;
            if (height == 0)
              height=transform_image->page.height;
            next=NewImageList();
            for (y=0; y < (long) transform_image->page.height; y+=height)
            {
              for (x=0; x < (long) transform_image->page.width; x+=width)
              {
                geometry.width=width;
                geometry.height=height;
                geometry.x=x;
                geometry.y=y;
                next=CropImage(transform_image,&geometry,&(*image)->exception);
                if (next == (Image *) NULL)
                  break;
                AppendImageToList(&crop_image,next);
              }
              if (next == (Image *) NULL)
                break;
            }
          }
      if (crop_image != (Image *) NULL)
        {
          transform_image=DestroyImage(transform_image);
          transform_image=GetFirstImageInList(crop_image);
        }
      *image=transform_image;
    }
  if (image_geometry == (const char *) NULL)
    return(MagickTrue);
  /*
    Scale image to a user specified size.
  */
  flags=ParseSizeGeometry(transform_image,image_geometry,&geometry);
  if ((transform_image->columns == geometry.width) &&
      (transform_image->rows == geometry.height))
    return(MagickTrue);
  resize_image=ZoomImage(transform_image,geometry.width,geometry.height,
    &(*image)->exception);
  if (resize_image == (Image *) NULL)
    return(MagickFalse);
  transform_image=DestroyImage(transform_image);
  transform_image=resize_image;
  *image=transform_image;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r a n s f o r m I m a g e s                                             %
%                                                                             %
%                                                                             %
%                                                                             % %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TransformImages() calls TransformImage() on each image of a sequence.
%
%  The format of the TransformImage method is:
%
%      MagickBooleanType TransformImages(Image **image,
%        const char *crop_geometry,const char *image_geometry)
%
%  A description of each parameter follows:
%
%    o image: The image The transformed image is returned as this parameter.
%
%    o crop_geometry: A crop geometry string.  This geometry defines a
%      subregion of the image to crop.
%
%    o image_geometry: An image geometry string.  This geometry defines the
%      final size of the image.
%
*/
MagickExport MagickBooleanType TransformImages(Image **images,
  const char *crop_geometry,const char *image_geometry)
{
  Image
    *image,
    **image_list,
    *transform_images;

  MagickStatusType
    status;

  register long
    i;

  assert(images != (Image **) NULL);
  assert((*images)->signature == MagickSignature);
  if ((*images)->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),(*images)->filename);
  image_list=ImageListToArray(*images,&(*images)->exception);
  if (image_list == (Image **) NULL)
    return(MagickFalse);
  status=MagickTrue;
  transform_images=NewImageList();
  for (i=0; image_list[i] != (Image *) NULL; i++)
  {
    image=image_list[i];
    status|=TransformImage(&image,crop_geometry,image_geometry);
    AppendImageToList(&transform_images,image);
  }
  *images=transform_images;
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r a n s p o s e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TransposeImage() creates a horizontal mirror image by reflecting the pixels
%  around the central y-axis while rotating them by 90 degrees.
%
%  The format of the TransposeImage method is:
%
%      Image *TransposeImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *TransposeImage(const Image *image,ExceptionInfo *exception)
{
#define TransposeImageTag  "Transpose/Image"

  Image
    *transpose_image;

  long
    y;

  MagickBooleanType
    status;

  RectangleInfo
    page;

  register const PixelPacket
    *p;

  register IndexPacket
    *transpose_indexes,
    *indexes;

  register PixelPacket
    *q;

  /*
    Initialize transpose image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  transpose_image=CloneImage(image,image->rows,image->columns,MagickTrue,
    exception);
  if (transpose_image == (Image *) NULL)
    return((Image *) NULL);
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,(long) image->rows-y-1,image->columns,1,
      exception);
    q=SetImagePixels(transpose_image,(long) (image->rows-y-1),0,1,
      transpose_image->rows);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    (void) CopyMagickMemory(q,p,(size_t) image->columns*sizeof(*q));
    indexes=GetIndexes(image);
    transpose_indexes=GetIndexes(transpose_image);
    if ((indexes != (IndexPacket *) NULL) &&
        (transpose_indexes != (IndexPacket *) NULL))
      (void) CopyMagickMemory(transpose_indexes,indexes,(size_t)
        image->columns*sizeof(*transpose_indexes));
    if (SyncImagePixels(transpose_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(TransposeImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  page=transpose_image->page;
  Swap(page.width,page.height);
  Swap(page.x,page.y);
  if (page.width != 0)
    page.x=(long) (page.width-transpose_image->columns-page.x);
  transpose_image->page=page;
  return(transpose_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r a n s v e r s e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TransverseImage() creates a vertical mirror image by reflecting the pixels
%  around the central x-axis while rotating them by 270 degrees.
%
%  The format of the TransverseImage method is:
%
%      Image *TransverseImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *TransverseImage(const Image *image,ExceptionInfo *exception)
{
#define TransverseImageTag  "Transverse/Image"

  Image
    *transverse_image;

  long
    y;

  MagickBooleanType
    status;

  RectangleInfo
    page;

  register const PixelPacket
    *p;

  register IndexPacket
    *transverse_indexes,
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Initialize transverse image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  transverse_image=CloneImage(image,image->rows,image->columns,MagickTrue,
    exception);
  if (transverse_image == (Image *) NULL)
    return((Image *) NULL);
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=SetImagePixels(transverse_image,(long) (image->rows-y-1),0,1,
      transverse_image->rows);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    q+=image->columns;
    for (x=0; x < (long) image->columns; x++)
      *--q=(*p++);
    indexes=GetIndexes(image);
    transverse_indexes=GetIndexes(transverse_image);
    if ((indexes != (IndexPacket *) NULL) &&
        (transverse_indexes != (IndexPacket *) NULL))
      for (x=0; x < (long) image->columns; x++)
        transverse_indexes[image->columns-x-1]=indexes[x];
    if (SyncImagePixels(transverse_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(TransposeImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  page=transverse_image->page;
  Swap(page.width,page.height);
  Swap(page.x,page.y);
  if (page.height != 0)
    page.y=(long) (page.height-transverse_image->rows-page.y);
  transverse_image->page=page;
  return(transverse_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r i m I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TrimImage() trims pixels from the image edges.  It allocates the memory
%  necessary for the new Image structure and returns a pointer to the new
%  image.
%
%  The format of the TrimImage method is:
%
%      Image *TrimImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *TrimImage(const Image *image,ExceptionInfo *exception)
{
  RectangleInfo
    geometry;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  geometry=GetImageBoundingBox(image,exception);
  if ((geometry.width == 0) || (geometry.height == 0))
    {
      Image
        *crop_image;

      crop_image=CloneImage(image,1,1,MagickTrue,exception);
      if (crop_image == (Image *) NULL)
        return((Image *) NULL);
      crop_image->background_color.opacity=(Quantum) TransparentOpacity;
      (void) SetImageBackgroundColor(crop_image);
      crop_image->page=image->page;
      crop_image->page.x=(-1);
      crop_image->page.y=(-1);
      return(crop_image);
    }
  geometry.x+=image->page.x;
  geometry.y+=image->page.y;
  return(CropImage(image,&geometry,exception));
}
