/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                        M   M  PPPP   EEEEE   GGGG                           %
%                        MM MM  P   P  E      G                               %
%                        M M M  PPPP   EEE    G  GG                           %
%                        M   M  P      E      G   G                           %
%                        M   M  P      EEEEE   GGGG                           %
%                                                                             %
%                                                                             %
%                       Read/Write MPEG Image Format.                         %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1999                                   %
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
#include "magick/constitute.h"
#include "magick/delegate.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/layer.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/resource_.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/module.h"
#include "magick/transform.h"
#include "magick/utility.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteMPEGImage(const ImageInfo *image_info,Image *image);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s M P E G                                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsMPEG() returns MagickTrue if the image format type, identified by the
%  magick string, is MPEG.
%
%  The format of the IsMPEG method is:
%
%      MagickBooleanType IsMPEG(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: This string is generally the first few bytes of an image file
%      or blob.
%
%    o length: Specifies the length of the magick string.
%
*/
static MagickBooleanType IsMPEG(const unsigned char *magick,const size_t length)
{
  if (length < 4)
    return(MagickFalse);
  if (memcmp(magick,"\000\000\001\263",4) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d M P E G I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadMPEGImage() reads an binary file in the MPEG video stream format
%  and returns it.  It allocates the memory necessary for the new Image
%  structure and returns a pointer to the new image.
%
%  The format of the ReadMPEGImage method is:
%
%      Image *ReadMPEGImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *ReadMPEGImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  Image
    *image,
    *images;

  ImageInfo
    *read_info;

  MagickBooleanType
    status;

  register long
    i;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=AllocateImage(image_info);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  CloseBlob(image);
  (void) DestroyImageList(image);
  /*
    Convert MPEG to PPM with delegate.
  */
  image=AllocateImage(image_info);
  read_info=CloneImageInfo(image_info);
  (void) InvokeDelegate(read_info,image,"mpeg-decode",(char *) NULL,exception);
  image=DestroyImage(image);
  /*
    Read PPM files.
  */
  images=NewImageList();
  for (i=(long) read_info->scene; ; i++)
  {
    (void) FormatMagickString(read_info->filename,MaxTextExtent,"%s%ld.ppm",
      read_info->unique,i);
    if (IsAccessible(read_info->filename) == MagickFalse)
      break;
    image=ReadImage(read_info,exception);
    if (image == (Image *) NULL)
      break;
    (void) CopyMagickString(image->magick,image_info->magick,MaxTextExtent);
    image->scene=(unsigned long) i;
    AppendImageToList(&images,image);
    if (read_info->number_scenes != 0)
      if (i >= (long) (read_info->scene+read_info->number_scenes-1))
        break;
  }
  /*
    Free resources.
  */
  for (i=0; ; i++)
  {
    (void) FormatMagickString(read_info->filename,MaxTextExtent,"%s%ld.ppm",
      read_info->unique,i);
    if (IsAccessible(read_info->filename) == MagickFalse)
      break;
    (void) RelinquishUniqueFileResource(read_info->filename);
  }
  read_info=DestroyImageInfo(read_info);
  return(images);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r M P E G I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterMPEGImage() adds attributes for the MPEG image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterMPEGImage method is:
%
%      unsigned long RegisterMPEGImage(void)
%
*/
ModuleExport unsigned long RegisterMPEGImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("MPEG");
  entry->decoder=(DecodeImageHandler *) ReadMPEGImage;
  entry->encoder=(EncodeImageHandler *) WriteMPEGImage;
  entry->magick=(IsImageFormatHandler *) IsMPEG;
  entry->blob_support=MagickFalse;
  entry->description=ConstantString("MPEG Video Stream");
  entry->module=ConstantString("MPEG");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("MPG");
  entry->decoder=(DecodeImageHandler *) ReadMPEGImage;
  entry->encoder=(EncodeImageHandler *) WriteMPEGImage;
  entry->magick=(IsImageFormatHandler *) IsMPEG;
  entry->blob_support=MagickFalse;
  entry->description=ConstantString("MPEG Video Stream");
  entry->module=ConstantString("MPEG");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("M2V");
  entry->decoder=(DecodeImageHandler *) ReadMPEGImage;
  entry->encoder=(EncodeImageHandler *) WriteMPEGImage;
  entry->magick=(IsImageFormatHandler *) IsMPEG;
  entry->blob_support=MagickFalse;
  entry->description=ConstantString("MPEG Video Stream");
  entry->module=ConstantString("MPEG");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r M P E G I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterMPEGImage() removes format registrations made by the
%  BIM module from the list of supported formats.
%
%  The format of the UnregisterBIMImage method is:
%
%      UnregisterMPEGImage(void)
%
*/
ModuleExport void UnregisterMPEGImage(void)
{
  (void) UnregisterMagickInfo("M2V");
  (void) UnregisterMagickInfo("MPEG");
  (void) UnregisterMagickInfo("MPG");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e M P E G I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteMPEGImage() writes an image to a file in MPEG video stream format.
%  Lawrence Livermore National Laboratory (LLNL) contributed code to adjust
%  the MPEG parameters to correspond to the compression quality setting.
%
%  The format of the WriteMPEGImage method is:
%
%      MagickBooleanType WriteMPEGImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
*/

static inline double MagickMax(const double x,const double y)
{
  if (x > y)
    return(x);
  return(y);
}

static inline double MagickMin(const double x,const double y)
{
  if (x < y)
    return(x);
  return(y);
}

static MagickBooleanType WriteMPEGParameterFiles(const ImageInfo *image_info,
  Image *image,const char *basename)
{
  char
    filename[MaxTextExtent];

  double
    delay,
    q;

  FILE
    *file,
    *parameter_file;

  long
    quant,
    vertical_factor;

  MagickBooleanType
    mpeg;

  register Image
    *p;

  register long
    i;

  ssize_t
    count;

  static int
    q_matrix[]=
    {
       8, 16, 19, 22, 26, 27, 29, 34,
      16, 16, 22, 24, 27, 29, 34, 37,
      19, 22, 26, 27, 29, 34, 34, 38,
      22, 22, 26, 27, 29, 34, 37, 40,
      22, 26, 27, 29, 32, 35, 40, 48,
      26, 27, 29, 32, 35, 40, 48, 58,
      26, 27, 29, 34, 38, 46, 56, 69,
      27, 29, 35, 38, 46, 56, 69, 83
    };

  /*
    Write parameter file (see mpeg2encode documentation for details).
  */
  file=MagickOpenStream(basename,"w");
  if (file == (FILE *) NULL)
    return(MagickFalse);
  (void) fprintf(file,"MPEG\n");  /* comment */
  (void) fprintf(file,"%s.%%d\n",image->filename); /* source frame file */
  (void) fprintf(file,"-\n");  /* reconstructed frame file */
  if (image->quality == UndefinedCompressionQuality)
    (void) fprintf(file,"-\n");  /* default intra quant matrix */
  else
    {
      /*
        Write intra quant matrix file.
      */
      (void) FormatMagickString(filename,MaxTextExtent,"%s.iqm",basename);
      (void) fprintf(file,"%s\n",filename);
      parameter_file=MagickOpenStream(filename,"w");
      if (parameter_file == (FILE *) NULL)
        return(MagickFalse);
      if (image->quality >= 75)
        {
          q=MagickMax(2.0*(image->quality-75.0),1.0);
          for (i=0; i < 64; i++)
          {
            quant=(long) MagickMin(MagickMax(q_matrix[i]/q,1.0),255.0);
            (void) fprintf(parameter_file," %ld",quant);
            if ((i % 8) == 7)
              (void) fprintf(parameter_file,"\n");
          }
        }
      else
        {
          q=MagickMax((75.0-image->quality)/8.0,1.0);
          for (i=0; i < 64; i++)
          {
            quant=(long) MagickMin(MagickMax(q*q_matrix[i]+0.5,1.0),255.0);
            (void) fprintf(parameter_file," %ld",quant);
            if ((i % 8) == 7)
              (void) fprintf(parameter_file,"\n");
          }
        }
      (void) fclose(parameter_file);
    }
  if (image->quality == UndefinedCompressionQuality)
    (void) fprintf(file,"-\n");  /* default non intra quant matrix */
  else
    {
      /*
        Write non intra quant matrix file.
      */
      (void) FormatMagickString(filename,MaxTextExtent,"%s.niq",basename);
      (void) fprintf(file,"%s\n",filename);
      parameter_file=MagickOpenStream(filename,"w");
      if (parameter_file == (FILE *) NULL)
        return(MagickFalse);
      q=MagickMin(MagickMax(66.0-(2*image->quality)/3.0,1.0),255);
      for (i=0; i < 64; i++)
      {
        (void) fprintf(parameter_file," %d",(int) q);
        if ((i % 8) == 7)
          (void) fprintf(parameter_file,"\n");
      }
      (void) fclose(parameter_file);
    }
  (void) fprintf(file,"%s.log\n",basename);  /* statistics log */
  (void) fprintf(file,"1\n");  /* input picture file format */
  count=0;
  for (p=image; p != (Image *) NULL; p=GetNextImageInList(p))
  {
    delay=100.0*p->delay/MagickMax(1.0*p->ticks_per_second,1.0);
    count+=(ssize_t) MagickMax((1.0*delay+1.0)/3.0,1.0);
  }
  (void) fprintf(file,"%lu\n",(unsigned long) count); /* number of frames */
  (void) fprintf(file,"0\n");  /* number of first frame */
  (void) fprintf(file,"00:00:00:00\n");  /* timecode of first frame */
  mpeg=LocaleCompare(image_info->magick,"M2V") != 0 ? MagickTrue : MagickFalse;
  if (image->quality > 98)
    (void) fprintf(file,"1\n");
  else
    (void) fprintf(file,"%d\n",mpeg != MagickFalse ? 12 : 15);
  if (image->quality > 98)
    (void) fprintf(file,"1\n");
  else
    (void) fprintf(file,"3\n");
  (void) fprintf(file,"%d\n",mpeg != MagickFalse ? 1 : 0);  /* ISO/IEC 11172-2 stream */
  (void) fprintf(file,"0\n");  /* select frame picture coding */
  (void) fprintf(file,"%lu\n",image->columns+
    ((image->columns & 0x01) != 0 ? 1 : 0));
  (void) fprintf(file,"%lu\n",image->rows+((image->rows & 0x01) != 0 ? 1 : 0));
  (void) fprintf(file,"%d\n",mpeg != MagickFalse ? 8 : 2);  /* aspect ratio */
  (void) fprintf(file,"%d\n",mpeg != MagickFalse ? 3 : 5);  /* frame rate code */
  (void) fprintf(file,"%g\n",mpeg != MagickFalse ? 1152000.0 : 5000000.0);  /* bit rate */
  (void) fprintf(file,"%d\n",mpeg != MagickFalse ? 20 : 112);  /* vbv buffer size */
  (void) fprintf(file,"0\n");  /* low delay */
  (void) fprintf(file,"%d\n",mpeg != MagickFalse ? 1 : 0);  /* constrained parameter */
  (void) fprintf(file,"%d\n",mpeg != MagickFalse ? 4 : 1);  /* profile ID */
  (void) fprintf(file,"%d\n",mpeg != MagickFalse ? 8 : 4);  /* level ID */
  (void) fprintf(file,"%d\n",mpeg != MagickFalse ? 1 : 0);  /* progressive sequence */
  vertical_factor=2;
  if (image_info->sampling_factor != (char *) NULL)
    {
      GeometryInfo
        geometry_info;

      long
        horizontal_factor;

      MagickStatusType
        flags;

      flags=ParseGeometry(image_info->sampling_factor,&geometry_info);
      horizontal_factor=(long) geometry_info.rho;
      vertical_factor=(long) geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        vertical_factor=horizontal_factor;
      if (mpeg != MagickFalse)
        {
          if ((horizontal_factor != 2) || (vertical_factor != 2))
            {
              (void) fclose(file);
              return(MagickFalse);
            }
        }
      else
        if ((horizontal_factor != 2) ||
            ((vertical_factor != 1) && (vertical_factor != 2)))
          {
            (void) fclose(file);
            return(MagickFalse);
          }
    }
  (void) fprintf(file,"%d\n",vertical_factor == 2 ? 1 : 2); /* chroma format */
  (void) fprintf(file,"%d\n",mpeg != MagickFalse ? 1 : 2);  /* video format */
  (void) fprintf(file,"5\n");  /* color primaries */
  (void) fprintf(file,"5\n");  /* transfer characteristics */
  (void) fprintf(file,"%d\n",mpeg != MagickFalse ? 5 : 4);  /* matrix coefficients */
  (void) fprintf(file,"%lu\n",image->columns+
    ((image->columns & 0x01) != 0 ? 1 : 0));
  (void) fprintf(file,"%lu\n",image->rows+((image->rows & 0x01) != 0 ? 1 : 0));
  (void) fprintf(file,"0\n");  /* intra dc precision */
  (void) fprintf(file,"%d\n",mpeg != MagickFalse ? 0 : 1);  /* top field */
  (void) fprintf(file,"%d %d %d\n",mpeg != MagickFalse ? 1 : 0,
    mpeg != MagickFalse ? 1 : 0, mpeg != MagickFalse ? 1 : 0);
  (void) fprintf(file,"0 0 0\n");  /* concealment motion vector */
  (void) fprintf(file,"%d %d %d\n",mpeg != MagickFalse ? 0 : 1,
    mpeg != MagickFalse ? 0 : 1,mpeg != MagickFalse ? 0 : 1);
  (void) fprintf(file,"%d 0 0\n",mpeg != MagickFalse ? 0 : 1);  /* intra vlc format */
  (void) fprintf(file,"0 0 0\n");  /* alternate scan */
  (void) fprintf(file,"0\n");  /* repeat first field */
  (void) fprintf(file,"%d\n",mpeg != MagickFalse ? 1 : 0);  /* progressive frame */
  (void) fprintf(file,"0\n");  /* intra slice refresh period */
  (void) fprintf(file,"0\n");  /* reaction parameter */
  (void) fprintf(file,"0\n");  /* initial average activity */
  (void) fprintf(file,"0\n");
  (void) fprintf(file,"0\n");
  (void) fprintf(file,"0\n");
  (void) fprintf(file,"0\n");
  (void) fprintf(file,"0\n");
  (void) fprintf(file,"0\n");
  (void) fprintf(file,"2 2 11 11\n");
  (void) fprintf(file,"1 1 3 3\n");
  (void) fprintf(file,"1 1 7 7\n");
  (void) fprintf(file,"1 1 7 7\n");
  (void) fprintf(file,"1 1 3 3\n");
  (void) fclose(file);
  return(MagickTrue);
}

static MagickBooleanType WriteMPEGImage(const ImageInfo *image_info,
  Image *image)
{
  char
    basename[MaxTextExtent],
    filename[MaxTextExtent];

  double
    delay;

  Image
    *coalesce_image,
    *next_image;

  ImageInfo
    *write_info;

  int
    file;

  MagickBooleanType
    status;

  register Image
    *p;

  register long
    i;

  size_t
    length;

  unsigned char
    *blob;

  unsigned long
    count,
    scene;

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == MagickFalse)
    return(status);
  CloseBlob(image);
  /*
    Determine if the sequence of images have identical page info.
  */
  coalesce_image=image;
  for (next_image=image; next_image != (Image *) NULL; )
  {
    if ((image->columns != next_image->columns) ||
        (image->rows != next_image->rows))
      break;
    if ((image->page.x != next_image->page.x) ||
        (image->page.y != next_image->page.y))
      break;
    next_image=GetNextImageInList(next_image);
  }
  if (next_image != (Image *) NULL)
    {
      coalesce_image=CoalesceImages(image,&image->exception);
      if (coalesce_image == (Image *) NULL)
        return(MagickFalse);
    }
  /*
    Write YUV files.
  */
  file=AcquireUniqueFileResource(basename);
  if (file != -1)
    file=close(file)-1;
  (void) FormatMagickString(coalesce_image->filename,MaxTextExtent,"%s",
    basename);
  write_info=CloneImageInfo(image_info);
  status=WriteMPEGParameterFiles(write_info,coalesce_image,basename);
  if (status == MagickFalse)
    {
      if (coalesce_image != image)
        coalesce_image=DestroyImage(coalesce_image);
      (void) RelinquishUniqueFileResource(basename);
      if (image->quality != UndefinedCompressionQuality)
        {
          (void) FormatMagickString(filename,MaxTextExtent,"%s.iqm",
            basename);
          (void) RelinquishUniqueFileResource(filename);
          (void) FormatMagickString(filename,MaxTextExtent,"%s.niq",
            basename);
          (void) RelinquishUniqueFileResource(filename);
        }
      ThrowWriterException(CoderError,"UnableToWriteMPEGParameters");
    }
  count=0;
  write_info->interlace=PlaneInterlace;
  for (p=coalesce_image; p != (Image *) NULL; p=GetNextImageInList(p))
  {
    char
      previous_image[MaxTextExtent];

    blob=(unsigned char *) NULL;
    length=0;
    scene=p->scene;
    delay=100.0*p->delay/MagickMax(1.0*p->ticks_per_second,1.0);
    for (i=0; i < (long) MagickMax((1.0*delay+1.0)/3.0,1.0); i++)
    {
      p->scene=count;
      count++;
      status=MagickFalse;
      switch (i)
      {
        case 0:
        {
          Image
            *frame;

          (void) FormatMagickString(p->filename,MaxTextExtent,"%s.%lu.yuv",
            basename,p->scene);
          (void) FormatMagickString(filename,MaxTextExtent,"%s.%lu.yuv",
            basename,p->scene);
          (void) FormatMagickString(previous_image,MaxTextExtent,
            "%s.%lu.yuv",basename,p->scene);
          frame=CloneImage(p,0,0,MagickTrue,&p->exception);
          if (frame == (Image *) NULL)
            break;
          status=WriteImage(write_info,frame);
          frame=DestroyImage(frame);
          break;
        }
        case 1:
        {
          blob=(unsigned char *)
            FileToBlob(previous_image,~0,&length,&image->exception);
        }
        default:
        {
          (void) FormatMagickString(filename,MaxTextExtent,"%s.%lu.yuv",
            basename,p->scene);
          if (length > 0)
            status=BlobToFile(filename,blob,length,&image->exception);
          break;
        }
      }
      if (image->debug != MagickFalse)
        {
          if (status != MagickFalse)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "%lu. Wrote YUV file for scene %lu:",i,p->scene);
          else
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "%lu. Failed to write YUV file for scene %lu:",i,p->scene);
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),"%s",
            filename);
        }
    }
    p->scene=scene;
    if (blob != (unsigned char *) NULL)
      blob=(unsigned char *) RelinquishMagickMemory(blob);
    if (status == MagickFalse)
      break;
  }
  /*
    Convert YUV to MPEG.
  */
  (void) CopyMagickString(coalesce_image->filename,basename,MaxTextExtent);
  status=InvokeDelegate(write_info,coalesce_image,(char *) NULL,"mpeg-encode",
    &image->exception);
  write_info=DestroyImageInfo(write_info);
  /*
    Free resources.
  */
  count=0;
  for (p=coalesce_image; p != (Image *) NULL; p=GetNextImageInList(p))
  {
    delay=100.0*p->delay/MagickMax(1.0*p->ticks_per_second,1.0);
    for (i=0; i < (long) MagickMax((1.0*delay+1.0)/3.0,1.0); i++)
    {
      (void) FormatMagickString(p->filename,MaxTextExtent,"%s.%lu.yuv",
        basename,count++);
      (void) RelinquishUniqueFileResource(p->filename);
    }
    (void) CopyMagickString(p->filename,image_info->filename,MaxTextExtent);
  }
  (void) RelinquishUniqueFileResource(basename);
  (void) FormatMagickString(filename,MaxTextExtent,"%s.iqm",basename);
  (void) RelinquishUniqueFileResource(filename);
  (void) FormatMagickString(filename,MaxTextExtent,"%s.niq",basename);
  (void) RelinquishUniqueFileResource(filename);
  (void) FormatMagickString(filename,MaxTextExtent,"%s.log",basename);
  (void) RelinquishUniqueFileResource(filename);
  if (coalesce_image != image)
    coalesce_image=DestroyImage(coalesce_image);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),"exit");
  return(status);
}
