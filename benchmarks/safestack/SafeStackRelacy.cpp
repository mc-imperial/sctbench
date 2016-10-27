
#include <relacy/relacy_std.hpp>

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
              pthread_yield();
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

struct test : rl::test_suite<test, 3>
{
  SafeStack<int> stack;

  test()
      : stack (3)
  {
  }

  void thread(unsigned idx)
  {
      for (size_t i = 0; i != 2; i += 1)
      {
          int elem;
          for (;;)
          {
              elem = stack.Pop();
              if (elem >= 0)
                  break;
              pthread_yield();
          }

          stack.array[elem].Value = idx;
          pthread_yield();
          assert(stack.array[elem].Value == idx);
          stack.Push(elem);
      }
  }
};

int main()
{
  rl::test_params p;
    p.iteration_count = 1000000;
    //p.search_type = rl::sched_bound;
    //p.context_bound = 5;
  rl::simulate<test>(p);
}

