/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                 RRRR    AAA   N   N  DDDD    OOO   M   M                    %
%                 R   R  A   A  NN  N  D   D  O   O  MM MM                    %
%                 RRRR   AAAAA  N N N  D   D  O   O  M M M                    %
%                 R R    A   A  N  NN  D   D  O   O  M   M                    %
%                 R  R   A   A  N   N  DDDD    OOO   M   M                    %
%                                                                             %
%                                                                             %
%                   Methods to Generate Random Numbers                        %
%                                                                             %
%                             Software Design                                 %
%                               John Cristy                                   %
%                              December 2001                                  %
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
%  The generation of random numbers is too important to be left to chance.
%                               -- Tom Christiansen <tchrist@mox.perl.com>
%
%
*/

/*
  Include declarations.
*/
#if defined(__VMS)
#include <time.h>
#endif
#if defined(__MINGW32__)
#include <sys/time.h>
#endif
#include "magick/studio.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/memory_.h"
#include "magick/semaphore.h"
#include "magick/random_.h"
#include "magick/resource_.h"
#include "magick/signature.h"
#include "magick/string_.h"
#include "magick/utility.h"

/*
  Typedef declarations.
*/
typedef struct _RandomInfo
{
  unsigned int
    w,
    x,
    y,
    z;
} RandomInfo;

/*
  Global declarations.
*/
static RandomInfo
  random_info = { ~0U, ~0U, ~0U, ~0U };

static SemaphoreInfo
  *random_semaphore = (SemaphoreInfo *) NULL;

static SignatureInfo
  *reservoir = (SignatureInfo *) NULL;

static unsigned long
  *roulette = (unsigned long *) NULL;

/*
  Forward declarations.
*/
static void
  InitializeRandomReservoir(void);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y R a n d o m R e s e r v i o r                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyRandomReservoir() deallocates memory associated with the random
