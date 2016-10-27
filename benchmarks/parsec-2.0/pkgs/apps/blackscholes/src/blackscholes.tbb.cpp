// Copyright (c) 2007 Intel Corp.

// Black-Scholes
// Analytical method for calculating European Options
//
// 
// Reference Source: Options, Futures, and Other Derivatives, 3rd Edition, Prentice 
// Hall, John C. Hull,

//disables lots of C4305 warnings - truncation from double to float
//#pragma warning (disable : 4305)

#ifdef TBB_VERSION
#include "tbb/blocked_range.h"
#include "tbb/parallel_reduce.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"
#else
#include <sys/time.h>
#endif

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#ifdef TBB_VERSION
/* created parallel calls to CNDF procedure */
//#define PARALLEL_CNDF
int NUM_TASKS;
using namespace std;
using namespace tbb;
#endif


#ifdef ENABLE_PARSEC_HOOKS
#include <hooks.h>
#endif

// Multi-threaded header
#ifdef ENABLE_THREADS
#define MAX_THREADS 128
// Add the following line so that icc 9.0 is compatible with pthread lib.
#define __thread __threadp
MAIN_ENV
#undef __thread
#endif

//#define PRINTINFO
//#define ERR_CHK

//Precision to use for calculations
#define fptype float

#define NUM_RUNS 100

typedef struct OptionData_ {
        fptype s;          // spot price
        fptype strike;     // strike price
        fptype r;          // risk-free interest rate
        fptype divq;       // dividend rate
        fptype v;          // volatility
        fptype t;          // time to maturity or option expiration in years 
                           //     (1yr = 1.0, 6mos = 0.5, 3mos = 0.25, ..., etc)  
        char OptionType;   // Option type.  "P"=PUT, "C"=CALL
        fptype divs;       // dividend vals (not used in this test)
        fptype DGrefval;   // DerivaGem Reference Value
} OptionData;

OptionData *data;
fptype *prices;
int numOptions;

int    * otype;
fptype * sptprice;
fptype * strike;
fptype * rate;
fptype * volatility;
fptype * otime;
int numError = 0;
int nThreads;

fptype CNDF ( fptype InputX );
fptype BlkSchlsEqEuroNoDiv( fptype sptprice,
                            fptype strike, fptype rate, fptype volatility,
                            fptype time, int otype, float timet );



#ifdef TBB_VERSION
struct mainWork {
  fptype price;
public: 
  mainWork():price(0){}
  mainWork(mainWork &w, tbb::split){price = 0;}

  void operator()(const tbb::blocked_range<int> &range)  {
    fptype local_price=0;
    fptype priceDelta;
    int begin = range.begin();
    int end = range.end();
    
    for (int i=begin; i!=end; i++) {
      /* Calling main function to calculate option value based on 
       * Black & Sholes's equation.
       */

#ifdef PRINTINFO
      fprintf(stderr,"%d: %lf\t",i,sptprice[i]);
      fprintf(stderr,"%lf\t",strike[i]);
      fprintf(stderr,"%lf\t",rate[i]);
      fprintf(stderr,"%lf\t",volatility[i]);
      fprintf(stderr,"%lf\t",otime[i]);
#endif
      local_price += BlkSchlsEqEuroNoDiv( sptprice[i], strike[i],
				   rate[i], volatility[i], otime[i], 
				   otype[i], 0);
      prices[i] = local_price;

#ifdef PRINTINFO
      fprintf(stderr,"%lf\n",price);
#endif      

#ifdef ERR_CHK   
      priceDelta = data[i].DGrefval - price;
      if( fabs(priceDelta) >= 1e-5 ){
	fprintf(stderr,"Error on %d. Computed=%.5f, Ref=%.5f, Delta=%.5f\n",
	       i, price, data[i].DGrefval, priceDelta);
	numError ++;
      }
#endif
    }

    price +=local_price;
  }


  void join(mainWork &rhs){price += rhs.getPrice();}
  fptype getPrice(){return price;}

};



class CNDFTask: public tbb::task{
public:
  fptype InputX;
  fptype *output;
  CNDFTask(fptype in, fptype *out):InputX(in), output(out){}
  task *execute() {
    //    _print("CNDF input",output);
    *output = CNDF(InputX);
    return NULL;
  }
};


