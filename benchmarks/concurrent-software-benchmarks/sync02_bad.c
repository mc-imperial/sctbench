#include <pthread.h>
#define N 2
int num;
pthread_mutex_t m;
pthread_cond_t empty, full;
void* producer(void* arg) {
  int i = 0;
  while (i < N) {
    pthread_mutex_lock(&m);
    while (num > 0) 
      pthread_cond_wait(&empty, &m);        
    num++; //produce
    pthread_mutex_unlock(&m);
    pthread_cond_signal(&full);
    i++;
} }
void* consumer(void* arg) {
  int j = 0;
  while (j < N){
    pthread_mutex_lock(&m);
    while (num == 0) 
      pthread_cond_wait(&full, &m);    
    num--; //consume
    pthread_mutex_unlock(&m);    
    pthread_cond_signal(&empty);
    j++;    
} } 
int main() {
  pthread_t  id1, id2;
  num = 2;
  pthread_mutex_init(&m, 0);
  pthread_cond_init(&empty, 0);
  pthread_cond_init(&full, 0);  
  pthread_create(&id1, 0, producer, 0);
  pthread_create(&id2, 0, consumer, 0);  
  pthread_join(id1, 0);
  pthread_join(id2, 0);
  return 0; 
}
