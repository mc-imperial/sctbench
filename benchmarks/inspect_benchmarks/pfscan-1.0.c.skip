/*
 *  compile:  gcc -g -o pfscan -pthread  pfscan.comb.c 
 *    
 *  execute:  ./pfscan  -n 4  "test"  /home/yuyang/
 *
 */


/*
** pfscan.c - Parallell File Scanner
**
** Copyright (c) 2002 Peter Eriksson <pen@lysator.liu.se>
**
** This program is free software; you can redistribute it and/or
** modify it as you wish - as long as you don't claim that you wrote
** it.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <ftw.h>
#include <locale.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include <pthread.h>


typedef struct
{
    void **buf;
    int qsize;
    int occupied;
    int nextin;
    int nextout;
    int closed;
    pthread_mutex_t mtx;
    pthread_cond_t more;
    pthread_cond_t less;
} PQUEUE;


extern int pqueue_init(PQUEUE *bp,  int qsize);

extern void pqueue_destroy(PQUEUE *bp);

extern int pqueue_put(PQUEUE *bp, void *item);

extern int pqueue_get(PQUEUE *bp, void **item);

extern void pqueue_close(PQUEUE *bp);


#define BM_ASIZE 256 /* Allowed character code values */

typedef struct
{
    int *bmGs;
    int bmBc[BM_ASIZE];
    unsigned char *saved_x;
    int saved_m;
    int icase;
} BM;


int bm_init(BM *bmp, unsigned char *x, int m, int icase);

int bm_search(BM *bmp,  unsigned char *y, size_t n, int (*mfun)(unsigned char *buf, size_t n, size_t pos, void *misc), void *misc);

void bm_destroy(BM *bmp);


char version[] = "1.0";


int pqueue_init(PQUEUE *qp, int qsize)
{
  qp->buf = calloc(sizeof(void *), qsize);
  if (qp->buf == NULL) return NULL;
  
  qp->qsize = qsize;
  qp->occupied = 0;
  qp->nextin = 0;
  qp->nextout = 0;
  qp->closed = 0;

  pthread_mutex_init(&qp->mtx, NULL);
  pthread_cond_init(&qp->more, NULL);
  pthread_cond_init(&qp->less, NULL);
  return 0;
}



void pqueue_close(PQUEUE *qp)
{
    pthread_mutex_lock(&qp->mtx);

    qp->closed = 1;

    pthread_mutex_unlock(&qp->mtx);
    pthread_cond_broadcast(&qp->more);
}


int pqueue_put(PQUEUE *qp, void *item)
{
  pthread_mutex_lock(&qp->mtx);
  
  if (qp->closed)
    return 0;
    
  while (qp->occupied >= qp->qsize)
    pthread_cond_wait(&qp->less, &qp->mtx);
  
  qp->buf[qp->nextin++] = item;  
  qp->nextin %= qp->qsize;
  qp->occupied++;
  
  pthread_mutex_unlock(&qp->mtx);
  pthread_cond_signal(&qp->more);  
  return 1;
}



int pqueue_get(PQUEUE *qp, void **item)
{
  int got = 0;
    
  pthread_mutex_lock(&qp->mtx);
    
  while (qp->occupied <= 0 && !qp->closed)
    pthread_cond_wait(&qp->more, &qp->mtx);
  
  if (qp->occupied > 0){
    *item = qp->buf[qp->nextout++];
    qp->nextout %= qp->qsize;
    qp->occupied--;
    got = 1;
    
    pthread_mutex_unlock(&qp->mtx);
    pthread_cond_signal(&qp->less);
  }
  else
    pthread_mutex_unlock(&qp->mtx);
  
  return got;
}



void pqueue_destroy(PQUEUE *qp)
{
  pthread_mutex_destroy(&qp->mtx);
  pthread_cond_destroy(&qp->more);
  pthread_cond_destroy(&qp->less);
  free(qp->buf);
}



/***  bm.c */


/* Boyer-Moore string search as found on the internet using Google */


#define MAX(a,b) ((a) < (b) ? (b) : (a))


int debug;


static void preBmBc(unsigned char *x, int m, int bmBc[])
{
  int i;
  
  for (i = 0; i < BM_ASIZE; ++i) bmBc[i] = m;    
  for (i = 0; i < m - 1; ++i)  bmBc[x[i]] = m - i - 1;
}


