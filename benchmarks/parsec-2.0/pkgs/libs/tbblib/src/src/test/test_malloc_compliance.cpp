/*
    Copyright 2005-2008 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

#define MByte 1048576 //1MB

#if _WIN32 || _WIN64
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#include <windows.h>
void limitMem( int limit )
{
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo;
    jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_PROCESS_MEMORY;
    jobInfo.ProcessMemoryLimit = limit*MByte;
    HANDLE hJob = CreateJobObject(NULL, NULL);
    AssignProcessToJobObject(hJob, GetCurrentProcess());
    SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jobInfo, sizeof(jobInfo));
}
#else
#include <sys/resource.h>
void limitMem( int limit )
{
    rlimit rlim;
    rlim.rlim_cur = limit*MByte;
    rlim.rlim_max = limit*MByte;
    setrlimit(RLIMIT_AS,&rlim);
}
#endif 

#include <time.h>
#include <errno.h>
#include <vector>
#include "tbb/scalable_allocator.h"

/* to avoid dependency on TBB shared library, the following macro should be
   defined to non-zero _before_ including any TBB or test harness headers.
   Also tbb_assert_impl.h from src/tbb should be included right after */
#define __TBB_NO_IMPLICIT_LINKAGE 1
#include "../tbb/tbb_assert_impl.h"

#include "tbb/blocked_range.h"
#include "harness.h"

#pragma pack(1)

#define COUNT_ELEM_CALLOC 2
#define COUNT_TESTS 1000

#define COUNT_ELEM 50000
#define MAX 1000
#define APPROACHES 100
#define COUNTEXPERIMENT 10000
#define MB 25
#define MB_MAX 500

const char strError[]="failed";
const char strOk[]="done";

typedef unsigned int UINT;
typedef unsigned char UCHAR;
typedef unsigned long DWORD;
typedef unsigned char BYTE;


typedef void* TestMalloc(size_t size);
typedef void* TestCalloc(size_t num, size_t size);
typedef void* TestRealloc(void* memblock, size_t size);
typedef void  TestFree(void* memblock);

TestMalloc* Tmalloc;
TestCalloc* Tcalloc;
TestRealloc* Trealloc;
TestFree* Tfree;



struct MemStruct
{
  void* Pointer;
  UINT Size;
};

class CMemTest
{
  UINT CountErrors;
public:
  bool FullLog;

  CMemTest();
  CMemTest(bool verb);
  void ReallocParam(); // realloc with different parameters
  void InvariantDataRealloc(); //realloc does not change data
  void NULLReturn(UINT MinSize, UINT MaxSize); // NULL pointer + check errno
  void UniquePointer(); // unique pointer - check with padding
  void AddrArifm(); // unique pointer - check with pointer arithmetic
  void Free_NULL(); // 
  void Zerofilling(); // check if arrays are zero-filled
  void RunAllTests();
  ~CMemTest() {};
};

int argC;
char** argV;

struct RoundRobin {
    const long number_of_threads;
	mutable CMemTest test;
    RoundRobin( long p ) : number_of_threads(p) ,test() {}

    void operator()( const tbb::blocked_range<long> &r ) const 
    {
        test.FullLog=Verbose;
        test.RunAllTests();
    }
};



int main(int argc, char* argv[])
{
	argC=argc;
	argV=argv;
	MaxThread = MinThread = 1;
  Tmalloc=scalable_malloc;
  Trealloc=scalable_realloc;
  Tcalloc=scalable_calloc;
  Tfree=scalable_free;
  // check if we were called to test standard behavior
  for (int i=1; i< argc; i++) {
    if (strcmp((char*)*(argv+i),"-s")==0)
    {
      argC--;
      Tmalloc=malloc;
      Trealloc=realloc;
      Tcalloc=calloc;
      Tfree=free;
      break;
    }
  }
  ParseCommandLine( argC, argV );
  limitMem( 500*MaxThread );
  //-------------------------------------
   for( int p=MaxThread; p>=MinThread; --p ) {
        limitMem( 500*p );
        if( Verbose ) printf("testing with %d threads\n", p );
        NativeParallelFor( tbb::blocked_range<long>(0,p,1), RoundRobin(p) );
    }
    printf("done\n");
  return 0;
}

struct TestStruct
{
  DWORD field1:2;
  DWORD field2:6;
  double field3;
  UCHAR field4[100];
  TestStruct* field5;
//  std::string field6;
  std::vector<int> field7;
  double field8;
  bool IzZero()
  {
    UCHAR *tmp;
    tmp=(UCHAR*)this;
    bool b=true;
    for (int i=0; i<(int)sizeof(TestStruct); i++)
      if (tmp[i]) b=false;
    return b;
  }
};