class launchCNDF: public tbb::task{
  bool is_continuation;
public:
  fptype InputX1;
  fptype *output1;
  fptype InputX2;
  fptype *output2;
  launchCNDF(fptype in1, fptype *out1, fptype in2, fptype *out2):
    is_continuation(false), InputX1(in1), output1(out1),InputX2(in2), output2(out2) 
  {}
  task *execute(){
    
    if(!is_continuation){
      recycle_as_continuation();
      set_ref_count(2);
      CNDFTask &task1 = *new ( tbb::task::allocate_child() ) CNDFTask(InputX1, output1);
      CNDFTask &task2 = *new ( tbb::task::allocate_child() ) CNDFTask(InputX2, output2);
      is_continuation = true;
      spawn(task1);
      return &task2;

    }else {
      /**/
      //_print("launchCNDF done!!",0);
      return NULL;
    }
  }
};

#endif // TBB_VERSION



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Cumulative Normal Distribution Function
// See Hull, Section 11.8, P.243-244
#define inv_sqrt_2xPI 0.39894228040143270286

fptype CNDF ( fptype InputX ) 
{
    int sign;

    fptype OutputX;
    fptype xInput;
    fptype xNPrimeofX;
    fptype expValues;
    fptype xK2;
    fptype xK2_2, xK2_3;
    fptype xK2_4, xK2_5;
    fptype xLocal, xLocal_1;
    fptype xLocal_2, xLocal_3;

    // Check for negative value of InputX
    if (InputX < 0.0) {
        InputX = -InputX;
        sign = 1;
    } else 
        sign = 0;

    xInput = InputX;
 
    // Compute NPrimeX term common to both four & six decimal accuracy calcs
    expValues = exp(-0.5f * InputX * InputX);
    xNPrimeofX = expValues;
    xNPrimeofX = xNPrimeofX * inv_sqrt_2xPI;

    xK2 = 0.2316419 * xInput;
    xK2 = 1.0 + xK2;
    xK2 = 1.0 / xK2;
    xK2_2 = xK2 * xK2;
    xK2_3 = xK2_2 * xK2;
    xK2_4 = xK2_3 * xK2;
    xK2_5 = xK2_4 * xK2;
    
    xLocal_1 = xK2 * 0.319381530;
    xLocal_2 = xK2_2 * (-0.356563782);
    xLocal_3 = xK2_3 * 1.781477937;
    xLocal_2 = xLocal_2 + xLocal_3;
    xLocal_3 = xK2_4 * (-1.821255978);
    xLocal_2 = xLocal_2 + xLocal_3;
    xLocal_3 = xK2_5 * 1.330274429;
    xLocal_2 = xLocal_2 + xLocal_3;

    xLocal_1 = xLocal_2 + xLocal_1;
    xLocal   = xLocal_1 * xNPrimeofX;
    xLocal   = 1.0 - xLocal;

    OutputX  = xLocal;
    
    if (sign) {
        OutputX = 1.0 - OutputX;
    }

    return OutputX;
} 