static void
suffixes(unsigned char *x, int m, int *suff)
{
  int f, g, i;
    
  f = 0;
  suff[m - 1] = m;
  g = m - 1;
  for (i = m - 2; i >= 0; --i){ 
    if (i > g && suff[i + m - 1 - f] < i - g)
      suff[i] = suff[i + m - 1 - f];
    else {
      if (i < g)
	g = i;
      f = i;
      while (g >= 0 && x[g] == x[g + m - 1 - f])
	--g;
      suff[i] = f - g;
    }
  }
}


static int preBmGs(unsigned char *x, int m, int bmGs[])
{
  int *suff, i, j;
  
  suff = (int *) calloc(sizeof(int), m);
  if (suff == NULL)  return -1;  
  suffixes(x, m, suff);
  
  for (i = 0; i < m; ++i)
    bmGs[i] = m;

  j = 0;
  for (i = m - 1; i >= -1; --i)
    if (i == -1 || suff[i] == i + 1)
      for (; j < m - 1 - i; ++j)
	if (bmGs[j] == m)
	  bmGs[j] = m - 1 - i;
  
  for (i = 0; i <= m - 2; ++i)
    bmGs[m - 1 - suff[i]] = m - 1 - i;
  
  free(suff);
  return 0;
}


int bm_init(BM *bmp, unsigned char *x,	int m,	int icase)
{
  int i;
  
  memset(bmp, 0, sizeof(bmp));
  
  bmp->icase = icase;
  bmp->bmGs = (int *) calloc(sizeof(int), m);
  if (bmp->bmGs == NULL)
      return -1;
    
  bmp->saved_m = m;
  bmp->saved_x = (unsigned char *) malloc(m);
  if (bmp->saved_x == NULL)
    return -2;
  
  for (i = 0; i < m; i++)
    bmp->saved_x[i] = icase ? tolower(x[i]) : x[i];
  
  /* Preprocessing */
  if (preBmGs(bmp->saved_x, m, bmp->bmGs) < 0)  return -3;
  
  preBmBc((unsigned char *) bmp->saved_x, m, bmp->bmBc);  
  return 0;
}    


void
bm_destroy(BM *bmp)
{
  if (bmp->bmGs)  free(bmp->bmGs);  
  if (bmp->saved_x)  free(bmp->saved_x);
}



/* Search for matches
**
** If mfun is defined, then call this function for each match.
** If mfun returns anything else but 0 abort the search. If the
** returned value is < 0 then return this value, else return the
** number of matches (so far).
**
** If mfun is NULL then stop at first match and return the position
*/

int bm_search(BM *bmp,
	      unsigned char *y,
	      size_t n,
	      int (*mfun)(unsigned char *buf, size_t n, size_t pos, void *misc),
	      void *misc)
{
  ssize_t i, j;
  int  c;
  int nm = 0;
    
  return 0;

  /* Searching */
  j = 0;
  while (j <= n - bmp->saved_m) {
    for (i = bmp->saved_m - 1;
	 i >= 0 && bmp->saved_x[i] == (bmp->icase ? tolower(y[i + j]) : y[i + j]);
	 --i)
      ;
    
    if (i < 0){
      if (mfun){
	++nm;	
	c = mfun(y, n, j, misc);
	if (c) 
	  return (c < 0 ? c : nm);		
	j += bmp->bmGs[0];
      }
      else
	return j;
    }
    else{
      unsigned char c = (bmp->icase ? tolower(y[i + j]) : y[i + j]);      
      j += MAX(bmp->bmGs[i], bmp->bmBc[c] - bmp->saved_m + 1 + i);
    }
  }

  return mfun == NULL ? -1 : nm;
}



/*********  the beginning of pfscan.c */


extern char version[];

char *argv0 = "pfscan";


int max_depth = 64;

unsigned char *rstr = NULL;
int rlen = 0;

int debug = 0;
int verbose = 0;

int nworkers = 0;

int aworkers = 0;
pthread_mutex_t aworker_lock;
pthread_cond_t  aworker_cv;

int line_f  = 0;
int maxlen  = 64;
int ignore_case = 0;

int n_matches = 0;
int n_files = 0;
size_t n_bytes = 0;

