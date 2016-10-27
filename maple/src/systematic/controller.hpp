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

// File: systematic/controller.hpp - The definition of the execution
// controller for systematic concurrency testing.

#ifndef SYSTEMATIC_CONTROLLER_HPP_
#define SYSTEMATIC_CONTROLLER_HPP_

#include "core/basictypes.h"
#include "core/execution_control.hpp"
#include "race/race.h"
#include "race/djit.h"
#include "systematic/scheduler.h"
#include "systematic/random.h"
#include "systematic/pct_random.h"
#include "systematic/chess.h"


namespace systematic {

// The main execution controller for systematic concurrency testing.
class Controller : public ExecutionControl, public ControllerInterface {
 public:
  Controller();
  virtual ~Controller();

  // return the knob used by this controller
  Knob *knob() { return knob_; }

 protected:
  // define the info for a thread's joiners
  class JoinInfo {
   public:
    typedef std::map<thread_id_t, JoinInfo *> Map;
    typedef std::list<thread_id_t> WaitQueue;

    JoinInfo() : exit(false) {}
    ~JoinInfo() {}

    bool exit;
    WaitQueue wait_queue;
  };

  // define a mutex info
  class MutexInfo {
   public:
    typedef std::tr1::unordered_map<address_t, MutexInfo *> Map;
    typedef std::list<thread_id_t> WaitQueue;
    typedef std::map<thread_id_t, bool> ReadyMap;

    MutexInfo() : holder(INVALID_THD_ID), free(false), recursive(-1) {}
    ~MutexInfo() {}

    thread_id_t holder; // the current mutex holder
    WaitQueue wait_queue;
    ReadyMap ready_map;
    bool free;
    int recursive;
  };

  // define a cond info
  class CondInfo {
   public:
    typedef std::tr1::unordered_map<address_t, CondInfo *> Map;
    typedef uint32 signal_id_t;
    typedef std::set<signal_id_t> SignalSet;
    typedef struct {
      bool timed; // whether it is a timed wait
      bool broadcasted; // whether it is broadcasted
      SignalSet signal_set;
    } WaitInfo;
    // have a wait info for each wait/timedwait
    typedef std::map<thread_id_t, WaitInfo> WaitMap;

    CondInfo() : curr_signal_id(0), free(false) {}
    ~CondInfo() {}

    signal_id_t curr_signal_id;
    WaitMap wait_map;
    bool free;
  };

  // define a barrier info
  class BarrierInfo {
   public:
    typedef std::tr1::unordered_map<address_t, BarrierInfo *> Map;
    typedef std::list<thread_id_t> WaitQueue;

    BarrierInfo() : count(0), free(false) {}
    ~BarrierInfo() {}

    unsigned count;
    WaitQueue wait_queue;
    bool free;
  };

  // define a region
  class Region {
   public:
    typedef std::map<address_t, Region *> Map;

    Region() : addr(0), size(0) {}
    virtual ~Region() {}

    address_t addr;
    size_t size;
    MutexInfo::Map mutex_info_table;
    CondInfo::Map cond_info_table;
    BarrierInfo::Map barrier_info_table;
  };

  // define a static region
  class SRegion : public Region {
   public:
    SRegion() : image(NULL) {}
    ~SRegion() {}

    Image *image;
  };

  // define a dynamic region
  class DRegion : public Region {
   public:
    DRegion() : creator(NULL), creator_inst(NULL), creator_idx(0), isFree(false) {}
    ~DRegion() {}

    Thread *creator;
    Inst *creator_inst;
    Object::idx_t creator_idx;
    bool isFree;
  };

  // define creation information for dynamic regions
  class CreationInfo {
   public:
    typedef size_t hash_val_t;
    typedef std::vector<CreationInfo *> Vec;
    typedef std::tr1::unordered_map<hash_val_t, Vec> HashMap;

    CreationInfo()
        : creator_thd_id(INVALID_THD_ID),
          creator_inst(NULL),
          curr_creator_idx(0) {}

    ~CreationInfo() {}

    hash_val_t Hash();
    bool Match(CreationInfo *info);

    thread_id_t creator_thd_id;
    Inst *creator_inst;
    Object::idx_t curr_creator_idx;
  };

