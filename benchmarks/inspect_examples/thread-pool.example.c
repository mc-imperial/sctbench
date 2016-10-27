#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>

#ifndef TPBOOL
typedef int TPBOOL;
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define BUSY_THRESHOLD 0.5	//(busy thread)/(all thread threshold)
#define MANAGE_INTERVAL 1	//tp manage thread sleep interval

typedef struct tp_work_desc_s tp_work_desc;
typedef struct tp_work_s tp_work;
typedef struct tp_thread_info_s tp_thread_info;
typedef struct tp_thread_pool_s tp_thread_pool;

//thread parm
struct tp_work_desc_s{
	char *inum;	//call in
	char *onum;	//call out
	int chnum;	//channel num
};

//base thread struct
struct tp_work_s{
	//main process function. user interface
	void (*process_job)(tp_work *this, tp_work_desc *job);
};

//thread info
struct tp_thread_info_s{
	pthread_t		thread_id;	//thread id num
	TPBOOL  		is_busy;	//thread status:true-busy;flase-idle
	pthread_cond_t          thread_cond;	
	pthread_mutex_t		thread_lock;
	tp_work			*th_work;
	tp_work_desc		*th_job;
	int stop;
};

//main thread pool struct
struct tp_thread_pool_s{
	TPBOOL (*init)(tp_thread_pool *this);
	void (*close)(tp_thread_pool *this);
	void (*process_job)(tp_thread_pool *this, tp_work *worker, tp_work_desc *job);
	int  (*get_thread_by_id)(tp_thread_pool *this, int id);
	TPBOOL (*add_thread)(tp_thread_pool *this);
	TPBOOL (*delete_thread)(tp_thread_pool *this);
	int (*get_tp_status)(tp_thread_pool *this);
	
	int min_th_num;		//min thread number in the pool
	int cur_th_num;		//current thread number in the pool
	int max_th_num;         //max thread number in the pool
	pthread_mutex_t tp_lock;
	pthread_t manage_thread_id;	//manage thread id num
	int stop;
	tp_thread_info *thread_info;	//work thread relative thread info
};

tp_thread_pool *creat_thread_pool(int min_num, int max_num);


static void *tp_work_thread(void *pthread);
static void *tp_manage_thread(void *pthread);

static TPBOOL tp_init(tp_thread_pool *this);
static void tp_close(tp_thread_pool *this);
static void tp_process_job(tp_thread_pool *this, tp_work *worker, tp_work_desc *job);
static int  tp_get_thread_by_id(tp_thread_pool *this, int id);
static TPBOOL tp_add_thread(tp_thread_pool *this);
static TPBOOL tp_delete_thread(tp_thread_pool *this);
static int  tp_get_tp_status(tp_thread_pool *this);

/**
  * user interface. creat thread pool.
  * para:
  * 	num: min thread number to be created in the pool
  * return:
  * 	thread pool struct instance be created successfully
  */
tp_thread_pool *creat_thread_pool(int min_num, int max_num){
	tp_thread_pool *this;
	this = (tp_thread_pool*)malloc(sizeof(tp_thread_pool));	

	memset(this, 0, sizeof(tp_thread_pool));
	
	//init member function ponter
	this->init = tp_init;
	this->close = tp_close;
	this->process_job = tp_process_job;
	this->get_thread_by_id = tp_get_thread_by_id;
	this->add_thread = tp_add_thread;
	this->delete_thread = tp_delete_thread;
	this->get_tp_status = tp_get_tp_status;

	//init member var
	this->min_th_num = min_num;
	this->cur_th_num = this->min_th_num;
	this->max_th_num = max_num;
	pthread_mutex_init(&this->tp_lock, NULL);

	//malloc mem for num thread info struct
	if(NULL != this->thread_info)
		free(this->thread_info);
	this->thread_info = (tp_thread_info*)malloc(sizeof(tp_thread_info)*this->max_th_num);

	return this;
}


/**
  * member function reality. thread pool init function.
  * para:
  * 	this: thread pool struct instance ponter
  * return:
  * 	true: successful; false: failed
  */
TPBOOL tp_init(tp_thread_pool *this){
	int i;
	int err;
	
	//creat work thread and init work thread info
	for(i=0;i<this->min_th_num;i++){
		this->thread_info[i].stop=0;
		this->stop=0;
		pthread_cond_init(&this->thread_info[i].thread_cond, NULL);
		pthread_mutex_init(&this->thread_info[i].thread_lock, NULL);
		
		err = pthread_create(&this->thread_info[i].thread_id, NULL, tp_work_thread, this);
		if(0 != err){
			printf("tp_init: creat work thread failed\n");
			return FALSE;
		}
		printf("tp_init: creat work thread %d\n", this->thread_info[i].thread_id);
	}

	//creat manage thread
	err = pthread_create(&this->manage_thread_id, NULL, tp_manage_thread, this);
	if(0 != err){
		printf("tp_init: creat manage thread failed\n");
		return FALSE;
	}
	printf("tp_init: creat manage thread %d\n", this->manage_thread_id);

	return TRUE;
}