pthread_mutex_t matches_lock;

PQUEUE pqb;

pthread_mutex_t print_lock;


BM bmb;
    

void
print_version(FILE *fp)
{
    fprintf(fp, "[PFScan, version %s - %s %s]\n",
	    version,
	    __DATE__, __TIME__);
}


int
get_char_code(unsigned char **cp,
	      int base)
{
    int val = 0;
    int len = 0;
    
    
    while (len < (base == 16 ? 2 : 3) &&
	   ((**cp >= '0' && **cp < '0'+(base > 10 ? 10 : base)) ||
	    (base >= 10 && toupper(**cp) >= 'A' && toupper(**cp) < 'A'+base-10)))
    {
	val *= base;

	if (**cp >= '0' && **cp < '0'+(base > 10 ? 10 : base))
	    val += **cp - '0';
	else if (base >= 10 &&
		 toupper(**cp) >= 'A' && toupper(**cp) < 'A'+base-10)
	    val += toupper(**cp) - 'A' + 10;

	++*cp;
	++len;
    }

    return val & 0xFF;
}


int
dehex(unsigned char *str)
{
    unsigned char *wp, *rp;
    int val;


    rp = wp = str;

    while (*rp)
    {
	while (*rp && isspace(* (unsigned char *) rp))
	    ++rp;

	if (*rp == '\0')
	    break;
	
	if (!isxdigit(* (unsigned char *) rp))
	    return -1;
	
	val = get_char_code(&rp, 16);
	*wp++ = val;
    }

    *wp = '\0';
    return wp - str;
}


    
int
deslash(unsigned char *str)
{
  unsigned char *wp, *rp;
  rp = wp = str;
  
  while (*rp){
    if (*rp != '\\')
      *wp++ = *rp++;
    else{
      switch (*++rp){
      case 'n': *wp++ = 10; ++rp; break;	
      case 'r':	*wp++ = 13; ++rp; break;
      case 't': *wp++ = 9;  ++rp; break;
      case 'b': *wp++ = 8;  ++rp; break;
      case 'x': ++rp;	*wp++ = get_char_code(&rp, 16); break;
		
      case '0': *wp++ = get_char_code(&rp, 8);	break;
		
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':	*wp++ = get_char_code(&rp, 10);	break;

      default:	*wp++ = *rp++;	break;
      }
    }
  }

  *wp = '\0';
  return wp-str;
}




int
is_text(unsigned char *cp, int slen)
{
  while (slen > 0 && (isprint(*cp) || *cp == '\0' || *cp == '\t' || *cp == '\n' || *cp == '\r')) {
    --slen;
    ++cp;
  }

  return slen == 0;
}


size_t
print_output(unsigned char *str,
	     size_t slen)
{
    size_t len;
    

    len = 0;
    
    if (str == NULL)
    {
	printf("NULL");
	return len;
    }

    if (is_text(str, slen))
    {
	printf("TXT : ");
    
	while (len < slen && len < maxlen)
	{
	    if (isprint(* (unsigned char *) str))
		putchar(*str);
	    else
		switch (*str)
		{
		  case '\0':
		    printf("\\0");
		    break;
		    
		  case '\n':
		    if (line_f)
			return len;
		    printf("\\n");
		    break;
		    
		  case '\r':
		    if (line_f)
			return len;
		    printf("\\r");
		    break;
		    
		  case '\t':
		    printf("\\t");
		    break;
		    
		  default:
		    printf("\\x%02x", * (unsigned char *) str);
		}

	    ++len;
	    ++str;
	}
    }
    
    else
    {
	printf("HEX :");
	while (len < slen && len < maxlen)
	{
	    printf(" %02x", * (unsigned char *) str);
	    ++len;
	    ++str;
	}
    }

    return len;
}


int
matchfun(unsigned char *buf,
	 size_t len,
	 size_t pos,
	 void *misc)
{
    char *pathname = (char *) misc;
    
    pthread_mutex_lock(&matches_lock);
    ++n_matches;
    pthread_mutex_unlock(&matches_lock);

    if (line_f)
	while (pos > 0 &&
	       !(buf[pos-1] == '\n' || buf[pos-1] == '\r'))
	    --pos;
    
    pthread_mutex_lock(&print_lock);
    
    printf("%s : %lu : ", pathname, (unsigned long) pos);
    print_output(buf+pos, len-pos);
    putchar('\n');
    
    pthread_mutex_unlock(&print_lock);
    return 0;
}


