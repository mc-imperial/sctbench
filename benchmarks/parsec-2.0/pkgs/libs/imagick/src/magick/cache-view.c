/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      CCCC   AAA    CCCC  H   H  EEEEE                       %
%                     C      A   A  C      H   H  E                           %
%                     C      AAAAA  C      HHHHH  EEE                         %
%                     C      A   A  C      H   H  E                           %
%                      CCCC  A   A   CCCC  H   H  EEEEE                       %
%                                                                             %
%                        V   V  IIIII  EEEEE  W   W                           %
%                        V   V    I    E      W   W                           %
%                        V   V    I    EEE    W W W                           %
%                         V V     I    E      WW WW                           %
%                          V    IIIII  EEEEE  W   W                           %
%                                                                             %
%                                                                             %
%                       ImageMagick Cache View Methods                        %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                               February 2000                                 %
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
#include "magick/cache.h"
#include "magick/cache-private.h"
#include "magick/cache-view.h"
#include "magick/memory_.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/string_.h"

/*
  Typedef declarations.
*/
struct _ViewInfo
{
  unsigned long
    id;

  Image
    *image;

  VirtualPixelMethod
    virtual_pixel_method;

  MagickBooleanType
    debug;

  unsigned long
    signature;
};

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e C a c h e V i e w I n d e x e s                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireCacheViewIndexes() returns the indexes associated with the specified
%  view.
%
%  The format of the AcquireCacheViewIndexes method is:
%
%      const IndexPacket *AcquireCacheViewIndexes(const ViewInfo *view_info)
%
%  A description of each parameter follows:
%
%    o view_info: The address of a structure of type ViewInfo.
%
*/
MagickExport const IndexPacket *AcquireCacheViewIndexes(
  const ViewInfo *view_info)
{
  assert(view_info != (ViewInfo *) NULL);
  assert(view_info->signature == MagickSignature);
  assert(view_info->image != (Image *) NULL);
  if (view_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      view_info->image->filename);
  return(AcquireNexusIndexes(view_info->image->cache,view_info->id));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e C a c h e V i e w P i x e l s                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireCacheViewPixels() gets pixels from the in-memory or disk pixel cache
%  as defined by the geometry parameters.   A pointer to the pixels is returned
%  if the pixels are transferred, otherwise a NULL is returned.
%
%  The format of the AcquireCacheViewPixels method is:
%
%      const PixelPacket *AcquireCacheViewPixels(const ViewInfo *view_info,
%        const long x,const long y,const unsigned long columns,
%        const unsigned long rows,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o view_info: The address of a structure of type ViewInfo.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport const PixelPacket *AcquireCacheViewPixels(
  const ViewInfo *view_info,const long x,const long y,
  const unsigned long columns,const unsigned long rows,ExceptionInfo *exception)
{
  const PixelPacket
    *pixels;

  assert(view_info != (ViewInfo *) NULL);
  assert(view_info->signature == MagickSignature);
  assert(view_info->image != (Image *) NULL);
  if (view_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      view_info->image->filename);
  pixels=AcquireCacheNexus(view_info->image,view_info->virtual_pixel_method,x,y,
    columns,rows,view_info->id,exception);
  return(pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e O n e C a c h e V i e w P i x e l                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireCacheViewPixels() returns a single pixel at the specified (x,y) location.
%  The image background color is returned if an error occurs.  If you plan to
%  modify the pixel, use GetOneCacheViewPixel() instead.
%
%  The format of the AcquireOneCacheViewPixel method is:
%
%      PixelPacket AcquireOneCacheViewPixel(const ViewInfo *view_info,
%        const long x,const long y,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o view_info: The address of a structure of type ViewInfo.
%
%    o x,y:  These values define the offset of the pixel.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport PixelPacket AcquireOneCacheViewPixel(const ViewInfo *view_info,
  const long x,const long y,ExceptionInfo *exception)
{
  assert(view_info != (ViewInfo *) NULL);
  assert(view_info->signature == MagickSignature);
  assert(view_info->image != (Image *) NULL);
  if (view_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      view_info->image->filename);
  return(AcquireOnePixel(view_info->image,x,y,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e C a c h e V i e w                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneCacheView()  makes an exact copy of the specified cache view.
%
%  The format of the CloneCacheView method is:
%
%      ViewInfo *CloneCacheView(const ViewInfo *view_info)
%
%  A description of each parameter follows:
%
%    o view_info: The cache view.
%
*/
MagickExport ViewInfo *CloneCacheView(const ViewInfo *view_info)
{
  ViewInfo
    *clone_view;

  assert(view_info != (ViewInfo *) NULL);
  assert(view_info->signature == MagickSignature);
  if (view_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      view_info->image->filename);
  clone_view=(ViewInfo *) AcquireMagickMemory(sizeof(*clone_view));
  if (clone_view == (ViewInfo *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  (void) ResetMagickMemory(clone_view,0,sizeof(*clone_view));
  clone_view->image=ReferenceImage(view_info->image);
  clone_view->id=GetNexus(clone_view->image->cache);
  clone_view->virtual_pixel_method=view_info->virtual_pixel_method;
  clone_view->debug=view_info->debug;
  clone_view->signature=MagickSignature;
  return(clone_view);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o s e C a c h e V i e w                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloseCacheView() closes the specified view returned by a previous call to
%  OpenCacheView().
%
%  The format of the CloseCacheView method is:
%
%      ViewInfo *CloseCacheView(ViewInfo *view_info)
%
%  A description of each parameter follows:
%
%    o view_info: The address of a structure of type ViewInfo.
%
*/
MagickExport ViewInfo *CloseCacheView(ViewInfo *view_info)
{
  assert(view_info != (ViewInfo *) NULL);
  assert(view_info->signature == MagickSignature);
  assert(view_info->image != (Image *) NULL);
  if (view_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      view_info->image->filename);
  if (view_info->id != 0)
    DestroyCacheNexus(view_info->image->cache,view_info->id);
  view_info->image=DestroyImage(view_info->image);
  view_info->signature=(~MagickSignature);
  view_info=(ViewInfo *) RelinquishMagickMemory(view_info);
  return(view_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t C a c h e V i e w C o l o r s p a c e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCacheViewColorspace() returns the image colorspace associated with the
%  specified view.
%
%  The format of the GetCacheViewColorspace method is:
%
%      ColorspaceType GetCacheViewColorspace(const ViewInfo *view_info)
%
%  A description of each parameter follows:
%
%    o view_info: The address of a structure of type ViewInfo.
%
*/
MagickExport ColorspaceType GetCacheViewColorspace(const ViewInfo *view_info)
{
  assert(view_info != (ViewInfo *) NULL);
  assert(view_info->signature == MagickSignature);
  assert(view_info->image != (Image *) NULL);
  if (view_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      view_info->image->filename);
  return(view_info->image->colorspace);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t C a c h e V i e w E x c e p t i o n                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCacheViewException() returns the image exception associated with the
%  specified view.
%
%  The format of the GetCacheViewException method is:
%
%      ExceptionInfo GetCacheViewException(const ViewInfo *view_info)
%
%  A description of each parameter follows:
%
%    o view_info: The address of a structure of type ViewInfo.
%
*/
MagickExport ExceptionInfo *GetCacheViewException(const ViewInfo *view_info)
{
  assert(view_info != (ViewInfo *) NULL);
  assert(view_info->signature == MagickSignature);
  assert(view_info->image != (Image *) NULL);
  if (view_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      view_info->image->filename);
  return(&view_info->image->exception);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t C a c h e V i e w I n d e x e s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCacheViewIndexes() returns the indexes associated with the specified
%  view.
%
%  The format of the GetCacheViewIndexes method is:
%
%      IndexPacket *GetCacheViewIndexes(const ViewInfo *view_info)
%
%  A description of each parameter follows:
%
%    o view_info: The address of a structure of type ViewInfo.
%
*/
MagickExport IndexPacket *GetCacheViewIndexes(const ViewInfo *view_info)
{
  assert(view_info != (ViewInfo *) NULL);
  assert(view_info->signature == MagickSignature);
  assert(view_info->image != (Image *) NULL);
  if (view_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      view_info->image->filename);
  return(GetNexusIndexes(view_info->image->cache,view_info->id));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t C a c h e V i e w P i x e l s                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCacheViewPixels() gets pixels from the in-memory or disk pixel cache as
%  defined by the geometry parameters.   A pointer to the pixels is returned if
%  the pixels are transferred, otherwise a NULL is returned.
%
%  The format of the GetCacheViewPixels method is:
%
%      PixelPacket *GetCacheViewPixels(ViewInfo *view_info,const long x,
%        const long y,const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o view_info: The address of a structure of type ViewInfo.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
*/
MagickExport PixelPacket *GetCacheViewPixels(ViewInfo *view_info,const long x,
  const long y,const unsigned long columns,const unsigned long rows)
{
  assert(view_info != (ViewInfo *) NULL);
  assert(view_info->signature == MagickSignature);
  assert(view_info->image != (Image *) NULL);
  if (view_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      view_info->image->filename);
  return(GetCacheNexus(view_info->image,x,y,columns,rows,view_info->id));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t C a c h e V i e w S t o r a g e C l a s s                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCacheViewStorageClass() returns the image storage class  associated with
%  the specified view.
%
%  The format of the GetCacheViewStorageClass method is:
%
%      ClassType GetCacheViewStorageClass(const ViewInfo *view_info)
%
%  A description of each parameter follows:
%
%    o view_info: The address of a structure of type ViewInfo.
%
*/
MagickExport ClassType GetCacheViewStorageClass(const ViewInfo *view_info)
{
  assert(view_info != (ViewInfo *) NULL);
  assert(view_info->signature == MagickSignature);
  assert(view_info->image != (Image *) NULL);
  if (view_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      view_info->image->filename);
  return(view_info->image->storage_class);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t O n e C a c h e V i e w P i x e l                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOneCacheViewPixel() returns a single pixel at the specified (x,y)
%  location.  The image background color is returned if an error occurs.
%
%  The format of the GetOneCacheViewPixel method is:
%
%      PixelPacket GetOneCacheViewPixel(const ViewInfo *view_info,
%        const long x,const long y)
%
%  A description of each parameter follows:
%
%    o view_info: The address of a structure of type ViewInfo.
%
%    o x,y:  These values define the offset of the pixel.
%
*/
MagickExport PixelPacket GetOneCacheViewPixel(const ViewInfo *view_info,
  const long x,const long y)
{
  assert(view_info != (ViewInfo *) NULL);
  assert(view_info->signature == MagickSignature);
  assert(view_info->image != (Image *) NULL);
  if (view_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      view_info->image->filename);
  return(GetOnePixel(view_info->image,x,y));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   O p e n C a c h e V i e w                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OpenCacheView() opens a view into the pixel cache, using the
%  VirtualPixelMethod that is defined within the given image itself.
%
%  The format of the OpenCacheView method is:
%
%      ViewInfo *OpenCacheView(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
MagickExport ViewInfo *OpenCacheView(const Image *image)
{
  ViewInfo
    *view_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  view_info=(ViewInfo *) AcquireMagickMemory(sizeof(*view_info));
  if (view_info == (ViewInfo *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  (void) ResetMagickMemory(view_info,0,sizeof(*view_info));
  view_info->image=ReferenceImage((Image *) image);
  view_info->id=GetNexus(view_info->image->cache);
  view_info->virtual_pixel_method=GetImageVirtualPixelMethod(image);
  view_info->debug=IsEventLogging();
  view_info->signature=MagickSignature;
  return(view_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t C a c h e V i e w                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetCacheView() gets pixels from the in-memory or disk pixel cache as
%  defined by the geometry parameters.   A pointer to the pixels is returned
%  if the pixels are transferred, otherwise a NULL is returned.
%
%  The format of the SetCacheView method is:
%
%      PixelPacket *SetCacheView(ViewInfo *view_info,const long x,const long y,
%        const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o view_info: The address of a structure of type ViewInfo.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
*/
MagickExport PixelPacket *SetCacheView(ViewInfo *view_info,const long x,
  const long y,const unsigned long columns,const unsigned long rows)
{
  assert(view_info != (ViewInfo *) NULL);
  assert(view_info->signature == MagickSignature);
  assert(view_info->image != (Image *) NULL);
  if (view_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      view_info->image->filename);
  return(SetCacheNexus(view_info->image,x,y,columns,rows,view_info->id));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t C a c h e V i e w S t o r a g e C l a s s                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetCacheViewStorageClass() sets the image storage class associated with
%  the specified view.
%
%  The format of the SetCacheViewStorageClass method is:
%
%      MagickBooleanType SetCacheViewStorageClass(ViewInfo *view_info,
%        const ClassType storage_class)
%
%  A description of each parameter follows:
%
%    o view_info: The address of a structure of type ViewInfo.
%
%    o storage_class: The image storage class: PseudoClass or DirectClass.
%
*/
MagickExport MagickBooleanType SetCacheViewStorageClass(ViewInfo *view_info,
  const ClassType storage_class)
{
  assert(view_info != (ViewInfo *) NULL);
  assert(view_info->signature == MagickSignature);
  assert(view_info->image != (Image *) NULL);
  if (view_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      view_info->image->filename);
  return(SetImageStorageClass(view_info->image,storage_class));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t C a c h e V i e w V i r t u a l P i x e l M e t h o d               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetCacheViewVirtualPixelMethod() sets the virtual pixel method associated
%  with the specified cache view.
%
%  The format of the SetCacheViewVirtualPixelMethod method is:
%
%      MagickBooleanType SetCacheViewVirtualPixelMethod(ViewInfo *view_info,
%        const VirtualPixelMethod virtual_pixel_method)
%
%  A description of each parameter follows:
%
%    o view_info: The address of a structure of type ViewInfo.
%
%    o virtual_pixel_method: The virtual pixel method.
%
*/
MagickExport MagickBooleanType SetCacheViewVirtualPixelMethod(
  ViewInfo *view_info,const VirtualPixelMethod virtual_pixel_method)
{
  assert(view_info != (ViewInfo *) NULL);
  assert(view_info->signature == MagickSignature);
  assert(view_info->image != (Image *) NULL);
  if (view_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      view_info->image->filename);
  view_info->virtual_pixel_method=virtual_pixel_method;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S y n c C a c h e V i e w                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncCacheView() saves the view_info pixels to the in-memory or disk cache.
%  The method returns MagickTrue if the pixel region is synced, otherwise
%  MagickFalse.
%
%  The format of the SyncCacheView method is:
%
%      MagickBooleanType SyncCacheView(ViewInfo *view_info)
%
%  A description of each parameter follows:
%
%    o view_info: The address of a structure of type ViewInfo.
%
*/
MagickExport MagickBooleanType SyncCacheView(ViewInfo *view_info)
{
  assert(view_info != (ViewInfo *) NULL);
  assert(view_info->signature == MagickSignature);
  assert(view_info->image != (Image *) NULL);
  if (view_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      view_info->image->filename);
  return(SyncCacheNexus(view_info->image,view_info->id));
}
