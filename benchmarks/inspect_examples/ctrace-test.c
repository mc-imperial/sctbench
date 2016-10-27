//args:2

#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define _STDOUT		stdout
#define _STDERR		stderr
#define _STDIN		stdin

typedef unsigned int uint_t;
typedef const char cchar_t;
typedef pthread_t tid_t;

/* --------------------------------------------------------------- */
/* tracing enabled flag typedef/values */
#define TRC_DISABLED	0
#define TRC_ENABLED	1

typedef uint_t tenable_t;
/* --------------------------------------------------------------- */


/* --------------------------------------------------------------- */
/* tracing turned on flag typedef/values */
#define TRC_OFF		0
#define TRC_ON		1

typedef uint_t ton_t;
/* --------------------------------------------------------------- */


/* --------------------------------------------------------------- */
/* tracing levels typedef/values */
#define TRC_NONE	0
#define TRC0		1	
#define TRC1		2	
#define TRC2		4
#define TRC3		8
#define TRC4		16
#define TRC5		32
#define TRC_ERR		64	
#define TRC_ALL		TRC0|TRC1|TRC2|TRC3|TRC4|TRC5|TRC_ERR

typedef uint_t tlevel_t;
/* --------------------------------------------------------------- */


/* --------------------------------------------------------------- */
/* separate trace thread flag typedef/values */
#define TRC_SERV_OFF	0
#define TRC_SERV_ON	1

typedef uint_t tserver_t;
/* --------------------------------------------------------------- */


/* --------------------------------------------------------------- */
/* tracing actions typedef/values */
#define TRCA_ENTER		0	
#define TRCA_RETURN		1	
#define TRCA_VOID_RETURN	2	
#define TRCA_PRINT		3	
#define TRCA_ERROR		4	

typedef uint_t taction_t;
/* --------------------------------------------------------------- */

extern tenable_t _trc; 
//extern tlevel_t _tlevel;

extern int trc_init(cchar_t *file, tenable_t trc, ton_t on, tlevel_t level, uint_t umax, tserver_t server);
extern void trc_end();
extern int trc_file(cchar_t *file);
extern int trc_add_thread(cchar_t *tname, tid_t id);
extern int trc_remove_thread(tid_t id);
extern void trc_turn_on();
extern void trc_turn_off();
extern int trc_turn_thread_on(tid_t id);
extern int trc_turn_thread_off(tid_t id);
extern int trc_turn_unit_on(uint_t i);
extern int trc_turn_unit_off(uint_t i);
extern int trc_set_level(tlevel_t l);
extern int trc_add_level(tlevel_t l);
extern int trc_remove_level(tlevel_t l);
extern int trc_set_thread_level(tlevel_t l, tid_t id);
extern int trc_add_thread_level(tlevel_t l, tid_t id);
extern int trc_remove_thread_level(tlevel_t l, tid_t id);
extern int trc_set_unit_level(uint_t i, tlevel_t level);
extern int trc_add_unit_level(uint_t i, tlevel_t level);
extern int trc_remove_unit_level(uint_t i, tlevel_t level);
extern char *trc_varargs(const char  *fmt, ...);
extern void trc_trace(taction_t a, uint_t i, tlevel_t tlevel, cchar_t *file,uint_t line, cchar_t *fn, cchar_t *kword, cchar_t *s);
extern void trc_state();
extern void trc_print_threads();


/* ----------------------------------------------------------------------
   TRACE MACROS 
   --------------------------------------------------------------------*/

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_INIT_DEFAULT()	// macro name
   desc: initialise trace library with default values
   --------------------------------------------------------------------*/
#define TRC_INIT_DEFAULT() trc_init(_STDOUT, TRC_DISABLED, TRC_ON, TRC_ERR,  100, 0)

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_INIT(		// macro name
   cchar_t *a,		// output stream(file, stdout, stderr), NULL=stdout  
   tenable_t b,		// trace enabled flag
   ton_t c,		// trace on flag
   tlevel_t d,		// trace levels to default to
   uint_t e,		// maximum number of logical software units
   tserver_t f)		// if server=1 run trace as separate server(thread)
   desc: initialise trace library
   --------------------------------------------------------------------*/
#define TRC_INIT(a,b,c,d,e,f) trc_init(a, b, c, d, e, f)


/* ----------------------------------------------------------------------
   void			// 0=success, 1=failure
   TRC_END()		// macro name
   desc: cleanup trace library. must be called by final sole running thread
   --------------------------------------------------------------------*/
#define TRC_END() trc_end()


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_FILE(		// macro name
   cchar_t *a)		// output stream(file, stdout, stderr), NULL=stdout  
   desc: new output stream for trace library becomes <a>
   --------------------------------------------------------------------*/
#define TRC_FILE(a) trc_file(a)


/* ----------------------------------------------------------------------
   void			// procedure 
   TRC_ENABLE()		// procedure name
   desc: enable tracing 
   --------------------------------------------------------------------*/
#define TRC_ENABLE() \
	_trc = TRC_ENABLED