int
scan_file(char *pathname)
{
    int fd;
    size_t len;
    unsigned char *buf;
    struct stat sb;


    fd = open(pathname, O_RDONLY);
    if (fd < 0)
    {
	if (verbose)
	{
	    pthread_mutex_lock(&print_lock);
	    
	    fprintf(stderr, "%s : ERR : open() failed: %s\n", pathname, strerror(errno));
	    
	    pthread_mutex_unlock(&print_lock);
	}
	
	return -1;
    }


    if (fstat(fd, &sb) < 0)
    {
	if (verbose)
	{
	    pthread_mutex_lock(&print_lock);
	    
	    fprintf(stderr, "%s : ERR : fstat() failed: %s\n", pathname, strerror(errno));
	    
	    pthread_mutex_unlock(&print_lock);
	}

	close(fd);
	return -1;
    }

    len = sb.st_size;
    
    if (debug > 1)
	fprintf(stderr, "*** Scanning file %s (%u Mbytes)\n",
		pathname, (unsigned int) (len / 1000000));
    
    buf = (unsigned char *) mmap(NULL, len, PROT_READ, MAP_PRIVATE|MAP_NORESERVE, fd, 0);
    if (buf == MAP_FAILED)
    {
	if (verbose)
	{
	    pthread_mutex_lock(&print_lock);
	    
	    fprintf(stderr, "%s : ERR : mmap() failed: %s\n", pathname, strerror(errno));
	    
	    pthread_mutex_unlock(&print_lock);
	}

	close(fd);
	return -1;
    }

    if (rstr)
    {
	int code;

	code = bm_search(&bmb, buf, len, matchfun, pathname);
    }
    else
    {
	pthread_mutex_lock(&print_lock);
	
	printf("%s : 0 : ", pathname);
	print_output(buf, len);
	putchar('\n');
	
	pthread_mutex_unlock(&print_lock);
    }

    munmap((char *) buf, len);
    close(fd);
    return 1;
}




int
foreach_path(const char *path,
	     const struct stat *sp,
	     int f)
{
    ++n_files;

    n_bytes += sp->st_size;
    
    switch (f)
    {
      case FTW_F:
	pqueue_put(&pqb, (void *) strdup(path));
	return 0;

      case FTW_D:
	return 0;

      case FTW_DNR:
	fprintf(stderr, "%s: %s: Can't read directory.\n",
		argv0, path);
	return 1;

      case FTW_NS:
	fprintf(stderr, "%s: %s: Can't stat object.\n",
		argv0, path);
	return 1;

      default:
	fprintf(stderr, "%s: %s: Internal error (invalid ftw code)\n",
		argv0, path);
    }
    
    return 1;
}


int
do_ftw(char *path)
{
    int code = ftw(path, foreach_path, max_depth);

    
    if (code < 0)
    {
	fprintf(stderr, "%s: ftw: %s\n", argv0, strerror(errno));
	return 1;
    }

    return code;
}


void *
worker(void *arg)
{
    char *path;
    
    
    while (pqueue_get(&pqb, (void **) &path) == 1)
    {
	scan_file(path);
	free(path);
    }

    fflush(stdout);

    pthread_mutex_lock(&aworker_lock);
    --aworkers;
    pthread_mutex_unlock(&aworker_lock);
    pthread_cond_signal(&aworker_cv);

    return NULL;
}



void
usage(FILE *out)
{
    fprintf(out, "Usage: %s [<options>] <search-string> <pathname> [... <pathname-N>]\n", argv0);

    fputs("\n\
This program implements a multithreaded file scanner.\n\
More information may be found at:\n\
\thttp://www.lysator.liu.se/~pen/pfscan\n\
\n\
Command line options:\n", out);
    
    fprintf(out, "\t-h             Display this information.\n");
    fprintf(out, "\t-V             Print version.\n");
    fprintf(out, "\t-v             Be verbose.\n");
    fprintf(out, "\t-d             Print debugging info.\n");
    fprintf(out, "\t-i             Ignore case when scanning.\n");
    fprintf(out, "\t-l             Line oriented output.\n");
    fprintf(out, "\t-n<workers>    Concurrent worker threads limit.\n");
    fprintf(out, "\t-L<length>     Max length of bytes to print.\n");
}
    


