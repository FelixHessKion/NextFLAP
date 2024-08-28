#include "planner.h"
#include "z3Checker.h"
#include "printPlan.h"

#include <iomanip>

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Planning search.										*/
/********************************************************/

using namespace std;

// Checks if the search space is empty
bool Planner::emptySearchSpace()
{
	return selector->size() == 0;
}

// Constructor
Planner::Planner(std::shared_ptr<SASTask> task, std::shared_ptr<Plan> initialPlan, std::shared_ptr<TState> initialState, bool forceAtEndConditions,
    bool filterRepeatedStates, bool generateTrace, std::vector<std::shared_ptr<SASAction>>* tilActions)
{
	this->bestH = MAX_INT32;
	this->task = task;
	this->initialPlan = initialPlan;
	this->initialState = initialState;
	this->forceAtEndConditions = forceAtEndConditions;
	this->filterRepeatedStates = filterRepeatedStates;
	cout << ";Filter repeated states: " << filterRepeatedStates << endl;
	this->parentPlanner = parentPlanner;
	this->expandedNodes = 0;
	this->generateTrace = generateTrace;
	this->tilActions = tilActions;
    successors = std::make_unique<Successors>(initialState, task, forceAtEndConditions, filterRepeatedStates, tilActions);
	this->initialH = FLOAT_INFINITY;
	this->solution = nullptr;
	selector = std::make_unique<SearchQueue>();
	successors->evaluator.calculateFrontierState(this->initialPlan);
	/*
	selector->addQueue(SEARCH_NRPG);
	if (successors->evaluator.informativeLandmarks()) {	// Landmarks available
		selector->addQueue(SEARCH_LAND);
	}*/
	selector->add(this->initialPlan);
	successors->evaluator.evaluateInitialPlan(initialPlan);
}

// Starts the search
std::shared_ptr<Plan> Planner::plan(float bestMakespan, clock_t startTime)
{
	this->startTime = startTime;
	this->bestMakespan = bestMakespan;
	while (solution == nullptr && this->selector->size() > 0) {
		if (toSeconds(startTime) > 600) break;
		searchStep();
	}
	return solution;
}

// Clears the last solution found
void Planner::clearSolution()
{
	solution = nullptr;
	successors->solution = nullptr;
}

// Checks if a plan is valid
bool Planner::checkPlan(std::shared_ptr<Plan> p) {
	Z3Checker checker;
	cout << ".";
	p->z3Checked = true;
	bool valid = checker.checkPlan(p, false);
	return valid;
}

// Marks a plan as invalid
void Planner::markAsInvalid(std::shared_ptr<Plan> p)
{
	markChildrenAsInvalid(p);
	std::shared_ptr<Plan> parent = p->parentPlan.lock();
	if (parent != nullptr && !parent->isRoot() && !parent->z3Checked) {
		if (!checkPlan(parent)) {
			markAsInvalid(parent);
		}
	}
}

// Marks a children plan as invalid
void Planner::markChildrenAsInvalid(std::shared_ptr<Plan> p) {
	if (p->childPlans != nullptr) {
		for (std::shared_ptr<Plan> child : *(p->childPlans)) {
			child->invalid = true;
			markChildrenAsInvalid(child);
		}
	}
}

// Makes one search step
void Planner::searchStep() {
	std::shared_ptr<Plan> base = selector->poll();
	float baseMakespan = PrintPlan::getMakespan(base);
	if (baseMakespan >= bestMakespan)
		return;

#if _DEBUG
        cout << "Base plan: " << right << setw(2) << setfill(' ') << base->id
             << ", " << left << setw(50) << setfill(' ') << base->action->name
             << "(G = " << right << setw(6) << setfill(' ') << base->g
             << ", H=" << right << setw(6) << setfill(' ') << base->h << ")"
             << endl;
#endif
	if (!base->invalid && !successors->repeatedState(base)) {
		if (base->action->startNumCond.size() > 0 || 
			base->action->overNumCond.size() > 0 || 
			base->action->endNumCond.size() > 0) {
			if (base->h <= 1) {			// <=
				if (!checkPlan(base)) {	// Validity checking
					return;
				}
			}
		}
		if (base->h < bestH) {
			cout << ";H: " << base->h << " (" << base->hLand << ")" << endl;
			bestH = base->h;
		}
		expandBasePlan(base);
		addSuccessors(base);		
	}
}

// Expands the current base plan
void Planner::expandBasePlan(std::shared_ptr<Plan> base)
{
	if (base->expanded()) {
		sucPlans.clear();
		return;
	}
	successors->computeSuccessors(base, &sucPlans, bestMakespan);
	++expandedNodes;
	if (successors->solution != nullptr) {
		//PrintPlan::print(successors->solution);
		if (checkPlan(successors->solution)) {
			solution = successors->solution;
		}
		else {
			markAsInvalid(successors->solution);
			successors->solution = nullptr;
		}
	}
}

// Adds the successor plans to the selector
void Planner::addSuccessors(std::shared_ptr<Plan> base)
{
	base->addChildren(sucPlans);
	if (solution == nullptr) {
		for (std::shared_ptr<Plan> p : sucPlans) {
			//cout << "* Successor: " << p->id << ", " << p->action->name << "(G = " << p->g << ", H=" << p->h << ")" << endl;
			selector->add(p);
		}
	}
}