/* ----------------------------------------------------------------------
   void			// procedure 
   TRC_DISABLE()	// procedure name
   desc: disable tracing 
   --------------------------------------------------------------------*/
#define TRC_DISABLE() \
	_trc = TRC_DISABLED


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_ADD_THREAD(	// macro name
   cchar_t *a,		// trace output thread name 
   tid_t b)		// thread id
   desc: add thread with display name <a> and id <b> to trace library
   if(b == 0) thread defaults to the caller of this macro
   --------------------------------------------------------------------*/
#define TRC_ADD_THREAD(a,b) trc_add_thread(a,b)


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_REMOVE_THREAD(	// macro name
   tid_t a)		// thread id
   desc: remove thread with id <a> from trace library
   if(a == 0) thread defaults to the caller of this macro
   --------------------------------------------------------------------*/
#define TRC_REMOVE_THREAD(a) trc_remove_thread(a)


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_TURN_ON()	// macro name
   desc: turn tracing on for all threads 
   --------------------------------------------------------------------*/
#define TRC_TURN_ON() trc_turn_on()


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_TURN_OFF()	// macro name
   desc: turn tracing off for all threads 
   --------------------------------------------------------------------*/
#define TRC_TURN_OFF() trc_turn_off()


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_TURN_THREAD_ON(	// macro name
   tid_t a)		// thread id
   desc: turn tracing on for thread with id <a> 
   if(a == 0) thread defaults to the caller of this macro
   --------------------------------------------------------------------*/
#define TRC_TURN_THREAD_ON(a) trc_turn_thread_on(a)


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_TURN_THREAD_OFF(	// macro name
   tid_t a)		// thread id
   desc: turn tracing off for thread with id <a> 
   if(a == 0) thread defaults to the caller of this macro
   --------------------------------------------------------------------*/
#define TRC_TURN_THREAD_OFF(a) trc_turn_thread_off(a)

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_TURN_UNIT_ON(	// macro name
   tunit_t a)		// tunit_t 
   desc: turn tracing on for tunit_t <a> 
   --------------------------------------------------------------------*/
#define TRC_TURN_UNIT_ON(a) trc_turn_unit_on(a)


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_TURN_UNIT_OFF(	// macro name
   tunit_t a)		// tunit_t 
   desc: turn tracing off for tunit_t <a> 
   --------------------------------------------------------------------*/
#define TRC_TURN_UNIT_OFF(a) trc_turn_unit_off(a)


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_SET_LEVEL(	// macro name
   tlevel_t a)		// trace level(s)
   desc: set level=<a> in all added threads 
   --------------------------------------------------------------------*/
#define TRC_SET_LEVEL(a) trc_set_level(a)


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_ADD_LEVEL(	// function name
   tlevel_t a)		// trace level(s)
   desc: add level <a> to all added threads 
   --------------------------------------------------------------------*/
#define TRC_ADD_LEVEL(a) trc_add_level(a)


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_REMOVE_LEVEL(	// function name
   tlevel_t a)		// trace level(s)
   desc: remove level <a> from all added threads 
   --------------------------------------------------------------------*/
#define TRC_REMOVE_LEVEL(a) trc_remove_level(a)


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_SET_THREAD_LEVEL(// function name
   tlevel_t a		// trace level(s)
   tid_t b)		// thread id
   desc: set level=<a> in thread <b>
   --------------------------------------------------------------------*/
#define TRC_SET_THREAD_LEVEL(a, b) trc_set_thread_level(a, b)


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_ADD_THREAD_LEVEL(	// function name
   tlevel_t a		// trace level(s)
   tid_t b)		// thread id
   desc: add level <a> to thread <b>
   --------------------------------------------------------------------*/
#define TRC_ADD_THREAD_LEVEL(a, b) trc_add_thread_level(a, b)


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_REMOVE_THREAD_LEVEL(	// function name
   tlevel_t a		// trace level(s)
   tid_t b)		// thread id
   desc: remove level <a> from thread <b>
   --------------------------------------------------------------------*/
#define TRC_REMOVE_THREAD_LEVEL(a, b) trc_remove_thread_level(a, b)


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_SET_UNIT_LEVEL(// function name
   tunit_t a		// tunit_t
   tlevel_t b)		// trace level(s) 
   desc: set level=<b> in tunit_t <a>
   --------------------------------------------------------------------*/
#define TRC_SET_UNIT_LEVEL(a, b) trc_set_unit_level(a, b)


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_ADD_UNIT_LEVEL(	// function name
   tunit_t a		// trace unit_t
   tlevel_t b)		// trace level(s) 
   desc: add level <b> to tunit_t <a>
   --------------------------------------------------------------------*/
#define TRC_ADD_UNIT_LEVEL(a, b) trc_add_unit_level(a, b)


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   TRC_REMOVE_UNIT_LEVEL(	// function name
   tunit_t a		// trace unit_t
   tlevel_t b)		// trace level(s) 
   desc: remove level <b> from tunit_t <a>
   --------------------------------------------------------------------*/
#define TRC_REMOVE_UNIT_LEVEL(a, b) trc_remove_unit_level(a, b)



