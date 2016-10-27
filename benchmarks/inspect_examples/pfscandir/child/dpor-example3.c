#include <pthread.h>

int a, b;

void * thread1(void * arg)
{
  a++;
}


void * thread2(void * arg)
{
  b++;
}


void * thread3(void * arg)
{
  b--;
  a+=5;
}


int main()
{
  pthread_t  t1, t2, t3;

  pthread_create(&t1, NULL, thread1, NULL);
  pthread_create(&t2, NULL, thread2, NULL);
  pthread_create(&t3, NULL, thread3, NULL);
  
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  pthread_join(t3, NULL);

  return 0;
}
