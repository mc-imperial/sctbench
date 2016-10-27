/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        DDDD   EEEEE  PPPP   RRRR   EEEEE   CCCC   AAA   TTTTT  EEEEE        %
%        D   D  E      P   P  R   R  E      C      A   A    T    E            %
%        D   D  EEE    PPPPP  RRRR   EEE    C      AAAAA    T    EEE          %
%        D   D  E      P      R R    E      C      A   A    T    E            %
%        DDDD   EEEEE  P      R  R   EEEEE   CCCC  A   A    T    EEEEE        %
%                                                                             %
%                                                                             %
%                       MagickWand Deprecated Methods                         %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                October 2002                                 %
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
#include "wand/studio.h"
#include "wand/MagickWand.h"
#include "wand/magick-wand-private.h"
#include "wand/wand.h"

/*
  Define declarations.
*/
#define ThrowWandException(severity,tag,context) \
{ \
  (void) ThrowMagickException(wand->exception,GetMagickModule(),severity, \
    tag,"`%s'",context); \
  return(MagickFalse); \
}

#if !defined(ExcludeMagickDeprecated)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t F i l l A l p h a                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetFillAlpha() returns the alpha used when drawing using the fill
%  color or fill texture.  Fully opaque is 1.0.
%
%  The format of the DrawGetFillAlpha method is:
%
%      double DrawGetFillAlpha(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport double DrawGetFillAlpha(const DrawingWand *wand)
{
  return(DrawGetFillOpacity(wand));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t S t r o k e A l p h a                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetStrokeAlpha() returns the alpha of stroked object outlines.
%
%  The format of the DrawGetStrokeAlpha method is:
%
%      double DrawGetStrokeAlpha(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
*/
WandExport double DrawGetStrokeAlpha(const DrawingWand *wand)
{
  return(DrawGetStrokeOpacity(wand));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P e e k G r a p h i c W a n d                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPeekGraphicWand() returns the current drawing wand.
%
%  The format of the PeekDrawingWand method is:
%
%      DrawInfo *DrawPeekGraphicWand(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport DrawInfo *DrawPeekGraphicWand(const DrawingWand *wand)
{
  return(PeekDrawingWand(wand));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P o p G r a p h i c C o n t e x t                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPopGraphicContext() destroys the current drawing wand and returns to the
%  previously pushed drawing wand. Multiple drawing wands may exist. It is an
%  error to attempt to pop more drawing wands than have been pushed, and it is
%  proper form to pop all drawing wands which have been pushed.
%
%  The format of the DrawPopGraphicContext method is:
%
%      MagickBooleanType DrawPopGraphicContext(DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport void DrawPopGraphicContext(DrawingWand *wand)
{
  (void) PopDrawingWand(wand);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P u s h G r a p h i c C o n t e x t                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPushGraphicContext() clones the current drawing wand to create a new
%  drawing wand.  The original drawing drawing wand(s) may be returned to by
%  invoking PopDrawingWand().  The drawing wands are stored on a drawing wand
%  stack.  For every Pop there must have already been an equivalent Push.
%
%  The format of the DrawPushGraphicContext method is:
%
%      MagickBooleanType DrawPushGraphicContext(DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport void DrawPushGraphicContext(DrawingWand *wand)
{
  (void) PushDrawingWand(wand);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t F i l l A l p h a                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetFillAlpha() sets the alpha to use when drawing using the fill
%  color or fill texture.  Fully opaque is 1.0.
%
%  The format of the DrawSetFillAlpha method is:
%
%      void DrawSetFillAlpha(DrawingWand *wand,const double fill_alpha)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o fill_alpha: fill alpha
%
*/
WandExport void DrawSetFillAlpha(DrawingWand *wand,const double fill_alpha)
{
  DrawSetFillOpacity(wand,fill_alpha);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t S t r o k e A l p h a                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetStrokeAlpha() specifies the alpha of stroked object outlines.
%
%  The format of the DrawSetStrokeAlpha method is:
%
%      void DrawSetStrokeAlpha(DrawingWand *wand,const double stroke_alpha)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o stroke_alpha: stroke alpha.  The value 1.0 is opaque.
%
*/
WandExport void DrawSetStrokeAlpha(DrawingWand *wand,const double stroke_alpha)
{
  DrawSetStrokeOpacity(wand,stroke_alpha);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C o l o r F l o o d f i l l I m a g e                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickColorFloodfillImage() changes the color value of any pixel that matches
%  target and is an immediate neighbor.  If the method FillToBorderMethod is
%  specified, the color value is changed for any neighbor pixel that does not
%  match the bordercolor member of image.
%
%  The format of the MagickColorFloodfillImage method is:
%
%      MagickBooleanType MagickColorFloodfillImage(MagickWand *wand,
%        const PixelWand *fill,const double fuzz,const PixelWand *bordercolor,
%        const long x,const long y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o fill: The floodfill color pixel wand.
%
%    o fuzz: By default target must match a particular pixel color
%      exactly.  However, in many cases two colors may differ by a small amount.
%      The fuzz member of image defines how much tolerance is acceptable to
%      consider two colors as the same.  For example, set fuzz to 10 and the
%      color red at intensities of 100 and 102 respectively are now interpreted
%      as the same color for the purposes of the floodfill.
%
%    o bordercolor: The border color pixel wand.
%
%    o x,y: The starting location of the operation.
%
*/
WandExport MagickBooleanType MagickColorFloodfillImage(MagickWand *wand,
  const PixelWand *fill,const double fuzz,const PixelWand *bordercolor,
  const long x,const long y)
{
  DrawInfo
    *draw_info;

  MagickBooleanType
    status;

  PixelPacket
    target;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  draw_info=CloneDrawInfo(wand->image_info,(DrawInfo *) NULL);
  PixelGetQuantumColor(fill,&draw_info->fill);
  target=AcquireOnePixel(wand->images,x % wand->images->columns,
    y % wand->images->rows,wand->exception);
  if (bordercolor != (PixelWand *) NULL)
    PixelGetQuantumColor(bordercolor,&target);
  wand->images->fuzz=fuzz;
  status=ColorFloodfillImage(wand->images,draw_info,target,x,y,
    bordercolor != (PixelWand *) NULL ? FillToBorderMethod : FloodfillMethod);
  if (status == MagickFalse)
    InheritException(wand->exception,&wand->images->exception);
  draw_info=DestroyDrawInfo(draw_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k D e s c r i b e I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickDescribeImage() identifies an image by printing its attributes to the
%  file.  Attributes include the image width, height, size, and others.
%
%  The format of the MagickDescribeImage method is:
%
%      const char *MagickDescribeImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport char *MagickDescribeImage(MagickWand *wand)
{
  return(MagickIdentifyImage(wand));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e A t t r i b u t e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageAttribute() returns a value associated with the specified
%  property.  Use MagickRelinquishMemory() to free the value when you are
%  finished with it.
%
%  The format of the MagickGetImageAttribute method is:
%
%      char *MagickGetImageAttribute(MagickWand *wand,const char *property)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o property: The property.
%
*/
WandExport char *MagickGetImageAttribute(MagickWand *wand,const char *property)
{
  return(MagickGetImageProperty(wand,property));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   M a g i c k G e t I m a g e I n d e x                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageIndex() returns the index of the current image.
%
%  The format of the MagickGetImageIndex method is:
%
%      long MagickGetImageIndex(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport long MagickGetImageIndex(MagickWand *wand)
{
  return(MagickGetIteratorIndex(wand));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   M a g i c k G e t I m a g e C h a n n e l E x t r e m a                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageChannelExtrema() gets the extrema for one or more image
%  channels.
%
%  The format of the MagickGetImageChannelExtrema method is:
%
%      MagickBooleanType MagickGetImageChannelExtrema(MagickWand *wand,
%        const ChannelType channel,unsigned long *minima,unsigned long *maxima)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: The image channel(s).
%
%    o minima:  The minimum pixel value for the specified channel(s).
%
%    o maxima:  The maximum pixel value for the specified channel(s).
%
*/
WandExport MagickBooleanType MagickGetImageChannelExtrema(MagickWand *wand,
  const ChannelType channel,unsigned long *minima,unsigned long *maxima)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=GetImageChannelExtrema(wand->images,channel,minima,maxima,
    wand->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   M a g i c k G e t I m a g e E x t r e m a                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageExtrema() gets the extrema for the image.
%
%  The format of the MagickGetImageExtrema method is:
%
%      MagickBooleanType MagickGetImageExtrema(MagickWand *wand,
%        unsigned long *minima,unsigned long *maxima)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o minima:  The minimum pixel value for the specified channel(s).
%
%    o maxima:  The maximum pixel value for the specified channel(s).
%
*/
WandExport MagickBooleanType MagickGetImageExtrema(MagickWand *wand,
  unsigned long *minima,unsigned long *maxima)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=GetImageExtrema(wand->images,minima,maxima,wand->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k M a t t e F l o o d f i l l I m a g e                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickMatteFloodfillImage() changes the transparency value of any pixel that
%  matches target and is an immediate neighbor.  If the method
%  FillToBorderMethod is specified, the transparency value is changed for any
%  neighbor pixel that does not match the bordercolor member of image.
%
%  The format of the MagickMatteFloodfillImage method is:
%
%      MagickBooleanType MagickMatteFloodfillImage(MagickWand *wand,
%        const double alpha,const double fuzz,const PixelWand *bordercolor,
%        const long x,const long y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o alpha: The level of transparency: 1.0 is fully opaque and 0.0 is fully
%      transparent.
%
%    o fuzz: By default target must match a particular pixel color
%      exactly.  However, in many cases two colors may differ by a small amount.
%      The fuzz member of image defines how much tolerance is acceptable to
%      consider two colors as the same.  For example, set fuzz to 10 and the
%      color red at intensities of 100 and 102 respectively are now interpreted
%      as the same color for the purposes of the floodfill.
%
%    o bordercolor: The border color pixel wand.
%
%    o x,y: The starting location of the operation.
%
*/
WandExport MagickBooleanType MagickMatteFloodfillImage(MagickWand *wand,
  const double alpha,const double fuzz,const PixelWand *bordercolor,
  const long x,const long y)
{
  DrawInfo
    *draw_info;

  MagickBooleanType
    status;

  PixelPacket
    target;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  draw_info=CloneDrawInfo(wand->image_info,(DrawInfo *) NULL);
  target=AcquireOnePixel(wand->images,x % wand->images->columns,
    y % wand->images->rows,wand->exception);
  if (bordercolor != (PixelWand *) NULL)
    PixelGetQuantumColor(bordercolor,&target);
  wand->images->fuzz=fuzz;
  status=MatteFloodfillImage(wand->images,target,RoundToQuantum(
    (MagickRealType) QuantumRange-QuantumRange*alpha),x,y,bordercolor !=
    (PixelWand *) NULL ? FillToBorderMethod : FloodfillMethod);
  if (status == MagickFalse)
    InheritException(wand->exception,&wand->images->exception);
  draw_info=DestroyDrawInfo(draw_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k O p a q u e I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickOpaqueImage() changes any pixel that matches color with the color
%  defined by fill.
%
%  The format of the MagickOpaqueImage method is:
%
%      MagickBooleanType MagickOpaqueImage(MagickWand *wand,
%        const PixelWand *target,const PixelWand *fill,const double fuzz)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: The channel(s).
%
%    o target: Change this target color to the fill color within the image.
%
%    o fill: The fill pixel wand.
%
%    o fuzz: By default target must match a particular pixel color
%      exactly.  However, in many cases two colors may differ by a small amount.
%      The fuzz member of image defines how much tolerance is acceptable to
%      consider two colors as the same.  For example, set fuzz to 10 and the
%      color red at intensities of 100 and 102 respectively are now interpreted
%      as the same color for the purposes of the floodfill.
%
*/
WandExport MagickBooleanType MagickOpaqueImage(MagickWand *wand,
  const PixelWand *target,const PixelWand *fill,const double fuzz)
{
  return(MagickPaintOpaqueImage(wand,target,fill,fuzz));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e A t t r i b u t e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageAttribute() associates a property with an image.
%
%  The format of the MagickSetImageAttribute method is:
%
%      MagickBooleanType MagickSetImageAttribute(MagickWand *wand,
%        const char *property,const char *value)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o property: The property.
%
%    o value: The value.
%
*/
WandExport MagickBooleanType MagickSetImageAttribute(MagickWand *wand,
  const char *property,const char *value)
{
  return(SetImageProperty(wand->images,property,value));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e I n d e x                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageIndex() set the current image to the position of the list
%  specified with the index parameter.
%
%  The format of the MagickSetImageIndex method is:
%
%      MagickBooleanType MagickSetImageIndex(MagickWand *wand,const long index)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o index: The scene number.
%
*/
WandExport MagickBooleanType MagickSetImageIndex(MagickWand *wand,
  const long index)
{
  return(MagickSetIteratorIndex(wand,index));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   M a g i c k S e t I m a g e O p t i o n                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageOption() associates one or options with a particular image
%  format (.e.g MagickSetImageOption(wand,"jpeg","perserve","yes").
%
%  The format of the MagickSetImageOption method is:
%
%      MagickBooleanType MagickSetImageOption(MagickWand *wand,
%        const char *format,const char *key,const char *value)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o format: The image format.
%
%    o key:  The key.
%
%    o value:  The value.
%
*/
WandExport MagickBooleanType MagickSetImageOption(MagickWand *wand,
  const char *format,const char *key,const char *value)
{
  char
    option[MaxTextExtent];

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) FormatMagickString(option,MaxTextExtent,"%s:%s=%s",format,key,value);
  return(DefineImageOption(wand->image_info,option));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k T r a n s p a r e n t I m a g e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickTransparentImage() changes any pixel that matches color with the
%  color defined by fill.
%
%  The format of the MagickTransparentImage method is:
%
%      MagickBooleanType MagickTransparentImage(MagickWand *wand,
%        const PixelWand *target,const double alpha,const double fuzz)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o target: Change this target color to specified opacity value within
%      the image.
%
%    o alpha: The level of transparency: 1.0 is fully opaque and 0.0 is fully
%      transparent.
%
%    o fuzz: By default target must match a particular pixel color
%      exactly.  However, in many cases two colors may differ by a small amount.
%      The fuzz member of image defines how much tolerance is acceptable to
%      consider two colors as the same.  For example, set fuzz to 10 and the
%      color red at intensities of 100 and 102 respectively are now interpreted
%      as the same color for the purposes of the floodfill.
%
*/
WandExport MagickBooleanType MagickTransparentImage(MagickWand *wand,
  const PixelWand *target,const double alpha,const double fuzz)
{
  return(MagickPaintTransparentImage(wand,target,alpha,fuzz));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k R e g i o n O f I n t e r e s t I m a g e                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickRegionOfInterestImage() extracts a region of the image and returns it
%  as a a new wand.
%
%  The format of the MagickRegionOfInterestImage method is:
%
%      MagickWand *MagickRegionOfInterestImage(MagickWand *wand,
%        const unsigned long width,const unsigned long height,const long x,
%        const long y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o width: The region width.
%
%    o height: The region height.
%
%    o x: The region x offset.
%
%    o y: The region y offset.
%
*/
WandExport MagickWand *MagickRegionOfInterestImage(MagickWand *wand,
  const unsigned long width,const unsigned long height,const long x,
  const long y)
{
  return(MagickGetImageRegion(wand,width,height,x,y));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e S i z e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageSize() returns the image length in bytes.
%
%  The format of the MagickGetImageSize method is:
%
%      MagickBooleanType MagickGetImageSize(MagickWand *wand,
%        MagickSizeType *length)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o length: The image length in bytes.
%
*/
WandExport MagickSizeType MagickGetImageSize(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  return(GetBlobSize(wand->images));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k W r i t e I m a g e B l o b                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickWriteImageBlob() implements direct to memory image formats.  It
%  returns the image as a blob and its length.   Use MagickSetFormat() to
%  set the format of the returned blob (GIF, JPEG,  PNG, etc.).
%
%  Use MagickRelinquishMemory() to free the blob when you are done with it.
%
%  The format of the MagickWriteImageBlob method is:
%
%      unsigned char *MagickWriteImageBlob(MagickWand *wand,size_t *length)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o length: The length of the blob.
%
*/
WandExport unsigned char *MagickWriteImageBlob(MagickWand *wand,size_t *length)
{
  return(MagickGetImageBlob(wand,length));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e V i r t u a l P i x e l M e t h o d           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageVirtualPixelMethod() sets the image virtual pixel method.
%
%  The format of the MagickSetImageVirtualPixelMethod method is:
%
%      VirtualPixelMethod MagickSetImageVirtualPixelMethod(MagickWand *wand,
%        const VirtualPixelMethod method)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o method: The image virtual pixel method : UndefinedVirtualPixelMethod,
%      ConstantVirtualPixelMethod,  EdgeVirtualPixelMethod,
%      MirrorVirtualPixelMethod, or TileVirtualPixelMethod.
%
*/
WandExport VirtualPixelMethod MagickSetImageVirtualPixelMethod(MagickWand *wand,
  const VirtualPixelMethod method)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images != (Image *) NULL)
    return(UndefinedVirtualPixelMethod);
  return(SetImageVirtualPixelMethod(wand->images,method));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l G e t N e x t R o w                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PixelGetNextRow() returns the next row as an array of pixel wands from the
%  pixel iterator.
%
%  The format of the PixelGetNextRow method is:
%
%      PixelWand **PixelGetNextRow(PixelIterator *iterator,
%        unsigned long *number_wands)
%
%  A description of each parameter follows:
%
%    o iterator: The pixel iterator.
%
%    o number_wands: The number of pixel wands.
%
*/
WandExport PixelWand **PixelGetNextRow(PixelIterator *iterator)
{
  unsigned long
    number_wands;

  return(PixelGetNextIteratorRow(iterator,&number_wands));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l I t e r a t o r G e t E x c e p t i o n                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PixelIteratorGetException() returns the severity, reason, and description of
%  any error that occurs when using other methods in this API.
%
%  The format of the PixelIteratorGetException method is:
%
%      char *PixelIteratorGetException(const Pixeliterator *iterator,
%        ExceptionType *severity)
%
%  A description of each parameter follows:
%
%    o iterator: The pixel iterator.
%
%    o severity: The severity of the error is returned here.
%
*/
WandExport char *PixelIteratorGetException(const PixelIterator *iterator,
  ExceptionType *severity)
{
  return(PixelGetIteratorException(iterator,severity));
}
#endif
