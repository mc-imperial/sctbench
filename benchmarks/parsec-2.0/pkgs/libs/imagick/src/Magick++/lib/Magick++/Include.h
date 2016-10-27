// This may look like C code, but it is really -*- C++ -*-
//
// Copyright Bob Friesenhahn, 1999, 2000, 2001, 2002
//
// Inclusion of ImageMagick headers (with namespace magic)

#ifndef Magick_Include_header
#define Magick_Include_header

#if !defined(_MAGICK_CONFIG_H)
# define _MAGICK_CONFIG_H
# if !defined(vms) && !defined(macintosh)
#  include "magick/magick-config.h"
# else
#  include "magick-config.h"
# endif
# undef inline // Remove possible definition from config.h
# undef class
#endif

// Needed for stdio FILE
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#if defined(HAVE_SYS_TYPES_H)
# include <sys/types.h>
#endif

#if defined(macintosh)
#  include <stat.mac.h>  /* Needed for off_t */
#endif

#if defined(__BORLANDC__)
# include <vcl.h> /* Borland C++ Builder 4.0 requirement */
#endif // defined(__BORLANDC__)

//
// Include ImageMagick headers into namespace "MagickLib". If
// MAGICKCORE_IMPLEMENTATION is defined, include ImageMagick development
// headers.  This scheme minimizes the possibility of conflict with
// user code.
//
namespace MagickLib
{
#include <magick/MagickCore.h>
#include <wand/MagickWand.h>
#undef inline // Remove possible definition from config.h

#undef class
}

//
// Provide appropriate DLL imports/exports for Visual C++,
// Borland C++Builder and MinGW builds
//
#if defined(WIN32) && !defined(__CYGWIN__) && !defined(__MINGW32__)
# define MagickCplusPlusDLLSupported
#endif
#if defined(MagickCplusPlusDLLSupported)
#  if defined(_MT) && defined(_DLL) && !defined(_LIB)
#    define MagickDLLBuild
#    if defined(_VISUALC_)
#      pragma warning( disable: 4273 )  /* Disable the stupid dll linkage warnings */
#      pragma warning( disable: 4251 )
#    endif
#    if !defined(MAGICKCORE_IMPLEMENTATION)
#      define MagickDLLDecl __declspec(dllimport)
#      define MagickDLLDeclExtern extern __declspec(dllimport)
#      if defined(_VISUALC_)
#        pragma message( "Magick++ lib DLL import" )
#      endif
#    else
#      if defined(__BORLANDC__)
#        define MagickDLLDecl __declspec(dllexport)
#        define MagickDLLDeclExtern __declspec(dllexport)
#        pragma message( "BCBMagick++ lib DLL export" )
#      else
#        define MagickDLLDecl __declspec(dllexport)
#        define MagickDLLDeclExtern extern __declspec(dllexport)
#      endif
#      if defined(_VISUALC_)
#        pragma message( "Magick++ lib DLL export" )
#      endif
#    endif
#  else
#    define MagickDLLDecl
#    define MagickDLLDeclExtern
#    if defined(_VISUALC_)
#      pragma message( "Magick++ lib static interface" )
#    endif
#  endif
#else
#  define MagickDLLDecl
#  define MagickDLLDeclExtern
#endif

#if defined(WIN32) && defined(_VISUALC_)
#  pragma warning(disable : 4996) /* function deprecation warnings */
#endif

//
// Import ImageMagick symbols and types which are used as part of the
// Magick++ API definition into namespace "Magick".
//
namespace Magick
{
  // The datatype for an RGB component
  using MagickLib::Quantum;
  using MagickLib::MagickSizeType;

  // Boolean types
  using MagickLib::MagickBooleanType;
  using MagickLib::MagickFalse;
  using MagickLib::MagickTrue;

  // Image class types
  using MagickLib::ClassType;
  using MagickLib::UndefinedClass;
  using MagickLib::DirectClass;
  using MagickLib::PseudoClass;
  
