#include <pthread.h>

pthread_mutex_t  a, b;
int counter=1;

void * thread1(void * arg)
{
  pthread_mutex_lock(&a);
  pthread_mutex_lock(&b); /* BAD: deadlock */
  counter++;
  pthread_mutex_unlock(&b);
  pthread_mutex_unlock(&a);

  return NULL;
}


void * thread2(void * arg)
{
  pthread_mutex_lock(&b);
  pthread_mutex_lock(&a); /* BAD: deadlock */
  counter--;
  pthread_mutex_unlock(&a);
  pthread_mutex_unlock(&b);

  return NULL;
}


int main()
{  
  pthread_t t1, t2;
  
  pthread_mutex_init(&a, 0);
  pthread_mutex_init(&b, 0);

  pthread_create(&t1, 0, thread1, 0);
  pthread_create(&t2, 0, thread2, 0);

  pthread_join(t1, 0);
  pthread_join(t2, 0);
  
  return 0;
}
