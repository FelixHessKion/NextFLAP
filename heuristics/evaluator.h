#ifndef EVALUATOR_H
#define EVALUATOR_H

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Plan heuristic evaluator                             */
/********************************************************/

/********************************************************/
/* CLASS: Evaluator                                     */
/********************************************************/

#include <deque>
#include "../utils/utils.h"
#include "../sas/sasTask.h"
#include "../planner/plan.h"
#include "../planner/state.h"
#include "../planner/linearizer.h"
#include "../planner/planComponents.h"
#include "hLand.h"
#include <memory>

// Entry of a priority queue to sort the plan timepoints
class ScheduledPoint : public PriorityQueueItem {
public:
	TTimePoint p;
	float time;
	std::shared_ptr<Plan> plan;

	ScheduledPoint(TTimePoint tp, float t, std::shared_ptr<Plan> pl) {
		p = tp;
		time = t;
		plan = pl;
	}
	virtual inline int compare(std::shared_ptr<PriorityQueueItem> other) {
		double otherTime = std::dynamic_pointer_cast<ScheduledPoint>(other)->time;
		if (time < otherTime) return -1;
		else if (time > otherTime) return 1;
		else return 0;
	}
};

// Heuristic evaluator
class Evaluator {
private:
  std::shared_ptr<SASTask> task;
	std::vector<std::shared_ptr<SASAction>>* tilActions;
	PlanComponents planComponents;
	PriorityQueue pq;
	//bool* usefulActions;
  std::unique_ptr<LandmarkHeuristic> landmarks;
	std::vector<std::shared_ptr<LandmarkCheck>> openNodes;				// For hLand calculation
	bool numericConditionsOrConditionalEffects;

	void calculateFrontierState(std::shared_ptr<TState> fs, std::shared_ptr<Plan> currentPlan);
	bool findOpenNode(std::shared_ptr<LandmarkCheck> l);

public:
	Evaluator();
	~Evaluator();
	void initialize(std::shared_ptr<TState> state, std::shared_ptr<SASTask> task, std::vector<std::shared_ptr<SASAction>>* a, bool forceAtEndConditions);
	void calculateFrontierState(std::shared_ptr<Plan> p);
	void evaluate(std::shared_ptr<Plan> p);
	void evaluateInitialPlan(std::shared_ptr<Plan> p);
	std::vector<std::shared_ptr<SASAction>>* getTILActions() { return tilActions; }
	bool informativeLandmarks();
};

#endif
