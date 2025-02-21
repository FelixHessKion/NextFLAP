#ifndef TEMPORALRPG_H
#define TEMPORALRPG_H

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Temporal relaxed planning graph.						*/
/********************************************************/

#include <unordered_map>
#include "../utils/utils.h"
#include "../utils/priorityQueue.h"
#include "../sas/sasTask.h"
#include "../planner/state.h"

class FluentLevel : public PriorityQueueItem {		// Level of a(sub)goal
public:
	TVariable variable;
	TValue value;
	float level;
	FluentLevel(TVariable var, TValue val, float lev) {
		variable = var;
		value = val;
		level = lev;
	}
	virtual inline int compare(std::shared_ptr<PriorityQueueItem> other) {
		float otherLevel = std::dynamic_pointer_cast<FluentLevel>(other)->level;
		if (level < otherLevel) return -1;
		else if (level > otherLevel) return 1;
		else return 0;
	}
	std::string toString(std::shared_ptr<SASTask> task) {
		return "(" + task->variables[variable].name + "," + task->values[value].name + ") -> " + std::to_string(level);
	}
};

class LMFluent {	// Landmark literal
public:
	TVariable variable;
	TValue value;
	float level;
	int index;
	bool isGoal;
	std::vector<std::shared_ptr<SASAction>> producers;
	LMFluent() { }
	LMFluent(std::shared_ptr<LMFluent> cp) {
		variable = cp->variable;
		value = cp->value;
		level = cp->level;
		index = cp->index;
		isGoal = cp->isGoal;
	}
	void initialize(TVarValue vv, float lev, int i) {
		variable = SASTask::getVariableIndex(vv);
		value = SASTask::getValueIndex(vv);
		level = lev;
		index = i;
		isGoal = false;
	}
	std::string toString(std::shared_ptr<SASTask> task) {
		return "[" + std::to_string(index) + "]: (" + task->variables[variable].name + "," +
			task->values[value].name + ") -> " + std::to_string(level);
	}
};

class TemporalRPG {
private:
	std::shared_ptr<SASTask> task;
	int numActions;
	std::unordered_map<TVarValue, float> firstGenerationTime;
	PriorityQueue qPNormal;
	bool untilGoals;
	std::vector<TVarValue> goalsToAchieve;
	bool verifyFluent;
	LMFluent fluentToVerify;
  std::unique_ptr<char[]> visitedAction;
	std::vector<std::shared_ptr<LMFluent>> fluentList;						// List of fluents (ComputeLiterals)
	std::unordered_map<TVarValue, int> fluentIndex;			// [variable,value] -> fluent index
	std::vector< std::vector<TVarValue> > fluentLevels;		// List of fluents at each level
	std::unordered_map<float, int> fluentLevelIndex;		// Time -> level
  std::unique_ptr<float[]> actionLevels;									// Starting time of each action
	std::vector<std::shared_ptr<SASAction>>* tilActions;

	void addGoalToAchieve(SASCondition& c);
	void init(std::shared_ptr<TState> state);
	inline float getFirstGenerationTime(TVariable v, TValue value) {
		return getFirstGenerationTime(SASTask::getVariableValueCode(v, value));
	}
	inline float getFirstGenerationTime(TVarValue vv) {
		std::unordered_map<TVarValue, float>::const_iterator got = firstGenerationTime.find(vv);
		if (got == firstGenerationTime.end()) return -1;
		else return got->second;
	}
	bool checkAcheivedGoals();
	bool actionProducesFluent(std::shared_ptr<SASAction> a);
	void clearPriorityQueue();
	float getActionLevel(std::shared_ptr<SASAction> a, std::shared_ptr<TState> state);
	void programAction(std::shared_ptr<SASAction> a, std::shared_ptr<TState> state);

public:
	void initialize(bool untilGoals, std::shared_ptr<SASTask> task, std::vector<std::shared_ptr<SASAction>>* tilActions);
	~TemporalRPG();
	void build(std::shared_ptr<TState> state);
	void computeLiteralLevels();
	void computeActionLevels(std::shared_ptr<TState> state);
	inline unsigned int getFluentListSize() { return (unsigned int)fluentList.size(); }
	inline unsigned int getNumFluentLevels() { return (unsigned int)fluentLevels.size(); }
	inline int getFluentIndex(TVariable v, TValue value) {
		std::unordered_map<TVarValue, int>::const_iterator got = fluentIndex.find(SASTask::getVariableValueCode(v, value));
		if (got == fluentIndex.end()) return -1;
		else return got->second;
	}
	inline std::shared_ptr<LMFluent> getFluentByIndex(int index) { return fluentList[index]; }
	inline int getLevelIndex(float level) {
		std::unordered_map<float, int>::const_iterator got = fluentLevelIndex.find(level);
		if (got == fluentLevelIndex.end()) return -1;
		else return got->second;
	}
};

#endif