CMemTest::CMemTest()
{
  time_t zzz;
  srand((UINT)time(&zzz));
  FullLog=false;
  rand();
}
CMemTest::CMemTest(bool verb)
{
  time_t zzz;
  srand((UINT)time(&zzz));
  FullLog=verb;
  rand();
}

void CMemTest::InvariantDataRealloc()
{
  UINT size, sizeMin;
  CountErrors=0;
  if (FullLog) printf("\nInvariant data by realloc....");
  UCHAR* pchar;
  sizeMin=size=rand()%MAX+10;
  pchar=(UCHAR*)Trealloc(NULL,size);
  for (UINT k=0; k<size; k++)
    pchar[k]=(UCHAR)k%255+1;
  for (UINT i=0; i<COUNTEXPERIMENT; i++)
  {
    size=rand()%MAX+10;
    sizeMin=size<sizeMin ? size : sizeMin;
    pchar=(UCHAR*)Trealloc(pchar,size);
    for (UINT k=0; k<sizeMin; k++)
      if (pchar[k] != (UCHAR)k%255+1)
      {
        CountErrors++;
        if (FullLog)
        {
          printf("stand '%c', must stand '%c'\n",pchar[k],(UCHAR)k%255+1);
          printf("error: data changed (at %d, SizeMin=%d)\n",k,sizeMin);
        }
      }
  }
  Trealloc(pchar,0);
  if (CountErrors) printf("%s\n",strError);
  else if (FullLog) printf("%s\n",strOk);
  //printf("end check\n");
}
void CMemTest::ReallocParam()
{
  TestStruct *TSold, *TSnew;
  CountErrors=0;
  UINT CountTry=0;
  if (FullLog) printf("\nrealloc with different params....");
  //
  for (UINT i=0; i<COUNTEXPERIMENT; i++)
  {
    TSnew=(TestStruct*)Tmalloc(sizeof(TestStruct)); // allocate memory
    TSold=TSnew;  // store address
    Tfree(TSnew); // free memory
    TSnew=(TestStruct*)Tmalloc(sizeof(TestStruct));
    while (TSnew != TSold)
    {
      CountTry++;
      TSold=TSnew;
      Tfree(TSnew);
      TSnew=(TestStruct*)Tmalloc(sizeof(TestStruct));
    }
    TSold=TSnew;
    Trealloc(TSnew,0); //realloc should work as free
    TSnew=NULL;
    TSnew=(TestStruct*)Trealloc(NULL,sizeof(TestStruct)); //realloc should work as malloc
    if (TSold == TSnew)
    {
/*     if (FullLog)
      {
        printf("CountTry: %d\n", CountTry);
        printf("realloc ok\n");
      }
*/
    }
    else
    {
      CountErrors++;
      if (FullLog) printf("realloc error");
    }
  }
  if (CountErrors) printf("%s\n",strError);
  else if (FullLog) printf("%s\n",strOk);
}

