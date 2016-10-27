
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>


#define FORK_VALID  0xABDADA


#define FORKS_SUM        5
#define PHILOSOPHERS_SUM  5
#define NUM_ITERATIONS  1


static char* states[] = { "thinking", 
			  "getting left fork", 
			  "getting right fork", 
			  "eating" ,
                          "putdown left fork", 
			  "putdown right fork"
			};



typedef struct fork_tag
{
  int valid; 
  pthread_mutex_t mutex;
  pthread_cond_t  free;
  
  int being_used;
  int waiting;
}fork_t;


typedef struct philosopher_tag
{
  int       id;
  pthread_t tid;

  fork_t  * left;
  fork_t  * right;
  char    * state;
}philosopher_t;



int fork_init(fork_t* fork)
{
  int status;
  
  if (fork == NULL) return EINVAL;
  
  fork->valid = FORK_VALID;

  status = pthread_mutex_init(&fork->mutex, NULL);
  if (status != 0) return status;

  status = pthread_cond_init(&fork->free, NULL);
  if (status != 0) {
    pthread_mutex_destroy(&fork->mutex);
    return status;
  }
  
  fork->being_used = fork->waiting = 0;  

  return 0;
}


int fork_destroy(fork_t * fork)
{
  int status, status2;
  
  if (fork == NULL || fork->valid != FORK_VALID) return EINVAL;
  
  status = pthread_mutex_lock(&fork->mutex);
  if (status != 0) return status;

  if (fork->being_used > 0 || fork->waiting > 0)
  {
    status2 = pthread_mutex_unlock(&fork->mutex);
    return (status2 == 0)? EBUSY : status2;
  }

  fork->valid = 0;
 
  status = pthread_mutex_unlock(&fork->mutex);
  if (status != 0) return status;

  status = pthread_mutex_destroy(&fork->mutex);
  status2= pthread_cond_destroy(&fork->free);
  
  return (status == 0)? status2 : status;
}


/**
 *  We do not disable PTHREAD_CANCEL or set up cleanup handler here
 *  just for simplicity. 
 */
int fork_get(fork_t * fork)
{
  int status, status2;

  if (fork == NULL || fork->valid != FORK_VALID) return EINVAL;

  status = pthread_mutex_lock(&fork->mutex);
  if (status != 0) return status;

  if (fork->being_used > 0){
    fork->waiting++;
    
    while (fork->being_used >0)
    {
      status = pthread_cond_wait(&fork->free, &fork->mutex);
      if (status != 0) break;
    }
    
    fork->waiting--;
  }
  
  if (status == 0){
    fork->being_used = 1;
  }   

  status2 = pthread_mutex_unlock(&fork->mutex);  

  return (status == 0)? status2 : status;
}


int fork_putdown(fork_t * fork)
{
  int status, status2;
  
  if (fork == NULL || fork->valid != FORK_VALID) return EINVAL;

  status = pthread_mutex_lock(&fork->mutex);
  if (status != 0) return status;

  fork->being_used = 0;

  if (fork->waiting > 0){
    status = pthread_cond_signal(&fork->free);
  }

  status2 = pthread_mutex_unlock(&fork->mutex);
  
  return (status== 0)? status2 : status;
}



void * philosopher_thread(void *arg)
{
  philosopher_t * philosopher = (philosopher_t*) arg;
  
  int odd_id = philosopher->id & 1;
  int status;
  int num = 0; 

  while ( num < NUM_ITERATIONS )
  {
    num++;
    if (odd_id)
    {
      philosopher->state = states[0];
      status = fork_get(philosopher->left);
      printf("%d: get left\n", philosopher->id);
      fflush (stdout);
      
      philosopher->state = states[1];
      status= fork_get(philosopher->right);
      printf("%d: get right\n", philosopher->id);
      fflush (stdout);

      philosopher->state = states[2];      
      philosopher->state = states[3];
    }
    else
    {
      philosopher->state = states[0];
      status= fork_get(philosopher->right);
      philosopher->state = states[1];
      printf("%d: get right\n", philosopher->id);
      fflush (stdout);

      status = fork_get(philosopher->left);
      printf("%d: get left\n", philosopher->id);
      fflush (stdout);
      philosopher->state = states[2];
      philosopher->state = states[3];

    }
    status = fork_putdown(philosopher->left);
    philosopher->state = states[4];
    printf("%d: put left\n", philosopher->id);
    fflush (stdout);

    status = fork_putdown(philosopher->right);
    philosopher->state = states[5];
    printf("%d: put right\n", philosopher->id);
    fflush (stdout);
  }

}


int main(int argc, char * argv)
{
  fork_t          forks[PHILOSOPHERS_SUM];
  philosopher_t   philosophers[PHILOSOPHERS_SUM];

  int i, status;

  for (i = 0; i < FORKS_SUM; i++)
    fork_init(&forks[i]);
    
  for (i = 0; i < PHILOSOPHERS_SUM; i++)
  {
    philosophers[i].id    = i;
    philosophers[i].state = states[0];
    philosophers[i].left  = &forks[i];
    philosophers[i].right  = 
      (i!= PHILOSOPHERS_SUM -1)? &forks[i+1] : &forks[0];
    philosophers[i].state = states[0];
  }

  for (i = 0; i < PHILOSOPHERS_SUM; i++)
    status = pthread_create(&philosophers[i].tid, NULL, 
		           philosopher_thread, &philosophers[i]);
  
  for (i = 0; i < PHILOSOPHERS_SUM; i++)
    status = pthread_join(philosophers[i].tid, NULL);

  for (i = 0; i < FORKS_SUM; i++)
    fork_destroy(&forks[i]);

  return 0;
}