  // overrided virtual functions
  virtual void HandlePreSetup();
  virtual void HandlePostSetup();
  virtual void HandlePreInstrumentTrace(TRACE trace);
  virtual void HandleProgramStart();
  virtual void HandleProgramExit();
  virtual void HandleImageLoad(IMG img, Image *image);
  virtual void HandleImageUnload(IMG img, Image *image);
  virtual void HandleThreadStart();
  virtual void HandleThreadExit();
  virtual void HandleSchedulerThread();
  virtual void HandleSchedulerThreadReclaim();
  virtual void HandleSignalReceived(THREADID tid, INT32 sig,
                                      const CONTEXT *ctxt_from, CONTEXT *ctxt_to);
  virtual void HandleSyscallEntry(THREADID tid, CONTEXT *ctxt,
                                    SYSCALL_STANDARD std);
  virtual void HandleSyscallExit(THREADID tid, CONTEXT *ctxt,
                                   SYSCALL_STANDARD std);
  
  virtual void HandleBeforeMemOp(THREADID tid, Inst *inst, address_t addr,
                                     size_t size, bool isWrite);
  
  virtual void HandleBeforeMemRead(THREADID tid, Inst *inst, address_t addr,
                                     size_t size);
  virtual void HandleBeforeMemWrite(THREADID tid, Inst *inst, address_t addr,
                                    size_t size);
  
  virtual void HandleBeforeRaceRead(THREADID tid, Inst *inst,
                                    address_t addr, size_t size);
  virtual void HandleAfterRaceRead(THREADID tid, Inst *inst,
                                   address_t addr, size_t size);
  virtual void HandleBeforeRaceWrite(THREADID tid, Inst *inst,
                                     address_t addr, size_t size);
  virtual void HandleAfterRaceWrite(THREADID tid, Inst *inst,
                                    address_t addr, size_t size);

  // main processing functions
  int MutexTryLock(thread_id_t self, address_t mutex_addr, Inst *inst);
  void MutexLock(thread_id_t self, address_t mutex_addr, Inst *inst);
  void MutexUnlock(thread_id_t self, address_t mutex_addr, Inst *inst);
  void CondSignal(thread_id_t self, address_t cond_addr, Inst *inst);
  void CondBroadcast(thread_id_t self, address_t cond_addr, Inst *inst);
  void CondWait(thread_id_t self, address_t cond_addr, Inst *inst);
  int CondTimedwait(thread_id_t self, address_t cond_addr, Inst *inst);
  void BarrierWait(thread_id_t self, address_t barrier_addr, Inst *inst);
  Action *CreateAction(thread_id_t thd_id,
                       address_t iaddr,
                       Operation op,
                       Inst *inst);
  State *CreateState();
  bool AllThreadsInactive();
  void WaitForNextState();
  State *Execute(State *state, Action *action);
  Action *Schedule(thread_id_t self, address_t iaddr, Operation op, Inst *inst);
  void ScheduleOnExit(thread_id_t self);
  
  Thread::Vec &GetThreadCreationOrder();

  // helper functions
  void SetScheduler(Scheduler *scheduler);
  Knob *GetKnob() { return knob_; }
  StaticInfo *GetStaticInfo() { return sinfo_; }
  Program *GetProgram() { return program_; }
  Execution *GetExecution() { return execution_; }
  void SemWait(Semaphore *sem);
  void SemPost(Semaphore *sem);
  void SetAffinity();
  void SetSchedPolicy();
  void Yield();
  JoinInfo *GetJoinInfo(thread_id_t thd_id);
  MutexInfo *GetMutexInfo(address_t iaddr, Inst *inst);
  CondInfo *GetCondInfo(address_t iaddr, Inst *inst);
  BarrierInfo *GetBarrierInfo(address_t iaddr, Inst *inst);
  Object *GetObject(address_t iaddr, Inst *inst);
  Thread *FindThread(thread_id_t thd_id);
  Region::Map::iterator FindRegion(address_t iaddr, Inst *inst);
  void AllocSRegion(address_t addr, size_t size, Image *image);
  void AllocDRegion(address_t addr, size_t size, Inst *inst);
  bool FreeSRegion(address_t addr);
  bool FreeDRegion(address_t addr);
  void FreeMutexInfo(Region *region);
  void FreeCondInfo(Region *region);
  void FreeBarrierInfo(Region *region);
  Object::idx_t GetCreatorIdx(thread_id_t thd_id, Inst *inst);