// For debugging
void print_xmm(fptype in, char* s) {
    fprintf(stderr,"%s: %lf\n", s, in);
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
fptype BlkSchlsEqEuroNoDiv( fptype sptprice,
                            fptype strike, fptype rate, fptype volatility,
                            fptype time, int otype, float timet )    
{
    fptype OptionPrice;

    // local private working variables for the calculation
    fptype xStockPrice;
    fptype xStrikePrice;
    fptype xRiskFreeRate;
    fptype xVolatility;
    fptype xTime;
    fptype xSqrtTime;

    fptype logValues;
    fptype xLogTerm;
    fptype xD1; 
    fptype xD2;
    fptype xPowerTerm;
    fptype xDen;
    fptype d1;
    fptype d2;
    fptype FutureValueX;
    fptype NofXd1;
    fptype NofXd2;
    fptype NegNofXd1;
    fptype NegNofXd2;    
    
    xStockPrice = sptprice;
    xStrikePrice = strike;
    xRiskFreeRate = rate;
    xVolatility = volatility;

    xTime = time;
    xSqrtTime = sqrt(xTime);

    logValues = log( sptprice / strike );
        
    xLogTerm = logValues;
        
    
    xPowerTerm = xVolatility * xVolatility;
    xPowerTerm = xPowerTerm * 0.5;
        
    xD1 = xRiskFreeRate + xPowerTerm;
    xD1 = xD1 * xTime;
    xD1 = xD1 + xLogTerm;

    xDen = xVolatility * xSqrtTime;

    xD1 = xD1 / xDen;

    xD2 = xD1 -  xDen;

    d1 = xD1;
    d2 = xD2;

#ifdef TBB_VERSION    

  #ifdef PARALLEL_CNDF
    launchCNDF &task1 = *new ( tbb::task::allocate_root() ) launchCNDF(d1, &NofXd1, d2, &NofXd2);    tbb::task::spawn_root_and_wait(task1);
  #else
    NofXd1 = CNDF( d1 );
    NofXd2 = CNDF( d2 ); 
  #endif //PARALLEL_CNDF

#else

    NofXd1 = CNDF( d1 );
    NofXd2 = CNDF( d2 ); 

#endif // !TBB_VERSION

    FutureValueX = strike * ( exp( -(rate)*(time) ) );        
    if (otype == 0) {            
        OptionPrice = (sptprice * NofXd1) - (FutureValueX * NofXd2);
    } else { 
        NegNofXd1 = (1.0 - NofXd1);
        NegNofXd2 = (1.0 - NofXd2);
        OptionPrice = (FutureValueX * NegNofXd2) - (sptprice * NegNofXd1);
    }
    
    return OptionPrice;
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
#ifdef TBB_VERSION
int bs_thread(void *tid_ptr) {
    int j;
    fptype price;
    fptype priceDelta;
    tbb::affinity_partitioner a;

    for (j=0; j<NUM_RUNS; j++) {
      mainWork doall;
      tbb::parallel_reduce(tbb::blocked_range<int>(0, numOptions), doall, a);
      price = doall.getPrice();
    }


    return 0;
}
#else // !TBB_VERSION

fptype *price;
int bs_thread(void *tid_ptr) {
    int i, j, k;
    fptype acc_price;
    fptype priceDelta;
    int tid = *(int *)tid_ptr;
    int start = tid * (numOptions / nThreads);
    int end = start + (numOptions / nThreads);

    if(tid == (nThreads-1))
       end = numOptions;

    for (j=0; j<NUM_RUNS; j++) {
      price[tid*LINESIZE] = 0;
        for (i=start; i<end; i++) {
            /* Calling main function to calculate option value based on 
             * Black & Sholes's equation.
             */
#ifdef PRINTINFO
	  fprintf(stderr,"%d: %lf\t",i,sptprice[i]);
	  fprintf(stderr,"%lf\t",strike[i]);
	  fprintf(stderr,"%lf\t",rate[i]);
	  fprintf(stderr,"%lf\t",volatility[i]);
	  fprintf(stderr,"%lf\t",otime[i]);
#endif

	  price[tid*LINESIZE] += BlkSchlsEqEuroNoDiv( sptprice[i], strike[i],
						      rate[i], volatility[i], otime[i], 
						      otype[i], 0);
	  
#ifdef PRINTINFO
	  fprintf(stderr,"%lf\n",price[tid*LINESIZE]);
#endif
	  
#ifdef ERR_CHK   
            priceDelta = data[i].DGrefval - price;
            if( fabs(priceDelta) >= 1e-4 ){
                printf("Error on %d. Computed=%.5f, Ref=%.5f, Delta=%.5f\n",
                       i, price, data[i].DGrefval, priceDelta);
                numError ++;
            }
#endif
        }

         if(tid==0) {
	   acc_price = 0;
	   for(i=0;i<nThreads;i++)
	     acc_price += price[i*LINESIZE];
	 }
    }

    return 0;
}

#endif // TBB_VERSION



int main (int argc, char **argv)
{
    FILE *file;
    int i;
    int loopnum;
    fptype * buffer;
    int * buffer2;
    int rv;

#ifdef PARSEC_VERSION
#define __PARSEC_STRING(x) #x
#define __PARSEC_XSTRING(x) __PARSEC_STRING(x)
        printf("PARSEC Benchmark Suite Version "__PARSEC_XSTRING(PARSEC_VERSION)"\n");
	fflush(NULL);
#else
        printf("PARSEC Benchmark Suite\n");
	fflush(NULL);
#endif //PARSEC_VERSION

#ifdef ENABLE_PARSEC_HOOKS
   __parsec_bench_begin(__parsec_blackscholes);
#endif

   if (argc != 4)
        {
                printf("Usage:\n\t%s <nthreads> <inputFile> <outputFile>\n", argv[0]);
                exit(1);
        }
    nThreads = atoi(argv[1]);
    char *inputFile = argv[2];
    char *outputFile = argv[3];

    //Read input data from file
    file = fopen(inputFile, "r");
    if(file == NULL) {
      printf("ERROR: Unable to open file `%s'.\n", inputFile);
      exit(1);
    }
    rv = fscanf(file, "%i", &numOptions);
    if(rv != 1) {
      printf("ERROR: Unable to read from file `%s'.\n", inputFile);
      fclose(file);
      exit(1);
    }
    if(nThreads > numOptions) {
      printf("WARNING: Not enough work, reducing number of threads to match number of options.");
      nThreads = numOptions;
    }

#ifdef TBB_VERSION
    //Determine work unit size etc.
    NUM_TASKS=(4*nThreads);
    if(NUM_TASKS > numOptions) NUM_TASKS = numOptions;
#endif

    // alloc spaces for the option data
    data = (OptionData*)malloc(numOptions*sizeof(OptionData));
    prices = (fptype*)malloc(numOptions*sizeof(fptype));
    for ( loopnum = 0; loopnum < numOptions; ++ loopnum )
    {
        rv = fscanf(file, "%f %f %f %f %f %f %c %f %f", &data[loopnum].s, &data[loopnum].strike, &data[loopnum].r, &data[loopnum].divq, &data[loopnum].v, &data[loopnum].t, &data[loopnum].OptionType, &data[loopnum].divs, &data[loopnum].DGrefval);
        if(rv != 9) {
          printf("ERROR: Unable to read from file `%s'.\n", inputFile);
          fclose(file);
          exit(1);
        }
    }
    rv = fclose(file);
    if(rv != 0) {
      printf("ERROR: Unable to close file `%s'.\n", inputFile);
      exit(1);
    }

#ifdef ENABLE_THREADS
    MAIN_INITENV(,8000000,nThreads);
#endif
    printf("Num of Options: %d\n", numOptions);
    printf("Num of Runs: %d\n", NUM_RUNS);

#ifdef PARALLEL_CNDF
    printf("Parallel CNDF active\n");
#endif

#define PAD 256
#define LINESIZE 64
   
    buffer = (fptype *) malloc(5 * numOptions * sizeof(fptype) + PAD);
    sptprice = (fptype *) (((unsigned long long)buffer + PAD) & ~(LINESIZE - 1));
    strike = sptprice + numOptions;
    rate = strike + numOptions;
    volatility = rate + numOptions;
    otime = volatility + numOptions;
    
    buffer2 = (int *) malloc(numOptions * sizeof(fptype) + PAD);
    otype = (int *) (((unsigned long long)buffer2 + PAD) & ~(LINESIZE - 1));
    
    for (i=0; i<numOptions; i++) {
        otype[i]      = (data[i].OptionType == 'P') ? 1 : 0;
        sptprice[i]   = data[i].s;
        strike[i]     = data[i].strike;
        rate[i]       = data[i].r;
        volatility[i] = data[i].v;    
        otime[i]      = data[i].t;
    }
    
    printf("Size of data: %d\n", numOptions * (sizeof(OptionData) + sizeof(int)));

#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_begin();
#endif

#ifdef TBB_VERSION
    tbb::task_scheduler_init init(nThreads);
#endif

#ifdef TBB_VERSION
    int tid=0;
    bs_thread(&tid);
#else

#ifdef ENABLE_THREADS
    int tids[nThreads];
    for(i=0; i<nThreads; i++) {
        tids[i]=i;
        CREATE_WITH_ARG(bs_thread, &tids[i]);
    }
    WAIT_FOR_END(nThreads);
#else
    int tid=0;
    bs_thread(&tid);
#endif //ENABLE_THREADS

#endif // TBB_VERSION

#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_end();
#endif

    //Write prices to output file
    file = fopen(outputFile, "w");
    if(file == NULL) {
      printf("ERROR: Unable to open file `%s'.\n", outputFile);
      exit(1);
    }
    rv = fprintf(file, "%i\n", numOptions);
    if(rv < 0) {
      printf("ERROR: Unable to write to file `%s'.\n", outputFile);
      fclose(file);
      exit(1);
    }
    for(i=0; i<numOptions; i++) {
      rv = fprintf(file, "%.18f\n", prices[i]);
      if(rv < 0) {
        printf("ERROR: Unable to write to file `%s'.\n", outputFile);
        fclose(file);
        exit(1);
      }
    }
    rv = fclose(file);
    if(rv != 0) {
      printf("ERROR: Unable to close file `%s'.\n", outputFile);
      exit(1);
    }

#ifdef ERR_CHK
    printf("Num Errors: %d\n", numError);
#endif
    free(data);
    free(prices);

#ifdef ENABLE_PARSEC_HOOKS
    __parsec_bench_end();
#endif

    return 0;
}

