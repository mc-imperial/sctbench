/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      SSSSS  H   H  EEEEE   AAA    RRRR                      %
%                      SS     H   H  E      A   A   R   R                     %
%                       SSS   HHHHH  EEE    AAAAA   RRRR                      %
%                         SS  H   H  E      A   A   R R                       %
%                      SSSSS  H   H  EEEEE  A   A   R  R                      %
%                                                                             %
%                                                                             %
%            Methods to Shear or Rotate an Image by an Arbitrary Angle        %
%                                                                             %
%                               Software Design                               %
%                                 John Cristy                                 %
%                                  July 1992                                  %
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
%  The RotateImage, XShearImage, and YShearImage methods are based on the
%  paper "A Fast Algorithm for General Raster Rotatation" by Alan W. Paeth,
%  Graphics Interface '86 (Vancouver).  RotateImage is adapted from a similar
%  method based on the Paeth paper written by Michael Halle of the Spatial
%  Imaging Group, MIT Media Lab.
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/colorspace-private.h"
#include "magick/composite.h"
#include "magick/composite-private.h"
#include "magick/decorate.h"
#include "magick/draw.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/gem.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/memory_.h"
#include "magick/list.h"
#include "magick/monitor.h"
#include "magick/pixel-private.h"
#include "magick/quantum.h"
#include "magick/shear.h"
#include "magick/transform.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A f f i n e T r a n s f o r m I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AffineTransformImage() transforms an image as dictated by the affine matrix.
%  It allocates the memory necessary for the new Image structure and returns
%  a pointer to the new image.
%
%  The format of the AffineTransformImage method is:
%
%      Image *AffineTransformImage(const Image *image,
%        AffineMatrix *affine_matrix,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o affine_matrix: The affine matrix.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *AffineTransformImage(const Image *image,
  const AffineMatrix *affine_matrix,ExceptionInfo *exception)
{
  AffineMatrix
    transform;

  Image
    *affine_image;

  PointInfo
    extent[4],
    min,
    max,
    point;

  register long
    i;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(affine_matrix != (AffineMatrix *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  /*
    Determine destination image bounding box.
  */
  extent[0].x=(double) image->page.x;
  extent[0].y=(double) image->page.y;
  extent[1].x=(double) (image->page.x+image->columns);
  extent[1].y=(double) image->page.y;
  extent[2].x=(double) (image->page.x+image->columns);
  extent[2].y=(double) (image->page.y+image->rows);
  extent[3].x=(double) image->page.x;
  extent[3].y=(double) (image->page.y+image->rows);
  for (i=0; i < 4; i++)
  {
    point=extent[i];
    extent[i].x=(double) (point.x*affine_matrix->sx+point.y*affine_matrix->ry+
      affine_matrix->tx);
    extent[i].y=(double) (point.x*affine_matrix->rx+point.y*affine_matrix->sy+
      affine_matrix->ty);
  }
  min=extent[0];
  max=extent[0];
  for (i=1; i < 4; i++)
  {
    if (min.x > extent[i].x)
      min.x=extent[i].x;
    if (min.y > extent[i].y)
      min.y=extent[i].y;
    if (max.x < extent[i].x)
      max.x=extent[i].x;
    if (max.y < extent[i].y)
      max.y=extent[i].y;
  }
  affine_image=CloneImage(image,(unsigned long) (max.x-min.x+0.5),
    (unsigned long) (max.y-min.y+0.5),MagickTrue,exception);
  if (affine_image == (Image *) NULL)
    return((Image *) NULL);
  affine_image->background_color.opacity=(Quantum) TransparentOpacity;
  (void) SetImageBackgroundColor(affine_image);
  /*
    Adjust transform translation for direct image to image transformation.
    That is, pixel 0,0 translates correctly to its location in destination.
  */
  transform=(*affine_matrix);
  transform.tx=extent[0].x-min.x;
  transform.ty=extent[0].y-min.y;
  /*
    Affine transform image.
  */
  (void) DrawAffineImage(affine_image,image,&transform);
  /*
    Add offset to resulting image.
  */
  affine_image->page.x=(long) floor(min.x+0.5);
  affine_image->page.y=(long) floor(min.y+0.5);
  /*
    Roughly calculate an appropriate virtual canvas (page) size.
  */
  affine_image->page.width=affine_image->columns;
  if (affine_image->page.x > 0)
    affine_image->page.width+=affine_image->page.x;
  affine_image->page.height=affine_image->rows;
  if (affine_image->page.y > 0)
    affine_image->page.height+=affine_image->page.y;
  return(affine_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C r o p T o F i t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CropToFitImage() crops the sheared image as determined by the bounding box
%  as defined by width and height and shearing angles.
%
%  The format of the CropToFitImage method is:
%
%      Image *CropToFitImage(Image **image,const MagickRealType x_shear,
%        const MagickRealType x_shear,const MagickRealType width,
%        const MagickRealType height,const MagickBooleanType rotate,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o x_shear, y_shear, width, height: Defines a region of the image to crop.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static inline void CropToFitImage(Image **image,const MagickRealType x_shear,
  const MagickRealType y_shear,const MagickRealType width,
  const MagickRealType height,const MagickBooleanType rotate,
  ExceptionInfo *exception)
{
  Image
    *crop_image;

  PointInfo
    extent[4],
    min,
    max;

  RectangleInfo
    geometry,
    page;

  register long
    i;

  /*
    Calculate the rotated image size.
  */
  extent[0].x=(double) (-width/2.0);
  extent[0].y=(double) (-height/2.0);
  extent[1].x=(double) width/2.0;
  extent[1].y=(double) (-height/2.0);
  extent[2].x=(double) (-width/2.0);
  extent[2].y=(double) height/2.0;
  extent[3].x=(double) width/2.0;
  extent[3].y=(double) height/2.0;
  for (i=0; i < 4; i++)
  {
    extent[i].x+=x_shear*extent[i].y;
    extent[i].y+=y_shear*extent[i].x;
    if (rotate != MagickFalse)
      extent[i].x+=x_shear*extent[i].y;
    extent[i].x+=(double) (*image)->columns/2.0;
    extent[i].y+=(double) (*image)->rows/2.0;
  }
  min=extent[0];
  max=extent[0];
  for (i=1; i < 4; i++)
  {
    if (min.x > extent[i].x)
      min.x=extent[i].x;
    if (min.y > extent[i].y)
      min.y=extent[i].y;
    if (max.x < extent[i].x)
      max.x=extent[i].x;
    if (max.y < extent[i].y)
      max.y=extent[i].y;
  }
  geometry.x=(long) (min.x+0.5);
  geometry.y=(long) (min.y+0.5);
  geometry.width=(unsigned long) ((long) (max.x+0.5)-(long) (min.x+0.5));
  geometry.height=(unsigned long) ((long) (max.y+0.5)-(long) (min.y+0.5));
  page=(*image)->page;
  (void) ParseAbsoluteGeometry("0x0+0+0",&(*image)->page);
  crop_image=CropImage(*image,&geometry,exception);
  (*image)->page=page;
  if (crop_image != (Image *) NULL)
    {
      crop_image->page=page;
      *image=DestroyImage(*image);
      *image=crop_image;
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n t e g r a l R o t a t e I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IntegralRotateImage()  rotates the image an integral of 90 degrees.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the rotated image.
%
%  The format of the IntegralRotateImage method is:
%
%      Image *IntegralRotateImage(const Image *image,unsigned long rotations,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o rotations: Specifies the number of 90 degree rotations.
%
%
*/
static Image *IntegralRotateImage(const Image *image,unsigned long rotations,
  ExceptionInfo *exception)
{
#define TileHeight  128
#define TileWidth  128
#define RotateImageTag  "Rotate/Image"

  Image
    *rotate_image;

  long
    tile_x,
    tile_y,
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  RectangleInfo
    page;

  register IndexPacket
    *indexes,
    *rotate_indexes;

  register const PixelPacket
    *p,
    *tile_pixels;

  register long
    x;

  register PixelPacket
    *q;

  unsigned long
    tile_width,
    tile_height;

  /*
    Initialize rotated image attributes.
  */
  assert(image != (Image *) NULL);
  page=image->page;
  rotations%=4;
  if ((rotations == 1) || (rotations == 3))
    rotate_image=CloneImage(image,image->rows,image->columns,MagickTrue,
      exception);
  else
    rotate_image=CloneImage(image,image->columns,image->rows,MagickTrue,
      exception);
  if (rotate_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Integral rotate the image.
  */
  GetMagickPixelPacket(image,&pixel);
  switch (rotations)
  {
    case 0:
    {
      /*
        Rotate 0 degrees.
      */
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,exception);
        q=SetImagePixels(rotate_image,0,y,rotate_image->columns,1);
        if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
          break;
        indexes=GetIndexes(image);
        rotate_indexes=GetIndexes(rotate_image);
        for (x=0; x < (long) image->columns; x++)
        {
          SetMagickPixelPacket(image,p,indexes+x,&pixel);
          SetPixelPacket(rotate_image,&pixel,q,rotate_indexes+x);
          p++;
          q++;
        }
        if (SyncImagePixels(rotate_image) == MagickFalse)
          break;
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(y,image->rows) != MagickFalse))
          {
            status=image->progress_monitor(RotateImageTag,y,image->rows,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
      break;
    }
    case 1:
    {
      /*
        Rotate 90 degrees.
      */
      for (tile_y=0; tile_y < (long) image->rows; tile_y+=TileHeight)
      {
        for (tile_x=0; tile_x < (long) image->columns; tile_x+=TileWidth)
        {
          tile_width=TileWidth;
          if ((tile_x+TileWidth) > (long) image->columns)
            tile_width=1UL*(TileWidth-(tile_x+TileWidth-image->columns));
          tile_height=TileHeight;
          if ((tile_y+TileHeight) > (long) image->rows)
            tile_height=1UL*(TileHeight-(tile_y+TileHeight-image->rows));
          tile_pixels=AcquireImagePixels(image,tile_x,tile_y,tile_width,
            tile_height,exception);
          if (tile_pixels == (const PixelPacket *) NULL)
            break;
          for (y=0; y < (long) tile_width; y++)
          {
            q=SetImagePixels(rotate_image,(long) rotate_image->columns-(tile_y+
              tile_height),tile_x+y,tile_height,1);
            if (q == (PixelPacket *) NULL)
              break;
            rotate_indexes=GetIndexes(rotate_image);
            p=tile_pixels+(tile_height-1)*tile_width+y;
            indexes=GetIndexes(image)+(tile_height-1)*tile_width+y;
            for (x=0; x < (long) tile_height; x++)
            {
              SetMagickPixelPacket(image,p,indexes,&pixel);
              SetPixelPacket(rotate_image,&pixel,q,rotate_indexes+x);
              p-=tile_width;
              indexes-=tile_width;
              q++;
            }
            if (SyncImagePixels(rotate_image) == MagickFalse)
              break;
          }
        }
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(tile_y,image->rows) != MagickFalse))
          {
            status=image->progress_monitor(RotateImageTag,tile_y,image->rows,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
      Swap(page.width,page.height);
      Swap(page.x,page.y);
      if (page.width != 0)
        page.x=(long) (page.width-rotate_image->columns-page.x);
      break;
    }
    case 2:
    {
      /*
        Rotate 180 degrees.
      */
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,exception);
        q=SetImagePixels(rotate_image,0,(long) (image->rows-y-1),
          image->columns,1);
        if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
          break;
        q+=image->columns;
        indexes=GetIndexes(image);
        rotate_indexes=GetIndexes(rotate_image);
        for (x=0; x < (long) image->columns; x++)
        {
          q--;
          SetMagickPixelPacket(image,p,indexes+x,&pixel);
          SetPixelPacket(rotate_image,&pixel,q,rotate_indexes+(image->columns-
            x-1));
          p++;
        }
        if (SyncImagePixels(rotate_image) == MagickFalse)
          break;
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(y,image->rows) != MagickFalse))
          {
            status=image->progress_monitor(RotateImageTag,y,image->rows,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
      if (page.width != 0)
        page.x=(long) (page.width-rotate_image->columns-page.x);
      if (page.height != 0)
        page.y=(long) (page.height-rotate_image->rows-page.y);
      break;
    }
    case 3:
    {
      /*
        Rotate 270 degrees.
      */
      for (tile_y=0; tile_y < (long) image->rows; tile_y+=TileHeight)
      {
        for (tile_x=0; tile_x < (long) image->columns; tile_x+=TileWidth)
        {
          tile_width=TileWidth;
          if ((tile_x+TileWidth) > (long) image->columns)
            tile_width=1UL*(TileWidth-(tile_x+TileWidth-image->columns));
          tile_height=TileHeight;
          if ((tile_y+TileHeight) > (long) image->rows)
            tile_height=1UL*(TileHeight-(tile_y+TileHeight-image->rows));
          tile_pixels=AcquireImagePixels(image,tile_x,tile_y,tile_width,
            tile_height,exception);
          if (tile_pixels == (const PixelPacket *) NULL)
            break;
          for (y=0; y < (long) tile_width; y++)
          {
            q=SetImagePixels(rotate_image,tile_y,(long) rotate_image->rows-
              (tile_x+tile_width)+y,tile_height,1);
            if (q == (PixelPacket *) NULL)
              break;
            rotate_indexes=GetIndexes(rotate_image);
            p=tile_pixels+(tile_width-1)-y;
            indexes=GetIndexes(image)+(tile_width-1)-y;
            for (x=0; x < (long) tile_height; x++)
            {
              SetMagickPixelPacket(image,p,indexes,&pixel);
              SetPixelPacket(rotate_image,&pixel,q,rotate_indexes+x);
              p+=tile_width;
              indexes+=tile_width;
              q++;
            }
            if (SyncImagePixels(rotate_image) == MagickFalse)
              break;
          }
        }
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(tile_y,image->rows) != MagickFalse))
          {
            status=image->progress_monitor(RotateImageTag,tile_y,image->rows,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
      Swap(page.width,page.height);
      Swap(page.x,page.y);
      if (page.height != 0)
        page.y=(long) (page.height-rotate_image->rows-page.y);
      break;
    }
  }
  rotate_image->page=page;
  return(rotate_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X S h e a r I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XShearImage() shears the image in the X direction with a shear angle of
%  'degrees'.  Positive angles shear counter-clockwise (right-hand rule), and
%  negative angles shear clockwise.  Angles are measured relative to a vertical
%  Y-axis.  X shears will widen an image creating 'empty' triangles on the left
%  and right sides of the source image.
%
%  The format of the XShearImage method is:
%
%      void XShearImage(Image *image,const MagickRealType degrees,
%        const unsigned long width,const unsigned long height,
%        const long x_offset,long y_offset)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o degrees: A MagickRealType representing the shearing angle along the X
%      axis.
%
%    o width, height, x_offset, y_offset: Defines a region of the image
%      to shear.
%
*/

static inline MagickRealType Blend_(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  return((1.0-QuantumScale*alpha)*p+(1.0-QuantumScale*beta)*q);
}

static inline void MagickCompositeBlend(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,const MagickRealType area,
  MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  if ((alpha == TransparentOpacity) && (beta == TransparentOpacity))
    {
      *composite=(*p);
      return;
    }
  gamma=RoundToUnity((1.0-QuantumScale*(QuantumRange-(1.0-area)*
    (QuantumRange-alpha)))+(1.0-QuantumScale*(QuantumRange-area*
    (QuantumRange-beta))));
  composite->opacity=(MagickRealType) QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Blend_((MagickRealType) p->red,
    (MagickRealType) QuantumRange-(1.0-area)*(QuantumRange-alpha),
    (MagickRealType) q->red,(MagickRealType) QuantumRange-area*(QuantumRange-
    beta));
  composite->green=gamma*Blend_((MagickRealType) p->green,
    (MagickRealType) QuantumRange-(1.0-area)*(QuantumRange-alpha),
    (MagickRealType) q->green,(MagickRealType) QuantumRange-area*(QuantumRange-
    beta));
  composite->blue=gamma*Blend_((MagickRealType) p->blue,
    (MagickRealType) QuantumRange-(1.0-area)*(QuantumRange-alpha),
    (MagickRealType) q->blue,(MagickRealType) QuantumRange-area*(QuantumRange-
    beta));
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Blend_((MagickRealType) p->index,
      (MagickRealType) QuantumRange-(1.0-area)*(QuantumRange-alpha),
      (MagickRealType) q->index,(MagickRealType) QuantumRange-area*
      (QuantumRange-beta));
}

static void XShearImage(Image *image,const MagickRealType degrees,
  const unsigned long width,const unsigned long height,const long x_offset,
  long y_offset)
{
#define XShearImageTag  "XShear/Image"

  enum {LEFT, RIGHT}
    direction;

  IndexPacket
    *indexes,
    *shear_indexes;

  long
    step,
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    background,
    pixel,
    source,
    destination;

  MagickRealType
    area,
    displacement;

  register long
    i;

  register PixelPacket
    *p,
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  y_offset--;
  for (y=0; y < (long) height; y++)
  {
    y_offset++;
    displacement=degrees*(MagickRealType) (y-height/2.0);
    if (displacement == 0.0)
      continue;
    if (displacement > 0.0)
      direction=RIGHT;
    else
      {
        displacement*=(-1.0);
        direction=LEFT;
      }
    step=(long) floor((double) displacement);
    area=(MagickRealType) (displacement-step);
    step++;
    GetMagickPixelPacket(image,&background);
    SetMagickPixelPacket(image,&image->background_color,(IndexPacket *) NULL,
      &background);
    if (image->colorspace == CMYKColorspace)
      ConvertRGBToCMYK(&background);
    pixel=background;
    GetMagickPixelPacket(image,&source);
    GetMagickPixelPacket(image,&destination);
    switch (direction)
    {
      case LEFT:
      {
        /*
          Transfer pixels left-to-right.
        */
        if (step > x_offset)
          break;
        p=GetImagePixels(image,0,y_offset,image->columns,1);
        if (p == (PixelPacket *) NULL)
          break;
        p+=x_offset;
        indexes=GetIndexes(image);
        indexes+=x_offset;
        q=p-step;
        shear_indexes=indexes-step;
        for (i=0; i < (long) width; i++)
        {
          if ((x_offset+i) < step)
            {
              SetMagickPixelPacket(image,++p,++indexes,&pixel);
              q++;
              shear_indexes++;
              continue;
            }
          SetMagickPixelPacket(image,p,indexes,&source);
          MagickCompositeBlend(&pixel,(MagickRealType) pixel.opacity,&source,
            (MagickRealType) p->opacity,area,&destination);
          SetPixelPacket(image,&destination,q++,shear_indexes++);
          SetMagickPixelPacket(image,p++,indexes++,&pixel);
        }
        MagickCompositeBlend(&pixel,(MagickRealType) pixel.opacity,&background,
          (MagickRealType) background.opacity,area,&destination);
        SetPixelPacket(image,&destination,q++,shear_indexes++);
        for (i=0; i < (step-1); i++)
          SetPixelPacket(image,&background,q++,shear_indexes++);
        break;
      }
      case RIGHT:
      {
        /*
          Transfer pixels right-to-left.
        */
        p=GetImagePixels(image,0,y_offset,image->columns,1);
        if (p == (PixelPacket *) NULL)
          break;
        p+=x_offset+width;
        indexes=GetIndexes(image);
        indexes+=x_offset+width;
        q=p+step;
        shear_indexes=indexes+step;
        for (i=0; i < (long) width; i++)
        {
          p--;
          indexes--;
          q--;
          shear_indexes--;
          if ((unsigned long) (x_offset+width+step-i) >= image->columns)
            continue;
          SetMagickPixelPacket(image,p,indexes,&source);
          MagickCompositeBlend(&pixel,(MagickRealType) pixel.opacity,&source,
            (MagickRealType) p->opacity,area,&destination);
          SetPixelPacket(image,&destination,q,shear_indexes);
          SetMagickPixelPacket(image,p,indexes,&pixel);
        }
        MagickCompositeBlend(&pixel,(MagickRealType) pixel.opacity,&background,
          (MagickRealType) background.opacity,area,&destination);
        SetPixelPacket(image,&destination,--q,--shear_indexes);
        for (i=0; i < (step-1); i++)
          SetPixelPacket(image,&background,--q,--shear_indexes);
        break;
      }
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,height) != MagickFalse))
      {
        status=image->progress_monitor(XShearImageTag,y,height,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   Y S h e a r I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  YShearImage shears the image in the Y direction with a shear angle of
%  'degrees'.  Positive angles shear counter-clockwise (right-hand rule), and
%  negative angles shear clockwise.  Angles are measured relative to a
%  horizontal X-axis.  Y shears will increase the height of an image creating
%  'empty' triangles on the top and bottom of the source image.
%
%  The format of the YShearImage method is:
%
%      void YShearImage(Image *image,const MagickRealType degrees,
%        const unsigned long width,const unsigned long height,long x_offset,
%        const long y_offset)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o degrees: A MagickRealType representing the shearing angle along the Y
%      axis.
%
%    o width, height, x_offset, y_offset: Defines a region of the image
%      to shear.
%
*/
static void YShearImage(Image *image,const MagickRealType degrees,
  const unsigned long width,const unsigned long height,long x_offset,
  const long y_offset)
{
#define YShearImageTag  "YShear/Image"

  enum {UP, DOWN}
    direction;

  IndexPacket
    *indexes,
    *shear_indexes;

  long
    step,
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    background,
    pixel,
    source,
    destination;

  MagickRealType
    area,
    displacement;

  register PixelPacket
    *p,
    *q;

  register long
    i;

  assert(image != (Image *) NULL);
  x_offset--;
  for (y=0; y < (long) width; y++)
  {
    x_offset++;
    displacement=degrees*(MagickRealType) (y-width/2.0);
    if (displacement == 0.0)
      continue;
    if (displacement > 0.0)
      direction=DOWN;
    else
      {
        displacement*=(-1.0);
        direction=UP;
      }
    step=(long) floor((double) displacement);
    area=(MagickRealType) (displacement-step);
    step++;
    GetMagickPixelPacket(image,&background);
    SetMagickPixelPacket(image,&image->background_color,(IndexPacket *) NULL,
      &background);
    if (image->colorspace == CMYKColorspace)
      ConvertRGBToCMYK(&background);
    pixel=background;
    GetMagickPixelPacket(image,&source);
    GetMagickPixelPacket(image,&destination);
    switch (direction)
    {
      case UP:
      {
        /*
          Transfer pixels top-to-bottom.
        */
        if (step > y_offset)
          break;
        p=GetImagePixels(image,x_offset,0,1,image->rows);
        if (p == (PixelPacket *) NULL)
          break;
        p+=y_offset;
        indexes=GetIndexes(image);
        indexes+=y_offset;
        q=p-step;
        shear_indexes=indexes-step;
        for (i=0; i < (long) height; i++)
        {
          if ((y_offset+i) < step)
            {
              SetMagickPixelPacket(image,++p,++indexes,&pixel);
              q++;
              shear_indexes++;
              continue;
            }
          SetMagickPixelPacket(image,p,indexes,&source);
          MagickCompositeBlend(&pixel,(MagickRealType) pixel.opacity,&source,
            (MagickRealType) p->opacity,area,&destination);
          SetPixelPacket(image,&destination,q++,shear_indexes++);
          SetMagickPixelPacket(image,p++,indexes++,&pixel);
        }
        MagickCompositeBlend(&pixel,(MagickRealType) pixel.opacity,&background,
          (MagickRealType) background.opacity,area,&destination);
        SetPixelPacket(image,&destination,q++,shear_indexes++);
        for (i=0; i < (step-1); i++)
          SetPixelPacket(image,&background,q++,shear_indexes++);
        break;
      }
      case DOWN:
      {
        /*
          Transfer pixels bottom-to-top.
        */
        p=GetImagePixels(image,x_offset,0,1,image->rows);
        if (p == (PixelPacket *) NULL)
          break;
        p+=y_offset+height;
        indexes=GetIndexes(image);
        indexes+=y_offset+height;
        q=p+step;
        shear_indexes=indexes+step;
        for (i=0; i < (long) height; i++)
        {
          p--;
          indexes--;
          q--;
          shear_indexes--;
          if ((unsigned long) (y_offset+height+step-i) >= image->rows)
            continue;
          SetMagickPixelPacket(image,p,indexes,&source);
          MagickCompositeBlend(&pixel,(MagickRealType) pixel.opacity,&source,
            (MagickRealType) p->opacity,area,&destination);
          SetPixelPacket(image,&destination,q,shear_indexes);
          SetMagickPixelPacket(image,p,indexes,&pixel);
        }
        MagickCompositeBlend(&pixel,(MagickRealType) pixel.opacity,&background,
          (MagickRealType) background.opacity,area,&destination);
        SetPixelPacket(image,&destination,--q,--shear_indexes);
        for (i=0; i < (step-1); i++)
          SetPixelPacket(image,&background,--q,--shear_indexes);
        break;
      }
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,width) != MagickFalse))
      {
        status=image->progress_monitor(XShearImageTag,y,width,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R o t a t e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RotateImage() creates a new image that is a rotated copy of an existing
%  one.  Positive angles rotate counter-clockwise (right-hand rule), while
%  negative angles rotate clockwise.  Rotated images are usually larger than
%  the originals and have 'empty' triangular corners.  X axis.  Empty
%  triangles left over from shearing the image are filled with the background
%  color defined by member 'background_color' of the image.  RotateImage
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  RotateImage() is based on the paper "A Fast Algorithm for General
%  Raster Rotatation" by Alan W. Paeth.  RotateImage is adapted from a similar
%  method based on the Paeth paper written by Michael Halle of the Spatial
%  Imaging Group, MIT Media Lab.
%
%  The format of the RotateImage method is:
%
%      Image *RotateImage(const Image *image,const double degrees,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o degrees: Specifies the number of degrees to rotate the image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *RotateImage(const Image *image,const double degrees,
  ExceptionInfo *exception)
{
  Image
    *integral_image,
    *rotate_image;

  long
    x_offset,
    y_offset;

  MagickRealType
    angle;

  PointInfo
    shear;

  RectangleInfo
    border_info;

  unsigned long
    height,
    rotations,
    width,
    y_width;

  /*
    Adjust rotation angle.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  angle=degrees;
  while (angle < -45.0)
    angle+=360.0;
  for (rotations=0; angle > 45.0; rotations++)
    angle-=90.0;
  rotations%=4;
  /*
    Calculate shear equations.
  */
  integral_image=IntegralRotateImage(image,rotations,exception);
  if (integral_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  shear.x=(-tan((double) DegreesToRadians(angle)/2.0));
  shear.y=sin((double) DegreesToRadians(angle));
  if ((shear.x == 0.0) && (shear.y == 0.0))
    return(integral_image);
  if (SetImageStorageClass(integral_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&integral_image->exception);
      integral_image=DestroyImage(integral_image);
      return(integral_image);
    }
  if (integral_image->matte == MagickFalse)
    (void) SetImageOpacity(integral_image,OpaqueOpacity);
  /*
    Compute image size.
  */
  width=image->columns;
  height=image->rows;
  if ((rotations == 1) || (rotations == 3))
    {
      width=image->rows;
      height=image->columns;
    }
  y_width=width+(long) (fabs(shear.x)*height+0.5);
  x_offset=(long) (width+((fabs(shear.y)*height+0.5)-width)/2.0+0.5);
  y_offset=(long) (height+((fabs(shear.y)*y_width+0.5)-height)/2.0+0.5);
  /*
    Surround image with a border.
  */
  integral_image->border_color=integral_image->background_color;
  integral_image->compose=CopyCompositeOp;
  border_info.width=(unsigned long) x_offset;
  border_info.height=(unsigned long) y_offset;
  rotate_image=BorderImage(integral_image,&border_info,exception);
  integral_image=DestroyImage(integral_image);
  if (rotate_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  /*
    Rotate the image.
  */
  XShearImage(rotate_image,shear.x,width,height,x_offset,
    ((long) rotate_image->rows-height)/2);
  YShearImage(rotate_image,shear.y,y_width,height,
    ((long) rotate_image->columns-y_width)/2,y_offset);
  XShearImage(rotate_image,shear.x,y_width,rotate_image->rows,
    ((long) rotate_image->columns-y_width)/2,0);
  CropToFitImage(&rotate_image,shear.x,shear.y,(MagickRealType) width,
    (MagickRealType) height,MagickTrue,exception);
  rotate_image->compose=image->compose;
  rotate_image->page.width=0;
  rotate_image->page.height=0;
  return(rotate_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S h e a r I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ShearImage() creates a new image that is a shear_image copy of an existing
%  one.  Shearing slides one edge of an image along the X or Y axis, creating
%  a parallelogram.  An X direction shear slides an edge along the X axis,
%  while a Y direction shear slides an edge along the Y axis.  The amount of
%  the shear is controlled by a shear angle.  For X direction shears, x_shear
%  is measured relative to the Y axis, and similarly, for Y direction shears
%  y_shear is measured relative to the X axis.  Empty triangles left over from
%  shearing the image are filled with the background color defined by member
%  'background_color' of the image..  ShearImage() allocates the memory
%  necessary for the new Image structure and returns a pointer to the new image.
%
%  ShearImage() is based on the paper "A Fast Algorithm for General Raster
%  Rotatation" by Alan W. Paeth.
%
%  The format of the ShearImage method is:
%
%      Image *ShearImage(const Image *image,const double x_shear,
%        const double y_shear,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o x_shear, y_shear: Specifies the number of degrees to shear the image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *ShearImage(const Image *image,const double x_shear,
  const double y_shear,ExceptionInfo *exception)
{
  Image
    *integral_image,
    *shear_image;

  long
    x_offset,
    y_offset;

  PointInfo
    shear;

  RectangleInfo
    border_info;

  unsigned long
    y_width;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if ((x_shear != 0.0) && (fmod(x_shear,90.0) == 0.0))
    ThrowImageException(ImageError,"AngleIsDiscontinuous");
  if ((y_shear != 0.0) && (fmod(y_shear,90.0) == 0.0))
    ThrowImageException(ImageError,"AngleIsDiscontinuous");
  /*
    Initialize shear angle.
  */
  integral_image=CloneImage(image,0,0,MagickTrue,exception);
  if (integral_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  shear.x=(-tan(DegreesToRadians(x_shear)));
  shear.y=tan(DegreesToRadians(y_shear));
  if ((shear.x == 0.0) && (shear.y == 0.0))
    return(integral_image);
  if (SetImageStorageClass(integral_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&integral_image->exception);
      integral_image=DestroyImage(integral_image);
      return(integral_image);
    }
  if (integral_image->matte == MagickFalse)
    (void) SetImageOpacity(integral_image,OpaqueOpacity);
  /*
    Compute image size.
  */
  y_width=image->columns+(long) (fabs(shear.x)*image->rows+0.5);
  x_offset=(long) (image->columns+((fabs(shear.x)*image->rows)-
    image->columns)/2.0+0.5);
  y_offset=(long) (image->rows+((fabs(shear.y)*y_width+0.5)-image->rows)/2.0+
    0.5);
  /*
    Surround image with border.
  */
  integral_image->border_color=integral_image->background_color;
  integral_image->compose=CopyCompositeOp;
  border_info.width=(unsigned long) x_offset;
  border_info.height=(unsigned long) y_offset;
  shear_image=BorderImage(integral_image,&border_info,exception);
  if (shear_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  integral_image=DestroyImage(integral_image);
  /*
    Shear the image.
  */
  if (shear_image->matte == MagickFalse)
    (void) SetImageOpacity(shear_image,OpaqueOpacity);
  XShearImage(shear_image,shear.x,image->columns,image->rows,x_offset,
    ((long) shear_image->rows-image->rows)/2);
  YShearImage(shear_image,shear.y,y_width,image->rows,
    ((long) shear_image->columns-y_width)/2,y_offset);
  CropToFitImage(&shear_image,shear.x,shear.y,(MagickRealType) image->columns,
    (MagickRealType) image->rows,MagickFalse,exception);
  shear_image->compose=image->compose;
  shear_image->page.width=0;
  shear_image->page.height=0;
  return(shear_image);
}
