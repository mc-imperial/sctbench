/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%             U   U  TTTTT  IIIII  L      IIIII  TTTTT  Y   Y                 %
%             U   U    T      I    L        I      T     Y Y                  %
%             U   U    T      I    L        I      T      Y                   %
%             U   U    T      I    L        I      T      Y                   %
%              UUU     T    IIIII  LLLLL  IIIII    T      Y                   %
%                                                                             %
%                                                                             %
%                       ImageMagick Utility Methods                           %
%                                                                             %
%                             Software Design                                 %
%                               John Cristy                                   %
%                              January 1993                                   %
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
#include "magick/property.h"
#include "magick/blob.h"
#include "magick/color.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/memory_.h"
#include "magick/option.h"
#include "magick/resource_.h"
#include "magick/semaphore.h"
#include "magick/signature.h"
#include "magick/statistic.h"
#include "magick/string_.h"
#include "magick/token.h"
#include "magick/utility.h"
#if defined(HAVE_MACH_O_DYLD_H)
#include <mach-o/dyld.h>
#endif
/*
  Static declarations.
*/
static const char
  Base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
  Forward declaration.
*/
static int
  IsDirectory(const char *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e U n i q u e F i l e n a m e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireUniqueFilename() replaces the contents of path by a unique path name.
%
%  The format of the AcquireUniqueFilename method is:
%
%      MagickBooleanType AcquireUniqueFilename(char *path)
%
%  A description of each parameter follows.
%
%   o  path:  Specifies a pointer to an array of characters.  The unique path
%      name is returned in this array.
%
*/
MagickExport MagickBooleanType AcquireUniqueFilename(char *path)
{
  int
    file;

  file=AcquireUniqueFileResource(path);
  if (file == -1)
    return(MagickFalse);
  file=close(file)-1;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e U n i q u e S ym b o l i c L i n k                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireUniqueSymbolicLink() creates a unique symbolic link to the specified
%  source path and returns MagickTrue on success otherwise MagickFalse.  If the
%  symlink() method fails or is not available, a unique file name is generated
%  and the source file copied to it.  When you are finished with the file, use
%  RelinquishUniqueFilename() to destroy it.
%
%  The format of the AcquireUniqueSymbolicLink method is:
%
%      MagickBooleanType AcquireUniqueSymbolicLink(const char *source,
%        char destination)
%
%  A description of each parameter follows.
%
%   o  source:  the source path.
%
%   o  destination:  the destination path.
%
*/

static inline size_t MagickMin(const size_t x,const size_t y)
{
  if (x < y)
    return(x);
  return(y);
}

MagickExport MagickBooleanType AcquireUniqueSymbolicLink(const char *source,
  char *destination)
{
  int
    destination_file,
    source_file;

  size_t
    length,
    quantum;

  ssize_t
    count;

  struct stat
    file_info;

  unsigned char
    *buffer;

  assert(source != (const char *) NULL);
  assert(destination != (char *) NULL);
#if defined(HAVE_SYMLINK)
  (void) AcquireUniqueFilename(destination);
  (void) RelinquishUniqueFileResource(destination);
  if (*source == *DirectorySeparator)
    {
      if (symlink(source,destination) == 0)
        return(MagickTrue);
    }
  else
    {
      char
        path[MaxTextExtent];

      *path='\0';
      if (getcwd(path,MaxTextExtent) == (char *) NULL)
        return(MagickFalse);
      (void) ConcatenateMagickString(path,DirectorySeparator,MaxTextExtent);
      (void) ConcatenateMagickString(path,source,MaxTextExtent);
      if (symlink(path,destination) == 0)
        return(MagickTrue);
    }
#endif
  destination_file=AcquireUniqueFileResource(destination);
  if (destination_file == -1)
    return(MagickFalse);
  source_file=open(source,O_RDONLY | O_BINARY);
  if (source_file == -1)
    {
      (void) close(destination_file);
      (void) RelinquishUniqueFileResource(destination);
      return(MagickFalse);
    }
  quantum=(size_t) MagickMaxBufferSize;
  if ((fstat(source_file,&file_info) == 0) && (file_info.st_size != 0))
    quantum=MagickMin((size_t) file_info.st_size,MagickMaxBufferSize);
  buffer=(unsigned char *) AcquireQuantumMemory(quantum,sizeof(*buffer));
  if (buffer == (unsigned char *) NULL)
    {
      (void) close(source_file);
      (void) close(destination_file);
      (void) RelinquishUniqueFileResource(destination);
      return(MagickFalse);
    }
  for (length=0; ; )
  {
    count=(ssize_t) read(source_file,buffer,quantum);
    if (count <= 0)
      break;
    length=(size_t) count;
    count=(ssize_t) write(destination_file,buffer,length);
    if ((size_t) count != length)
      {
        (void) close(destination_file);
        (void) close(source_file);
        buffer=(unsigned char *) RelinquishMagickMemory(buffer);
        (void) RelinquishUniqueFileResource(destination);
        return(MagickFalse);
      }
  }
  (void) close(destination_file);
  (void) close(source_file);
  buffer=(unsigned char *) RelinquishMagickMemory(buffer);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  A p p e n d I m a g e F o r m a t                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AppendImageFormat() appends the image format type to the filename.  If an
%  extension to the file already exists, it is first removed.
%
%  The format of the AppendImageFormat method is:
%
%      void AppendImageFormat(const char *format,char *filename)
%
%  A description of each parameter follows.
%
%   o  format:  Specifies a pointer to an array of characters.  This is the
%      format of the image.
%
%   o  filename:  Specifies a pointer to an array of characters.  The unique
%      file name is returned in this array.
%
%
*/
MagickExport void AppendImageFormat(const char *format,char *filename)
{
  char
    root[MaxTextExtent];

  assert(format != (char *) NULL);
  assert(filename != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  if ((*format == '\0') || (*filename == '\0'))
    return;
  if (LocaleCompare(filename,"-") == 0)
    {
      char
        message[MaxTextExtent];

      (void) FormatMagickString(message,MaxTextExtent,"%s:%s",format,filename);
      (void) CopyMagickString(filename,message,MaxTextExtent);
      return;
    }
  GetPathComponent(filename,RootPath,root);
  (void) FormatMagickString(filename,MaxTextExtent,"%s.%s",root,format);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   B a s e 6 4 D e c o d e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Base64Decode() decodes Base64-encoded text and returns its binary
%  equivalent.  NULL is returned if the text is not valid Base64 data, or a
%  memory allocation failure occurs.
%
%  The format of the Base64Decode method is:
%
%      unsigned char *Base64Decode(const char *source,length_t *length)
%
%  A description of each parameter follows:
%
%    o source:  A pointer to a Base64-encoded string.
%
%    o length: The number of bytes decoded.
%
*/
MagickExport unsigned char *Base64Decode(const char *source,size_t *length)
{
  int
    state;

  register const char
    *p,
    *q;

  register size_t
    i;

  unsigned char
    *decode;

  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(source != (char *) NULL);
  assert(length != (size_t *) NULL);
  *length=0;
  decode=(unsigned char *) AcquireQuantumMemory(strlen(source)/4+4,
    3*sizeof(*decode));
  if (decode == (unsigned char *) NULL)
    return((unsigned char *) NULL);
  i=0;
  state=0;
  for (p=source; *p != '\0'; p++)
  {
    if (isspace((int) ((unsigned char) *p)) != 0)
      continue;
    if (*p == '=')
      break;
    q=strchr(Base64,*p);
    if (q == (char *) NULL)
      {
        decode=(unsigned char *) RelinquishMagickMemory(decode);
        return((unsigned char *) NULL);  /* non-Base64 character */
      }
    switch (state)
    {
      case 0:
      {
        decode[i]=(q-Base64) << 2;
        state++;
        break;
      }
      case 1:
      {
        decode[i++]|=(q-Base64) >> 4;
        decode[i]=((q-Base64) & 0x0f) << 4;
        state++;
        break;
      }
      case 2:
      {
        decode[i++]|=(q-Base64) >> 2;
        decode[i]=((q-Base64) & 0x03) << 6;
        state++;
        break;
      }
      case 3:
      {
        decode[i++]|=(q-Base64);
        state=0;
        break;
      }
    }
  }
  /*
    Verify Base-64 string has proper terminal characters.
  */
  if (*p != '=')
    {
      if (state != 0)
        {
          decode=(unsigned char *) RelinquishMagickMemory(decode);
          return((unsigned char *) NULL);
        }
    }
  else
    {
      p++;
      switch (state)
      {
        case 0:
        case 1:
        {
          /*
            Unrecognized '=' character.
          */
          decode=(unsigned char *) RelinquishMagickMemory(decode);
          return((unsigned char *) NULL);
        }
        case 2:
        {
          for ( ; *p != '\0'; p++)
            if (isspace((int) ((unsigned char) *p)) == 0)
              break;
          if (*p != '=')
            {
              decode=(unsigned char *) RelinquishMagickMemory(decode);
              return((unsigned char *) NULL);
            }
          p++;
        }
        case 3:
        {
          for ( ; *p != '\0'; p++)
            if (isspace((int) ((unsigned char) *p)) == 0)
              {
                decode=(unsigned char *) RelinquishMagickMemory(decode);
                return((unsigned char *) NULL);
              }
          if ((int) decode[i] != 0)
            {
              decode=(unsigned char *) RelinquishMagickMemory(decode);
              return((unsigned char *) NULL);
            }
        }
      }
    }
  *length=i;
  return(decode);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   B a s e 6 4 E n c o d e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Base64Encode() encodes arbitrary binary data to Base64 encoded format as
%  described by the "Base64 Content-Transfer-Encoding" section of RFC 2045 and
%  returns the result as a null-terminated ASCII string.  NULL is returned if
%  a memory allocation failure occurs.
%
%  The format of the Base64Encode method is:
%
%      char *Base64Encode(const unsigned char *blob,const size_t blob_length,
%        size_t *encode_length)
%
%  A description of each parameter follows:
%
%    o blob:  A pointer to binary data to encode.
%
%    o blob_length: The number of bytes to encode.
%
%    o encode_length:  The number of bytes encoded.
%
*/
MagickExport char *Base64Encode(const unsigned char *blob,
  const size_t blob_length,size_t *encode_length)
{
  char
    *encode;

  register const unsigned char
    *p;

  register size_t
    i;

  size_t
    remainder;

  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(blob != (const unsigned char *) NULL);
  assert(blob_length != 0);
  assert(encode_length != (size_t *) NULL);
  *encode_length=0;
  encode=(char *) AcquireQuantumMemory(blob_length/3+4,4*sizeof(*encode));
  if (encode == (char *) NULL)
    return((char *) NULL);
  i=0;
  for (p=blob; p < (blob+blob_length-2); p+=3)
  {
    encode[i++]=Base64[(int) (*p >> 2)];
    encode[i++]=Base64[(int) (((*p & 0x03) << 4)+(*(p+1) >> 4))];
    encode[i++]=Base64[(int) (((*(p+1) & 0x0f) << 2)+(*(p+2) >> 6))];
    encode[i++]=Base64[(int) (*(p+2) & 0x3f)];
  }
  remainder=blob_length % 3;
  if (remainder != 0)
    {
      long
        j;

      unsigned char
        code[3];

      code[0]='\0';
      code[1]='\0';
      code[2]='\0';
      for (j=0; j < (long) remainder; j++)
        code[j]=(*p++);
      encode[i++]=Base64[(int) (code[0] >> 2)];
      encode[i++]=Base64[(int) (((code[0] & 0x03) << 4)+(code[1] >> 4))];
      if (remainder == 1)
        encode[i++]='=';
      else
        encode[i++]=Base64[(int) (((code[1] & 0x0f) << 2)+(code[2] >> 6))];
      encode[i++]='=';
    }
  *encode_length=i;
  encode[i++]='\0';
  return(encode);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C h o p P a t h C o m p o n e n t s                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ChopPathComponents() removes the number of specified file components from a
%  path.
%
%  The format of the ChopPathComponents method is:
%
%      ChopPathComponents(char *path,unsigned long components)
%
%  A description of each parameter follows:
%
%    o path:  The path.
%
%    o components:  The number of components to chop.
%
*/
MagickExport void ChopPathComponents(char *path,const unsigned long components)
{
  register long
    i;

  for (i=0; i < (long) components; i++)
    GetPathComponent(path,HeadPath,path);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x p a n d F i l e n a m e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExpandFilename() expands '~' in a path.
%
%  The format of the ExpandFilename function is:
%
%      ExpandFilename(char *path)
%
%  A description of each parameter follows:
%
%    o path: Specifies a pointer to a character array that contains the
%      path.
%
*/
MagickExport void ExpandFilename(char *path)
{
  char
    expand_path[MaxTextExtent];

  if (path == (char *) NULL)
    return;
  if (*path != '~')
    return;
  (void) CopyMagickString(expand_path,path,MaxTextExtent);
  if ((*(path+1) == *DirectorySeparator) || (*(path+1) == '\0'))
    {
      char
        *home;

      /*
        Substitute ~ with $HOME.
      */
      (void) CopyMagickString(expand_path,".",MaxTextExtent);
      (void) ConcatenateMagickString(expand_path,path+1,MaxTextExtent);
      home=GetEnvironmentValue("HOME");
      if (home == (char *) NULL)
        home=GetEnvironmentValue("USERPROFILE");
      if (home != (char *) NULL)
        {
          (void) CopyMagickString(expand_path,home,MaxTextExtent);
          (void) ConcatenateMagickString(expand_path,path+1,MaxTextExtent);
          home=DestroyString(home);
        }
    }
  else
    {
#if defined(POSIX)
      char
        username[MaxTextExtent];

      register char
        *p;

      struct passwd
        *entry;

      /*
        Substitute ~ with home directory from password file.
      */
      (void) CopyMagickString(username,path+1,MaxTextExtent);
      p=strchr(username,'/');
      if (p != (char *) NULL)
        *p='\0';
      entry=getpwnam(username);
      if (entry == (struct passwd *) NULL)
        return;
      (void) CopyMagickString(expand_path,entry->pw_dir,MaxTextExtent);
      if (p != (char *) NULL)
        {
          (void) ConcatenateMagickString(expand_path,"/",MaxTextExtent);
          (void) ConcatenateMagickString(expand_path,p+1,MaxTextExtent);
        }
#endif
    }
  (void) CopyMagickString(path,expand_path,MaxTextExtent);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x p a n d F i l e n a m e s                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExpandFilenames() checks each argument of the command line vector and
%  expands it if they have a wildcard character.  For example, *.jpg might
%  expand to:  bird.jpg rose.jpg tiki.jpg.
%
%  The format of the ExpandFilenames function is:
%
%      status=ExpandFilenames(int *argc,char ***argv)
%
%  A description of each parameter follows:
%
%    o argc: Specifies a pointer to an integer describing the number of
%      elements in the argument vector.
%
%    o argv: Specifies a pointer to a text array containing the command line
%      arguments.
%
*/
MagickExport MagickBooleanType ExpandFilenames(int *argc,char ***argv)
{
  char
    **filelist,
    filename[MaxTextExtent],
    home_directory[MaxTextExtent],
    magick[MaxTextExtent],
    *option,
    path[MaxTextExtent],
    subimage[MaxTextExtent],
    **vector;

  long
    count,
    parameters;

  register long
    i,
    j;

  unsigned long
    number_files;

  /*
    Allocate argument vector.
  */
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(argc != (int *) NULL);
  assert(argv != (char ***) NULL);
  vector=(char **) AcquireQuantumMemory(*argc+1UL,sizeof(*vector));
  if (vector == (char **) NULL)
    return(MagickFalse);
  /*
    Expand any wildcard filenames.
  */
  if (getcwd(home_directory,MaxTextExtent) == (char *) NULL)
    return(MagickFalse);
  count=0;
  for (i=0; i < (long) *argc; i++)
  {
    option=(*argv)[i];
    vector[count++]=ConstantString(option);
    parameters=ParseMagickOption(MagickCommandOptions,MagickFalse,option);
    if (parameters > 0)
      {
        /*
          Do not expand command option parameters.
        */
        for (j=0; j < parameters; j++)
        {
          i++;
          if (i == (long) *argc)
            break;
          option=(*argv)[i];
          vector[count++]=ConstantString(option);
        }
        continue;
      }
    if ((*option == '"') || (*option == '\''))
      continue;
    GetPathComponent(option,TailPath,filename);
    if (IsGlob(filename) == MagickFalse)
      continue;
    GetPathComponent(option,MagickPath,magick);
    if ((LocaleCompare(magick,"CAPTION") == 0) ||
        (LocaleCompare(magick,"LABEL") == 0) ||
        (LocaleCompare(magick,"VID") == 0))
      continue;
    GetPathComponent(option,HeadPath,path);
    GetPathComponent(option,SubimagePath,subimage);
    ExpandFilename(path);
    filelist=ListFiles(*path == '\0' ? home_directory : path,filename,
      &number_files);
    if (filelist == (char **) NULL)
      continue;
    for (j=0; j < (long) number_files; j++)
      if (IsDirectory(filelist[j]) <= 0)
        break;
    if (j == (long) number_files)
      {
        for (j=0; j < (long) number_files; j++)
          filelist[j]=DestroyString(filelist[j]);
        filelist=(char **) RelinquishMagickMemory(filelist);
        continue;
      }
    /*
      Transfer file list to argument vector.
    */
    vector=(char **) ResizeQuantumMemory(vector,(size_t) *argc+count+
      number_files+1,sizeof(*vector));
    if (vector == (char **) NULL)
      return(MagickFalse);
    count--;
    for (j=0; j < (long) number_files; j++)
    {
      (void) CopyMagickString(filename,path,MaxTextExtent);
      if (*path != '\0')
        (void) ConcatenateMagickString(filename,DirectorySeparator,
          MaxTextExtent);
      (void) ConcatenateMagickString(filename,filelist[j],MaxTextExtent);
      filelist[j]=DestroyString(filelist[j]);
      if (IsAccessible(filename) != MagickFalse)
        {
          char
            path[MaxTextExtent];

          *path='\0';
          if (*magick != '\0')
            {
              (void) ConcatenateMagickString(path,magick,MaxTextExtent);
              (void) ConcatenateMagickString(path,":",MaxTextExtent);
            }
          (void) ConcatenateMagickString(path,filename,MaxTextExtent);
          if (*subimage != '\0')
            {
              (void) ConcatenateMagickString(path,"[",MaxTextExtent);
              (void) ConcatenateMagickString(path,subimage,MaxTextExtent);
              (void) ConcatenateMagickString(path,"]",MaxTextExtent);
            }
          vector[count++]=ConstantString(path);
        }
    }
    filelist=(char **) RelinquishMagickMemory(filelist);
  }
  vector[count]=(char *) NULL;
  if (IsEventLogging() != MagickFalse)
    {
      char
        *command_line;

      command_line=AcquireString(vector[0]);
      for (i=1; i < count; i++)
      {
        (void) ConcatenateString(&command_line," {");
        (void) ConcatenateString(&command_line,vector[i]);
        (void) ConcatenateString(&command_line,"}");
      }
      (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
        "Command line: %s",command_line);
      command_line=DestroyString(command_line);
    }
  *argc=(int) count;
  *argv=vector;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t E x e c u t i o n P a t h                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetExecutionPath() returns the pathname of the executable that started
%  the process.  On success MagickTrue is returned, otherwise MagickFalse.
%
%  The format of the GetExecutionPath method is:
%
%      MagickBooleanType GetExecutionPath(char *path,const size_t extent)
%
%  A description of each parameter follows:
%
%    o path: The pathname of the executable that started the process.
%
%    o extent: the maximum extent of the path.
%
*/
MagickExport MagickBooleanType GetExecutionPath(char *path,const size_t extent)
{
  *path='\0';
  if (getcwd(path,extent) == (char *) NULL)
    return(MagickFalse);
#if defined(__WINDOWS__)
  return(NTGetExecutionPath(path,extent));
#endif
#if defined(HAVE__NSGETEXECUTABLEPATH)
  {
    uint32_t
      length;

    length=extent;
    _NSGetExecutablePath(path,&length);
    return(MagickTrue);
  }
#endif
#if defined(HAVE_GETEXECNAME)
  {
    const char
      *execution_path;

    execution_path=(const char *) getexecname();
    if (execution_path != (const char *) NULL)
      {
        if (*execution_path != *DirectorySeparator)
          (void) ConcatenateMagickString(path,DirectorySeparator,extent);
        (void) ConcatenateMagickString(path,execution_path,extent);
      }
  }
#endif
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t P a t h C o m p o n e n t                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPathComponent() returns the parent directory name, filename, basename, or
%  extension of a file path.
%
%  The format of the GetPathComponent function is:
%
%      GetPathComponent(const char *path,PathType type,char *component)
%
%  A description of each parameter follows:
%
%    o path: Specifies a pointer to a character array that contains the
%      file path.
%
%    o type: Specififies which file path component to return.
%
%    o component: The selected file path component is returned here.
%
*/
MagickExport void GetPathComponent(const char *path,PathType type,
  char *component)
{
  char
    magick[MaxTextExtent],
    *q,
    subimage[MaxTextExtent];

  register char
    *p;

  assert(path != (const char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",path);
  assert(component != (char *) NULL);
  if (*path == '\0')
    {
      *component='\0';
      return;
    }
  (void) CopyMagickString(component,path,MaxTextExtent);
  *magick='\0';
  for (p=component; *p != '\0'; p++)
    if ((*p == ':') && (IsDirectory(path) < 0) &&
        (IsAccessible(path) == MagickFalse))
      {
        /*
          Look for image format specification (e.g. ps3:image).
        */
        (void) CopyMagickString(magick,component,(size_t) (p-component+1));
        if (IsMagickConflict(magick) != MagickFalse)
          *magick='\0';
        else
          for (q=component; *q != '\0'; q++)
            *q=(*++p);
        break;
      }
  *subimage='\0';
  p=component;
  if (*p != '\0')
    p=component+strlen(component)-1;
  if ((*p == ']') && (strchr(component,'[') != (char *) NULL))
    {
      /*
        Look for scene specification (e.g. img0001.pcd[4]).
      */
      for (q=p-1; q > component; q--)
        if (*q == '[')
          break;
      if (*q == '[')
        {
          (void) CopyMagickString(subimage,q+1,MaxTextExtent);
          subimage[p-q-1]='\0';
          if ((IsSceneGeometry(subimage,MagickFalse) == MagickFalse) &&
              (IsGeometry(subimage) == MagickFalse))
            *subimage='\0';
          else
            *q='\0';
        }
    }
  p=component;
  if (*p != '\0')
    for (p=component+strlen(component)-1; p > component; p--)
      if (IsBasenameSeparator(*p) != MagickFalse)
        break;
  switch (type)
  {
    case MagickPath:
    {
      (void) CopyMagickString(component,magick,MaxTextExtent);
      break;
    }
    case RootPath:
    {
      for (p=component+(strlen(component)-1); p > component; p--)
      {
        if (IsBasenameSeparator(*p) != MagickFalse)
          break;
        if (*p == '.')
          break;
      }
      if (*p == '.')
        *p='\0';
      break;
    }
    case HeadPath:
    {
      *p='\0';
      break;
    }
    case TailPath:
    {
      if (IsBasenameSeparator(*p) != MagickFalse)
        (void) CopyMagickMemory((unsigned char *) component,
          (const unsigned char *) (p+1),strlen(p+1)+1);
      break;
    }
    case BasePath:
    {
      if (IsBasenameSeparator(*p) != MagickFalse)
        (void) CopyMagickString(component,p+1,MaxTextExtent);
      for (p=component+(strlen(component)-1); p > component; p--)
        if (*p == '.')
          {
            *p='\0';
            break;
          }
      break;
    }
    case ExtensionPath:
    {
      if (IsBasenameSeparator(*p) != MagickFalse)
        (void) CopyMagickString(component,p+1,MaxTextExtent);
      p=component;
      if (*p != '\0')
        for (p=component+strlen(component)-1; p > component; p--)
          if (*p == '.')
            break;
      *component='\0';
      if (*p == '.')
        (void) CopyMagickString(component,p+1,MaxTextExtent);
      break;
    }
    case SubimagePath:
    {
      (void) CopyMagickString(component,subimage,MaxTextExtent);
      break;
    }
    case CanonicalPath:
    case UndefinedPath:
      break;
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  G e t P a t h C o m p o n e n t s                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPathComponents() returns a list of path components.
%
%  The format of the GetPathComponents method is:
%
%      char **GetPathComponents(const char *path,
%        unsigned long *number_componenets)
%
%  A description of each parameter follows:
%
%    o path:  Specifies the string to segment into a list.
%
%    o number_components:  return the number of components in the list
%
*/
MagickExport char **GetPathComponents(const char *path,
  unsigned long *number_components)
{
  char
    **components;

  register char
    *q;

  register const char
    *p;

  register long
    i;

  if (path == (char *) NULL)
    return((char **) NULL);
  *number_components=1;
  for (p=path; *p != '\0'; p++)
    if (IsBasenameSeparator(*p))
      (*number_components)++;
  components=(char **) AcquireQuantumMemory((size_t) *number_components+1UL,
    sizeof(*components));
  if (components == (char **) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  p=path;
  for (i=0; i < (long) *number_components; i++)
  {
    for (q=(char *) p; *q != '\0'; q++)
      if (IsBasenameSeparator(*q))
        break;
    components[i]=(char *) AcquireQuantumMemory((size_t) (q-p)+MaxTextExtent,
      sizeof(*components));
    if (components[i] == (char *) NULL)
      ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
    (void) CopyMagickString(components[i],p,(size_t) (q-p+1));
    p=q+1;
  }
  components[i]=(char *) NULL;
  return(components);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  I s A c c e s s i b l e                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsAccessible() returns MagickTrue if the file as defined by the path is
%  accessible.
%
%  The format of the IsAccessible method is:
%
%      MagickBooleanType IsAccessible(const char *filename)
%
%  A description of each parameter follows.
%
%    o path:  Specifies a path to a file.
%
%
*/
MagickExport MagickBooleanType IsAccessible(const char *path)
{
  int
    status;

  struct stat
    file_info;

  if ((path == (const char *) NULL) || (*path == '\0'))
    return(MagickFalse);
  status=stat(path,&file_info);
  if (status != 0)
    return(MagickFalse);
  if (S_ISREG(file_info.st_mode) == 0)
    return(MagickFalse);
  if (access(path,F_OK) != 0)
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  I s D i r e c t o r y                                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsDirectory() returns -1 if the directory does not exist,  1 is returned
%  if the path represents a directory otherwise 0.
%
%  The format of the IsAccessible method is:
%
%      int IsDirectory(const char *path)
%
%  A description of each parameter follows.
%
%   o  path:  The directory path.
%
*/
static int IsDirectory(const char *path)
{
#if !defined(X_OK)
#define X_OK  1
#endif

  int
    status;

  struct stat
    file_info;

  if ((path == (const char *) NULL) || (*path == '\0'))
    return(MagickFalse);
  status=stat(path,&file_info);
  if (status != 0)
    return(-1);
  if (S_ISDIR(file_info.st_mode) == 0)
    return(0);
  if (access(path,X_OK) != 0)
    return(0);
  return(1);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s M a g i c k T r u e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsMagickTrue() returns MagickTrue if the value is "true", "on", "yes" or
%  "1".
%
%  The format of the IsMagickTrue method is:
%
%      MagickBooleanType IsMagickTrue(const char *value)
%
%  A description of each parameter follows:
%
%    o option: either MagickTrue or MagickFalse depending on the value
%      parameter.
%
%    o value: Specifies a pointer to a character array.
%
*/
MagickExport MagickBooleanType IsMagickTrue(const char *value)
{
  if (value == (char *) NULL)
    return(MagickFalse);
  if (LocaleCompare(value,"true") == 0)
    return(MagickTrue);
  if (LocaleCompare(value,"on") == 0)
    return(MagickTrue);
  if (LocaleCompare(value,"yes") == 0)
    return(MagickTrue);
  if (LocaleCompare(value,"1") == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L i s t F i l e s                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListFiles() reads the directory specified and returns a list of filenames
%  contained in the directory sorted in ascending alphabetic order.
%
%  The format of the ListFiles function is:
%
%      char **ListFiles(const char *directory,const char *pattern,
%        long *number_entries)
%
%  A description of each parameter follows:
%
%    o filelist: Method ListFiles returns a list of filenames contained
%      in the directory.  If the directory specified cannot be read or it is
%      a file a NULL list is returned.
%
%    o directory: Specifies a pointer to a text string containing a directory
%      name.
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_entries:  This integer returns the number of filenames in the
%      list.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int FileCompare(const void *x,const void *y)
{
  register const char
    **p,
    **q;

  p=(const char **) x;
  q=(const char **) y;
  return(LocaleCompare(*p,*q));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport char **ListFiles(const char *directory,const char *pattern,
  unsigned long *number_entries)
{
  char
    **filelist;

  DIR
    *current_directory;

  struct dirent
    *entry;

  unsigned long
    max_entries;

  /*
    Open directory.
  */
  assert(directory != (const char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",directory);
  assert(pattern != (const char *) NULL);
  assert(number_entries != (unsigned long *) NULL);
  *number_entries=0;
  current_directory=opendir(directory);
  if (current_directory == (DIR *) NULL)
    return((char **) NULL);
  /*
    Allocate filelist.
  */
  max_entries=2048;
  filelist=(char **) AcquireQuantumMemory((size_t) max_entries,
    sizeof(*filelist));
  if (filelist == (char **) NULL)
    {
      (void) closedir(current_directory);
      return((char **) NULL);
    }
  /*
    Save the current and change to the new directory.
  */
  entry=readdir(current_directory);
  while (entry != (struct dirent *) NULL)
  {
    if (*entry->d_name == '.')
      {
        entry=readdir(current_directory);
        continue;
      }
    if ((IsDirectory(entry->d_name) > 0) ||
#if defined(__WINDOWS__)
        (GlobExpression(entry->d_name,pattern,MagickTrue) != MagickFalse))
#else
        (GlobExpression(entry->d_name,pattern,MagickFalse) != MagickFalse))
#endif
      {
        if (*number_entries >= max_entries)
          {
            /*
              Extend the file list.
            */
            max_entries<<=1;
            filelist=(char **) ResizeQuantumMemory(filelist,(size_t)
              max_entries,sizeof(*filelist));
            if (filelist == (char **) NULL)
              {
                (void) closedir(current_directory);
                return((char **) NULL);
              }
          }
#if defined(vms)
        {
          register char
            *p;

          p=strchr(entry->d_name,';');
          if (p)
            *p='\0';
          if (*number_entries > 0)
            if (LocaleCompare(entry->d_name,filelist[*number_entries-1]) == 0)
              {
                entry=readdir(current_directory);
                continue;
              }
        }
#endif
        filelist[*number_entries]=(char *) AcquireString(entry->d_name);
        if (IsDirectory(entry->d_name) > 0)
          (void) ConcatenateMagickString(filelist[*number_entries],
            DirectorySeparator,MaxTextExtent);
        (*number_entries)++;
      }
    entry=readdir(current_directory);
  }
  (void) closedir(current_directory);
  /*
    Sort filelist in ascending order.
  */
  qsort((void *) filelist,(size_t) *number_entries,sizeof(*filelist),
    FileCompare);
  return(filelist);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k O p e n S t r e a m                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickOpenStream() opens the file whose name is the string pointed to by
%  path and associates a stream with it.
%
%  The path of the MagickOpenStream method is:
%
%      FILE *MagickOpenStream(const char *path,const char *mode)
%
%  A description of each parameter follows.
%
%   o  path: the file path.
%
%   o  mode: the file mode.
%
*/

#if defined(__WINDOWS__)
static wchar_t *Latin1ToUnicodeString(const char *string)
{
  int
    count;

  size_t
    length;

  wchar_t
    *unicode_string;

  length=strlen(string);
  count=MultiByteToWideChar(CP_ACP,0,string,(int) length+1,NULL,0);
  if (count == 0)
    return((wchar_t *) "");
  unicode_string=(wchar_t *) AcquireQuantumMemory(count,
    sizeof(*unicode_string));
  MultiByteToWideChar(CP_ACP,0,string,(int) length+1,unicode_string,count);
  return(unicode_string);
}
#endif

MagickExport FILE *MagickOpenStream(const char *path,const char *mode)
{
  FILE
    *file;

  file=(FILE *) NULL;
#if defined(__WINDOWS__)
  {
    wchar_t
      *unicode_mode,
      *unicode_path;

    unicode_path=Latin1ToUnicodeString(path);
    unicode_mode=Latin1ToUnicodeString(mode);
    file=_wfopen(unicode_path,unicode_mode);
    unicode_mode=(wchar_t *) RelinquishMagickMemory(unicode_mode);
    unicode_path=(wchar_t *) RelinquishMagickMemory(unicode_path);
  }
#endif
  if (file == (FILE *) NULL)
    file=fopen(path,mode);
  return(file);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  M u l t i l i n e C e n s u s                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MultilineCensus() returns the number of lines within a label.  A line is
%  represented by a \n character.
%
%  The format of the MultilineCenus method is:
%
%      unsigned long MultilineCensus(const char *label)
%
%  A description of each parameter follows.
%
%   o  label:  This character string is the label.
%
%
*/
MagickExport unsigned long MultilineCensus(const char *label)
{
  unsigned long
    number_lines;

  /*
    Determine the number of lines within this label.
  */
  if (label == (char *) NULL)
    return(0);
  for (number_lines=1; *label != '\0'; label++)
    if (*label == '\n')
      number_lines++;
  return(number_lines);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S y s t e m C o m m a n d                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SystemCommand() executes the specified command and waits until it
%  terminates.  The returned value is the exit status of the command.
%
%  The format of the SystemCommand method is:
%
%      int SystemCommand(const MagickBooleanType verbose,const char *command)
%
%  A description of each parameter follows:
%
%    o verbose: An MagickBooleanType other than 0 prints the executed command
%      before it is invoked.
%
%    o command: This string is the command to execute.
%
*/
MagickExport int SystemCommand(const MagickBooleanType verbose,
  const char *command)
{
  int
    status;

  if (verbose != MagickFalse)
    {
      (void) fprintf(stderr,"%s\n",command);
      (void) fflush(stderr);
    }
  status=(-1);
#if defined(POSIX)
#if !defined(HAVE_EXECVP)
  status=system(command);
#else
  if (strspn(command,"&;<>|") != 0)
    status=system(command);
  else
    {
      char
        **argv;

      int
        argc;

      argv=StringToArgv(command,&argc);
      if (argv == (char **) NULL)
        status=system(command);
      else
        {
          pid_t
            child_pid;

          register long
            i;

          /*
            Call application directly rather than from a shell.
          */
          child_pid=fork();
          if (child_pid == (pid_t) -1)
            status=system(command);
          else
            if (child_pid == 0)
              {
                status=execvp(argv[1],argv+1);
                _exit(1);
              }
            else
              {
                int
                  child_status;

                pid_t
                  pid;

                child_status=0;
                pid=waitpid(child_pid,&child_status,0);
                if (pid == -1)
                  status=(-1);
                else
                  {
                    if (WIFEXITED(child_status) != 0)
                      status=WEXITSTATUS(child_status);
                    else
                      if (WIFSIGNALED(child_status))
                        status=(-1);
                  }
              }
          for (i=1; i < argc; i++)
            argv[i]=DestroyString(argv[i]);
          argv=(char **) RelinquishMagickMemory(argv);
        }
    }
#endif
#elif defined(__WINDOWS__)
  status=NTSystemCommand(command);
#elif defined(macintosh)
  status=MACSystemCommand(command);
#elif defined(vms)
  status=system(command);
#else
#  error No suitable system() method.
#endif
  if (status < 0)
    {
      char
        *message;

      ExceptionInfo
        *exception;

      exception=AcquireExceptionInfo();
      message=GetExceptionMessage(errno);
      (void) ThrowMagickException(exception,GetMagickModule(),DelegateError,
        "`%s': %s",command,message);
      message=DestroyString(message);
      CatchException(exception);
      exception=DestroyExceptionInfo(exception);
    }
  return(status);
}