/* ----------------------------------------------------------------------
   void			// procedure 
   TRC_ENTER(		// macro name
   tunit_t a,		// unit trace flag for enabling unit trace 
   tlevel_t b,		// trace level
   cchar_t *c)		// formatted vararglist of fn argument values 
   desc: write trace enter function line 
   argument d takes form of ("%d, %s\n", arg1, arg2)
   eg: TRC_ENTER(tunit, TRC0, "main", ("%d, %s", 1, "string"));
   --------------------------------------------------------------------*/
#define TRC_ENTER(a,b,c) \
	if(_trc) \
		trc_trace(TRCA_ENTER, a, b, __FILE__, __LINE__, __FUNCTION__, NULL, trc_varargs c)

/* ----------------------------------------------------------------------
   void			// procedure 
   TRC_RETURN(		// macro name
   tunit_t a,		// unit trace flag for enabling unit trace 
   tlevel_t b,		// trace level
   cchar_t *c,		// formatted arglist of fn return value 
   void *d)		// return value 
   desc: write trace return from function line 
   this macro also performs function return
   argument d takes form of ("%d\n", arg)
   eg: TRC_RETURN(tunit, TRC0, NULL, ("0x%x", NULL)); 
   --------------------------------------------------------------------*/
#define TRC_RETURN(a,b,c,d){ \
	if(_trc) \
		trc_trace(TRCA_RETURN, a, b, __FILE__, __LINE__, __FUNCTION__, NULL, trc_varargs c); \
   	return(d); \
}


/* ----------------------------------------------------------------------
   void			// procedure 
   TRC_VOID_RETURN(	// macro name
   tunit_t a,		// unit trace flag for enabling unit trace 
   tlevel_t b)		// trace level
   desc: write trace return procedure line 
   this macro also performs procedure return
   eg: TRC_VOID_RETURN(tunit, TRC0)
   --------------------------------------------------------------------*/
#define TRC_VOID_RETURN(a,b){ \
	if(_trc) \
		trc_trace(TRCA_VOID_RETURN, a, b, __FILE__, __LINE__, __FUNCTION__, NULL, NULL); \
	return; \
}


/* ----------------------------------------------------------------------
   void			// procedure 
   TRC_PRINT(		// macro name
   tunit_t a,		// unit trace flag for enabling unit trace 
   tlevel_t b,		// trace level
   cchar_t *c)		// formatted arglist  
   desc: write trace print line 
   argument c takes form of ("val=%d, string=%s\n", arg1, arg2)
   eg: TRC_PRINT(tunit, TRC0, ("loopvals: val=%d, string=%s", i, str)); 
   --------------------------------------------------------------------*/
#define TRC_PRINT(a,b,c) \
	if(_trc) \
		trc_trace(TRCA_PRINT, a, b, __FILE__, __LINE__, __FUNCTION__, NULL,trc_varargs c)


/* ----------------------------------------------------------------------
   void			// procedure 
   TRC_ERROR(		// macro name
   tunit_t a,		// unit trace flag for enabling unit trace 
   cchar_t *b)		// formatted arglist  
   desc: write trace error line 
   argument d takes form of ("val=%d, string=%s\n", arg1, arg2)
   eg: TRC_ERROR(tunit, TRC0, "loopvals", ("val=%d, string=%s", i, str)); 
   --------------------------------------------------------------------*/
#define TRC_ERROR(a,b) \
	if(_trc) \
		trc_trace(TRCA_ERROR, a, TRC_ERR, __FILE__, __LINE__, __FUNCTION__, NULL, trc_varargs b)


/* debugging purposes only */
#define TRC_PRINT_THREADS() trc_print_threads()
#define TRC_STATE() trc_state()




#define TRC_PORT 2013

#define SYSFILE(a) ((a == _STDOUT) || (a == _STDERR))  

tenable_t _trc = TRC_DISABLED;		/* trace enabled ? */
static ton_t _on = TRC_OFF;		/* trace on */	
static tlevel_t _tlevel = TRC_NONE;	/* trace levels enabled */
static FILE *_fp = NULL;		/* trc msgs output file stream */
static tserver_t  _server = 0;		/* run trace library as separate thread */
static int _msgs = 0;			/* msgs still being processed by server */
static pthread_t _sid;			/* server thread id */
static int _serv_sockfd;		/* server socket */
static sem_t _startclient;		/* dont start client until server inited */
static int _initialised = 0;		/* static trace library initialised */


/* trace field sizes */ 

#define T1	30		/* file name 	*/
#define T2	4		/* line number  */
#define T3	30		/* thread name  */
#define T4	30		/* keyword max 	*/
#define T5	1024		/* info max 	*/
#define TRACELEN (T1 + T2 + T3 + T4 + T5) 

/* indented trace fields start at WSPACEMAX + 1 */
#define WSPACEMAX (T1 + T2 + T3)	
#define WSPACELEN(a) WSPACEMAX - a


/* dont create thread data buffers on the fly */
/* instead keep them permanently in memory as */
/* part of the tthread_t struct */