/**
  * member function reality. thread pool entirely close function.
  * para:
  * 	this: thread pool struct instance ponter
  * return:
  */
void tp_close(tp_thread_pool *this){
	int i;
	
	//close work thread
	for(i=0;i<this->cur_th_num;i++){
		//kill(this->thread_info[i].thread_id, SIGKILL);
		pthread_mutex_lock(&this->thread_info[i].thread_lock);
		this->thread_info[i].stop = 1;
		pthread_cond_signal(&this->thread_info[i].thread_cond);
		pthread_mutex_unlock(&this->thread_info[i].thread_lock);

		pthread_join(this->thread_info[i].thread_id, NULL);
		pthread_mutex_destroy(&this->thread_info[i].thread_lock);
		pthread_cond_destroy(&this->thread_info[i].thread_cond);
		printf("tp_close: kill work thread %d\n", this->thread_info[i].thread_id);
	}

	//close manage thread
	//kill(this->manage_thread_id, SIGKILL);
	this->stop = 1;
	pthread_join(this->manage_thread_id, NULL);
	
	pthread_mutex_destroy(&this->tp_lock);
	printf("tp_close: kill manage thread %d\n", this->manage_thread_id);
	
	//free thread struct
	free(this->thread_info);
}

/**
  * member function reality. main interface opened. 
  * after getting own worker and job, user may use the function to process the task.
  * para:
  * 	this: thread pool struct instance ponter
  *	worker: user task reality.
  *	job: user task para
  * return:
  */
void tp_process_job(tp_thread_pool *this, tp_work *worker, tp_work_desc *job){
	int i;
	int tmpid;

	//fill this->thread_info's relative work key
	for(i=0;i<this->cur_th_num;i++){
		pthread_mutex_lock(&this->thread_info[i].thread_lock);
		if(!this->thread_info[i].is_busy){
			printf("tp_process_job: %d thread idle, thread id is %d\n", i, this->thread_info[i].thread_id);
			//thread state be set busy before work
		  	this->thread_info[i].is_busy = TRUE;
			pthread_mutex_unlock(&this->thread_info[i].thread_lock);
			
			this->thread_info[i].th_work = worker;
			this->thread_info[i].th_job = job;
			
			printf("tp_process_job: informing idle working thread %d, thread id is %d\n", i, this->thread_info[i].thread_id);
			pthread_cond_signal(&this->thread_info[i].thread_cond);

			return;
		}
		else 
			pthread_mutex_unlock(&this->thread_info[i].thread_lock);		
	}//end of for

	//if all current thread are busy, new thread is created here
	pthread_mutex_lock(&this->tp_lock);
	if( this->add_thread(this) ){
		i = this->cur_th_num - 1;
		tmpid = this->thread_info[i].thread_id;
		this->thread_info[i].th_work = worker;
		this->thread_info[i].th_job = job;
	}
	pthread_mutex_unlock(&this->tp_lock);
	
	//send cond to work thread
	printf("tp_process_job: informing idle working thread %d, thread id is %d\n", i, this->thread_info[i].thread_id);
	pthread_cond_signal(&this->thread_info[i].thread_cond);
	return;	
}

/**
  * member function reality. get real thread by thread id num.
  * para:
  * 	this: thread pool struct instance ponter
  *	id: thread id num
  * return:
  * 	seq num in thread info struct array
  */
int tp_get_thread_by_id(tp_thread_pool *this, int id){
	int i;

	for(i=0;i<this->cur_th_num;i++){
		if(id == this->thread_info[i].thread_id)
			return i;
	}

	return -1;
}

/**
  * member function reality. add new thread into the pool.
  * para:
  * 	this: thread pool struct instance ponter
  * return:
  * 	true: successful; false: failed
  */
static TPBOOL tp_add_thread(tp_thread_pool *this){
	int err;
	tp_thread_info *new_thread;
	
	if( this->max_th_num <= this->cur_th_num )
		return FALSE;
		
	//malloc new thread info struct
	new_thread = &this->thread_info[this->cur_th_num];
	
	//init new thread's cond & mutex
	pthread_cond_init(&new_thread->thread_cond, NULL);
	pthread_mutex_init(&new_thread->thread_lock, NULL);

	//init status is busy
	new_thread->is_busy = TRUE;

	//add current thread number in the pool.
	this->cur_th_num++;
	
	err = pthread_create(&new_thread->thread_id, NULL, tp_work_thread, this);
	if(0 != err){
		free(new_thread);
		return FALSE;
	}
	printf("tp_add_thread: creat work thread %d\n", this->thread_info[this->cur_th_num-1].thread_id);
	
	return TRUE;
}

