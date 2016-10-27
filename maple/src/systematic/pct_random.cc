// Copyright 2011 The University of Michigan
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Authors - Jie Yu (jieyu@umich.edu)

// File: systematic/random.cc - The implementation of the random scheduler
// which picks a random thread to run at each schedule point.

#include "systematic/pct_random.h"

#include <algorithm>
#include <chrono>

#include <cstdlib>
#include "core/logging.h"

namespace systematic {

PCTRandomScheduler::PCTRandomScheduler(ControllerInterface *controller)
    : Scheduler(controller)
{
  // empty
}

PCTRandomScheduler::~PCTRandomScheduler() {
  // empty;
}

void PCTRandomScheduler::Register() {
  knob()->RegisterBool("enable_pct_scheduler", "whether use the pct random scheduler", "0");
  knob()->RegisterInt("pct_n", "max number of threads", "2");
  knob()->RegisterInt("pct_k", "max number of scheduling points", "100");
  knob()->RegisterInt("pct_d", "d (depth) for pct algorithm", "2");
  knob()->RegisterInt("seed", "seed for pct algorithm", "0");
  knob()->RegisterBool("use_seed", "use the seed parameter", "0");

}

bool PCTRandomScheduler::Enabled() {
  return knob()->ValueBool("enable_pct_scheduler");
}

static unsigned long long rdtsc(){
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((unsigned long long)hi << 32) | lo;
}

void PCTRandomScheduler::Setup() {

  desc()->SetHookYieldFunc();

  // seed the random number generator
  if(knob()->ValueBool("use_seed")) {
    int seed = knob()->ValueInt("seed");
    random.seed(seed);
    std::cout << "SEED: " << seed << std::endl;
  } else {
    unsigned long long seed = rdtsc();
    random.seed(seed);
    std::cout << "SEED: " << seed << std::endl;
  }

  n_ = knob()->ValueInt("pct_n");
  k_ = knob()->ValueInt("pct_k");
  d_ = knob()->ValueInt("pct_d");

  /// pi
  std::vector<int> perm;
  for(int i=1; i <= n_; ++i)
  {
    perm.push_back(i);
  }
  std::shuffle(perm.begin(), perm.end(), random);

  // thread priorities (p)
  for(int i=0; i < n_; ++i)
  {
    priorities_.push_back(d_ + perm[i] - 1);
  }

  // change points
  std::uniform_int_distribution<int> intDist(1, k_);
  for(int i=1; i <= d_-1; ++i)
  {
    changePoints_.push_back(intDist(random));
  }

//  std::cout << "n_: " << n_ << "\n" << "k_: " << k_ << "\n" << "d_: " << d_ << std::endl;

  std::cout << "Priorities: " << priorities_[0];
  for(int i=1; i < n_; ++i)
  {
    std::cout << ", " << priorities_[i];
  }
  std::cout << std::endl;

}

void PCTRandomScheduler::ProgramStart() {
  // empty
}

void PCTRandomScheduler::ProgramExit() {
  // empty
}

void PCTRandomScheduler::Explore(State *init_state) {
  // start with the initial state
  State *state = init_state;

  int yieldPriority = 0;

  unsigned int steps = 0;
  // run until no enabled thread
  while (!state->IsTerminal()) {

    Action::Map *enabled = state->enabled();
    assert(enabled->size() > 0);

    Action::Map::iterator maxElement;
    Action *action = 0;

//    std::cout << "Finding highest priority enabled thread!" << std::endl;

    // pick highest priority enabled thread
    maxElement = std::max_element(enabled->begin(), enabled->end(),
        [this] (Action::Map::value_type const& lhs, Action::Map::value_type const& rhs)
        {
          return this->priorities_.at(lhs.first->uid()-1) < this->priorities_.at(rhs.first->uid()-1);
        } );

    action = maxElement->second;
//    std::cout << "Picked thread " << (maxElement->first->uid()-1) << " with op " << action->op() << std::endl;

    if((action->op() == OP_SCHED_YIELD || action->op() == OP_SLEEP
        || action->op() == OP_USLEEP
        || action->op() == OP_COND_TIMEDWAIT))
    {
      std::cout << "..Lowering " << (maxElement->first->uid()-1) << std::endl;

      priorities_.at(maxElement->first->uid()-1) = yieldPriority;
      --yieldPriority;
    }

    // increment num steps
    if(enabled->size() > 1 || steps > 0)
    {
      ++steps;
    }
    // execute the action and move to next state
    state = Execute(state, action);

    // Are we at a change point?
    for(int i=1; i <= d_-1; ++i)
    {
      if(steps == changePoints_[i-1])
      {
//        std::cout << "Change point: lowering thread " << (maxElement->first->uid()-1) << std::endl;
        priorities_.at(maxElement->first->uid()-1) = d_ - i;
      }
    }

  }
  std::cout << "PCT NUM STEPS: " << steps << std::endl;
}

} // namespace systematic

