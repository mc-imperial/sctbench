/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                   OOO   PPPP   TTTTT  IIIII   OOO   N   N                   %
%                  O   O  P   P    T      I    O   O  NN  N                   %
%                  O   O  PPPP     T      I    O   O  N N N                   %
%                  O   O  P        T      I    O   O  N  NN                   %
%                   OOO   P        T    IIIII   OOO   N   N                   %
%                                                                             %
%                                                                             %
%                        ImageMagick Option Methods                           %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 March 2000                                  %
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
#include "magick/studio.h"
#include "magick/artifact.h"
#include "magick/cache.h"
#include "magick/color.h"
#include "magick/compare.h"
#include "magick/constitute.h"
#include "magick/distort.h"
#include "magick/draw.h"
#include "magick/effect.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/fx.h"
#include "magick/gem.h"
#include "magick/geometry.h"
#include "magick/layer.h"
#include "magick/mime-private.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/montage.h"
#include "magick/option.h"
#include "magick/property.h"
#include "magick/quantum.h"
#include "magick/resource_.h"
#include "magick/splay-tree.h"
#include "magick/string_.h"
#include "magick/token.h"
#include "magick/utility.h"

/*
  ImageMagick options.
*/
static const OptionInfo
  AlignOptions[] =
  {
    { "Undefined", (long) UndefinedAlign },
    { "Center", (long) CenterAlign },
    { "End", (long) RightAlign },
    { "Left", (long) LeftAlign },
    { "Middle", (long) CenterAlign },
    { "Right", (long) RightAlign },
    { "Start", (long) LeftAlign },
    { (char *) NULL, (long) UndefinedAlign }
  },
  AlphaOptions[] =
  {
    { "Undefined", (long) UndefinedAlphaChannel },
    { "Activate", (long) ActivateAlphaChannel },
    { "Deactivate", (long) DeactivateAlphaChannel },
    { "Off", (long) DeactivateAlphaChannel },
    { "On", (long) ActivateAlphaChannel },
    { "Reset", (long) ResetAlphaChannel },
    { "Set", (long) SetAlphaChannel },
    { (char *) NULL, (long) UndefinedAlphaChannel }
  },
  BooleanOptions[] =
  {
    { "False", 0L },
    { "True", 1L },
    { "0", 0L },
    { "1", 1L },
    { (char *) NULL, 0L }
  },
  ChannelOptions[] =
  {
    { "Undefined", (long) UndefinedChannel },
    { "All", (long) AllChannels },
    { "Alpha", (long) OpacityChannel },
    { "Black", (long) BlackChannel },
    { "Blue", (long) BlueChannel },
    { "Cyan", (long) CyanChannel },
    { "Default", (long) DefaultChannels },
    { "Gray", (long) GrayChannel },
    { "Green", (long) GreenChannel },
    { "Hue", (long) RedChannel },
    { "Index", (long) IndexChannel },
    { "Luminosity", (long) BlueChannel },
    { "Magenta", (long) MagentaChannel },
    { "Matte", (long) OpacityChannel },
    { "Opacity", (long) OpacityChannel },
    { "Red", (long) RedChannel },
    { "Saturation", (long) GreenChannel },
    { "Yellow", (long) YellowChannel },
    { (char *) NULL, (long) UndefinedChannel }
  },
  ClassOptions[] =
  {
    { "Undefined", (long) UndefinedClass },
    { "DirectClass", (long) DirectClass },
    { "PseudoClass", (long) PseudoClass },
    { (char *) NULL, (long) UndefinedClass }
  },
  ClipPathOptions[] =
  {
    { "Undefined", (long) UndefinedPathUnits },
    { "ObjectBoundingBox", (long) ObjectBoundingBox },
    { "UserSpace", (long) UserSpace },
    { "UserSpaceOnUse", (long) UserSpaceOnUse },
    { (char *) NULL, (long) UndefinedPathUnits }
  },
  CommandOptions[] =
  {
    { "+adjoin", 0L },
    { "-adjoin", 0L },
    { "-adaptive-sharpen", 1L },
    { "+adaptive-sharpen", 1L },
    { "-adaptive-threshold", 1L },
    { "+adaptive-threshold", 1L },
    { "+affine", 0L },
    { "-affine", 1L },
    { "+alpha", 1L },
    { "-alpha", 1L },
    { "+annotate", 0L },
    { "-annotate", 2L },
    { "+antialias", 0L },
    { "-antialias", 0L },
    { "+append", 0L },
    { "-append", 0L },
    { "+authenticate", 0L },
    { "-authenticate", 1L },
    { "+auto-orient", 0L },
    { "-auto-orient", 0L },
    { "+average", 0L },
    { "-average", 0L },
    { "+backdrop", 0L },
    { "-backdrop", 1L },
    { "+background", 0L },
    { "-background", 1L },
    { "+bias", 0L },
    { "-bias", 1L },
    { "+black-threshold", 0L },
    { "-black-threshold", 1L },
    { "+blend", 0L },
    { "-blend", 1L },
    { "+blue-primary", 0L },
    { "-blue-primary", 1L },
    { "+blur", 0L },
    { "-blur", 1L },
    { "+border", 0L },
    { "-border", 1L },
    { "+bordercolor", 0L },
    { "-bordercolor", 1L },
    { "+borderwidth", 0L },
    { "-borderwidth", 1L },
    { "+box", 0L },
    { "-box", 1L },
    { "+cache", 0L },
    { "-cache", 1L },
    { "+channel", 0L },
    { "-channel", 1L },
    { "+charcoal", 0L },
    { "-charcoal", 0L },
    { "+chop", 0L },
    { "-chop", 1L },
    { "+clip", 0L },
    { "-clip", 0L },
    { "+clip-mask", 0L },
    { "-clip-mask", 1L },
    { "+clip-path", 0L },
    { "-clip-path", 1L },
    { "+clone", 0L },
    { "-clone", 1L },
    { "+clut", 0L },
    { "-clut", 0L },
    { "+coalesce", 0L },
    { "-coalesce", 0L },
    { "+colorize", 0L },
    { "-colorize", 1L },
    { "+colormap", 0L },
    { "-colormap", 1L },
    { "+colors", 0L },
    { "-colors", 1L },
    { "+colorspace", 0L },
    { "-colorspace", 1L },
    { "+combine", 0L },
    { "-combine", 0L },
    { "+comment", 0L },
    { "-comment", 1L },
    { "+compose", 0L },
    { "-compose", 1L },
    { "+composite", 0L },
    { "-composite", 0L },
    { "+compress", 0L },
    { "-compress", 1L },
    { "+contrast", 0L },
    { "-contrast", 0L },
    { "+contrast-stretch", 0L },
    { "-contrast-stretch", 1L },
    { "+convolve", 0L },
    { "-convolve", 1L },
    { "+crop", 0L },
    { "-crop", 1L },
    { "+cycle", 0L },
    { "-cycle", 1L },
    { "+debug", 0L },
    { "-debug", 1L },
    { "+deconstruct", 0L },
    { "-deconstruct", 0L },
    { "+define", 1L },
    { "-define", 1L },
    { "+delay", 0L },
    { "-delay", 1L },
    { "+delete", 0L },
    { "-delete", 1L },
    { "+density", 0L },
    { "-density", 1L },
    { "+depth", 0L },
    { "-depth", 1L },
    { "+descend", 0L },
    { "-descend", 1L },
    { "+despeckle", 0L },
    { "-despeckle", 0L },
    { "+displace", 0L },
    { "-displace", 1L },
    { "+display", 0L },
    { "-display", 1L },
    { "+dispose", 0L },
    { "-dispose", 1L },
    { "+dissolve", 0L },
    { "-dissolve", 1L },
    { "+distort", 2L },
    { "-distort", 2L },
    { "+dither", 0L },
    { "-dither", 0L },
    { "+draw", 0L },
    { "-draw", 1L },
    { "+edge", 0L },
    { "-edge", 1L },
    { "+emboss", 0L },
    { "-emboss", 1L },
    { "+encoding", 0L },
    { "-encoding", 1L },
    { "+endian", 0L },
    { "-endian", 1L },
    { "+enhance", 0L },
    { "-enhance", 0L },
    { "+equalize", 0L },
    { "-equalize", 0L },
    { "+evaluate", 0L },
    { "-evaluate", 2L },
    { "+extent", 0L },
    { "-extent", 1L },
    { "+extract", 0L },
    { "-extract", 1L },
    { "+family", 0L },
    { "-family", 1L },
    { "+fill", 0L },
    { "-fill", 1L },
    { "+filter", 0L },
    { "-filter", 1L },
    { "+flatten", 0L },
    { "-flatten", 0L },
    { "+flip", 0L },
    { "-flip", 0L },
    { "+floodfill", 0L },
    { "-floodfill", 2L },
    { "+flop", 0L },
    { "-flop", 0L },
    { "+font", 0L },
    { "-font", 1L },
    { "+foreground", 0L },
    { "-foreground", 1L },
    { "+format", 0L },
    { "-format", 1L },
    { "+frame", 0L },
    { "-frame", 1L },
    { "+fuzz", 0L },
    { "-fuzz", 1L },
    { "+fx", 0L },
    { "-fx", 1L },
    { "+gamma", 0L },
    { "-gamma", 1L },
    { "+gaussian", 0L },
    { "-gaussian", 1L },
    { "+gaussian-blur", 0L },
    { "-gaussian-blur", 1L },
    { "+geometry", 0L },
    { "-geometry", 1L },
    { "+gravity", 0L },
    { "-gravity", 1L },
    { "+green-primary", 0L },
    { "-green-primary", 1L },
    { "+help", 0L },
    { "-help", 0L },
    { "+iconGeometry", 0L },
    { "-iconGeometry", 1L },
    { "+iconic", 0L },
    { "-iconic", 1L },
    { "+identify", 0L },
    { "-identify", 0L },
    { "+immutable", 0L },
    { "-immutable", 1L },
    { "+implode", 0L },
    { "-implode", 1L },
    { "+insert", 0L },
    { "-insert", 1L },
    { "+intent", 0L },
    { "-intent", 1L },
    { "+interlace", 0L },
    { "-interlace", 1L },
    { "+interpolate", 0L },
    { "-interpolate", 1L },
    { "+label", 0L },
    { "-label", 1L },
    { "+lat", 0L },
    { "-lat", 1L },
    { "+layers", 0L },
    { "-layers", 1L },
    { "+level", 0L },
    { "-level", 1L },
    { "+limit", 0L },
    { "-limit", 2L },
    { "+linear-stretch", 0L },
    { "-linear-stretch", 1L },
    { "+linewidth", 0L },
    { "-linewidth", 1L },
    { "+list", 0L },
    { "-list", 1L },
    { "+log", 0L },
    { "-log", 1L },
    { "+loop", 0L },
    { "-loop", 1L },
    { "+magnify", 0L },
    { "-magnify", 1L },
    { "+map", 0L },
    { "-map", 1L },
    { "+mask", 0L },
    { "-mask", 1L },
    { "+matte", 0L },
    { "-matte", 0L },
    { "+mattecolor", 0L },
    { "-mattecolor", 1L },
    { "+median", 0L },
    { "-median", 1L },
    { "+metric", 0L },
    { "-metric", 1L },
    { "+mode", 0L },
    { "-mode", 1L },
    { "+modulate", 0L },
    { "-modulate", 1L },
    { "+monitor", 0L },
    { "-monitor", 0L },
    { "+monochrome", 0L },
    { "-monochrome", 0L },
    { "+morph", 0L },
    { "-morph", 1L },
    { "+mosaic", 0L },
    { "-mosaic", 0L },
    { "+motion-blur", 0L },
    { "-motion-blur", 1L },
    { "+name", 0L },
    { "-name", 1L },
    { "+negate", 0L },
    { "-negate", 0L },
    { "+noise", 1L },
    { "-noise", 1L },
    { "+noop", 0L },
    { "-noop", 0L },
    { "+normalize", 0L },
    { "-normalize", 0L },
    { "+opaque", 0L },
    { "-opaque", 1L },
    { "+ordered-dither", 0L },
    { "-ordered-dither", 1L },
    { "+orient", 0L },
    { "-orient", 1L },
    { "+origin", 0L },
    { "-origin", 1L },
    { "+page", 0L },
    { "-page", 1L },
    { "+paint", 0L },
    { "-paint", 1L },
    { "+path", 0L },
    { "-path", 1L },
    { "+pause", 0L },
    { "-pause", 1L },
    { "+pen", 0L },
    { "-pen", 1L },
    { "+ping", 0L },
    { "-ping", 0L },
    { "+pointsize", 0L },
    { "-pointsize", 1L },
    { "+polaroid", 0L },
    { "-polaroid", 1L },
    { "+posterize", 0L },
    { "-posterize", 1L },
    { "+preview", 0L },
    { "-preview", 1L },
    { "+process", 0L },
    { "-process", 1L },
    { "+profile", 1L },
    { "-profile", 1L },
    { "+quality", 0L },
    { "-quality", 1L },
    { "+quiet", 0L },
    { "-quiet", 0L },
    { "+radial-blur", 0L },
    { "-radial-blur", 1L },
    { "+raise", 0L },
    { "-raise", 1L },
    { "+random-threshold", 0L },
    { "-random-threshold", 1L },
    { "+recolor", 0L },
    { "-recolor", 1L },
    { "+red-primary", 0L },
    { "-red-primary", 1L },
    { "+regard-warnings", 0L },
    { "-regard-warnings", 0L },
    { "+region", 0L },
    { "-region", 1L },
    { "+remote", 0L },
    { "-remote", 1L },
    { "+render", 0L },
    { "-render", 0L },
    { "+repage", 0L },
    { "-repage", 1L },
    { "+resample", 0L },
    { "-resample", 1L },
    { "+resize", 0L },
    { "-resize", 1L },
    { "+reverse", 0L },
    { "-reverse", 0L },
    { "+roll", 0L },
    { "-roll", 1L },
    { "+rotate", 0L },
    { "-rotate", 1L },
    { "+sample", 0L },
    { "-sample", 1L },
    { "+sampling-factor", 0L },
    { "-sampling-factor", 1L },
    { "+sans", 0L },
    { "-sans", 1L },
    { "+sans0", 0L },
    { "-sans0", 0L },
    { "+sans2", 2L },
    { "-sans2", 2L },
    { "+scale", 0L },
    { "-scale", 1L },
    { "+scene", 0L },
    { "-scene", 1L },
    { "+scenes", 0L },
    { "-scenes", 1L },
    { "+screen", 0L },
    { "-screen", 1L },
    { "+seed", 0L },
    { "-seed", 1L },
    { "+segment", 0L },
    { "-segment", 1L },
    { "+separate", 0L },
    { "-separate", 0L },
    { "+sepia-tone", 0L },
    { "-sepia-tone", 1L },
    { "+set", 1L },
    { "-set", 2L },
    { "+shade", 0L },
    { "-shade", 1L },
    { "+shadow", 0L },
    { "-shadow", 1L },
    { "+shared-memory", 0L },
    { "-shared-memory", 1L },
    { "+sharpen", 0L },
    { "-sharpen", 1L },
    { "+shave", 0L },
    { "-shave", 1L },
    { "+shear", 0L },
    { "-shear", 1L },
    { "+sigmoidal-contrast", 0L },
    { "-sigmoidal-contrast", 1L },
    { "+silent", 0L },
    { "-silent", 1L },
    { "+size", 0L },
    { "-size", 1L },
    { "+sketch", 0L },
    { "-sketch", 1L },
    { "+snaps", 0L },
    { "-snaps", 1L },
    { "+solarize", 0L },
    { "-solarize", 1L },
    { "+splice", 0L },
    { "-splice", 1L },
    { "+spread", 0L },
    { "-spread", 1L },
    { "+stegano", 0L },
    { "-stegano", 1L },
    { "+stereo", 0L },
    { "-stereo", 1L },
    { "+stretch", 0L },
    { "-stretch", 1L },
    { "+strip", 0L },
    { "-strip", 0L },
    { "+stroke", 0L },
    { "-stroke", 1L },
    { "+strokewidth", 0L },
    { "-strokewidth", 1L },
    { "+style", 0L },
    { "-style", 1L },
    { "+support", 0L },
    { "-support", 1L },
    { "+swap", 0L },
    { "-swap", 1L },
    { "+swirl", 0L },
    { "-swirl", 1L },
    { "+text-font", 0L },
    { "-text-font", 1L },
    { "+texture", 0L },
    { "-texture", 1L },
    { "+threshold", 0L },
    { "-threshold", 1L },
    { "+thumbnail", 0L },
    { "-thumbnail", 1L },
    { "+thumnail", 0L },
    { "-thumnail", 1L },
    { "+tile", 0L },
    { "-tile", 1L },
    { "+tile-offset", 0L },
    { "-tile-offset", 1L },
    { "+tint", 0L },
    { "-tint", 1L },
    { "+title", 0L },
    { "-title", 1L },
    { "+transform", 0L },
    { "-transform", 0L },
    { "+transparent", 0L },
    { "-transparent", 1L },
    { "+transpose", 0L },
    { "-transpose", 0L },
    { "+transverse", 0L },
    { "-transverse", 0L },
    { "+treedepth", 0L },
    { "-treedepth", 1L },
    { "+trim", 0L },
    { "-trim", 0L },
    { "+type", 0L },
    { "-type", 1L },
    { "+undercolor", 0L },
    { "-undercolor", 1L },
    { "+unique-colors", 0L },
    { "-unique-colors", 0L },
    { "+units", 0L },
    { "-units", 1L },
    { "+unsharp", 0L },
    { "-unsharp", 1L },
    { "+update", 0L },
    { "-update", 1L },
    { "+use-pixmap", 0L },
    { "-use-pixmap", 1L },
    { "+verbose", 0L },
    { "-verbose", 0L },
    { "+version", 0L },
    { "-version", 1L },
    { "+view", 0L },
    { "-view", 1L },
    { "+vignette", 0L },
    { "-vignette", 1L },
    { "+virtual-pixel", 0L },
    { "-virtual-pixel", 1L },
    { "+visual", 0L },
    { "-visual", 1L },
    { "+watermark", 0L },
    { "-watermark", 1L },
    { "+wave", 0L },
    { "-wave", 1L },
    { "+weight", 0L },
    { "-weight", 1L },
    { "+white-point", 0L },
    { "-white-point", 1L },
    { "+white-threshold", 0L },
    { "-white-threshold", 1L },
    { "+window", 0L },
    { "-window", 1L },
    { "+window-group", 0L },
    { "-window-group", 1L },
    { "+write", 0L },
    { "-write", 1L },
    { (char *) NULL, (long) 0L }
  },
  ComposeOptions[] =
  {
    { "Undefined", (long) UndefinedCompositeOp },
    { "Add", (long) AddCompositeOp },
    { "Atop", (long) AtopCompositeOp },
    { "Blend", (long) BlendCompositeOp },
    { "Bumpmap", (long) BumpmapCompositeOp },
    { "ChangeMask", (long) ChangeMaskCompositeOp },
    { "Clear", (long) ClearCompositeOp },
    { "ColorBurn", (long) ColorBurnCompositeOp },
    { "ColorDodge", (long) ColorDodgeCompositeOp },
    { "Colorize", (long) ColorizeCompositeOp },
    { "CopyBlack", (long) CopyBlackCompositeOp },
    { "CopyBlue", (long) CopyBlueCompositeOp },
    { "CopyCyan", (long) CopyCyanCompositeOp },
    { "CopyGreen", (long) CopyGreenCompositeOp },
    { "Copy", (long) CopyCompositeOp },
    { "CopyMagenta", (long) CopyMagentaCompositeOp },
    { "CopyOpacity", (long) CopyOpacityCompositeOp },
    { "CopyRed", (long) CopyRedCompositeOp },
    { "CopyYellow", (long) CopyYellowCompositeOp },
    { "Darken", (long) DarkenCompositeOp },
    { "Divide", (long) DivideCompositeOp },
    { "Dst", (long) DstCompositeOp },
    { "Difference", (long) DifferenceCompositeOp },
    { "Displace", (long) DisplaceCompositeOp },
    { "Dissolve", (long) DissolveCompositeOp },
    { "DstAtop", (long) DstAtopCompositeOp },
    { "DstIn", (long) DstInCompositeOp },
    { "DstOut", (long) DstOutCompositeOp },
    { "DstOver", (long) DstOverCompositeOp },
    { "Dst", (long) DstCompositeOp },
    { "Exclusion", (long) ExclusionCompositeOp },
    { "HardLight", (long) HardLightCompositeOp },
    { "Hue", (long) HueCompositeOp },
    { "In", (long) InCompositeOp },
    { "Lighten", (long) LightenCompositeOp },
    { "LinearLight", (long) LinearLightCompositeOp },
    { "Luminize", (long) LuminizeCompositeOp },
    { "Minus", (long) MinusCompositeOp },
    { "Modulate", (long) ModulateCompositeOp },
    { "Multiply", (long) MultiplyCompositeOp },
    { "None", (long) NoCompositeOp },
    { "Out", (long) OutCompositeOp },
    { "Overlay", (long) OverlayCompositeOp },
    { "Over", (long) OverCompositeOp },
    { "Plus", (long) PlusCompositeOp },
    { "Replace", (long) ReplaceCompositeOp },
    { "Saturate", (long) SaturateCompositeOp },
    { "Screen", (long) ScreenCompositeOp },
    { "SoftLight", (long) SoftLightCompositeOp },
    { "Src", (long) SrcCompositeOp },
    { "SrcAtop", (long) SrcAtopCompositeOp },
    { "SrcIn", (long) SrcInCompositeOp },
    { "SrcOut", (long) SrcOutCompositeOp },
    { "SrcOver", (long) SrcOverCompositeOp },
    { "Src", (long) SrcCompositeOp },
    { "Subtract", (long) SubtractCompositeOp },
    { "Threshold", (long) ThresholdCompositeOp },
    { "Xor", (long) XorCompositeOp },
    { (char *) NULL, (long) UndefinedCompositeOp }
  },
  CompressOptions[] =
  {
    { "Undefined", (long) UndefinedCompression },
    { "BZip", (long) BZipCompression },
    { "Fax", (long) FaxCompression },
    { "Group4", (long) Group4Compression },
    { "JPEG", (long) JPEGCompression },
    { "JPEG2000", (long) JPEG2000Compression },
    { "Lossless", (long) LosslessJPEGCompression },
    { "LosslessJPEG", (long) LosslessJPEGCompression },
    { "LZW", (long) LZWCompression },
    { "None", (long) NoCompression },
    { "RLE", (long) RLECompression },
    { "Zip", (long) ZipCompression },
    { "RunlengthEncoded", (long) RLECompression },
    { (char *) NULL, (long) UndefinedCompression }
  },
  ColorspaceOptions[] =
  {
    { "Undefined", (long) UndefinedColorspace },
    { "CMY", (long) CMYColorspace },
    { "CMYK", (long) CMYKColorspace },
    { "Gray", (long) GRAYColorspace },
    { "HSB", (long) HSBColorspace },
    { "HSL", (long) HSLColorspace },
    { "HWB", (long) HWBColorspace },
    { "Lab", (long) LabColorspace },
    { "Log", (long) LogColorspace },
    { "OHTA", (long) OHTAColorspace },
    { "Rec601Luma", (long) Rec601LumaColorspace },
    { "Rec601YCbCr", (long) Rec601YCbCrColorspace },
    { "Rec709Luma", (long) Rec709LumaColorspace },
    { "Rec709YCbCr", (long) Rec709YCbCrColorspace },
    { "RGB", (long) RGBColorspace },
    { "sRGB", (long) sRGBColorspace },
    { "Transparent", (long) TransparentColorspace },
    { "XYZ", (long) XYZColorspace },
    { "YCbCr", (long) YCbCrColorspace },
    { "YCC", (long) YCCColorspace },
    { "YIQ", (long) YIQColorspace },
    { "YPbPr", (long) YPbPrColorspace },
    { "YUV", (long) YUVColorspace },
    { (char *) NULL, (long) UndefinedColorspace }
  },
  DataTypeOptions[] =
  {
    { "Undefined", (long) UndefinedData },
    { "Byte", (long) ByteData },
    { "Long", (long) LongData },
    { "Short", (long) ShortData },
    { "String", (long) StringData },
    { (char *) NULL, (long) UndefinedData }
  },
  DecorateOptions[] =
  {
    { "Undefined", (long) UndefinedDecoration },
    { "LineThrough", (long) LineThroughDecoration },
    { "None", (long) NoDecoration },
    { "Overline", (long) OverlineDecoration },
    { "Underline", (long) UnderlineDecoration },
    { (char *) NULL, (long) UndefinedDecoration }
  },
  DisposeOptions[] =
  {
    { "Background", (long) BackgroundDispose },
    { "None", (long) NoneDispose },
    { "Previous", (long) PreviousDispose },
    { "Undefined", (long) UndefinedDispose },
    { "0", (long) UndefinedDispose },
    { "1", (long) NoneDispose },
    { "2", (long) BackgroundDispose },
    { "3", (long) PreviousDispose },
    { (char *) NULL, (long) UndefinedDispose }
  },
  DistortOptions[] =
  {
    { "Undefined", (long) UndefinedDistortion },
    { "Affine", (long) AffineDistortion },
    { "AffineProjection", (long) AffineProjectionDistortion },
    { "Bilinear", (long) BilinearDistortion },
    { "Perspective", (long) PerspectiveDistortion },
    { "PerspectiveProjection", (long) PerspectiveProjectionDistortion },
    { "ScaleRotateTranslate", (long) ScaleRotateTranslateDistortion },
    { "SRT", (long) ScaleRotateTranslateDistortion },
    { "Arc", (long) ArcDistortion },
    { (char *) NULL, (long) UndefinedDistortion }
  },
  EndianOptions[] =
  {
    { "Undefined", (long) UndefinedEndian },
    { "LSB", (long) LSBEndian },
    { "MSB", (long) MSBEndian },
    { (char *) NULL, (long) UndefinedEndian }
  },
  EvaluateOptions[] =
  {
    { "Undefined", (long) UndefinedEvaluateOperator },
    { "Add", (long) AddEvaluateOperator },
    { "And", (long) AndEvaluateOperator },
    { "Divide", (long) DivideEvaluateOperator },
    { "LeftShift", (long) LeftShiftEvaluateOperator },
    { "Max", (long) MaxEvaluateOperator },
    { "Min", (long) MinEvaluateOperator },
    { "Multiply", (long) MultiplyEvaluateOperator },
    { "Or", (long) OrEvaluateOperator },
    { "RightShift", (long) RightShiftEvaluateOperator },
    { "Set", (long) SetEvaluateOperator },
    { "Subtract", (long) SubtractEvaluateOperator },
    { "Xor", (long) XorEvaluateOperator },
    { (char *) NULL, (long) UndefinedEvaluateOperator }
  },
  FillRuleOptions[] =
  {
    { "Undefined", (long) UndefinedRule },
    { "Evenodd", (long) EvenOddRule },
    { "NonZero", (long) NonZeroRule },
    { (char *) NULL, (long) UndefinedRule }
  },
  FilterOptions[] =
  {
    { "Undefined", (long) UndefinedFilter },
    { "Bessel", (long) BesselFilter },
    { "Blackman", (long) BlackmanFilter },
    { "Box", (long) BoxFilter },
    { "Catrom", (long) CatromFilter },
    { "Cubic", (long) CubicFilter },
    { "Gaussian", (long) GaussianFilter },
    { "Hamming", (long) HammingFilter },
    { "Hanning", (long) HanningFilter },
    { "Hermite", (long) HermiteFilter },
    { "Lanczos", (long) LanczosFilter },
    { "Mitchell", (long) MitchellFilter },
    { "Point", (long) PointFilter },
    { "Quadratic", (long) QuadraticFilter },
    { "Sinc", (long) SincFilter },
    { "Triangle", (long) TriangleFilter },
    { (char *) NULL, (long) UndefinedFilter }
  },
  GravityOptions[] =
  {
    { "Undefined", (long) UndefinedGravity },
    { "None", (long) UndefinedGravity },
    { "Center", (long) CenterGravity },
    { "East", (long) EastGravity },
    { "Forget", (long) ForgetGravity },
    { "NorthEast", (long) NorthEastGravity },
    { "North", (long) NorthGravity },
    { "NorthWest", (long) NorthWestGravity },
    { "SouthEast", (long) SouthEastGravity },
    { "South", (long) SouthGravity },
    { "SouthWest", (long) SouthWestGravity },
    { "West", (long) WestGravity },
    { "Static", (long) StaticGravity },
    { (char *) NULL, UndefinedGravity }
  },
  IntentOptions[] =
  {
    { "Undefined", (long) UndefinedIntent },
    { "Absolute", (long) AbsoluteIntent },
    { "Perceptual", (long) PerceptualIntent },
    { "Relative", (long) RelativeIntent },
    { "Saturation", (long) SaturationIntent },
    { (char *) NULL, (long) UndefinedIntent }
  },
  InterlaceOptions[] =
  {
    { "Undefined", (long) UndefinedInterlace },
    { "Line", (long) LineInterlace },
    { "None", (long) NoInterlace },
    { "Plane", (long) PlaneInterlace },
    { "Partition", (long) PartitionInterlace },
    { "GIF", (long) GIFInterlace },
    { "JPEG", (long) JPEGInterlace },
    { "PNG", (long) PNGInterlace },
    { (char *) NULL, (long) UndefinedInterlace }
  },
  InterpolateOptions[] =
  {
    { "Undefined", (long) UndefinedInterpolatePixel },
    { "Average", (long) AverageInterpolatePixel },
    { "Bicubic", (long) BicubicInterpolatePixel },
    { "Bilinear", (long) BilinearInterpolatePixel },
    { "filter", (long) FilterInterpolatePixel },
    { "Integer", (long) IntegerInterpolatePixel },
    { "Mesh", (long) MeshInterpolatePixel },
    { "NearestNeighbor", (long) NearestNeighborInterpolatePixel },
    { "Spline", (long) SplineInterpolatePixel },
    { (char *) NULL, (long) UndefinedInterpolatePixel }
  },
  LayersOptions[] =
  {
    { "Undefined", (long) UndefinedLayer },
    { "Coalesce", (long) CoalesceLayer },
    { "CompareAny", (long) CompareAnyLayer },
    { "CompareClear", (long) CompareClearLayer },
    { "CompareOverlay", (long) CompareOverlayLayer },
    { "Dispose", (long) DisposeLayer },
    { "Optimize", (long) OptimizeLayer },
    { "OptimizeFrame", (long) OptimizeImageLayer },
    { "OptimizePlus", (long) OptimizePlusLayer },
    { "OptimizeTransparency", (long) OptimizeTransLayer },
    { "RemoveDups", (long) RemoveDupsLayer },
    { "RemoveZero", (long) RemoveZeroLayer },
    { "Composite", (long) CompositeLayer },
    { (char *) NULL, (long) UndefinedLayer }
  },
  LineCapOptions[] =
  {
    { "Undefined", (long) UndefinedCap },
    { "Butt", (long) ButtCap },
    { "Round", (long) RoundCap },
    { "Square", (long) SquareCap },
    { (char *) NULL, (long) UndefinedCap }
  },
  LineJoinOptions[] =
  {
    { "Undefined", (long) UndefinedJoin },
    { "Bevel", (long) BevelJoin },
    { "Miter", (long) MiterJoin },
    { "Round", (long) RoundJoin },
    { (char *) NULL, (long) UndefinedJoin }
  },
  ListOptions[] =
  {
    { "Undefined", (long) MagickUndefinedOptions },
    { "Align", (long) MagickAlignOptions },
    { "Alpha", (long) MagickAlphaOptions },
    { "Boolean", (long) MagickBooleanOptions },
    { "Channel", (long) MagickChannelOptions },
    { "Class", (long) MagickClassOptions },
    { "ClipPath", (long) MagickClipPathOptions },
    { "Colorspace", (long) MagickColorspaceOptions },
    { "Compose", (long) MagickComposeOptions },
    { "Command", (long) MagickCommandOptions },
    { "Compress", (long) MagickCompressOptions },
    { "DataType", (long) MagickDataTypeOptions },
    { "Debug", (long) MagickDebugOptions },
    { "Decoration", (long) MagickDecorateOptions },
    { "Dispose", (long) MagickDisposeOptions },
    { "Distort", (long) MagickDistortOptions },
    { "Endian", (long) MagickEndianOptions },
    { "Evaluate", (long) MagickEvaluateOptions },
    { "FillRule", (long) MagickFillRuleOptions },
    { "Filter", (long) MagickFilterOptions },
    { "Font", (long) MagickFontOptions },
    { "Gravity", (long) MagickGravityOptions },
    { "Intent", (long) MagickIntentOptions },
    { "Interlace", (long) MagickInterlaceOptions },
    { "Interpolate", (long) MagickInterpolateOptions },
    { "Layers", (long) MagickLayersOptions },
    { "LineCap", (long) MagickLineCapOptions },
    { "LineJoin", (long) MagickLineJoinOptions },
    { "List", (long) MagickListOptions },
    { "LogEvent", (long) MagickLogEventOptions },
    { "Metric", (long) MagickMetricOptions },
    { "Method", (long) MagickMethodOptions },
    { "Mime", (long) MagickMimeOptions },
    { "Mode", (long) MagickModeOptions },
    { "Mogrify", (long) MagickMogrifyOptions },
    { "Noise", (long) MagickNoiseOptions },
    { "Orientation", (long) MagickOrientationOptions },
    { "Preview", (long) MagickPreviewOptions },
    { "Primitive", (long) MagickPrimitiveOptions },
    { "QuantumFormat", (long) MagickQuantumFormatOptions },
    { "Resolution", (long) MagickResolutionOptions },
    { "Resource", (long) MagickResourceOptions },
    { "Storage", (long) MagickStorageOptions },
    { "Stretch", (long) MagickStretchOptions },
    { "Style", (long) MagickStyleOptions },
    { "VirtualPixel", (long) MagickVirtualPixelOptions },
    { "Coder", (long) MagickCoderOptions },
    { "Color", (long) MagickColorOptions },
    { "Configure", (long) MagickConfigureOptions },
    { "Delegate", (long) MagickDelegateOptions },
    { "Format", (long) MagickFormatOptions },
    { "Locale", (long) MagickLocaleOptions },
    { "Log", (long) MagickLogOptions },
    { "Magic", (long) MagickMagicOptions },
    { "Module", (long) MagickModuleOptions },
    { "Threshold", (long) MagickThresholdOptions },
    { "Type", (long) MagickTypeOptions },
    { (char *) NULL, (long) MagickUndefinedOptions }
  },
  LogEventOptions[] =
  {
    { "Undefined", (long) UndefinedEvents },
    { "All", (long) (AllEvents &~ TraceEvent) },
    { "Annotate", (long) AnnotateEvent },
    { "Blob", (long) BlobEvent },
    { "Cache", (long) CacheEvent },
    { "Coder", (long) CoderEvent },
    { "Configure", (long) ConfigureEvent },
    { "Deprecate", (long) DeprecateEvent },
    { "Draw", (long) DrawEvent },
    { "Exception", (long) ExceptionEvent },
    { "Locale", (long) LocaleEvent },
    { "Module", (long) ModuleEvent },
    { "None", (long) NoEvents },
    { "Resource", (long) ResourceEvent },
    { "Trace", (long) TraceEvent },
    { "Transform", (long) TransformEvent },
    { "User", (long) UserEvent },
    { "Wand", (long) WandEvent },
    { "X11", (long) X11Event },
    { (char *) NULL, (long) UndefinedEvents }
  },
  MetricOptions[] =
  {
    { "Undefined", (long) UndefinedMetric },
    { "AE", (long) AbsoluteErrorMetric },
    { "MAE", (long) MeanAbsoluteErrorMetric },
    { "MEPP", (long) MeanErrorPerPixelMetric },
    { "MSE", (long) MeanSquaredErrorMetric },
    { "PAE", (long) PeakAbsoluteErrorMetric },
    { "PSNR", (long) PeakSignalToNoiseRatioMetric },
    { "RMSE", (long) RootMeanSquaredErrorMetric },
    { (char *) NULL, (long) UndefinedMetric }
  },
  MethodOptions[] =
  {
    { "Undefined", (long) UndefinedMethod },
    { "FillToBorder", (long) FillToBorderMethod },
    { "Floodfill", (long) FloodfillMethod },
    { "Point", (long) PointMethod },
    { "Replace", (long) ReplaceMethod },
    { "Reset", (long) ResetMethod },
    { (char *) NULL, (long) UndefinedMethod }
  },
  ModeOptions[] =
  {
    { "Undefined", (long) UndefinedMode },
    { "Concatenate", (long) ConcatenateMode },
    { "Frame", (long) FrameMode },
    { "Unframe", (long) UnframeMode },
    { (char *) NULL, (long) UndefinedMode }
  },
  MogrifyOptions[] =
  {
    { "append", MagickTrue },
    { "average", MagickTrue },
    { "clut", MagickTrue },
    { "coalesce", MagickTrue },
    { "combine", MagickTrue },
    { "composite", MagickTrue },
    { "crop", MagickTrue },
    { "debug", MagickTrue },
    { "deconstruct", MagickTrue },
    { "delete", MagickTrue },
    { "flatten", MagickTrue },
    { "fx", MagickTrue },
    { "insert", MagickTrue },
    { "limit", MagickTrue },
    { "map", MagickTrue },
    { "morph", MagickTrue },
    { "mosaic", MagickTrue },
    { "optimize", MagickTrue },
    { "process", MagickTrue },
    { "quiet", MagickTrue },
    { "separate", MagickTrue },
    { "scene", MagickTrue },
    { "swap", MagickTrue },
    { "write", MagickTrue },
    { (char *) NULL, MagickFalse }
  },
  NoiseOptions[] =
  {
    { "Undefined", (long) UndefinedNoise },
    { "Gaussian", (long) (long) GaussianNoise },
    { "Impulse", (long) ImpulseNoise },
    { "Laplacian", (long) LaplacianNoise },
    { "Multiplicative", (long) MultiplicativeGaussianNoise },
    { "Poisson", (long) PoissonNoise },
    { "Random", (long) RandomNoise },
    { "Uniform", (long) UniformNoise },
    { (char *) NULL, (long) UndefinedNoise }
  },
  OrientationOptions[] =
  {
    { "Undefined", (long) UndefinedOrientation },
    { "TopLeft", (long) TopLeftOrientation },
    { "TopRight", (long) TopRightOrientation },
    { "BottomRight", (long) BottomRightOrientation },
    { "BottomLeft", (long) BottomLeftOrientation },
    { "LeftTop", (long) LeftTopOrientation },
    { "RightTop", (long) RightTopOrientation },
    { "RightBottom", (long) RightBottomOrientation },
    { "LeftBottom", (long) LeftBottomOrientation }
  },
  PreviewOptions[] =
  {
    { "Undefined", (long) UndefinedPreview },
    { "AddNoise", (long) AddNoisePreview },
    { "Blur", (long) BlurPreview },
    { "Brightness", (long) BrightnessPreview },
    { "Charcoal", (long) CharcoalDrawingPreview },
    { "Despeckle", (long) DespecklePreview },
    { "Dull", (long) DullPreview },
    { "EdgeDetect", (long) EdgeDetectPreview },
    { "Gamma", (long) GammaPreview },
    { "Grayscale", (long) GrayscalePreview },
    { "Hue", (long) HuePreview },
    { "Implode", (long) ImplodePreview },
    { "JPEG", (long) JPEGPreview },
    { "OilPaint", (long) OilPaintPreview },
    { "Quantize", (long) QuantizePreview },
    { "Raise", (long) RaisePreview },
    { "ReduceNoise", (long) ReduceNoisePreview },
    { "Roll", (long) RollPreview },
    { "Rotate", (long) RotatePreview },
    { "Saturation", (long) SaturationPreview },
    { "Segment", (long) SegmentPreview },
    { "Shade", (long) ShadePreview },
    { "Sharpen", (long) SharpenPreview },
    { "Shear", (long) ShearPreview },
    { "Solarize", (long) SolarizePreview },
    { "Spiff", (long) SpiffPreview },
    { "Spread", (long) SpreadPreview },
    { "Swirl", (long) SwirlPreview },
    { "Threshold", (long) ThresholdPreview },
    { "Wave", (long) WavePreview },
    { (char *) NULL, (long) UndefinedPreview }
  },
  PrimitiveOptions[] =
  {
    { "Undefined", (long) UndefinedPrimitive },
    { "Arc", (long) ArcPrimitive },
    { "Bezier", (long) BezierPrimitive },
    { "Circle", (long) CirclePrimitive },
    { "Color", (long) ColorPrimitive },
    { "Ellipse", (long) EllipsePrimitive },
    { "Image", (long) ImagePrimitive },
    { "Line", (long) LinePrimitive },
    { "Matte", (long) MattePrimitive },
    { "Path", (long) PathPrimitive },
    { "Point", (long) PointPrimitive },
    { "Polygon", (long) PolygonPrimitive },
    { "Polyline", (long) PolylinePrimitive },
    { "Rectangle", (long) RectanglePrimitive },
    { "roundRectangle", (long) RoundRectanglePrimitive },
    { "Text", (long) TextPrimitive },
    { (char *) NULL, (long) UndefinedPrimitive }
  },
  QuantumFormatOptions[] =
  {
    { "Undefined", (long) UndefinedQuantumFormat },
    { "FloatingPoint", (long) FloatingPointQuantumFormat },
    { "Signed", (long) SignedQuantumFormat },
    { "Unsigned", (long) UnsignedQuantumFormat },
    { (char *) NULL, (long) FloatingPointQuantumFormat }
  },
  ResolutionOptions[] =
  {
    { "Undefined", (long) UndefinedResolution },
    { "PixelsPerInch", (long) PixelsPerInchResolution },
    { "PixelsPerCentimeter", (long) PixelsPerCentimeterResolution },
    { (char *) NULL, (long) UndefinedResolution }
  },
  ResourceOptions[] =
  {
    { "Undefined", (long) UndefinedResource },
    { "Area", (long) AreaResource },
    { "Disk", (long) DiskResource },
    { "File", (long) FileResource },
    { "Map", (long) MapResource },
    { "Memory", (long) MemoryResource },
    { (char *) NULL, (long) UndefinedResource }
  },
  StorageOptions[] =
  {
    { "Undefined", (long) UndefinedPixel },
    { "Char", (long) CharPixel },
    { "Double", (long) DoublePixel },
    { "Float", (long) FloatPixel },
    { "Integer", (long) IntegerPixel },
    { "Long", (long) LongPixel },
    { "Quantum", (long) QuantumPixel },
    { "Short", (long) ShortPixel },
    { (char *) NULL, (long) UndefinedResource }
  },
  StretchOptions[] =
  {
    { "Undefined", (long) UndefinedStretch },
    { "Any", (long) AnyStretch },
    { "Condensed", (long) CondensedStretch },
    { "Expanded", (long) ExpandedStretch },
    { "ExtraCondensed", (long) ExtraCondensedStretch },
    { "ExtraExpanded", (long) ExtraExpandedStretch },
    { "Normal", (long) NormalStretch },
    { "SemiCondensed", (long) SemiCondensedStretch },
    { "SemiExpanded", (long) SemiExpandedStretch },
    { "UltraCondensed", (long) UltraCondensedStretch },
    { "UltraExpanded", (long) UltraExpandedStretch },
    { (char *) NULL, (long) UndefinedStretch }
  },
  StyleOptions[] =
  {
    { "Undefined", (long) UndefinedStyle },
    { "Any", (long) AnyStyle },
    { "Italic", (long) ItalicStyle },
    { "Normal", (long) NormalStyle },
    { "Oblique", (long) ObliqueStyle },
    { (char *) NULL, (long) UndefinedStyle }
  },
  TypeOptions[] =
  {
    { "Undefined", (long) UndefinedType },
    { "Bilevel", (long) BilevelType },
    { "ColorSeparation", (long) ColorSeparationType },
    { "ColorSeparationMatte", (long) ColorSeparationMatteType },
    { "Grayscale", (long) GrayscaleType },
    { "GrayscaleMatte", (long) GrayscaleMatteType },
    { "Optimize", (long) OptimizeType },
    { "Palette", (long) PaletteType },
    { "PaletteBilevelMatte", (long) PaletteBilevelMatteType },
    { "PaletteMatte", (long) PaletteMatteType },
    { "TrueColorMatte", (long) TrueColorMatteType },
    { "TrueColor", (long) TrueColorType },
    { (char *) NULL, (long) UndefinedType }
  },
  VirtualPixelOptions[] =
  {
    { "Undefined", (long) UndefinedVirtualPixelMethod },
    { "Background", (long) BackgroundVirtualPixelMethod },
    { "Black", (long) BlackVirtualPixelMethod },
    { "Constant", (long) BackgroundVirtualPixelMethod },
    { "Dither", (long) DitherVirtualPixelMethod },
    { "Edge", (long) EdgeVirtualPixelMethod },
    { "Gray", (long) GrayVirtualPixelMethod },
    { "Mirror", (long) MirrorVirtualPixelMethod },
    { "Random", (long) RandomVirtualPixelMethod },
    { "Tile", (long) TileVirtualPixelMethod },
    { "Transparent", (long) TransparentVirtualPixelMethod },
    { "White", (long) WhiteVirtualPixelMethod },
    { (char *) NULL, (long) UndefinedVirtualPixelMethod }
  };

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e I m a g e O p t i o n s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneImageOptions() clones one or more image options.
%
%  The format of the CloneImageOptions method is:
%
%      MagickBooleanType CloneImageOptions(ImageInfo *image_info,
%        const ImageInfo *clone_info)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o clone_info: The clone image info.
%
*/
MagickExport MagickBooleanType CloneImageOptions(ImageInfo *image_info,
  const ImageInfo *clone_info)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(clone_info != (const ImageInfo *) NULL);
  assert(clone_info->signature == MagickSignature);
  if (clone_info->options != (void *) NULL)
    image_info->options=CloneSplayTree((SplayTreeInfo *) clone_info->options,
      (void *(*)(void *)) ConstantString,(void *(*)(void *)) ConstantString);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e f i n e I m a g e O p t i o n                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DefineImageOption() associates a key/value pair with an image option.