  // settings and flags
  Scheduler *scheduler_; // the scheduler that controls the execution
  RandomScheduler *random_scheduler_;
  ChessScheduler *chess_scheduler_;
  PCTRandomScheduler *pct_scheduler_;
  Program *program_; // the modeled program
  Execution *execution_; // the current execution of the modeled program
  race::RaceDB *race_db_;
  race::Djit *djit_analyzer_;
  bool sched_app_; // whether only care about ops in the application
  bool sched_race_; // whether schedule racy memory operations
  address_t unit_size_; // the granularity
  bool check_mem_; // whether to check memory out of bounds
  bool control_cs_;

  // global analysis states
  PIN_THREAD_UID scheduler_thd_uid_; // the pin uid for the scheduler thread
  bool volatile program_exiting_; // whether the program is about to exit
  bool next_state_ready_; // whether the next state is ready
  Semaphore *next_state_sem_; // used to notify the scheduler thread
  std::map<thread_id_t, Semaphore *> perm_sem_table_;
  std::map<thread_id_t, Thread *> thread_table_;
  std::map<Thread *, thread_id_t> thread_reverse_table_;
  std::map<thread_id_t, Action *> action_table_;
  std::map<thread_id_t, bool> enable_table_;
  std::map<thread_id_t, bool> active_table_; // whether in free state
  std::map<thread_id_t, Thread::idx_t> thread_creation_info_;
  CreationInfo::HashMap creation_info_;
  Region::Map region_table_;
  JoinInfo::Map join_info_table_;
  
  Thread::Vec  thread_creation_order_;

  // racy memory op related
  std::map<thread_id_t, bool> race_active_table_;
  address_t tls_race_read_addr_[PIN_MAX_THREADS];
  size_t tls_race_read_size_[PIN_MAX_THREADS];
  address_t tls_race_write_addr_[PIN_MAX_THREADS];
  size_t tls_race_write_size_[PIN_MAX_THREADS];
  address_t tls_race_read2_addr_[PIN_MAX_THREADS];
  
  AFUNPTR pthreadExitFunPtr_;

 private:
  static void __SchedulerThread(VOID *arg);
  static void __SchedulerThreadReclaim(INT32 code, VOID *v);
  static void __BeforeRaceRead(THREADID tid, Inst *inst, ADDRINT addr,
                               UINT32 size);
  static void __AfterRaceRead(THREADID tid, Inst *inst);
  static void __BeforeRaceWrite(THREADID tid, Inst *inst, ADDRINT addr,
                                UINT32 size);
  static void __AfterRaceWrite(THREADID tid, Inst *inst);
  static void __BeforeRaceRead2(THREADID tid, Inst *inst, ADDRINT addr,
                                UINT32 size);
  static void __AfterRaceRead2(THREADID tid, Inst *inst);

  DISALLOW_COPY_CONSTRUCTORS(Controller);

  // Override wrappers.
  DECLARE_MEMBER_WRAPPER_HANDLER(PthreadCreate);
  DECLARE_MEMBER_WRAPPER_HANDLER(PthreadJoin);
  DECLARE_MEMBER_WRAPPER_HANDLER(PthreadMutexTryLock);
  DECLARE_MEMBER_WRAPPER_HANDLER(PthreadMutexLock);
  DECLARE_MEMBER_WRAPPER_HANDLER(PthreadMutexInit);
  DECLARE_MEMBER_WRAPPER_HANDLER(PthreadMutexUnlock);
  DECLARE_MEMBER_WRAPPER_HANDLER(PthreadCondSignal);
  DECLARE_MEMBER_WRAPPER_HANDLER(PthreadCondBroadcast);
  DECLARE_MEMBER_WRAPPER_HANDLER(PthreadCondWait);
  DECLARE_MEMBER_WRAPPER_HANDLER(PthreadCondTimedwait);
  DECLARE_MEMBER_WRAPPER_HANDLER(PthreadBarrierInit);
  DECLARE_MEMBER_WRAPPER_HANDLER(PthreadBarrierWait);

  DECLARE_MEMBER_WRAPPER_HANDLER(Sleep);
  DECLARE_MEMBER_WRAPPER_HANDLER(Usleep);
  DECLARE_MEMBER_WRAPPER_HANDLER(SchedYield);
  
  DECLARE_MEMBER_WRAPPER_HANDLER(Exit);

  DECLARE_MEMBER_WRAPPER_HANDLER(Malloc);
  DECLARE_MEMBER_WRAPPER_HANDLER(Calloc);
  DECLARE_MEMBER_WRAPPER_HANDLER(Realloc);
  DECLARE_MEMBER_WRAPPER_HANDLER(Free);
  DECLARE_MEMBER_WRAPPER_HANDLER(Valloc);
};

} // namespace systematic

#endif