  // Channel types
  using MagickLib::ChannelType;
  using MagickLib::UndefinedChannel;
  using MagickLib::RedChannel;
  using MagickLib::CyanChannel;
  using MagickLib::GreenChannel;
  using MagickLib::MagentaChannel;
  using MagickLib::BlueChannel;
  using MagickLib::YellowChannel;
  using MagickLib::OpacityChannel;
  using MagickLib::BlackChannel;
  using MagickLib::MatteChannel;
  using MagickLib::DefaultChannels;
  using MagickLib::AllChannels;
  
  // Color-space types
  using MagickLib::CMYKColorspace;
  using MagickLib::ColorspaceType;
  using MagickLib::GRAYColorspace;
  using MagickLib::HSLColorspace;
  using MagickLib::HWBColorspace;
  using MagickLib::LABColorspace;
  using MagickLib::LogColorspace;
  using MagickLib::OHTAColorspace;
  using MagickLib::Rec601LumaColorspace;
  using MagickLib::Rec709LumaColorspace;
  using MagickLib::RGBColorspace;
  using MagickLib::sRGBColorspace;
  using MagickLib::TransparentColorspace;
  using MagickLib::UndefinedColorspace;
  using MagickLib::XYZColorspace;
  using MagickLib::YCbCrColorspace;
  using MagickLib::YCCColorspace;
  using MagickLib::YIQColorspace;
  using MagickLib::YPbPrColorspace;
  using MagickLib::YUVColorspace;
  
  // Composition operations
  using MagickLib::AddCompositeOp;
  using MagickLib::AtopCompositeOp;
  using MagickLib::BlendCompositeOp;
  using MagickLib::BumpmapCompositeOp;
  using MagickLib::ClearCompositeOp;
  using MagickLib::ColorizeCompositeOp;
  using MagickLib::CompositeOperator;
  using MagickLib::CopyBlueCompositeOp;
  using MagickLib::CopyCompositeOp;
  using MagickLib::CopyCyanCompositeOp;
  using MagickLib::CopyGreenCompositeOp;
  using MagickLib::CopyMagentaCompositeOp;
  using MagickLib::CopyOpacityCompositeOp;
  using MagickLib::CopyRedCompositeOp;
  using MagickLib::CopyYellowCompositeOp;
  using MagickLib::DarkenCompositeOp;
  using MagickLib::DifferenceCompositeOp;
  using MagickLib::DisplaceCompositeOp;
  using MagickLib::DissolveCompositeOp;
  using MagickLib::DstOverCompositeOp;
  using MagickLib::ExclusionCompositeOp;
  using MagickLib::HardLightCompositeOp;
  using MagickLib::HueCompositeOp;
  using MagickLib::InCompositeOp;
  using MagickLib::LightenCompositeOp;
  using MagickLib::LuminizeCompositeOp;
  using MagickLib::MinusCompositeOp;
  using MagickLib::ModulateCompositeOp;
  using MagickLib::MultiplyCompositeOp;
  using MagickLib::NoCompositeOp;
  using MagickLib::OutCompositeOp;
  using MagickLib::OverCompositeOp;
  using MagickLib::OverlayCompositeOp;
  using MagickLib::PlusCompositeOp;
  using MagickLib::SaturateCompositeOp;
  using MagickLib::ScreenCompositeOp;
  using MagickLib::SoftLightCompositeOp;
  using MagickLib::SubtractCompositeOp;
  using MagickLib::ThresholdCompositeOp;
  using MagickLib::UndefinedCompositeOp;
  using MagickLib::XorCompositeOp;
  using MagickLib::CopyBlackCompositeOp;
  
  // Compression algorithms
  using MagickLib::CompressionType;
  using MagickLib::UndefinedCompression;
  using MagickLib::NoCompression;
  using MagickLib::BZipCompression;
  using MagickLib::FaxCompression;
  using MagickLib::Group4Compression;
  using MagickLib::JPEGCompression;
  using MagickLib::LZWCompression;
  using MagickLib::RLECompression;
  using MagickLib::ZipCompression;

  using MagickLib::DisposeType;
  using MagickLib::UndefinedDispose;
  using MagickLib::NoneDispose;
  using MagickLib::BackgroundDispose;
  using MagickLib::PreviousDispose;

  // Endian options
  using MagickLib::EndianType;
  using MagickLib::UndefinedEndian;
  using MagickLib::LSBEndian;
  using MagickLib::MSBEndian;

