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

#include "systematic/random.h"


#include <cstdlib>
#include "core/logging.h"

namespace systematic {

RandomScheduler::RandomScheduler(ControllerInterface *controller)
    : Scheduler(controller) {
  // empty
}

RandomScheduler::~RandomScheduler() {
  // empty;
}

void RandomScheduler::Register() {
  knob()->RegisterBool("enable_random_scheduler", "whether use the random scheduler", "0");
  knob()->RegisterInt("seed", "seed for pct algorithm", "0");
  knob()->RegisterBool("use_seed", "use the seed parameter", "0");
}

bool RandomScheduler::Enabled() {
  return knob()->ValueBool("enable_random_scheduler");
}

static unsigned long long rdtsc(){
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((unsigned long long)hi << 32) | lo;
}

void RandomScheduler::Setup() {
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
  srand(0);
}

void RandomScheduler::ProgramStart() {
  // empty
}

void RandomScheduler::ProgramExit() {
  // empty
}

void RandomScheduler::Explore(State *init_state) {
  // start with the initial state
  State *state = init_state;
  // run until no enabled thread
  while (!state->IsTerminal()) {
    // randomly pick the next thread to run
    Action *action = PickNextRandom(state);
    // execute the action and move to next state
    state = Execute(state, action);
  }
}

bool RandomScheduler::RandomChoice(double true_rate) {
  double val = rand() / (RAND_MAX + 1.0);
  if (val < true_rate)
    return true;
  else
    return false;
}

Action *RandomScheduler::PickNextRandom(State *state) {
  
  Action::Map *enabled = state->enabled();
  assert(enabled->size() > 0);
  
  std::uniform_int_distribution<int> intDist(0, enabled->size()-1);

  size_t index = intDist(random); // rand() % enabled->size();
  Action::Map::iterator it = enabled->begin();

  for(size_t i=0; i < index; ++i) {
    it++;
  }
//  std::cout << "Randomly chosen " << index << " out of " << enabled->size() << "."
//      << std::endl;
  return it->second;

//  Action *target = NULL;
//  int counter = 1;
//  for (Action::Map::iterator it = enabled->begin(); it != enabled->end(); ++it){
//    Action *current = it->second;
//    // decide whether to pick the current one
//    if (RandomChoice(1.0 / (double)counter)) {
//      target = current;
//    }
//    // increment the counter
//    counter += 1;
//  }
//  DEBUG_ASSERT(target);
//  return target;
}

} // namespace systematic

