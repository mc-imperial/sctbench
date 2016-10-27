/* 
 * Copyright (C) 2005 Abel Bennett.
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the GNU C Library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307 USA.
 */

/**
 * @author Abel Bennett
 * @date 2002/11/17
 * @version 1.0
 */


#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <assert.h>


#include <alloca.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


/**
 * @brief sort base in the order specified by the compare function
 * @param base data array
 * @param nel number of elements in data array
 * @param width size of element in data array
 * @param compare compare function for element in data array
 * @param num_threads number of threads to use
 * @warning <ul><li>default thread attributes used</li></ul>
 * @warning <ul><li>cancellation is disabled</li></ul>
 */
extern void pthread_qsort(void * base, const size_t nel, const size_t width,
        int (*compare)(const void *, const void *), const size_t num_threads);


/**
 * @brief ctf major version number
 */
#define PTHREAD_CTF_MAJOR 0

/**
 * @brief ctf minor version number
 */
#define PTHREAD_CTF_MINOR 1

/**
 * @brief ctf micro version number
 */
#define PTHREAD_CTF_MICRO 0


/**
 * @brief returns the number of on-line processors
 * @param number number of processors
 * @return
 * 0 is returned on success<br>
 * non-zero error code is returned on failure
 */
extern int pthread_get_nproc(int * number);

/**
 * @brief returns a string representing the pthread_t
 * @param tid pthread_t ID [input]
 * @param buf pointer to a character array
 * @param len maximum length for buffer
 * @return
 * 0 is returned on success<br>
 * non-zero error code is returned on failure
 */
extern int pthread_get_name(const pthread_t tid, char * buf, const size_t len);

/**
 * @brief returns the version of pthreads library
 * @param buf pointer to a character array
 * @param len maximum length for buffer
 * @return
 * 0 is returned on success<br>
 * non-zero error code is returned on failure
 */
extern int pthread_version(char * buf, const size_t len);

/**
 * @brief returns the version of ctf library
 * @param buf pointer to a character array
 * @param len maximum length for buffer
 * @return
 * 0 is returned on success<br>
 * non-zero error code is returned on failure
 */
extern int pthread_ctf_version(char * buf, const size_t len);



struct pthread_qsort_data_t
{
    void * base; /* address of data */
    size_t width; /* size of data element */
    size_t first; /* index of first data (inclusive) */
    size_t last; /* index of last data (inclusive) */
    int (*compare)(const void *, const void *); /* compare function */
    size_t switch_size; /* number of elements to use in threading */
    pthread_mutex_t * mutex; /* lock for thread function */
    pthread_cond_t * cond; /* condition for thread function */
    int * finished; /* return value of thread */
};


static void * pthread_qsort_local_call(void * arg);
/*
    arguments:
        (1) pointer to an initialized pthread_qsort_t [input]
    functionality:
        (1) act as a wrapper to convert from threading function
            call, single argument, to the non-threaded function
            call, multiple arguments
*/

inline static void pthread_qsort_local(void * base, const size_t width,
        const size_t first, const size_t last,
        int (*compare)(const void *, const void *),
        const size_t switch_size);
/*
    arguments:
        (1) pointer to data array [input/output]
        (2) element size [input]
        (3) first index (inclusive) [input]
        (4) last index (inclusive) [input]
        (5) compare function for element in data array [input]
        (6) size to switch to non-threaded qsort [input]
    functionality:
        (1) to sort array in base in the order specified by the
            compare function
*/

inline static void pthread_qsort_split(void * base, const size_t width,
        const size_t first, const size_t last, size_t * middle,
        int (*compare)(const void *, const void *));
/*
    arguments:
        (1) pointer to data array [input/output]
        (2) element size [input]
        (3) first index (inclusive) [input]
        (4) last index (inclusive) [input]
        (5) pointer to sorted middle [output]
        (6) compare function for element in data array [input]
    functionality:
        (1) to find sorted middle of base array
*/

inline static void pthread_qsort_swap(void * base, const size_t width,
        const size_t elem1, const size_t elem2);
/*
    arguments:
        (1) pointer to data array [input/output]
        (2) element size [input]
        (3) element index (inclusive) [input]
        (4) element index (inclusive) [input]
    functionality:
        (1) to swap base array elements first and last
*/


