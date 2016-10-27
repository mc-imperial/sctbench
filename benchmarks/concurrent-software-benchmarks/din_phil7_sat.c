#include <pthread.h>
#include <assert.h>
#include "common.inc"

#define N 7

//void __ESBMC_atomic_begin();
//void __ESBMC_atomic_end();

int phil;
pthread_mutex_t  x[N];

void *thread1(void *arg)
{
  int id, *aptr1, left, right;
  
  aptr1=(int *)arg;
  id=*aptr1;

  left=id;
  right=(id+1)%N;

  __ESBMC_atomic_begin();
  pthread_mutex_lock(&x[right]);
  pthread_mutex_lock(&x[left]);
  pthread_mutex_unlock(&x[left]);
  pthread_mutex_unlock(&x[right]);
  __ESBMC_atomic_begin();

  __ESBMC_atomic_begin();
  ++phil;
  if (phil==N)
    assert(0);
  __ESBMC_atomic_end();
}

int main()
{
  int i;
  int arg[N];
  pthread_t trd_id[N];

  for(i=0; i<N; i++)
    pthread_mutex_init(&x[i], NULL);

  for(i=0; i<N; i++)
  {
    arg[i]=i;
    pthread_create(&trd_id[i], 0, thread1, &arg[i]);
  }

  for(i=0; i<N; i++)
    pthread_join(trd_id[i], 0);

  return 0;
}