%
%  The format of the DefineImageOption method is:
%
%      MagickBooleanType DefineImageOption(ImageInfo *image_info,
%        const char *option)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o option: The image option.
%
*/
MagickExport MagickBooleanType DefineImageOption(ImageInfo *image_info,
  const char *option)
{
  char
    key[MaxTextExtent],
    value[MaxTextExtent];

  register char
    *p;

  assert(image_info != (ImageInfo *) NULL);
  assert(option != (const char *) NULL);
  (void) CopyMagickString(key,option,MaxTextExtent);
  for (p=key; *p != '\0'; p++)
    if (*p == '=')
      break;
  *value='\0';
  if (*p == '=')
    (void) CopyMagickString(value,p+1,MaxTextExtent);
  *p='\0';
  return(SetImageOption(image_info,key,value));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e l e t e I m a g e O p t i o n                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DeleteImageOption() deletes an key from the image map.
%
%  The format of the DeleteImageOption method is:
%
%      MagickBooleanType DeleteImageOption(ImageInfo *image_info,
%        const char *key)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o option: The image option.
%
*/
MagickExport MagickBooleanType DeleteImageOption(ImageInfo *image_info,
  const char *option)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options == (void *) NULL)
    return(MagickFalse);
  return(DeleteNodeFromSplayTree((SplayTreeInfo *) image_info->options,option));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y I m a g e O p t i o n s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyImageOptions() releases memory associated with image option values.
