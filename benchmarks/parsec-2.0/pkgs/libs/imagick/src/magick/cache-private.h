/*
  Copyright 1999-2007 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.
  
  You may not use this file except in compliance with the License.
  obtain a copy of the License at
  
    http://www.imagemagick.org/script/license.php
  
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore cache private methods.
*/
#ifndef _MAGICKCORE_CACHE_PRIVATE_H
#define _MAGICKCORE_CACHE_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <time.h>
#include "magick/semaphore.h"

typedef enum
{
  UndefinedCache,
  MemoryCache,
  MapCache,
  DiskCache
} CacheType;

typedef void
  *Cache;

typedef const IndexPacket
  *(*AcquireIndexesFromHandler)(const Image *);

typedef IndexPacket
  *(*GetIndexesFromHandler)(const Image *);

typedef MagickBooleanType
  (*SyncPixelHandler)(Image *);

typedef const PixelPacket
  *(*AcquirePixelHandler)(const Image *,const VirtualPixelMethod,const long,
    const long,const unsigned long,const unsigned long,ExceptionInfo *);

typedef PixelPacket
  (*AcquireOnePixelFromHandler)(const Image *,const VirtualPixelMethod,
    const long,const long,ExceptionInfo *);

typedef PixelPacket
  (*GetOnePixelFromHandler)(Image *,const long,const long);

typedef PixelPacket
  *(*GetPixelHandler)(Image *,const long,const long,const unsigned long,
    const unsigned long);

typedef PixelPacket
  *(*GetPixelsFromHandler)(const Image *);

typedef PixelPacket
  *(*SetPixelHandler)(Image *,const long,const long,const unsigned long,
    const unsigned long);

typedef void
  (*DestroyPixelHandler)(Image *);

typedef struct _CacheMethods
{
  AcquireIndexesFromHandler
    acquire_indexes_from_handler;

  AcquireOnePixelFromHandler
    acquire_one_pixel_from_handler;

  AcquirePixelHandler
    acquire_pixel_handler;

  DestroyPixelHandler
    destroy_pixel_handler;

  GetIndexesFromHandler
    get_indexes_from_handler;

  GetOnePixelFromHandler
    get_one_pixel_from_handler;

  GetPixelHandler
    get_pixel_handler;

  GetPixelsFromHandler
    get_pixels_from_handler;

  SetPixelHandler
    set_pixel_handler;

  SyncPixelHandler
    sync_pixel_handler;
} CacheMethods;

typedef struct _NexusInfo NexusInfo;

typedef struct _CacheInfo
{
  unsigned long
    id;

  NexusInfo
    *nexus_info;

  unsigned long
    number_views;

  ClassType
    storage_class;

  ColorspaceType
    colorspace;

  CacheType
    type;

  MagickBooleanType
    mapped;

  unsigned long
    columns,
    rows;

  MagickOffsetType
    offset;

  MagickSizeType
    length;

  PixelPacket
    *pixels;

  IndexPacket
    *indexes;

  VirtualPixelMethod
    virtual_pixel_method;

  PixelPacket
    virtual_pixel;

  int
    file;

  MagickSizeType
    serial_number;

  char
    filename[MaxTextExtent],
    cache_filename[MaxTextExtent];

  CacheMethods
    methods;

  MagickBooleanType
    debug;

  unsigned long
    thread;

  long
    reference_count;

  SemaphoreInfo
    *semaphore;

  unsigned long
    signature;
} CacheInfo;

extern MagickExport Cache
  DestroyCacheInfo(Cache),
  ReferenceCache(Cache);

extern MagickExport ClassType
  GetCacheClass(const Cache);

extern MagickExport ColorspaceType
  GetCacheColorspace(const Cache);

extern MagickExport const IndexPacket
  *AcquireNexusIndexes(const Cache,const unsigned long);

extern MagickExport IndexPacket
  *GetNexusIndexes(const Cache,const unsigned long);

extern MagickExport MagickBooleanType
  GetCacheInfo(Cache *);

extern MagickExport PixelPacket
  *GetNexusPixels(const Cache,const unsigned long);

extern MagickExport unsigned long
  GetNexus(Cache);

extern MagickExport void
  CloneCacheMethods(Cache,const Cache),
  DestroyCacheNexus(Cache,const unsigned long),
  GetCacheMethods(CacheMethods *),
  SetCacheMethods(Cache,CacheMethods *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
