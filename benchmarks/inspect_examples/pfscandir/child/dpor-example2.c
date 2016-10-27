#include<stdlib.h>
#include<pthread.h>
#include<stdio.h>

#define NUMBLOCKS 26
#define NUMINODE 32
#define NUM_THREADS 26

pthread_mutex_t locki[NUMBLOCKS];
pthread_mutex_t lockb[NUMBLOCKS];
int busy[NUMBLOCKS];
int inode[NUMINODE];

pthread_t tids[NUM_THREADS];


void *thread_routine(void * arg)
{
  int i,b;
  int tid;

  tid = (int)arg;

  //printf(" Thread id: %d\n", tid);
  // getch();
  
  i = tid % NUMINODE;
  pthread_mutex_lock( &locki[i] );
  if( inode[i]==0 )
    {
      b = (i*2) % NUMBLOCKS;
      //printf("thread id: %d , I am here and making progress\n ", tid);
      while(1)
	{
	  pthread_mutex_lock( &lockb[b] );
	  if( !busy[b] )
	    {
	      busy[b] = 1;
	      inode[i] = b+1;
	      printf("  ");
	      pthread_mutex_unlock( &lockb[b] );
	      break;
	    }
	  pthread_mutex_unlock( &lockb[b] );
	  b = (b+1) % NUMBLOCKS;
	}
    }
  pthread_mutex_unlock( &locki[i] );
  pthread_exit(NULL);
}


int main()
{
  int i;
  for(i = 0; i < NUMBLOCKS;i++)
    {
      pthread_mutex_init(&locki[i],NULL);
      pthread_mutex_init(&lockb[i],NULL);
      busy[i] = 0;
    }

  for(i=0;i<NUM_THREADS;i++)
    pthread_create(&tids[i], NULL, thread_routine, (void *)(i));

  for(i=0;i<NUM_THREADS;i++)
    pthread_join(tids[i], NULL);

  for(i=0;i<NUMBLOCKS;i++)
    {
      pthread_mutex_destroy(&locki[i]);
      pthread_mutex_destroy(&lockb[i]);
    }

  return 0;
}
