/*
 *
 * Test BLOB operations via write/read/write/read sequence to detect
 * any data corruption problems. This does not verify that the image is
 * correct, only that encode/decode process is repeatable.
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

  const MagickInfo
    *magick_info;

  size_t
    blob_length = 0;

  char
    *blob = NULL,
    infile[MaxTextExtent],
    format[MaxTextExtent],
    size[MaxTextExtent];

  int
    arg = 1,
    exit_status = 0,
    rows = 0,
    columns = 0,
    pause = 0;

  double
    fuzz_factor = 0;

  ImageInfo
    *imageInfo;

  ExceptionInfo
    exception;

  if (LocaleNCompare("rwblob",argv[0],7) == 0)
    MagickCoreGenesis((char *) NULL,MagickTrue);
  else
    MagickCoreGenesis(*argv,MagickTrue);

  imageInfo=CloneImageInfo(0);
  GetExceptionInfo( &exception );

  for (arg=1; arg < argc; arg++)
    {
      char
        *option = argv[arg];
    
      if (*option == '-')
        {
          if (LocaleCompare("compress",option+1) == 0)
            {
              arg++;
              option=argv[arg];
              imageInfo->compression=UndefinedCompression;
              if (LocaleCompare("None",option) == 0)
                imageInfo->compression=NoCompression;
              if (LocaleCompare("BZip",option) == 0)
                imageInfo->compression=BZipCompression;
              if (LocaleCompare("Fax",option) == 0)
                imageInfo->compression=FaxCompression;
              if (LocaleCompare("Group4",option) == 0)
                imageInfo->compression=Group4Compression;
              if (LocaleCompare("JPEG",option) == 0)
                imageInfo->compression=JPEGCompression;
              if (LocaleCompare("Lossless",option) == 0)
                imageInfo->compression=LosslessJPEGCompression;
              if (LocaleCompare("LZW",option) == 0)
                imageInfo->compression=LZWCompression;
              if (LocaleCompare("RLE",option) == 0)
                imageInfo->compression=RLECompression;
              if (LocaleCompare("Zip",option) == 0)
                imageInfo->compression=ZipCompression;
            }
          else if (LocaleCompare("debug",option+1) == 0)
              (void) SetLogEventMask(argv[++arg]);
          else if (LocaleCompare("depth",option+1) == 0)
            {
              imageInfo->depth=QuantumDepth;
              arg++;
              if ((arg == argc) || !sscanf(argv[arg],"%ld",&imageInfo->depth))
                {
                  (void) printf("-depth argument missing or not integer\n");
                  (void) fflush(stdout);
                  exit_status = 1;
                  goto program_exit;
                }
              if(imageInfo->depth != 8 && imageInfo->depth != 16 && imageInfo->depth != 32)
                {
                  (void) printf("-depth (%ld) not 8, 16, or 32\n", imageInfo->depth);
                  (void) fflush(stdout);
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
                  (void) fflush(stdout);
                  exit_status = 1;
                  goto program_exit;
                }
              (void) CloneString(&imageInfo->size,argv[arg]);
            }
          else if (LocaleCompare("verbose",option+1) == 0)
            {
              imageInfo->verbose+=1;
            }
        }
      else
        break;
    }
  if (arg != argc-2)
    {
      (void) printf("arg=%d, argc=%d\n", arg, argc);
      (void) printf ( "Usage: %s [-compress algorithm -debug events -depth integer -log format -size geometry -verbose] infile format\n", argv[0] );
      (void) fflush(stdout);
      exit_status = 1;
      goto program_exit;
    }

  (void) strncpy(infile, argv[arg], MaxTextExtent-1 );
  arg++;
  (void) strncpy( format, argv[arg], MaxTextExtent-1 );

  for (arg=0; arg < argc; arg++)
    (void) printf("%s ", argv[arg]);
  (void) printf("\n");
  (void) fflush(stdout);

  /*
   * Read original image
   */
   imageInfo =DestroyImageInfo( imageInfo );
  imageInfo=CloneImageInfo(0);
  GetExceptionInfo( &exception );
  imageInfo->dither = MagickFalse;
  (void) strncpy( imageInfo->filename, infile, MaxTextExtent-1 );
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "Reading image %s", imageInfo->filename);
  original = ReadImage ( imageInfo, &exception );
  if (exception.severity >= ErrorException)
    {
      CatchException(&exception);
      exit_status = 1;
      goto program_exit;
    }
  if ( original == (Image *)NULL )
    {
      (void) printf ( "Failed to read original image %s\n", imageInfo->filename );
      (void) fflush(stdout);
      exit_status = 1;
      goto program_exit;
    }

  /*
   * Obtain original image size if format requires it
   */
  rows    = original->rows;
  columns = original->columns;
  size[0]='\0';
  magick_info=GetMagickInfo(format,&exception);
  if (magick_info && magick_info->raw)
    FormatMagickString( size, MaxTextExtent, "%dx%d", columns, rows );

  /*
   * Save image to BLOB
   */
  blob_length = 8192;
  (void) strncpy(original->magick,format,MaxTextExtent-1);
  (void) CopyMagickString(imageInfo->filename,"",MaxTextExtent);
  original->delay = 10;
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "Writing image to BLOB");
  blob =(char *) ImageToBlob ( imageInfo, original, &blob_length, &exception );
  if (exception.severity >= ErrorException)
    {
      CatchException(&exception);
      exit_status = 1;
      goto program_exit;
    }
  if ( blob == NULL )
    {
      (void) printf ( "Failed to write BLOB in format %s\n", imageInfo->magick );
      (void) fflush(stdout);
      exit_status = 1;
      goto program_exit;
    }
  imageInfo->depth=original->depth;
   original =DestroyImage( original );
  original = (Image*)NULL;

  /*
   * Read image back from BLOB
   */
  (void) strncpy( imageInfo->magick, format, MaxTextExtent-1 );
  (void) CopyMagickString(imageInfo->filename,"",MaxTextExtent);
  if ( size[0] != '\0' )
    CloneString( &imageInfo->size, size );
  original = BlobToImage( imageInfo, blob, blob_length, &exception );
  if (exception.severity >= ErrorException)
    {
      CatchException(&exception);
      exit_status = 1;
      goto program_exit;
    }
  if ( original == (Image *)NULL )
    {
      (void) printf ( "Failed to read image from BLOB in format %s\n",imageInfo->magick );
      (void) fflush(stdout);
      exit_status = 1;
      goto program_exit;
    }
  blob=DestroyString(blob);

  /*
   * Save image to BLOB
   */
  blob_length = 8192;
  (void) strncpy( original->magick, format, MaxTextExtent-1 );
  (void) CopyMagickString(imageInfo->filename,"",MaxTextExtent);
  original->delay = 10;
  blob = (char *) ImageToBlob ( imageInfo, original, &blob_length, &exception );
  if (exception.severity >= ErrorException)
    {
      CatchException(&exception);
      exit_status = 1;
      goto program_exit;
    }
  imageInfo->depth=original->depth;
  if ( blob == NULL )
    {
      (void) printf ( "Failed to write BLOB in format %s\n", imageInfo->magick );
      (void) fflush(stdout);
      exit_status = 1;
      goto program_exit;
    }

  /*
   * Read image back from BLOB
   */
  (void) strncpy( imageInfo->magick, format, MaxTextExtent-1 );
  (void) CopyMagickString(imageInfo->filename,"",MaxTextExtent);
  if ( size[0] != '\0' )
    CloneString( &imageInfo->size, size );
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "Reading image from BLOB");
  final = BlobToImage( imageInfo, blob, blob_length, &exception );
  if (exception.severity >= ErrorException)
    {
      CatchException(&exception);
      exit_status = 1;
      goto program_exit;
    }
  if ( final == (Image *)NULL )
    {
      (void) printf ( "Failed to read image from BLOB in format %s\n",imageInfo->magick );
      (void) fflush(stdout);
      exit_status = 1;
      goto program_exit;
    }
  blob=DestroyString(blob);

  /*
   * Check final output
   */

  fuzz_factor = 0.0;
