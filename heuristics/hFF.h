#ifndef FF_RPG_H
#define FF_RPG_H

#include "../utils/utils.h"
#include "../utils/priorityQueue.h"
#include "../sas/sasTask.h"
#include "../planner/state.h"

#include <memory>

class FF_RPGCondition : public PriorityQueueItem {
public:
	TVariable var;
	TValue value;
	int level;
	FF_RPGCondition(TVariable v, TValue val, int l) {
		var = v;
		value = val;
		level = l;
	}
	inline int compare(std::shared_ptr<PriorityQueueItem> other) {
		return std::dynamic_pointer_cast<FF_RPGCondition>(other)->level - level;
	}
	virtual ~FF_RPGCondition() { }
};

class FF_RPGVarValue {
public:
	TVariable var;
	TValue value;
	FF_RPGVarValue(TVariable var, TValue value);
};

class FF_RPG {
private:
	std::shared_ptr<SASTask> task;
	std::vector< std::vector<int> > literalLevels;
    std::vector<int> actionLevels;
    unsigned int numLevels;
    std::unique_ptr<std::vector<FF_RPGVarValue>> lastLevel;
    std::unique_ptr<std::vector<FF_RPGVarValue>> newLevel;
    std::vector<TVarValue> reachedValues;
    
    void initialize();
    void addEffects(std::shared_ptr<SASAction> a);
    void addEffect(TVariable var, TValue value);
	void expand();
	void addSubgoals(std::vector<TVarValue>* goals, PriorityQueue* openConditions);
	void addSubgoal(TVariable var, TValue value, PriorityQueue* openConditions);
	void addSubgoals(std::shared_ptr<SASAction> a, PriorityQueue* openConditions);
	uint16_t getDifficulty(std::shared_ptr<SASAction> a);
	uint16_t getDifficulty(SASCondition* c);
	void addTILactions(std::vector<std::shared_ptr<SASAction>>* tilActions);
	uint16_t computeHeuristic(PriorityQueue* openConditions);
	void resetReachedValues();
	bool isExecutable(std::shared_ptr<SASAction> a);

public:
	std::vector<std::shared_ptr<SASAction>> relaxedPlan;

	FF_RPG(std::shared_ptr<TState> fs, std::vector<std::shared_ptr<SASAction>>* tilActions, std::shared_ptr<SASTask> task);
	uint16_t evaluate();
};

#endif
