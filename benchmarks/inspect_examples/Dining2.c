#include <stdlib.h>     // Dining Philosophers with no deadlock
#include <pthread.h>    // all phils but "odd" one pickup their
#include <stdio.h>      // left fork first; odd phil picks
#include <string.h>     // up right fork first
#include <malloc.h>
#include <errno.h>
#include <sys/types.h>
#include <assert.h>

#define NUM_THREADS 2 

pthread_mutex_t mutexes[NUM_THREADS];
pthread_cond_t conditionVars[NUM_THREADS];
int permits[NUM_THREADS];
pthread_t tids[NUM_THREADS];

int data = 0;

void * Philosopher(void * arg){
  int i;
  i = (int)arg;

  // pickup left fork
  pthread_mutex_lock(&mutexes[i%NUM_THREADS]);
  while (permits[i%NUM_THREADS] == 0) {
    //printf("P%d : attempt to get F%d\n", i, i%NUM_THREADS);
    pthread_cond_wait(&conditionVars[i%NUM_THREADS],&mutexes[i%NUM_THREADS]);
  }
  permits[i%NUM_THREADS] = 0;
  //printf("P%d : get F%d\n", i, i%NUM_THREADS);
  pthread_mutex_unlock(&mutexes[i%NUM_THREADS]); 

  // pickup right fork
  pthread_mutex_lock(&mutexes[(i+1)%NUM_THREADS]);
  while (permits[(i+1)%NUM_THREADS] == 0) { 
    //printf("P%d : attempt to get F%d\n", i, (i+1)%NUM_THREADS);
    pthread_cond_wait(&conditionVars[(i+1)%NUM_THREADS],&mutexes[(i+1)%NUM_THREADS]);
  }
  permits[(i+1)%NUM_THREADS] = 0;
  //printf("P%d : get F%d\n", i, (i+1)%NUM_THREADS);
  pthread_mutex_unlock(&mutexes[(i+1)%NUM_THREADS]); 

  printf("philosopher %d thinks \n",i); 

  //  data = 10 * data + i;

  fflush(stdout);


  // putdown right fork
  pthread_mutex_lock(&mutexes[(i+1)%NUM_THREADS]);
  permits[(i+1)%NUM_THREADS] = 1;
  //printf("P%d : put F%d\n", i, (i+1)%NUM_THREADS);
  pthread_cond_signal(&conditionVars[(i+1)%NUM_THREADS]);
  pthread_mutex_unlock(&mutexes[(i+1)%NUM_THREADS]);


  // putdown left fork
  pthread_mutex_lock(&mutexes[i%NUM_THREADS]);
  permits[i%NUM_THREADS] = 1;
  //printf("P%d : put F%d\n", i, i%NUM_THREADS);
  pthread_cond_signal(&conditionVars[i%NUM_THREADS]);
  pthread_mutex_unlock(&mutexes[i%NUM_THREADS]);


  return NULL;

}

void * OddPhilosopher(void * arg){
  int i;
  i = (int)arg;

  // pickup right fork
  pthread_mutex_lock(&mutexes[(i+1)%NUM_THREADS]);
  while (permits[(i+1)%NUM_THREADS] == 0) {
    //printf("P%d : attempt to get F%d\n", i, (i+1)%NUM_THREADS);
    pthread_cond_wait(&conditionVars[(i+1)%NUM_THREADS],&mutexes[(i+1)%NUM_THREADS]);
  }
  permits[(i+1)%NUM_THREADS] = 0;
  //printf("P%d : get F%d \n", i, (i+1)%NUM_THREADS);
  pthread_mutex_unlock(&mutexes[(i+1)%NUM_THREADS]);

  // pickup left fork
  pthread_mutex_lock(&mutexes[i%NUM_THREADS]);
  while (permits[i%NUM_THREADS] == 0) {
    //printf("P%d : attempt to get F%d\n", i, i%NUM_THREADS);
    pthread_cond_wait(&conditionVars[i%NUM_THREADS],&mutexes[i%NUM_THREADS]);
  }
  permits[i%NUM_THREADS] = 0;
  //printf("P%d : get F%d \n", i, i%NUM_THREADS);
  pthread_mutex_unlock(&mutexes[i%NUM_THREADS]); 


  // data = 10 * data + i;

  printf("philosopher %d thinks\n",i); 
  fflush(stdout);

  // putdown left fork
  pthread_mutex_lock(&mutexes[i%NUM_THREADS]);
  permits[i%NUM_THREADS] = 1; 
  //printf("P%d : put F%d \n", i, i%NUM_THREADS);
  pthread_cond_signal(&conditionVars[i%NUM_THREADS]);
  pthread_mutex_unlock(&mutexes[i%NUM_THREADS]);

  // putdown right fork
  pthread_mutex_lock(&mutexes[(i+1)%NUM_THREADS]);
  permits[(i+1)%NUM_THREADS] = 1;
  //printf("P%d : put F%d \n", i, (i+1)%NUM_THREADS);
  pthread_cond_signal(&conditionVars[(i+1)%NUM_THREADS]);
  pthread_mutex_unlock(&mutexes[(i+1)%NUM_THREADS]);


  return NULL;

}

int main(){
  int i;

  for (i = 0; i < NUM_THREADS; i++)
    pthread_mutex_init(&mutexes[i], NULL);
  for (i = 0; i < NUM_THREADS; i++)
    pthread_cond_init(&conditionVars[i], NULL);
  for (i = 0; i < NUM_THREADS; i++)
    permits[i] = 1;

  for (i = 0; i < NUM_THREADS-1; i++){
    pthread_create(&tids[i], NULL,  Philosopher, (void*)(i) );
  }

  pthread_create(&tids[NUM_THREADS-1], NULL,  OddPhilosopher, (void*)(NUM_THREADS-1) );

  for (i = 0; i < NUM_THREADS; i++){
    pthread_join(tids[i], NULL);
  }

  for (i = 0; i < NUM_THREADS; i++){
    pthread_mutex_destroy(&mutexes[i]);
  }
  for (i = 0; i < NUM_THREADS; i++){
    pthread_cond_destroy(&conditionVars[i]);
  }

  //printf(" data = %d \n", data);

  //assert( data != 201);
  return 0;
}

