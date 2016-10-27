/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%          CCCC   OOO   N   N  FFFFF  IIIII   GGGG  U   U  RRRR   EEEEE       %
%         C      O   O  NN  N  F        I    G      U   U  R   R  E           %
%         C      O   O  N N N  FFF      I    G GG   U   U  RRRR   EEE         %
%         C      O   O  N  NN  F        I    G   G  U   U  R R    E           %
%          CCCC   OOO   N   N  F      IIIII   GGG    UUU   R  R   EEEEE       %
%                                                                             %
%                                                                             %
%                     ImageMagick Image Configure Methods                     %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 2003                                   %
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
#include "magick/client.h"
#include "magick/configure.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/hashmap.h"
#include "magick/log.h"
#include "magick/memory_.h"
#include "magick/semaphore.h"
#include "magick/string_.h"
#include "magick/token.h"
#include "magick/utility.h"
#include "magick/xml-tree.h"

/*
  Define declarations.
*/
#define ConfigureFilename  "configure.xml"

/*
  Static declarations.
*/
static const char
  *ConfigureMap = (char *)
    "<?xml version=\"1.0\"?>"
    "<configuremap>"
    "  <configure stealth=\"True\" />"
    "</configuremap>";

static LinkedListInfo
  *configure_list = (LinkedListInfo *) NULL;

static SemaphoreInfo
  *configure_semaphore = (SemaphoreInfo *) NULL;

static volatile MagickBooleanType
  instantiate_configure = MagickFalse;