void CMemTest::AddrArifm()
{
  int* MasPointer[COUNT_ELEM];
  UINT MasCountElem[COUNT_ELEM];
  DWORD count;
  int* tmpAddr;
  UINT j;
  UINT CountZero=0;
  CountErrors=0;
  if (FullLog) printf("\nUnique pointer using Address arithmetics\n");
  if (FullLog) printf("malloc....");
  for (UINT i=0; i<COUNT_ELEM; i++)
  {
    count=rand()%MAX;
    if (count == 0)
      CountZero++;
    tmpAddr=(int*)Tmalloc(count*sizeof(int));
    for (j=0; j<i; j++) // find a place for the new address
    {
      if (*(MasPointer+j)>tmpAddr) break;
    }
    for (UINT k=i; k>j; k--)
    {
      MasPointer[k]=MasPointer[k-1];
      MasCountElem[k]=MasCountElem[k-1];
    }
    MasPointer[j]=tmpAddr;
    MasCountElem[j]=count*sizeof(int);/**/
  }
  if (FullLog) printf("Count zero: %d\n",CountZero);
  for (int i=0; i<COUNT_ELEM-1; i++)
  {
    if ((uintptr_t)MasPointer[i]+MasCountElem[i] > (uintptr_t)MasPointer[i+1])
    {
      CountErrors++;
//      if (FullLog) printf("intersection detect at 0x%x between %d element(int) and 0x%x\n"
//      ,(MasPointer+i),MasCountElem[i],(MasPointer+i+1));
    }
  }
  if (CountErrors) printf("%s\n",strError);
  else if (FullLog) printf("%s\n",strOk);
  //----------------------------------------------------------------
  CountErrors=0;
  if (FullLog) printf("realloc....");
  for (UINT i=0; i<COUNT_ELEM; i++)
  {
    count=MasCountElem[i]*2;
    if (count == 0)
      CountZero++;
    tmpAddr=(int*)Trealloc(MasPointer[i],count);
    for (j=0; j<i; j++) // find a place for the new address
    {
      if (*(MasPointer+j)>tmpAddr) break;
    }
    for (UINT k=i; k>j; k--)
    {
      MasPointer[k]=MasPointer[k-1];
      MasCountElem[k]=MasCountElem[k-1];
    }
    MasPointer[j]=tmpAddr;
    MasCountElem[j]=count;//*sizeof(int);/**/
  }
  if (FullLog) printf("Count zero: %d\n",CountZero);

  // now we have a sorted array of pointers
  for (int i=0; i<COUNT_ELEM-1; i++)
  {
    if ((uintptr_t)MasPointer[i]+MasCountElem[i] > (uintptr_t)MasPointer[i+1])
    {
      CountErrors++;
//      if (FullLog) printf("intersection detect at 0x%x between %d element(int) and 0x%x\n"
//      ,(MasPointer+i),MasCountElem[i],(MasPointer+i+1));
    }
  }
  if (CountErrors) printf("%s\n",strError);
  else if (FullLog) printf("%s\n",strOk);
  for (int i=0; i<COUNT_ELEM; i++)
  {
    Tfree(MasPointer[i]);
  }
  //-------------------------------------------
  CountErrors=0;
  if (FullLog) printf("calloc....");
  for (UINT i=0; i<COUNT_ELEM; i++)
  {
    count=rand()%MAX;
    if (count == 0)
      CountZero++;
    tmpAddr=(int*)Tcalloc(count*sizeof(int),2);
    for (j=0; j<i; j++) // find a place for the new address
    {
      if (*(MasPointer+j)>tmpAddr) break;
    }
    for (UINT k=i; k>j; k--)
    {
      MasPointer[k]=MasPointer[k-1];
      MasCountElem[k]=MasCountElem[k-1];
    }
    MasPointer[j]=tmpAddr;
    MasCountElem[j]=count*sizeof(int)*2;/**/
  }
  if (FullLog) printf("Count zero: %d\n",CountZero);

  // now we have a sorted array of pointers
  for (int i=0; i<COUNT_ELEM-1; i++)
  {
    if ((uintptr_t)MasPointer[i]+MasCountElem[i] > (uintptr_t)MasPointer[i+1])
    {
      CountErrors++;
//      if (FullLog) printf("intersection detect at 0x%x between %d element(int) and 0x%x\n"
//      ,(MasPointer+i),MasCountElem[i],(MasPointer+i+1));
    }
  }
  if (CountErrors) printf("%s\n",strError);
  else if (FullLog) printf("%s\n",strOk);
  for (int i=0; i<COUNT_ELEM; i++)
  {
    Tfree(MasPointer[i]);
  }
}

void CMemTest::Zerofilling()
{
  TestStruct* TSMas;
  int CountElement;
  CountErrors=0;
  if (FullLog) printf("\nzeroings elements of array....");
  //test struct
  for (int i=0; i<COUNTEXPERIMENT; i++)
  {
    CountElement=rand()%MAX;
    TSMas=(TestStruct*)Tcalloc(CountElement,sizeof(TestStruct));
    for (int j=0; j<CountElement; j++)
    {
      if (!TSMas->IzZero())
      {
        CountErrors++;
        if (FullLog) printf("detect nonzero element at TestStruct\n");
      }
    }
    Tfree(TSMas);
  }
  if (CountErrors) printf("%s\n",strError);
  else if (FullLog) printf("%s\n",strOk);
}



