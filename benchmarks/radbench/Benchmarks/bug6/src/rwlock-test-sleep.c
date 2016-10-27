#include <stdio.h>
#include "nspr.h"

/* Function Prototypes */
static void reader_main(void *arg);
static void writer_main(void *arg);

/* Globals */
static PRRWLock *thelock;

int main()
{
    int rc = 0;
    PRThread *reader_thread = NULL;
    PRThread *writer_thread = NULL;

    /* Create the RWlock. */
    thelock = PR_NewRWLock(PR_RWLOCK_RANK_NONE,"the lock");
    if (thelock == NULL) {
        printf("Error creating RWlock.\n");
        rc = 1;
        goto bail;
    }

    /* Create the reader thread. */
    reader_thread = PR_CreateThread(PR_USER_THREAD,
                             reader_main,
                             NULL,
                             PR_PRIORITY_NORMAL,
                             PR_GLOBAL_THREAD,
                             PR_JOINABLE_THREAD,
                             0);

    /* Create the writer thread. */
    writer_thread = PR_CreateThread(PR_USER_THREAD,
                             writer_main,
                             NULL,
                             PR_PRIORITY_NORMAL,
                             PR_GLOBAL_THREAD,
                             PR_JOINABLE_THREAD,
                             0);

    if ((reader_thread == NULL) || (writer_thread == NULL)) {
        printf("Error creating threads.\n");
        rc = 1;
        goto bail;
    }

    /* Don't exit until the threads are done. */
    rc = PR_JoinThread(reader_thread);
    PR_ASSERT(PR_SUCCESS == rc);
    rc = PR_JoinThread(writer_thread);
    PR_ASSERT(PR_SUCCESS == rc);

bail:
    return rc;
}

/*
 * reader_main()
 *
 * This function will attempt to get multiple reader
 * locks.  We want to guarantee that we get the first
 * lock before the writer thread gets a writer lock.
 * We also want to guarantee that the writer thead
 * is waiting on the writer lock before we attempt to
 * get the second reader lock.
 */
static void reader_main(void *arg)
{
    printf("Reader running.  Attempting to get first reader lock.\n");
    PR_RWLock_Rlock(thelock);
    printf("Reader obtained first reader lock.\n");

    printf("Reader sleeping for 2 seconds.\n");
    PR_Sleep(PR_SecondsToInterval(2));

    printf("Reader attempting to get second reader lock.\n");
    PR_RWLock_Rlock(thelock);
    printf("Reader obtained second reader lock.\n");

    printf("Reader releasing second reader lock.\n");
    PR_RWLock_Unlock(thelock);
    printf("Reader releasing first reader lock.\n");
    PR_RWLock_Unlock(thelock);
}

/*
 * writer_main()
 *
 * This function will attempt to get a writer lock, but
 * we expect to be blocked by the reader thread since
 * it should already hold it's first reader lock.
 */
static void writer_main(void *arg)
{
    printf("Writer running.  Sleeping for 1 second.\n");
    PR_Sleep(PR_SecondsToInterval(1));

    printf("Writer attempting to get writer lock.\n");
    PR_RWLock_Wlock(thelock);
    printf("Writer obtained writer lock.\n");

    printf("Writer releasing writer lock.\n");
    PR_RWLock_Unlock(thelock);
}