/* trace thread type */
typedef struct tthread_t{
	tid_t id;
	char name[T3];	
	int on;
	tlevel_t level;
	char wspace[WSPACEMAX];	
	char fmt[T5];
	char trace[TRACELEN];
	struct tthread_t *next;	
} tthread_t;

#define THRDMAX	307	/* keep this prime - used in hashing fn */ 
#define HASH(a)  abs(a % THRDMAX)

static tthread_t *_thread[THRDMAX]; 	/* trace thread hash lookup table */
static uint_t _numthreads = 0;	
static pthread_mutex_t _hashmutex;	/* for _thread hash table integrity */
static int _hashreads = 0;
static sem_t _hashsem;

/* trace unit type */
typedef struct tunit_t{
	int on;
	tlevel_t level;
}tunit_t;

#define UNITMAX 1000
static tunit_t *_unit = NULL;		/* trace unit array */
static uint_t _unitmax = 0;		/* size of trace unit array */

/* write has exclusive use of hash table */
/* muliple reads can occur concurrently */
#define HASH_READ_ENTER() \
	pthread_mutex_lock(&_hashmutex); \
	if(!_hashreads) \
		sem_wait(&_hashsem); \
	_hashreads++; \
	pthread_mutex_unlock(&_hashmutex);

#define HASH_READ_EXIT() \
	pthread_mutex_lock(&_hashmutex); \
	--_hashreads; \
	if(!_hashreads) \
		sem_post(&_hashsem); \
	pthread_mutex_unlock(&_hashmutex);

#define HASH_WRITE_ENTER() sem_wait(&_hashsem)
#define HASH_WRITE_EXIT() sem_post(&_hashsem)


/* functions requiring forward delcarations */
int trc_file(cchar_t *file);	
void *trc_start_server(void);
int trc_stop_server();

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   trc_init(		// function name
   cchar_t *file,	// output stream(file, stdout, stderr), NULL=stdout  
   tenable_t trc,	// trace enabled flag
   ton_t on,		// trace on flag
   tlevel_t level,	// trace levels to default to
   tserver_t server)	// if server=1 run trace as separate server
   --------------------------------------------------------------------*/
int trc_init(cchar_t *file, tenable_t trc, ton_t on, tlevel_t level, uint_t umax, tserver_t server)
{
	int i, ret = 0;

	if(_initialised) return 1;

	_trc = trc;
	_on = on;
	_tlevel = level;
	_server = server;

	if(trc_file(file))
		return 1;
	
	for(i=0; i<THRDMAX; i++)
		_thread[i] = NULL;

	/* initialise logical software unit array */
	if(umax > 0){
		_unitmax = umax;
		_unit = (tunit_t *)malloc(sizeof(tunit_t)*_unitmax);
		for(i=0; i<_unitmax; i++){
			_unit[i].on = TRC_OFF;
			_unit[i].level = TRC_ALL;
		}
	}

	if(pthread_mutex_init(&_hashmutex, NULL))
		return 1;

	sem_init(&_hashsem, 0, 1);
	sem_init(&_startclient, 0, 1);

	/* run trace as separate thread */
	if(_server == TRC_SERV_ON){
		sem_wait(&_startclient);
		if(pthread_create(&_sid, NULL, trc_start_server, NULL)){
			trc_end();
			return 1;	
		}
		trc_start_client();
	}

	_initialised = 1;

	return 0;
}

/* ----------------------------------------------------------------------
   void			// procedure 
   trc_end()		// procedure name
   --------------------------------------------------------------------*/
void trc_end()
{
	int i;
	tthread_t *cur, *next;

	_trc = TRC_DISABLED;

	if(_server){
		trc_stop_server();
		pthread_join(_sid, NULL);
		trc_stop_client();
	}

	for(i=0; i<THRDMAX; i++){
		for(cur=_thread[i]; cur !=NULL; cur = next){
			next = cur->next;
			free(cur);
		}
	}

	pthread_mutex_destroy(&_hashmutex);
	sem_destroy(&_hashsem);
	sem_destroy(&_startclient);

	if(!SYSFILE(_fp))
		fclose(_fp);

	_initialised = 0;
}


/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   trc_file(		// function name
   cchar_t *file)	// output stream(file, stdout, stderr), NULL=stdout  
   --------------------------------------------------------------------*/
int trc_file(cchar_t *file)
{
	if(_fp && !SYSFILE(_fp))
		fclose(_fp);
	
	if(!file)
		_fp = _STDOUT;
	else if(!(_fp = fopen(file, "w")))
		return 1; 

	return 0;
}

/* ----------------------------------------------------------------------
   tthread_t *		// thread struct of global data   
   trc_thread(		// function name
   tid_t id)		// thread id for index into _threads hash table 
   --------------------------------------------------------------------*/
tthread_t *trc_thread(tid_t id)
{
	tthread_t *t = NULL;
	int index;
	
	index = HASH((int)id);
	t =  _thread[index];

	/* if thread already added return thread */
	for(t=_thread[index]; t!=NULL; t=t->next)
		if(t->id == id)
			break;
	return t; 
}