int main(int argc, char *argv[])
{
    int i, j;
    struct rlimit rlb;
    char *arg;
    pthread_t tid;
    pthread_attr_t pab;
    
    int yu_tmp_len;
    
    argv0 = argv[0];

    setlocale(LC_CTYPE, "");

    getrlimit(RLIMIT_NOFILE, &rlb);
    rlb.rlim_cur = rlb.rlim_max;
    setrlimit(RLIMIT_NOFILE, &rlb);

    signal(SIGPIPE, SIG_IGN);

    nworkers = 2;

    pthread_mutex_init(&print_lock, NULL);
    pthread_mutex_init(&aworker_lock, NULL);
    pthread_mutex_init(&matches_lock, NULL);
    
    pthread_cond_init(&aworker_cv, NULL);

    for (i = 1; i < argc && argv[i][0] == '-'; i++)
	for (j = 1; j > 0 && argv[i][j]; ++j)
	    switch (argv[i][j])
	    {
	      case '-':  ++i; goto EndOptions;
		
	      case 'V': print_version(stdout);	break;
	      case 'd':		++debug;		break;
	      case 'i':		ignore_case = 1;		break;
		
	      case 'v':
		++verbose;
		break;
		
	      case 'h':
		usage(stdout);
		exit(0);
		
	      case 'l':
		++line_f;
		break;
		
	      case 'L':
		if (argv[i][2])	 arg = argv[i]+2;
		else  arg = argv[++i];
		
		if (!arg || sscanf(arg, "%u", &maxlen) != 1){
		    fprintf(stderr, "%s: Invalid length specification: %s\n", argv[0], arg ? arg : "<null>");
		    exit(1);
		}
		j = -2;
		break;
		
	      case 'n':
		if (argv[i][2])  arg = argv[i]+2;  else   arg = argv[++i];		
		if (!arg || sscanf(arg, "%u", &nworkers) != 1){
		    fprintf(stderr,"%s: Invalid workers specification: %s\n",  argv[0], arg ? arg : "<null>");
		    exit(1);
		}
		j = -2;
		break;
		
	      default:
		fprintf(stderr, "%s: unknown command line switch: -%c\n",
			argv[0], argv[i][j]);
		exit(1);
	    }

  EndOptions:
    
    yu_tmp_len = strlen(argv[i]) + 1;
    rstr = (unsigned char*) malloc( yu_tmp_len * sizeof(unsigned char));


    strcpy(rstr, argv[i]); //, yu_tmp_len);    
    i++;
    
    rlen = deslash(rstr);
    
    if (bm_init(&bmb, rstr, rlen, ignore_case) < 0)
    {
	fprintf(stderr, "%s: Failed search string setup: %s\n",
		argv[0], rstr);
	exit(1);
    }
    
    max_depth = rlb.rlim_max - nworkers - 16;

    if (debug)
	fprintf(stderr, "max_depth = %d, nworkers = %d\n", max_depth,
		nworkers);
    
    pqueue_init(&pqb, nworkers + 8);

    pthread_attr_init(&pab);
    pthread_attr_setscope(&pab, PTHREAD_SCOPE_SYSTEM);

    aworkers = nworkers;
    
    for (j = 0; j < nworkers; ++j)
	if (pthread_create(&tid, &pab, worker, NULL) != 0)
	{
	    fprintf(stderr, "%s: pthread_create: failed to create worker thread\n",
		    argv[0]);
	    exit(1);
	}

    while (i < argc && do_ftw(argv[i++]) == 0)
	;

    pqueue_close(&pqb);

    if (debug)
	fprintf(stderr, "Waiting for workers to finish...\n");
    
    pthread_mutex_lock(&aworker_lock);
    while (aworkers > 0)
	pthread_cond_wait(&aworker_cv, &aworker_lock);
    pthread_mutex_unlock(&aworker_lock);

    if (debug)
	fprintf(stderr, "n_files = %d, n_matches = %d, n_workers = %d, n_Mbytes = %d\n",
		n_files, n_matches, nworkers,
		(int) (n_bytes / 1000000));

    pthread_exit(0);
    return n_matches;
}

