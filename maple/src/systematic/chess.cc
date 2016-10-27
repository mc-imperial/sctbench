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

// File: systematic/chess.cc - The implementation of the CHESS scheduler
// which systematically explore thread interleavings using iterative
// preemption bound.

#include "systematic/chess.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <sys/stat.h>
#include <sstream>
#include "core/logging.h"

namespace systematic {

ChessScheduler::ChessScheduler(ControllerInterface *controller)
    : Scheduler(controller),
      pb_enable_(false),
      pb_useDelayBound_(false),
      por_enable_(false),
      pb_limit_(0),
      useless_(false),
      divergence_(false),
      curr_state_(NULL),
      curr_action_(NULL),
      curr_node_(NULL),
      prefix_size_(0),
      curr_preemptions_(0),
      seal_after_one_(false),
      curr_hash_val_(0),
      curr_exec_id_(0) {
  // empty
}

ChessScheduler::~ChessScheduler() {
  // empty
}

void ChessScheduler::Register() {
  knob()->RegisterBool("enable_chess_scheduler", "whether use the CHESS scheduler", "0");
  knob()->RegisterBool("fair", "whether enable the fair control module", "1");
  knob()->RegisterBool("pb", "whether enable preemption bound search", "1");
  knob()->RegisterBool("delay_bound", "instead of preemption bound, use a delay bound", "1");
  knob()->RegisterBool("por", "whether enable parital order reduction", "1");
  knob()->RegisterBool("abort_diverge", "whether abort when divergence happens", "1");
  knob()->RegisterInt("pb_limit", "the maximum number of preemption an execution can have", "2");
  knob()->RegisterBool("seal_after_one", "seal a racey memory op after it has been preempted once", "0");
  knob()->RegisterStr("search_in", "the input file that contains the search information", "search.db");
  knob()->RegisterStr("search_out", "the output file that contains the search information", "search.db");
  knob()->RegisterStr("por_info_path", "the dir path that stores the partial order reduction information", "por-info");
}

bool ChessScheduler::Enabled() {
  return knob()->ValueBool("enable_chess_scheduler");
}

void ChessScheduler::Setup() {
  // settings and flags
  fair_enable_ = knob()->ValueBool("fair");
  pb_enable_ = knob()->ValueBool("pb");
  pb_useDelayBound_ = knob()->ValueBool("delay_bound");
  if (pb_useDelayBound_) {
    assert(
        pb_enable_ && "Must enable preemption bound search to use delay bound");
  }
  por_enable_ = knob()->ValueBool("por");
  pb_limit_ = knob()->ValueInt("pb_limit");
  por_info_path_ = knob()->ValueStr("por_info_path");
  
  seal_after_one_ = knob()->ValueBool("seal_after_one");
  
  // load search info
  search_info_.Load(knob()->ValueStr("search_in"), sinfo(), program());
  if (search_info_.Done()) {
    printf("[CHESS] search done\n");
    exit(77);
  }
  prefix_size_ = search_info_.StackSize();
  DEBUG_FMT_PRINT_SAFE("prefix size = %d\n", (int)prefix_size_);

  // setup descriptor
  desc()->SetHookYieldFunc();

  // seed the random number generator
  //srand((unsigned int)time(NULL));
  srand((unsigned int)0);
}

void ChessScheduler::ProgramStart() {
  // init components
  if (pb_enable_)
    PbInit();
  if (por_enable_)
    PorInit();
}

void ChessScheduler::ProgramExit() {
  // fini components
  if (pb_enable_) {
    PbFini();
    std::cout << std::endl;
    std::cout << "Number of preemptions/delays: " << curr_preemptions_ << std::endl;
  }
  if (por_enable_)
    PorFini();

  // save search info
  if (!divergence_) {
    search_info_.UpdateForNext();
    search_info_.Save(knob()->ValueStr("search_out"), sinfo(), program());
    if(search_info_.Done()) {
    	printf("[CHESS] search done\n");
    	exit(77);
    }
  }
}

bool ChessScheduler::IsInvisibleOp(Operation op) {
  return op == OP_MUTEX_UNLOCK
      || op == OP_THREAD_CREATE
      || op == OP_THREAD_JOIN
      || op == OP_BARRIER_WAIT;
}

State *ChessScheduler::getPreviousState() {
  if(!curr_state_) {
    return NULL;
  }
  State* state = curr_state_->Prev();
  State* prevState = NULL;
  while(true) {
    if(!state) {
      // did not find a previous state
      break;
    }
    if(!IsInvisibleOp(state->taken()->op())) {
      prevState = state;
      break;
    }
    state = state->Prev();
  }
  return prevState;
}

void ChessScheduler::Explore(State *init_state) {
  // start with the initial state
  curr_state_ = init_state;
  // run until no enabled thread
  while (!curr_state_->IsTerminal()) {
    
    // SCTEdit: a simple POR. Take these actions immediately and
    // do not add them to the search stack.
//    Action *immediate_action = NULL;
//    for (Action::Map::iterator it = curr_state_->enabled()->begin(),
//        end = curr_state_->enabled()->end();
//           it != end; ++it) {
//      Action *action = it->second;
//      if(IsInvisibleOp(action->op())) 
//      {
//        immediate_action = action;
//        break;
//      }
//    }
//    if(immediate_action) {
//      curr_action_ = immediate_action;
//      curr_state_ = Execute(curr_state_, immediate_action);
//      continue;
//    }
    
//    if(curr_node_) // prev search node
//    {
//      Thread *prevThread = curr_node_->sel();
//      auto nextOpSameThreadIt = curr_state_->enabled()->find(prevThread);
//      // Is prev thread is still enabled?
//      if(nextOpSameThreadIt != curr_state_->enabled()->end())
//      {
//        Action *nextOp = nextOpSameThreadIt->second;
//        // Does its op form a persistent set?
//        if(IsInvisibleOp(nextOp->op()))
//        {
//          // disable all other threads
//          curr_state_->enabled()->clear();
//          (*curr_state_->enabled())[prevThread] = nextOp;
//        }
//      }
//    }
    
    
    // SCTEdit: a hacky/unsafe way to handle yield, sleep, etc.
    // Just disable the thread (temporarily) when we encounter these ops.
    // This forces a preemption but does not "cost" us a preemption.
    if(curr_state_->enabled()->size() > 1) {
      for (Action::Map::iterator it = curr_state_->enabled()->begin(),
          end = curr_state_->enabled()->end();
             it != end; ++it) {
        Action *action = it->second;
        if (curr_node_ && curr_node_->sel() == action->thd()
          && (action->op() == OP_SCHED_YIELD || action->op() == OP_SLEEP
              || action->op() == OP_USLEEP
              || action->op() == OP_COND_TIMEDWAIT)) {
            curr_state_->enabled()->erase(it);
            break;
        }
      }
    }
    
    // get next node in the search stack
    curr_node_ = search_info_.GetNextNode(curr_state_);
    if (!curr_node_) {
      // divergence run
      DivergenceRun();
      return;
    }
    // update backtrack: add all enabled thread to backtrack
    // this is necessary because we want to explore all possible
    // interleavings. we only need to do nextOpSameThread once.
    if (!IsPrefix())
      UpdateBacktrack();
    // update fair control status
    if (fair_enable_)
      FairUpdate();
    // pick the next action to execute
    Action *next_action = PickNext();
    if (!next_action) {
      // useless run
      UselessRun();
      return;
    }
    
    if(seal_after_one_) {
      // find previous action
      State* prevState = getPreviousState();
      if(prevState) {
        Action* prevAction = prevState->taken();
        // find non-preempting action in current state
        auto it = curr_state_->enabled()->find(prevAction->thd());
        if(it != curr_state_->enabled()->end()) {
          Action* nonPreemptingAction = it->second;
          // is nextOpSameThread a mem op and can we see which instruction nextOpSameThread is?
          if(nonPreemptingAction->inst() && nonPreemptingAction->IsMemOp()) {
            // loop through each action to see if nextOpSameThread preempts the mem op
            // and is undone
            for (auto& thr_action : *curr_state_->enabled()) {
              Action* action = thr_action.second;
              if (action != nonPreemptingAction
                  && !curr_node_->IsDone(action->thd())) {
                // add to list
                search_info_.InstructionsPreempted()->insert(
                    nonPreemptingAction->inst());
              }
            }
            
          }
        }
      }
    }
    
    // update search node
    curr_node_->set_sel(next_action->thd());
    if (!IsPrefix())
      curr_node_->AddDone(next_action->thd());
    DEBUG_FMT_PRINT_SAFE("Schedule Point: %s\n",
                         curr_node_->ToString().c_str());
    // execute the action and move to next state
    if (pb_enable_)
      PbUpdate(next_action);
    if (por_enable_)
      PorUpdate(next_action);
    curr_action_ = next_action;
    curr_state_ = Execute(curr_state_, next_action);
  }
}

void ChessScheduler::DivergenceRun() {
  printf("[CHESS] divergence happens\n");
  // mark this run as divergence
  divergence_ = true;

  // abort if needed
  if (knob()->ValueBool("abort_diverge")) {
    std::cout << "PROBLEM: divergence" << std::endl;
    assert(0);
  }

  // run until no enabled threads
  while (!curr_state_->IsTerminal()) {
    // just pick an enabled thread randomly
    Action *next_action = PickNextRandom();
    DEBUG_ASSERT(next_action);
    // execute the next action
    curr_action_ = next_action;
    curr_state_ = Execute(curr_state_, next_action);
  }
}

void ChessScheduler::UselessRun() {
  printf("[CHESS] useless run\n");
  // mark this run as useless
  useless_ = true;
  std::cout << "PROBLEM: Useless run" << std::endl;
  // run until no enabled threads
//  while (!curr_state_->IsTerminal()) {
//    // just pick an enabled thread randomly
//    Action *next_action = PickNextRandom();
//    DEBUG_ASSERT(next_action);
//    // execute the next action
//    curr_action_ = next_action;
//    curr_state_ = Execute(curr_state_, next_action);
//  }
}

Action *ChessScheduler::PickNext() {
  // replay the prefix
  if (IsPrefix()) {
    Action *next_action = curr_state_->FindEnabled(curr_node_->sel());
    DEBUG_ASSERT(next_action);
    return next_action;
  }
  
  State* prevState = getPreviousState();
  Action* prevAction = NULL;
  Action* noPreemptAction = NULL;
  if(seal_after_one_ && prevState) {
    prevAction = prevState->taken();
    Action::Map::iterator it2 = curr_state_->enabled()->find(prevAction->thd());
    if(it2 != curr_state_->enabled()->end()) {
      noPreemptAction = it2->second;
    }
  }
  
  Action::Set actionsToSetAsDoneUnlessNoneLeft;
  
  // first pass, for each undone enabled action, check whether
  // the next state is visted or not. if yes, mark it as done
  // also, check preemptions. if exceed the bound, mark it as done
  for (Action::Map::iterator it = curr_state_->enabled()->begin();
       it != curr_state_->enabled()->end(); ++it) {
    Action *action = it->second;
    if (!curr_node_->IsDone(action->thd())) {
      // action is not done
      // 1) check fair (if fair is enabled)
      if (fair_enable_) {
        if (!FairEnabled(action)) {
          DEBUG_FMT_PRINT_SAFE("Fair pruned\n");
          curr_node_->AddDone(action->thd());
        }
      }
      // 2) check preemptions (if pb is enabled)
      if (pb_enable_) {
        if (!PbEnabled(action)) {
          DEBUG_FMT_PRINT_SAFE("PB pruned\n");
          curr_node_->AddDone(action->thd());
        }
      }
      // 3) check visited (if por is enabled)
      if (por_enable_) {
        if (PorVisited(action)) {
          DEBUG_FMT_PRINT_SAFE("POR pruned\n");
          curr_node_->AddDone(action->thd());
        }
      }
      if(seal_after_one_) {
        if (noPreemptAction && noPreemptAction->IsMemOp() && noPreemptAction->inst()
            && action != noPreemptAction) {
          assert(prevAction);
          if (search_info_.InstructionsPreempted()->count(noPreemptAction->inst())) {
  //          std::cout << "IGNORING MEM OP THAT WAS PREEMPTED ALREADY" << std::endl;
            //curr_node_->AddDone(action->thd());
            actionsToSetAsDoneUnlessNoneLeft.insert(action);
          }
        }
      }
    }
  }
  
  if(seal_after_one_)
  {
    bool ok = false;
    for (auto& thd_action : *curr_state_->enabled()) {
      if (!curr_node_->IsDone(thd_action.first)
          && actionsToSetAsDoneUnlessNoneLeft.count(thd_action.second) == 0) {
        ok = true;
        break;
      }
    }
    if(ok) {
      for(Action* action : actionsToSetAsDoneUnlessNoneLeft) {
        curr_node_->AddDone(action->thd());
      }
    }
  }
  
  // second pass, find an undone enabled action
  
  Thread::Vec& thr_crea_order = controller_->GetThreadCreationOrder();
  // DEBUGGING
//  std::cout << "Picking next undone action" << std::endl;
//  std::cout << "Cost of execution: " << curr_preemptions_ << std::endl;
//  std::cout << "Threads: ";
//  for (size_t i = 0, end = thr_crea_order.size(); i < end; i++) {
//    if(!curr_state_->IsEnabled(thr_crea_order[i])) {
//      std::cout << "(" << i+1 << ")";
//    }
//    else if(curr_node_->IsDone(thr_crea_order[i])) {
//      std::cout << "[" << i+1 << "]";
//    }
//    else
//    {
//      std::cout << " " << i+1 << " ";
//    }
//  }
//  std::cout << std::endl;
  
  Action *next_action = NULL;

  if (curr_state_->enabled()->size() > 0) {
    assert(curr_node_->Prev() || curr_node_->idx() == 0);
    size_t tindex = 0;
    if (curr_node_->Prev()) {
      // Previous node exists
      Thread * prev_thread = curr_node_->Prev()->sel();
      // find previous thread
      while (thr_crea_order[tindex]->uid() != prev_thread->uid()) {
        tindex++;
        if (tindex >= thr_crea_order.size()) {
          tindex = 0;
        }
      }
    }
    size_t prevTindex = tindex;

//    std::cout << "current thread: " << tindex+1 << std::endl;

    // find next enabled action that is not done
    for (size_t i = 0, end = thr_crea_order.size(); i < end; i++) {
      if (curr_state_->IsEnabled(thr_crea_order[tindex])
          && !curr_node_->IsDone(thr_crea_order[tindex])) {
        next_action = curr_state_->enabled()->at(thr_crea_order[tindex]);
//          std::cout << "chosen tindex=" << tindex+1 << std::endl;
        break;
      }
      tindex++;
      if (tindex >= thr_crea_order.size()) {
        tindex = 0;
      }
    }
    
    // potentially disable any non-deterministic thread switches
    if(next_action && !thr_crea_order[prevTindex]->enable_nondet_switches_) {
      for (auto& thr_action : (*curr_state_->enabled())) {
        if(thr_action.second != next_action) {
          curr_node_->AddDone(thr_action.first);
        }
      }
    }
  }
  
//  if(next_action) {
//    std::cout << Operation_Name(next_action->op()) << " " << std::flush;
//  } else {
//    std::cout << "No enabled and undone thread!" << std::endl;
//  }
  
  // return the next action (could be NULL)
  return next_action;
}

Action *ChessScheduler::PickNextRandom() {
  Action::Map *enabled = curr_state_->enabled();
  Action *target = NULL;
  int counter = 1;
  for (Action::Map::iterator it = enabled->begin(); it != enabled->end(); ++it){
    Action *current = it->second;
    // decide whether to pick the current one
    if (RandomChoice(1.0 / (double)counter)) {
      target = current;
    }
    // increment the counter
    counter += 1;
  }
  DEBUG_ASSERT(target);
  return target;
}

bool ChessScheduler::IsPreemptiveChoice(Action *action) {
  Thread* prevThread = NULL;
  assert(curr_node_);
  
  if(curr_node_->Prev()) {
    prevThread = curr_node_->Prev()->sel();
  } else {
    Thread::Vec& thr_crea_order = controller_->GetThreadCreationOrder();
    prevThread = thr_crea_order[0];
  }
  
  if (curr_state_->enabled()->find(prevThread)
          != curr_state_->enabled()->end()
      && prevThread != action->thd())
    return true;
  else
    return false;
}

bool ChessScheduler::IsFrontier() {
  return curr_node_->idx() + 1 == prefix_size_;
}

bool ChessScheduler::IsPrefix() {
  return !IsFrontier() && curr_node_->idx() < prefix_size_;
}

void ChessScheduler::UpdateBacktrack() {
  for (Action::Map::iterator it = curr_state_->enabled()->begin();
       it != curr_state_->enabled()->end(); ++it) {
    Action *action = it->second;
    curr_node_->AddBacktrack(action->thd());
  }
}

bool ChessScheduler::RandomChoice(double true_rate) {
  double val = rand() / (RAND_MAX + 1.0);
  if (val < true_rate)
    return true;
  else
    return false;
}

ChessScheduler::hash_val_t ChessScheduler::Hash(Action *action) {
  DEBUG_ASSERT(action->obj() && action->inst());
  hash_val_t hash_val = 0;
  hash_val += (hash_val_t)action->thd()->uid();
  hash_val += (hash_val_t)(action->obj()->uid() << 2);
  hash_val += (hash_val_t)(action->op() << 5);
  hash_val += (hash_val_t)(action->inst()->id() << 7);
  hash_val += (hash_val_t)(action->tc() << 13);
  hash_val += (hash_val_t)(action->oc() << 23);
  return hash_val;
}

// fair related
void ChessScheduler::FairUpdate() {
  fair_ctrl_.Update(curr_state_);
  DEBUG_FMT_PRINT_SAFE("Fair control status\n%s",
                       fair_ctrl_.ToString().c_str());
}

bool ChessScheduler::FairEnabled(Action *next_action) {
  return fair_ctrl_.Enabled(curr_state_, next_action);
}

// preemption bound related
void ChessScheduler::PbInit() {
  DEBUG_ASSERT(pb_enable_);
  curr_preemptions_ = 0;
}

void ChessScheduler::PbFini() {
  DEBUG_ASSERT(pb_enable_);
  // empty
}

template<class Iterator, class Container> void incrementWrap(Iterator& it,
    Container& cont) {
  it++;
  if(it == cont.end()) {
    it = cont.begin();
  }
}

int ChessScheduler::DbGetDelayCost(Action *next_action) {
  int cost = 0;
  //std::cout << "Calculating delay cost" << std::endl;
  
  if(curr_state_->enabled()->size() > 1) {
    Thread::Vec& thr_crea_order = controller_->GetThreadCreationOrder();
    
    // DEBUGGING
//    std::cout << "Threads: ";
//    for (size_t i = 0, end = thr_crea_order.size(); i != end; ++i) {
//      if(!curr_state_->IsEnabled(thr_crea_order[i])) {
//        std::cout << "(" << i << ")";
//      } else {
//        std::cout << " " << i << " ";
//      }
//    }
//    std::cout << std::endl;
    
    // find current thread
    size_t tindex = 0;
    assert(curr_node_);
    if(curr_node_->Prev()) {
      while (thr_crea_order[tindex]->uid() != curr_node_->Prev()->sel()->uid()) {
        tindex++;
        if(tindex >= thr_crea_order.size())
        {
          tindex=0;
        }
      }
    }
    //std::cout << "curren thread tindex=" << tindex << std::endl;
    // find next enabled thread
    while(!curr_state_->IsEnabled(thr_crea_order[tindex])) {
      tindex++;
      if(tindex >= thr_crea_order.size())
      {
        tindex=0;
      }
      assert((size_t)cost < curr_state_->enabled()->size());
    }
    // find next_action's thread and calculate cost
    while(thr_crea_order[tindex]->uid() != next_action->thd()->uid()) {
      if(curr_state_->IsEnabled(thr_crea_order[tindex])) {
        cost++;
      }
      tindex++;
      if(tindex >= thr_crea_order.size())
      {
        tindex=0;
      }
    }
//  std::cout << "next thread tindex=" << tindex << std::endl;
  }
//  std::cout << "cost=" << cost << std::endl;
  return cost;
}

int ChessScheduler::GetActionCost(Action *next_action) {
  int cost = 0;
  if (pb_useDelayBound_) {
    cost = DbGetDelayCost(next_action);
  }
  else if (pb_enable_) {
    if(IsPreemptiveChoice(next_action)) {
      cost = 1;
    }
  }
  return cost;
}

void ChessScheduler::PbUpdate(Action *next_action) {
  DEBUG_ASSERT(pb_enable_);
  curr_preemptions_ += GetActionCost(next_action);
}

bool ChessScheduler::PbEnabled(Action *next_action) {
  return (curr_preemptions_ + GetActionCost(next_action) <= pb_limit_);
}

// partial order reduction related functions
void ChessScheduler::PorInit() {
  DEBUG_ASSERT(por_enable_);
  curr_hash_val_ = 0;
  PorLoad();
}

void ChessScheduler::PorFini() {
  DEBUG_ASSERT(por_enable_);
  if (!divergence_ && !useless_)
    PorSave();
}

void ChessScheduler::PorUpdate(Action *next_action) {
  DEBUG_ASSERT(por_enable_);

  // skip transparent actions
  if (!next_action->obj())
    return;

  curr_hash_val_ = HashJoin(curr_hash_val_, Hash(next_action));
  // update visited states
  VisitedState *vs = new VisitedState;
  vs->hash_val = curr_hash_val_;
  vs->preemptions = curr_preemptions_; // PbUpdate is called already
  vs->curr_thread = (curr_action_ ? curr_action_->thd()->uid() : 1); //curr_action is still "previous"
  vs->exec_id = curr_exec_id_;
  vs->state_idx = curr_state_->idx() + 1; //  next state idx
  curr_visited_states_.push_back(vs);
}

bool ChessScheduler::PorVisited(Action *next_action) {
  DEBUG_ASSERT(por_enable_);

  // skip transparent actions
  if (!next_action->obj())
    return false;

  // check whether the state to which the next_action will
  // lead is visted or not
  hash_val_t new_hash_val = HashJoin(curr_hash_val_, Hash(next_action));
  int new_preemptions = curr_preemptions_;
  if (IsPreemptiveChoice(next_action))
    new_preemptions += 1;
  VisitedState::HashMap::iterator hit = visited_states_.find(new_hash_val);
  if (hit != visited_states_.end()) {
    for (VisitedState::Vec::iterator vit = hit->second.begin();
         vit != hit->second.end(); ++vit) {
      VisitedState *vs = *vit;
      Execution *vs_exec = PorGetExec(vs->exec_id);
      State *vs_state = vs_exec->FindState(vs->state_idx);
      DEBUG_ASSERT(vs_state);
      DEBUG_FMT_PRINT_SAFE("matching hash found, val = 0x%lx\n", new_hash_val);
      DEBUG_FMT_PRINT_SAFE("   preemption = %d, exec_id = %d, state_idx = %d\n",
                           vs->preemptions, vs->exec_id, (int)vs->state_idx);
      if (vs->preemptions <= new_preemptions
      //if ((vs->preemptions < new_preemptions || 
      //    (vs->preemptions == new_preemptions && vs->curr_thread == next_action->thd()->uid()))
          &&
          PorStateMatch(curr_state_, next_action, vs_state)) {
        return true;
      }
    }
  }
  return false;
}

bool ChessScheduler::PorStateMatch(State *state,
                                   Action *action,
                                   State *vs_state) {
  DEBUG_ASSERT(state->exec() != vs_state->exec());
  // check whether there will is an one-on-one mapping
  // between actions in the two executions

  // 1) get all actions before vs_state in vs_exec
  ActionHashMap vs_action_hash_table;
  for (State *s = vs_state->Prev(); s; s = s->Prev()) {
    Action *a = s->taken();
    // skip transparent actions
    if (!a->obj())
      continue;
    vs_action_hash_table[Hash(a)].push_back(a);
  }

  // 2) check with all actions in exec
  for (State *s = state; s; s = s->Prev()) {
    Action *a = (s == state ? action : s->taken());
    // skip transparent actions
    if (!a->obj())
      continue;
    ActionHashMap::iterator hit = vs_action_hash_table.find(Hash(a));
    if (hit == vs_action_hash_table.end()) {
      DEBUG_FMT_PRINT_SAFE("   vs hash not found\n");
      DEBUG_FMT_PRINT_SAFE("   %s\n", a->ToString().c_str());
      return false;
    } else {
      bool found = false;
      for (Action::List::iterator lit = hit->second.begin();
           lit != hit->second.end(); ++lit) {
        Action *vs_a = *lit;
        if (a->thd() == vs_a->thd() &&
            a->obj() == vs_a->obj() &&
            a->op() == vs_a->op() &&
            a->inst() == vs_a->inst() &&
            a->tc() == vs_a->tc() &&
            a->oc() == vs_a->oc()) {
          hit->second.erase(lit);
          found = true;
          break;
        }
      }
      if (!found) {
        DEBUG_FMT_PRINT_SAFE("   vs match not found\n");
        return false;
      }
    }
  }
  // two states match when reach here
  return true;
}

Execution *ChessScheduler::PorGetExec(int exec_id) {
  DEBUG_ASSERT(por_enable_);

  // check whether the execution is loaded or not
  // if not, load it from file
  ExecutionTable::iterator it = loaded_execs_.find(exec_id);
  if (it == loaded_execs_.end()) {
    DEBUG_FMT_PRINT_SAFE("loading execution %d\n", exec_id);
    // prepare the directory for por
    PorPrepareDir();
    // load execution from file
    std::stringstream exec_path_ss;
    exec_path_ss << por_info_path_ << '/' << std::dec << exec_id;
    Execution *exec = new Execution;
    exec->Load(exec_path_ss.str().c_str(), sinfo(), program());
    loaded_execs_[exec_id] = exec;
    return exec;
  } else {
    return it->second;
  }
}

void ChessScheduler::PorLoad() {
  DEBUG_ASSERT(por_enable_);
  
  std::cout << "START Loading POR" << std::endl;

  // prepare the directory for por
  PorPrepareDir();

  // load info from file
  ChessPorProto info_proto;
  std::stringstream por_info_path_ss;
  por_info_path_ss << por_info_path_ << "/info";
  std::fstream in(por_info_path_ss.str().c_str(),
                  std::ios::in | std::ios::binary);
  
  google::protobuf::io::IstreamInputStream fs(&in);
      
  google::protobuf::io::CodedInputStream coded_fs(&fs);
  coded_fs.SetTotalBytesLimit(1000000000, -1);
  if (in.is_open()) {
    //info_proto.ParseFromIstream(&in);
    info_proto.ParseFromCodedStream(&coded_fs);
  }
  in.close();
  curr_exec_id_ = info_proto.num_execs() + 1; // initially is zero
  visited_states_.reserve(info_proto.visited_state_size());
  for (int i = 0; i < info_proto.visited_state_size(); i++) {
    ChessPorProto::VisitedStateProto *proto
        = info_proto.mutable_visited_state(i);
    VisitedState *vs = new VisitedState;
    vs->hash_val = proto->hash_val();
    vs->preemptions = proto->preemptions();
    vs->exec_id = proto->exec_id();
    vs->state_idx = proto->state_idx();
    visited_states_[vs->hash_val].push_back(vs);
  }
  std::cout << "END Loading POR" << std::endl;
}

void ChessScheduler::PorSave() {
  DEBUG_ASSERT(por_enable_);

  std::cout << "START Saving POR" << std::endl;
  
  // prepare the directory for por
  PorPrepareDir();

  // save info to file
  ChessPorProto info_proto;
  std::stringstream por_info_path_ss;
  por_info_path_ss << por_info_path_ << "/info";
  info_proto.set_num_execs(curr_exec_id_);
  for (VisitedState::HashMap::iterator hit = visited_states_.begin(),
      end1 = visited_states_.end();
       hit != end1; ++hit) {
    for (VisitedState::Vec::iterator vit = hit->second.begin(),
        end2 = hit->second.end();
         vit != end2; ++vit) {
      VisitedState *vs = *vit;
      ChessPorProto::VisitedStateProto *proto = info_proto.add_visited_state();
      proto->set_hash_val(vs->hash_val);
      proto->set_preemptions(vs->preemptions);
      proto->set_exec_id(vs->exec_id);
      proto->set_state_idx(vs->state_idx);
    }
  }
  for (VisitedState::Vec::iterator vit = curr_visited_states_.begin();
       vit != curr_visited_states_.end(); ++vit) {
    VisitedState *vs = *vit;
    ChessPorProto::VisitedStateProto *proto = info_proto.add_visited_state();
    proto->set_hash_val(vs->hash_val);
    proto->set_preemptions(vs->preemptions);
    proto->set_exec_id(vs->exec_id);
    proto->set_state_idx(vs->state_idx);
  }
  std::fstream out(por_info_path_ss.str().c_str(),
                   std::ios::out | std::ios::trunc | std::ios::binary);
  info_proto.SerializeToOstream(&out);
  out.close();

  // save the current execution to file
  std::stringstream exec_path_ss;
  exec_path_ss << por_info_path_ << '/' << std::dec << curr_exec_id_;
  execution()->Save(exec_path_ss.str().c_str(), sinfo(), program());
  
  std::cout << "END Saving POR" << std::endl;
}

void ChessScheduler::PorPrepareDir() {
  // check whether the path exists. if not, create it
  struct stat sb;
  if (stat(por_info_path_.c_str(), &sb)) {
    // the dir does not exists, create it
    int res = mkdir(por_info_path_.c_str(), 0755);
    assert(!res);
  } else {
    assert(S_ISDIR(sb.st_mode)); // it should be a directory
  }
}

} // namespace systematic

