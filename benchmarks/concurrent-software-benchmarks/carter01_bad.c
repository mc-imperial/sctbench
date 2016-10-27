#include <pthread.h>
pthread_mutex_t  m, l;
int A = 0, B = 0;
void *t1(void *arg) {
  pthread_mutex_lock(&m);
  A++;
  if (A == 1) pthread_mutex_lock(&l);
  pthread_mutex_unlock(&m);
  //perform class A operation
  pthread_mutex_lock(&m);
  A--;
  if (A == 0) pthread_mutex_unlock(&l);
  pthread_mutex_unlock(&m);
}
void *t2(void *arg) {
  pthread_mutex_lock(&m);
  B++;
  if (B == 1) pthread_mutex_lock(&l);
  pthread_mutex_unlock(&m);
  //perform class B operation
  pthread_mutex_lock(&m);
  B--;
  if (B == 0) pthread_mutex_unlock(&l);
  pthread_mutex_unlock(&m);
}
void *t3(void *arg) {
}
void *t4(void *arg) {
}
int main(void) {
  pthread_mutex_init(&m,NULL);
  pthread_mutex_init(&l,NULL);
  pthread_t a1, a2, b1, b2;
  pthread_create(&a1, NULL, t1, NULL); 
  pthread_create(&b1, NULL, t2, NULL);
  pthread_create(&a2, NULL, t3, NULL); 
  pthread_create(&b2, NULL, t4, NULL);
  pthread_join(a1, NULL);
  pthread_join(b1, NULL);
  pthread_join(a2, NULL);
  pthread_join(b2, NULL);
  return 0;
}