/* ----------------------------------------------------------------------
   void			// procedure 
   trc_print()		// procedure name
   desc: Print all current added threads, for debugging purposes.
   --------------------------------------------------------------------*/
void trc_print_threads()
{
	int i;
	tthread_t *t = NULL;

	HASH_WRITE_ENTER();
	for(i=0; i<THRDMAX; i++)
		for(t=_thread[i]; _thread[i] && t!=NULL; t=t->next)
			printf("t->id: %d\t t->name: %s\t t->level: %d\tt->on: %d\n", 
				t->id, t->name, t->level, t->on);
	HASH_WRITE_EXIT();
}

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   trc_add_thread(	// function name
   cchar_t *tname,	// trace output thread name 
   tid_t id)		// thread id
   desc: add thread struct to _thread hash table for thread with <id>
   --------------------------------------------------------------------*/
int trc_add_thread(cchar_t *tname, tid_t id)
{
	uint_t i;
	tthread_t *t = NULL;

	if(id == 0)
		id = pthread_self();

	HASH_WRITE_ENTER();
	if(t = trc_thread(id)){
		//strncpy(t->name, tname, T3);
		if(tname)
			sprintf(t->name, "%s", tname);
		else
			sprintf(t->name, "%d", id);
	}
	else{

		t = (tthread_t *)malloc(sizeof(tthread_t));
		if(!t){
			HASH_WRITE_EXIT();
			return 1;
		}

		i = HASH((int)id);
		t->id = id;
		//strncpy(t->name, tname, T3);
		if(tname)
			sprintf(t->name, "%s", tname);
		else
			sprintf(t->name, "%d", id);
		t->on = _on; 
		t->level = _tlevel;
		memset(t->wspace, ' ', WSPACEMAX);

		t->next = _thread[i];
		_thread[i] = t;
		_numthreads++;
	}
	HASH_WRITE_EXIT();

	return 0;
}

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   trc_remove_thread(	// function name
   tid_t id)		// thread id
   desc: remove thread struct from _thread hash table for thread with <id>
   --------------------------------------------------------------------*/
int trc_remove_thread(tid_t id)
{
	int i, ret = 1;
	tthread_t *cur, *next;

	if(id == 0)
		id = pthread_self();
	
	HASH_WRITE_ENTER();
	i = HASH((int)id);

	// if head of list is target, remove head
	if(_thread[i] && _thread[i]->id == id){
		cur = _thread[i];
		_thread[i] = cur->next;
		free(cur);
		_numthreads--;
		ret = 0;
	}
	// else we need to find and remove target
	else{
		for(cur=_thread[i]; cur!=NULL; cur=cur->next){
			next = cur->next;
			if(next && (next->id == id)){
				cur->next = next->next;
				free(next);
				_numthreads--;
				ret = 0;
			}
		}
	}
	HASH_WRITE_EXIT();

	return ret;
}

/* ----------------------------------------------------------------------
   void			// procedure 
   trc_turn_on()	// procedure name
   desc: turn tracing on for all added threads 
   --------------------------------------------------------------------*/
void trc_turn_on()
{
	uint_t i;
	tthread_t *t = NULL;

	HASH_WRITE_ENTER();
	_on = TRC_ON;
	for(i=0; i<THRDMAX; i++)
		for(t=_thread[i]; _thread[i] && t!=NULL; t=t->next)
			t->on = 1;
	HASH_WRITE_EXIT();
}

/* ----------------------------------------------------------------------
   void			// procedure 
   trc_turn_off()	// procedure name
   desc: turn tracing on for all added threads 
   --------------------------------------------------------------------*/
void trc_turn_off()
{
	uint_t i;
	tthread_t *t = NULL;

	HASH_WRITE_ENTER();
	_on = TRC_OFF;
	for(i=0; i<THRDMAX; i++)
		for(t=_thread[i]; _thread[i] && t!=NULL; t=t->next)
			t->on = 0;
	HASH_WRITE_EXIT();
}

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   trc_turn_thread_on(	// function name
   tid_t id)		// thread id
   desc: turn tracing on for thread <id> 
   --------------------------------------------------------------------*/
int trc_turn_thread_on(tid_t id)
{
	int ret = 1;
	tthread_t *t = NULL;

	if(id == 0)
		id = pthread_self();

	HASH_READ_ENTER();
	if(t = trc_thread(id)){
		t->on = TRC_ON;
		ret = 0;
	}
	HASH_READ_EXIT();
	
	return ret;
}

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   trc_turn_thread_off(	// function name
   tid_t id)		// thread id
   desc: turn tracing off for thread <id> 
   --------------------------------------------------------------------*/
int trc_turn_thread_off(tid_t id)
{
	int ret = 1;
	tthread_t *t = NULL;

	if(id == 0)
		id = pthread_self();

	HASH_READ_ENTER();
	if(t = trc_thread(id)){
		t->on = TRC_OFF;
		ret = 0;
	}
	HASH_READ_EXIT();
	
	return ret;
}

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure 
   trc_turn_unit_on(	// procedure name
   tunit_t *tl)	// unit software section variable
   desc: turn tracing on for unit section of software 
   --------------------------------------------------------------------*/
