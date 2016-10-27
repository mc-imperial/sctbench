/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               DDDD   IIIII  SSSSS  TTTTT   OOO   RRRR   TTTTT               %
%               D   D    I    SS       T    O   O  R   R    T                 %
%               D   D    I     SSS     T    O   O  RRRR     T                 %
%               D   D    I       SS    T    O   O  R R      T                 %
%               DDDD   IIIII  SSSSS    T     OOO   R  R     T                 %
%                                                                             %
%                                                                             %
%                     ImageMagick Image Distortion Methods.                   %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                              Anthony Thyssen                                %
%                                 June 2007                                   %
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
#include "magick/artifact.h"
#include "magick/cache-view.h"
#include "magick/colorspace-private.h"
#include "magick/composite-private.h"
#include "magick/distort.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/gem.h"
#include "magick/hashmap.h"
#include "magick/image.h"
#include "magick/list.h"
#include "magick/matrix.h"
#include "magick/memory_.h"
#include "magick/pixel.h"
#include "magick/pixel-private.h"
#include "magick/resample.h"
#include "magick/registry.h"
#include "magick/semaphore.h"
#include "magick/splay-tree.h"
#include "magick/string_.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D i s t o r t I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DistortImage() distorts an image using various distortion methods, by
%  mapping color lookups of the source image to a new destination image
%  usally of the same size as the source image, unless 'bestfit' is set to
%  true.
%
%  If 'bestfit' is enabled, and distortion allows it, the destination image is
%  adjusted to ensure the whole source 'image' will just fit within the final
%  destination image, which will be sized and offset accordingly.  Also in
%  many cases the virtual offset of the source image will be taken into
%  account in the mapping.
%
%  ArcDistortion will always ignore source image offset, and always 'bestfit'
%  the destination image with the top left corner offset relative to the polar
%  mapping center.
%
%  Bilinear has no simple inverse mapping so will not allow 'bestfit' style
%  of image distortion.
%
%  The format of the DistortImage() method is:
%
%      Image *DistortImage(const Image *image,const DistortImageMethod method,
%        const unsigned long number_arguments,const double *arguments,
%        const MagickBooleanType bestfit, ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image to be distorted.
%
%    o method: The method of image distortion.
%
%    o number_arguments: The number of arguments given.
%
%    o arguments: The arguments for this distortion method.
%
%    o bestfit: Attempt to resize destination to fit distorted source.
%
%    o exception: Return any errors or warnings in this structure
%
*/
static MagickBooleanType SolveAffineDistortion(
  const unsigned long number_points,const PointInfo *points,double **matrix,
  double *vector)
{
  /*
    Given the transformations of the coordinates for two triangles
      u0,v0, x0,y0,  u1,v1, x1,y1,  u2,v2, x2,y2, ...

    Solve for the 6 cofficients c0..c6 for a affine distortion:
      u = c0*x + c2*y + c4
      v = c1*x + c3*y + c5
  */
  register int
    i;
  double
    x,y,u,v;
  if (number_points < 6)
    return(MagickFalse);
  for( i = 0; i < 6; ) {
    u = points[i].x;
    v = points[i].y;
    x = points[i+1].x;
    y = points[i+1].y;

    vector[i]=u;
    matrix[i][0]=x;
    matrix[i][2]=y;
    matrix[i][4]=1.0;
    i++;

    vector[i]=v;
    matrix[i][1]=x;
    matrix[i][3]=y;
    matrix[i][5]=1.0;
    i++;
  }
  return(GaussJordanElimination(matrix,6UL,&vector,1UL));
}

static void InvertAffineCoefficients(
  const double *coefficients,double *inverse)
{
  double determinant;

  determinant=1.0/(coefficients[0]*coefficients[3]-coefficients[1]*coefficients[2]);
  inverse[0]=determinant*coefficients[3];
  inverse[1]=determinant*(-coefficients[1]);
  inverse[2]=determinant*(-coefficients[2]);
  inverse[3]=determinant*coefficients[0];
  inverse[4]=(-coefficients[4])*inverse[0]-coefficients[5]*inverse[2];
  inverse[5]=(-coefficients[4])*inverse[1]-coefficients[5]*inverse[3];
}

static MagickBooleanType SolveBilinearDistortion(
  const unsigned long number_points,const PointInfo *points,double **matrix,
  double *vector)
{
  /*
    Given the transformations of the coordinates of two quadrilaterals:
      u0,v0, x0,y0,  u1,v1, x1,y1,  u2,v2, x2,y2, ...

    Solve for the 8 coeffecients c0..c7 for a bilinear distortion:
      u = c0*x + c1*y + c2*x*y + c3
      v = c4*x + c5*y + c6*x*y + c7
  */
  register int
    i;
  double
    x,y,u,v;
  if (number_points < 8)
    return(MagickFalse);
  for( i = 0; i < 8; ) {
    u = points[i].x;
    v = points[i].y;
    x = points[i+1].x;
    y = points[i+1].y;

    vector[i]=u;
    matrix[i][0]=x;
    matrix[i][1]=y;
    matrix[i][2]=x*y;
    matrix[i][3]=1.0;
    i++;

    vector[i]=v;
    matrix[i][4]=x;
    matrix[i][5]=y;
    matrix[i][6]=x*y;
    matrix[i][7]=1.0;
    i++;
  }
  return(GaussJordanElimination(matrix,8UL,&vector,1UL));
}

static MagickBooleanType SolvePerspectiveDistortion(
  const unsigned long number_points,const PointInfo *points,double **matrix,
  double *vector)
{
  /*
    Given the coordinates of two quadrilaterals:
      u0,v0, x0,y0,  u1,v1, x1,y1,  u2,v2, x2,y2, ...

    Solve for the 8 coefficients c0..c7 for a perspective distortion:
       u = ( c0*x + c1*y + c2 ) / ( c6*x + c7*y + 1 )
       v = ( c3*x + c4*y + c5 ) / ( c6*x + c7*y + 1 )
   */
  register int
    i;
  double
    x,y,u,v;
  if (number_points < 8)
    return(MagickFalse);
  for( i = 0; i < 8; ) {
    u = points[i].x;
    v = points[i].y;
    x = points[i+1].x;
    y = points[i+1].y;

    vector[i]=u;
    matrix[i][0]=x;
    matrix[i][1]=y;
    matrix[i][2]=1.0;
    matrix[i][6]=-x*u;
    matrix[i][7]=-y*u;
    i++;

    vector[i]=v;
    matrix[i][3]=x;
    matrix[i][4]=y;
    matrix[i][5]=1.0;
    matrix[i][6]=-x*v;
    matrix[i][7]=-y*v;
    i++;
  }
  return(GaussJordanElimination(matrix,8UL,&vector,1UL));
}

static void InvertPerspectiveCoefficients(
  const double *coefficients,double *inverse)
{
  double determinant;
  /* From "Digital Image Warping" by George Wolberg, page 53 */

  determinant=1.0/(coefficients[0]*coefficients[4]-coefficients[3]*coefficients[1]);
  inverse[0]=determinant*(coefficients[4]-coefficients[7]*coefficients[5]);
  inverse[1]=determinant*(coefficients[7]*coefficients[2]-coefficients[1]);
  inverse[2]=determinant*(coefficients[1]*coefficients[5]-coefficients[4]*coefficients[2]);
  inverse[3]=determinant*(coefficients[6]*coefficients[5]-coefficients[3]);
  inverse[4]=determinant*(coefficients[0]-coefficients[6]*coefficients[2]);
  inverse[5]=determinant*(coefficients[3]*coefficients[2]-coefficients[0]*coefficients[5]);
  inverse[6]=determinant*(coefficients[3]*coefficients[7]-coefficients[6]*coefficients[4]);
  inverse[7]=determinant*(coefficients[6]*coefficients[1]-coefficients[0]*coefficients[7]);
}

static inline double MagickRound(double x)
{
  assert(x >= (1.0*LONG_MIN-0.5));
  assert(x <= (1.0*LONG_MAX+0.5));
  if (x >= 0.0)
    return((double) ((long) (x+0.5)));
  return((double) ((long) (x-0.5)));
}

MagickExport Image *DistortImage(Image *image,const DistortImageMethod method,
  const unsigned long number_arguments,const double *arguments,
  MagickBooleanType bestfit, ExceptionInfo *exception)
{
#define DistortImageTag  "Distort/Image"

  double
    coefficients[9],
    validity;

  Image
    *distort_image;

  register long
    i,
    x;

  long
    j,
    y;

  PointInfo
    point;          /* point to sample (center of filtered resample of area) */

  MagickPixelPacket
    pixel,          /* pixel to assign to distorted image */
    invalid;  /* the color to assign when distort result is invalid */

  register IndexPacket
    *indexes;

  register PixelPacket
    *q;

  ResampleFilter
    *resample_filter;

  ViewInfo
    *distort_view;

  MagickBooleanType
    status;

  RectangleInfo
    geometry;

  const char
     *property;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  (void) ResetMagickMemory(coefficients,0,sizeof(coefficients));

  /*
    Convert input arguments into mapping coefficents for distortion
  */
  switch (method)
  {
    case AffineDistortion:
    {
      double
        **matrix;

      PointInfo
        *points;

      /* Affine Distortion

            u =   c0*x + c2*y + c4

            v =   c1*x + c3*y + c5

         Input Arguments are pairs of distorted coodinates (minimum 3 sets)
         Which will be used to generate the coefficients of the above.
            u1,v1, x1,y1,  u2,v2, x2,y2,  u3,v3, x3,y3, ...
      */
      if (number_arguments != 12) {  /* to be removed */
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'","distort Affine",
                     "Needs 12 numbers");
        return((Image *) NULL);
      }
      if (number_arguments%4 != 0) {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'","distort Affine",
                     "Requires pairs of coords (4 numbers U,V,X,Y)");
        return((Image *) NULL);
      }
      if (number_arguments < 12) {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'","distort Affine",
                     "Minimum of 3 pairs of coords needed (12 numbers)");
        return((Image *) NULL);
      }
      points=(PointInfo *) AcquireQuantumMemory((number_arguments)/2UL,
        sizeof(*points));
      if (points == (PointInfo *) NULL)
        ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
      for (i=0; i < (long) number_arguments; i++)
        if ((i % 2 ) == 0)
          points[i/2].x=arguments[i];
        else
          points[i/2].y=arguments[i];
      matrix = AcquireMagickMatrix(6UL,6UL);
      if (matrix == (double **) NULL)
      {
        points=(PointInfo *) RelinquishMagickMemory(points);
        ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
      }
      status=SolveAffineDistortion(6UL,points,matrix,coefficients);
      matrix = RelinquishMagickMatrix(matrix, 6UL);
      points = (PointInfo *) RelinquishMagickMemory(points);
      if ( status == MagickFalse ) {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'","distort Affine",
                     "Degenerate Result");
        return((Image *) NULL);
      }
      break;
    }
    case AffineProjectionDistortion:
    {
      /*
        Arguments: Affine Matrix (forward mapping)
           sx, rx, ry, sy, tx, ty
      */
      if (number_arguments != 6) {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'","distort AffineProjection",
                     "Needs 6 numbers");
        return((Image *) NULL);
      }
      InvertAffineCoefficients(arguments, coefficients);
      break;
    }
    case BilinearDistortion:
    {
      double
        **matrix;

      PointInfo
        *points;

      /* Bilinear Distortion (reversed)

            u = c0*x + c1*y + c2*x*y + c3;

            v = c4*x + c5*y + c6*x*y + c7;

         Input Arguments are pairs of distorted coodinates (minimum 4 pairs)
         Which will be used to generate the coefficients of the above.
            u1,v1, x1,y1,  u2,v2, x2,y2,  u3,v3, x3,y3,  u4,v4, x4,y4 ...
      */
      if (number_arguments != 16) {  /* to be removed */
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'","distort Bilinear",
                     "Needs 16 numbers");
        return((Image *) NULL);
      }
      if (number_arguments%4 != 0) {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'","distort Bilinear",
                     "Requires pairs of coords (4 numbers U,V,X,Y)");
        return((Image *) NULL);
      }
      if (number_arguments < 16) {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'","distort Bilinear",
                     "Minimum of 4 pairs of coords needed (16 numbers)");
        return((Image *) NULL);
      }
      points=(PointInfo *) AcquireQuantumMemory((number_arguments+1UL)/2UL,
        sizeof(*points));
      if (points == (PointInfo *) NULL)
        ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
      for (i=0; i < (long) number_arguments; i++)
        if ((i % 2 ) == 0)
          points[i/2].x=arguments[i];
        else
          points[i/2].y=arguments[i];
      matrix = AcquireMagickMatrix(8UL,8UL);
      if (matrix == (double **) NULL)
      {
        points=(PointInfo *) RelinquishMagickMemory(points);
        ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
      }
      status=SolveBilinearDistortion(8UL,points,matrix,coefficients);
      matrix = RelinquishMagickMatrix(matrix, 8UL);
      points = (PointInfo *) RelinquishMagickMemory(points);
      if ( status == MagickFalse ) {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'","distort Bilinear",
                     "Degenerate Result");
        return((Image *) NULL);
      }
      break;
    }
    case PerspectiveDistortion:
    {
      double
        **matrix;

      PointInfo
        *points;

      /* Perspective Distortion (a ratio of affine distortions)

                 p     c0*x + c1*y + c2
            u = --- = ------------------
                 r     c6*x + c7*y + 1

                 q     c3*x + c4*y + c5
            v = --- = ------------------
                 r     c6*x + c7*y + 1

            c8 = Sign of 'r', or the denominator affine, for the actual image.
                 This determines what part of the distorted image is 'ground'
                 side of the horizon, the other part is 'sky' or invalid.

         Input Arguments are pairs of distorted coodinates (minimum 4 pairs)
         Which will be used to generate the coefficients of the above.
            u1,v1, x1,y1,  u2,v2, x2,y2,  u3,v3, x3,y3,  u4,v4, x4,y4 ...
      */
      if (number_arguments != 16) {  /* to be removed */
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'","distort Bilinear",
                     "Needs 16 numbers");
        return((Image *) NULL);
      }
      if (number_arguments%4 != 0) {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'","distort Bilinear",
                     "Requires pairs of coords (4 numbers U,V,X,Y)");
        return((Image *) NULL);
      }
      if (number_arguments < 16) {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'","distort Bilinear",
                     "Minimum of 4 pairs of coords needed (16 numbers)");
        return((Image *) NULL);
      }
      points=(PointInfo *) AcquireQuantumMemory((number_arguments+1UL)/2UL,
        sizeof(*points));
      if (points == (PointInfo *) NULL)
        ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
      for (i=0; i < (long) number_arguments; i++)
        if ((i % 2 ) == 0)
          points[i/2].x=arguments[i];
        else
          points[i/2].y=arguments[i];
      matrix = AcquireMagickMatrix(8UL,8UL);
      if (matrix == (double **) NULL)
      {
        points=(PointInfo *) RelinquishMagickMemory(points);
        ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
      }
      status=SolvePerspectiveDistortion(8UL,points,matrix,coefficients);
      matrix = RelinquishMagickMatrix(matrix, 8UL);
      points = (PointInfo *) RelinquishMagickMemory(points);
      if ( status == MagickFalse ) {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'","distort Prespective",
                     "Degenerate Result");
        return((Image *) NULL);
      }
      /*
        What is the 'ground' of the perspective distortion.
        Just find the sign of denomitor affine for any image coordinate.
        This is a 9'th coefficient!
      */
      coefficients[8] = coefficients[6]*arguments[number_arguments-2]
                  + coefficients[7]*arguments[number_arguments-1] + 1.0;
      coefficients[8] = (coefficients[8] < 0.0) ? -1.0 : +1.0;
      break;
    }
    case PerspectiveProjectionDistortion:
    {
      /*
        Arguments: Perspective Coefficents (forward mapping)
      */
      if (number_arguments != 8) {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'",
                     "distort PerspectiveProjection", "Needs 8 numbers");
        return((Image *) NULL);
      }
      InvertPerspectiveCoefficients(arguments, coefficients);
      /*
        What is the 'ground' of the perspective distortion?  For a forward
        mapped perspective the images 0,0 coord will map to c2,c5  in the
        distorted image, so get the sign of denominator of that.
        This is a 9'th coefficient!
      */
      coefficients[8] = coefficients[6]*arguments[2]
                  + coefficients[7]*arguments[5] + 1.0;
      coefficients[8] = (coefficients[8] < 0.0) ? -1.0 : +1.0;
      break;
    }
    case ScaleRotateTranslateDistortion:
    {
      double
        cosine, sine,
        x,y,sx,sy,a,nx,ny;

      /*
         Argument options, by number of arguments given:
           7: x,y, sx,sy, a, nx,ny
           6: x,y,   s,   a, nx,ny
           5: x,y, sx,sy, a
           4: x,y,   s,   a
           3: x,y,        a
           2:        s,   a
           1:             a
         Where actions are (in order of application)
            x,y     'center' of transforms     (default = image center)
            sx,sy   scale image by this amount (default = 1)
            a       angle of rotation          (argument required)
            nx,ny   move 'center' here         (default = no movement)
         And convert to affine mapping coefficients
      */
      x = nx = (double)image->columns/2.0;
      y = ny = (double)image->rows/2.0;
      if ( bestfit ) {
        x = nx += (double)image->page.x;
        y = ny += (double)image->page.y;
      }
      sx = sy = 1.0;
      switch ( number_arguments ) {
      case 0:
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'",
                     "distort ScaleTranslateRotate",
                     "Needs at least 1 argument");
        return((Image *) NULL);
      case 1:
        a = arguments[0];
        break;
      case 2:
        sx = sy = arguments[0];
        a = arguments[1];
        break;
      default:
        x = nx = arguments[0];
        y = ny = arguments[1];
        switch ( number_arguments ) {
        case 3:
          a = arguments[2];
          break;
        case 4:
          sx = sy = arguments[2];
          a = arguments[3];
          break;
        case 5:
          sx = arguments[2];
          sy = arguments[3];
          a = arguments[4];
          break;
        case 6:
          sx = sy = arguments[2];
          a = arguments[3];
          nx = arguments[4];
          ny = arguments[5];
          break;
        case 7:
          sx = arguments[2];
          sy = arguments[3];
          a = arguments[4];
          nx = arguments[5];
          ny = arguments[6];
          break;
        default:
          (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'",
                     "distort ScaleTranslateRotate",
                     "Too Many Arguments (7 or less)");
          return((Image *) NULL);
        }
        break;
      }
      a=DegreesToRadians(a);
      cosine=cos(a);
      sine=sin(a);
      coefficients[0]=cosine/sx;
      coefficients[1]=(-sine)/sy;
      coefficients[2]=sine/sx;
      coefficients[3]=cosine/sy;
      coefficients[4]=x-nx*coefficients[0]-ny*coefficients[2];
      coefficients[5]=y-nx*coefficients[1]-ny*coefficients[3];
      break;
    }
    case ArcDistortion:
    {
      /* Arc Distortion
         Args: arc_width  rotate  top_edge_radius  bottom_edge_radius
         All but first argument are optional
            arc_width      The angle over which to arc the image side-to-side
            rotate         Angle to rotate image from vertical center
            top_radius     Set top edge of source image at this radius
            bottom_radius  Set bootom edge to this radius (radial scaling)

         By default, if the radii arguments are nor provided the image radius
         is calculated so the horizontal center-line is fits the given arc
         without scaling.

         The output image size is ALWAYS adjusted to contain the whole image,
         and an offset is given to position image relative to the 0,0 point of
         the origin, allowing users to use relative positioning onto larger
         background (via -flatten).

         The arguments are converted to these coefficents
            c0: angle for center of source image
            c1: angle scale for mapping to source image
            c2: radius for top of source image
            c3: radius scale for mapping source image
            c4: centerline of arc within source image

         Note the coefficents use a center angle, so asymptotic join is
         furthest from both sides of the source image. This also means that
         for arc angles greater than 360 the sides of the image will be
         trimmed equally.
      */
      if ( number_arguments >= 1 && arguments[0] < MagickEpsilon ) {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'", "distort Arc",
                     "Arc Angle Too Small");
        return((Image *) NULL);
      }
      if ( number_arguments >= 3 && arguments[2] < MagickEpsilon ) {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                     "InvalidArgument","%s : '%s'", "distort Arc",
                     "Outer Radius Too Small");
        return((Image *) NULL);
      }
      coefficients[0] = -MagickPI/2.0;
      if ( number_arguments >= 1 )
        coefficients[1] = DegreesToRadians(arguments[0]);
      else
        coefficients[1] = MagickPI/2.0;
      if ( number_arguments >= 2 )
        coefficients[0] += DegreesToRadians(arguments[1]);
      coefficients[0] -= MagickRound(coefficients[0]/(2.0*MagickPI))
                             *2.0*MagickPI;
      coefficients[3] = 1.0*image->rows-1;
      coefficients[2] = 1.0*image->columns/coefficients[1] + coefficients[3]/2.0;
      if ( number_arguments >= 3 ) {
        if ( number_arguments >= 4 )
          coefficients[3] = arguments[2] - arguments[3];
        else
          coefficients[3] *= arguments[2]/coefficients[2];
        coefficients[2] = arguments[2];
      }
      coefficients[4] = (1.0*image->columns-1.0)/2.0;
      /* Always work out the 'best fit' for Arc Distort (required) */
      bestfit = MagickTrue;
      break;
    }
    default:
      break;
  }

  /*
    Determine the size and offset for a 'bestfit' destination.
    Usally the four corners of the source image is enough.
  */

  /* default output image bounds, when no 'bestfit' is requested */
  geometry.width=image->columns;
  geometry.height=image->rows;
  geometry.x=0;
  geometry.y=0;
  point.x=0.0;
  point.y=0.0;

  if ( bestfit ) {
    double
      min_x,max_x,min_y,max_y;

/* defines to figure out the bounds of the distorted image */
#define InitalBounds(px,py) \
{ \
  min_x = max_x = (px); \
  min_y = max_y = (py); \
}
#define ExpandBounds(px,py) \
{ \
  if ( (px) < min_x )  min_x = (px); \
  if ( (px) > max_x )  max_x = (px); \
  if ( (py) < min_y )  min_y = (py); \
  if ( (py) > max_y )  max_y = (py); \
}

    switch (method)
    {
      case AffineDistortion:
      case AffineProjectionDistortion:
      case ScaleRotateTranslateDistortion:
      { double inverse[6];
        InvertAffineCoefficients(coefficients, inverse);
        x = image->page.x;  y = image->page.y;
        point.x=inverse[0]*x+inverse[2]*y+inverse[4];
        point.y=inverse[1]*x+inverse[3]*y+inverse[5];
        InitalBounds( point.x, point.y );
        x = image->page.x+image->columns-1;  y = image->page.y;
        point.x=inverse[0]*x+inverse[2]*y+inverse[4];
        point.y=inverse[1]*x+inverse[3]*y+inverse[5];
        ExpandBounds( point.x, point.y );
        x = image->page.x;  y = image->page.y+image->rows-1;
        point.x=inverse[0]*x+inverse[2]*y+inverse[4];
        point.y=inverse[1]*x+inverse[3]*y+inverse[5];
        ExpandBounds( point.x, point.y );
        x = image->page.x+image->columns-1;
        y = image->page.y+image->rows-1;
        point.x=inverse[0]*x+inverse[2]*y+inverse[4];
        point.y=inverse[1]*x+inverse[3]*y+inverse[5];
        ExpandBounds( point.x, point.y );
        break;
      }
      case PerspectiveDistortion:
      case PerspectiveProjectionDistortion:
      { double inverse[8], scale;
        InvertPerspectiveCoefficients(coefficients, inverse);
        x = image->page.x;  y = image->page.y;
        scale=inverse[6]*x+inverse[7]*y+1.0;
        scale=1.0/(  (fabs(scale) <= MagickEpsilon) ? 1.0 : scale );
        point.x=scale*(inverse[0]*x+inverse[1]*y+inverse[2]);
        point.y=scale*(inverse[3]*x+inverse[4]*y+inverse[5]);
        InitalBounds( point.x, point.y );
        x = image->page.x+image->columns-1;  y = image->page.y;
        scale=inverse[6]*x+inverse[7]*y+1.0;
        scale=1.0/(  (fabs(scale) <= MagickEpsilon) ? 1.0 : scale );
        point.x=scale*(inverse[0]*x+inverse[1]*y+inverse[2]);
        point.y=scale*(inverse[3]*x+inverse[4]*y+inverse[5]);
        ExpandBounds( point.x, point.y );
        x = image->page.x;  y = image->page.y+image->rows-1;
        scale=inverse[6]*x+inverse[7]*y+1.0;
        scale=1.0/(  (fabs(scale) <= MagickEpsilon) ? 1.0 : scale );
        point.x=scale*(inverse[0]*x+inverse[1]*y+inverse[2]);
        point.y=scale*(inverse[3]*x+inverse[4]*y+inverse[5]);
        ExpandBounds( point.x, point.y );
        x = image->page.x+image->columns-1;
        y = image->page.y+image->rows-1;
        scale=inverse[6]*x+inverse[7]*y+1.0;
        scale=1.0/(  (fabs(scale) <= MagickEpsilon) ? 1.0 : scale );
        point.x=scale*(inverse[0]*x+inverse[1]*y+inverse[2]);
        point.y=scale*(inverse[3]*x+inverse[4]*y+inverse[5]);
        ExpandBounds( point.x, point.y );
        break;
      }
      case ArcDistortion:
      { double a, ca, sa;
        /* Forward Map Corners */
        a = coefficients[0]-coefficients[1]/2; ca = cos(a); sa = sin(a);
        point.x = coefficients[2]*ca;
        point.y = coefficients[2]*sa;
        InitalBounds( point.x, point.y );
        point.x = (coefficients[2]-coefficients[3])*ca;
        point.y = (coefficients[2]-coefficients[3])*sa;
        ExpandBounds( point.x, point.y );
        a = coefficients[0]+coefficients[1]/2; ca = cos(a); sa = sin(a);
        point.x = coefficients[2]*ca;
        point.y = coefficients[2]*sa;
        ExpandBounds( point.x, point.y );
        point.x = (coefficients[2]-coefficients[3])*ca;
        point.y = (coefficients[2]-coefficients[3])*sa;
        ExpandBounds( point.x, point.y );
        /* orthogonal points along top of arc */
        for( a=ceil((coefficients[0]-coefficients[1]/2.0)*2.0/MagickPI)
                              *MagickPI/2.0;
               a<(coefficients[0]+coefficients[1]/2.0); a+=MagickPI/2.0 ) {
          ca = cos(a); sa = sin(a);
          point.x = coefficients[2]*ca;
          point.y = coefficients[2]*sa;
          ExpandBounds( point.x, point.y );
        }
        /*
          Convert the angle_to_width and radius_to_height
          to appropriate scaling factors, to allow faster processing
          in the mapping function.
        */
        coefficients[1] = 2.0*MagickPI*image->columns/coefficients[1];
        coefficients[3] = 1.0*image->rows/coefficients[3];
        break;
      }
      case BilinearDistortion:
      default:
        /* no bestfit available for this distortion YET */
        bestfit = MagickFalse;
    }
    /* Set the output image geometry to the 'bestfit' of the image */
    if ( bestfit ) {
      geometry.x=(long) floor(min_x-MagickEpsilon);
      geometry.y=(long) floor(min_y-MagickEpsilon);
      geometry.width=(unsigned long) ceil(max_x-geometry.x+1+MagickEpsilon);
      geometry.height=(unsigned long) ceil(max_y-geometry.y+1+MagickEpsilon);
    }
  }

  /* User provided override of the output geometry */
  property=GetImageArtifact(image,"distort:viewport");
  if (property != (const char *) NULL)
    (void) ParseAbsoluteGeometry(property, &geometry);

  /*
    Initialize the distort image attributes.
  */
  distort_image=CloneImage(image,geometry.width,geometry.height,MagickTrue,
    exception);
  if (distort_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(distort_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&distort_image->exception);
      distort_image=DestroyImage(distort_image);
      return((Image *) NULL);
    }
  distort_image->page.x=geometry.x;
  distort_image->page.y=geometry.y;
  if (distort_image->background_color.opacity != OpaqueOpacity)
    distort_image->matte=MagickTrue;

  /* Open Image views as needed. */
  resample_filter=AcquireResampleFilter(image,exception);
  GetMagickPixelPacket(distort_image,&pixel);
  distort_view=OpenCacheView(distort_image);

  /* Define constant scaling vectors for Affine Distortions */
  switch (method)
  {
    case AffineDistortion:
    case AffineProjectionDistortion:
    case ScaleRotateTranslateDistortion:
      ScaleResampleFilter( resample_filter,
        coefficients[0], coefficients[2],
        coefficients[1], coefficients[3] );
      break;
    default:
      break;
  }

  /* Initialize default pixel validity
   *    negative:         pixel is invalid  output 'matte_color'
   *    0.0 to 1.0:       antialiased, mix with resample output
   *    1.0 or greater:   use resampled output.
   */
  validity = 1.0;
  GetMagickPixelPacket(distort_image,&invalid);
  SetMagickPixelPacket(distort_image, &distort_image->matte_color,
             (IndexPacket *) NULL, &invalid);
  if (distort_image->colorspace == CMYKColorspace)
        ConvertRGBToCMYK(&invalid);   /* what about other color spaces? */

  /* Sample the source image to each pixel in the distort image.  */
  for (j=0; j < (long) distort_image->rows; j++)
  {
    q=SetCacheView(distort_view,0,j,distort_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetCacheViewIndexes(distort_view);
    y = j+geometry.y;
    for (i=0; i < (long) distort_image->columns; i++)
    {
      x = i+geometry.x;
      switch (method)
      {
        case AffineDistortion:
        case AffineProjectionDistortion:
        case ScaleRotateTranslateDistortion:
        {
          point.x=coefficients[0]*x+coefficients[2]*y+coefficients[4];
          point.y=coefficients[1]*x+coefficients[3]*y+coefficients[5];
          /* Affine partical derivitives are constant -- set above */
          break;
        }
        case BilinearDistortion:
        {
          point.x=coefficients[0]*x+coefficients[1]*y+coefficients[2]*x*y+
            coefficients[3];
          point.y=coefficients[4]*x+coefficients[5]*y+coefficients[6]*x*y+
            coefficients[7];
          /* Bilinear partical derivitives of scaling vectors */
          ScaleResampleFilter( resample_filter,
              coefficients[0] + coefficients[2]*y,
              coefficients[1] + coefficients[2]*x,
              coefficients[4] + coefficients[6]*y,
              coefficients[5] + coefficients[6]*x );
          break;
        }
        case PerspectiveDistortion:
        case PerspectiveProjectionDistortion:
        {
          double
            p,q,r,abs_r,abs_c6,abs_c7,scale;
          /* perspective is a ratio of affines */
          p=coefficients[0]*x+coefficients[1]*y+coefficients[2];
          q=coefficients[3]*x+coefficients[4]*y+coefficients[5];
          r=coefficients[6]*x+coefficients[7]*y+1.0;
          /* Pixel Validity -- is it a 'sky' or 'ground' pixel */
          validity = (r*coefficients[8] < 0.0) ? 0.0 : 1.0;
          /* Determine horizon anti-aliase blending */
          abs_r = fabs(r)*2;
          abs_c6 = fabs(coefficients[6]);
          abs_c7 = fabs(coefficients[7]);
          if ( abs_c6 > abs_c7 ) {
            if ( abs_r < abs_c6 )
              validity = 0.5 - coefficients[8]*r/coefficients[6];
          }
          else if ( abs_r < abs_c7 )
            validity = .5 - coefficients[8]*r/coefficients[7];
          /* Perspective Sampling Point (if valid) */
          if ( validity > 0.0 ) {
            scale = 1.0/r;
            point.x = p*scale;
            point.y = q*scale;
            /* Perspective Partial Derivatives or Scaling Vectors */
            scale *= scale;
            ScaleResampleFilter( resample_filter,
              (r*coefficients[0] - p*coefficients[6])*scale,
              (r*coefficients[1] - p*coefficients[7])*scale,
              (r*coefficients[3] - q*coefficients[6])*scale,
              (r*coefficients[4] - q*coefficients[7])*scale );
          }
          break;
        }
        case ArcDistortion:
        {
          double radius = sqrt((double) x*x+y*y);
          point.x = (atan2((double)y,(double)x) - coefficients[0])/(2*MagickPI);
          point.x -= MagickRound(point.x);
          point.x = point.x*coefficients[1] + coefficients[4];
          point.y = (coefficients[2] - radius) * coefficients[3];
          /* Polar Distortion Partial Derivatives or Scaling Vectors
             We give the deritives of  du/dr, dv/dr  and du/da, dv/da
             rather than provide the complex  du/dx,dv/dx and du/dy,dv/dy.
             The result will be the same, but it is simplier to calculate.
          */
          if ( radius > MagickEpsilon )
            ScaleResampleFilter( resample_filter,
                coefficients[1]/(2*MagickPI) / radius, 0, 0, coefficients[3] );
          else
            ScaleResampleFilter( resample_filter,
                 MagickHuge, 0, 0, coefficients[3] );
          break;
        }
        default:
        {
          /*
            Noop distortion (failsafe).
          */
          point.x=(double) i;
          point.y=(double) j;
          break;
        }
      }
      if ( bestfit && method != ArcDistortion ) {
        point.x -= image->page.x;
        point.y -= image->page.y;
      }

      if ( validity <= 0.0 ) {
        /* result of distortion is an invalid pixel - don't resample */
        SetPixelPacket(distort_image,&invalid,q,indexes);
      }
      else {
        /* resample the source image to find its correct color */
        pixel=ResamplePixelColor(resample_filter,point.x,point.y);
        /* if validity between 0.0 and 1.0 mix result with invalid pixel */
        if ( validity < 1.0 ) {
          /* Do a blend of sample color and invalid pixel */
          /* should this be a 'Blend', or an 'Over' compose */
          MagickPixelCompositeBlend(&pixel, validity,
               &invalid, (1.0-validity), &pixel);
        }
        SetPixelPacket(distort_image,&pixel,q,indexes);
      }
      q++;
      indexes++;
    }
    if (SyncImagePixels(distort_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(DistortImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  distort_view=CloseCacheView(distort_view);
  resample_filter=DestroyResampleFilter(resample_filter);

  /* Arc does not return an offset unless 'bestfit' is in effect */
  if ( method == ArcDistortion && !bestfit &&
       property==(const char *)NULL ) {
    distort_image->page.x = 0;
    distort_image->page.y = 0;
  }
  return(distort_image);
}
