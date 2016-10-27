

#include <atomic>
#include <pthread.h>
#include <unistd.h>
#include <cassert>

template<typename T>
struct SafeStackItem
{
  volatile T Value;
  std::atomic<int> Next;
};

template<typename T>
class SafeStack
{
  std::atomic<int> head;
  std::atomic<int> count;

public:
  SafeStackItem<T>* array;

  SafeStack(int pushCount)
  {
      array = new SafeStackItem<T> [pushCount];
      count.store(pushCount, std::memory_order_relaxed);
      head.store(0, std::memory_order_relaxed);
      for (int i = 0; i < pushCount - 1; i++)
          array[i].Next.store(i + 1, std::memory_order_relaxed);
      array[pushCount - 1].Next.store(-1, std::memory_order_relaxed);
  }

  ~SafeStack()
  {
      delete [] array;
  }

  int Pop()
  {
      while (count.load(std::memory_order_acquire) > 1)
      {
          int head1 = head.load(std::memory_order_acquire);
          int next1 = array[head1].Next.exchange(-1, std::memory_order_seq_cst);

          if (next1 >= 0)
          {
              int head2 = head1;
              if (head.compare_exchange_strong(head2, next1, std::memory_order_seq_cst))
              {
                  count.fetch_sub(1, std::memory_order_seq_cst);
                  return head1;
              }
              else
              {
                  array[head1].Next.exchange(next1, std::memory_order_seq_cst);
              }
          }
          else
          {
            sched_yield();
          }
      }

      return -1;
  }

  void Push(int index)
  {
      int head1 = head.load(std::memory_order_acquire);
      do
      {
          array[index].Next.store(head1, std::memory_order_release);
         
      } while (!head.compare_exchange_strong(head1, index, std::memory_order_seq_cst));
      count.fetch_add(1, std::memory_order_seq_cst);
  }
};

const unsigned NUM_THREADS = 3;
SafeStack<int> stack(3);

pthread_t threads[NUM_THREADS];

void* thread(void* arg)
{
  int idx = (int)(size_t)arg;
    for (size_t i = 0; i != 2; i += 1)
    {
        int elem;
        for (;;)
        {
            elem = stack.Pop();
            if (elem >= 0)
                break;
            sched_yield();
        }

        stack.array[elem].Value = idx;
        assert(stack.array[elem].Value == idx);

        stack.Push(elem);
    }
    return NULL;
}

int main()
{
  for (unsigned i = 0; i < NUM_THREADS; ++i) {
    pthread_create(&threads[i], NULL, thread, (void*)i);
  }
  
  for (unsigned i = 0; i < NUM_THREADS; ++i) {
    pthread_join(threads[i], NULL);
  }
  
  return 0;
}