int trc_turn_unit_on(uint_t i)
{
	int ret = 1;

	if(_unitmax == 0 || i<0 || i>=_unitmax)
		return ret;

	/* this wont happen often so just reuse hash table locks */
	HASH_WRITE_ENTER();
	if(i < UNITMAX){
		_unit[i].on = TRC_ON;
		ret = 0;
	}
	HASH_WRITE_EXIT();

	return ret;
}

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure 
   trc_turn_unit_off(	// procedure name
   tunit_t *tl)	// unit software section variable
   desc: turn tracing off for unit section of software 
   --------------------------------------------------------------------*/
int trc_turn_unit_off(uint_t i)
{
	int ret = 1;

	if(_unitmax == 0 || i<0 || i>=_unitmax)
		return ret;

	/* this wont happen often so just reuse hash table locks */
	HASH_WRITE_ENTER();
	if(i < UNITMAX){
		_unit[i].on = TRC_OFF;
		ret = 0;
	}
	HASH_WRITE_EXIT();

	return ret;
}

/* ----------------------------------------------------------------------
   int			// 0=false, 1=true
   trc_valid_level(	// function name
   tlevel_t tlevel)	// trace level(s) 
   desc: validate tlevel 
   --------------------------------------------------------------------*/
int trc_valid_level(tlevel_t tlevel)
{
	return ((tlevel & ~(TRC_ALL)) == 0) ;
}

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   trc_set_level(	// function name
   tlevel_t tlevel)	// trace level(s)
   desc: set level=<level> in all added threads 
   --------------------------------------------------------------------*/
int trc_set_level(tlevel_t level)
{
	uint_t i;
	tthread_t *t = NULL;

	if(!trc_valid_level(level))
		return 1;

	HASH_WRITE_ENTER();
	_tlevel = level;
	for(i=0; i<THRDMAX; i++)
		for(t=_thread[i]; _thread[i] && t!=NULL; t=t->next)
			t->level = level;
	HASH_WRITE_EXIT();

	return 0;
}

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   trc_add_level(	// function name
   tlevel_t tlevel)	// trace level(s)
   desc: add <level> to all added threads 
   --------------------------------------------------------------------*/
int trc_add_level(tlevel_t level)
{
	uint_t i;
	tthread_t *t = NULL;

	if(!trc_valid_level(level)) return 1;

	HASH_WRITE_ENTER();
	_tlevel |= level;
	for(i=0; i<THRDMAX; i++)
		for(t=_thread[i]; _thread[i] && t!=NULL; t=t->next)
			t->level |= level;
	HASH_WRITE_EXIT();

	return 0;
}

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   trc_remove_level(	// function name
   tlevel_t tlevel)	// trace level(s)
   desc: remove <level> in all added threads 
   --------------------------------------------------------------------*/
int trc_remove_level(tlevel_t level)
{
	uint_t i;
	tthread_t *t = NULL;

	if(!trc_valid_level(level)) return 1;

	HASH_WRITE_ENTER();
	_tlevel &= ~level;
	for(i=0; i<THRDMAX; i++)
		for(t=_thread[i]; _thread[i] && t!=NULL; t=t->next)
			t->level &= ~level;
	HASH_WRITE_EXIT();

	return 0;
}

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   trc_set_thread_level(	// function name
   tlevel_t tlevel	// trace level(s)
   tid_t id)		// thread id
   desc: set level=<level> in thread <id>
   --------------------------------------------------------------------*/
int trc_set_thread_level(tlevel_t level, tid_t id)
{
	int ret = 1;
	tthread_t *t = NULL;

	if(id == 0)
		id = pthread_self();

	HASH_WRITE_ENTER();
	if((t = trc_thread(id)) && trc_valid_level(level)){
		t->level = level;
		ret = 0;
	}
	HASH_WRITE_EXIT();

	return ret;
}

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   trc_add_thread_level(	// function name
   tlevel_t tlevel	// trace level(s)
   tid_t id)		// thread id
   desc: add <level> to thread <id>
   --------------------------------------------------------------------*/
int trc_add_thread_level(tlevel_t level, tid_t id)
{
	int ret = 1;
	tthread_t *t = NULL;

	if(id == 0)
		id = pthread_self();

	HASH_WRITE_ENTER();
	if((t = trc_thread(id)) && trc_valid_level(level)){
		t->level |= level;
		ret = 0;
	}
	HASH_WRITE_EXIT();

	return ret;
}

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   trc_remove_thread_level(	// function name
   tlevel_t tlevel	// trace level(s)
   tid_t id)		// thread id
   desc: remove <level> from thread <id>
   --------------------------------------------------------------------*/
int trc_remove_thread_level(tlevel_t level, tid_t id)
{
	int ret = 1;
	tthread_t *t = NULL;

	if(id == 0)
		id = pthread_self();

	HASH_WRITE_ENTER();
	if((t = trc_thread(id)) && trc_valid_level(level)){
		t->level &= ~level;
		ret = 0;
	}
	HASH_WRITE_EXIT();

	return ret;
}

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   trc_set_unit_level(	// function name
   tunit_t *tl		// trace level(s)
   tlevel_t level)	// tlevel_t 
   desc: set level=<level> in tunit_t <tl>
   --------------------------------------------------------------------*/
