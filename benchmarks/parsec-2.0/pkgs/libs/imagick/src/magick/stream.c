/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  SSSSS  TTTTT  RRRR   EEEEE   AAA   M   M                   %
%                  SS       T    R   R  E      A   A  MM MM                   %
%                   SSS     T    RRRR   EEE    AAAAA  M M M                   %
%                     SS    T    R R    E      A   A  M   M                   %
%                  SSSSS    T    R  R   EEEEE  A   A  M   M                   %
%                                                                             %
%                                                                             %
%                      ImageMagick Pixel Stream Methods                       %
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
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/cache.h"
#include "magick/cache-private.h"
#include "magick/color-private.h"
#include "magick/composite-private.h"
#include "magick/constitute.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/memory_.h"
#include "magick/quantum.h"
#include "magick/semaphore.h"
#include "magick/stream.h"
#include "magick/stream-private.h"
#include "magick/string_.h"

/*
  Typedef declaractions.
*/
struct _StreamInfo
{
  const ImageInfo
    *image_info;

  const Image
    *image;

  Image
    *stream;

  QuantumInfo
    *quantum_info;

  char
    *map;

  StorageType
    storage_type;

  unsigned char
    *pixels;

  RectangleInfo
    extract_info;

  long
    y;

  ExceptionInfo
    *exception;

  const void
    *client_data;

  unsigned long
    signature;
};

/*
  Declare pixel cache interfaces.
*/
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static const PixelPacket
  *AcquirePixelStream(const Image *,const VirtualPixelMethod,const long,
    const long,const unsigned long,const unsigned long,ExceptionInfo *);

static PixelPacket
  AcquireOnePixelFromStream(const Image *,const VirtualPixelMethod,const long,
    const long,ExceptionInfo *),
  GetOnePixelFromStream(Image *,const long,const long),
  *GetPixelStream(Image *,const long,const long,const unsigned long,
    const unsigned long),
  *GetPixelsFromStream(const Image *),
  *SetPixelStream(Image *,const long,const long,const unsigned long,
    const unsigned long);

static MagickBooleanType
  StreamImagePixels(const StreamInfo *,const Image *,ExceptionInfo *),
  SyncPixelStream(Image *);

