/*
 * PCTSet.h
 *
 */

#ifndef PCTSET_H_
#define PCTSET_H_

#include <set>
#include <cassert>
#include <vector>
#include <cstddef>

typedef signed int ThreadIndex;
typedef std::set<ThreadIndex> ThreadIndexSet;

class PCTState {
public:
	typedef std::set<PCTState> Set;
	typedef std::vector<ThreadIndex> PriorityList;
	
public:
	PriorityList high;
	PriorityList low;
	size_t numChanges;
	
	size_t size() const {
		return high.size()+low.size();
	}
	
	ThreadIndex atIndex(size_t index) const {
		if(index < high.size()) {
			return high.at(index);
		}
		if(index < size()) {
			return low.at(index-high.size());
		}
		return -1;
	}
	
	static void insertAt(PriorityList& list, size_t pos, ThreadIndex threadIndex) {
		list.insert(list.begin()+pos, threadIndex);
	}
	
	void insertHigh(ThreadIndex threadIndex, Set& result) {
		size_t numPlaces = high.size()+1;
		high.reserve(numPlaces);
		for(size_t i=0; i< numPlaces; ++i) {
			PCTState copy(*this);
			insertAt(copy.high, i, threadIndex);
			result.insert(copy);
		}
	}
	void insertLow(ThreadIndex threadIndex, Set& result) {
		size_t numPlaces = low.size()+1;
		low.reserve(numPlaces);
		for(size_t i=0; i< numPlaces; ++i) {
			PCTState copy(*this);
			insertAt(copy.low, i, threadIndex);
			result.insert(copy);
		}
	}
	
	
	
	bool contains(ThreadIndex threadIndex) const {
		for(ThreadIndex t : high) {
			if(t==threadIndex) {
				return true;
			}
		}
		for(ThreadIndex t : low) {
			if(t==threadIndex) {
				return true;
			}
		}
		return false;
	}
	
	// must have called PCTStates::changePoint first. 
	bool canBeScheduled(ThreadIndex threadIndex, ThreadIndexSet& enabled) const {
//		PCTState copy(*this);
//		for(size_t i=0; i < size(); ++i) {
//			ThreadIndex current = atIndex(i);
//			if(current == threadIndex) {
//				return true;
//			}
//			if(enabled.count(current) == 0) {
//				continue;
//			}
//			return false;
//		}
//		
//		assert(false && "Should not get here.");
//		return false;
		return getHighestEnabled(enabled) == threadIndex;
	}
	
	ThreadIndex removeHighestEnabled(ThreadIndexSet& enabled) {
		for(auto it = high.begin(), end = high.end(); it != end; it++) {
			ThreadIndex thread = *it;
			if(enabled.count(thread) > 0) {
				high.erase(it);
				return thread;
			}
		}
		for(auto it = low.begin(), end = low.end(); it != end; it++) {
			ThreadIndex thread = *it;
			if(enabled.count(thread) > 0) {
				low.erase(it);
				return thread;
			}
		}
		assert(false && "Should not get here.");
		return -1;
	}
	
	ThreadIndex getHighestEnabled(ThreadIndexSet& enabled) const {
		for(size_t i=0; i < size(); ++i) {
			ThreadIndex thread = atIndex(i);
			if(enabled.count(thread) > 0) {
				return thread;
			}
		}
		assert(false && "Could not find enabled thread in PCT. Deadlock?");
		return -1;
	}
	
//	void changePoint(size_t posInLow, ThreadIndexSet& enabled) {
//		ThreadIndex highest = getHighestEnabled(enabled);
//		insertAt(low, posInLow, highest);
//	}
	
	/// Call for each state in a set. Then repeatedly call on new states 
	/// until no new states are added to out.
	void getChangePointSet(ThreadIndexSet& enabled, size_t bound, Set& out) {
		if(numChanges >= bound)
			return;
		PCTState copy(*this);
		copy.numChanges++;
		ThreadIndex highest = copy.removeHighestEnabled(enabled);
		copy.insertLow(highest, out);
	}

	bool isValid(ThreadIndex currThread, ThreadIndexSet& enabled) {
		return getHighestEnabled(enabled) == currThread;
	}
	
	
};


class PCTStates {
private:
	PCTState::Set states;
	size_t bound;
	size_t numThreadsCreated;
public:
	PCTStates(size_t _bound) : bound(_bound), numThreadsCreated(0)
  {}
	//virtual ~PCTStates();

	bool canBeScheduled(ThreadIndex threadIndex, ThreadIndexSet& enabled) {
		for(const PCTState& state: states) {
			if(state.canBeScheduled(threadIndex, enabled)) {
				return true;
			}
		}
		return false;
	}
private:
	void addNewThreadsIfNeeded(size_t numThreadsCreated) {
		
		size_t numNew = numThreadsCreated - this->numThreadsCreated;
		
		if(numNew > 0) {
			PCTState::Set newStates;
			for(PCTState& state : states) {
				// insert several new states into newStates
				for(size_t i=0; i < numNew; ++i) {
					state.insertHigh(this->numThreadsCreated+i, newStates);
				}
			}
			states.swap(newStates);
		}
		this->numThreadsCreated = numThreadsCreated;
	}
public:
	void changePoint(ThreadIndexSet& enabled, size_t numThreadsCreated) {
		addNewThreadsIfNeeded(numThreadsCreated);
		PCTState::Set fromStates;
		PCTState::Set toStates;
		
		fromStates = states;
		while(true) {
			for(PCTState state : fromStates) {
				state.getChangePointSet(enabled, bound, toStates);
			}
			if(toStates.empty()) {
				break;
			}
			states.insert(toStates.begin(), toStates.end());
			toStates.swap(fromStates);
			toStates.clear();
			// continue
		}
	}
	
	void threadScheduled(ThreadIndex thread, ThreadIndexSet& enabled) {
		for(PCTState::Set::iterator it = states.begin(), end = states.end(); it != end; ++it) {
			if(!it->canBeScheduled(thread, enabled)) {
				auto oldIt = it;
				++it;
				states.erase(oldIt);
			}
		}
	}
	
};

#endif /* PCTSET_H_ */
