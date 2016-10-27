#ifndef RTTL_THREAD_HXX
#define RTTL_THREAD_HXX

#include "RTInclude.hxx"

#include <pthread.h>

#if !defined(_WIN32)

#include "atomic/atomic.h"
#include <stdint.h>

typedef volatile int32_t atomic_t;

_INLINE int atomic_add(atomic_t *v, const int c) 
{
  int rv,flag;
  do {
    register int temp_v = *v;
    rv = temp_v + c;
    flag=atomic_cmpset_32(v, temp_v, rv); 
  } while(!flag);
  return rv;
}

_INLINE int atomic_inc(atomic_t *v) {
  return atomic_add(v,1);
} 

_INLINE int atomic_dec(atomic_t *v) {
  return atomic_add(v,-1);
}

#else

typedef volatile LONG atomic_t;

_INLINE int atomic_add(atomic_t *v, const int c) 
{
  LONG value = InterlockedExchangeAdd(v,(LONG)c);
  return (int)value;
}

_INLINE int atomic_inc(atomic_t *v)
{
  LONG value = atomic_add(v,1);
  return (int)value;
}

_INLINE int atomic_dec(atomic_t *v)
{
  LONG value = atomic_add(v,-1);
  return (int)value;
}


#endif

/* class should have the right alignment to prevent cache trashing */
class AtomicCounter
{
private:
  atomic_t m_counter;
  char dummy[64-sizeof(atomic_t)]; // (iw) to make sure it's the only
                                   // counter sitting in its
                                   // cacheline....
public:

  AtomicCounter() {
    reset();
  }

  AtomicCounter(const int v) {
    m_counter = v;
  }

  _INLINE void reset() {
#if defined(_WIN32)
    m_counter = 0;
#else
    m_counter = -1;
#endif
  }

  _INLINE int inc() {
    return atomic_inc(&m_counter);
  }

  _INLINE int dec() {
    return atomic_dec(&m_counter);
  }

  _INLINE int add(const int i) {
    return atomic_add(&m_counter,i);
  }


};

#define DBG_THREAD(x) 

#undef NEEDS_PTHREAD_BARRIER_T_WRAPPER
#ifdef NEEDS_PTHREAD_BARRIER_T_WRAPPER
class Barrier
{
  int total;
  int waiting;
  pthread_cond_t m_cond;
  pthread_mutex_t m_mutex;

public:
  void init(int count)
  {
    pthread_mutex_init(&m_mutex,NULL);
    pthread_cond_init(&m_cond,NULL);
    total = count; 
    waiting = 0;
  }
  void wait()
  {
    pthread_mutex_lock(&m_mutex);
    ++waiting;
    if (waiting == total)
      {
        pthread_cond_broadcast(&m_cond);
        waiting = 0;
      }
    else
      {
        pthread_cond_wait(&m_cond,&m_mutex);
      }
    pthread_mutex_unlock(&m_mutex);
  }
};
#else
class Barrier
{
protected:
  pthread_barrier_t m_barrier;
public:
  void init(int count)
  {
    pthread_barrier_init(&m_barrier,NULL,count);
  }
  void wait()
  {
    pthread_barrier_wait(&m_barrier);
  }
};
#endif

class MultiThreadedTaskQueue
{

protected:
  int m_state;
  pthread_cond_t m_cond;
  pthread_mutex_t m_mutex;
//   pthread_barrier_t m_barrier;
  Barrier m_barrier;
  pthread_t *m_thread;
  int m_threads;

  _INLINE void lock()
  {
    DBG_THREAD(PING);
    pthread_mutex_lock(&m_mutex);
  }

  _INLINE void unlock()
  {
    DBG_THREAD(PING);
    pthread_mutex_unlock(&m_mutex);
  }

  _INLINE void cond_wait()
  {
    DBG_THREAD(PING);
    pthread_cond_wait(&m_cond,&m_mutex);
  }

  _INLINE void cond_broadcast()
  {
    DBG_THREAD(PING);
    pthread_cond_broadcast(&m_cond);
  }

  _INLINE void init_barrier(const int count) {
    DBG_THREAD(PING);
    m_barrier.init(count);
//     pthread_barrier_init(&m_barrier,NULL,count);
  }

  static void *threadFunc(void *_mttq);

public:

  enum {
    THREAD_EXIT,
    THREAD_RUNNING
  };

  MultiThreadedTaskQueue() 
  {
    DBG_THREAD(PING);
    m_thread = NULL;
    m_state = 0;
    m_threads = 1;
    pthread_mutex_init(&m_mutex,NULL);
    pthread_cond_init(&m_cond,NULL);

  }

  virtual int task(const int threadId)
  {
    DBG_THREAD(PING);
    /* nothing to do */
    return THREAD_RUNNING;
  }

  void createThreads(int threads) {
    DBG_THREAD(PING);
    assert(threads>=1);
    m_threads = threads;
    m_thread = aligned_malloc<pthread_t>(m_threads);
    init_barrier(m_threads);
    int res;
    for (int i=0;i<m_threads;i++)
      if ( (res = pthread_create(&m_thread[i],NULL,threadFunc,this)) != 0)
        {
          cerr << "can't create thread " << i << ". reason : " << res << endl;
          exit(EXIT_FAILURE);
        }   
  }

  void joinThreads() {
    DBG_THREAD(PING);
    assert(m_threads);
    int res;
    for (int i=0;i<m_threads;i++)
      {
    if ( (res = pthread_join(m_thread[i],NULL)) != 0)
      {
        cerr << "can't join thread " << i << ". reason : " << res << endl;
        exit(EXIT_FAILURE);
      }
      }
    aligned_free(m_thread);
  }
  
  _INLINE void startThreads() {
    DBG_THREAD(PING);
    lock();
    m_state = (1<<m_threads)-1;
    DBG_THREAD(DBG_PRINT(m_state));
    unlock();
    cond_broadcast();
  }

  _INLINE void waitForAllThreads() {
    DBG_THREAD(PING);
    lock();
    DBG_THREAD(DBG_PRINT(m_state));
    while(m_state != 0) 
      {
    cond_wait();
    DBG_THREAD(DBG_PRINT(m_state));
      }
    unlock();
  }

  _INLINE void waitForThreadActivation(const int threadId) {
    DBG_THREAD(PING);
    lock();
    DBG_THREAD(DBG_PRINT(m_state));
    while((m_state & (1<<threadId)) == 0) 
      {
    cond_wait();
    DBG_THREAD(DBG_PRINT(m_state));
      }
    unlock();
  }

  _INLINE void deactivateThreadAndBroadcast(const int threadId) {
    DBG_THREAD(PING);
    lock();
    m_state &= ~(1<<threadId);
    DBG_THREAD(DBG_PRINT(m_state));
    if (m_state == 0) cond_broadcast();
    unlock();
  }

  _INLINE void waitOnBarrier() {
    m_barrier.wait();
//     pthread_barrier_wait(&m_barrier);
  }
};

#endif
