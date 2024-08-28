#ifndef HLAND_H
#define HLAND_H

#include <vector>
#include <unordered_map>
#include "../sas/sasTask.h"
#include "../planner/state.h"
#include "landmarks.h"

class LandmarkCheck {			// Landmarks reachability for heuristic purposes
private:
	std::vector<TVariable> vars;
	std::vector<TValue> values;
	std::vector<std::weak_ptr<LandmarkCheck>> prev;
	std::vector<std::shared_ptr<LandmarkCheck>> next;
	bool checked;
	bool single;

public:
	LandmarkCheck(LandmarkNode* n);
	void addNext(std::shared_ptr<LandmarkCheck> n);
	void addPrev(std::weak_ptr<LandmarkCheck> n);
	void removeSuccessor(std::shared_ptr<LandmarkCheck> n);
	void removePredecessor(std::shared_ptr<LandmarkCheck> n);
	bool isGoal(std::shared_ptr<SASTask> task);
	bool goOn(std::shared_ptr<TState> s);
	bool isInitialState(std::shared_ptr<TState> state);
	inline void uncheck() { checked = false; }
	inline void check() { checked = true; }
	inline bool isChecked() { return checked; }
	inline bool isSingle() { return single; }
	inline unsigned int numPrev() { return prev.size(); }
	inline unsigned int numNext() { return next.size(); }
	inline TVariable getVar() { return vars[0]; }
	inline TValue getValue() { return values[0]; }
	inline std::shared_ptr<LandmarkCheck> getNext(unsigned int i) { return next[i]; }
	inline std::weak_ptr<LandmarkCheck> getPrev(unsigned int i) { return prev[i]; }
	std::string toString(std::shared_ptr<SASTask> task, bool showNext);
};

class LandmarkHeuristic {		// Landmarks heuristic
private:
	std::shared_ptr<SASTask> task;
	std::vector<std::shared_ptr<LandmarkCheck>> nodes;
	std::vector<std::shared_ptr<LandmarkCheck>> rootNodes;

	void addRootNode(std::shared_ptr<LandmarkCheck> n, std::shared_ptr<TState> state, std::vector<std::shared_ptr<LandmarkCheck>>* toDelete);
	bool hasRootPredecessor(std::shared_ptr<LandmarkCheck> n);

public:
	LandmarkHeuristic(); 
	~LandmarkHeuristic();
	void initialize(std::shared_ptr<SASTask> task, std::vector<std::shared_ptr<SASAction>>* tilActions);
	void initialize(std::shared_ptr<TState> state, std::shared_ptr<SASTask> task, std::vector<std::shared_ptr<SASAction>>* tilActions);
	void uncheckNodes();
	uint16_t evaluate();
	void copyRootNodes(std::vector<std::shared_ptr<LandmarkCheck>>* v);
	std::string toString(std::shared_ptr<SASTask> task);
	inline unsigned int getNumNodes() { return nodes.size(); }
	inline uint16_t countUncheckedNodes() {
		uint16_t n = 0;
		for (unsigned int i = 0; i < nodes.size(); i++)
			if (!nodes[i]->isChecked()) n++;
		return n;
	}
	int getNumInformativeNodes();
};

#endif