  // Evaluate options
  using MagickLib::UndefinedEvaluateOperator;
  using MagickLib::AddEvaluateOperator;
  using MagickLib::AndEvaluateOperator;
  using MagickLib::DivideEvaluateOperator;
  using MagickLib::LeftShiftEvaluateOperator;
  using MagickLib::MaxEvaluateOperator;
  using MagickLib::MinEvaluateOperator;
  using MagickLib::MultiplyEvaluateOperator;
  using MagickLib::OrEvaluateOperator;
  using MagickLib::RightShiftEvaluateOperator;
  using MagickLib::SetEvaluateOperator;
  using MagickLib::SubtractEvaluateOperator;
  using MagickLib::XorEvaluateOperator;
  using MagickLib::MagickEvaluateOperator;

  // Fill rules
  using MagickLib::FillRule;
  using MagickLib::UndefinedRule;
  using MagickLib::EvenOddRule;
  using MagickLib::NonZeroRule;
  
  // Filter types
  using MagickLib::FilterTypes;
  using MagickLib::UndefinedFilter;
  using MagickLib::PointFilter;
  using MagickLib::BoxFilter;
  using MagickLib::TriangleFilter;
  using MagickLib::HermiteFilter;
  using MagickLib::HanningFilter;
  using MagickLib::HammingFilter;
  using MagickLib::BlackmanFilter;
  using MagickLib::GaussianFilter;
  using MagickLib::QuadraticFilter;
  using MagickLib::CubicFilter;
  using MagickLib::CatromFilter;
  using MagickLib::MitchellFilter;
  using MagickLib::LanczosFilter;
  using MagickLib::BesselFilter;
  using MagickLib::SincFilter;

  // Bit gravity
  using MagickLib::GravityType;
  using MagickLib::ForgetGravity;
  using MagickLib::NorthWestGravity;
  using MagickLib::NorthGravity;
  using MagickLib::NorthEastGravity;
  using MagickLib::WestGravity;
  using MagickLib::CenterGravity;
  using MagickLib::EastGravity;
  using MagickLib::SouthWestGravity;
  using MagickLib::SouthGravity;
  using MagickLib::SouthEastGravity;
  using MagickLib::StaticGravity;

  // Image types
  using MagickLib::ImageType;
  using MagickLib::UndefinedType;
  using MagickLib::BilevelType;
  using MagickLib::GrayscaleType;
  using MagickLib::GrayscaleMatteType;
  using MagickLib::PaletteType;
  using MagickLib::PaletteMatteType;
  using MagickLib::TrueColorType;
  using MagickLib::TrueColorMatteType;
  using MagickLib::ColorSeparationType;
  using MagickLib::ColorSeparationMatteType;
  using MagickLib::OptimizeType;
  
  // Interlace types
  using MagickLib::InterlaceType;
  using MagickLib::UndefinedInterlace;
  using MagickLib::NoInterlace;
  using MagickLib::LineInterlace;
  using MagickLib::PlaneInterlace;
  using MagickLib::PartitionInterlace;

  // Line cap types
  using MagickLib::LineCap;
  using MagickLib::UndefinedCap;
  using MagickLib::ButtCap;
  using MagickLib::RoundCap;
  using MagickLib::SquareCap;

  // Line join types
  using MagickLib::LineJoin;
  using MagickLib::UndefinedJoin;
  using MagickLib::MiterJoin;
  using MagickLib::RoundJoin;
  using MagickLib::BevelJoin;

  // Noise types
  using MagickLib::NoiseType;
  using MagickLib::UniformNoise;
  using MagickLib::GaussianNoise;
  using MagickLib::MultiplicativeGaussianNoise;
  using MagickLib::ImpulseNoise;
  using MagickLib::LaplacianNoise;
  using MagickLib::PoissonNoise;

  // Orientation types
  using MagickLib::OrientationType;
  using MagickLib::UndefinedOrientation;
  using MagickLib::TopLeftOrientation;
  using MagickLib::TopRightOrientation;
  using MagickLib::BottomRightOrientation;
  using MagickLib::BottomLeftOrientation;
  using MagickLib::LeftTopOrientation;
  using MagickLib::RightTopOrientation;
  using MagickLib::RightBottomOrientation;
  using MagickLib::LeftBottomOrientation;
  
