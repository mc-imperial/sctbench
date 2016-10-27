#include "RTTL/common/RTThread.hxx"

void *MultiThreadedTaskQueue::threadFunc(void *_mttq)
{
  DBG_THREAD(PING);
  _ALIGN(DEFAULT_ALIGNMENT) static AtomicCounter threadId;    
  int id = threadId.inc();
  //cout << "created thread id " << id << endl << flush;
  MultiThreadedTaskQueue *mttq = (MultiThreadedTaskQueue*)_mttq;    
  mttq->waitOnBarrier();
  while(1)
    {
      DBG_THREAD(cout << "thread " << id << " wait for activation " << endl << flush);
      mttq->waitForThreadActivation(id);
      DBG_THREAD(cout << "thread " << id << " activated " << endl << flush);
      const int action = mttq->task(id);
      mttq->deactivateThreadAndBroadcast(id);
      if (action == THREAD_EXIT) break;
    }
  return NULL;
}
