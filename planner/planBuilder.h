#ifndef PLAN_BUILDER_H
#define PLAN_BUILDER_H

#include "../utils/utils.h"
#include "../utils/priorityQueue.h"
#include "../sas/sasTask.h"
#include "planEffects.h"
#include "plan.h"
#include "intervalCalculations.h"

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Classes for building a new plan (search node).       */
/********************************************************/

// Class to represent a causal link
class PlanBuilderCausalLink {
public:
	TOrdering ordering;					// New orderings (first time point [lower 16 bits] -> second time point [higher 16 bits])
	TVarValue varValue;					// Variable [lower 16 bits] = value (higher 16 bits)
	inline TTimePoint firstPoint() {
		return ordering & 0xFFFF;
	}
	inline TTimePoint secondPoint() {
		return ordering >> 16;
	}
	inline TVariable getVar() {
		return varValue >> 16;
	}
	inline TValue getValue() {
		return varValue & 0xFFFF;
	}
	PlanBuilderCausalLink();
	PlanBuilderCausalLink(TVariable var, TValue v, TTimePoint p1, TTimePoint p2);
	PlanBuilderCausalLink(TVarValue vv, TTimePoint p1, TTimePoint p2);
};

// Timepoint that needs to be delayed
class PBTimepointToDelay : public PriorityQueueItem {
public:
	TTimePoint tp;
	TFloatValue newTime;
	int linearOrderIndex;

	PBTimepointToDelay(TTimePoint p, TFloatValue t, int i) {
		tp = p;
		newTime = t;
		linearOrderIndex = i;
	}
	inline int compare(std::shared_ptr<PriorityQueueItem> other) {
		int res = linearOrderIndex - std::dynamic_pointer_cast<PBTimepointToDelay>(other)->linearOrderIndex;
		if (res == 0) {
			if (newTime < std::dynamic_pointer_cast<PBTimepointToDelay>(other)->newTime) return 1;
			else return -1;
		}
		else return res;
	}
	virtual ~PBTimepointToDelay() { }
};

// Plan builder
class PlanBuilder {
private:
	std::shared_ptr<SASTask> task;
	unsigned int iteration;
	std::vector< std::vector<unsigned int> >* matrix;
	std::vector<TTimePoint> prevPoints;	// For internal calculations
	std::vector<TTimePoint> nextPoints;	// For internal calculations
	PlanEffects* planEffects;

	inline bool existOrder(TTimePoint t1, TTimePoint t2) { return (*matrix)[t1][t2] == iteration; }
	inline void setOrder(TTimePoint t1, TTimePoint t2) { (*matrix)[t1][t2] = iteration; }
	inline void clearOrder(TTimePoint t1, TTimePoint t2) { (*matrix)[t1][t2] = 0; }
	void addCausalLinkToPlan(std::shared_ptr<Plan> p, TTimePoint p1, TTimePoint p2, TVarValue varValue);
	void addNumericCausalLinkToPlan(std::shared_ptr<Plan> p, TTimePoint p1, TTimePoint p2, TVariable var);
	void setActionStartTime(std::shared_ptr<Plan> p);
	bool checkFollowingSteps(std::shared_ptr<Plan> p, std::vector<TTimePoint>& linearOrder);
	bool invalidTILorder(TTimePoint p1, TTimePoint p2);
	void topologicalOrder(std::vector<TTimePoint>* linearOrder);
	unsigned int topologicalOrder(TTimePoint orig, std::vector<TTimePoint>* linearOrder, unsigned int pos,
		std::vector<bool>* visited);
	bool delaySteps(std::shared_ptr<Plan> p, std::vector<TTimePoint>& pointToDelay, std::vector<TFloatValue>& newTime,
		std::vector<TTimePoint>& linearOrder);

public:
	std::shared_ptr<SASAction> action;					// New action added
	unsigned int currentPrecondition;
	unsigned int currentEffect;
	unsigned int setPrecondition;
	TTimePoint lastTimePoint;
	std::vector<PlanBuilderCausalLink> causalLinks;
	std::vector<unsigned int> numOrderingsAdded;
	std::vector<TOrdering> orderings;
	std::vector<unsigned int> openCond;
	int numSupportState;
	std::shared_ptr<bool[]> condEffHold;

	PlanBuilder(std::shared_ptr<SASAction> a, TStep lastStep, std::vector< std::vector<unsigned int> >* matrix,
		int numSupportState, PlanEffects* planEffects, std::shared_ptr<SASTask> task);
	~PlanBuilder();
	bool addLink(SASCondition* c, TTimePoint p1, TTimePoint p2);
	bool addNumLink(TVariable v, TTimePoint p1, TTimePoint p2);
	bool addOrdering(TTimePoint p1, TTimePoint p2);
	void removeLastLink();
	void removeLastOrdering();
	std::shared_ptr<Plan> generatePlan(std::shared_ptr<Plan> basePlan, TPlanId idPlan);
};

#endif
