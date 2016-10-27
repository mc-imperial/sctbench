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

  MagickCore Binary Large OBjects private methods.
*/
#ifndef _MAGICKCORE_BLOB_PRIVATE_H
#define _MAGICKCORE_BLOB_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include "magick/image.h"
#include "magick/stream.h"

typedef enum
{
  UndefinedBlobMode,
  ReadBlobMode,
  ReadBinaryBlobMode,
  WriteBlobMode,
  WriteBinaryBlobMode
} BlobMode;

typedef int
  *(*BlobFifo)(const Image *,const void *,const size_t);

extern MagickExport BlobInfo
  *CloneBlobInfo(const BlobInfo *),
  *ReferenceBlob(BlobInfo *);

extern MagickExport char
  *ReadBlobString(Image *,char *);

extern MagickExport double
  ReadBlobDouble(Image *);

extern MagickExport float
  ReadBlobFloat(Image *);

extern MagickExport int
  EOFBlob(const Image *),
  ReadBlobByte(Image *);

extern MagickExport  MagickBooleanType
  OpenBlob(const ImageInfo *,Image *,const BlobMode,ExceptionInfo *),
  UnmapBlob(void *,const size_t);

extern MagickExport MagickOffsetType
  SeekBlob(Image *,const MagickOffsetType,const int),
  TellBlob(const Image *image);

extern MagickExport MagickSizeType
  ReadBlobLongLong(Image *);

extern MagickExport ssize_t
  ReadBlob(Image *,const size_t,unsigned char *),
  WriteBlob(Image *,const size_t,const unsigned char *),
  WriteBlobByte(Image *,const unsigned char),
  WriteBlobFloat(Image *,const float),
  WriteBlobLong(Image *,const unsigned long),
  WriteBlobShort(Image *,const unsigned short),
  WriteBlobLSBLong(Image *,const unsigned long),
  WriteBlobLSBShort(Image *,const unsigned short),
  WriteBlobMSBLong(Image *,const unsigned long),
  WriteBlobMSBShort(Image *,const unsigned short),
  WriteBlobString(Image *,const char *);

extern MagickExport unsigned char
  *DetachBlob(BlobInfo *),
  *MapBlob(int,const MapMode,const MagickOffsetType,const size_t);

extern MagickExport unsigned long
  ReadBlobLong(Image *),
  ReadBlobLSBLong(Image *),
  ReadBlobMSBLong(Image *);

extern MagickExport unsigned short
  ReadBlobShort(Image *),
  ReadBlobLSBShort(Image *),
  ReadBlobMSBShort(Image *);

extern MagickExport void
  AttachBlob(BlobInfo *,const void *,const size_t),
  CloseBlob(Image *),
  GetBlobInfo(BlobInfo *),
  MSBOrderLong(unsigned char *,const size_t),
  MSBOrderShort(unsigned char *,const size_t);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