int trc_set_unit_level(uint_t i, tlevel_t level)
{
	int ret = 1;

	if(_unitmax == 0 || i<0 || i>=_unitmax)
		return ret;

	/* this wont happen often so just reuse hash table locks */
	HASH_WRITE_ENTER();
	if(i < UNITMAX && trc_valid_level(level)){
		_unit[i].level = level;
		ret = 0;
	}
	HASH_WRITE_EXIT();

	return ret;
}

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   trc_add_unit_level(	// function name
   tunit_t *tl		// trace level(s)
   tlevel_t level)	// tlevel_t 
   desc: add level=<level> in tunit_t <tl>
   --------------------------------------------------------------------*/
int trc_add_unit_level(uint_t i, tlevel_t level)
{
	int ret = 1;

	if(_unitmax == 0 || i<0 || i>=_unitmax)
		return ret;

	/* this wont happen often so just reuse hash table locks */
	HASH_WRITE_ENTER();
	if(i < UNITMAX  && trc_valid_level(level)){
		_unit[i].level |= level;
		ret = 0;
	}
	HASH_WRITE_EXIT();

	return ret;
}

/* ----------------------------------------------------------------------
   int			// 0=success, 1=failure
   trc_remove_unit_level(// function name
   tunit_t *tl		// trace level(s)
   tlevel_t level)	// tlevel_t 
   desc: remove level=<level> from tunit_t <tl>
   --------------------------------------------------------------------*/
int trc_remove_unit_level(uint_t i, tlevel_t level)
{
	int ret = 1;

	if(_unitmax == 0 || i<0 || i>=_unitmax)
		return ret;

	/* this wont happen often so just reuse hash table locks */
	HASH_WRITE_ENTER();
	if(i < UNITMAX && trc_valid_level(level)){
		_unit[i].level &= ~level;
		ret = 0;
	}
	HASH_WRITE_EXIT();

	return ret;
}


/* ----------------------------------------------------------------------
   void			// procedure 
   trc_state()		// procedure name
   desc: print trace state info for debugging purposes.
   --------------------------------------------------------------------*/
void trc_state()
{
	printf("_trc: %d\t_level: %d\t_t: %d\t", _trc, _tlevel, _server);
	if(_fp == _STDOUT)
		printf("_fp: _STDOUT");
	else if(_fp)
		printf("_fp: user file");
	else
		printf("_fp: NULL");
	printf("\n");
}

/* ----------------------------------------------------------------------
   char *		// string output of fmt/argslist combination 
   trc_varargs(		// function name
   const char *fmt,	// format
   ...)			// arguments embedded in <format>
   desc: convert fmt/arglist combination into string 
   --------------------------------------------------------------------*/
char *trc_varargs(const char *fmt, ...)
{
	va_list args;
	tthread_t *t = NULL;

	HASH_READ_ENTER();
	if(t = trc_thread(pthread_self())){
		if(fmt){
			va_start(args, fmt);
			vsprintf(t->fmt, fmt, args);
			va_end(args);
		}
		else
			t->fmt[0] = NULL;
	}
	HASH_READ_EXIT();

	return t? t->fmt: NULL;
}

/* ----------------------------------------------------------------------
   void			// procedure 
   trc_trace(		// procedure name
   taction_t a,		// action
   tlevel_t level,	// trace level
   cchar_t *file, 	// source file 
   uint_t line,		// source file line number 
   cchar_t *kword,	// keyword for trace info
   cchar_t *s)		// trace info
   desc: write trace line 
   --------------------------------------------------------------------*/