%  reservoir.
%
%  The format of the DestroyRandomReservoir method is:
%
%      DestroyRandomReservoir(void)
%
*/
MagickExport void DestroyRandomReservoir(void)
{
  AcquireSemaphoreInfo(&random_semaphore);
  if (reservoir != (SignatureInfo *) NULL)
    {
      (void) ResetMagickMemory(reservoir,0,sizeof(*reservoir));
      reservoir=(SignatureInfo *) RelinquishMagickMemory(reservoir);
    }
  if (roulette != (unsigned long *) NULL)
    {
      (void) ResetMagickMemory(roulette,0,sizeof(*roulette));
      roulette=(unsigned long *) RelinquishMagickMemory(roulette);
    }
  (void) ResetMagickMemory(&random_info,0xff,sizeof(random_info));
  RelinquishSemaphoreInfo(random_semaphore);
  random_semaphore=DestroySemaphoreInfo(random_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D i s t i l l R a n d o m E v e n t                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DistillRandomEvent() distills randomness from an event and stores it int
%  the reservoir.  This method should be called before GetRandomKey() and it
%  should be called a number of times using different random events (e.g.
%  thread completion time, fine grained time-of-day clock in a tight loop,
%  keystroke timing, etc.) to build up sufficient randomness in the reservoir.
%
%  The format of the DistillRandomEvent method is:
%
%      DistillRandomEvent(const unsigned char *event,const size_t length)
%
%  A description of each parameter follows:
%
%    o event: A random event.
%
%    o length: The length of the event.
%
*/
MagickExport void DistillRandomEvent(const unsigned char *event,
  const size_t length)
{
  SignatureInfo
    digest_info;

  /*
    Distill a random event.
  */
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(event != (const unsigned char *) NULL);
  if ((reservoir == (SignatureInfo *) NULL) ||
      (roulette == (unsigned long *) NULL))
    {
      AcquireSemaphoreInfo(&random_semaphore);
      if (reservoir == (SignatureInfo *) NULL)
        reservoir=(SignatureInfo *) AcquireMagickMemory(sizeof(*reservoir));
      if (roulette == (unsigned long *) NULL)
        roulette=(unsigned long *) AcquireMagickMemory(sizeof(*roulette));
      RelinquishSemaphoreInfo(random_semaphore);
      if ((reservoir == (SignatureInfo *) NULL) ||
          (roulette == (unsigned long *) NULL))
        ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
      (void) ResetMagickMemory(reservoir,0,sizeof(*reservoir));
      (void) ResetMagickMemory(roulette,0,sizeof(*roulette));
      (void) ResetMagickMemory(&random_info,0xff,sizeof(random_info));
    }
  AcquireSemaphoreInfo(&random_semaphore);
  GetSignatureInfo(&digest_info);
  UpdateSignature(&digest_info,(const unsigned char *) reservoir->digest,
    sizeof(reservoir->digest));
  UpdateSignature(&digest_info,event,length);
  FinalizeSignature(&digest_info);
  (void) CopyMagickMemory(reservoir->digest,digest_info.digest,
    sizeof(reservoir->digest));
  RelinquishSemaphoreInfo(random_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t R a n d o m K e y                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetRandomKey() gets a random key from the reservoir.
%
%  The format of the GetRandomKey method is:
%
%      GetRandomKey(unsigned char *key,const size_t length)
%
%  A description of each parameter follows:
%
%    o key: The key.
%
%    o length: The key length.
%
*/
MagickExport void GetRandomKey(unsigned char *key,const size_t length)
{
  SignatureInfo
    digest_info;

  long
    n;

  assert(key != (unsigned char *) NULL);
  if ((roulette == (unsigned long *) NULL) ||
      (reservoir == (SignatureInfo *) NULL))
    InitializeRandomReservoir();
  AcquireSemaphoreInfo(&random_semaphore);
  for (n=(long) length; n > 0; n-=sizeof(reservoir->digest))
  {
    GetSignatureInfo(&digest_info);
    UpdateSignature(&digest_info,(const unsigned char *) reservoir->digest,
      sizeof(reservoir->digest));
    UpdateSignature(&digest_info,(const unsigned char *) roulette,
      sizeof(roulette));
    FinalizeSignature(&digest_info);
    (*roulette)++;
    (void) CopyMagickMemory(key,digest_info.digest,(size_t) (n < (long)
      sizeof(reservoir->digest) ? n : (long) sizeof(reservoir->digest)));
    key+=sizeof(reservoir->digest);
  }
  RelinquishSemaphoreInfo(random_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t R a n d o m V a l u e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetRandomValue() return a non-negative double-precision floating-point
%  value uniformly distributed over the interval [0.0, 1.0) with a 2 to the
%  128th-1 period.
%
%  The format of the GetRandomValue method is:
%
%      double GetRandomValue(void)
%
*/
MagickExport double GetRandomValue(void)
{
  unsigned int
    range,
    t;

  if ((roulette == (unsigned long *) NULL) ||
      (reservoir == (SignatureInfo *) NULL))
    InitializeRandomReservoir();
  while ((random_info.w == ~0U) || (random_info.x == (~0U)) ||
         (random_info.y == (~0U)) || (random_info.z == (~0U)))
  {
    GetRandomKey((unsigned char *) &random_info.w,sizeof(random_info.w));
    GetRandomKey((unsigned char *) &random_info.x,sizeof(random_info.x));
    GetRandomKey((unsigned char *) &random_info.y,sizeof(random_info.y));
    GetRandomKey((unsigned char *) &random_info.z,sizeof(random_info.z));
  }
  range=(~0U);
  do
  {
    t=(random_info.x ^ (random_info.x << 11));
    random_info.x=random_info.y;
    random_info.y=random_info.z;
    random_info.z=random_info.w;
    random_info.w=(random_info.w ^ (random_info.w >> 19)) ^ (t ^ (t >> 8));
  }
  while (random_info.w == range);
  return((double) random_info.w/range);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n i t i a l i z e R a n d o m R e s e r v i o r                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InitializeRandomReservoir() initializes the random reservoir with entropy.
%
%  The format of the InitializeRandomReservoir method is:
%
%      InitializeRandomReservoir(void)
%
*/
static void InitializeRandomReservoir(void)
{
  char
    filename[MaxTextExtent];

  int
    file;

  long
    pid;

  time_t
    nanoseconds,
    seconds;

  unsigned char
    random[MaxTextExtent];

  /*
    Initialize random reservoir.
  */
  seconds=time((time_t *) 0);
  nanoseconds=0;
#if defined(HAVE_GETTIMEOFDAY)
  {
    struct timeval
      timer;

    if (gettimeofday(&timer,0) == 0)
      {
        seconds=timer.tv_sec;
        nanoseconds=(time_t) (1000*timer.tv_usec);
      }
  }
#endif
#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_HIGHRES)
  {
    struct timespec
      timer;

    if (clock_gettime(CLOCK_HIGHRES,&timer) == 0)
      {
        seconds=timer.tv_sec;
        nanoseconds=timer.tv_nsec;
      }
  }
#endif
  DistillRandomEvent((const unsigned char *) &seconds,sizeof(seconds));
  DistillRandomEvent((const unsigned char *) &nanoseconds,sizeof(nanoseconds));
  nanoseconds=0;
#if defined(HAVE_TIMES)
  {
    struct tms
      timer;

    (void) times(&timer);
    nanoseconds=timer.tms_utime+timer.tms_stime;
  }
#else
#if defined(__WINDOWS__)
  nanoseconds=NTElapsedTime()+NTUserTime();
#else
  nanoseconds=clock();
#endif
#endif
  DistillRandomEvent((const unsigned char *) &nanoseconds,sizeof(nanoseconds));
  pid=(long) getpid();
  DistillRandomEvent((const unsigned char *) &pid,sizeof(pid));
  DistillRandomEvent((const unsigned char *) &roulette,sizeof(roulette));
  (void) AcquireUniqueFilename(filename);
  DistillRandomEvent((const unsigned char *) filename,strlen(filename));
  (void) RelinquishUniqueFileResource(filename);
  file=open("/dev/urandom",O_RDONLY | O_BINARY);
  if (file != -1)
    {
      ssize_t
        count;

      count=read(file,random,MaxTextExtent);
      file=close(file)-1;
      DistillRandomEvent(random,(size_t) count);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e e d R a n d o m E v e n t                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SeedRandomReservoir() sets its argument as the seed for a new sequence of
%  pseudo-random numbers to be returned by GetRandomValue().
%
%  The format of the SeedRandomReservoir method is:
%
%      SeedRandomReservoir(const unsigned long seed)
%
%  A description of each parameter follows:
%
%    o seed: The seed.
%
*/
MagickExport void SeedRandomReservoir(const unsigned long seed)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  DestroyRandomReservoir();
  DistillRandomEvent((const unsigned char *) &seed,sizeof(seed));
}
