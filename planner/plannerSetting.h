#ifndef PLANNER_SETTING_H
#define PLANNER_SETTING_H

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Initializes and launches the planner.                */
/********************************************************/

#include "../sas/sasTask.h"
#include "state.h"
#include "plan.h"
#include "planner.h"
#include "../heuristics/rpg.h"
#include <time.h>
#include <memory>

class PlannerSetting {
private:
  std::shared_ptr<SASTask> task;
	clock_t initialTime;
	bool generateTrace;
  std::shared_ptr<Plan> initialPlan;
	std::vector<std::shared_ptr<SASAction>> tilActions;
	bool forceAtEndConditions;
	bool filterRepeatedStates;
	std::shared_ptr<TState> initialState;
	Planner* planner;

	void createInitialPlan();
	std::shared_ptr<SASAction> createInitialAction();
	std::shared_ptr<SASAction> createFictitiousAction(float actionDuration, std::vector<unsigned int>& varList,
		float timePoint, std::string name, bool isTIL, bool isGoal);
	void createTILactions(std::shared_ptr<Plan> parentPlan, std::shared_ptr<Plan> resultPlan);
	bool checkForceAtEndConditions();	// Check if it's required to leave at-end conditions not supported for some actions
	bool checkRepeatedStates();

public:
	PlannerSetting(std::shared_ptr<SASTask> task);
	std::shared_ptr<Plan> plan(float bestMakespan, clock_t startTime);
};

#endif