void pthread_qsort(void * base, const size_t nel, const size_t width,
        int (*compare)(const void *, const void *), const size_t num_threads)
{
    int switch_size;

    switch_size = nel / num_threads;

    /* call threaded qsort function */
    pthread_qsort_local(base, width, 0, (nel - 1), compare,
            (size_t)switch_size);

    return;
}

static void * pthread_qsort_local_call(void * arg)
{
    struct pthread_qsort_data_t * data = (struct pthread_qsort_data_t *)arg;


    /*
        convert from threaded, single argument, to non-threaded,
        multiple arguments, function call
    */
    pthread_qsort_local(data->base, data->width, data->first, data->last,
            data->compare, data->switch_size);

    /* lock thread lock */
    if(pthread_mutex_lock(data->mutex) != 0)
    {
        abort();
    }

    (*(data->finished)) = 1;

    /* signal thread caller */
    if(pthread_cond_signal(data->cond) != 0)
    {
        abort();
    }

    /* unlock thread lock */
    if(pthread_mutex_unlock(data->mutex) != 0)
    {
        abort();
    }

    return NULL;
}

inline static void pthread_qsort_local(void * base, const size_t width,
        const size_t first, const size_t last,
        int (*compare)(const void *, const void *),
        const size_t switch_size)
{
    int thread_finished;
    size_t middle;
    struct pthread_qsort_data_t work;
    pthread_mutex_t mutex;

    pthread_mutex_init(&mutex, 0);

    pthread_cond_t cond;
    
    pthread_cond_init(&cond, NULL); /* = PTHREAD_COND_INITIALIZER; */

    pthread_t tid;
    pthread_attr_t attr;



    if((last - first) > switch_size)
    {
        /* split the array */
        pthread_qsort_split(base, width, first, last, &middle, compare);

        /* initialize number of threads completed */
        thread_finished = 0;

        /* copy to threaded function call structure */
        work.base = base;
        work.width = width;
        work.first = first;
        work.last = (middle - 1);
        work.compare = compare;
        work.switch_size = switch_size;
        work.mutex = &mutex;
        work.cond = &cond;
        work.finished = &thread_finished;

        /* initialize thread attributes to default */
        if(pthread_attr_init(&attr) != 0)
        {
            abort();
        }

        /* assign detach attribute */
        if(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
        {
            abort();
        }

        /* call a new thread for first half of data */
        if(pthread_create(&tid, &attr, pthread_qsort_local_call, &work) != 0)
        {
            abort();
        }

        /* use current thread for second half of data */
        pthread_qsort_local(base, width, (middle + 1), last,
                compare, switch_size);

        /* lock local lock */
        if(pthread_mutex_lock(&mutex) != 0)
        {
            abort();
        }

        /* while create thread is working */
        while(thread_finished == 0)
        {
            /* wait for created thread to signal */
            if(pthread_cond_wait(&cond, &mutex) != 0)
            {
                abort();
            }
        }

        /* unlock local lock */
        if(pthread_mutex_unlock(&mutex) != 0)
        {
            abort();
        }

        /* destroy thread attributes */
        if(pthread_attr_destroy(&attr) != 0)
        {
            abort();
        }
    }
    else
    {
        /* call non-threaded libc qsort */
        qsort((void *)((size_t)base + (first * width)), (last - first + 1),
                width, compare);
    }

    return;
}

inline static void pthread_qsort_split(void * base, const size_t width,
        const size_t first, const size_t last, size_t * middle,
        int (*compare)(const void *, const void *))
{
    short done;
    size_t ii, jj, temp_middle;
    void * test_temp;


    /* allocate temporary item */
    test_temp = alloca(width);

    /* find the middle of the data */
    temp_middle = (first + last) / 2;

    /* find three median of the three: first, middle, last */

    if(compare((void *)((size_t)base + (temp_middle * width)),
            (void *)((size_t)base + (first * width))) < 0)
    {
        pthread_qsort_swap(base, width, temp_middle, first);
    }

    if(compare((void *)((size_t)base + (last * width)),
            (void *)((size_t)base + (first * width))) < 0)
    {
        pthread_qsort_swap(base, width, last, first);
    }

    if(compare((void *)((size_t)base + (last * width)),
            (void *)((size_t)base + (temp_middle * width))) < 0)
    {
        pthread_qsort_swap(base, width, last, temp_middle);
    }

    /* copy median to temporary location, test_temp */
    (void)memcpy(test_temp, (void *)((size_t)base +
            (temp_middle * width)), width);

    /* swap middle with last */
    pthread_qsort_swap(base, width, temp_middle, (last - 1));

    /* initialize variables */
    ii = first;
    jj = last - 1;
    done = 0;

    while(done == 0)
    {
        ii++;

        /* while index ii is less than median */
        while(compare((void *)((size_t)base + (ii * width)), test_temp) < 0)
        {
            ii++;
        }

        jj--;

        /* while index jj is greater than median */
        while(compare((void *)((size_t)base + (jj * width)), test_temp) > 0)
        {
            jj--;
        }

        /* if more work to be done */
        if(ii < jj)
        {
            /* swap index ii and jj */
            pthread_qsort_swap(base, width, ii, jj);
        }
        else
        {
            /* all finished */
            done = 1;
        }
    }

    /* insert median into correct position */
    pthread_qsort_swap(base, width, ii, (last - 1));

    /* set median location */
    (*middle) = ii;

    return;
}

inline static void pthread_qsort_swap(void * base, const size_t width,
        const size_t elem1, const size_t elem2)
{
    void * swap_temp;


    /* allocate swap space */
    swap_temp = alloca(width);

    /* copy index first to temp */
    (void)memcpy(swap_temp, (void *)((size_t)base + (elem1 * width)), width);

    /* copy index last to index first */
    (void)memcpy((void *)((size_t)base + (elem1 * width)),
            (void *)((size_t)base + (elem2 * width)), width);

    /* copy temp to index last */
    (void)memcpy((void *)((size_t)base + (elem2 * width)), swap_temp, width);

    return;
}








int pthread_ctf_version(char * buf, const size_t len)
{
    (void)snprintf(buf, len, "%d.%d.%d", PTHREAD_CTF_MAJOR,
            PTHREAD_CTF_MINOR, PTHREAD_CTF_MICRO);

    return 0;
}


#if defined(__sun)

#include <stdio.h>
#include <sys/utsname.h>


int pthread_get_nproc(int * number)
{
    if(number == NULL)
    {
        return EFAULT;
    }

    /* get number of processors online */
    if(((*number) = (int)sysconf(_SC_NPROCESSORS_ONLN)) == -1)
    {
        return ENOTSUP;
    }

    return 0;
}

int pthread_get_name(const pthread_t tid, char * buf, const size_t len)
{
    /* convert to a string */
    (void)snprintf(buf, len, "%d", (int)tid);

    return 0;
}

int pthread_version(char * buf, const size_t len)
{
    struct utsname info;


    /* get system uname information */
    if(uname(&info) == -1)
    {
        abort();
    }

    /* write version informatin to string */
    (void)snprintf(buf, len, "%s", info.release);

    return 0;
}

#elif defined(__linux)

int pthread_get_nproc(int * number)
{
    if(number == NULL)
    {
        return EFAULT;
    }

    /* get number of processors online */
    if(((*number) = (int)sysconf(_SC_NPROCESSORS_ONLN)) == -1)
    {
        return ENOTSUP;
    }

    return 0;
}

int pthread_get_name(const pthread_t tid, char * buf, const size_t len)
{
    /* convert to a string */
    (void)snprintf(buf, len, "%d", (int)tid);

    return 0;
}

int pthread_version(char * buf, const size_t len)
{
#if defined(_CS_GNU_LIBPTHREAD_VERSION)
    confstr(_CS_GNU_LIBPTHREAD_VERSION, buf, len);
#else
    (void)snprintf(buf, len, "linuxthreads 0.00");
#endif

    return 0;
}

#elif defined(__sgi)

int pthread_get_nproc(int * number)
{
    if(number == NULL)
    {
        return EFAULT;
    }

    /* get number of processors online */
    if(((*number) = (int)sysconf(_SC_NPROC_ONLN)) == -1)
    {
        return ENOTSUP;
    }

    return 0;
}

int pthread_get_name(const pthread_t tid, char * buf, const size_t len)
{
    /* convert to a string */
    (void)snprintf(buf, len, "%d", (int)tid);

    return 0;
}

int pthread_version(char * buf, const size_t len)
{
    struct utsname info;


    /* get system uname information */
    if(uname(&info) == -1)
    {
        abort();
    }

    /* write version informatin to string */
    (void)snprintf(buf, len, "%s", info.release);

    return 0;
}

#else

#error New Operating System for pthread_get_nproc()
#error New Operating System for pthread_get_name()
#error New Operating System for pthread_get_version()

#endif







#define SIZE 100


static long orig_array[SIZE];
static long sort_array[SIZE];


static int work_long(const void * arg1, const void * arg2);


int main(void)
{
    int ii, rtn;
    int num_cpus;
    int hour, min;
    float sec;
    struct timeval start, stop;


    for(ii = 0; ii < SIZE; ii++)
    {
        orig_array[ii] = mrand48();
        sort_array[ii] = orig_array[ii];
    }

    if(gettimeofday(&start, NULL) != 0)
    {
        fprintf(stderr, "%s: %d: gettimeofday() ERROR: %s\n",
                __FILE__, __LINE__, strerror(errno));
        exit(EXIT_FAILURE);
    }

    qsort(sort_array, SIZE, sizeof(long), work_long);

    if(gettimeofday(&stop, NULL) != 0)
    {
        fprintf(stderr, "%s: %d: gettimeofday() ERROR: %s\n",
                __FILE__, __LINE__, strerror(errno));
        exit(EXIT_FAILURE);
    }

    sec = (stop.tv_sec - start.tv_sec) +
            ((stop.tv_usec - start.tv_usec) / 1.0e6);

    min = sec / 60;
    sec = sec - (min * 60);

    hour = min / 60;
    min = min - (hour * 60);
    
#if 0
    fprintf(stdout, "qsort test time: %02d:%02d:%07.4f\n",
            hour, min, sec);
#endif
    
    for(ii = 1; ii < SIZE; ii++)
    {
        if(sort_array[ii-1] > sort_array[ii])
        {
            fprintf(stderr, "%s: %d: ERROR: array not sorted\n",
                    __FILE__, __LINE__);
            assert(0);
            exit(EXIT_FAILURE);
        }
    }

    for(ii = 0; ii < SIZE; ii++)
    {
        sort_array[ii] = orig_array[ii];
    }

    //if((rtn = pthread_get_nproc(&num_cpus)) != 0)
    //{
    //    fprintf(stderr, "%s: %d: pthread_get_nproc() ERROR: %s\n",
    //            __FILE__, __LINE__, strerror(rtn));
    //    exit(EXIT_FAILURE);
    //}

    //if(num_cpus == 1)
    //{
        num_cpus = 4;
    //}

#if (defined(__sun) || defined(__sgi))
    //pthread_setconcurrency(num_cpus);
#endif

    if(gettimeofday(&start, NULL) != 0)
    {
        fprintf(stderr, "%s: %d: gettimeofday() ERROR: %s\n",
                __FILE__, __LINE__, strerror(errno));
        exit(EXIT_FAILURE);
    }

    pthread_qsort(sort_array, SIZE, sizeof(long), work_long, (size_t)num_cpus);

    if(gettimeofday(&stop, NULL) != 0)
    {
        fprintf(stderr, "%s: %d: gettimeofday() ERROR: %s\n",
                __FILE__, __LINE__, strerror(errno));
        exit(EXIT_FAILURE);
    }

    sec = (stop.tv_sec - start.tv_sec) +
            ((stop.tv_usec - start.tv_usec) / 1.0e6);

    min = sec / 60;
    sec = sec - (min * 60);

    hour = min / 60;
    min = min - (hour * 60);

#if 0
    fprintf(stdout, "pthread_qsort_t test time: %02d:%02d:%07.4f\n",
            hour, min, sec);
#endif

    for(ii = 1; ii < SIZE; ii++)
    {
        if(sort_array[ii-1] > sort_array[ii])
        {
            fprintf(stderr, "%s: %d: ERROR: array not sorted\n",
                    __FILE__, __LINE__);
            assert(0);
            exit(EXIT_FAILURE);
        }
    }

    return 1;
}

static int work_long(const void * arg1, const void * arg2)
{
    const long * data1 = (const long *)arg1;
    const long * data2 = (const long *)arg2;


    if((*data1) < (*data2))
    {
        return -1;
    }
    else if((*data1) > (*data2))
    {
        return 1;
    }

    return 0;
}