static void
  DestroyPixelStream(Image *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A c q u i r e I n d e x e s F r o m S t r e a m                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireIndexesFromStream() returns the indexes associated with the last call to
%  SetPixelStream() or AcquirePixelStream().
%
%  The format of the AcquireIndexesFromStream() method is:
%
%      const IndexPacket *AcquireIndexesFromStream(const Image *image)
%
%  A description of each parameter follows:
%
%    o indexes: return the indexes associated with the last call to
%      SetPixelStream() or AcquirePixelStream().
%
%    o image: The image.
%
*/
static const IndexPacket *AcquireIndexesFromStream(const Image *image)
{
  CacheInfo
    *cache_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  return(cache_info->indexes);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A c q u i r e O n e P i x e l F r o m S t r e a m                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireOnePixelFromStream() returns a single pixel at the specified (x.y)
%  location.  The image background color is returned if an error occurs.
%
%  The format of the AcquireOnePixelFromStream() method is:
%
%      PixelPacket *AcquireOnePixelFromStream(const Image image,
%        const VirtualPixelMethod virtual_pixel_method,const long x,
%        const long y,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pixels: AcquireOnePixelFromStream() returns a pixel at the specified
%      (x,y) location.
%
%    o image: The image.
%
%    o virtual_pixel_method: The virtual pixel method.
%
%    o x,y:  These values define the location of the pixel to return.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static PixelPacket AcquireOnePixelFromStream(const Image *image,
  const VirtualPixelMethod virtual_pixel_method,const long x,const long y,
  ExceptionInfo *exception)
{
  register const PixelPacket
    *pixel;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  pixel=AcquirePixelStream(image,virtual_pixel_method,x,y,1,1,exception);
  if (pixel != (PixelPacket *) NULL)
    return(*pixel);
  return(image->background_color);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A c q u i r e P i x e l S t r e a m                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquirePixelStream() gets pixels from the in-memory or disk pixel cache as
%  defined by the geometry parameters.   A pointer to the pixels is returned if
%  the pixels are transferred, otherwise a NULL is returned.  For streams this
%  method is a no-op.
%
%  The format of the AcquirePixelStream() method is:
%
%      const PixelPacket *AcquirePixelStream(const Image *image,
%        const VirtualPixelMethod virtual_pixel_method,const long x,
%        const long y,const unsigned long columns,const unsigned long rows,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o virtual_pixel_method: The virtual pixel method.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

static inline void AcquireStreamPixels(CacheInfo *cache_info)
{
  assert(cache_info != (CacheInfo *) NULL);
  assert(cache_info->length == (MagickSizeType) ((size_t) cache_info->length));
  cache_info->pixels=(PixelPacket *) MapBlob(-1,IOMode,0,(size_t)
    cache_info->length);
  if (cache_info->pixels != (PixelPacket *) NULL)
    {
      cache_info->mapped=MagickTrue;
      return;
    }
  cache_info->pixels=(PixelPacket *) AcquireMagickMemory((size_t)
    cache_info->length);
  if (cache_info->pixels != (PixelPacket *) NULL)
    (void) ResetMagickMemory(cache_info->pixels,0,(size_t)
      cache_info->length);
  cache_info->mapped=MagickFalse;
}

static const PixelPacket *AcquirePixelStream(const Image *image,
  const VirtualPixelMethod magick_unused(virtual_pixel_method),const long x,
  const long y,const unsigned long columns,const unsigned long rows,
  ExceptionInfo *exception)
{
  CacheInfo
    *cache_info;

  MagickSizeType
    number_pixels;

  size_t
    length;

  /*
    Validate pixel cache geometry.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((x < 0) || (y < 0) || ((x+(long) columns) > (long) image->columns) ||
      ((y+(long) rows) > (long) image->rows) || (columns == 0) || (rows == 0))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),StreamError,
        "ImageDoesNotContainTheStreamGeometry","`%s'",image->filename);
      return((PixelPacket *) NULL);
    }
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->type == UndefinedCache)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),StreamError,
        "PixelCacheIsNotOpen","`%s'",image->filename);
      return((PixelPacket *) NULL);
    }
  /*
    Pixels are stored in a temporary buffer until they are synced to the cache.
  */
  number_pixels=(MagickSizeType) columns*rows;
  length=(size_t) number_pixels*sizeof(PixelPacket);
  if ((image->storage_class == PseudoClass) ||
      (image->colorspace == CMYKColorspace))
    length+=number_pixels*sizeof(IndexPacket);
  cache_info->length=length;
  AcquireStreamPixels(cache_info);
  if (cache_info->pixels == (void *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  cache_info->length=(MagickSizeType) length;
  cache_info->indexes=(IndexPacket *) NULL;
  if ((image->storage_class == PseudoClass) ||
      (image->colorspace == CMYKColorspace))
    cache_info->indexes=(IndexPacket *) (cache_info->pixels+number_pixels);
  return(cache_info->pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A c q u i r e S t r e a m I n f o                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireStreamInfo() allocates the StreamInfo structure.
%
%  The format of the AcquireStreamInfo method is:
%
%      StreamInfo *AcquireStreamInfo(const ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
*/
MagickExport StreamInfo *AcquireStreamInfo(const ImageInfo *image_info)
{
  StreamInfo
    *stream_info;

  stream_info=(StreamInfo *) AcquireMagickMemory(sizeof(*stream_info));
  if (stream_info == (StreamInfo *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  (void) ResetMagickMemory(stream_info,0,sizeof(*stream_info));
  stream_info->pixels=(unsigned char *) AcquireMagickMemory(
    sizeof(*stream_info->pixels));
  if (stream_info->pixels == (unsigned char *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  stream_info->map=ConstantString("RGB");
  stream_info->storage_type=CharPixel;
  stream_info->stream=AllocateImage(image_info);
  stream_info->signature=MagickSignature;
  return(stream_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y P i x e l S t r e a m                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyPixelStream() deallocates memory associated with the pixel stream.
%
%  The format of the DestroyPixelStream() method is:
%
%      void DestroyPixelStream(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/

static inline void RelinquishStreamPixels(CacheInfo *cache_info)
{
  assert(cache_info != (CacheInfo *) NULL);
  if (cache_info->mapped == MagickFalse)
    (void) RelinquishMagickMemory(cache_info->pixels);
  else
    (void) UnmapBlob(cache_info->pixels,(size_t) cache_info->length);
  cache_info->pixels=(PixelPacket *) NULL;
  cache_info->indexes=(IndexPacket *) NULL;
}

static void DestroyPixelStream(Image *image)
{
  CacheInfo
    *cache_info;

  MagickBooleanType
    destroy;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  destroy=MagickFalse;
  AcquireSemaphoreInfo(&cache_info->semaphore);
  cache_info->reference_count--;
  if (cache_info->reference_count == 0)
    destroy=MagickTrue;
  RelinquishSemaphoreInfo(cache_info->semaphore);
  if (destroy == MagickFalse)
    return;
  RelinquishStreamPixels(cache_info);
  if (cache_info->nexus_info != (NexusInfo *) NULL)
    {
      register long
        id;

      for (id=0; id < (long) cache_info->number_views; id++)
        DestroyCacheNexus(cache_info,(unsigned long) id);
      cache_info->nexus_info=(NexusInfo *) RelinquishMagickMemory(
        cache_info->nexus_info);
    }
  if (cache_info->semaphore != (SemaphoreInfo *) NULL)
    cache_info->semaphore=DestroySemaphoreInfo(cache_info->semaphore);
  cache_info=(CacheInfo *) RelinquishMagickMemory(cache_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y S t r e a m I n f o                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyStreamInfo() destroys memory associated with the StreamInfo
%  structure.
%
%  The format of the DestroyStreamInfo method is:
%
%      StreamInfo *DestroyStreamInfo(StreamInfo *stream_info)
%
%  A description of each parameter follows:
%
%    o stream_info: The stream info.
%
*/
MagickExport StreamInfo *DestroyStreamInfo(StreamInfo *stream_info)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(stream_info != (StreamInfo *) NULL);
  assert(stream_info->signature == MagickSignature);
  if (stream_info->map != (char *) NULL)
    stream_info->map=DestroyString(stream_info->map);
  if (stream_info->pixels != (unsigned char *) NULL)
    stream_info->pixels=(unsigned char *) RelinquishMagickMemory(
      stream_info->pixels);
  if (stream_info->stream != (Image *) NULL)
    {
      CloseBlob(stream_info->stream);
      stream_info->stream=DestroyImage(stream_info->stream);
    }
  if (stream_info->quantum_info != (QuantumInfo *) NULL)
    stream_info->quantum_info=(QuantumInfo *) RelinquishMagickMemory(
      stream_info->quantum_info);
  stream_info->signature=(~MagickSignature);
  stream_info=(StreamInfo *) RelinquishMagickMemory(stream_info);
  return(stream_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t I n d e x e s F r o m S t r e a m                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetIndexesFromStream() returns the indexes associated with the last call to
%  SetPixelStream() or GetPixelStream().
%
%  The format of the GetIndexesFromStream() method is:
%
%      IndexPacket *GetIndexesFromStream(const Image *image)
%
%  A description of each parameter follows:
%
%    o indexes: Method GetIndexesFromStream() returns the indexes associated
%      with the last call to SetPixelStream() or GetPixelStream().
%
%    o image: The image.
%
*/
static IndexPacket *GetIndexesFromStream(const Image *image)
{
  CacheInfo
    *cache_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  return(cache_info->indexes);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t O n e P i x e l F r o m S t r e a m                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOnePixelFromStream() returns a single pixel at the specified (x,y)
%  location.  The image background color is returned if an error occurs.
%
%  The format of the GetOnePixelFromStream() method is:
%
%      PixelPacket *GetOnePixelFromStream(const Image image,const long x,
%        const long y)
%
%  A description of each parameter follows:
%
%    o pixels: Method GetOnePixelFromStream returns a pixel at the specified
%      (x,y) location.
%
%    o image: The image.
%
%    o x,y:  These values define the location of the pixel to return.
%
*/
static PixelPacket GetOnePixelFromStream(Image *image,const long x,const long y)
{
  register PixelPacket
    *pixel;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  pixel=GetPixelStream(image,x,y,1,1);
  if (pixel != (PixelPacket *) NULL)
    return(*pixel);
  return(image->background_color);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t P i x e l S t r e a m                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelStream() gets pixels from the in-memory or disk pixel cache as
%  defined by the geometry parameters.   A pointer to the pixels is returned if
%  the pixels are transferred, otherwise a NULL is returned.  For streams
%  this method is a no-op.
%
%  The format of the GetPixelStream() method is:
%
%      PixelPacket *GetPixelStream(Image *image,const long x,const long y,
%        const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
*/
static PixelPacket *GetPixelStream(Image *image,const long x,const long y,
  const unsigned long columns,const unsigned long rows)
{
  PixelPacket
    *pixels;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  pixels=SetPixelStream(image,x,y,columns,rows);
  return(pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t P i x e l F r o m S t e a m                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelsFromStream() returns the pixels associated with the last call to
%  SetPixelStream() or GetPixelStream().
%
%  The format of the GetPixelsFromStream() method is:
%
%      PixelPacket *GetPixelsFromStream(const Image image)
%
%  A description of each parameter follows:
%
%    o pixels: Method GetPixelsFromStream returns the pixels associated with
%      the last call to SetPixelStream() or GetPixelStream().
%
%    o image: The image.
%
*/
static PixelPacket *GetPixelsFromStream(const Image *image)
{
  CacheInfo
    *cache_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  return(cache_info->pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t S t r e a m I n f o C l i e n t D a t a                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetStreamInfoClientData() gets the stream info client data.
%
%  The format of the SetStreamInfoClientData method is:
%
%      const void *GetStreamInfoClientData(StreamInfo *stream_info)
%
%  A description of each parameter follows:
%
%    o stream_info: The stream info.
%
*/
MagickExport const void *GetStreamInfoClientData(StreamInfo *stream_info)
{
  assert(stream_info != (StreamInfo *) NULL);
  assert(stream_info->signature == MagickSignature);
  return(stream_info->client_data);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   O p e n S t r e a m                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OpenStream() opens a stream for writing by the StreamImage() method.
%
%  The format of the OpenStream method is:
%
%       MagickBooleanType OpenStream(const ImageInfo *image_info,
%        StreamInfo *stream_info,const char *filename,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o stream_info: The stream info.
%
%    o filename: The stream filename.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType OpenStream(const ImageInfo *image_info,
  StreamInfo *stream_info,const char *filename,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  (void) CopyMagickString(stream_info->stream->filename,filename,MaxTextExtent);
  status=OpenBlob(image_info,stream_info->stream,WriteBinaryBlobMode,exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d S t r e a m                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadStream() makes the image pixels available to a user supplied
%  callback method immediately upon reading a scanline with the ReadImage()
%  method.
%
%  The format of the ReadStream() method is:
%
%      Image *ReadStream(const ImageInfo *image_info,StreamHandler stream,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o stream: a callback method.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *ReadStream(const ImageInfo *image_info,StreamHandler stream,
  ExceptionInfo *exception)
{
  CacheMethods
    cache_methods;

  Image
    *image;

  ImageInfo
    *read_info;

  /*
    Stream image pixels.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  read_info=CloneImageInfo(image_info);
  (void) GetCacheInfo(&read_info->cache);
  GetCacheMethods(&cache_methods);
  cache_methods.acquire_pixel_handler=AcquirePixelStream;
  cache_methods.acquire_indexes_from_handler=AcquireIndexesFromStream;
  cache_methods.get_pixel_handler=GetPixelStream;
  cache_methods.set_pixel_handler=SetPixelStream;
  cache_methods.sync_pixel_handler=SyncPixelStream;
  cache_methods.get_pixels_from_handler=GetPixelsFromStream;
  cache_methods.get_indexes_from_handler=GetIndexesFromStream;
  cache_methods.acquire_one_pixel_from_handler=AcquireOnePixelFromStream;
  cache_methods.get_one_pixel_from_handler=GetOnePixelFromStream;
  cache_methods.destroy_pixel_handler=DestroyPixelStream;
  SetCacheMethods(read_info->cache,&cache_methods);
  read_info->stream=stream;
  image=ReadImage(read_info,exception);
  read_info=DestroyImageInfo(read_info);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S e t P i x e l S t r e a m                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetPixelStream() allocates an area to store image pixels as defined by the
%  region rectangle and returns a pointer to the area.  This area is
%  subsequently transferred from the pixel cache with method SyncPixelStream().
%  A pointer to the pixels is returned if the pixels are transferred,
%  otherwise a NULL is returned.
%
%  The format of the SetPixelStream() method is:
%
%      PixelPacket *SetPixelStream(Image *image,const long x,const long y,
%        const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o pixels: Method SetPixelStream returns a pointer to the pixels is
%      returned if the pixels are transferred, otherwise a NULL is returned.
%
%    o image: The image.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
*/
static PixelPacket *SetPixelStream(Image *image,const long x,const long y,
  const unsigned long columns,const unsigned long rows)
{
  CacheInfo
    *cache_info;

  MagickSizeType
    number_pixels;

  size_t
    length;

  StreamHandler
    stream_handler;

  /*
    Validate pixel cache geometry.
  */
  assert(image != (Image *) NULL);
  if ((x < 0) || (y < 0) || ((x+(long) columns) > (long) image->columns) ||
      ((y+(long) rows) > (long) image->rows) || (columns == 0) || (rows == 0))
    {
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        StreamError,"ImageDoesNotContainTheStreamGeometry","`%s'",
        image->filename);
      return((PixelPacket *) NULL);
    }
  stream_handler=GetBlobStreamHandler(image);
  if (stream_handler == (StreamHandler) NULL)
    {
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        StreamError,"NoStreamHandlerIsDefined","`%s'",image->filename);
      return((PixelPacket *) NULL);
    }
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  if ((image->storage_class != GetCacheClass(image->cache)) ||
      (image->colorspace != GetCacheColorspace(image->cache)))
    {
      if (GetCacheClass(image->cache) == UndefinedClass)
        (void) stream_handler(image,(const void *) NULL,(size_t)
          cache_info->columns);
      cache_info->storage_class=image->storage_class;
      cache_info->colorspace=image->colorspace;
      cache_info->columns=image->columns;
      cache_info->rows=image->rows;
      image->cache=cache_info;
    }
  /*
    Pixels are stored in a temporary buffer until they are synced to the cache.
  */
  cache_info->columns=columns;
  cache_info->rows=rows;
  number_pixels=(MagickSizeType) columns*rows;
  length=(size_t) number_pixels*sizeof(PixelPacket);
  if ((image->storage_class == PseudoClass) ||
      (image->colorspace == CMYKColorspace))
    length+=number_pixels*sizeof(IndexPacket);
  if (cache_info->pixels == (PixelPacket *) NULL)
    {
      cache_info->pixels=(PixelPacket *) AcquireMagickMemory(length);
      cache_info->length=(MagickSizeType) length;
    }
  else
    if (cache_info->length < (MagickSizeType) length)
      {
        cache_info->pixels=(PixelPacket *) ResizeMagickMemory(
          cache_info->pixels,length);
        cache_info->length=(MagickSizeType) length;
      }
  if (cache_info->pixels == (void *) NULL)
    ThrowFatalException(ResourceLimitFatalError,
      "UnableToAllocateImagePixels");
  cache_info->indexes=(IndexPacket *) NULL;
  if ((image->storage_class == PseudoClass) ||
      (image->colorspace == CMYKColorspace))
    cache_info->indexes=(IndexPacket *) (cache_info->pixels+number_pixels);
  return(cache_info->pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S e t S t r e a m I n f o C l i e n t D a t a                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetStreamInfoClientData() sets the stream info client data.
%
%  The format of the SetStreamInfoClientData method is:
%
%      void SetStreamInfoClientData(StreamInfo *stream_info,
%        const void *client_data)
%
%  A description of each parameter follows:
%
%    o stream_info: The stream info.
%
%    o client_data: The client data.
%
*/
MagickExport void SetStreamInfoClientData(StreamInfo *stream_info,
  const void *client_data)
{
  assert(stream_info != (StreamInfo *) NULL);
  assert(stream_info->signature == MagickSignature);
  stream_info->client_data=client_data;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S e t S t r e a m I n f o M a p                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetStreamInfoMap() sets the stream info map member.
%
%  The format of the SetStreamInfoMap method is:
%
%      void SetStreamInfoMap(StreamInfo *stream_info,const char *map)
%
%  A description of each parameter follows:
%
%    o stream_info: The stream info.
%
%    o map: The map.
%
*/
MagickExport void SetStreamInfoMap(StreamInfo *stream_info,const char *map)
{
  assert(stream_info != (StreamInfo *) NULL);
  assert(stream_info->signature == MagickSignature);
  (void) CloneString(&stream_info->map,map);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S e t S t r e a m I n f o S t o r a g e T y p e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetStreamInfoStorageType() sets the stream info storage type member.
%
%  The format of the SetStreamInfoStorageType method is:
%
%      void SetStreamInfoStorageType(StreamInfo *stream_info,
%        const StoreageType *storage_type)
%
%  A description of each parameter follows:
%
%    o stream_info: The stream info.
%
%    o storage_type: The storage type.
%
*/
MagickExport void SetStreamInfoStorageType(StreamInfo *stream_info,
  const StorageType storage_type)
{
  assert(stream_info != (StreamInfo *) NULL);
  assert(stream_info->signature == MagickSignature);
  stream_info->storage_type=storage_type;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S t r e a m I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StreamImage() streams pixels from an image and writes them in a user
%  defined format and storage type (e.g. RGBA as 8-bit unsigned char).
%
%  The format of he wStreamImage() method is:
%
%      Image *StreamImage(const ImageInfo *image_info,
%        StreamInfo *stream_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o stream_info: The stream info.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static size_t WriteStreamImage(const Image *image,const void *pixels,
  const size_t columns)
{
  RectangleInfo
    extract_info;

  size_t
    length,
    packet_size;

  ssize_t
    count;

  StreamInfo
    *stream_info;

  stream_info=(StreamInfo *) image->client_data;
  switch (stream_info->storage_type)
  {
    default: packet_size=sizeof(char); break;
    case CharPixel: packet_size=sizeof(char); break;
    case DoublePixel: packet_size=sizeof(double); break;
    case FloatPixel: packet_size=sizeof(float); break;
    case IntegerPixel: packet_size=sizeof(int); break;
    case LongPixel: packet_size=sizeof(long); break;
    case QuantumPixel: packet_size=sizeof(Quantum); break;
    case ShortPixel: packet_size=sizeof(unsigned short); break;
  }
  packet_size*=strlen(stream_info->map);
  length=packet_size*image->columns;
  if (image != stream_info->image)
    {
      ImageInfo
        *write_info;

      /*
        Prepare stream for writing.
      */
      stream_info->pixels=(unsigned char *) ResizeQuantumMemory(
        stream_info->pixels,length,sizeof(*stream_info->pixels));
      if (pixels == (unsigned char *) NULL)
        return(0);
      stream_info->image=image;
      write_info=CloneImageInfo(stream_info->image_info);
      (void) SetImageInfo(write_info,MagickFalse,stream_info->exception);
      if (write_info->extract != (char *) NULL)
        (void) ParseAbsoluteGeometry(write_info->extract,
          &stream_info->extract_info);
      stream_info->y=0;
      write_info=DestroyImageInfo(write_info);
    }
  extract_info=stream_info->extract_info;
  if ((extract_info.width == 0) ||
      (extract_info.height == 0))
    {
      /*
        Write all pixels to stream.
      */
      (void) StreamImagePixels(stream_info,image,stream_info->exception);
      count=WriteBlob(stream_info->stream,length,stream_info->pixels);
      stream_info->y++;
      return(count == 0 ? 0 : columns);
    }
  if ((stream_info->y < extract_info.y) ||
      (stream_info->y >= (long) (extract_info.y+extract_info.height)))
    {
      stream_info->y++;
      return(columns);
    }
  /*
    Write a portion of the pixel row to the stream.
  */
  (void) StreamImagePixels(stream_info,image,stream_info->exception);
  length=packet_size*extract_info.width;
  count=WriteBlob(stream_info->stream,length,stream_info->pixels+
    packet_size*extract_info.x);
  stream_info->y++;
  return(count == 0 ? 0 : columns);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport Image *StreamImage(const ImageInfo *image_info,
  StreamInfo *stream_info,ExceptionInfo *exception)
{
  Image
    *image;

  ImageInfo
    *read_info;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(stream_info != (StreamInfo *) NULL);
  assert(stream_info->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  read_info=CloneImageInfo(image_info);
  stream_info->image_info=image_info;
  stream_info->quantum_info=AcquireQuantumInfo(image_info);
  stream_info->exception=exception;
  read_info->client_data=(void *) stream_info;
  image=ReadStream(read_info,&WriteStreamImage,exception);
  read_info=DestroyImageInfo(read_info);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S t r e a m I m a g e P i x e l s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StreamImagePixels() extracts pixel data from an image and returns it in the
%  stream_info->pixels structure in the format as defined by
%  stream_info->quantum_info->map and stream_info->quantum_info->storage_type.
%
%  The format of the StreamImagePixels method is:
%
%      MagickBooleanType StreamImagePixels(const StreamInfo *stream_info,
%        const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o stream_info: The stream info.
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static MagickBooleanType StreamImagePixels(const StreamInfo *stream_info,
  const Image *image,ExceptionInfo *exception)
{
  QuantumInfo
    *quantum_info;

  QuantumType
    *quantum_map;

  register long
    i,
    x;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  size_t
    length;

  assert(stream_info != (StreamInfo *) NULL);
  assert(stream_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  length=strlen(stream_info->map);
  quantum_map=(QuantumType *) AcquireQuantumMemory(length,sizeof(*quantum_map));
  if (quantum_map == (QuantumType *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(MagickFalse);
    }
  for (i=0; i < (long) length; i++)
  {
    switch (stream_info->map[i])
    {
      case 'A':
      case 'a':
      {
        quantum_map[i]=AlphaQuantum;
        break;
      }
      case 'B':
      case 'b':
      {
        quantum_map[i]=BlueQuantum;
        break;
      }
      case 'C':
      case 'c':
      {
        quantum_map[i]=CyanQuantum;
        if (image->colorspace == CMYKColorspace)
          break;
        quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
        (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
          "ColorSeparatedImageRequired","`%s'",stream_info->map);
        return(MagickFalse);
      }
      case 'g':
      case 'G':
      {
        quantum_map[i]=GreenQuantum;
        break;
      }
      case 'I':
      case 'i':
      {
        quantum_map[i]=IndexQuantum;
        break;
      }
      case 'K':
      case 'k':
      {
        quantum_map[i]=BlackQuantum;
        if (image->colorspace == CMYKColorspace)
          break;
        quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
        (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
          "ColorSeparatedImageRequired","`%s'",stream_info->map);
        return(MagickFalse);
      }
      case 'M':
      case 'm':
      {
        quantum_map[i]=MagentaQuantum;
        if (image->colorspace == CMYKColorspace)
          break;
        quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
        (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
          "ColorSeparatedImageRequired","`%s'",stream_info->map);
        return(MagickFalse);
      }
      case 'o':
      case 'O':
      {
        quantum_map[i]=OpacityQuantum;
        break;
      }
      case 'P':
      case 'p':
      {
        quantum_map[i]=UndefinedQuantum;
        break;
      }
      case 'R':
      case 'r':
      {
        quantum_map[i]=RedQuantum;
        break;
      }
      case 'Y':
      case 'y':
      {
        quantum_map[i]=YellowQuantum;
        if (image->colorspace == CMYKColorspace)
          break;
        quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
        (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
          "ColorSeparatedImageRequired","`%s'",stream_info->map);
        return(MagickFalse);
      }
      default:
      {
        quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "UnrecognizedPixelMap","`%s'",stream_info->map);
        return(MagickFalse);
      }
    }
  }
  quantum_info=stream_info->quantum_info;
  switch (stream_info->storage_type)
  {
    case CharPixel:
    {
      register unsigned char
        *q;

      q=(unsigned char *) stream_info->pixels;
      if (LocaleCompare(stream_info->map,"BGR") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToChar(p->blue);
            *q++=ScaleQuantumToChar(p->green);
            *q++=ScaleQuantumToChar(p->red);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"BGRA") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToChar(p->blue);
            *q++=ScaleQuantumToChar(p->green);
            *q++=ScaleQuantumToChar(p->red);
            *q++=ScaleQuantumToChar((Quantum) (QuantumRange-p->opacity));
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"BGRP") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
              break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToChar(p->blue);
            *q++=ScaleQuantumToChar(p->green);
            *q++=ScaleQuantumToChar(p->red);
            *q++=ScaleQuantumToChar((Quantum) 0);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"I") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGB") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToChar(p->red);
            *q++=ScaleQuantumToChar(p->green);
            *q++=ScaleQuantumToChar(p->blue);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGBA") == 0)
        {
          p=GetPixels(image);
            if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToChar(p->red);
            *q++=ScaleQuantumToChar(p->green);
            *q++=ScaleQuantumToChar(p->blue);
            *q++=ScaleQuantumToChar((Quantum) (QuantumRange-p->opacity));
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGBP") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToChar(p->red);
            *q++=ScaleQuantumToChar(p->green);
            *q++=ScaleQuantumToChar(p->blue);
            *q++=ScaleQuantumToChar((Quantum) 0);
            p++;
          }
          break;
        }
      p=GetPixels(image);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      for (x=0; x < (long) GetPixelCacheArea(image); x++)
      {
        for (i=0; i < (long) length; i++)
        {
          *q=0;
          switch (quantum_map[i])
          {
            case RedQuantum:
            case CyanQuantum:
            {
              *q=ScaleQuantumToChar(p->red);
              break;
            }
            case GreenQuantum:
            case MagentaQuantum:
            {
              *q=ScaleQuantumToChar(p->green);
              break;
            }
            case BlueQuantum:
            case YellowQuantum:
            {
              *q=ScaleQuantumToChar(p->blue);
              break;
            }
            case AlphaQuantum:
            {
              *q=ScaleQuantumToChar((Quantum) (QuantumRange-p->opacity));
              break;
            }
            case OpacityQuantum:
            {
              *q=ScaleQuantumToChar(p->opacity);
              break;
            }
            case BlackQuantum:
            {
              if (image->colorspace == CMYKColorspace)
                *q=ScaleQuantumToChar(indexes[x]);
              break;
            }
            case IndexQuantum:
            {
              *q=ScaleQuantumToChar(PixelIntensityToQuantum(p));
              break;
            }
            default:
              break;
          }
          q++;
        }
        p++;
      }
      break;
    }
    case DoublePixel:
    {
      register double
        *q;

      q=(double *) stream_info->pixels;
      if (LocaleCompare(stream_info->map,"BGR") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(double) ((QuantumScale*p->blue)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(double) ((QuantumScale*p->green)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(double) ((QuantumScale*p->red)*quantum_info->scale+
              quantum_info->minimum);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"BGRA") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(double) ((QuantumScale*p->blue)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(double) ((QuantumScale*p->green)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(double) ((QuantumScale*p->red)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(double) ((QuantumScale*((Quantum) (QuantumRange-p->opacity)))*
              quantum_info->scale+quantum_info->minimum);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"BGRP") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(double) ((QuantumScale*p->blue)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(double) ((QuantumScale*p->green)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(double) ((QuantumScale*p->red)*quantum_info->scale+
              quantum_info->minimum);
            *q++=0.0;
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"I") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(double) ((QuantumScale*PixelIntensityToQuantum(p))*
              quantum_info->scale+quantum_info->minimum);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGB") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(double) ((QuantumScale*p->red)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(double) ((QuantumScale*p->green)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(double) ((QuantumScale*p->blue)*quantum_info->scale+
              quantum_info->minimum);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGBA") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(double) ((QuantumScale*p->red)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(double) ((QuantumScale*p->green)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(double) ((QuantumScale*p->blue)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(double) ((QuantumScale*((Quantum) (QuantumRange-p->opacity)))*
              quantum_info->scale+quantum_info->minimum);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGBP") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(double) ((QuantumScale*p->red)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(double) ((QuantumScale*p->green)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(double) ((QuantumScale*p->blue)*quantum_info->scale+
              quantum_info->minimum);
            *q++=0.0;
            p++;
          }
          break;
        }
      p=GetPixels(image);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      for (x=0; x < (long) GetPixelCacheArea(image); x++)
      {
        for (i=0; i < (long) length; i++)
        {
          *q=0;
          switch (quantum_map[i])
          {
            case RedQuantum:
            case CyanQuantum:
            {
              *q=(double) ((QuantumScale*p->red)*quantum_info->scale+
                quantum_info->minimum);
              break;
            }
            case GreenQuantum:
            case MagentaQuantum:
            {
              *q=(double) ((QuantumScale*p->green)*quantum_info->scale+
                quantum_info->minimum);
              break;
            }
            case BlueQuantum:
            case YellowQuantum:
            {
              *q=(double) ((QuantumScale*p->blue)*quantum_info->scale+
                quantum_info->minimum);
              break;
            }
            case AlphaQuantum:
            {
              *q=(double) ((QuantumScale*((Quantum) (QuantumRange-
                p->opacity)))*quantum_info->scale+quantum_info->minimum);
              break;
            }
            case OpacityQuantum:
            {
              *q=(double) ((QuantumScale*p->opacity)*quantum_info->scale+
                quantum_info->minimum);
              break;
            }
            case BlackQuantum:
            {
              if (image->colorspace == CMYKColorspace)
                *q=(double) ((QuantumScale*indexes[x])*quantum_info->scale+
                  quantum_info->minimum);
              break;
            }
            case IndexQuantum:
            {
              *q=(double) ((QuantumScale*PixelIntensityToQuantum(p))*
                quantum_info->scale+quantum_info->minimum);
              break;
            }
            default:
              *q=0;
          }
          q++;
        }
        p++;
      }
      break;
    }
    case FloatPixel:
    {
      register float
        *q;

      q=(float *) stream_info->pixels;
      if (LocaleCompare(stream_info->map,"BGR") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(float) ((QuantumScale*p->blue)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(float) ((QuantumScale*p->green)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(float) ((QuantumScale*p->red)*quantum_info->scale+
              quantum_info->minimum);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"BGRA") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(float) ((QuantumScale*p->blue)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(float) ((QuantumScale*p->green)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(float) ((QuantumScale*p->red)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(float) ((QuantumScale*(Quantum) (QuantumRange-p->opacity))*
              quantum_info->scale+quantum_info->minimum);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"BGRP") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(float) ((QuantumScale*p->blue)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(float) ((QuantumScale*p->green)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(float) ((QuantumScale*p->red)*quantum_info->scale+
              quantum_info->minimum);
            *q++=0.0;
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"I") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(float) ((QuantumScale*PixelIntensityToQuantum(p))*
              quantum_info->scale+quantum_info->minimum);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGB") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(float) ((QuantumScale*p->red)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(float) ((QuantumScale*p->green)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(float) ((QuantumScale*p->blue)*quantum_info->scale+
              quantum_info->minimum);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGBA") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(float) ((QuantumScale*p->red)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(float) ((QuantumScale*p->green)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(float) ((QuantumScale*p->blue)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(float) ((QuantumScale*((Quantum) (QuantumRange-p->opacity)))*
              quantum_info->scale+quantum_info->minimum);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGBP") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(float) ((QuantumScale*p->red)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(float) ((QuantumScale*p->green)*quantum_info->scale+
              quantum_info->minimum);
            *q++=(float) ((QuantumScale*p->blue)*quantum_info->scale+
              quantum_info->minimum);
            *q++=0.0;
            p++;
          }
          break;
        }
      p=GetPixels(image);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      for (x=0; x < (long) GetPixelCacheArea(image); x++)
      {
        for (i=0; i < (long) length; i++)
        {
          *q=0;
          switch (quantum_map[i])
          {
            case RedQuantum:
            case CyanQuantum:
            {
              *q=(float) ((QuantumScale*p->red)*quantum_info->scale+
                quantum_info->minimum);
              break;
            }
            case GreenQuantum:
            case MagentaQuantum:
            {
              *q=(float) ((QuantumScale*p->green)*quantum_info->scale+
                quantum_info->minimum);
              break;
            }
            case BlueQuantum:
            case YellowQuantum:
            {
              *q=(float) ((QuantumScale*p->blue)*quantum_info->scale+
                quantum_info->minimum);
              break;
            }
            case AlphaQuantum:
            {
              *q=(float) ((QuantumScale*((Quantum) (QuantumRange-
                p->opacity)))*quantum_info->scale+quantum_info->minimum);
              break;
            }
            case OpacityQuantum:
            {
              *q=(float) ((QuantumScale*p->opacity)*quantum_info->scale+
                quantum_info->minimum);
              break;
            }
            case BlackQuantum:
            {
              if (image->colorspace == CMYKColorspace)
                *q=(float) ((QuantumScale*indexes[x])*quantum_info->scale+
                  quantum_info->minimum);
              break;
            }
            case IndexQuantum:
            {
              *q=(float) ((QuantumScale*PixelIntensityToQuantum(p))*
                quantum_info->scale+quantum_info->minimum);
              break;
            }
            default:
              *q=0;
          }
          q++;
        }
        p++;
      }
      break;
    }
    case IntegerPixel:
    {
      register unsigned int
        *q;

      q=(unsigned int *) stream_info->pixels;
      if (LocaleCompare(stream_info->map,"BGR") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(unsigned int) ScaleQuantumToLong(p->blue);
            *q++=(unsigned int) ScaleQuantumToLong(p->green);
            *q++=(unsigned int) ScaleQuantumToLong(p->red);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"BGRA") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(unsigned int) ScaleQuantumToLong(p->blue);
            *q++=(unsigned int) ScaleQuantumToLong(p->green);
            *q++=(unsigned int) ScaleQuantumToLong(p->red);
            *q++=(unsigned int) ScaleQuantumToLong((Quantum) (QuantumRange-
              p->opacity));
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"BGRP") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(unsigned int) ScaleQuantumToLong(p->blue);
            *q++=(unsigned int) ScaleQuantumToLong(p->green);
            *q++=(unsigned int) ScaleQuantumToLong(p->red);
            *q++=0U;
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"I") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(unsigned int) ScaleQuantumToLong(
              PixelIntensityToQuantum(p));
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGB") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(unsigned int) ScaleQuantumToLong(p->red);
            *q++=(unsigned int) ScaleQuantumToLong(p->green);
            *q++=(unsigned int) ScaleQuantumToLong(p->blue);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGBA") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(unsigned int) ScaleQuantumToLong(p->red);
            *q++=(unsigned int) ScaleQuantumToLong(p->green);
            *q++=(unsigned int) ScaleQuantumToLong(p->blue);
            *q++=(unsigned int) ScaleQuantumToLong((Quantum)
              (QuantumRange-p->opacity));
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGBP") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=(unsigned int) ScaleQuantumToLong(p->red);
            *q++=(unsigned int) ScaleQuantumToLong(p->green);
            *q++=(unsigned int) ScaleQuantumToLong(p->blue);
            *q++=0U;
            p++;
          }
          break;
        }
      p=GetPixels(image);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      for (x=0; x < (long) GetPixelCacheArea(image); x++)
      {
        for (i=0; i < (long) length; i++)
        {
          *q=0;
          switch (quantum_map[i])
          {
            case RedQuantum:
            case CyanQuantum:
            {
              *q=(unsigned int) ScaleQuantumToLong(p->red);
              break;
            }
            case GreenQuantum:
            case MagentaQuantum:
            {
              *q=(unsigned int) ScaleQuantumToLong(p->green);
              break;
            }
            case BlueQuantum:
            case YellowQuantum:
            {
              *q=(unsigned int) ScaleQuantumToLong(p->blue);
              break;
            }
            case AlphaQuantum:
            {
              *q=(unsigned int) ScaleQuantumToLong((Quantum) (QuantumRange-
                p->opacity));
              break;
            }
            case OpacityQuantum:
            {
              *q=(unsigned int) ScaleQuantumToLong(p->opacity);
              break;
            }
            case BlackQuantum:
            {
              if (image->colorspace == CMYKColorspace)
                *q=(unsigned int) ScaleQuantumToLong(indexes[x]);
              break;
            }
            case IndexQuantum:
            {
              *q=(unsigned int)
                ScaleQuantumToLong(PixelIntensityToQuantum(p));
              break;
            }
            default:
              *q=0;
          }
          q++;
        }
        p++;
      }
      break;
    }
    case LongPixel:
    {
      register unsigned long
        *q;

      q=(unsigned long *) stream_info->pixels;
      if (LocaleCompare(stream_info->map,"BGR") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToLong(p->blue);
            *q++=ScaleQuantumToLong(p->green);
            *q++=ScaleQuantumToLong(p->red);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"BGRA") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToLong(p->blue);
            *q++=ScaleQuantumToLong(p->green);
            *q++=ScaleQuantumToLong(p->red);
            *q++=ScaleQuantumToLong((Quantum) (QuantumRange-p->opacity));
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"BGRP") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToLong(p->blue);
            *q++=ScaleQuantumToLong(p->green);
            *q++=ScaleQuantumToLong(p->red);
            *q++=0;
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"I") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToLong(PixelIntensityToQuantum(p));
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGB") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToLong(p->red);
            *q++=ScaleQuantumToLong(p->green);
            *q++=ScaleQuantumToLong(p->blue);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGBA") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToLong(p->red);
            *q++=ScaleQuantumToLong(p->green);
            *q++=ScaleQuantumToLong(p->blue);
            *q++=ScaleQuantumToLong((Quantum) (QuantumRange-p->opacity));
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGBP") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToLong(p->red);
            *q++=ScaleQuantumToLong(p->green);
            *q++=ScaleQuantumToLong(p->blue);
            *q++=0;
            p++;
          }
          break;
        }
      p=GetPixels(image);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      for (x=0; x < (long) GetPixelCacheArea(image); x++)
      {
        for (i=0; i < (long) length; i++)
        {
          *q=0;
          switch (quantum_map[i])
          {
            case RedQuantum:
            case CyanQuantum:
            {
              *q=ScaleQuantumToLong(p->red);
              break;
            }
            case GreenQuantum:
            case MagentaQuantum:
            {
              *q=ScaleQuantumToLong(p->green);
              break;
            }
            case BlueQuantum:
            case YellowQuantum:
            {
              *q=ScaleQuantumToLong(p->blue);
              break;
            }
            case AlphaQuantum:
            {
              *q=ScaleQuantumToLong((Quantum) (QuantumRange-p->opacity));
              break;
            }
            case OpacityQuantum:
            {
              *q=ScaleQuantumToLong(p->opacity);
              break;
            }
            case BlackQuantum:
            {
              if (image->colorspace == CMYKColorspace)
                *q=ScaleQuantumToLong(indexes[x]);
              break;
            }
            case IndexQuantum:
            {
              *q=ScaleQuantumToLong(PixelIntensityToQuantum(p));
              break;
            }
            default:
              break;
          }
          q++;
        }
        p++;
      }
      break;
    }
    case QuantumPixel:
    {
      register Quantum
        *q;

      q=(Quantum *) stream_info->pixels;
      if (LocaleCompare(stream_info->map,"BGR") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=p->blue;
            *q++=p->green;
            *q++=p->red;
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"BGRA") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=p->blue;
            *q++=p->green;
            *q++=p->red;
            *q++=(Quantum) (QuantumRange-p->opacity);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"BGRP") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=p->blue;
            *q++=p->green;
            *q++=p->red;
            *q++=0;
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"I") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=PixelIntensityToQuantum(p);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGB") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=p->red;
            *q++=p->green;
            *q++=p->blue;
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGBA") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=p->red;
            *q++=p->green;
            *q++=p->blue;
            *q++=(Quantum) (QuantumRange-p->opacity);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGBP") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=p->red;
            *q++=p->green;
            *q++=p->blue;
            *q++=0U;
            p++;
          }
          break;
        }
      p=GetPixels(image);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      for (x=0; x < (long) GetPixelCacheArea(image); x++)
      {
        for (i=0; i < (long) length; i++)
        {
          *q=(Quantum) 0;
          switch (quantum_map[i])
          {
            case RedQuantum:
            case CyanQuantum:
            {
              *q=p->red;
              break;
            }
            case GreenQuantum:
            case MagentaQuantum:
            {
              *q=p->green;
              break;
            }
            case BlueQuantum:
            case YellowQuantum:
            {
              *q=p->blue;
              break;
            }
            case AlphaQuantum:
            {
              *q=(Quantum) (QuantumRange-p->opacity);
              break;
            }
            case OpacityQuantum:
            {
              *q=p->opacity;
              break;
            }
            case BlackQuantum:
            {
              if (image->colorspace == CMYKColorspace)
                *q=indexes[x];
              break;
            }
            case IndexQuantum:
            {
              *q=(PixelIntensityToQuantum(p));
              break;
            }
            default:
              *q=0;
          }
          q++;
        }
        p++;
      }
      break;
    }
    case ShortPixel:
    {
      register unsigned short
        *q;

      q=(unsigned short *) stream_info->pixels;
      if (LocaleCompare(stream_info->map,"BGR") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToShort(p->blue);
            *q++=ScaleQuantumToShort(p->green);
            *q++=ScaleQuantumToShort(p->red);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"BGRA") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToShort(p->blue);
            *q++=ScaleQuantumToShort(p->green);
            *q++=ScaleQuantumToShort(p->red);
            *q++=ScaleQuantumToShort((Quantum) (QuantumRange-p->opacity));
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"BGRP") == 0)
        {
          p=GetPixels(image);
            if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToShort(p->blue);
            *q++=ScaleQuantumToShort(p->green);
            *q++=ScaleQuantumToShort(p->red);
            *q++=0;
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"I") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToShort(PixelIntensityToQuantum(p));
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGB") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToShort(p->red);
            *q++=ScaleQuantumToShort(p->green);
            *q++=ScaleQuantumToShort(p->blue);
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGBA") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToShort(p->red);
            *q++=ScaleQuantumToShort(p->green);
            *q++=ScaleQuantumToShort(p->blue);
            *q++=ScaleQuantumToShort((Quantum) (QuantumRange-p->opacity));
            p++;
          }
          break;
        }
      if (LocaleCompare(stream_info->map,"RGBP") == 0)
        {
          p=GetPixels(image);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) GetPixelCacheArea(image); x++)
          {
            *q++=ScaleQuantumToShort(p->red);
            *q++=ScaleQuantumToShort(p->green);
            *q++=ScaleQuantumToShort(p->blue);
            *q++=0;
            p++;
          }
          break;
        }
      p=GetPixels(image);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      for (x=0; x < (long) GetPixelCacheArea(image); x++)
      {
        for (i=0; i < (long) length; i++)
        {
          *q=0;
          switch (quantum_map[i])
          {
            case RedQuantum:
            case CyanQuantum:
            {
              *q=ScaleQuantumToShort(p->red);
              break;
            }
            case GreenQuantum:
            case MagentaQuantum:
            {
              *q=ScaleQuantumToShort(p->green);
              break;
            }
            case BlueQuantum:
            case YellowQuantum:
            {
              *q=ScaleQuantumToShort(p->blue);
              break;
            }
            case AlphaQuantum:
            {
              *q=ScaleQuantumToShort((Quantum) (QuantumRange-p->opacity));
              break;
            }
            case OpacityQuantum:
            {
              *q=ScaleQuantumToShort(p->opacity);
              break;
            }
            case BlackQuantum:
            {
              if (image->colorspace == CMYKColorspace)
                *q=ScaleQuantumToShort(indexes[x]);
              break;
            }
            case IndexQuantum:
            {
              *q=ScaleQuantumToShort(PixelIntensityToQuantum(p));
              break;
            }
            default:
              break;
          }
          q++;
        }
        p++;
      }
      break;
    }
    default:
    {
      quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "UnrecognizedPixelMap","`%s'",stream_info->map);
      break;
    }
  }
  quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S y n c P i x e l S t r e a m                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncPixelStream() calls the user supplied callback method with the latest
%  stream of pixels.
%
%  The format of the SyncPixelStream method is:
%
%      MagickBooleanType SyncPixelStream(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
static MagickBooleanType SyncPixelStream(Image *image)
{
  CacheInfo
    *cache_info;

  size_t
    length;

  StreamHandler
    stream_handler;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  stream_handler=GetBlobStreamHandler(image);
  if (stream_handler == (StreamHandler) NULL)
    {
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        StreamError,"NoStreamHandlerIsDefined","`%s'",image->filename);
      return(MagickFalse);
    }
  length=stream_handler(image,cache_info->pixels,(size_t) cache_info->columns);
  return(length == cache_info->columns ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e S t r e a m                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteStream() makes the image pixels available to a user supplied
%  callback method immediately upon writing pixel data with the WriteImage()
%  method.
%
%  The format of the WriteStream() method is:
%
%      MagickBooleanType WriteStream(const ImageInfo *image_info,Image *,
%        StreamHandler stream)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o stream: A callback method.
%
*/
MagickExport MagickBooleanType WriteStream(const ImageInfo *image_info,
  Image *image,StreamHandler stream)
{
  ImageInfo
    *write_info;

  MagickBooleanType
    status;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  write_info=CloneImageInfo(image_info);
  write_info->stream=stream;
  status=WriteImage(write_info,image);
  write_info=DestroyImageInfo(write_info);
  return(status);
}
