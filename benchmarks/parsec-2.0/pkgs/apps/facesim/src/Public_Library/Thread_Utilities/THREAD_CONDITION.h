//#####################################################################
// Copyright 2005, Andrew Selle.
// This file is part of PhysBAM whose distribution is governed by the license contained in the accompanying file PHYSBAM_COPYRIGHT.txt.
//#####################################################################
#ifndef __THREAD_CONDITION__
#define __THREAD_CONDITION__

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include "THREAD_LOCK.h"

#if defined(ALAMERE_PTHREADS)
#   include "alamere.h"
#else
#    include <pthread.h>
#endif 

namespace PhysBAM{

class THREAD_CONDITION
{
private:
    pthread_cond_t cond;
public:
    
    THREAD_CONDITION()
    {pthread_cond_init(&cond,0);}

    ~THREAD_CONDITION()
    {pthread_cond_destroy(&cond);}

    void Wait(THREAD_LOCK& lock)
    {pthread_cond_wait(&cond,&lock.mutex);}

    void Signal()
    {pthread_cond_signal(&cond);}

    void Broadcast()
    {pthread_cond_broadcast(&cond);}
};
}
#endif

