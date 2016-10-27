/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                      PPPP    AAA   IIIII  N   N  TTTTT                      %
%                      P   P  A   A    I    NN  N    T                        %
%                      PPPP   AAAAA    I    N N N    T                        %
%                      P      A   A    I    N  NN    T                        %
%                      P      A   A  IIIII  N   N    T                        %
%                                                                             %
%                                                                             %
%                        Methods to Paint on an Image                         %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1998                                   %
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
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace-private.h"
#include "magick/composite.h"
#include "magick/composite-private.h"
#include "magick/draw.h"
#include "magick/draw-private.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/monitor.h"
#include "magick/paint.h"
#include "magick/pixel-private.h"
#include "magick/string_.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a i n t F l o o d f i l l I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PaintFloodfill() changes the color value of any pixel that matches
%  target and is an immediate neighbor.  If the method FillToBorderMethod is
%  specified, the color value is changed for any neighbor pixel that does not
%  match the bordercolor member of image.
%
%  By default target must match a particular pixel color exactly.
%  However, in many cases two colors may differ by a small amount.  The
%  fuzz member of image defines how much tolerance is acceptable to
%  consider two colors as the same.  For example, set fuzz to 10 and the
%  color red at intensities of 100 and 102 respectively are now
%  interpreted as the same color for the purposes of the floodfill.
%
%  The format of the PaintFloodfillImage method is:
%
%      MagickBooleanType PaintFloodfillImage(Image *image,
%        const ChannelType channel,const MagickPixelPacket target,
%        const long x_offset,const long y_offset,const DrawInfo *draw_info,
%        const PaintMethod method)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel(s).
%
%    o target: The RGB value of the target color.
%
%    o x_offset,y_offset: The starting location of the operation.
%
%    o draw_info: The draw info.
%
%    o method: Choose either FloodfillMethod or FillToBorderMethod.
%
*/
MagickExport MagickBooleanType PaintFloodfillImage(Image *image,
  const ChannelType channel,const MagickPixelPacket *target,
  const long x_offset,const long y_offset,const DrawInfo *draw_info,
  const PaintMethod method)
{
#define MaxStacksize  (1UL << 15)
#define PushSegmentStack(up,left,right,delta) \
{ \
  if (s >= (segment_stack+MaxStacksize)) \
    ThrowBinaryException(DrawError,"SegmentStackOverflow",image->filename) \
  else \
    { \
      if ((((up)+(delta)) >= 0) && (((up)+(delta)) < (long) image->rows)) \
        { \
          s->x1=(double) (left); \
          s->y1=(double) (up); \
          s->x2=(double) (right); \
          s->y2=(double) (delta); \
          s++; \
        } \
    } \
}

  Image
    *floodplane_image;

  long
    offset,
    start,
    x1,
    x2,
    y;

  MagickBooleanType
    skip;

  MagickPixelPacket
    fill,
    pixel;

  PixelPacket
    fill_color;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  register SegmentInfo
    *s;

  SegmentInfo
    *segment_stack;

  /*
    Check boundary conditions.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(draw_info != (DrawInfo *) NULL);
  assert(draw_info->signature == MagickSignature);
  if ((x_offset < 0) || (x_offset >= (long) image->columns))
    return(MagickFalse);
  if ((y_offset < 0) || (y_offset >= (long) image->rows))
    return(MagickFalse);
  if (SetImageStorageClass(image,DirectClass) == MagickFalse)
    return(MagickFalse);
  if (image->matte == MagickFalse)
    (void) SetImageOpacity(image,OpaqueOpacity);
  /*
    Set floodfill state.
  */
  floodplane_image=CloneImage(image,image->columns,image->rows,MagickTrue,
    &image->exception);
  if (floodplane_image == (Image *) NULL)
    return(MagickFalse);
  (void) SetImageOpacity(floodplane_image,OpaqueOpacity);
  segment_stack=(SegmentInfo *) AcquireQuantumMemory(MaxStacksize,
    sizeof(*segment_stack));
  if (segment_stack == (SegmentInfo *) NULL)
    {
      floodplane_image=DestroyImage(floodplane_image);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  /*
    Push initial segment on stack.
  */
  x=x_offset;
  y=y_offset;
  start=0;
  s=segment_stack;
  PushSegmentStack(y,x,x,1);
  PushSegmentStack(y+1,x,x,-1);
  GetMagickPixelPacket(image,&fill);
  GetMagickPixelPacket(image,&pixel);
  while (s > segment_stack)
  {
    /*
      Pop segment off stack.
    */
    s--;
    x1=(long) s->x1;
    x2=(long) s->x2;
    offset=(long) s->y2;
    y=(long) s->y1+offset;
    /*
      Recolor neighboring pixels.
    */
    p=AcquireImagePixels(image,0,y,(unsigned long) (x1+1),1,&image->exception);
    q=GetImagePixels(floodplane_image,0,y,(unsigned long) (x1+1),1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    p+=x1;
    q+=x1;
    for (x=x1; x >= 0; x--)
    {
      if (q->opacity == (Quantum) TransparentOpacity)
        break;
      SetMagickPixelPacket(image,p,indexes+x,&pixel);
      if (method == FloodfillMethod)
        {
          if (IsMagickColorSimilar(&pixel,target) == MagickFalse)
            break;
        }
      else
        if (IsMagickColorSimilar(&pixel,target) != MagickFalse)
          break;
      q->opacity=(Quantum) TransparentOpacity;
      p--;
      q--;
    }
    if (SyncImagePixels(floodplane_image) == MagickFalse)
      break;
    skip=x >= x1 ? MagickTrue : MagickFalse;
    if (skip == MagickFalse)
      {
        start=x+1;
        if (start < x1)
          PushSegmentStack(y,start,x1-1,-offset);
        x=x1+1;
      }
    do
    {
      if (skip == MagickFalse)
        {
          if (x < (long) image->columns)
            {
              p=AcquireImagePixels(image,x,y,image->columns-x,1,
                &image->exception);
              q=GetImagePixels(floodplane_image,x,y,image->columns-x,1);
              if ((p == (const PixelPacket *) NULL) ||
                  (q == (PixelPacket *) NULL))
                break;
              indexes=GetIndexes(image);
              for ( ; x < (long) image->columns; x++)
              {
                if (q->opacity == (Quantum) TransparentOpacity)
                  break;
                SetMagickPixelPacket(image,p,indexes+x,&pixel);
                if (method == FloodfillMethod)
                  {
                    if (IsMagickColorSimilar(&pixel,target) == MagickFalse)
                      break;
                  }
                else
                  if (IsMagickColorSimilar(&pixel,target) != MagickFalse)
                    break;
                q->opacity=(Quantum) TransparentOpacity;
                p++;
                q++;
              }
              if (SyncImagePixels(floodplane_image) == MagickFalse)
                break;
            }
          PushSegmentStack(y,start,x-1,offset);
          if (x > (x2+1))
            PushSegmentStack(y,x2+1,x-1,-offset);
        }
      skip=MagickFalse;
      x++;
      if (x <= x2)
        {
          p=AcquireImagePixels(image,x,y,(unsigned long) (x2-x+1),1,
            &image->exception);
          q=GetImagePixels(floodplane_image,x,y,(unsigned long) (x2-x+1),1);
          if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
            break;
          indexes=GetIndexes(image);
          for ( ; x <= x2; x++)
          {
            if (q->opacity == (Quantum) TransparentOpacity)
              break;
            SetMagickPixelPacket(image,p,indexes+x,&pixel);
            if (method == FloodfillMethod)
              {
                if (IsMagickColorSimilar(&pixel,target) != MagickFalse)
                  break;
              }
            else
              if (IsMagickColorSimilar(&pixel,target) == MagickFalse)
                break;
            p++;
            q++;
          }
        }
      start=x;
    } while (x <= x2);
  }
  for (y=0; y < (long) image->rows; y++)
  {
    /*
      Tile fill color onto floodplane.
    */
    p=AcquireImagePixels(floodplane_image,0,y,image->columns,1,
      &image->exception);
    q=GetImagePixels(image,0,y,image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      if (p->opacity != OpaqueOpacity)
        {
          fill_color=GetFillColor(draw_info,x,y);
          SetMagickPixelPacket(image,&fill_color,(IndexPacket *) NULL,&fill);
          if (image->colorspace == CMYKColorspace)
            ConvertRGBToCMYK(&fill);
          if ((channel & RedChannel) != 0)
            q->red=RoundToQuantum(fill.red);
          if ((channel & GreenChannel) != 0)
            q->green=RoundToQuantum(fill.green);
          if ((channel & BlueChannel) != 0)
            q->blue=RoundToQuantum(fill.blue);
          if ((channel & OpacityChannel) != 0)
            q->opacity=RoundToQuantum(fill.opacity);
          if (((channel & IndexChannel) != 0) &&
              (image->colorspace == CMYKColorspace))
            indexes[x]=RoundToQuantum(fill.index);
        }
      p++;
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
  }
  segment_stack=(SegmentInfo *) RelinquishMagickMemory(segment_stack);
  floodplane_image=DestroyImage(floodplane_image);
  return(y == (long) image->rows ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     P a i n t O p a q u e I m a g e                                         %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PaintOpaqueImage() changes any pixel that matches color with the color
%  defined by fill.
%
%  By default color must match a particular pixel color exactly.  However,
%  in many cases two colors may differ by a small amount.  Fuzz defines
%  how much tolerance is acceptable to consider two colors as the same.
%  For example, set fuzz to 10 and the color red at intensities of 100 and
%  102 respectively are now interpreted as the same color.
%
%  The format of the PaintOpaqueImage method is:
%
%      MagickBooleanType PaintOpaqueImage(Image *image,
%        const PixelPacket *target,const PixelPacket *fill)
%      MagickBooleanType PaintOpaqueImageChannel(Image *image,
%        const ChannelType channel,const PixelPacket *target,
%        const PixelPacket *fill)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel(s).
%
%    o target: The RGB value of the target color.
%
%    o fill: The replacement color.
%
*/

MagickExport MagickBooleanType PaintOpaqueImage(Image *image,
  const MagickPixelPacket *target,const MagickPixelPacket *fill)
{
  return(PaintOpaqueImageChannel(image,DefaultChannels,target,fill));
}

MagickExport MagickBooleanType PaintOpaqueImageChannel(Image *image,
  const ChannelType channel,const MagickPixelPacket *target,
  const MagickPixelPacket *fill)
{
#define PaintOpaqueImageTag  "Opaque/Image"

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Make image color opaque.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(target != (MagickPixelPacket *) NULL);
  assert(fill != (MagickPixelPacket *) NULL);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetImageStorageClass(image,DirectClass) == MagickFalse)
    return(MagickFalse);
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
      if (IsMagickColorSimilar(&pixel,target) != MagickFalse)
        {
          if ((channel & RedChannel) != 0)
            q->red=RoundToQuantum(fill->red);
          if ((channel & GreenChannel) != 0)
            q->green=RoundToQuantum(fill->green);
          if ((channel & BlueChannel) != 0)
            q->blue=RoundToQuantum(fill->blue);
          if ((channel & OpacityChannel) != 0)
            q->opacity=RoundToQuantum(fill->opacity);
          if (((channel & IndexChannel) != 0) &&
              (image->colorspace == CMYKColorspace))
            indexes[x]=RoundToQuantum(fill->index);
        }
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(PaintOpaqueImageTag,y,image->rows,
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
%     P a i n t T r a n s p a r e n t I m a g e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PaintTransparentImage() changes the opacity value associated with any pixel
%  that matches color to the value defined by opacity.
%
%  By default color must match a particular pixel color exactly.  However,
%  in many cases two colors may differ by a small amount.  Fuzz defines
%  how much tolerance is acceptable to consider two colors as the same.
%  For example, set fuzz to 10 and the color red at intensities of 100 and
%  102 respectively are now interpreted as the same color.
%
%  The format of the PaintTransparentImage method is:
%
%      MagickBooleanType PaintTransparentImage(Image *image,
%        const MagickPixelPacket *target,const Quantum opacity)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o target: The RGB value of the target color.
%
%    o opacity: The replacement opacity value.
%
*/
MagickExport MagickBooleanType PaintTransparentImage(Image *image,
  const MagickPixelPacket *target,const Quantum opacity)
{
#define PaintTransparentImageTag  "Transparent/Image"

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Make image color transparent.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(target != (MagickPixelPacket *) NULL);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetImageStorageClass(image,DirectClass) == MagickFalse)
    return(MagickFalse);
  if (image->matte == MagickFalse)
    (void) SetImageOpacity(image,OpaqueOpacity);
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
      if (IsMagickColorSimilar(&pixel,target) != MagickFalse)
        q->opacity=opacity;
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(PaintTransparentImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(MagickTrue);
}
