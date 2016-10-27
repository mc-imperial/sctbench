/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  L       OOO    CCCC   AAA   L      EEEEE                   %
%                  L      O   O  C      A   A  L      E                       %
%                  L      O   O  C      AAAAA  L      EEE                     %
%                  L      O   O  C      A   A  L      E                       %
%                  LLLLL   OOO    CCCC  A   A  LLLLL  EEEEE                   %
%                                                                             %
%                                                                             %
%                      ImageMagick Image Locale Methods                       %
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
#include "magick/locale_.h"
#include "magick/log.h"
#include "magick/memory_.h"
#include "magick/semaphore.h"
#include "magick/splay-tree.h"
#include "magick/string_.h"
#include "magick/token.h"
#include "magick/utility.h"
#include "magick/xml-tree.h"

/*
  Define declarations.
*/
#define LocaleFilename  "locale.xml"
#define MaxRecursionDepth  200

/*
  Static declarations.
*/
static const char
  *LocaleMap = (char *)
    "<?xml version=\"1.0\"?>"
    "<localemap>"
    "  <locale name=\"C\">"
    "    <Exception>"
    "     <Message name=\"\">"
    "     </Message>"
    "    </Exception>"
    "  </locale>"
    "</localemap>";

static SemaphoreInfo
  *locale_semaphore = (SemaphoreInfo *) NULL;

static SplayTreeInfo
  *locale_list = (SplayTreeInfo *) NULL;

static volatile MagickBooleanType
  instantiate_locale = MagickFalse;