%
%  The format of the DestroyDefines method is:
%
%      void DestroyImageOptions(ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
*/
MagickExport void DestroyImageOptions(ImageInfo *image_info)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options != (void *) NULL)
    image_info->options=DestroySplayTree((SplayTreeInfo *) image_info->options);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e O p t i o n                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageOption() gets a value associated with an image option.
%
%  The format of the GetImageOption method is:
%
%      const char *GetImageOption(const ImageInfo *image_info,
%        const char *key)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o key: The key.
%
*/
MagickExport const char *GetImageOption(const ImageInfo *image_info,
  const char *key)
{
  const char
    *option;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options == (void *) NULL)
    return((const char *) NULL);
  option=(const char *) GetValueFromSplayTree((SplayTreeInfo *)
    image_info->options,key);
  return(option);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k O p t i o n s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickOptions() returns a list of values.
%
%  The format of the GetMagickOptions method is:
%
%      const char **GetMagickOptions(const MagickOption value)
%
%  A description of each parameter follows:
%
%    o value: The value.
%
*/

static const OptionInfo *GetOptionInfo(const MagickOption option)
{
  switch (option)
  {
    case MagickAlignOptions: return(AlignOptions);
    case MagickAlphaOptions: return(AlphaOptions);
    case MagickBooleanOptions: return(BooleanOptions);
    case MagickChannelOptions: return(ChannelOptions);
    case MagickClassOptions: return(ClassOptions);
    case MagickClipPathOptions: return(ClipPathOptions);
    case MagickColorspaceOptions: return(ColorspaceOptions);
    case MagickCommandOptions: return(CommandOptions);
    case MagickComposeOptions: return(ComposeOptions);
    case MagickCompressOptions: return(CompressOptions);
    case MagickDataTypeOptions: return(DataTypeOptions);
    case MagickDebugOptions: return(LogEventOptions);
    case MagickDecorateOptions: return(DecorateOptions);
    case MagickDisposeOptions: return(DisposeOptions);
    case MagickDistortOptions: return(DistortOptions);
    case MagickEndianOptions: return(EndianOptions);
    case MagickEvaluateOptions: return(EvaluateOptions);
    case MagickFillRuleOptions: return(FillRuleOptions);
    case MagickFilterOptions: return(FilterOptions);
    case MagickGravityOptions: return(GravityOptions);
    case MagickIntentOptions: return(IntentOptions);
    case MagickInterlaceOptions: return(InterlaceOptions);
    case MagickInterpolateOptions: return(InterpolateOptions);
    case MagickLayersOptions: return(LayersOptions);
    case MagickLineCapOptions: return(LineCapOptions);
    case MagickLineJoinOptions: return(LineJoinOptions);
    case MagickListOptions: return(ListOptions);
    case MagickLogEventOptions: return(LogEventOptions);
    case MagickMetricOptions: return(MetricOptions);
    case MagickMethodOptions: return(MethodOptions);
    case MagickModeOptions: return(ModeOptions);
    case MagickMogrifyOptions: return(MogrifyOptions);
    case MagickNoiseOptions: return(NoiseOptions);
    case MagickOrientationOptions: return(OrientationOptions);
    case MagickPreviewOptions: return(PreviewOptions);
    case MagickPrimitiveOptions: return(PrimitiveOptions);
    case MagickQuantumFormatOptions: return(QuantumFormatOptions);
    case MagickResolutionOptions: return(ResolutionOptions);
    case MagickResourceOptions: return(ResourceOptions);
    case MagickStorageOptions: return(StorageOptions);
    case MagickStretchOptions: return(StretchOptions);
    case MagickStyleOptions: return(StyleOptions);
    case MagickTypeOptions: return(TypeOptions);
    case MagickVirtualPixelOptions: return(VirtualPixelOptions);
    default: break;
  }
  return((const OptionInfo *) NULL);
}