  // Paint methods
  using MagickLib::PaintMethod;
  using MagickLib::PointMethod;
  using MagickLib::ReplaceMethod;
  using MagickLib::FloodfillMethod;
  using MagickLib::FillToBorderMethod;
  using MagickLib::ResetMethod;

  // Preview types.  Not currently used by Magick++
  using MagickLib::PreviewType;
  using MagickLib::UndefinedPreview;
  using MagickLib::RotatePreview;
  using MagickLib::ShearPreview;
  using MagickLib::RollPreview;
  using MagickLib::HuePreview;
  using MagickLib::SaturationPreview;
  using MagickLib::BrightnessPreview;
  using MagickLib::GammaPreview;
  using MagickLib::SpiffPreview;
  using MagickLib::DullPreview;
  using MagickLib::GrayscalePreview;
  using MagickLib::QuantizePreview;
  using MagickLib::DespecklePreview;
  using MagickLib::ReduceNoisePreview;
  using MagickLib::AddNoisePreview;
  using MagickLib::SharpenPreview;
  using MagickLib::BlurPreview;
  using MagickLib::ThresholdPreview;
  using MagickLib::EdgeDetectPreview;
  using MagickLib::SpreadPreview;
  using MagickLib::SolarizePreview;
  using MagickLib::ShadePreview;
  using MagickLib::RaisePreview;
  using MagickLib::SegmentPreview;
  using MagickLib::SwirlPreview;
  using MagickLib::ImplodePreview;
  using MagickLib::WavePreview;
  using MagickLib::OilPaintPreview;
  using MagickLib::CharcoalDrawingPreview;
  using MagickLib::JPEGPreview;

  // Quantum types
  using MagickLib::QuantumType;
  using MagickLib::IndexQuantum;
  using MagickLib::GrayQuantum;
  using MagickLib::IndexAlphaQuantum;
  using MagickLib::GrayAlphaQuantum;
  using MagickLib::RedQuantum;
  using MagickLib::CyanQuantum;
  using MagickLib::GreenQuantum;
  using MagickLib::YellowQuantum;
  using MagickLib::BlueQuantum;
  using MagickLib::MagentaQuantum;
  using MagickLib::AlphaQuantum;
  using MagickLib::BlackQuantum;
  using MagickLib::RGBQuantum;
  using MagickLib::RGBAQuantum;
  using MagickLib::CMYKQuantum;

  // Rendering intents
  using MagickLib::RenderingIntent;
  using MagickLib::UndefinedIntent;
  using MagickLib::SaturationIntent;
  using MagickLib::PerceptualIntent;
  using MagickLib::AbsoluteIntent;
  using MagickLib::RelativeIntent;
  
  // Resource types
  using MagickLib::MemoryResource;

  // Resolution units
  using MagickLib::ResolutionType;
  using MagickLib::UndefinedResolution;
  using MagickLib::PixelsPerInchResolution;
  using MagickLib::PixelsPerCentimeterResolution;

  // PixelPacket structure
  using MagickLib::PixelPacket;

  // IndexPacket type
  using MagickLib::IndexPacket;

  // StorageType type
  using MagickLib::StorageType;
  using MagickLib::CharPixel;
  using MagickLib::ShortPixel;
  using MagickLib::IntegerPixel;
  using MagickLib::FloatPixel;
  using MagickLib::DoublePixel;

  // StretchType type
  using MagickLib::StretchType;
  using MagickLib::NormalStretch;
  using MagickLib::UltraCondensedStretch;
  using MagickLib::ExtraCondensedStretch;
  using MagickLib::CondensedStretch;
  using MagickLib::SemiCondensedStretch;
  using MagickLib::SemiExpandedStretch;
  using MagickLib::ExpandedStretch;
  using MagickLib::ExtraExpandedStretch;
  using MagickLib::UltraExpandedStretch;
  using MagickLib::AnyStretch;

  // StyleType type
  using MagickLib::StyleType;
  using MagickLib::NormalStyle;
  using MagickLib::ItalicStyle;
  using MagickLib::ObliqueStyle;
  using MagickLib::AnyStyle;

