/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%           PPPP    RRRR    EEEEE  PPPP   RRRR   EEEEE  SSSSS  SSSSS          %
%           P   P   R   R   E      P   P  R   R  E      SS     SS             %
%           PPPP    RRRR    EEE    PPPP   RRRR   EEE     SSS    SSS           %
%           P       R R     E      P      R R    E         SS     SS          %
%           P       R  R    EEEEE  P      R  R   EEEEE  SSSSS  SSSSS          %
%                                                                             %
%                                                                             %
%                        ImageMagick Prepress Methods.                        %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                October 2001                                 %
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
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/hashmap.h"
#include "magick/image.h"
#include "magick/list.h"
#include "magick/memory_.h"
#include "magick/prepress.h"
#include "magick/registry.h"
#include "magick/semaphore.h"
#include "magick/splay-tree.h"
#include "magick/string_.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e T o t a l I n k D e n s i t y                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageTotalInkDensity() returns the total ink density for a CMYK image.
%  Total Ink Density (TID) is determined by adding the CMYK values in the
%  darkest shadow area in an image.
%
%  The format of the GetImageTotalInkDensity method is:
%
%      double GetImageTotalInkDensity(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
MagickExport double GetImageTotalInkDensity(Image *image)
{
  double
    density,
    total_ink_density;

  long
    y;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    x;

  assert(image != (Image *) NULL);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickSignature);
  if (image->colorspace != CMYKColorspace)
    {
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        ImageError,"ColorSeparatedImageRequired","`%s'",image->filename);
      return(0.0);
    }
  total_ink_density=0.0;
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
    if (p == (const PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      density=(double) p->red+p->green+p->blue+indexes[x];
      if (density > total_ink_density)
        total_ink_density=density;
      p++;
    }
  }
  return(total_ink_density);
}
