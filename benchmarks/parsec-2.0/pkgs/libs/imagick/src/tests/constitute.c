/*
 *
 * Test ExportImagePixels/ImportImagePixels operations via
 * write/read/write/read sequence to detect any data corruption
 * problems.
 *
 * Written by Bob Friesenhahn <bfriesen@simple.dallas.tx.us>
 *
 * The image returned by both reads must be identical in order for the
 * test to pass.
 *
 */

#if !defined(_VISUALC_)
#include <magick/magick-config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#if defined(_VISUALC_)
#include <stdlib.h>
#include <sys\types.h>
#endif
#include <time.h>
#include <magick/MagickCore.h>

int main ( int argc, char **argv )
{
  Image
    *final = (Image *) NULL,
    *original = (Image *) NULL;

  void
    *pixels=0;

  char
    infile[MaxTextExtent],
    map[MaxTextExtent];

  int
    arg = 1,
    exit_status = 0,
    rows = 0,
    columns = 0,
    pause = 0;

  unsigned int
    quantum_size=sizeof(unsigned char);

  size_t
    pixels_size;

  double
    fuzz_factor = 0.0;

  ImageInfo
    *imageInfo;

  ExceptionInfo
    exception;

  MagickBooleanType
    status;

  StorageType
    storage_type=CharPixel;

  if (LocaleNCompare("constitute",argv[0],7) == 0)
    MagickCoreGenesis((char *) NULL,MagickTrue);
  else
    MagickCoreGenesis(*argv,MagickTrue);

  imageInfo=CloneImageInfo(0);
  GetExceptionInfo( &exception );
  fuzz_factor = 0.0;
#if defined(UseHDRI)
  fuzz_factor = 0.06;
#endif

  for (arg=1; arg < argc; arg++)
    {
      char
        *option = argv[arg];
    
      if (*option == '-')
        {
          if (LocaleCompare("debug",option+1) == 0)
            (void) SetLogEventMask(argv[++arg]);
          else if (LocaleCompare("depth",option+1) == 0)
            {
              imageInfo->depth=QuantumDepth;
              arg++;
              if ((arg == argc) || !sscanf(argv[arg],"%ld",&imageInfo->depth))
                {
                  (void) printf("-depth argument missing or not integer\n");
                  exit_status = 1;
                  goto program_exit;
                }
              if(imageInfo->depth != 8 && imageInfo->depth != 16 && imageInfo->depth != 32)
                {
                  (void) printf("-depth (%ld) not 8, 16, or 32\n", imageInfo->depth);
                  exit_status = 1;
                  goto program_exit;
                }
            }
          else if (LocaleCompare("log",option+1) == 0)
            (void) SetLogFormat(argv[++arg]);
          else if (LocaleCompare("pause",option+1) == 0)
            pause=1;
          else if (LocaleCompare("size",option+1) == 0)
            {
              arg++;
              if ((arg == argc) || !IsGeometry(argv[arg]))
                {
                  (void) printf("-size argument missing or not geometry\n");
                  exit_status = 1;
                  goto program_exit;
                }
              (void) CloneString(&imageInfo->size,argv[arg]);
            }
          else if (LocaleCompare("storagetype",option+1) == 0)
            {
              arg++;
              if ((arg == argc))
                {
                  (void) printf("-storagetype argument missing\n");
                  exit_status = 1;
                  goto program_exit;
                }
              if (LocaleCompare("Char",argv[arg]) == 0)
                {
                  storage_type=CharPixel;
                  quantum_size=sizeof(unsigned char);
                }
              else if (LocaleCompare("Short",argv[arg]) == 0)
                {
                  storage_type=ShortPixel;
                  quantum_size=sizeof(unsigned short);
                }
              else if (LocaleCompare("Integer",argv[arg]) == 0)
                {
                  storage_type=IntegerPixel;
                  quantum_size=sizeof(unsigned int);
                }
              else if (LocaleCompare("Long",argv[arg]) == 0)
                {
                  storage_type=LongPixel;
                  quantum_size=sizeof(unsigned long);
                }
              else if (LocaleCompare("Float",argv[arg]) == 0)
                {
                  storage_type=FloatPixel;
                  quantum_size=sizeof(float);
                }
              else if (LocaleCompare("Double",argv[arg]) == 0)
                {
                  storage_type=DoublePixel;
                  quantum_size=sizeof(double);
                }
              else
                {
                  (void) printf("Unrecognized storagetype argument %s\n",argv[arg]);
                  exit_status = 1;
                  goto program_exit;
                }
            }
        }
      else
        break;
    }
  if (arg != argc-2)
    {
      (void) printf ( "Usage: %s -debug events -depth integer -log format -size geometry -storagetype type] infile map\n", argv[0] );
      exit_status = 1;
      goto program_exit;
    }

  (void) strncpy(infile, argv[arg], MaxTextExtent-1 );
  arg++;
  (void) strncpy( map, argv[arg], MaxTextExtent-1 );

  for (arg=0; arg < argc; arg++)
    (void) printf("%s ", argv[arg]);
  (void) printf("\n");
  (void) fflush(stdout);

  /*
   * Read original image
   */
  GetExceptionInfo( &exception );
  imageInfo->dither = MagickFalse;
  (void) strncpy( imageInfo->filename, infile, MaxTextExtent-1 );
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                        "Reading image %s", imageInfo->filename);
  original = ReadImage ( imageInfo, &exception );
  if (exception.severity >= ErrorException)
    CatchException(&exception);
  if ( original == (Image *)NULL )
    {
      (void) printf ( "Failed to read original image %s\n", imageInfo->filename );
      exit_status = 1;
      goto program_exit;
    }

  /*  If a CMYK map is specified, make sure that input image is CMYK */
  if (strchr(map,'c') || strchr(map,'C') || strchr(map,'m') || strchr(map,'M') ||
      strchr(map,'y') || strchr(map,'y') || strchr(map,'k') || strchr(map,'k'))
    SetImageColorspace(original,CMYKColorspace);

  /*
   * Obtain original image size if format requires it
   */
  rows    = original->rows;
  columns = original->columns;

  /*
   * Save image to array
   */
  pixels_size=quantum_size*strlen(map)*rows*columns;
  pixels=AcquireQuantumMemory(pixels_size,sizeof(unsigned char));
  if( !pixels )
    {
      (void) printf ( "Failed to allocate memory for pixels\n");
      exit_status = 1;
      goto program_exit;
    }
  memset((void *) pixels, 0, pixels_size);

  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                        "Writing image to pixel array");
  if( !ExportImagePixels(original,0,0,original->columns,original->rows,map,
                     storage_type,pixels,&exception) )
    {
      (void) printf ( "ExportImagePixels returned error status\n");
      if (exception.severity >= ErrorException)
        CatchException(&exception);
      exit_status = 1;
      goto program_exit;
    }

   original =DestroyImage( original );
  original = (Image*)NULL;

  /*
   * Read image back from pixel array
   */
  original = AllocateImage((ImageInfo *) NULL);
  SetImageExtent(original,columns,rows);
  if (strchr(map,'c') || strchr(map,'C') || strchr(map,'m') || strchr(map,'M') ||
      strchr(map,'y') || strchr(map,'y') || strchr(map,'k') || strchr(map,'k'))
    original->colorspace=CMYKColorspace;
  (void) SetImageBackgroundColor(original);

  status = ImportImagePixels(original,0,0,columns,rows,map,storage_type,pixels);
  if ( status == MagickFalse)
    {
      (void) printf ( "Failed to read image from pixels array\n" );
      if (exception.severity >= ErrorException)
        CatchException(&exception);
      exit_status = 1;
      goto program_exit;
    }

  /*
   * Save image to pixel array
   */
  memset((void *) pixels, 0, pixels_size);
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                        "Writing image to pixel array");
  if( !ExportImagePixels(original,0,0,original->columns,original->rows,map,
                     storage_type,pixels,&exception) )
    {
      (void) printf ( "ExportImagePixels returned error status\n" );
      if (exception.severity >= ErrorException)
        CatchException(&exception);
      exit_status = 1;
      goto program_exit;
    }

  /*
   * Read image back from pixel array
   */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                        "Reading image from pixel array");
  final = AllocateImage((ImageInfo *) NULL);
  SetImageExtent(final,columns,rows);
  (void) SetImageBackgroundColor(final);
  status = ImportImagePixels(final,0,0,columns,rows,map,storage_type,pixels);
  if ( status == MagickFalse)
    {
      (void) printf ( "Failed to read image from pixels array\n" );
      if (exception.severity >= ErrorException)
        CatchException(&exception);
      exit_status = 1;
      goto program_exit;
    }

  pixels=(void *) RelinquishMagickMemory(pixels);

  /*
   * Check final output
   */

  if (original->colorspace != final->colorspace)
    {
      (void) printf("Original colorspace (%s) != final colorspace (%s)\n",
       MagickOptionToMnemonic(MagickColorspaceOptions,original->colorspace),
       MagickOptionToMnemonic(MagickColorspaceOptions,final->colorspace));
      exit_status = 1;
      goto program_exit;
    }
  
  if ( !IsImagesEqual(original, final) &&
      (original->error.normalized_mean_error > fuzz_factor) )
    {
      CatchException(&original->exception);
      CatchException(&final->exception);
      (void) printf( "Constitute check failed: %u/%g/%g\n",
             (unsigned int) original->error.mean_error_per_pixel,
             original->error.normalized_mean_error,
             original->error.normalized_maximum_error);
      exit_status = 1;
      goto program_exit;
    }

  /* DisplayImages( imageInfo, final ); */

 program_exit:
  (void) fflush(stdout);
  if (original)
     original =DestroyImage( original );
  original = 0;
  if (final)
    {
      if (getenv("SHOW_RESULT") != 0)
			  DisplayImages( imageInfo, final );
       final =DestroyImage( final );
    }
  final = 0;
  if (pixels)
    pixels=(void *) RelinquishMagickMemory(pixels);
  if (imageInfo)
    imageInfo=DestroyImageInfo(imageInfo);
  imageInfo = 0;

  MagickCoreTerminus();

  if (pause)
    (void) getc(stdin);
  return exit_status;
}
