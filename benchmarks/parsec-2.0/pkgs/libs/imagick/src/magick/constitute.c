/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     CCCC   OOO   N   N  SSSSS  TTTTT  IIIII  TTTTT  U   U  TTTTT  EEEEE     %
%    C      O   O  NN  N  SS       T      I      T    U   U    T    E         %
%    C      O   O  N N N  ESSS     T      I      T    U   U    T    EEE       %
%    C      O   O  N  NN     SS    T      I      T    U   U    T    E         %
%     CCCC   OOO   N   N  SSSSS    T    IIIII    T     UUU     T    EEEEE     %
%                                                                             %
%                                                                             %
%                       Methods to Consitute an Image                         %
%                                                                             %
%                             Software Design                                 %
%                               John Cristy                                   %
%                               October 1998                                  %
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
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/cache.h"
#include "magick/client.h"
#include "magick/constitute.h"
#include "magick/delegate.h"
#include "magick/geometry.h"
#include "magick/identify.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/option.h"
#include "magick/pixel.h"
#include "magick/profile.h"
#include "magick/property.h"
#include "magick/quantum.h"
#include "magick/resize.h"
#include "magick/resource_.h"
#include "magick/semaphore.h"
#include "magick/statistic.h"
#include "magick/stream.h"
#include "magick/string_.h"
#include "magick/timer.h"
#include "magick/transform.h"
#include "magick/utility.h"

static SemaphoreInfo
  *constitute_semaphore = (SemaphoreInfo *) NULL;