/*
  Forward declarations.
*/
static MagickBooleanType
  InitializeLocaleList(ExceptionInfo *),
  LoadLocaleLists(const char *,const char *,ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y L o c a l e L i s t                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyLocaleList() deallocates memory associated with the locale list.
%
%  The format of the DestroyLocaleList method is:
%
%      DestroyLocaleList(void)
%
*/
MagickExport void DestroyLocaleList(void)
{
  AcquireSemaphoreInfo(&locale_semaphore);
  if (locale_list != (SplayTreeInfo *) NULL)
    locale_list=DestroySplayTree(locale_list);
  instantiate_locale=MagickFalse;
  RelinquishSemaphoreInfo(locale_semaphore);
  locale_semaphore=DestroySemaphoreInfo(locale_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y L o c a l e O p t i o n s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyLocaleOptions() releases memory associated with an locale
%  messages.
%
%  The format of the DestroyProfiles method is:
%
%      LinkedListInfo *DestroyLocaleOptions(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/

static void *DestroyOptions(void *message)
{
  return(DestroyStringInfo((StringInfo *) message));
}

MagickExport LinkedListInfo *DestroyLocaleOptions(LinkedListInfo *messages)
{
  assert(messages != (LinkedListInfo *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  return(DestroyLinkedList(messages,DestroyOptions));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t L o c a l e I n f o _                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetLocaleInfo_() searches the locale list for the specified tag and if
%  found returns attributes for that element.
%
%  The format of the GetLocaleInfo method is:
%
%      const LocaleInfo *GetLocaleInfo_(const char *tag,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o tag: The locale tag.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport const LocaleInfo *GetLocaleInfo_(const char *tag,
  ExceptionInfo *exception)
{
  assert(exception != (ExceptionInfo *) NULL);
  if ((locale_list == (SplayTreeInfo *) NULL) ||
      (instantiate_locale == MagickFalse))
    if (InitializeLocaleList(exception) == MagickFalse)
      return((const LocaleInfo *) NULL);
  if ((locale_list == (SplayTreeInfo *) NULL) ||
      (GetNumberOfNodesInSplayTree(locale_list) == 0))
    return((const LocaleInfo *) NULL);
  if ((tag == (const char *) NULL) || (LocaleCompare(tag,"*") == 0))
    {
      ResetSplayTreeIterator(locale_list);
      return((const LocaleInfo *) GetNextValueInSplayTree(locale_list));
    }
  return((const LocaleInfo *) GetValueFromSplayTree(locale_list,tag));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t L o c a l e I n f o L i s t                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetLocaleInfoList() returns any locale messages that match the
%  specified pattern.
%
%  The format of the GetLocaleInfoList function is:
%
%      const LocaleInfo **GetLocaleInfoList(const char *pattern,
%        unsigned long *number_messages,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_messages:  This integer returns the number of locale messages in
%    the list.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int LocaleInfoCompare(const void *x,const void *y)
{
  const LocaleInfo
    **p,
    **q;

  p=(const LocaleInfo **) x,
  q=(const LocaleInfo **) y;
  if (LocaleCompare((*p)->path,(*q)->path) == 0)
    return(LocaleCompare((*p)->tag,(*q)->tag));
  return(LocaleCompare((*p)->path,(*q)->path));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport const LocaleInfo **GetLocaleInfoList(const char *pattern,
  unsigned long *number_messages,ExceptionInfo *exception)
{
  const LocaleInfo
    **messages;

  register const LocaleInfo
    *p;

  register long
    i;

  /*
    Allocate locale list.
  */
  assert(pattern != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_messages != (unsigned long *) NULL);
  *number_messages=0;
  p=GetLocaleInfo_("*",exception);
  if (p == (const LocaleInfo *) NULL)
    return((const LocaleInfo **) NULL);
  messages=(const LocaleInfo **) AcquireQuantumMemory((size_t)
    GetNumberOfNodesInSplayTree(locale_list)+1UL,sizeof(*messages));
  if (messages == (const LocaleInfo **) NULL)
    return((const LocaleInfo **) NULL);
  /*
    Generate locale list.
  */
  AcquireSemaphoreInfo(&locale_semaphore);
  ResetSplayTreeIterator(locale_list);
  p=(const LocaleInfo *) GetNextValueInSplayTree(locale_list);
  for (i=0; p != (const LocaleInfo *) NULL; )
  {
    if ((p->stealth == MagickFalse) &&
        (GlobExpression(p->tag,pattern,MagickTrue) != MagickFalse))
      messages[i++]=p;
    p=(const LocaleInfo *) GetNextValueInSplayTree(locale_list);
  }
  RelinquishSemaphoreInfo(locale_semaphore);
  qsort((void *) messages,(size_t) i,sizeof(*messages),LocaleInfoCompare);
  messages[i]=(LocaleInfo *) NULL;
  *number_messages=(unsigned long) i;
  return(messages);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t L o c a l e L i s t                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetLocaleList() returns any locale messages that match the specified
%  pattern.
%
%  The format of the GetLocaleList function is:
%
%      char **GetLocaleList(const char *pattern,unsigned long *number_messages,
%        Exceptioninfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_messages:  This integer returns the number of messages in the
%      list.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int LocaleTagCompare(const void *x,const void *y)
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

MagickExport char **GetLocaleList(const char *pattern,
  unsigned long *number_messages,ExceptionInfo *exception)
{
  char
    **messages;

  register const LocaleInfo
    *p;

  register long
    i;

  /*
    Allocate locale list.
  */
  assert(pattern != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_messages != (unsigned long *) NULL);
  *number_messages=0;
  p=GetLocaleInfo_("*",exception);
  if (p == (const LocaleInfo *) NULL)
    return((char **) NULL);
  AcquireSemaphoreInfo(&locale_semaphore);
  RelinquishSemaphoreInfo(locale_semaphore);
  messages=(char **) AcquireQuantumMemory((size_t)
    GetNumberOfNodesInSplayTree(locale_list)+1UL,sizeof(*messages));
  if (messages == (char **) NULL)
    return((char **) NULL);
  AcquireSemaphoreInfo(&locale_semaphore);
  p=(const LocaleInfo *) GetNextValueInSplayTree(locale_list);
  for (i=0; p != (const LocaleInfo *) NULL; )
  {
    if ((p->stealth == MagickFalse) &&
        (GlobExpression(p->tag,pattern,MagickTrue) != MagickFalse))
      messages[i++]=ConstantString(p->tag);
    p=(const LocaleInfo *) GetNextValueInSplayTree(locale_list);
  }
  RelinquishSemaphoreInfo(locale_semaphore);
  qsort((void *) messages,(size_t) i,sizeof(*messages),LocaleTagCompare);
  messages[i]=(char *) NULL;
  *number_messages=(unsigned long) i;
  return(messages);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t L o c a l e M e s s a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetLocaleMessage() returns a message in the current locale that matches the
%  supplied tag.
%
%  The format of the GetLocaleMessage method is:
%
%      const char *GetLocaleMessage(const char *tag)
%
%  A description of each parameter follows:
%
%    o tag: Return a message that matches this tag in the current locale.
%
*/
MagickExport const char *GetLocaleMessage(const char *tag)
{
  char
    name[MaxTextExtent];

  const LocaleInfo
    *locale_info;

  ExceptionInfo
    *exception;

  if ((tag == (const char *) NULL) || (*tag == '\0'))
    return(tag);
  exception=AcquireExceptionInfo();
  (void) FormatMagickString(name,MaxTextExtent,"%s/",tag);
  locale_info=GetLocaleInfo_(name,exception);
  exception=DestroyExceptionInfo(exception);
  if (locale_info != (const LocaleInfo *) NULL)
    return(locale_info->message);
  return(tag);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  G e t L o c a l e O p t i o n s                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetLocaleOptions() returns any Magick configuration messages associated
%  with the specified filename.
%
%  The format of the GetLocaleOptions method is:
%
%      LinkedListInfo *GetLocaleOptions(const char *filename,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: The locale file tag.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport LinkedListInfo *GetLocaleOptions(const char *filename,
  ExceptionInfo *exception)
{
  char
    path[MaxTextExtent];

  const char
    *element;

  LinkedListInfo
    *messages,
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
  messages=NewLinkedList(0);
  paths=GetConfigurePaths(filename,exception);
  if (paths != (LinkedListInfo *) NULL)
    {
      ResetLinkedListIterator(paths);
      element=(const char *) GetNextValueInLinkedList(paths);
      while (element != (const char *) NULL)
      {
        (void) FormatMagickString(path,MaxTextExtent,"%s%s",element,filename);
        (void) LogMagickEvent(LocaleEvent,GetMagickModule(),
          "Searching for locale file: \"%s\"",path);
        xml=ConfigureFileToStringInfo(path);
        if (xml != (StringInfo *) NULL)
          (void) AppendValueToLinkedList(messages,xml);
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
        (void) AppendValueToLinkedList(messages,xml);
        blob=DestroyString(blob);
      }
  }
#endif
  ResetLinkedListIterator(messages);
  return(messages);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t L o c a l e V a l u e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetLocaleValue() returns the message associated with the locale info.
%
%  The format of the GetLocaleValue method is:
%
%      const char *GetLocaleValue(const LocaleInfo *locale_info)
%
%  A description of each parameter follows:
%
%    o locale_info:  The locale info.
%
*/
MagickExport const char *GetLocaleValue(const LocaleInfo *locale_info)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(locale_info != (LocaleInfo *) NULL);
  assert(locale_info->signature == MagickSignature);
  return(locale_info->message);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n i t i a l i z e L o c a l e L i s t                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InitializeLocaleList() initializes the locale list.
%
%  The format of the InitializeLocaleList method is:
%
%      MagickBooleanType InitializeLocaleList(ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static MagickBooleanType InitializeLocaleList(ExceptionInfo *exception)
{
  if ((locale_list == (SplayTreeInfo *) NULL) &&
      (instantiate_locale == MagickFalse))
    {
      AcquireSemaphoreInfo(&locale_semaphore);
      if ((locale_list == (SplayTreeInfo *) NULL) &&
          (instantiate_locale == MagickFalse))
        {
          char
            *locale;

          register const char
            *p;

          (void) setlocale(LC_ALL,"");
          locale=(char *) NULL;
          p=setlocale(LC_CTYPE,(const char *) NULL);
          if (p != (const char *) NULL)
            locale=ConstantString(p);
          if (locale == (char *) NULL)
            locale=GetEnvironmentValue("LC_ALL");
          if (locale == (char *) NULL)
            locale=GetEnvironmentValue("LC_MESSAGES");
          if (locale == (char *) NULL)
            locale=GetEnvironmentValue("LC_CTYPE");
          if (locale == (char *) NULL)
            locale=GetEnvironmentValue("LANG");
          if (locale == (char *) NULL)
            locale=ConstantString("C");
          (void) LoadLocaleLists(LocaleFilename,locale,exception);
          locale=DestroyString(locale);
          instantiate_locale=MagickTrue;
        }
      RelinquishSemaphoreInfo(locale_semaphore);
    }
  return(locale_list != (SplayTreeInfo *) NULL ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L i s t L o c a l e I n f o                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListLocaleInfo() lists the locale info to a file.
%
%  The format of the ListLocaleInfo method is:
%
%      MagickBooleanType ListLocaleInfo(FILE *file,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  An pointer to a FILE.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType ListLocaleInfo(FILE *file,
  ExceptionInfo *exception)
{
  const char
    *path;

  const LocaleInfo
    **locale_info;

  register long
    i;

  unsigned long
    number_messages;

  if (file == (const FILE *) NULL)
    file=stdout;
  number_messages=0;
  locale_info=GetLocaleInfoList("*",&number_messages,exception);
  if (locale_info == (const LocaleInfo **) NULL)
    return(MagickFalse);
  path=(const char *) NULL;
  for (i=0; i < (long) number_messages; i++)
  {
    if (locale_info[i]->stealth != MagickFalse)
      continue;
    if ((path == (const char *) NULL) ||
        (LocaleCompare(path,locale_info[i]->path) != 0))
      {
        if (locale_info[i]->path != (char *) NULL)
          (void) fprintf(file,"\nPath: %s\n\n",locale_info[i]->path);
        (void) fprintf(file,"Tag/Message\n");
        (void) fprintf(file,"-------------------------------------------------"
          "------------------------------\n");
      }
    path=locale_info[i]->path;
    (void) fprintf(file,"%s\n",locale_info[i]->tag);
    if (locale_info[i]->message != (char *) NULL)
      (void) fprintf(file,"  %s",locale_info[i]->message);
    (void) fprintf(file,"\n");
  }
  (void) fflush(file);
  locale_info=(const LocaleInfo **)
    RelinquishMagickMemory((void *) locale_info);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   L o a d L o c a l e L i s t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadLocaleList() loads the locale configuration file which provides a mapping
%  between locale attributes and a locale name.
%
%  The format of the LoadLocaleList method is:
%
%      MagickBooleanType LoadLocaleList(const char *xml,const char *filename,
%        const unsigned long depth,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o xml:  The locale list in XML format.
%
%    o filename:  The locale list filename.
%
%    o depth: depth of <include /> statements.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

static void *DestroyLocaleNode(void *locale_info)
{
  register LocaleInfo
    *p;

  p=(LocaleInfo *) locale_info;
  if (p->path != (char *) NULL)
    p->path=DestroyString(p->path);
  if (p->tag != (char *) NULL)
    p->tag=DestroyString(p->tag);
  if (p->message != (char *) NULL)
    p->message=DestroyString(p->message);
  return(RelinquishMagickMemory(p));
}

static MagickBooleanType TraverseLocaleMap(const char *filename,
  XMLTreeInfo **components,XMLTreeInfo *node,const unsigned long depth,
  ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  status=MagickTrue;
  if (depth >= MaxRecursionDepth)
    (void) ThrowMagickException(exception,GetMagickModule(),ConfigureError,
      "IncludeElementNestedTooDeeply","`%s'",filename);
  else
    if (node != (XMLTreeInfo *) NULL)
      {
        XMLTreeInfo
          *child,
          *sibling;

        components[depth]=node;
        if (strcmp(GetXMLTreeTag(node),"message") == 0)
          {
            char
              *message,
              *tag;

            const char
              *attribute,
              *content;

            LocaleInfo
              *locale_info;

            register long
              i;

            locale_info=(LocaleInfo *) AcquireMagickMemory(
              sizeof(*locale_info));
            if (locale_info == (LocaleInfo *) NULL)
              ThrowFatalException(ResourceLimitFatalError,
                "MemoryAllocationFailed");
            (void) ResetMagickMemory(locale_info,0,sizeof(*locale_info));
            locale_info->path=ConstantString(filename);
            locale_info->signature=MagickSignature;
            tag=AcquireString((char *) NULL);
            for (i=1; i < (long) depth; i++)
            {
              (void) ConcatenateString(&tag,GetXMLTreeTag(components[i]));
              (void) ConcatenateString(&tag,"/");
            }
            attribute=GetXMLTreeAttribute(node,"name");
            if (attribute != (const char *) NULL)
              {
                (void) ConcatenateString(&tag,attribute);
                (void) ConcatenateString(&tag,"/");
              }
            locale_info->tag=ConstantString(tag);
            tag=DestroyString(tag);
            message=AcquireString((char *) NULL);
            content=GetXMLTreeContent(node);
            if (content != (const char *) NULL)
              {
                (void) ConcatenateString(&message,content);
                StripString(message);
              }
            locale_info->message=ConstantString(message);
            message=DestroyString(message);
            status=AddValueToSplayTree(locale_list,locale_info->tag,
              locale_info);
            if (status == MagickFalse)
              (void) ThrowMagickException(exception,GetMagickModule(),
                ResourceLimitError,"MemoryAllocationFailed","`%s'",filename);
          }
        child=GetXMLTreeChild(node,(const char *) NULL);
        if (child != (XMLTreeInfo *) NULL)
          status=TraverseLocaleMap(filename,components,child,depth+1,exception);
        sibling=GetXMLTreeOrdered(node);
        if (sibling != (XMLTreeInfo *) NULL)
          status=TraverseLocaleMap(filename,components,sibling,depth,exception);
      }
  return(status);
}

static MagickBooleanType LoadLocaleList(const char *xml,const char *filename,
  const char *locale,const unsigned long depth,ExceptionInfo *exception)
{
  const char
    *attribute;

  MagickBooleanType
    status;

  XMLTreeInfo
    *include,
    **components,
    *locale_map;

  /*
    Load the locale map file.
  */
  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Loading locale map \"%s\" ...",filename);
  if (xml == (const char *) NULL)
    return(MagickFalse);
  if (locale_list == (SplayTreeInfo *) NULL)
    {
      locale_list=NewSplayTree(CompareSplayTreeString,(void *(*)(void *)) NULL,
        DestroyLocaleNode);
      if (locale_list == (SplayTreeInfo *) NULL)
        {
          ThrowFileException(exception,ResourceLimitError,
            "MemoryAllocationFailed",filename);
          return(MagickFalse);
        }
    }
  locale_map=NewXMLTree(xml,exception);
  if (locale_map == (XMLTreeInfo *) NULL)
    return(MagickFalse);
  include=GetXMLTreeChild(locale_map,"include");
  while (include != (XMLTreeInfo *) NULL)
  {
    /*
      Process include element.
    */
    attribute=GetXMLTreeAttribute(include,"locale");
    if ((attribute != (const char *) NULL) &&
        (LocaleCompare(locale,attribute) != 0))
      {
        include=GetNextXMLTreeTag(include);
        continue;
      }
    attribute=GetXMLTreeAttribute(include,"file");
    if (attribute != (const char *) NULL)
      {
        if (depth > MaxRecursionDepth)
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
                status=LoadLocaleList(xml,path,locale,depth+1,exception);
                xml=DestroyString(xml);
              }
          }
      }
    include=GetNextXMLTreeTag(include);
  }
  components=(XMLTreeInfo **) AcquireQuantumMemory(MaxRecursionDepth,
    sizeof(*components));
  if (components == (XMLTreeInfo **) NULL)
    {
      locale_map=DestroyXMLTree(locale_map);
      ThrowFileException(exception,ResourceLimitError,
        "MemoryAllocationFailed",filename);
      return(MagickFalse);
    }
  status=TraverseLocaleMap(filename,components,locale_map,0,exception);
  components=(XMLTreeInfo **) RelinquishMagickMemory(components);
  locale_map=DestroyXMLTree(locale_map);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L o a d L o c a l e L i s t s                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadLocaleList() loads one or more locale configuration file which
%  provides a mapping between locale attributes and a locale tag.
%
%  The format of the LoadLocaleLists method is:
%
%      MagickBooleanType LoadLocaleLists(const char *filename,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: The font file tag.
%
%    o locale: The actual locale.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static MagickBooleanType LoadLocaleLists(const char *filename,
  const char *locale,ExceptionInfo *exception)
{
#if defined(UseEmbeddableMagick)
  return(LoadLocaleList(LocaleMap,"built-in",locale,0,exception));
#else
  const StringInfo
    *option;

  LinkedListInfo
    *options;

  MagickStatusType
    status;

  status=MagickFalse;
  options=GetLocaleOptions(filename,exception);
  option=(const StringInfo *) GetNextValueInLinkedList(options);
  while (option != (const StringInfo *) NULL)
  {
    status|=LoadLocaleList((const char *) GetStringInfoDatum(option),
      GetStringInfoPath(option),locale,0,exception);
    option=(const StringInfo *) GetNextValueInLinkedList(options);
  }
  options=DestroyLocaleOptions(options);
  if ((locale_list == (SplayTreeInfo *) NULL) ||
      (GetNumberOfNodesInSplayTree(locale_list) == 0))
    {
      options=GetLocaleOptions("english.xml",exception);
      option=(const StringInfo *) GetNextValueInLinkedList(options);
      while (option != (const StringInfo *) NULL)
      {
        status|=LoadLocaleList((const char *) GetStringInfoDatum(option),
          GetStringInfoPath(option),locale,0,exception);
        option=(const StringInfo *) GetNextValueInLinkedList(options);
      }
      options=DestroyLocaleOptions(options);
    }
  if ((locale_list == (SplayTreeInfo *) NULL) ||
      (GetNumberOfNodesInSplayTree(locale_list) == 0))
    status|=LoadLocaleList(LocaleMap,"built-in",locale,0,exception);
  return(status != 0 ? MagickTrue : MagickFalse);
#endif
}
