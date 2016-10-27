/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                AAA   N   N  IIIII  M   M   AAA   TTTTT  EEEEE               %
%               A   A  NN  N    I    MM MM  A   A    T    E                   %
%               AAAAA  N N N    I    M M M  AAAAA    T    EEE                 %
%               A   A  N  NN    I    M   M  A   A    T    E                   %
%               A   A  N   N  IIIII  M   M  A   A    T    EEEEE               %
%                                                                             %
%                                                                             %
%              Methods to Interactively Animate an Image Sequence             %
%                                                                             %
%                             Software Design                                 %
%                               John Cristy                                   %
%                                July 1992                                    %
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
#include "wand/studio.h"
#include "wand/MagickWand.h"
#include "wand/mogrify-private.h"
#include "magick/animate-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A n i m a t e I m a g e C o m m a n d                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AnimateImageCommand() displays a sequence of images on any workstation
%  display running an X server. Animate first determines the hardware
%  capabilities of the workstation. If the number of unique colors in an image
%  is less than or equal to the number the workstation can support, the image
%  is displayed in an X window. Otherwise the number of colors in the image is
%  first reduced to match the color resolution of the workstation before it is
%  displayed.
%
%  This means that a continuous-tone 24 bits/pixel image can display on a 8
%  bit pseudo-color device or monochrome device. In most instances the reduced
%  color image closely resembles the original. Alternatively, a monochrome or
%  pseudo-color image sequence can display on a continuous-tone 24 bits/pixels
%  device.
%
%  The format of the AnimateImageCommand method is:
%
%      MagickBooleanType AnimateImageCommand(ImageInfo *image_info,int argc,
%        char **argv,char **metadata,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o argc: The number of elements in the argument vector.
%
%    o argv: A text array containing the command line arguments.
%
%    o metadata: any metadata is returned here.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

static void AnimateUsage(void)
{
  const char
    **p;

  static const char
    *buttons[]=
    {
      "Press any button to map or unmap the Command widget",
      (char *) NULL
    },
    *operators[]=
    {
      "-colors value        preferred number of colors in the image",
      "-crop geometry       preferred size and location of the cropped image",
      "-extract geometry    extract area from image",
      "-monochrome          transform image to black and white",
      "-repage geometry     size and location of an image canvas (operator)",
      "-resample geometry   change the resolution of an image",
      "-resize geometry     resize the image",
      "-rotate degrees      apply Paeth rotation to the image",
      "-strip               strip image of all profiles and comments",
      "-trim                trim image edges",
      (char *) NULL
    },
    *settings[]=
    {
      "-alpha option        activate, deactivate, reset, or set the alpha channel",
      "-authenticate value  decrypt image with this password",
      "-backdrop            display image centered on a backdrop",
      "-channel type        apply option to select image channels",
      "-colormap type       Shared or Private",
      "-colorspace type     alternate image colorspace",
      "-debug events        display copious debugging information",
      "-define format:option",
      "                     define one or more image format options",
      "-delay value         display the next image after pausing",
      "-density geometry    horizontal and vertical density of the image",
      "-depth value         image depth",
      "-display server      display image to this X server",
      "-dither              apply Floyd/Steinberg error diffusion to image",
      "-format \"string\"     output formatted image characteristics",
      "-gamma value         level of gamma correction",
      "-geometry geometry   preferred size and location of the Image window",
      "-help                print program options",
      "-identify            identify the format and characteristics of the image",
      "-interlace type      type of image interlacing scheme",
      "-interpolate method  pixel color interpolation method",
      "-limit type value    pixel cache resource limit",
      "-log format          format of debugging information",
      "-loop iterations     loop images then exit",
      "-map type            display image using this Standard Colormap",
      "-monitor             monitor progress",
      "-pause               seconds to pause before reanimating",
      "-page geometry       size and location of an image canvas (setting)",
      "-quantize colorspace reduce colors in this colorspace",
      "-quiet               suppress all warning messages",
      "-regard-warnings     pay attention to warning messages",
      "-remote command      execute a command in an remote display process",
      "-sampling-factor geometry",
      "                     horizontal and vertical sampling factor",
      "-scenes range        image scene range",
      "-seed value          seed a new sequence of pseudo-random numbers",
      "-set attribute value set an image attribute",
      "-size geometry       width and height of image",
      "-support factor      resize support: > 1.0 is blurry, < 1.0 is sharp",
      "-transparent-color color",
      "                     transparent color",
      "-treedepth value     color tree depth",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      "-visual type         display image using this visual type",
      "-virtual-pixel method",
      "                     virtual pixel access method",
      "-window id           display image to background of this window",
      (char *) NULL
    },
    *sequence_operators[]=
    {
      "-coalesce            merge a sequence of images",
      "-flatten             flatten a sequence of images",
      (char *) NULL
    };

  (void) printf("Version: %s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %s\n\n",GetMagickCopyright());
  (void) printf("Usage: %s [options ...] file [ [options ...] file ...]\n",
    GetClientName());
  (void) printf("\nImage Settings:\n");
  for (p=settings; *p != (char *) NULL; p++)
    (void) printf("  %s\n",*p);
  (void) printf("\nImage Operators:\n");
  for (p=operators; *p != (char *) NULL; p++)
    (void) printf("  %s\n",*p);
  (void) printf("\nImage Sequence Operators:\n");
  for (p=sequence_operators; *p != (char *) NULL; p++)
    (void) printf("  %s\n",*p);
  (void) printf(
    "\nIn addition to those listed above, you can specify these standard X\n");
  (void) printf(
    "resources as command line options:  -background, -bordercolor,\n");
  (void) printf(
    "-borderwidth, -font, -foreground, -iconGeometry, -iconic, -name,\n");
  (void) printf("-mattecolor, -shared-memory, or -title.\n");
  (void) printf(
    "\nBy default, the image format of `file' is determined by its magic\n");
  (void) printf(
    "number.  To specify a particular image format, precede the filename\n");
  (void) printf(
    "with an image format name and a colon (i.e. ps:image) or specify the\n");
  (void) printf(
    "image type as the filename suffix (i.e. image.ps).  Specify 'file' as\n");
  (void) printf("'-' for standard input or output.\n");
  (void) printf("\nButtons: \n");
  for (p=buttons; *p != (char *) NULL; p++)
    (void) printf("  %s\n",*p);
  exit(0);
}

WandExport MagickBooleanType AnimateImageCommand(ImageInfo *image_info,
  int argc,char **argv,char **wand_unused(metadata),ExceptionInfo *exception)
{
#if defined(HasX11)
#define DestroyAnimate() \
{ \
  XDestroyResourceInfo(&resource_info); \
  if (display != (Display *) NULL) \
    { \
      XCloseDisplay(display); \
      display=(Display *) NULL; \
    } \
  XDestroyResourceInfo(&resource_info); \
  for ( ; k >= 0; k--) \
    image_stack[k]=DestroyImageList(image_stack[k]); \
  for (i=0; i < (long) argc; i++) \
    argv[i]=DestroyString(argv[i]); \
  argv=(char **) RelinquishMagickMemory(argv); \
}
#define ThrowAnimateException(asperity,tag,option) \
{ \
  (void) ThrowMagickException(exception,GetMagickModule(),asperity,tag,"`%s'", \
    option); \
  DestroyAnimate(); \
  return(MagickFalse); \
}
#define ThrowAnimateInvalidArgumentException(option,argument) \
{ \
  (void) ThrowMagickException(exception,GetMagickModule(),OptionError, \
    "InvalidArgument","`%s': %s",argument,option); \
  DestroyAnimate(); \
  return(MagickFalse); \
}

  char
    *resource_value,
    *server_name;

  const char
    *option;

  Display
    *display;

  Image
    *image_stack[MaxImageStackDepth+1];

  long
    first_scene,
    j,
    k,
    last_scene,
    scene;

  MagickBooleanType
    fire,
    pend;

  MagickStatusType
    status;

  QuantizeInfo
    *quantize_info;

  register long
    i;

  XResourceInfo
    resource_info;

  XrmDatabase
    resource_database;

  /*
    Set defaults.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(exception != (ExceptionInfo *) NULL);
  if (argc == 2)
    {
      option=argv[1];
      if ((LocaleCompare("version",option+1) == 0) ||
          (LocaleCompare("-version",option+1) == 0))
        {
          (void) fprintf(stdout,"Version: %s\n",
            GetMagickVersion((unsigned long *) NULL));
          (void) fprintf(stdout,"Copyright: %s\n\n",GetMagickCopyright());
          return(MagickTrue);
        }
    }
  status=MagickTrue;
  SetNotifyHandlers;
  display=(Display *) NULL;
  first_scene=0;
  j=1;
  k=0;
  image_stack[k]=NewImageList();
  last_scene=0;
  option=(char *) NULL;
  pend=MagickFalse;
  resource_database=(XrmDatabase) NULL;
  (void) ResetMagickMemory(&resource_info,0,sizeof(XResourceInfo));
  server_name=(char *) NULL;
  status=MagickTrue;
  /*
    Check for server name specified on the command line.
  */
  ReadCommandlLine(argc,&argv);
  status=ExpandFilenames(&argc,&argv);
  if (status == MagickFalse)
    ThrowAnimateException(ResourceLimitError,"MemoryAllocationFailed",
      image_info->filename);
  for (i=1; i < (long) argc; i++)
  {
    /*
      Check command line for server name.
    */
    option=argv[i];
    if (IsMagickOption(option) == MagickFalse)
      continue;
    if (LocaleCompare("display",option+1) == 0)
      {
        /*
          User specified server name.
        */
        i++;
        if (i == (long) argc)
          ThrowAnimateException(OptionError,"MissingArgument",option);
        server_name=argv[i];
      }
    if ((LocaleCompare("help",option+1) == 0) ||
        (LocaleCompare("-help",option+1) == 0))
      AnimateUsage();
  }
  /*
    Get user defaults from X resource database.
  */
  display=XOpenDisplay(server_name);
  if (display == (Display *) NULL)
    ThrowAnimateException(XServerError,"UnableToOpenXServer",
      XDisplayName(server_name));
  (void) XSetErrorHandler(XError);
  resource_database=XGetResourceDatabase(display,GetClientName());
  XGetResourceInfo(resource_database,GetClientName(),&resource_info);
  image_info=resource_info.image_info;
  quantize_info=resource_info.quantize_info;
  image_info->density=XGetResourceInstance(resource_database,GetClientName(),
    "density",(char *) NULL);
  if (image_info->density == (char *) NULL)
    image_info->density=XGetScreenDensity(display);
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "interlace","none");
  image_info->interlace=(InterlaceType)
    ParseMagickOption(MagickInterlaceOptions,MagickFalse,resource_value);
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "verbose","False");
  image_info->verbose=IsMagickTrue(resource_value);
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "dither","True");
  quantize_info->dither=IsMagickTrue(resource_value);
  /*
    Parse command line.
  */
  for (i=1; i <= (long) argc; i++)
  {
    if (i < (long) argc)
      option=argv[i];
    else
      if (image_stack[k] != (Image *) NULL)
        break;
      else
        if (isatty(STDIN_FILENO) != MagickFalse)
          option="logo:";
        else
          {
            int
              c;

            c=getc(stdin);
            if (c == EOF)
              option="logo:";
            else
              {
                c=ungetc(c,stdin);
                option="-";
              }
          }
    if (LocaleCompare(option,"(") == 0)
      {
        MogrifyImageStack(image_stack[k],MagickTrue,pend);
        if (k == MaxImageStackDepth)
          ThrowAnimateException(OptionError,"ParenthesisNestedTooDeeply",
            option);
        k++;
        image_stack[k]=NewImageList();
        continue;
      }
    if (LocaleCompare(option,")") == 0)
      {
        if (k == 0)
          ThrowAnimateException(OptionError,"UnableToParseExpression",option);
        if (image_stack[k] != (Image *) NULL)
          {
            MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
            AppendImageToList(&image_stack[k-1],image_stack[k]);
          }
        k--;
        continue;
      }
    if (IsMagickOption(option) == MagickFalse)
      {
        Image
          *image;

        /*
          Option is a file name.
        */
        MogrifyImageStack(image_stack[k],MagickTrue,pend);
        for (scene=first_scene; scene <= last_scene ; scene++)
        {
          const char
            *filename;

          /*
            Read image.
          */
          filename=option;
          if ((LocaleCompare(filename,"--") == 0) && (i < (argc-1)))
            {
              option=argv[++i];
              filename=option;
            }
          (void) CopyMagickString(image_info->filename,filename,MaxTextExtent);
          if (first_scene != last_scene)
            {
              char
                filename[MaxTextExtent];

              /*
                Form filename for multi-part images.
              */
              (void) InterpretImageFilename(filename,MaxTextExtent,
                image_info->filename,(int) scene);
              if (LocaleCompare(filename,image_info->filename) == 0)
                (void) FormatMagickString(filename,MaxTextExtent,"%s[%lu]",
                  image_info->filename,scene);
              (void) CopyMagickString(image_info->filename,filename,
                MaxTextExtent);
            }
          image=ReadImage(image_info,exception);
          status&=(image != (Image *) NULL) &&
            (exception->severity < ErrorException);
          if (image == (Image *) NULL)
            continue;
          AppendImageToList(&image_stack[k],image);
        }
        continue;
      }
    pend=image_stack[k] != (Image *) NULL ? MagickTrue : MagickFalse;
    switch (*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("alpha",option+1) == 0)
          {
            long
              type;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            type=ParseMagickOption(MagickAlphaOptions,MagickFalse,argv[i]);
            if (type < 0)
              ThrowAnimateException(OptionError,"UnrecognizedAlphaChannelType",
                argv[i]);
            break;
          }
        if (LocaleCompare("authenticate",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 'b':
      {
        if (LocaleCompare("backdrop",option+1) == 0)
          {
            resource_info.backdrop=(*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        if (LocaleCompare("background",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            resource_info.background_color=argv[i];
            break;
          }
        if (LocaleCompare("bordercolor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            resource_info.border_color=argv[i];
            break;
          }
        if (LocaleCompare("borderwidth",option+1) == 0)
          {
            resource_info.border_width=0;
            if (*option == '+')
              break;
            i++;
            if ((i == (long) argc) || (IsGeometry(argv[i]) == MagickFalse))
              ThrowAnimateException(OptionError,"MissingArgument",option);
            resource_info.border_width=(unsigned int) atoi(argv[i]);
            break;
          }
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 'c':
      {
        if (LocaleCompare("cache",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("channel",option+1) == 0)
          {
            long
              channel;

            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowAnimateException(OptionError,"MissingArgument",option);
            channel=ParseChannelOption(argv[i]);
            if (channel < 0)
              ThrowAnimateException(OptionError,"UnrecognizedChannelType",
                argv[i]);
            break;
          }
        if (LocaleCompare("clone",option+1) == 0)
          {
            Image
              *clone_images;

            clone_images=image_stack[k];
            if (k != 0)
              clone_images=image_stack[k-1];
            if (clone_images == (Image *) NULL)
              ThrowAnimateException(ImageError,"ImageSequenceRequired",option);
            if (*option == '+')
              clone_images=CloneImages(clone_images,"-1",exception);
            else
              {
                i++;
                if (i == (long) (argc-1))
                  ThrowAnimateException(OptionError,"MissingArgument",option);
                if (IsSceneGeometry(argv[i],MagickFalse) == MagickFalse)
                  ThrowAnimateInvalidArgumentException(option,argv[i]);
                clone_images=CloneImages(clone_images,argv[i],exception);
              }
            if (clone_images == (Image *) NULL)
              ThrowAnimateException(OptionError,"NoSuchImage",option);
            MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
            AppendImageToList(&image_stack[k],clone_images);
            break;
          }
        if (LocaleCompare("coalesce",option+1) == 0)
          break;
        if (LocaleCompare("colormap",option+1) == 0)
          {
            resource_info.colormap=PrivateColormap;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            resource_info.colormap=UndefinedColormap;
            if (LocaleCompare("private",argv[i]) == 0)
              resource_info.colormap=PrivateColormap;
            if (LocaleCompare("shared",argv[i]) == 0)
              resource_info.colormap=SharedColormap;
            if (resource_info.colormap == UndefinedColormap)
              ThrowAnimateException(OptionError,"UnrecognizedColormapType",
                argv[i]);
            break;
          }
        if (LocaleCompare("colors",option+1) == 0)
          {
            quantize_info->number_colors=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            quantize_info->number_colors=(unsigned long) atol(argv[i]);
            break;
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            long
              colorspace;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            colorspace=ParseMagickOption(MagickColorspaceOptions,
              MagickFalse,argv[i]);
            if (colorspace < 0)
              ThrowAnimateException(OptionError,"UnrecognizedColorspace",
                argv[i]);
            break;
          }
        if (LocaleCompare("crop",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 'd':
      {
        if (LocaleCompare("debug",option+1) == 0)
          {
            long
              event;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            event=ParseMagickOption(MagickLogEventOptions,MagickFalse,argv[i]);
            if (event < 0)
              ThrowAnimateException(OptionError,"UnrecognizedEventType",
                argv[i]);
            (void) SetLogEventMask(argv[i]);
            break;
          }
        if (LocaleCompare("define",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (*option == '+')
              {
                const char
                  *define;

                define=GetImageOption(image_info,argv[i]);
                if (define == (const char *) NULL)
                  ThrowAnimateException(OptionError,"NoSuchOption",argv[i]);
                break;
              }
            break;
          }
        if (LocaleCompare("delay",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("density",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("display",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("dither",option+1) == 0)
          {
            quantize_info->dither=(*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 'e':
      {
        if (LocaleCompare("extract",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 'f':
      {
        if (LocaleCompare("flatten",option+1) == 0)
          break;
        if (LocaleCompare("font",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            resource_info.font=XGetResourceClass(resource_database,
              GetClientName(),"font",argv[i]);
            break;
          }
        if (LocaleCompare("foreground",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            resource_info.foreground_color=argv[i];
            break;
          }
        if (LocaleCompare("format",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowAnimateException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 'g':
      {
        if (LocaleCompare("gamma",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("geometry",option+1) == 0)
          {
            resource_info.image_geometry=(char *) NULL;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            resource_info.image_geometry=ConstantString(argv[i]);
            break;
          }
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 'h':
      {
        if ((LocaleCompare("help",option+1) == 0) ||
            (LocaleCompare("-help",option+1) == 0))
          break;
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 'i':
      {
        if (LocaleCompare("identify",option+1) == 0)
          break;
        if (LocaleCompare("iconGeometry",option+1) == 0)
          {
            resource_info.icon_geometry=(char *) NULL;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            resource_info.icon_geometry=argv[i];
            break;
          }
        if (LocaleCompare("iconic",option+1) == 0)
          {
            resource_info.iconic=(*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        if (LocaleCompare("interlace",option+1) == 0)
          {
            long
              interlace;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            interlace=ParseMagickOption(MagickInterlaceOptions,MagickFalse,
              argv[i]);
            if (interlace < 0)
              ThrowAnimateException(OptionError,"UnrecognizedInterlaceType",
                argv[i]);
            break;
          }
        if (LocaleCompare("interpolate",option+1) == 0)
          {
            long
              interpolate;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            interpolate=ParseMagickOption(MagickInterpolateOptions,MagickFalse,
              argv[i]);
            if (interpolate < 0)
              ThrowAnimateException(OptionError,"UnrecognizedInterpolateMethod",
                argv[i]);
            break;
          }
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 'l':
      {
        if (LocaleCompare("label",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("limit",option+1) == 0)
          {
            long
              resource;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            resource=ParseMagickOption(MagickResourceOptions,MagickFalse,
              argv[i]);
            if (resource < 0)
              ThrowAnimateException(OptionError,"UnrecognizedResourceType",
                argv[i]);
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if ((LocaleCompare("unlimited",argv[i]) != 0) &&
                (IsGeometry(argv[i]) == MagickFalse))
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if ((i == (long) argc) ||
                (strchr(argv[i],'%') == (char *) NULL))
              ThrowAnimateException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("loop",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 'm':
      {
        if (LocaleCompare("map",option+1) == 0)
          {
            resource_info.map_type=(char *) NULL;
            if (*option == '+')
              break;
            (void) CopyMagickString(argv[i]+1,"sans",MaxTextExtent);
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            resource_info.map_type=argv[i];
            break;
          }
        if (LocaleCompare("matte",option+1) == 0)
          break;
        if (LocaleCompare("mattecolor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            resource_info.matte_color=argv[i];
            break;
          }
        if (LocaleCompare("monitor",option+1) == 0)
          break;
        if (LocaleCompare("monochrome",option+1) == 0)
          {
            if (*option == '+')
              break;
            quantize_info->number_colors=2;
            quantize_info->colorspace=GRAYColorspace;
            break;
          }
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 'n':
      {
        if (LocaleCompare("name",option+1) == 0)
          {
            resource_info.name=(char *) NULL;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            resource_info.name=argv[i];
            break;
          }
        if (LocaleCompare("noop",option+1) == 0)
          break;
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 'p':
      {
        if (LocaleCompare("pause",option+1) == 0)
          {
            resource_info.pause=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            resource_info.pause=(unsigned int) atoi(argv[i]);
            break;
          }
        if (LocaleCompare("page",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("profile",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 'q':
      {
        if (LocaleCompare("quantize",option+1) == 0)
          {
            long
              colorspace;

            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowAnimateException(OptionError,"MissingArgument",option);
            colorspace=ParseMagickOption(MagickColorspaceOptions,
              MagickFalse,argv[i]);
            if (colorspace < 0)
              ThrowAnimateException(OptionError,"UnrecognizedColorspace",
                argv[i]);
            break;
          }
        if (LocaleCompare("quiet",option+1) == 0)
          break;
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 'r':
      {
        if (LocaleCompare("regard-warnings",option+1) == 0)
          break;
        if (LocaleCompare("remote",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (XRemoteCommand(display,resource_info.window_id,argv[i]) != 0)
              return(MagickFalse);
            i--;
            break;
          }
        if (LocaleCompare("repage",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("resample",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("resize",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("rotate",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 's':
      {
        if (LocaleCompare("sampling-factor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("seed",option+1) == 0)
          {
            unsigned long
              seed;

            if (*option == '+')
              {
                seed=(unsigned long) time((time_t *) NULL);
                SeedRandomReservoir(seed);
                break;
              }
            i++;
            if (i == (long) (argc-1))
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            seed=(unsigned long) atol(argv[i]);
            SeedRandomReservoir(seed);
            break;
          }
        if (LocaleCompare("scenes",option+1) == 0)
          {
            first_scene=0;
            last_scene=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsSceneGeometry(argv[i],MagickFalse) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            first_scene=atol(argv[i]);
            last_scene=first_scene;
            (void) sscanf(argv[i],"%ld-%ld",&first_scene,&last_scene);
            break;
          }
        if (LocaleCompare("set",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("shared-memory",option+1) == 0)
          {
            resource_info.use_shared_memory=(*option == '-') ? MagickTrue :
              MagickFalse;
            break;
          }
        if (LocaleCompare("size",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("strip",option+1) == 0)
          break;
        if (LocaleCompare("support",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 't':
      {
        if (LocaleCompare("text-font",option+1) == 0)
          {
            resource_info.text_font=(char *) NULL;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            resource_info.text_font=XGetResourceClass(resource_database,
              GetClientName(),"font",argv[i]);
            break;
          }
        if (LocaleCompare("thumbnail",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("title",option+1) == 0)
          {
            resource_info.title=(char *) NULL;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            resource_info.title=argv[i];
            break;
          }
        if (LocaleCompare("transparent-color",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowAnimateException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("treedepth",option+1) == 0)
          {
            quantize_info->tree_depth=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowAnimateInvalidArgumentException(option,argv[i]);
            quantize_info->tree_depth=(unsigned long) atol(argv[i]);
            break;
          }
        if (LocaleCompare("trim",option+1) == 0)
          break;
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          break;
        if ((LocaleCompare("version",option+1) == 0) ||
            (LocaleCompare("-version",option+1) == 0))
          {
            (void) fprintf(stdout,"Version: %s\n",
              GetMagickVersion((unsigned long *) NULL));
            (void) fprintf(stdout,"Copyright: %s\n\n",GetMagickCopyright());
            break;
          }
        if (LocaleCompare("virtual-pixel",option+1) == 0)
          {
            long
              method;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            method=ParseMagickOption(MagickVirtualPixelOptions,MagickFalse,
              argv[i]);
            if (method < 0)
              ThrowAnimateException(OptionError,
                "UnrecognizedVirtualPixelMethod",argv[i]);
            break;
          }
        if (LocaleCompare("visual",option+1) == 0)
          {
            resource_info.visual_type=(char *) NULL;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            resource_info.visual_type=argv[i];
            break;
          }
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case 'w':
      {
        if (LocaleCompare("window",option+1) == 0)
          {
            resource_info.window_id=(char *) NULL;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowAnimateException(OptionError,"MissingArgument",option);
            resource_info.window_id=argv[i];
            break;
          }
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
      }
      case '?':
        break;
      default:
        ThrowAnimateException(OptionError,"UnrecognizedOption",option);
    }
    fire=(MagickBooleanType) ParseMagickOption(MagickMogrifyOptions,
      MagickFalse,option+1);
    if (fire == MagickTrue)
      MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
  }
  i--;
  if (k != 0)
    ThrowAnimateException(OptionError,"UnbalancedParenthesis",argv[i]);
  if (image_stack[k] == (Image *) NULL)
    ThrowAnimateException(OptionError,"MissingAnImageFilename",argv[argc-1])
  MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
  if (resource_info.window_id != (char *) NULL)
    XAnimateBackgroundImage(display,&resource_info,image_stack[k]);
  else
    {
      Image
        *animate_image;

      /*
        Animate image to X server.
      */
      animate_image=XAnimateImages(display,&resource_info,argv,argc,
        image_stack[k]);
      while (animate_image != (Image *) NULL)
      {
        image_stack[k]=animate_image;
        animate_image=XAnimateImages(display,&resource_info,argv,argc,
          image_stack[k]);
      }
    }
  DestroyAnimate();
  return(status != 0 ? MagickTrue : MagickFalse);
#else
  (void) ThrowMagickException(exception,GetMagickModule(),MissingDelegateError,
    "XWindowLibraryIsNotAvailable","`%s'",image_info->filename);
  AnimateUsage();
  return(MagickFalse);
#endif
}