MagickExport char **GetMagickOptions(const MagickOption value)
{
  char
    **values;

  const OptionInfo
    *option_info;

  register long
    i;

  option_info=GetOptionInfo(value);
  if (option_info == (const OptionInfo *) NULL)
    return((char **) NULL);
  for (i=0; option_info[i].mnemonic != (const char *) NULL; i++);
  values=(char **) AcquireQuantumMemory((size_t) i+1UL,sizeof(*values));
  if (values == (char **) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  for (i=0; option_info[i].mnemonic != (const char *) NULL; i++)
    values[i]=AcquireString(option_info[i].mnemonic);
  values[i]=(char *) NULL;
  return(values);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t N e x t I m a g e O p t i o n                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetNextImageOption() gets the next image option value.
%
%  The format of the GetNextImageOption method is:
%
%      char *GetNextImageOption(const ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
*/
MagickExport char *GetNextImageOption(const ImageInfo *image_info)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options == (void *) NULL)
    return((char *) NULL);
  return((char *) GetNextKeyInSplayTree((SplayTreeInfo *) image_info->options));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     I s M a g i c k O p t i o n                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsMagickOption() returns MagickTrue if the option begins with a - or + and
%  the first character that follows is alphanumeric.
%
%  The format of the IsMagickOption method is:
%
%      MagickBooleanType IsMagickOption(const char *option)
%
%  A description of each parameter follows:
%
%    o option: The option.
%
*/
MagickExport MagickBooleanType IsMagickOption(const char *option)
{
  assert(option != (const char *) NULL);
  if ((*option != '-') && (*option != '+'))
    return(MagickFalse);
  if (strlen(option) == 1)
    return(MagickFalse);
  option++;
  if (isalpha((int) ((unsigned char) *option)) == 0)
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k O p t i o n T o M n e m o n i c                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickOptionToMnemonic() returns an enumerated value as a mnemonic.
%
%  The format of the MagickOptionToMnemonic method is:
%
%      const char *MagickOptionToMnemonic(const MagickOption option,
%        const long type)
%
%  A description of each parameter follows:
%
%    o option: The option.
%
%    o type: one or more values separated by commas.
%
*/
MagickExport const char *MagickOptionToMnemonic(const MagickOption option,
  const long type)
{
  const OptionInfo
    *option_info;

  register long
    i;

  option_info=GetOptionInfo(option);
  if (option_info == (const OptionInfo *) NULL)
    return((const char *) NULL);
  for (i=0; option_info[i].mnemonic != (const char *) NULL; i++)
    if (type == option_info[i].type)
      break;
  if (option_info[i].mnemonic == (const char *) NULL)
    return("undefined");
  return(option_info[i].mnemonic);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L i s t M a g i c k O p t i o n s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListMagickOptions() lists the contents of enumerated option type(s).
%
%  The format of the ListMagickOptions method is:
%
%      MagickBooleanType ListMagickOptions(FILE *file,const MagickOption option,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o file:  list options to this file handle.
%
%    o option:  list these options.
%
%    o exception:  return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType ListMagickOptions(FILE *file,
  const MagickOption option,ExceptionInfo *magick_unused(exception))
{
  const OptionInfo
    *option_info;

  register long
    i;

  if (file == (FILE *) NULL)
    file=stdout;
  option_info=GetOptionInfo(option);
  if (option_info == (const OptionInfo *) NULL)
    return(MagickFalse);
  for (i=0; option_info[i].mnemonic != (char *) NULL; i++)
  {
    if ((i == 0) && (strcmp(option_info[i].mnemonic,"Undefined") == 0))
      continue;
    (void) fprintf(file,"%s\n",option_info[i].mnemonic);
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e C h a n n e l O p t i o n                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseChannelOption() parses a string and returns an enumerated channel
%  type(s).
%
%  The format of the ParseChannelOption method is:
%
%      long ParseChannelOption(const char *channels)
%
%  A description of each parameter follows:
%
%    o options: One or more values separated by commas.
%
*/
MagickExport long ParseChannelOption(const char *channels)
{
  long
    channel;

  register long
    i;

  channel=ParseMagickOption(MagickChannelOptions,MagickTrue,channels);
  if (channel >= 0)
    return(channel);
  channel=0;
  for (i=0; i < (long) strlen(channels); i++)
  {
    switch (channels[i])
    {
      case 'A':
      case 'a':
      {
        channel|=OpacityChannel;
        break;
      }
      case 'B':
      case 'b':
      {
        channel|=BlueChannel;
        break;
      }
      case 'C':
      case 'c':
      {
        channel|=CyanChannel;
        break;
      }
      case 'g':
      case 'G':
      {
        channel|=GreenChannel;
        break;
      }
      case 'I':
      case 'i':
      {
        channel|=IndexChannel;
        break;
      }
      case 'K':
      case 'k':
      {
        channel|=BlackChannel;
        break;
      }
      case 'M':
      case 'm':
      {
        channel|=MagentaChannel;
        break;
      }
      case 'o':
      case 'O':
      {
        channel|=OpacityChannel;
        break;
      }
      case 'R':
      case 'r':
      {
        channel|=RedChannel;
        break;
      }
      case 'Y':
      case 'y':
      {
        channel|=YellowChannel;
        break;
      }
      default:
        return(-1);
    }
  }
  return(channel);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e M a g i c k O p t i o n                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseMagickOption() parses a string and returns an enumerated option type(s).
%
%  The format of the ParseMagickOption method is:
%
%      long ParseMagickOption(const MagickOption option,
%        const MagickBooleanType list,const char *options)
%
%  A description of each parameter follows:
%
%    o option: Index to the option table to lookup
%
%    o list: A option other than zero permits more than one option separated by
%      commas.
%
%    o options: One or more options separated by commas.
%
*/
MagickExport long ParseMagickOption(const MagickOption option,
  const MagickBooleanType list,const char *options)
{
  char
    token[MaxTextExtent];

  const OptionInfo
    *option_info;

  long
    option_types;

  MagickBooleanType
    negate;

  register char
    *q;

  register const char
    *p;

  register long
    i;

  option_info=GetOptionInfo(option);
  if (option_info == (const OptionInfo *) NULL)
    return(-1);
  option_types=0;
  for (p=options; p != (char *) NULL; p=strchr(p,','))
  {
    while (((isspace((int) ((unsigned char) *p)) != 0) || (*p == ',')) &&
           (*p != '\0'))
      p++;
    negate=(*p == '!') ? MagickTrue : MagickFalse;
    if (negate != MagickFalse)
      p++;
    q=token;
    while (((isspace((int) ((unsigned char) *p)) == 0) && (*p != ',')) &&
           (*p != '\0'))
    {
      if ((q-token) >= MaxTextExtent)
        break;
      *q++=(*p++);
    }
    *q='\0';
    for (i=0; option_info[i].mnemonic != (char *) NULL; i++)
      if (LocaleCompare(token,option_info[i].mnemonic) == 0)
        {
          if (*token == '!')
            option_types=option_types &~ option_info[i].type;
          else
            option_types=option_types | option_info[i].type;
          break;
        }
    if ((option_info[i].mnemonic == (char *) NULL) &&
        ((strchr(token+1,'-') != (char *) NULL) || 
         (strchr(token+1,'_') != (char *) NULL)))
      {
        while ((q=strchr(token+1,'-')) != (char *) NULL)
          (void) CopyMagickString(q,q+1,MaxTextExtent-strlen(q));
        while ((q=strchr(token+1,'_')) != (char *) NULL)
          (void) CopyMagickString(q,q+1,MaxTextExtent-strlen(q));
        for (i=0; option_info[i].mnemonic != (char *) NULL; i++)
          if (LocaleCompare(token,option_info[i].mnemonic) == 0)
            {
              if (*token == '!')
                option_types=option_types &~ option_info[i].type;
              else
                option_types=option_types | option_info[i].type;
              break;
            }
      }
    if (option_info[i].mnemonic == (char *) NULL)
      return(-1);
    if (list == MagickFalse)
      break;
  }
  return(option_types);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e m o v e I m a g e O p t i o n                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RemoveImageOption() removes an option from the image and returns its value.
%
%  The format of the RemoveImageOption method is:
%
%      char *RemoveImageOption(ImageInfo *image_info,const char *option)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o option: The image option.
%
*/
MagickExport char *RemoveImageOption(ImageInfo *image_info,const char *option)
{
  char
    *value;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options == (void *) NULL)
    return((char *) NULL);
  value=(char *) RemoveNodeFromSplayTree((SplayTreeInfo *)
    image_info->options,option);
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s e t I m a g e O p t i o n I t e r a t o r                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetImageOptionIterator() resets the image_info values iterator.  Use it
%  in conjunction with GetNextImageOption() to iterate over all the values
%  associated with an image option.
%
%  The format of the ResetImageOptionIterator method is:
%
%      ResetImageOptionIterator(ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
*/
MagickExport void ResetImageOptionIterator(const ImageInfo *image_info)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options == (void *) NULL)
    return;
  ResetSplayTreeIterator((SplayTreeInfo *) image_info->options);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e O p t i o n                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageOption() associates an value with an image option.
%
%  The format of the SetImageOption method is:
%
%      MagickBooleanType SetImageOption(ImageInfo *image_info,
%        const char *option,const char *value)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o option: The image option.
%
%    o values: The image option values.
%
*/
MagickExport MagickBooleanType SetImageOption(ImageInfo *image_info,
  const char *option,const char *value)
{
  MagickBooleanType
    status;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (LocaleCompare(option,"size") == 0)
    (void) CloneString(&image_info->size,value);
  if (image_info->options == (void *) NULL)
    image_info->options=NewSplayTree(CompareSplayTreeString,
      RelinquishMagickMemory,RelinquishMagickMemory);
  status=AddValueToSplayTree((SplayTreeInfo *) image_info->options,
    ConstantString(option),ConstantString(value));
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S y n c I m a g e O p t i o n s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncImageOptions() set options in the ImageInfo structure as attributes in
%  the image.
%
%  The format of the SyncImageOptions method is:
%
%      MagickBooleanType SyncImageOptions(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o image: The image.
%
*/
MagickExport MagickBooleanType SyncImageOptions(const ImageInfo *image_info,
  Image *image)
{
  char
    property[MaxTextExtent];

  const char
    *value,
    *option;

  MagickStatusType
    flags;

  /*
    Sync image options.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  option=GetImageOption(image_info,"depth");
  if (option != (const char *) NULL)
    image->depth=(unsigned long) atol(option);
  option=GetImageOption(image_info,"delay");
  if (option != (const char *) NULL)
    {
      GeometryInfo
        geometry_info;

      flags=ParseGeometry(option,&geometry_info);
      if ((flags & GreaterValue) != 0)
        {
          if (image->delay > (unsigned long) (geometry_info.rho+0.5))
            image->delay=(unsigned long) (geometry_info.rho+0.5);
        }
      else
        if ((flags & LessValue) != 0)
          {
            if (image->delay < (unsigned long) (geometry_info.rho+0.5))
              image->ticks_per_second=(long) (geometry_info.sigma+0.5);
          }
        else
          image->delay=(unsigned long) (geometry_info.rho+0.5);
      if ((flags & SigmaValue) != 0)
        image->ticks_per_second=(long) (geometry_info.sigma+0.5);
    }
  option=GetImageOption(image_info,"dispose");
  if (option != (const char *) NULL)
    image->dispose=(DisposeType) ParseMagickOption(MagickDisposeOptions,
      MagickFalse,option);
  option=GetImageOption(image_info,"gravity");
  if (option != (const char *) NULL)
    image->gravity=(GravityType) ParseMagickOption(MagickGravityOptions,
      MagickFalse,option);
  option=GetImageOption(image_info,"intent");
  if (option != (const char *) NULL)
    image->rendering_intent=(RenderingIntent) ParseMagickOption(
      MagickIntentOptions,MagickFalse,option);
  option=GetImageOption(image_info,"interpolate");
  if (option != (const char *) NULL)
    image->interpolate=(InterpolatePixelMethod) ParseMagickOption(
      MagickInterpolateOptions,MagickFalse,option);
  option=GetImageOption(image_info,"tile-offset");
  if (option != (const char *) NULL)
    {
      char
        *geometry;

      geometry=GetPageGeometry(option);
      flags=ParseAbsoluteGeometry(geometry,&image->tile_offset);
      geometry=DestroyString(geometry);
    }
  option=GetImageOption(image_info,"page");
  if (option != (const char *) NULL)
    {
      char
        *geometry;

      geometry=GetPageGeometry(option);
      flags=ParseAbsoluteGeometry(geometry,&image->page);
      geometry=DestroyString(geometry);
    }
  ResetImageOptionIterator(image_info);
  for (option=GetNextImageOption(image_info); option != (const char *) NULL; )
  {
    value=GetImageOption(image_info,option);
    if (value != (const char *) NULL)
      {
        (void) FormatMagickString(property,MaxTextExtent,"%s",option);
        (void) SetImageArtifact(image,property,value);
      }
    option=GetNextImageOption(image_info);
  }
  return(MagickTrue);
}