  // Decoration types
  using MagickLib::DecorationType;
  using MagickLib::NoDecoration;
  using MagickLib::UnderlineDecoration;
  using MagickLib::OverlineDecoration;
  using MagickLib::LineThroughDecoration;

#if defined(MAGICKCORE_IMPLEMENTATION)
  //
  // ImageMagick symbols used in implementation code
  //
  using MagickLib::AcquireCacheViewPixels;
  using MagickLib::AcquireImagePixels;
  using MagickLib::AcquireMagickMemory;
  using MagickLib::AcquireString;
	using MagickLib::AcquireStringInfo;
  using MagickLib::AdaptiveBlurImage;
  using MagickLib::AdaptiveThresholdImage;
  using MagickLib::AddNoiseImage;
  using MagickLib::AffineMatrix;
  using MagickLib::AffineTransformImage;
  using MagickLib::AllocateImage;
  using MagickLib::AnnotateImage;
  using MagickLib::AspectValue;
  using MagickLib::Base64Decode;
  using MagickLib::Base64Encode;
  using MagickLib::BilevelImage;
  using MagickLib::BlobError;
  using MagickLib::BlobFatalError;
  using MagickLib::BlobToImage;
  using MagickLib::BlobWarning;
  using MagickLib::BlurImage;
  using MagickLib::BorderImage;
  using MagickLib::CacheError;
  using MagickLib::CacheFatalError;
  using MagickLib::CacheWarning;
  using MagickLib::CharcoalImage;
  using MagickLib::ChopImage;
  using MagickLib::ClearMagickException;
  using MagickLib::CloneDrawInfo;
  using MagickLib::CloneImage;
  using MagickLib::CloneImageInfo;
  using MagickLib::CloneQuantizeInfo;
  using MagickLib::CloseCacheView;
  using MagickLib::CoderError;
  using MagickLib::CoderFatalError;
  using MagickLib::CoderWarning;
  using MagickLib::ColorizeImage;
  using MagickLib::ColorPacket;
  using MagickLib::CompositeImage;
  using MagickLib::ConfigureError;
  using MagickLib::ConfigureFatalError;
  using MagickLib::ConfigureWarning;
  using MagickLib::ConstituteImage;
  using MagickLib::ContrastImage;
  using MagickLib::ConvolveImage;
  using MagickLib::CopyMagickString;
  using MagickLib::CorruptImageError;
  using MagickLib::CorruptImageFatalError;
  using MagickLib::CorruptImageWarning;
  using MagickLib::CropImage;
  using MagickLib::CycleColormapImage;
  using MagickLib::DelegateError;
  using MagickLib::DelegateFatalError;
  using MagickLib::DelegateWarning;
  using MagickLib::DeleteImageOption;
  using MagickLib::DeleteImageRegistry;
  using MagickLib::DespeckleImage;
  using MagickLib::DestroyDrawInfo;
  using MagickLib::DestroyDrawingWand;
  using MagickLib::DestroyExceptionInfo;
  using MagickLib::DestroyImageInfo;
  using MagickLib::DestroyImageList;
  using MagickLib::DestroyMagickWand;
  using MagickLib::DestroyPixelWand;
  using MagickLib::DestroyQuantizeInfo;
  using MagickLib::DestroyStringInfo;
  using MagickLib::DisplayImages;
  using MagickLib::DrawAffine;
  using MagickLib::DrawAllocateWand;
  using MagickLib::DrawAnnotation;
  using MagickLib::DrawArc;
  using MagickLib::DrawBezier;
  using MagickLib::DrawCircle;
  using MagickLib::DrawColor;
  using MagickLib::DrawComment;
  using MagickLib::DrawComposite;
  using MagickLib::DrawEllipse;
  using MagickLib::DrawError;
  using MagickLib::DrawFatalError;
  using MagickLib::DrawImage;
  using MagickLib::DrawInfo;
  using MagickLib::DrawingWand;
  using MagickLib::DrawLine;
  using MagickLib::DrawMatte;
  using MagickLib::DrawPathClose;
  using MagickLib::DrawPathCurveToAbsolute;
  using MagickLib::DrawPathCurveToQuadraticBezierAbsolute;
  using MagickLib::DrawPathCurveToQuadraticBezierRelative;
  using MagickLib::DrawPathCurveToQuadraticBezierSmoothAbsolute;
  using MagickLib::DrawPathCurveToQuadraticBezierSmoothRelative;
  using MagickLib::DrawPathCurveToRelative;
  using MagickLib::DrawPathCurveToSmoothAbsolute;
  using MagickLib::DrawPathCurveToSmoothRelative;
  using MagickLib::DrawPathEllipticArcAbsolute;
  using MagickLib::DrawPathEllipticArcRelative;
  using MagickLib::DrawPathFinish;
  using MagickLib::DrawPathLineToAbsolute;
  using MagickLib::DrawPathLineToHorizontalAbsolute;
  using MagickLib::DrawPathLineToHorizontalRelative;
  using MagickLib::DrawPathLineToRelative;
  using MagickLib::DrawPathLineToVerticalAbsolute;
  using MagickLib::DrawPathLineToVerticalRelative;
  using MagickLib::DrawPathMoveToAbsolute;
  using MagickLib::DrawPathMoveToRelative;
  using MagickLib::DrawPathStart;
  using MagickLib::DrawPoint;
  using MagickLib::DrawPolygon;
  using MagickLib::DrawPolyline;
  using MagickLib::DrawPopClipPath;
  using MagickLib::DrawPopDefs;
  using MagickLib::PopDrawingWand;
  using MagickLib::DrawPopPattern;
  using MagickLib::DrawPushClipPath;
  using MagickLib::DrawPushDefs;
  using MagickLib::PushDrawingWand;
  using MagickLib::DrawPushPattern;
  using MagickLib::DrawRectangle;
  using MagickLib::DrawRender;
  using MagickLib::DrawRotate;
  using MagickLib::DrawRoundRectangle;
  using MagickLib::DrawScale;
  using MagickLib::DrawSetClipPath;
  using MagickLib::DrawSetClipRule;
  using MagickLib::DrawSetClipUnits;
  using MagickLib::DrawSetFillOpacity;
  using MagickLib::DrawSetFillColor;
  using MagickLib::DrawSetFillPatternURL;
  using MagickLib::DrawSetFillRule;
  using MagickLib::DrawSetFont;
  using MagickLib::DrawSetFontFamily;
  using MagickLib::DrawSetFontSize;
  using MagickLib::DrawSetFontStretch;
  using MagickLib::DrawSetFontStyle;
  using MagickLib::DrawSetFontWeight;
  using MagickLib::DrawSetGravity;
  using MagickLib::DrawSetStrokeOpacity;
  using MagickLib::DrawSetStrokeAntialias;
  using MagickLib::DrawSetStrokeColor;
  using MagickLib::DrawSetStrokeDashArray;
  using MagickLib::DrawSetStrokeDashOffset;
  using MagickLib::DrawSetStrokeLineCap;
  using MagickLib::DrawSetStrokeLineJoin;
  using MagickLib::DrawSetStrokeMiterLimit;
  using MagickLib::DrawSetStrokePatternURL;
  using MagickLib::DrawSetStrokeWidth;
  using MagickLib::DrawSetTextAntialias;
  using MagickLib::DrawSetTextDecoration;
  using MagickLib::DrawSetTextEncoding;
  using MagickLib::DrawSetTextUnderColor;
  using MagickLib::DrawSetViewbox;
  using MagickLib::DrawSkewX;
  using MagickLib::DrawSkewY;
  using MagickLib::DrawTranslate;
  using MagickLib::DrawWarning;
  using MagickLib::EdgeImage;
  using MagickLib::EmbossImage;
  using MagickLib::EnhanceImage;
  using MagickLib::EqualizeImage;
  using MagickLib::EvaluateImage;
  using MagickLib::EvaluateImageChannel;
  using MagickLib::ExceptionInfo;
  using MagickLib::ExceptionType;
  using MagickLib::ExportImagePixels;
  using MagickLib::ExportQuantumPixels;
  using MagickLib::FileOpenError;
  using MagickLib::FileOpenFatalError;
  using MagickLib::FileOpenWarning;
  using MagickLib::FlattenImages;
  using MagickLib::FlipImage;
  using MagickLib::FlopImage;
  using MagickLib::FormatMagickString;
  using MagickLib::FrameImage;
  using MagickLib::FrameInfo;
  using MagickLib::GammaImage;
  using MagickLib::GammaImage;
  using MagickLib::GaussianBlurImage;
  using MagickLib::GetAffineMatrix;
  using MagickLib::GetBlobSize;
  using MagickLib::GetCacheViewPixels;
  using MagickLib::GetCacheViewIndexes;
  using MagickLib::GetImageChannelMean;
  using MagickLib::GetImageChannelRange;
  using MagickLib::GetImageClipMask;
  using MagickLib::GetColorTuple;
  using MagickLib::GetDrawInfo;
  using MagickLib::GetExceptionInfo;
  using MagickLib::GetGeometry;
  using MagickLib::GetImageBoundingBox;
  using MagickLib::GetImageChannelDepth;
  using MagickLib::GetImageDepth;
  using MagickLib::GetImageInfo;
  using MagickLib::GetImageOption;
  using MagickLib::GetImagePixels;
  using MagickLib::GetImageProperty;
  using MagickLib::GetImageProfile;
  using MagickLib::GetImageQuantizeError;
  using MagickLib::GetImageType;
  using MagickLib::GetIndexes;
  using MagickLib::GetMagickInfo;
  using MagickLib::GetMagickPixelPacket;
  using MagickLib::GetNumberColors;
  using MagickLib::GetPageGeometry;
  using MagickLib::GetPixels;
  using MagickLib::GetQuantizeInfo;
  using MagickLib::GetQuantumInfo;
  using MagickLib::GetStringInfoDatum;
  using MagickLib::GetStringInfoLength;
  using MagickLib::GetTypeMetrics;
  using MagickLib::GlobExpression;
  using MagickLib::GreaterValue;
  using MagickLib::ConvertHSLToRGB;
  using MagickLib::HeightValue;
  using MagickLib::ImageError;
  using MagickLib::ImageFatalError;
  using MagickLib::ImageInfo;
  using MagickLib::ImageRegistryType;
  using MagickLib::ImageToBlob;
  using MagickLib::ImageWarning;
  using MagickLib::ImplodeImage;
  using MagickLib::ImportQuantumPixels;
  using MagickLib::InvokeDynamicImageFilter;
  using MagickLib::IsEventLogging;
  using MagickLib::IsGeometry;
  using MagickLib::IsImagesEqual;
  using MagickLib::LessValue;
  using MagickLib::LevelImage;
  using MagickLib::LevelImageChannel;
  using MagickLib::LocaleCompare;
  using MagickLib::LogMagickEvent;
  using MagickLib::MagickCoreTerminus;
  using MagickLib::MagickInfo;
  using MagickLib::MagickPixelPacket;
  using MagickLib::MagickToMime;
  using MagickLib::MagickWand;
  using MagickLib::MagnifyImage;
  using MagickLib::MapImage;
  using MagickLib::MedianFilterImage;
  using MagickLib::MinifyImage;
  using MagickLib::MissingDelegateError;
  using MagickLib::MissingDelegateFatalError;
  using MagickLib::MissingDelegateWarning;
  using MagickLib::ModulateImage;
  using MagickLib::ModuleError;
  using MagickLib::ModuleFatalError;
  using MagickLib::ModuleWarning;
  using MagickLib::MonitorError;
  using MagickLib::MonitorFatalError;
  using MagickLib::MonitorWarning;
  using MagickLib::MontageInfo;
  using MagickLib::NegateImage;
  using MagickLib::NewPixelWand;
  using MagickLib::NewMagickWandFromImage;
  using MagickLib::NoValue;
  using MagickLib::NoiseType;
  using MagickLib::NormalizeImage;
  using MagickLib::OilPaintImage;
  using MagickLib::OpenCacheView;
  using MagickLib::OptionError;
  using MagickLib::OptionFatalError;
  using MagickLib::OptionWarning;
  using MagickLib::PaintFloodfillImage;
  using MagickLib::ParseMetaGeometry;
  using MagickLib::PercentValue;
  using MagickLib::PaintOpaqueImage;
  using MagickLib::PaintTransparentImage;
  using MagickLib::PingBlob;
  using MagickLib::PingImage;
  using MagickLib::PixelSetQuantumColor;
	using MagickLib::PixelWand;
  using MagickLib::PointInfo;
  using MagickLib::ProfileImage;
  using MagickLib::ProfileInfo;
  using MagickLib::QuantizeImage;
  using MagickLib::QuantizeInfo;
  using MagickLib::QuantumInfo;
  using MagickLib::QueryColorDatabase;
  using MagickLib::QueryMagickColor;
  using MagickLib::RGBTransformImage;
  using MagickLib::RaiseImage;
  using MagickLib::ReadImage;
  using MagickLib::RectangleInfo;
  using MagickLib::RectangleInfo;
  using MagickLib::ReduceNoiseImage;
  using MagickLib::RegisterMagickInfo;
  using MagickLib::RegistryError;
  using MagickLib::RegistryFatalError;
  using MagickLib::RegistryType;
  using MagickLib::RegistryWarning;
  using MagickLib::RelinquishMagickMemory;
  using MagickLib::ResizeImage;
  using MagickLib::ResizeMagickMemory;
  using MagickLib::ResourceLimitError;
  using MagickLib::ResourceLimitFatalError;
  using MagickLib::ResourceLimitWarning;
  using MagickLib::RollImage;
  using MagickLib::RotateImage;
  using MagickLib::SampleImage;
  using MagickLib::ScaleImage;
  using MagickLib::SegmentImage;
  using MagickLib::SeparateImageChannel;
  using MagickLib::SetCacheView;
  using MagickLib::SetClientName;
  using MagickLib::SetImageBackgroundColor;
  using MagickLib::SetImageChannelDepth;
  using MagickLib::SetImageClipMask;
  using MagickLib::SetImageDepth;
  using MagickLib::SetImageExtent;
  using MagickLib::SetImageInfo;
  using MagickLib::SetImageOpacity;
  using MagickLib::SetImageOption;
  using MagickLib::SetImagePixels;
  using MagickLib::SetImageProfile;
  using MagickLib::SetImageProperty;
  using MagickLib::SetImageType;
  using MagickLib::SetLogEventMask;
  using MagickLib::SetMagickInfo;
  using MagickLib::SetImageRegistry;
  using MagickLib::SetMagickResourceLimit;
  using MagickLib::SetStringInfoDatum;
  using MagickLib::ShadeImage;
  using MagickLib::SharpenImage;
  using MagickLib::ShaveImage;
  using MagickLib::ShearImage;
  using MagickLib::SigmoidalContrastImageChannel;
  using MagickLib::SignatureImage;
  using MagickLib::SolarizeImage;
  using MagickLib::SpreadImage;
  using MagickLib::SteganoImage;
  using MagickLib::StereoImage;
  using MagickLib::StreamError;
  using MagickLib::StreamFatalError;
  using MagickLib::StreamWarning;
  using MagickLib::StringInfo;
  using MagickLib::SwirlImage;
  using MagickLib::SyncCacheView;
  using MagickLib::SyncImage;
  using MagickLib::SyncImagePixels;
  using MagickLib::TextureImage;
  using MagickLib::ThrowException;
  using MagickLib::ConvertRGBToHSL;
  using MagickLib::TransformImage;
  using MagickLib::TransformRGBImage;
  using MagickLib::TrimImage;
  using MagickLib::TypeError;
  using MagickLib::TypeFatalError;
  using MagickLib::TypeWarning;
  using MagickLib::UndefinedException;
  using MagickLib::UndefinedRegistryType;
  using MagickLib::UnregisterMagickInfo;
  using MagickLib::UnsharpMaskImage;
  using MagickLib::ViewInfo;
  using MagickLib::WaveImage;
  using MagickLib::WidthValue;
  using MagickLib::WriteImage;
  using MagickLib::XNegative;
  using MagickLib::XServerError;
  using MagickLib::XServerFatalError;
  using MagickLib::XServerWarning;
  using MagickLib::XValue;
  using MagickLib::YNegative;
  using MagickLib::YValue;
  using MagickLib::ZoomImage;

#endif // MAGICKCORE_IMPLEMENTATION

}

#endif // Magick_Include_header
