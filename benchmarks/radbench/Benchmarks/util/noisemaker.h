#ifndef NOISEMAKER_H
#define NOISEMAKER_H
#include <sys/syscall.h>
#include <sys/time.h>

int nick_noisemaker_count = 0;

#define NoiseMaker(X, Y) { \
    if (Y) { \
        long int tid = syscall(__NR_gettid); \
        fprintf(stderr, "Thread %lu ", tid); \
        fprintf(stderr, "is at noise maker on "); \
        fprintf(stderr, "line %d in file %s ", __LINE__, __FILE__); \
        fprintf(stderr, "of function %s ", __FUNCTION__); \
        fprintf(stderr, "with %d other threads\n", nick_noisemaker_count); \
    } \
    struct timeval my_time; \
    gettimeofday(&my_time, NULL); \
    srand(my_time.tv_usec); \
    if (!(rand() % X)) { \
        nick_noisemaker_count++; \
        usleep(rand() % 1000000); \
        nick_noisemaker_count--; \
    } \
}

#endif


#ifndef NICKTRACER_H
#define NICKTRACER_H

#include <unistd.h>
#include <sys/syscall.h>

#define NickTracer(x) { \
    long int tid = syscall(__NR_gettid); \
    fprintf(stderr, "Thread %lu ", tid); \
    fprintf(stderr, "is at "); \
    fprintf(stderr, "line %d in file %s ", __LINE__, __FILE__); \
    fprintf(stderr, "of function %s (", __FUNCTION__); \
    fprintf(stderr, "bug " #x ")\n"); \
}

#endif


