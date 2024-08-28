#ifndef RPG_H
#define RPG_H

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Non-temporal relaxed planning graph                  */
/********************************************************/


#include "../utils/utils.h"
#include "../utils/priorityQueue.h"
#include "../sas/sasTask.h"
#include "../planner/state.h"

class RPGCondition : public PriorityQueueItem {
public:
	TVariable var;
	TValue value;
	int level;
	RPGCondition(TVariable v, TValue val, int l) {
		var = v;
		value = val;
		level = l;
	}
	inline int compare(std::shared_ptr<PriorityQueueItem> other) {
		return std::dynamic_pointer_cast<RPGCondition>(other)->level - level;
	}
	virtual ~RPGCondition() { }
};

class RPGVarValue {
public:
	TVariable var;
	TValue value;
	RPGVarValue(TVariable var, TValue value);
};

class RPG {
private:
	std::shared_ptr<SASTask> task;
	bool forceAtEndConditions;
	std::vector< std::vector<int> > literalLevels;
	std::vector<int> actionLevels;
	unsigned int numLevels;
  std::unique_ptr<std::vector<RPGVarValue>> lastLevel;
	std::unique_ptr<std::vector<RPGVarValue>> newLevel;
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
	uint16_t getDifficultyWithPermanentMutex(std::shared_ptr<SASAction> a);
	void addTILactions(std::vector<std::shared_ptr<SASAction>>* tilActions);
	void addUsefulAction(std::shared_ptr<SASAction> a, std::vector<std::shared_ptr<SASAction>>* usefulActions);
	uint16_t computeHeuristic(bool mutex, PriorityQueue* openConditions);
	void resetReachedValues();

public:
	std::vector<std::shared_ptr<SASAction>> relaxedPlan;

	RPG(std::vector< std::vector<TValue> >& varValues, std::shared_ptr<SASTask> task, bool forceAtEndConditions,
		std::vector<std::shared_ptr<SASAction>>* tilActions);
	RPG(std::shared_ptr<TState> state, std::shared_ptr<SASTask> task, bool forceAtEndConditions, std::vector<std::shared_ptr<SASAction>>* tilActions);
	bool isExecutable(std::shared_ptr<SASAction> a);
	uint16_t evaluate(bool mutex);
	uint16_t evaluate(TVarValue goal, bool mutex);
	uint16_t evaluate(std::vector<TVarValue>* goals, bool mutex);
	bool isReachable(TVariable v, TValue val) { return literalLevels[v][val] < MAX_INT32; }
};

#endif