void trc_trace(taction_t a, uint_t i, tlevel_t level, cchar_t *file, uint_t line, cchar_t *fn, cchar_t *kword, cchar_t *s)
{
	int n;			/* length of non-indented fields */
	tthread_t *t;
	int num, bytesWritten;
	int unit, thread;
	time_t tim;
	struct tm *timeptr;
	char buf[64];

	HASH_READ_ENTER();
	
	/* check for valid output stream and this thread is registered */
	if(!_fp || !(t = trc_thread(pthread_self()))){
		HASH_READ_EXIT();
		return;
	}

	/* if unit or thread is on and has level set, do trace */
	unit = _unitmax>0 && i>=0 && i<_unitmax && _unit[i].on && ((_unit[i].level & level)!=0); 
	thread = t->on && ((t->level & level)!=0);
	if(!unit && !thread){
		HASH_READ_EXIT();
		return;
	}

	time(&tim);
	timeptr = localtime(&tim);
	strftime(buf, 64, "%D-%T", timeptr);

	/* generate trace output */
	sprintf(t->trace, "%s:%s:%d:%s:%n", buf, file, line, t->name, &n);
	t->wspace[WSPACELEN(n)] = '\0';	

	switch(a){
	case(TRCA_ENTER):
		sprintf(t->trace+n, "%senter %s(%s)\n", t->wspace, fn, s);
		break;
	case(TRCA_RETURN):
		sprintf(t->trace+n, "%sreturn %s(%s)\n", t->wspace, fn, s);
			break;
	case(TRCA_VOID_RETURN):
		sprintf(t->trace+n, "%s%s: return\n", t->wspace, fn);
			break;
	case(TRCA_PRINT):
		sprintf(t->trace+n, "%s%s\n", t->wspace, s);
			break;
	case(TRCA_ERROR):
		sprintf(t->trace+n, "%sERROR in fn %s: %s\n", t->wspace, fn, s);
			break;
	}
	
	t->wspace[WSPACELEN(n)] = ' ';	

	/* write trace output to _fd */
	if(_server){	
		num = 0;
		for(bytesWritten=0; bytesWritten<TRACELEN; bytesWritten+=num)
			num = write(_serv_sockfd, 
				(void*)((char *)t->trace + bytesWritten),
				 TRACELEN - bytesWritten);

		if(bytesWritten == TRACELEN){
			_msgs ++;
                    fprintf(stderr, "_msgs++  , _msgs= %d \n", _msgs);
                }
	}
	else{
		fprintf(_fp, "%s", t->trace);
		_msgs++;
	}
	HASH_READ_EXIT();
}

/* ----------------------------------------------------------------------
   void			// procedure 
   trc_start_client()	// procedure name
   --------------------------------------------------------------------*/
int trc_start_client()
{
	int nread;
	struct sockaddr_in serv_addr = {0};
	char name[1024];
	struct hostent *hostptr = NULL;

	sem_wait(&_startclient);

	/* create endpoint */
	if((_serv_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror(NULL);
		return 1;
	}

	gethostname(name, 1024);

	hostptr = gethostbyname(name);

	(void) memset(&serv_addr, 0, sizeof(serv_addr));
	(void) memcpy(&serv_addr.sin_addr, hostptr->h_addr, hostptr->h_length);

	/* connect to trace server */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(TRC_PORT);

	if(connect(_serv_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		printf("connect error\n");
		perror(NULL);
		return 1;
	}	

	return 0;
}

int trc_stop_client()
{
	close(_serv_sockfd);
}

/* ----------------------------------------------------------------------
   void			// procedure 
   trc_start_server()	// procedure name
   --------------------------------------------------------------------*/
void *trc_start_server(void)
{
	char buf[TRACELEN];
	int sockfd, client_sockfd;
	int status = 0, on =1;
	int nread, len;
	struct sockaddr_in serv_addr, client_addr;

	/* create endpoint */
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror(NULL);
		return NULL;
	}

	/* turn off bind address checking */
	status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
		(const char *)&on, sizeof(on));
	if(status == -1){
		perror("setsockopt(..., SO_REUSEADDR,...)");
		return NULL;
	}

	/* bind adress */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(TRC_PORT);

	if(bind(sockfd, &serv_addr, sizeof(serv_addr)) < 0){
		perror(NULL);
		return NULL;
	}

	/* specify queue */
	listen(sockfd, 1);

	/* server ready, initialise client */
	sem_post(&_startclient);
	
	len = sizeof(client_addr);
	client_sockfd = accept(sockfd, &client_addr, &len);

	while((_server && (client_sockfd != -1))|| _msgs){
		nread = 0;
		while((nread += read(client_sockfd, buf, TRACELEN)) < TRACELEN)
			;
		_msgs--;
		fprintf(stderr, "msg--  %d\n", _msgs);
		fprintf(_fp, "%s", buf);
	}
	close(client_sockfd);

	return NULL;
}

int trc_stop_server()
{
	return (_server = 0);
}





void *thread1(void *);
void *thread2(void *);

int main(int argc, char **argv)
{
  int x = (int) argv[1][0];
  
  printf("In main function\n");
  int i = 6;
  tid_t tid1, tid2;
  
  TRC_INIT(NULL, TRC_ENABLED, TRC_ON, TRC0, 0, TRC_SERV_OFF);
  TRC_ADD_THREAD(argv[0], 0);
  
  if(pthread_create(&tid1, NULL, thread1, &x))
    return 1;
  
  if(pthread_create(&tid2, NULL, thread2, &x))
    return 1;

  TRC_PRINT(0, TRC0, ("i=%d", 0)); //if(_trc) trc_trace
  TRC_ERROR(0,  ("mystring=%s", "stringval")); //if(trc) trc_trace
  
  
  pthread_join(tid1, NULL);
  TRC_REMOVE_THREAD(tid1);
  pthread_join(tid2, NULL);
  TRC_REMOVE_THREAD(tid2);
  TRC_RETURN(0, TRC0, ("%d", i), i); //if(_trc) trc_trace
  TRC_END();	
}

void *thread1(void *arg)
{
  printf("Executing trace end 1\n");
  TRC_END();
  return 0;
}

void *thread2(void *arg)
{
  if((*(int*)arg) == 'b'){
    printf("Executing trace end 2\n");
    TRC_END();
  }
  return 0;
}