#if defined(UseHDRI)
  fuzz_factor = 0.06;
#endif
  if ( !strcmp( "JPEG", format ) ||
       !strcmp( "JNG", format ) ||
       !strcmp( "JPG", format ) ||
       !strcmp( "JPG24", format ) ||
       !strcmp( "JP2", format ) ||
       !strcmp( "GRAY", format ) ||
       !strcmp( "CMYK", format ) ||
       !strcmp( "PAL", format ) ||
       !strcmp( "PCD", format ) ||
       !strcmp( "PCDS", format ) ||
       !strcmp( "PGM", format ) ||
       !strcmp( "PPM", format ) ||
       !strcmp( "PNM", format ) ||
       !strcmp( "SGI", format ) ||
       !strcmp( "XPM", format ) ||
       !strcmp( "UYVY", format ) ||
       !strcmp( "YUV", format ) || (final->compression == JPEGCompression))
    fuzz_factor = 0.06;
  
  SetImageColorspace(final,original->colorspace);
  if ( !IsImagesEqual(original, final ) &&
       (original->error.normalized_mean_error > fuzz_factor) )
    {
      (void) printf( "R/W file check for format \"%s\" failed: %u/%.6f/%.6fe\n",
              format,(unsigned int) original->error.mean_error_per_pixel,
              original->error.normalized_mean_error,
              original->error.normalized_maximum_error);
      (void) fflush(stdout);
      exit_status = 1;
      goto program_exit;
    }

 program_exit:
  if (original)
     original =DestroyImage( original );
  original = (Image*)NULL;
  if (final)
     final =DestroyImage( final );
  final = (Image*)NULL;

  imageInfo =DestroyImageInfo( imageInfo );
  DestroyExceptionInfo( &exception );
  MagickCoreTerminus();

  if (pause)
    (void) getc(stdin);
  return exit_status;
}