void CMemTest::NULLReturn(UINT MinSize, UINT MaxSize)
{
  std::vector<MemStruct> PointerList;
  MemStruct MStruct;
  void *tmp=Tmalloc(MByte);
  CountErrors=0;
  int CountNULL;
  if (FullLog) printf("\nNULL return & check errno:\n");
  UINT i=0;
  UINT Size;
  BYTE *zer;
  /* //----------------------------------------------
  float mb=0;
  UINT mbInd=0;
  if (FullLog)
  {
    printf("0");
    for (i=0; i<MB_MAX/MB; i++)
      printf(" ");
    printf("500Mb");
    for (i=0; i<MB_MAX/MB-4; i++)
      printf(" ");
    printf("1Gb\n");
  }
  */ //----------------------------------------------
  while(tmp != NULL)
  {
    Size=rand()%(MaxSize-MinSize)+MinSize;
  /* //----------------------------------------------
    if (FullLog)
    {
      mb+=((float)Size)/MByte;
      if (mb > mbInd)
      {
        printf(".");
        mbInd+=MB;
      }
    }
  */ //----------------------------------------------
    tmp=Tmalloc(Size);
    if (tmp != NULL)
    {
      zer=(BYTE*)tmp;
      for (UINT k=0; k<Size; k++)
        zer[k]=0;
      MStruct.Pointer=tmp;
      MStruct.Size=Size;
      PointerList.push_back(MStruct);
    }
    i++;
  }
  if (FullLog) printf("\n");

  // preparation complete, now running tests
  // malloc
  if (FullLog) printf("malloc....");
  CountNULL = 0;
  while (CountNULL==0)
   for (int j=0; j<COUNT_TESTS; j++)
  {
    Size=rand()%(MaxSize-MinSize)+MinSize;
    errno = 0;
    tmp=Tmalloc(Size);
    if (tmp == NULL)
    {
      CountNULL++;
      if (errno != ENOMEM) {
        CountErrors++;
        if (FullLog) printf("NULL returned, error: errno != ENOMEM\n");
      }
    }
    else
    {
      if (errno != 0) {
        CountErrors++;
        if (FullLog) printf("valid pointer returned, error: errno not kept\n");
      }      
      zer=(BYTE*)tmp;
      for (UINT k=0; k<Size; k++)
        zer[k]=0;
      MStruct.Pointer=tmp;
      MStruct.Size=Size;
      PointerList.push_back(MStruct);
      i++;
    }
  }
  if (FullLog) printf("end malloc\n");
  if (CountErrors) printf("%s\n",strError);
  else if (FullLog) printf("%s\n",strOk);
  CountErrors=0;
  //calloc
  if (FullLog) printf("calloc....");
  CountNULL = 0;
  while (CountNULL==0)
   for (int j=0; j<COUNT_TESTS; j++)
  {
    Size=rand()%(MaxSize-MinSize)+MinSize;
    errno = 0;
    tmp=Tcalloc(COUNT_ELEM_CALLOC,Size);
    if (tmp == NULL)
    {
      CountNULL++;
      if (errno != ENOMEM) {
        CountErrors++;
        if (FullLog) printf("NULL returned, error: errno != ENOMEM\n");
      }
    }
    else
    {
      if (errno != 0) {
        CountErrors++;
        if (FullLog) printf("valid pointer returned, error: errno not kept\n");
      }      
      MStruct.Pointer=tmp;
      MStruct.Size=Size;
      PointerList.push_back(MStruct);
      i++;
    }
  }
  if (FullLog) printf("end calloc\n");
  if (CountErrors) printf("%s\n",strError);
  else if (FullLog) printf("%s\n",strOk);
  CountErrors=0;
  if (FullLog) printf("realloc....");
  CountNULL = 0;
  while (CountNULL==0)
   for (i=0;i<COUNT_TESTS && i<PointerList.size();i++)
  {
    errno = 0;
    tmp=Trealloc(PointerList[i].Pointer,PointerList[i].Size*2);
    if (PointerList[i].Pointer == tmp) // the same place
    {
      if (errno != 0) {
        CountErrors++;
        if (FullLog) printf("valid pointer returned, error: errno not kept\n");
      }      
      PointerList[i].Size *= 2;
    }
    else if (tmp != PointerList[i].Pointer && tmp != NULL) // another place
    {
      if (errno != 0) {
        CountErrors++;
        if (FullLog) printf("valid pointer returned, error: errno not kept\n");
      }      
      PointerList[i].Pointer = tmp;
      PointerList[i].Size *= 2;
    }
    else if (tmp == NULL)
    {
      CountNULL++;
      if (errno != ENOMEM)
      {
        CountErrors++;
        if (FullLog) printf("NULL returned, error: errno != ENOMEM\n");
      }
      // check data integrity
      zer=(BYTE*)PointerList[i].Pointer;
      for (UINT k=0; k<PointerList[i].Size; k++)
        if (zer[k] != 0)
        {
          CountErrors++;
          if (FullLog) printf("NULL returned, error: data changed\n");
        }
    }
  }
  if (FullLog) printf("realloc end\n");
  if (CountErrors) printf("%s\n",strError);
  else if (FullLog) printf("%s\n",strOk);
  for (UINT i=0; i<PointerList.size(); i++)
  {
    Tfree(PointerList[i].Pointer);
  }
}


