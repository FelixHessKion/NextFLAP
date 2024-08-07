#ifndef PLANNER_H
#define PLANNER_H

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Planning search.										*/
/********************************************************/

#include "../sas/sasTask.h"
#include "state.h"
#include "plan.h"
#include "successors.h"
#include "selector.h"

// #define _DEBUG true

class Planner {
private:
	std::shared_ptr<SASTask> task;
	std::shared_ptr<Plan> initialPlan;
	TState* initialState;
	bool forceAtEndConditions;
	bool filterRepeatedStates;
	bool generateTrace;
	Planner* parentPlanner;
	unsigned int expandedNodes;
	Successors* successors;
	std::vector<std::shared_ptr<SASAction>>* tilActions;
	float initialH;
	std::shared_ptr<Plan> solution;
	std::vector<std::shared_ptr<Plan>> sucPlans;
	SearchQueue* selector;
	clock_t startTime;
	float bestMakespan;
	int bestNumSteps;
	int bestH;

	bool emptySearchSpace();
	void searchStep();
	void expandBasePlan(std::shared_ptr<Plan> base);
	void addSuccessors(std::shared_ptr<Plan> base);
	bool checkPlan(std::shared_ptr<Plan> p);
	void markAsInvalid(std::shared_ptr<Plan> p);
	void markChildrenAsInvalid(std::shared_ptr<Plan> p);

public:
	Planner(std::shared_ptr<SASTask> task, std::shared_ptr<Plan> initialPlan, TState* initialState, bool forceAtEndConditions,
		bool filterRepeatedStates, bool generateTrace, std::vector<std::shared_ptr<SASAction>>* tilActions);
	std::shared_ptr<Plan> plan(float bestMakespan, clock_t startTime);
	void clearSolution();
};

#endif