/*
  Forward declarations.
*/
static Image
  *ReadImages(const ImageInfo *,ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n s t i t u t e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConstituteImage() returns an image from the the pixel data you supply.
%  The pixel data must be in scanline order top-to-bottom.  The data can be
%  char, short int, int, float, or double.  Float and double require the
%  pixels to be normalized [0..1], otherwise [0..QuantumRange].  For example, to
%  create a 640x480 image from unsigned red-green-blue character data, use:
%
%      image = ConstituteImage(640,480,"RGB",CharPixel,pixels,&exception);
%
%  The format of the ConstituteImage method is:
%
%      Image *ConstituteImage(const unsigned long columns,
%        const unsigned long rows,const char *map,const StorageType storage,
%        const void *pixels,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o columns: width in pixels of the image.
%
%    o rows: height in pixels of the image.
%
%    o map:  This string reflects the expected ordering of the pixel array.
%      It can be any combination or order of R = red, G = green, B = blue,
%      A = alpha (0 is transparent), O = opacity (0 is opaque), C = cyan,
%      Y = yellow, M = magenta, K = black, I = intensity (for grayscale),
%      P = pad.
%
%    o storage: Define the data type of the pixels.  Float and double types are
%      expected to be normalized [0..1] otherwise [0..QuantumRange].  Choose
%      from these types: CharPixel, DoublePixel, FloatPixel, IntegerPixel,
%      LongPixel, QuantumPixel, or ShortPixel.
%
%    o pixels: This array of values contain the pixel components as defined by
%      map and type.  You must preallocate this array where the expected
%      length varies depending on the values of width, height, map, and type.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *ConstituteImage(const unsigned long columns,
  const unsigned long rows,const char *map,const StorageType storage,
  const void *pixels,ExceptionInfo *exception)
{
  Image
    *image;

  MagickBooleanType
    status;

  /*
    Allocate image structure.
  */
  assert(map != (const char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",map);
  assert(pixels != (void *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=AllocateImage((ImageInfo *) NULL);
  if (image == (Image *) NULL)
    return((Image *) NULL);
  if ((columns == 0) || (rows == 0))
    ThrowImageException(OptionError,"NonZeroWidthAndHeightRequired");
  image->columns=columns;
  image->rows=rows;
  (void) SetImageBackgroundColor(image);
  status=ImportImagePixels(image,0,0,columns,rows,map,storage,pixels);
  if (status == MagickFalse)
    {
      InheritException(exception,&image->exception);
      image=DestroyImage(image);
    }
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y C o n s t i t u t e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyConstitute() destroys the constitute environment.
%
%  The format of the DestroyConstitute method is:
%
%      DestroyConstitute(void)
%
%
*/
MagickExport void DestroyConstitute(void)
{
  if (constitute_semaphore != (SemaphoreInfo *) NULL)
    constitute_semaphore=DestroySemaphoreInfo(constitute_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i n g I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PingImage() returns all the properties of an image or image sequence
%  except for the pixels.  It is much faster and consumes far less memory
%  than ReadImage().  On failure, a NULL image is returned and exception
%  describes the reason for the failure.
%
%  The format of the PingImage method is:
%
%      Image *PingImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: Ping the image defined by the file or filename members of
%      this structure.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static size_t PingStream(const Image *magick_unused(image),
  const void *magick_unused(pixels),const size_t columns)
{
  return(columns);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport Image *PingImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  Image
    *image;

  ImageInfo
    *ping_info;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  ping_info=CloneImageInfo(image_info);
  ping_info->ping=MagickTrue;
  image=ReadStream(ping_info,&PingStream,exception);
  if (image != (Image *) NULL)
    {
      ResetTimer(&image->timer);
      if (ping_info->verbose != MagickFalse)
        (void) IdentifyImage(image,stdout,MagickFalse);
    }
  ping_info=DestroyImageInfo(ping_info);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadImage() reads an image or image sequence from a file or file handle.
%  The method returns a NULL if there is a memory shortage or if the image
%  cannot be read.  On failure, a NULL image is returned and exception
%  describes the reason for the failure.
%
%  The format of the ReadImage method is:
%
%      Image *ReadImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: Read the image defined by the file or filename members of
%      this structure.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *ReadImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  char
    filename[MaxTextExtent],
    magick[MaxTextExtent],
    magick_filename[MaxTextExtent];

  const char
    *value;

  const DelegateInfo
    *delegate_info;

  const MagickInfo
    *magick_info;

  ExceptionInfo
    sans_exception;

  GeometryInfo
    geometry_info;

  Image
    *image,
    *next;

  ImageInfo
    *read_info;

  MagickStatusType
    flags;

  /*
    Determine image type from filename prefix or suffix (e.g. image.jpg).
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image_info->filename != (char *) NULL);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  if (*image_info->filename == '@')
    return(ReadImages(image_info,exception));
  read_info=CloneImageInfo(image_info);
  (void) CopyMagickString(magick_filename,read_info->filename,MaxTextExtent);
  (void) SetImageInfo(read_info,MagickFalse,exception);
  (void) CopyMagickString(filename,read_info->filename,MaxTextExtent);
  (void) CopyMagickString(magick,read_info->magick,MaxTextExtent);
  /*
    Call appropriate image reader based on image type.
  */
  GetExceptionInfo(&sans_exception);
  magick_info=GetMagickInfo(read_info->magick,&sans_exception);
  (void) DestroyExceptionInfo(&sans_exception);
  if ((magick_info != (const MagickInfo *) NULL) &&
      (GetMagickSeekableStream(magick_info) != MagickFalse))
    {
      MagickBooleanType
        status;

      image=AllocateImage(read_info);
      (void) CopyMagickString(image->filename,read_info->filename,
        MaxTextExtent);
      status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
      if (status == MagickFalse)
        {
          read_info=DestroyImageInfo(read_info);
          image=DestroyImage(image);
          return((Image *) NULL);
        }
      if (IsBlobSeekable(image) == MagickFalse)
        {
          /*
            Coder requires a seekable stream.
          */
          *read_info->filename='\0';
          status=ImageToFile(image,read_info->filename,exception);
          if (status == MagickFalse)
            {
              CloseBlob(image);
              read_info=DestroyImageInfo(read_info);
              image=DestroyImage(image);
              return((Image *) NULL);
            }
          read_info->temporary=MagickTrue;
        }
      CloseBlob(image);
      image=DestroyImage(image);
    }
  image=NewImageList();
  if ((magick_info != (const MagickInfo *) NULL) &&
      (GetImageDecoder(magick_info) != (DecodeImageHandler *) NULL))
    {
      if ((GetMagickThreadSupport(magick_info) & DecoderThreadSupport) == 0)
        AcquireSemaphoreInfo(&constitute_semaphore);
      image=GetImageDecoder(magick_info)(read_info,exception);
      if ((GetMagickThreadSupport(magick_info) & DecoderThreadSupport) == 0)
        RelinquishSemaphoreInfo(constitute_semaphore);
    }
  else
    {
      delegate_info=GetDelegateInfo(read_info->magick,(char *) NULL,exception);
      if (delegate_info == (const DelegateInfo *) NULL)
        {
          if (IsAccessible(read_info->filename) != MagickFalse)
            (void) ThrowMagickException(exception,GetMagickModule(),
              MissingDelegateError,"NoDecodeDelegateForThisImageFormat","`%s'",
              read_info->filename);
          if (read_info->temporary != MagickFalse)
            (void) RelinquishUniqueFileResource(read_info->filename);
          read_info=DestroyImageInfo(read_info);
          return((Image *) NULL);
        }
      /*
        Let our decoding delegate process the image.
      */
      image=AllocateImage(read_info);
      if (image == (Image *) NULL)
        {
          read_info=DestroyImageInfo(read_info);
          return((Image *) NULL);
        }
      (void) CopyMagickString(image->filename,read_info->filename,
        MaxTextExtent);
      *read_info->filename='\0';
      if (GetDelegateThreadSupport(delegate_info) == MagickFalse)
        AcquireSemaphoreInfo(&constitute_semaphore);
      (void) InvokeDelegate(read_info,image,read_info->magick,(char *) NULL,
        exception);
      if (GetDelegateThreadSupport(delegate_info) == MagickFalse)
        RelinquishSemaphoreInfo(constitute_semaphore);
      image=DestroyImageList(image);
      read_info->temporary=MagickTrue;
      (void) SetImageInfo(read_info,MagickFalse,exception);
      magick_info=GetMagickInfo(read_info->magick,exception);
      if ((magick_info == (const MagickInfo *) NULL) ||
          (GetImageDecoder(magick_info) == (DecodeImageHandler *) NULL))
        {
          if (IsAccessible(read_info->filename) != MagickFalse)
            (void) ThrowMagickException(exception,GetMagickModule(),
              MissingDelegateError,"NoDecodeDelegateForThisImageFormat","`%s'",
              read_info->filename);
          else
            ThrowFileException(exception,FileOpenError,"UnableToOpenBlob",
              read_info->filename);
          read_info=DestroyImageInfo(read_info);
          return((Image *) NULL);
        }
      if ((GetMagickThreadSupport(magick_info) & DecoderThreadSupport) == 0)
        AcquireSemaphoreInfo(&constitute_semaphore);
      image=(Image *) (GetImageDecoder(magick_info))(read_info,exception);
      if ((GetMagickThreadSupport(magick_info) & DecoderThreadSupport) == 0)
        RelinquishSemaphoreInfo(constitute_semaphore);
    }
  if (read_info->temporary != MagickFalse)
    {
      (void) RelinquishUniqueFileResource(read_info->filename);
      read_info->temporary=MagickFalse;
      if (image != (Image *) NULL)
        (void) CopyMagickString(image->filename,filename,MaxTextExtent);
    }
  if (image == (Image *) NULL)
    {
      read_info=DestroyImageInfo(read_info);
      return(image);
    }
  if (exception->severity >= ErrorException)
    (void) LogMagickEvent(ExceptionEvent,GetMagickModule(),
      "Coder (%s) generated an image despite an error (%d), "
      "notify the developers",image->magick,exception->severity);
  if (IsBlobTemporary(image) != MagickFalse)
    (void) RelinquishUniqueFileResource(read_info->filename);
  if ((GetNextImageInList(image) != (Image *) NULL) &&
      (IsSceneGeometry(read_info->scenes,MagickFalse) != MagickFalse))
    {
      Image
        *clones;

      clones=CloneImages(image,read_info->scenes,exception);
      if (clones == (Image *) NULL)
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "SubimageSpecificationReturnsNoImages","`%s'",read_info->filename);
      else
        {
          image=DestroyImageList(image);
          image=GetFirstImageInList(clones);
        }
    }
  if (GetBlobError(image) != MagickFalse)
    {
      ThrowFileException(exception,FileOpenError,
        "AnErrorHasOccurredReadingFromFile",read_info->filename);
      image=DestroyImageList(image);
      read_info=DestroyImageInfo(read_info);
      return((Image *) NULL);
    }
  for (next=image; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    const StringInfo
      *profile;

    next->taint=MagickFalse;
    if (next->magick_columns == 0)
      next->magick_columns=next->columns;
    if (next->magick_rows == 0)
      next->magick_rows=next->rows;
    (void) CopyMagickString(next->magick,magick,MaxTextExtent);
    (void) CopyMagickString(next->magick_filename,magick_filename,
      MaxTextExtent);
    if (IsBlobTemporary(image) != MagickFalse)
      (void) CopyMagickString(next->filename,filename,MaxTextExtent);
    value=GetImageProperty(next,"EXIF:Orientation");
    if (value != (char *) NULL)
      {
        next->orientation=(OrientationType) atol(value);
        (void) DeleteImageProperty(next,"EXIF:Orientation");
      }
    value=GetImageProperty(next,"TIFF:XResolution");
    if (value == (char *) NULL)
      value=GetImageProperty(next,"EXIF:XResolution");
    if (value != (char *) NULL)
      {
        geometry_info.rho=next->x_resolution;
        geometry_info.sigma=1.0;
        flags=ParseGeometry(value,&geometry_info);
        if (geometry_info.sigma != 0)
          next->x_resolution=geometry_info.rho/geometry_info.sigma;
        (void) DeleteImageProperty(next,"EXIF:XResolution");
        (void) DeleteImageProperty(next,"TIFF:XResolution");
      }
    value=GetImageProperty(next,"TIFF:YResolution");
    if (value == (char *) NULL)
      value=GetImageProperty(next,"EXIF:YResolution");
    if (value != (char *) NULL)
      {
        geometry_info.rho=next->y_resolution;
        geometry_info.sigma=1.0;
        flags=ParseGeometry(value,&geometry_info);
        if (geometry_info.sigma != 0)
          next->y_resolution=geometry_info.rho/geometry_info.sigma;
        (void) DeleteImageProperty(next,"EXIF:YResolution");
        (void) DeleteImageProperty(next,"TIFF:YResolution");
      }
    value=GetImageProperty(next,"TIFF:ResolutionUnit");
    if (value == (char *) NULL)
      value=GetImageProperty(next,"EXIF:YResolution");
    if (value != (char *) NULL)
      {
        next->units=(ResolutionType) (atoi(value)-1);
        (void) DeleteImageProperty(next,"EXIF:ResolutionUnit");
        (void) DeleteImageProperty(next,"TIFF:ResolutionUnit");
      }
    if (next->page.width == 0)
      next->page.width=next->columns;
    if (next->page.height == 0)
      next->page.height=next->rows;
    (void) SyncImageOptions(read_info,next);
    if (*read_info->filename != '\0')
      {
        const char
          *option;

        option=GetImageOption(read_info,"caption");
        if (option != (const char *) NULL)
          (void) SetImageProperty(next,"caption",InterpretImageProperties(
            read_info,next,option));
        option=GetImageOption(read_info,"comment");
        if (option != (const char *) NULL)
          (void) SetImageProperty(next,"comment",InterpretImageProperties(
            read_info,next,option));
        option=GetImageOption(read_info,"label");
        if (option != (const char *) NULL)
          (void) SetImageProperty(next,"label",InterpretImageProperties(
            read_info,next,option));
      }
    if (LocaleCompare(next->magick,"TEXT") == 0)
      (void) ParseAbsoluteGeometry("0x0+0+0",&next->page);
    if ((read_info->extract != (char *) NULL) &&
        (read_info->stream == (StreamHandler) NULL))
      {
        RectangleInfo
          geometry;

        flags=ParseAbsoluteGeometry(read_info->extract,&geometry);
        if ((next->columns != geometry.width) ||
            (next->rows != geometry.height))
          {
            if (((flags & XValue) != 0) || ((flags & YValue) != 0))
              {
                Image
                  *crop_image;

                crop_image=CropImage(next,&geometry,exception);
                if (crop_image != (Image *) NULL)
                  ReplaceImageInList(&next,crop_image);
              }
            else
              if (((flags & WidthValue) != 0) || ((flags & HeightValue) != 0))
                {
                  Image
                    *size_image;

                  flags=ParseSizeGeometry(next,read_info->extract,&geometry);
                  size_image=ResizeImage(next,geometry.width,geometry.height,
                    next->filter,next->blur,exception);
                  if (size_image != (Image *) NULL)
                    ReplaceImageInList(&next,size_image);
                }
          }
      }
    profile=GetImageProfile(next,"icc");
    if (profile == (const StringInfo *) NULL)
      profile=GetImageProfile(next,"icm");
    if (profile != (const StringInfo *) NULL)
      {
        image->color_profile.length=GetStringInfoLength(profile);
        image->color_profile.info=GetStringInfoDatum(profile);
      }
    profile=GetImageProfile(next,"iptc");
    if (profile == (const StringInfo *) NULL)
      profile=GetImageProfile(next,"8bim");
    if (profile != (const StringInfo *) NULL)
      {
        image->iptc_profile.length=GetStringInfoLength(profile);
        image->iptc_profile.info=GetStringInfoDatum(profile);
      }
    if (read_info->verbose != MagickFalse)
      (void) IdentifyImage(next,stdout,MagickFalse);
    image=next;
  }
  read_info=DestroyImageInfo(read_info);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e a d I m a g e s                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadImages() reads a list of image names from a file and then returns the
%  images as a linked list.
%
%  The format of the ReadImage method is:
%
%      Image *ReadImages(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: Method ReadImage returns a pointer to the image after
%      reading.  A null image is returned if there is a memory shortage or
%      if the image cannot be read.
%
%    o image_info: The list of filenames are defined in the filename member of
%      this structure.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
static Image *ReadImages(const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    *command,
    **images;

  Image
    *image;

  ImageInfo
    *read_info;

  int
    number_images;

  register Image
    *next;

  register long
    i;

  /*
    Read image list from a file.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  command=FileToString(image_info->filename+1,~0,exception);
  if (command == (char *) NULL)
    return((Image *) NULL);
  StripString(command);
  images=StringToArgv(command,&number_images);
  command=DestroyString(command);
  /*
    Read the images into a linked list.
  */
  read_info=CloneImageInfo(image_info);
  image=NewImageList();
  for (i=1; i < number_images; i++)
  {
    (void) CopyMagickString(read_info->filename,images[i],MaxTextExtent);
    next=ReadImage(read_info,exception);
    if (next == (Image *) NULL)
      continue;
    AppendImageToList(&image,next);
  }
  read_info=DestroyImageInfo(read_info);
  for (i=1; i < number_images; i++)
    images[i]=DestroyString(images[i]);
  images=(char **) RelinquishMagickMemory(images);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e a d I n l i n e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadInlineImage() reads a Base64-encoded inline image or image sequence.
%  The method returns a NULL if there is a memory shortage or if the image
%  cannot be read.  On failure, a NULL image is returned and exception
%  describes the reason for the failure.
%
%  The format of the ReadInlineImage method is:
%
%      Image *ReadInlineImage(const ImageInfo *image_info,const char *content,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o content: The image encoded in Base64.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *ReadInlineImage(const ImageInfo *image_info,
  const char *content,ExceptionInfo *exception)
{
  Image
    *image;

  ImageInfo
    *read_info;

  unsigned char
    *blob;

  size_t
    length;

  register const char
    *p;

  image=NewImageList();
  for (p=content; (*p != ',') && (*p != '\0'); p++);
  if (*p == '\0')
    ThrowReaderException(CorruptImageWarning,"CorruptImage");
  p++;
  length=0;
  blob=Base64Decode(p,&length);
  if (length == 0)
    ThrowReaderException(CorruptImageWarning,"CorruptImage");
  read_info=CloneImageInfo(image_info);
  (void) SetImageInfoProgressMonitor(read_info,(MagickProgressMonitor) NULL,
    (void *) NULL);
  image=BlobToImage(read_info,blob,length,exception);
  blob=(unsigned char *) RelinquishMagickMemory(blob);
  read_info=DestroyImageInfo(read_info);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteImage() writes an image or an image sequence to a file or filehandle.
%  If writing to a file on disk, the name is defined by the filename member of
%  the image structure.  Write() returns MagickFalse is these is a memory
%  shortage or if the image cannot be written.  Check the exception member of
%  image to determine the cause for any failure.
%
%  The format of the WriteImage method is:
%
%      MagickBooleanType WriteImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o image: The image.
%
*/

static void SyncImageInfo(ImageInfo *image_info,Image *image,
  ExceptionInfo *exception)
{
  const MagickInfo
    *magick_info;

  QuantumInfo
    quantum_info;

  (void) CopyMagickString(image->filename,image_info->filename,MaxTextExtent);
  if (image_info->extract != (char *) NULL)
    (void) ParseAbsoluteGeometry(image_info->extract,&image->extract_info);
  if (image_info->depth != 0)
    (void) SetImageDepth(image,image_info->depth);
  else
    if (image->taint != MagickFalse)
      image->depth=QuantumDepth;
  if (image_info->compression != UndefinedCompression)
    image->compression=image_info->compression;
  if (image_info->quality != UndefinedCompressionQuality)
    image->quality=image_info->quality;
  if (image_info->interlace != UndefinedInterlace)
    image->interlace=image_info->interlace;
  if (image_info->orientation != UndefinedOrientation)
    image->orientation=image_info->orientation;
  if (image_info->endian != UndefinedEndian)
    image->endian=image_info->endian;
  magick_info=GetMagickInfo(image_info->magick,exception);
  if ((magick_info == (const MagickInfo *) NULL) ||
      (GetMagickEndianSupport(magick_info) == MagickFalse))
    image->endian=UndefinedEndian;
  GetQuantumInfo(image_info,&quantum_info);
  if ((quantum_info.format == UndefinedQuantumFormat) &&
      (IsHighDynamicRangeImage(image,exception) != MagickFalse))
    (void) SetImageOption(image_info,"quantum:format","floating-point");
}

MagickExport MagickBooleanType WriteImage(const ImageInfo *image_info,
  Image *image)
{
  char
    filename[MaxTextExtent];

  const DelegateInfo
    *delegate_info;

  const MagickInfo
    *magick_info;

  ExceptionInfo
    *sans_exception;

  ImageInfo
    *write_info;

  MagickBooleanType
    status,
    temporary;

  /*
    Determine image type from filename prefix or suffix (e.g. image.jpg).
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  sans_exception=AcquireExceptionInfo();
  write_info=CloneImageInfo(image_info);
  (void) CopyMagickString(write_info->filename,image->filename,MaxTextExtent);
  if (*write_info->magick == '\0')
    (void) CopyMagickString(write_info->magick,image->magick,MaxTextExtent);
  (void) SetImageInfo(write_info,MagickTrue,sans_exception);
  if (LocaleCompare(write_info->magick,"clipmask") == 0)
    {
      if (image->clip_mask == (Image *) NULL)
        {
          (void) ThrowMagickException(&image->exception,GetMagickModule(),
            OptionError,"NoClipPathDefined","`%s'",image->filename);
          return(MagickFalse);
        }
      image=image->clip_mask;
      (void) SetImageInfo(write_info,MagickTrue,sans_exception);
    }
  (void) CopyMagickString(filename,image->filename,MaxTextExtent);
  SyncImageInfo(write_info,image,sans_exception);
  (void) SyncImageProfiles(image);
  if ((GetPreviousImageInList(image) == (Image *) NULL) &&
      (GetNextImageInList(image) == (Image *) NULL) &&
      (write_info->page == (char *) NULL) &&
      (IsTaintImage(image) == MagickFalse))
    {
      delegate_info=GetDelegateInfo(image->magick,write_info->magick,
        &image->exception);
      if ((delegate_info != (const DelegateInfo *) NULL) &&
          (GetDelegateMode(delegate_info) == 0) &&
          (IsAccessible(image->magick_filename) != MagickFalse))
        {
          /*
            Process image with bi-modal delegate.
          */
          (void) CopyMagickString(image->filename,image->magick_filename,
            MaxTextExtent);
          status=InvokeDelegate(write_info,image,image->magick,
            write_info->magick,&image->exception);
          write_info=DestroyImageInfo(write_info);
          (void) CopyMagickString(image->filename,filename,MaxTextExtent);
          sans_exception=DestroyExceptionInfo(sans_exception);
          return(status);
        }
    }
  status=MagickFalse;
  temporary=MagickFalse;
  magick_info=GetMagickInfo(write_info->magick,sans_exception);
  if ((magick_info != (const MagickInfo *) NULL) &&
      (GetMagickSeekableStream(magick_info) != MagickFalse))
    {
      char
        filename[MaxTextExtent];

      (void) CopyMagickString(filename,image->filename,MaxTextExtent);
      status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
      (void) CopyMagickString(image->filename,filename,MaxTextExtent);
      if (status != MagickFalse)
        {
          if (IsBlobSeekable(image) == MagickFalse)
            {
              /*
                A seekable stream is required by the encoder.
              */
              (void) CopyMagickString(write_info->filename,image->filename,
                MaxTextExtent);
              (void) AcquireUniqueFilename(image->filename);
              temporary=MagickTrue;
            }
          CloseBlob(image);
        }
    }
  if ((magick_info != (const MagickInfo *) NULL) &&
      (GetImageEncoder(magick_info) != (EncodeImageHandler *) NULL))
    {
      /*
        Call appropriate image writer based on image type.
      */
      if ((GetMagickThreadSupport(magick_info) & EncoderThreadSupport) == 0)
        AcquireSemaphoreInfo(&constitute_semaphore);
      status=GetImageEncoder(magick_info)(write_info,image);
      if ((GetMagickThreadSupport(magick_info) & EncoderThreadSupport) == 0)
        RelinquishSemaphoreInfo(constitute_semaphore);
    }
  else
    {
      delegate_info=GetDelegateInfo((char *) NULL,write_info->magick,
        &image->exception);
      if (delegate_info != (DelegateInfo *) NULL)
        {
          /*
            Process the image with delegate.
          */
          *write_info->filename='\0';
          if (GetDelegateThreadSupport(delegate_info) == MagickFalse)
            AcquireSemaphoreInfo(&constitute_semaphore);
          status=InvokeDelegate(write_info,image,(char *) NULL,
            write_info->magick,&image->exception);
          if (GetDelegateThreadSupport(delegate_info) == MagickFalse)
            RelinquishSemaphoreInfo(constitute_semaphore);
          (void) CopyMagickString(image->filename,filename,MaxTextExtent);
        }
      else
        {
          magick_info=GetMagickInfo(write_info->magick,sans_exception);
          if ((write_info->affirm == MagickFalse) &&
              (magick_info == (const MagickInfo *) NULL))
            {
              (void) CopyMagickString(write_info->magick,image->magick,
                MaxTextExtent);
              magick_info=GetMagickInfo(write_info->magick,&image->exception);
            }
          if ((magick_info == (const MagickInfo *) NULL) ||
              (GetImageEncoder(magick_info) == (EncodeImageHandler *) NULL))
            (void) ThrowMagickException(&image->exception,GetMagickModule(),
              MissingDelegateError,"NoEncodeDelegateForThisImageFormat","`%s'",
              image->filename);
          else
            {
              /*
                Call appropriate image writer based on image type.
              */
              if ((GetMagickThreadSupport(magick_info) & EncoderThreadSupport) == 0)
                AcquireSemaphoreInfo(&constitute_semaphore);
              status=GetImageEncoder(magick_info)(write_info,image);
              if ((GetMagickThreadSupport(magick_info) & EncoderThreadSupport) == 0)
                RelinquishSemaphoreInfo(constitute_semaphore);
            }
        }
    }
  if (GetBlobError(image) != MagickFalse)
    ThrowFileException(&image->exception,FileOpenError,
      "AnErrorHasOccurredWritingToFile",image->filename);
  if (temporary == MagickTrue)
    {
      /*
        Copy temporary image file to permanent.
      */
      status=OpenBlob(write_info,image,ReadBinaryBlobMode,&image->exception);
      if (status != MagickFalse)
        status=ImageToFile(image,write_info->filename,&image->exception);
      (void) RelinquishUniqueFileResource(image->filename);
      (void) CopyMagickString(image->filename,write_info->filename,
        MaxTextExtent);
      CloseBlob(image);
    }
  if ((LocaleCompare(write_info->magick,"info") != 0) &&
      (write_info->verbose != MagickFalse))
    (void) IdentifyImage(image,stdout,MagickFalse);
  write_info=DestroyImageInfo(write_info);
  sans_exception=DestroyExceptionInfo(sans_exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e I m a g e s                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteImages() writes an image sequence.
%
%  The format of the WriteImages method is:
%
%      MagickBooleanType WriteImages(const ImageInfo *image_info,Image *images,
%        const char *filename,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o images: The image list.
%
%    o filename: The image filename.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType WriteImages(const ImageInfo *image_info,
  Image *images,const char *filename,ExceptionInfo *exception)
{
  ExceptionInfo
    *sans_exception;

  ImageInfo
    *write_info;

  MagickStatusType
    status;

  register Image
    *p;

  /*
    Write converted imagess.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(images != (Image *) NULL);
  assert(images->signature == MagickSignature);
  if (images->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  assert(exception != (ExceptionInfo *) NULL);
  write_info=CloneImageInfo(image_info);
  images=GetFirstImageInList(images);
  if (filename != (const char *) NULL)
    for (p=images; p != (Image *) NULL; p=GetNextImageInList(p))
      (void) CopyMagickString(p->filename,filename,MaxTextExtent);
  (void) CopyMagickString(write_info->filename,images->filename,MaxTextExtent);
  if (*write_info->magick == '\0')
    (void) CopyMagickString(write_info->magick,images->magick,MaxTextExtent);
  sans_exception=AcquireExceptionInfo();
  (void) SetImageInfo(write_info,MagickTrue,sans_exception);
  sans_exception=DestroyExceptionInfo(sans_exception);
  p=images;
  for ( ; GetNextImageInList(p) != (Image *) NULL; p=GetNextImageInList(p))
    if (p->scene >= GetNextImageInList(p)->scene)
      {
        register long
          i;

        /*
          Generate consistent scene numbers.
        */
        i=0;
        for (p=images; p != (Image *) NULL; p=GetNextImageInList(p))
          p->scene=(unsigned long) i++;
        break;
      }
  status=MagickTrue;
  for (p=images; p != (Image *) NULL; p=GetNextImageInList(p))
  {
    status&=WriteImage(write_info,p);
    GetImageException(p,exception);
    if (write_info->adjoin != MagickFalse)
      break;
  }
  write_info=DestroyImageInfo(write_info);
  return(status != 0 ? MagickTrue : MagickFalse);
}