void CMemTest::UniquePointer()
{
  CountErrors=0;
  int* MasPointer[COUNT_ELEM];
  UINT MasCountElem[COUNT_ELEM];
  if (FullLog) printf("\nUnique pointer using 0\n");
  //
  //-------------------------------------------------------
  //malloc
  for (int i=0; i<COUNT_ELEM; i++)
  {
    MasCountElem[i]=rand()%MAX;
    MasPointer[i]=(int*)Tmalloc(MasCountElem[i]*sizeof(int));
    for (UINT j=0; j<MasCountElem[i]; j++)
      *(MasPointer[i]+j)=0;
  }
  if (FullLog) printf("malloc....");
  for (UINT i=0; i<COUNT_ELEM-1; i++)
  {
    for (UINT j=0; j<MasCountElem[i]; j++)
    {
      if (*(*(MasPointer+i)+j)!=0)
      {
        CountErrors++;
//        if (FullLog) printf("error, detect 1 with 0x%x\n",(*(MasPointer+i)+j));
      }
      *(*(MasPointer+i)+j)+=1;
    }
  }
  if (CountErrors) printf("%s\n",strError);
  else if (FullLog) printf("%s\n",strOk);
  //----------------------------------------------------------
  //calloc
  for (int i=0; i<COUNT_ELEM; i++)
    Tfree(MasPointer[i]);
  CountErrors=0;
  for (long i=0; i<COUNT_ELEM; i++)
  {
    MasPointer[i]=(int*)Tcalloc(MasCountElem[i]*sizeof(int),2);
  }
  if (FullLog) printf("calloc....");
  for (int i=0; i<COUNT_ELEM-1; i++)
  {
    for (UINT j=0; j<*(MasCountElem+i); j++)
    {
      if (*(*(MasPointer+i)+j)!=0)
      {
        CountErrors++;
//        if (FullLog) printf("error, detect 1 with 0x%x\n",(*(MasPointer+i)+j));
      }
      *(*(MasPointer+i)+j)+=1;
    }
  }
  if (CountErrors) printf("%s\n",strError);
  else if (FullLog) printf("%s\n",strOk);
  //---------------------------------------------------------
  //realloc
  CountErrors=0;
  for (UINT i=0; i<COUNT_ELEM; i++)
  {
    MasCountElem[i]*=2;
    *(MasPointer+i)=(int*)Trealloc(*(MasPointer+i),MasCountElem[i]*sizeof(int));
    for (UINT j=0; j<MasCountElem[i]; j++)
      *(*(MasPointer+i)+j)=0;
  }
  if (FullLog) printf("realloc....");
  for (int i=0; i<COUNT_ELEM-1; i++)
  {
    for (UINT j=0; j<*(MasCountElem+i); j++)
    {
      if (*(*(MasPointer+i)+j)!=0)
      {
        CountErrors++;
      }
      *(*(MasPointer+i)+j)+=1;
    }
  }
  if (CountErrors) printf("%s\n",strError);
  else if (FullLog) printf("%s\n",strOk);
  for (int i=0; i<COUNT_ELEM; i++)
    Tfree(MasPointer[i]);
}

void CMemTest::Free_NULL()
{
  CountErrors=0;
  if (FullLog) printf("\ncall free with parameter NULL....");
  for (UINT i=0; i<COUNTEXPERIMENT; i++)
  {
    Tfree(NULL);
    if (errno != 0)
    {
      CountErrors++;
      if (FullLog) printf("error is found by a call free with parameter NULL\n");
    }
  }
  if (CountErrors) printf("%s\n",strError);
  else if (FullLog) printf("%s\n",strOk);
}

void CMemTest::RunAllTests()
{
  Zerofilling();
  Free_NULL();
  InvariantDataRealloc();
  ReallocParam();
#if __APPLE__
  printf("Warning: skipping some tests (known issue on Mac OS* X)\n");
  return;
#else
  UniquePointer();
  AddrArifm();
  NULLReturn(1*MByte,100*MByte);
#endif
  if (FullLog) printf("All tests ended\nclearing memory....");
}