/**
  * member function reality. delete idle thread in the pool.
  * only delete last idle thread in the pool.
  * para:
  * 	this: thread pool struct instance ponter
  * return:
  * 	true: successful; false: failed
  */
static TPBOOL tp_delete_thread(tp_thread_pool *this){
	//current thread num can't < min thread num
	if(this->cur_th_num <= this->min_th_num) return FALSE;

	//if last thread is busy, do nothing
	if(this->thread_info[this->cur_th_num-1].is_busy) return FALSE;

	//kill the idle thread and free info struct
	//kill(this->thread_info[this->cur_th_num-1].thread_id, SIGKILL);
	pthread_mutex_lock(&this->thread_info[this->cur_th_num-1].thread_lock);
	this->thread_info[this->cur_th_num-1].stop = 1;
	pthread_cond_signal(&this->thread_info[this->cur_th_num-1].thread_cond);
	pthread_mutex_unlock(&this->thread_info[this->cur_th_num-1].thread_lock);

	pthread_join(this->thread_info[this->cur_th_num-1].thread_id, NULL);
	
	pthread_mutex_destroy(&this->thread_info[this->cur_th_num-1].thread_lock);
	pthread_cond_destroy(&this->thread_info[this->cur_th_num-1].thread_cond);

	//after deleting idle thread, current thread num -1
	this->cur_th_num--;

	return TRUE;
}

/**
  * member function reality. get current thread pool status:idle, normal, busy, .etc.
  * para:
  * 	this: thread pool struct instance ponter
  * return:
  * 	0: idle; 1: normal or busy(don't process)
  */
static int  tp_get_tp_status(tp_thread_pool *this){
	float busy_num = 0.0;
	int i;

	//get busy thread number
	for(i=0;i<this->cur_th_num;i++){
		if(this->thread_info[i].is_busy)
			busy_num++;
	}

	//0.2? or other num?
	if(busy_num/(this->cur_th_num) < BUSY_THRESHOLD)
		return 0;//idle status
	else
		return 1;//busy or normal status	
}

/**
  * internal interface. real work thread.
  * para:
  * 	pthread: thread pool struct ponter
  * return:
  */
static void *tp_work_thread(void *pthread){
	pthread_t curid;//current thread id
	int nseq;//current thread seq in the this->thread_info array
	tp_thread_pool *this = (tp_thread_pool*)pthread;//main thread pool struct instance

	//get current thread id
	curid = pthread_self();
	
	//get current thread's seq in the thread info struct array.
	nseq = this->get_thread_by_id(this, curid);
	if(nseq < 0)
		return;
	printf("entering working thread %d, thread id is %d\n", nseq, curid);

	//wait cond for processing real job.
	while( TRUE ){
		pthread_mutex_lock(&this->thread_info[nseq].thread_lock);
		if(!this->thread_info[nseq].stop) {
			pthread_cond_wait(&this->thread_info[nseq].thread_cond, &this->thread_info[nseq].thread_lock);
		}
		if(this->thread_info[nseq].stop) {
			pthread_exit(NULL);
		}
		pthread_mutex_unlock(&this->thread_info[nseq].thread_lock);		
		
		printf("%d thread do work!\n", pthread_self());

		tp_work *work = this->thread_info[nseq].th_work;
		tp_work_desc *job = this->thread_info[nseq].th_job;

		//process
		work->process_job(work, job);

		//thread state be set idle after work
		pthread_mutex_lock(&this->thread_info[nseq].thread_lock);		
		this->thread_info[nseq].is_busy = FALSE;
		pthread_mutex_unlock(&this->thread_info[nseq].thread_lock);
		
		printf("%d do work over\n", pthread_self());
	}	
}

/**
  * internal interface. manage thread pool to delete idle thread.
  * para:
  * 	pthread: thread pool struct ponter
  * return:
  */
static void *tp_manage_thread(void *pthread){
	tp_thread_pool *this = (tp_thread_pool*)pthread;//main thread pool struct instance

	//1?
	sleep(MANAGE_INTERVAL);
	
	do{
		if( this->get_tp_status(this) == 0 ){
			do{
				if( !this->delete_thread(this) )
					break;
				sleep(MANAGE_INTERVAL);
			}while(TRUE);
		}//end for if

		//1?
		sleep(MANAGE_INTERVAL);
	}while(!this->stop);
}
	


int main(int argc, char *argv[])
{
  tp_thread_pool  * pool;

  pool = creat_thread_pool(2, 4);
  tp_init(pool);
  tp_close(pool);

}
