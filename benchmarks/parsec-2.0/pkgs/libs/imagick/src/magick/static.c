/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  SSSSS  TTTTT   AAA   TTTTT  IIIII   CCCC                   %
%                  SS       T    A   A    T      I    C                       %
%                   SSS     T    AAAAA    T      I    C                       %
%                     SS    T    A   A    T      I    C                       %
%                  SSSSS    T    A   A    T    IIIII   CCCC                   %
%                                                                             %
%                                                                             %
%                         ImageMagick Static Methods                          %
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
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/module.h"
#include "magick/static.h"
#include "magick/string_.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n v o k e S t a t i c I m a g e F i l t e r                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InvokeStaticImageFilter() invokes a static image filter.
%
%  The format of the InvokeStaticImageFilter method is:
%
%      MagickBooleanType InvokeStaticImageFilter(const char *tag,Image **image,
%        const int argc,char **argv)
%
%  A description of each parameter follows:
%
%    o tag: The module tag.
%
%    o image: The image.
%
%    o argc: The number of elements in the argument vector.
%
%    o argv: A text array containing the command line arguments.
%
%    o argv: A text array containing the command line arguments.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
#if defined(SupportMagickModules)
MagickExport MagickBooleanType InvokeStaticImageFilter(const char *tag,
  Image **image,const int argc,char **argv,ExceptionInfo *exception)
{
  assert(image != (Image **) NULL);
  assert((*image)->signature == MagickSignature);
  if ((*image)->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",(*image)->filename);
#if defined(BuildMagickModules)
  (void) tag;
  (void) argc;
  (void) argv;
  (void) exception;
#else
  {
    extern unsigned long
      analyzeImage(Image **,const int,char **,ExceptionInfo *);

    ImageFilterHandler
      *image_filter;

    image_filter=(ImageFilterHandler *) NULL;
    if (LocaleCompare("analyze",tag) == 0)
      image_filter=analyzeImage;
    if (image_filter != (ImageFilterHandler *) NULL)
      {
        unsigned long
          signature;

        if ((*image)->debug != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Invoking \"%s\" static image filter",tag);
        signature=image_filter(image,argc,argv,exception);
        if ((*image)->debug != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),"\"%s\" completes",
            tag);
        if (signature != MagickImageFilterSignature)
          {
            (void) ThrowMagickException(exception,GetMagickModule(),ModuleError,
              "ImageFilterSignatureMismatch","`%s': %8lx != %8lx",tag,signature,
              MagickImageFilterSignature);
            return(MagickFalse);
          }
      }
  }
#endif
  return(MagickTrue);
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r S t a t i c M o d u l e s                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  (void) RegisterStaticModules() statically registers all the available module
%  handlers.
%
%  The format of the RegisterStaticModules method is:
%
%      (void) RegisterStaticModules(void)
%
*/
MagickExport void RegisterStaticModules(void)
{
#if !defined(BuildMagickModules)
  (void) RegisterARTImage();
  (void) RegisterAVIImage();
  (void) RegisterAVSImage();
  (void) RegisterBMPImage();
  (void) RegisterCAPTIONImage();
  (void) RegisterCINImage();
  (void) RegisterCIPImage();
  (void) RegisterCLIPImage();
#if defined(HasWINGDI32)
  (void) RegisterCLIPBOARDImage();
#endif
  (void) RegisterCMYKImage();
  (void) RegisterCUTImage();
  (void) RegisterDCMImage();
  (void) RegisterDIBImage();
#if defined(HasDJVU)
  (void) RegisterDJVUImage();
#endif
  (void) RegisterDNGImage();
  (void) RegisterDPSImage();
  (void) RegisterDPXImage();
#if defined(HasWINGDI32)
  (void) RegisterEMFImage();
#endif
#if defined(HasTIFF)
  (void) RegisterEPTImage();
#endif
  (void) RegisterEXRImage();
  (void) RegisterFAXImage();
  (void) RegisterFITSImage();
#if defined(HasFPX)
  (void) RegisterFPXImage();
#endif
  (void) RegisterGIFImage();
  (void) RegisterGRAYImage();
  (void) RegisterGRADIENTImage();
  (void) RegisterHISTOGRAMImage();
  (void) RegisterHTMLImage();
  (void) RegisterICONImage();
  (void) RegisterINFOImage();
  (void) RegisterIPLImage();
#if defined(HasJBIG)
  (void) RegisterJBIGImage();
#endif
#if defined(HasJPEG)
  (void) RegisterJPEGImage();
#endif
#if defined(HasJP2)
  (void) RegisterJP2Image();
#endif
  (void) RegisterLABELImage();
  (void) RegisterMAGICKImage();
  (void) RegisterMAPImage();
  (void) RegisterMATImage();
  (void) RegisterMATTEImage();
  (void) RegisterMETAImage();
  (void) RegisterMIFFImage();
  (void) RegisterMONOImage();
  (void) RegisterMPCImage();
  (void) RegisterMPEGImage();
  (void) RegisterMPRImage();
  (void) RegisterMSLImage();
  (void) RegisterMTVImage();
  (void) RegisterMVGImage();
  (void) RegisterNULLImage();
  (void) RegisterOTBImage();
  (void) RegisterPALMImage();
  (void) RegisterPATTERNImage();
  (void) RegisterPCDImage();
  (void) RegisterPCLImage();
  (void) RegisterPCXImage();
  (void) RegisterPDBImage();
  (void) RegisterPDFImage();
  (void) RegisterPICTImage();
  (void) RegisterPIXImage();
  (void) RegisterPLASMAImage();
#if defined(HasPNG)
  (void) RegisterPNGImage();
#endif
  (void) RegisterPNMImage();
  (void) RegisterPREVIEWImage();
  (void) RegisterPSImage();
  (void) RegisterPS2Image();
  (void) RegisterPS3Image();
  (void) RegisterPSDImage();
  (void) RegisterPWPImage();
  (void) RegisterRAWImage();
  (void) RegisterRGBImage();
  (void) RegisterRLAImage();
  (void) RegisterRLEImage();
  (void) RegisterSCRImage();
  (void) RegisterSCTImage();
  (void) RegisterSFWImage();
  (void) RegisterSGIImage();
  (void) RegisterSTEGANOImage();
  (void) RegisterSUNImage();
  (void) RegisterSVGImage();
  (void) RegisterTGAImage();
  (void) RegisterTHUMBNAILImage();
#if defined(HasTIFF)
  (void) RegisterTIFFImage();
#endif
  (void) RegisterTILEImage();
  (void) RegisterTIMImage();
  (void) RegisterTTFImage();
  (void) RegisterTXTImage();
  (void) RegisterUILImage();
  (void) RegisterURLImage();
  (void) RegisterUYVYImage();
  (void) RegisterVICARImage();
  (void) RegisterVIDImage();
  (void) RegisterVIFFImage();
  (void) RegisterWBMPImage();
  (void) RegisterWMFImage();
  (void) RegisterWPGImage();
#if defined(HasX11)
  (void) RegisterXImage();
#endif
  (void) RegisterXBMImage();
  (void) RegisterXCImage();
  (void) RegisterXCFImage();
  (void) RegisterXPMImage();
#if defined(_VISUALC_)
  (void) RegisterXTRNImage();
#endif
#if defined(HasX11)
  (void) RegisterXWDImage();
#endif
  (void) RegisterYCBCRImage();
  (void) RegisterYUVImage();
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r S t a t i c M o d u l e s                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterStaticModules() statically unregisters all the available module
%  handlers.
%
%  The format of the UnregisterStaticModules method is:
%
%      UnregisterStaticModules(void)
%
%
*/
MagickExport void UnregisterStaticModules(void)
{
#if !defined(BuildMagickModules)
  UnregisterARTImage();
  UnregisterAVIImage();
  UnregisterAVSImage();
  UnregisterBMPImage();
  UnregisterCAPTIONImage();
  UnregisterCINImage();
  UnregisterCIPImage();
  UnregisterCLIPImage();
#if defined(HasWINGDI32)
  UnregisterCLIPBOARDImage();
#endif
  UnregisterCMYKImage();
  UnregisterCUTImage();
  UnregisterDCMImage();
  UnregisterDIBImage();
#if defined(HasDJVU)
  UnregisterDJVUImage();
#endif
  UnregisterDNGImage();
  UnregisterDPSImage();
  UnregisterDPXImage();
#if defined(HasWINGDI32)
  UnregisterEMFImage();
#endif
#if defined(HasTIFF)
  UnregisterEPTImage();
#endif
  UnregisterEXRImage();
  UnregisterFAXImage();
  UnregisterFITSImage();
#if defined(HasFPX)
  UnregisterFPXImage();
#endif
  UnregisterGIFImage();
  UnregisterGRAYImage();
  UnregisterGRADIENTImage();
  UnregisterHISTOGRAMImage();
  UnregisterHTMLImage();
  UnregisterICONImage();
  UnregisterINFOImage();
  UnregisterIPLImage();
#if defined(HasJBIG)
  UnregisterJBIGImage();
#endif
#if defined(HasJPEG)
  UnregisterJPEGImage();
#endif
#if defined(HasJP2)
  UnregisterJP2Image();
#endif
  UnregisterLABELImage();
  UnregisterMAGICKImage();
  UnregisterMAPImage();
  UnregisterMATImage();
  UnregisterMATTEImage();
  UnregisterMETAImage();
  UnregisterMIFFImage();
  UnregisterMONOImage();
  UnregisterMPCImage();
  UnregisterMPEGImage();
  UnregisterMPRImage();
  UnregisterMSLImage();
  UnregisterMTVImage();
  UnregisterMVGImage();
  UnregisterNULLImage();
  UnregisterOTBImage();
  UnregisterPALMImage();
  UnregisterPATTERNImage();
  UnregisterPCDImage();
  UnregisterPCLImage();
  UnregisterPCXImage();
  UnregisterPDBImage();
  UnregisterPDFImage();
  UnregisterPICTImage();
  UnregisterPIXImage();
  UnregisterPLASMAImage();
#if defined(HasPNG)
  UnregisterPNGImage();
#endif
  UnregisterPNMImage();
  UnregisterPREVIEWImage();
  UnregisterPSImage();
  UnregisterPS2Image();
  UnregisterPS3Image();
  UnregisterPSDImage();
  UnregisterPWPImage();
  UnregisterRAWImage();
  UnregisterRGBImage();
  UnregisterRLAImage();
  UnregisterRLEImage();
  UnregisterSCRImage();
  UnregisterSCTImage();
  UnregisterSFWImage();
  UnregisterSGIImage();
  UnregisterSTEGANOImage();
  UnregisterSUNImage();
  UnregisterSVGImage();
  UnregisterTGAImage();
  UnregisterTHUMBNAILImage();
#if defined(HasTIFF)
  UnregisterTIFFImage();
#endif
  UnregisterTILEImage();
  UnregisterTIMImage();
  UnregisterTTFImage();
  UnregisterTXTImage();
  UnregisterUILImage();
  UnregisterURLImage();
  UnregisterUYVYImage();
  UnregisterVICARImage();
  UnregisterVIDImage();
  UnregisterVIFFImage();
  UnregisterWBMPImage();
  UnregisterWMFImage();
  UnregisterWPGImage();
#if defined(HasX11)
  UnregisterXImage();
#endif
  UnregisterXBMImage();
  UnregisterXCImage();
  UnregisterXCFImage();
  UnregisterXPMImage();
#if defined(_VISUALC_)
  UnregisterXTRNImage();
#endif
#if defined(HasX11)
  UnregisterXWDImage();
#endif
  UnregisterYCBCRImage();
  UnregisterYUVImage();
#endif
}
