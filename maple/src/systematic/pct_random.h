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

// File: systematic/random.h - The definition of the random scheduler
// which picks a random thread to run at each schedule point.

#ifndef SYSTEMATIC_PCT_RANDOM_H_
#define SYSTEMATIC_PCT_RANDOM_H_

#include "core/basictypes.h"
#include "systematic/scheduler.h"
#include <random>


namespace systematic {

class PCTRandomScheduler : public Scheduler {
 public:
  std::default_random_engine random;

  /// p
  std::vector<int> priorities_;
  /// k_1,...,k_{d-1}
  std::vector<unsigned> changePoints_;
  int n_;
  int d_;
  int k_;

  explicit PCTRandomScheduler(ControllerInterface *controller);
  ~PCTRandomScheduler();

  // overrided virtual functions
  void Register();
  bool Enabled();
  void Setup();
  void ProgramStart();
  void ProgramExit();
  void Explore(State *init_state);

 protected:

 private:
  DISALLOW_COPY_CONSTRUCTORS(PCTRandomScheduler);
};

} // namespace systematic

#endif