/*
  Forward declarations.
*/
static MagickBooleanType
  InitializeConfigureList(ExceptionInfo *),
  LoadConfigureLists(const char *,ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y C o n f i g u r e L i s t                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyConfigureList() deallocates memory associated with the configure list.
%
%  The format of the DestroyConfigureList method is:
%
%      DestroyConfigureList(void)
%
*/

static void *DestroyConfigureElement(void *configure_info)
{
  register ConfigureInfo
    *p;

  p=(ConfigureInfo *) configure_info;
  if (p->path != (char *) NULL)
    p->path=DestroyString(p->path);
  if (p->name != (char *) NULL)
    p->name=DestroyString(p->name);
  if (p->value != (char *) NULL)
    p->value=DestroyString(p->value);
  p=(ConfigureInfo *) RelinquishMagickMemory(p);
  return((void *) NULL);
}

MagickExport void DestroyConfigureList(void)
{
  AcquireSemaphoreInfo(&configure_semaphore);
  if (configure_list != (LinkedListInfo *) NULL)
    configure_list=DestroyLinkedList(configure_list,DestroyConfigureElement);
  configure_list=(LinkedListInfo *) NULL;
  instantiate_configure=MagickFalse;
  RelinquishSemaphoreInfo(configure_semaphore);
  configure_semaphore=DestroySemaphoreInfo(configure_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y C o n f i g u r e O p t i o n s                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyConfigureOptions() releases memory associated with an configure
%  options.
%
%  The format of the DestroyProfiles method is:
%
%      LinkedListInfo *DestroyConfigureOptions(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/

static void *DestroyOptions(void *option)
{
  return(DestroyStringInfo((StringInfo *) option));
}

MagickExport LinkedListInfo *DestroyConfigureOptions(LinkedListInfo *options)
{
  assert(options != (LinkedListInfo *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  return(DestroyLinkedList(options,DestroyOptions));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t C o n f i g u r e I n f o                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetConfigureInfo() searches the configure list for the specified name and if
%  found returns attributes for that element.
%
%  The format of the GetConfigureInfo method is:
%
%      const ConfigureInfo *GetConfigureInfo(const char *name,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o configure_info: GetConfigureInfo() searches the configure list for the
%      specified name and if found returns attributes for that element.
%
%    o name: The configure name.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport const ConfigureInfo *GetConfigureInfo(const char *name,
  ExceptionInfo *exception)
{
  register const ConfigureInfo
    *p;

  assert(exception != (ExceptionInfo *) NULL);
  if ((configure_list == (LinkedListInfo *) NULL) ||
      (instantiate_configure == MagickFalse))
    if (InitializeConfigureList(exception) == MagickFalse)
      return((const ConfigureInfo *) NULL);
  if ((configure_list == (LinkedListInfo *) NULL) ||
      (IsLinkedListEmpty(configure_list) != MagickFalse))
    return((const ConfigureInfo *) NULL);
  if ((name == (const char *) NULL) || (LocaleCompare(name,"*") == 0))
    return((const ConfigureInfo *) GetValueFromLinkedList(configure_list,0));
  /*
    Search for named configure.
  */
  AcquireSemaphoreInfo(&configure_semaphore);
  ResetLinkedListIterator(configure_list);
  p=(const ConfigureInfo *) GetNextValueInLinkedList(configure_list);
  while (p != (const ConfigureInfo *) NULL)
  {
    if (LocaleCompare(name,p->name) == 0)
      break;
    p=(const ConfigureInfo *) GetNextValueInLinkedList(configure_list);
  }
  if (p == (ConfigureInfo *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
      "NoSuchElement","`%s'",name);
  RelinquishSemaphoreInfo(configure_semaphore);
  return(p);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t C o n f i g u r e I n f o L i s t                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetConfigureInfoList() returns any configure options that match the
%  specified pattern.
%
%  The format of the GetConfigureInfoList function is:
%
%      const ConfigureInfo **GetConfigureInfoList(const char *pattern,
%        unsigned long *number_options,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_options:  This integer returns the number of configure options in
%    the list.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int ConfigureInfoCompare(const void *x,const void *y)
{
  const ConfigureInfo
    **p,
    **q;

  p=(const ConfigureInfo **) x,
  q=(const ConfigureInfo **) y;
  if (LocaleCompare((*p)->path,(*q)->path) == 0)
    return(LocaleCompare((*p)->name,(*q)->name));
  return(LocaleCompare((*p)->path,(*q)->path));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport const ConfigureInfo **GetConfigureInfoList(const char *pattern,
  unsigned long *number_options,ExceptionInfo *exception)
{
  const ConfigureInfo
    **options;

  register const ConfigureInfo
    *p;

  register long
    i;

  /*
    Allocate configure list.
  */
  assert(pattern != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_options != (unsigned long *) NULL);
  *number_options=0;
  p=GetConfigureInfo("*",exception);
  if (p == (const ConfigureInfo *) NULL)
    return((const ConfigureInfo **) NULL);
  options=(const ConfigureInfo **) AcquireQuantumMemory((size_t)
    GetNumberOfElementsInLinkedList(configure_list)+1UL,sizeof(*options));
  if (options == (const ConfigureInfo **) NULL)
    return((const ConfigureInfo **) NULL);
  /*
    Generate configure list.
  */
  AcquireSemaphoreInfo(&configure_semaphore);
  ResetLinkedListIterator(configure_list);
  p=(const ConfigureInfo *) GetNextValueInLinkedList(configure_list);
  for (i=0; p != (const ConfigureInfo *) NULL; )
  {
    if ((p->stealth == MagickFalse) &&
        (GlobExpression(p->name,pattern,MagickFalse) != MagickFalse))
      options[i++]=p;
    p=(const ConfigureInfo *) GetNextValueInLinkedList(configure_list);
  }
  RelinquishSemaphoreInfo(configure_semaphore);
  qsort((void *) options,(size_t) i,sizeof(*options),ConfigureInfoCompare);
  options[i]=(ConfigureInfo *) NULL;
  *number_options=(unsigned long) i;
  return(options);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t C o n f i g u r e L i s t                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetConfigureList() returns any configure options that match the specified
%  pattern.
%
%  The format of the GetConfigureList function is:
%
%      char **GetConfigureList(const char *pattern,
%        unsigned long *number_options,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_options:  This integer returns the number of options in the list.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int ConfigureCompare(const void *x,const void *y)
{
  register char
    **p,
    **q;

  p=(char **) x;
  q=(char **) y;
  return(LocaleCompare(*p,*q));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport char **GetConfigureList(const char *pattern,
  unsigned long *number_options,ExceptionInfo *exception)
{
  char
    **options;

  register const ConfigureInfo
    *p;

  register long
    i;

  /*
    Allocate configure list.
  */
  assert(pattern != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_options != (unsigned long *) NULL);
  *number_options=0;
  p=GetConfigureInfo("*",exception);
  if (p == (const ConfigureInfo *) NULL)
    return((char **) NULL);
  AcquireSemaphoreInfo(&configure_semaphore);
  RelinquishSemaphoreInfo(configure_semaphore);
  options=(char **) AcquireQuantumMemory((size_t)
    GetNumberOfElementsInLinkedList(configure_list)+1UL,sizeof(*options));
  if (options == (char **) NULL)
    return((char **) NULL);
  AcquireSemaphoreInfo(&configure_semaphore);
  ResetLinkedListIterator(configure_list);
  p=(const ConfigureInfo *) GetNextValueInLinkedList(configure_list);
  for (i=0; p != (const ConfigureInfo *) NULL; )
  {
    if ((p->stealth == MagickFalse) &&
        (GlobExpression(p->name,pattern,MagickFalse) != MagickFalse))
      options[i++]=ConstantString(p->name);
    p=(const ConfigureInfo *) GetNextValueInLinkedList(configure_list);
  }
  RelinquishSemaphoreInfo(configure_semaphore);
  qsort((void *) options,(size_t) i,sizeof(*options),ConfigureCompare);
  options[i]=(char *) NULL;
  *number_options=(unsigned long) i;
  return(options);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  G e t C o n f i g u r e O p t i o n s                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetConfigureOptions() returns any Magick configuration options associated
%  with the specified filename.
%
%  The format of the GetConfigureOptions method is:
%
%      LinkedListInfo *GetConfigureOptions(const char *filename,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: The configure file name.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport LinkedListInfo *GetConfigureOptions(const char *filename,
  ExceptionInfo *exception)
{
  char
    path[MaxTextExtent];

  const char
    *element;

  LinkedListInfo
    *options,
    *paths;

  StringInfo
    *xml;

  assert(filename != (const char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  assert(exception != (ExceptionInfo *) NULL);
  (void) CopyMagickString(path,filename,MaxTextExtent);
  /*
    Load XML from configuration files to linked-list.
  */
  options=NewLinkedList(0);
  paths=GetConfigurePaths(filename,exception);
  if (paths != (LinkedListInfo *) NULL)
    {
      ResetLinkedListIterator(paths);
      element=(const char *) GetNextValueInLinkedList(paths);
      while (element != (const char *) NULL)
      {
        (void) FormatMagickString(path,MaxTextExtent,"%s%s",element,filename);
        (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
          "Searching for configure file: \"%s\"",path);
        xml=ConfigureFileToStringInfo(path);
        if (xml != (StringInfo *) NULL)
          (void) AppendValueToLinkedList(options,xml);
        element=(const char *) GetNextValueInLinkedList(paths);
      }
      paths=DestroyLinkedList(paths,RelinquishMagickMemory);
    }
#if defined(__WINDOWS__)
  {
    char
      *blob;

    blob=(char *) NTResourceToBlob(filename);
    if (blob != (char *) NULL)
      {
        xml=StringToStringInfo(blob);
        SetStringInfoPath(xml,filename);
        (void) AppendValueToLinkedList(options,xml);
        blob=DestroyString(blob);
      }
  }
#endif
  if (GetNumberOfElementsInLinkedList(options) == 0)
    (void) ThrowMagickException(exception,GetMagickModule(),ConfigureWarning,
      "UnableToOpenConfigureFile","`%s'",filename);
  ResetLinkedListIterator(options);
  return(options);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  G e t C o n f i g u r e P a t h s                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetConfigurePaths() returns any Magick configuration paths associated
%  with the specified filename.
%
%  The format of the GetConfigurePaths method is:
%
%      LinkedListInfo *GetConfigurePaths(const char *filename,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: The configure file name.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport LinkedListInfo *GetConfigurePaths(const char *filename,
  ExceptionInfo *exception)
{
  char
    path[MaxTextExtent];

  LinkedListInfo
    *paths;

  assert(filename != (const char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  assert(exception != (ExceptionInfo *) NULL);
  (void) CopyMagickString(path,filename,MaxTextExtent);
  paths=NewLinkedList(0);
  {
    char
      *configure_path;

    /*
      Search $MAGICK_CONFIGURE_PATH.
    */
    configure_path=GetEnvironmentValue("MAGICK_CONFIGURE_PATH");
    if (configure_path != (char *) NULL)
      {
        register char
          *p,
          *q;

        for (p=configure_path-1; p != (char *) NULL; )
        {
          (void) CopyMagickString(path,p+1,MaxTextExtent);
          q=strchr(path,DirectoryListSeparator);
          if (q != (char *) NULL)
            *q='\0';
          q=path+strlen(path)-1;
          if ((q >= path) && (*q != *DirectorySeparator))
            (void) ConcatenateMagickString(path,DirectorySeparator,
              MaxTextExtent);
          (void) AppendValueToLinkedList(paths,ConstantString(path));
          p=strchr(p+1,DirectoryListSeparator);
        }
        configure_path=DestroyString(configure_path);
      }
  }
#if defined(UseInstalledMagick)
#if defined(MagickLibConfigPath)
  (void) AppendValueToLinkedList(paths,ConstantString(MagickLibConfigPath));
#endif
#if defined(MagickShareConfigPath)
  (void) AppendValueToLinkedList(paths,ConstantString(MagickShareConfigPath));
#endif
#if defined(MagickDocumentPath)
  (void) AppendValueToLinkedList(paths,ConstantString(MagickDocumentPath));
#endif
#if defined(MagickSharePath)
  (void) AppendValueToLinkedList(paths,ConstantString(MagickSharePath));
#endif
#if defined(__WINDOWS__) && !(defined(MagickLibConfigPath) || defined(MagickShareConfigPath))
  {
    char
      *registry_key,
      *key_value;

    /*
      Locate file via registry key.
    */
    registry_key="ConfigurePath";
    key_value=NTRegistryKeyLookup(registry_key);
    if (key_value != (char *) NULL)
      {
        (void) FormatMagickString(path,MaxTextExtent,"%s%s",key_value,
          DirectorySeparator);
        (void) AppendValueToLinkedList(paths,ConstantString(path));
      }
  }
#endif
#else
  {
    char
      *home;

    /*
      Search under MAGICK_HOME.
    */
    home=GetEnvironmentValue("MAGICK_HOME");
    if (home != (char *) NULL)
      {
#if !defined(POSIX)
        (void) FormatMagickString(path,MaxTextExtent,"%s%s",home,
          DirectorySeparator);
        (void) AppendValueToLinkedList(paths,ConstantString(path));
#else
        (void) FormatMagickString(path,MaxTextExtent,"%s/lib/%s/",home,
          MagickLibConfigSubDir);
        (void) AppendValueToLinkedList(paths,ConstantString(path));
        (void) FormatMagickString(path,MaxTextExtent,"%s/share/%s/",home,
          MagickShareConfigSubDir);
        (void) AppendValueToLinkedList(paths,ConstantString(path));
#endif
        home=DestroyString(home);
      }
    }
  if (*GetClientPath() != '\0')
    {
#if !defined(POSIX)
      (void) FormatMagickString(path,MaxTextExtent,"%s%s",GetClientPath(),
        DirectorySeparator);
      (void) AppendValueToLinkedList(paths,ConstantString(path));
#else
      char
        prefix[MaxTextExtent];

      /*
        Search based on executable directory if directory is known.
      */
      (void) CopyMagickString(prefix,GetClientPath(),MaxTextExtent);
      ChopPathComponents(prefix,1);
      (void) FormatMagickString(path,MaxTextExtent,"%s/lib/%s/",prefix,
        MagickLibConfigSubDir);
      (void) AppendValueToLinkedList(paths,ConstantString(path));
      (void) FormatMagickString(path,MaxTextExtent,"%s/share/%s/",prefix,
        MagickShareConfigSubDir);
      (void) AppendValueToLinkedList(paths,ConstantString(path));
#endif
    }
#endif
  {
    char
      *home;

    home=GetEnvironmentValue("HOME");
    if (home == (char *) NULL)
      home=GetEnvironmentValue("USERPROFILE");
    if (home != (char *) NULL)
      {
        /*
          Search $HOME/.magick.
        */
        (void) FormatMagickString(path,MaxTextExtent,"%s%s%s",home,
          *home == '/' ? "/.magick" : "",DirectorySeparator);
        (void) AppendValueToLinkedList(paths,ConstantString(path));
        home=DestroyString(home);
      }
  }
#if defined(__WINDOWS__)
  {
    char
      module_path[MaxTextExtent];

    if (NTGetModulePath("CORE_RL_magick_.dll",module_path) != MagickFalse)
      {
        char
          *element;

        /*
          Search module path.
        */
        (void) FormatMagickString(path,MaxTextExtent,"%s%s",module_path,
          DirectorySeparator);
        element=(char *) RemoveElementByValueFromLinkedList(paths,path);
        if (element != (char *) NULL)
          element=DestroyString(element);
        (void) AppendValueToLinkedList(paths,ConstantString(path));
      }
    if (NTGetModulePath("Magick.dll",module_path) != MagickFalse)
      {
        /*
          Search PerlMagick module path.
        */
        (void) FormatMagickString(path,MaxTextExtent,"%s%s",module_path,
          DirectorySeparator);
        (void) AppendValueToLinkedList(paths,ConstantString(path));
        (void) FormatMagickString(path,MaxTextExtent,"%s%s",module_path,
          "\\inc\\lib\\auto\\Image\\Magick\\");
        (void) AppendValueToLinkedList(paths,ConstantString(path));
      }
  }
#endif
  /*
    Search current directory.
  */
  (void) AppendValueToLinkedList(paths,ConstantString(""));
  return(paths);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t C o n f i g u r e V a l u e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetConfigureValue() returns the value associated with the configure info.
%
%  The format of the GetConfigureValue method is:
%
%      const char *GetConfigureValue(const ConfigureInfo *configure_info)
%
%  A description of each parameter follows:
%
%    o configure_info:  The configure info.
%
*/
MagickExport const char *GetConfigureValue(const ConfigureInfo *configure_info)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(configure_info != (ConfigureInfo *) NULL);
  assert(configure_info->signature == MagickSignature);
  return(configure_info->value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n i t i a l i z e C o n f i g u r e L i s t                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InitializeConfigureList() initializes the configure list.
%
%  The format of the InitializeConfigureList method is:
%
%      MagickBooleanType InitializeConfigureList(ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static MagickBooleanType InitializeConfigureList(ExceptionInfo *exception)
{
  if ((configure_list == (LinkedListInfo *) NULL) &&
      (instantiate_configure == MagickFalse))
    {
      AcquireSemaphoreInfo(&configure_semaphore);
      if ((configure_list == (LinkedListInfo *) NULL) &&
          (instantiate_configure == MagickFalse))
        {
          (void) LoadConfigureLists(ConfigureFilename,exception);
          instantiate_configure=MagickTrue;
        }
      RelinquishSemaphoreInfo(configure_semaphore);
    }
  return(configure_list != (LinkedListInfo *) NULL ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L i s t C o n f i g u r e I n f o                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListConfigureInfo() lists the configure info to a file.
%
%  The format of the ListConfigureInfo method is:
%
%      MagickBooleanType ListConfigureInfo(FILE *file,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  An pointer to a FILE.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType ListConfigureInfo(FILE *file,
  ExceptionInfo *exception)
{
  const char
    *name,
    *path,
    *value;

  const ConfigureInfo
    **configure_info;

  long
    j;

  register long
    i;

  unsigned long
    number_options;

  if (file == (const FILE *) NULL)
    file=stdout;
  configure_info=GetConfigureInfoList("*",&number_options,exception);
  if (configure_info == (const ConfigureInfo **) NULL)
    return(MagickFalse);
  path=(const char *) NULL;
  for (i=0; i < (long) number_options; i++)
  {
    if (configure_info[i]->stealth != MagickFalse)
      continue;
    if ((path == (const char *) NULL) ||
        (LocaleCompare(path,configure_info[i]->path) != 0))
      {
        if (configure_info[i]->path != (char *) NULL)
          (void) fprintf(file,"\nPath: %s\n\n",configure_info[i]->path);
        (void) fprintf(file,"Name          Value\n");
        (void) fprintf(file,"-------------------------------------------------"
          "------------------------------\n");
      }
    path=configure_info[i]->path;
    name="unknown";
    if (configure_info[i]->name != (char *) NULL)
      name=configure_info[i]->name;
    (void) fprintf(file,"%s",name);
    for (j=(long) strlen(name); j <= 12; j++)
      (void) fprintf(file," ");
    (void) fprintf(file," ");
    value="unknown";
    if (configure_info[i]->value != (char *) NULL)
      value=configure_info[i]->value;
    (void) fprintf(file,"%s",value);
    (void) fprintf(file,"\n");
  }
  (void) fflush(file);
  configure_info=(const ConfigureInfo **)
    RelinquishMagickMemory((void *) configure_info);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   L o a d C o n f i g u r e L i s t                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadConfigureList() loads the configure configuration file which provides a
%  mapping between configure attributes and a configure name.
%
%  The format of the LoadConfigureList method is:
%
%      MagickBooleanType LoadConfigureList(const char *xml,const char *filename,
%        const unsigned long depth,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o xml:  The configure list in XML format.
%
%    o filename:  The configure list filename.
%
%    o depth: depth of <include /> statements.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static MagickBooleanType LoadConfigureList(const char *xml,const char *filename,
  const unsigned long depth,ExceptionInfo *exception)
{
  const char
    *attribute;

  ConfigureInfo
    *configure_info = (ConfigureInfo *) NULL;

  MagickBooleanType
    status;

  XMLTreeInfo
    *configure,
    *configure_map,
    *include;

  /*
    Load the configure map file.
  */
  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Loading configure map \"%s\" ...",filename);
  if (xml == (const char *) NULL)
    return(MagickFalse);
  if (configure_list == (LinkedListInfo *) NULL)
    {
      configure_list=NewLinkedList(0);
      if (configure_list == (LinkedListInfo *) NULL)
        {
          ThrowFileException(exception,ResourceLimitError,
            "MemoryAllocationFailed",filename);
          return(MagickFalse);
        }
    }
  configure_map=NewXMLTree(xml,exception);
  if (configure_map == (XMLTreeInfo *) NULL)
    return(MagickFalse);
  status=MagickTrue;
  include=GetXMLTreeChild(configure_map,"include");
  while (include != (XMLTreeInfo *) NULL)
  {
    /*
      Process include element.
    */
    attribute=GetXMLTreeAttribute(include,"file");
    if (attribute != (const char *) NULL)
      {
        if (depth > 200)
          (void) ThrowMagickException(exception,GetMagickModule(),
            ConfigureError,"IncludeElementNestedTooDeeply","`%s'",filename);
        else
          {
            char
              path[MaxTextExtent],
              *xml;

            GetPathComponent(filename,HeadPath,path);
            if (*path != '\0')
              (void) ConcatenateMagickString(path,DirectorySeparator,
                MaxTextExtent);
            (void) ConcatenateMagickString(path,attribute,MaxTextExtent);
            xml=FileToString(path,~0,exception);
            if (xml != (char *) NULL)
              {
                status=LoadConfigureList(xml,path,depth+1,exception);
                xml=DestroyString(xml);
              }
          }
      }
    include=GetNextXMLTreeTag(include);
  }
  configure=GetXMLTreeChild(configure_map,"configure");
  while (configure != (XMLTreeInfo *) NULL)
  {
    const char
      *attribute;

    /*
      Process configure element.
    */
    configure_info=(ConfigureInfo *) AcquireMagickMemory(
      sizeof(*configure_info));
    if (configure_info == (ConfigureInfo *) NULL)
      ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
    (void) ResetMagickMemory(configure_info,0,sizeof(*configure_info));
    configure_info->path=ConstantString(filename);
    configure_info->signature=MagickSignature;
    attribute=GetXMLTreeAttribute(configure,"name");
    if (attribute != (const char *) NULL)
      configure_info->name=ConstantString(attribute);
    attribute=GetXMLTreeAttribute(configure,"stealth");
    if (attribute != (const char *) NULL)
      configure_info->stealth=IsMagickTrue(attribute);
    attribute=GetXMLTreeAttribute(configure,"value");
    if (attribute != (const char *) NULL)
      configure_info->value=ConstantString(attribute);
    status=AppendValueToLinkedList(configure_list,configure_info);
    if (status == MagickFalse)
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",filename);
    configure=GetNextXMLTreeTag(configure);
  }
  configure_map=DestroyXMLTree(configure_map);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L o a d C o n f i g u r e L i s t s                                        %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadConfigureList() loads one or more configure configuration file which
%  provides a mapping between configure attributes and a configure name.
%
%  The format of the LoadConfigureLists method is:
%
%      MagickBooleanType LoadConfigureLists(const char *filename,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: The font file name.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static MagickBooleanType LoadConfigureLists(const char *filename,
  ExceptionInfo *exception)
{
#if defined(UseEmbeddableMagick)
  return(LoadConfigureList(ConfigureMap,"built-in",0,exception));
#else
  const StringInfo
    *option;

  LinkedListInfo
    *options;

  MagickStatusType
    status;

  status=MagickFalse;
  options=GetConfigureOptions(filename,exception);
  option=(const StringInfo *) GetNextValueInLinkedList(options);
  while (option != (const StringInfo *) NULL)
  {
    status|=LoadConfigureList((const char *) GetStringInfoDatum(option),
      GetStringInfoPath(option),0,exception);
    option=(const StringInfo *) GetNextValueInLinkedList(options);
  }
  options=DestroyConfigureOptions(options);
  if ((configure_list == (LinkedListInfo *) NULL) ||
      (IsLinkedListEmpty(configure_list) != MagickFalse))
    status|=LoadConfigureList(ConfigureMap,"built-in",0,exception);
  return(status != 0 ? MagickTrue : MagickFalse);
#endif
}
