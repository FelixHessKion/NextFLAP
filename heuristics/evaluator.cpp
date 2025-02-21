#include "evaluator.h"
#include <time.h>
#include "numericRPG.h"
#include "hFF.h"
using namespace std;

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Plan heuristic evaluator                             */
/********************************************************/

/********************************************************/
/* CLASS: Evaluator                                     */
/********************************************************/

// Evaluates a plan. Its heuristic value is stored in the plan (p->h)
void Evaluator::evaluate(std::shared_ptr<Plan> p) {
	int limit = p->parentPlan.lock()->h;
	if (numericConditionsOrConditionalEffects) {
		NumericRPG rpg(p->fs, tilActions, task, limit);
		p->h = rpg.evaluate();
	}
	else {
		FF_RPG rpg(p->fs, tilActions, task);
		p->h = rpg.evaluate();
	}
	if (landmarks != nullptr)
	p->hLand = landmarks->countUncheckedNodes();
}

// Evaluates the initial plan. Its heuristic value is stored in the plan (p->h)
void Evaluator::evaluateInitialPlan(std::shared_ptr<Plan> p)
{
	int numActions = (int)task->actions.size(), limit = 100;
	//usefulActions = new bool[numActions];
	//for (int i = 0; i < numActions; i++) usefulActions[i] = false;
	NumericRPG rpg(p->fs, tilActions, task, limit);
	p->h = rpg.evaluateInitialPlan(/*usefulActions*/);
}

bool Evaluator::informativeLandmarks()
{
	return landmarks != nullptr && landmarks->getNumInformativeNodes() > 0;
}

// Calculates the frontier state of a given plan. It also computes the number of useful actions included in the plan
void Evaluator::calculateFrontierState(std::shared_ptr<TState> fs, std::shared_ptr<Plan> currentPlan)
{
	if (landmarks != nullptr) {
		landmarks->uncheckNodes();
		landmarks->copyRootNodes(&openNodes);
	}
	std::unordered_set<int> visitedActions;
	pq.clear();
	for (unsigned int i = 1; i < planComponents.size(); i++) {
		std::shared_ptr<Plan> p = planComponents.get(i);
		pq.add(std::make_shared<ScheduledPoint>(stepToStartPoint(i), p->startPoint.updatedTime, p));
		pq.add(std::make_shared<ScheduledPoint>(stepToEndPoint(i), p->endPoint.updatedTime, p));
		if (!p->action->isTIL && !p->action->isGoal) {
			int index = p->action->index;
			if (visitedActions.find(index) == visitedActions.end()) {
				visitedActions.insert(index);
				/*
				if (usefulActions[index])
					currentPlan->numUsefulActions++;*/
			}
		}
	}
	std::shared_ptr<LandmarkCheck> l, al;
	while (pq.size() > 0) {
		std::shared_ptr<ScheduledPoint> p = std::dynamic_pointer_cast<ScheduledPoint>(pq.poll());
		std::shared_ptr<SASAction> a = p->plan->action;
		bool atStart = (p->p & 1) == 0;
		std::vector<SASCondition>* eff = atStart ? &a->startEff : &a->endEff;
    std::shared_ptr<std::vector<TFluentInterval>> numEff = atStart ? p->plan->startPoint.numVarValues : p->plan->endPoint.numVarValues;
		for (SASCondition& c : *eff) {
			fs->state[c.var] = c.value;
		}
		if (p->plan->holdCondEff != nullptr) {
			for (int numCondEff : *p->plan->holdCondEff) {
				SASConditionalEffect& ce = a->conditionalEff[numCondEff];
				eff = atStart ? &ce.startEff: &ce.endEff;
				for (SASCondition &c : *eff) {
					fs->state[c.var] = c.value;
				}
			}
		}
		if (numEff != nullptr) {
			for (TFluentInterval& f : *numEff) {
				fs->minState[f.numVar] = f.interval.minValue;
				fs->maxState[f.numVar] = f.interval.maxValue;
			}
		}
		if (landmarks != nullptr) {
			int j = 0;
			while (j < openNodes.size()) {
				l = openNodes[j];
				//cout << "Landmark " << l->toString(task, false) << endl;
				if (l->goOn(fs)) {	// The landmark holds in the state and we can progress
					l->check();
					openNodes.erase(openNodes.begin() + j); // Remove node from the open nodes list
					for (unsigned int k = 0; k < l->numNext(); k++) { // Go to the adjacent nodes
						al = l->getNext(k);
						if (!al->isChecked() && !findOpenNode(al)) {
							openNodes.push_back(al); // Non-visited node -> append to open nodes
						}
					}
				}
				else {
					j++;
				}
			}
		}
	}
	/*
		for (int i = 0; i < fs->numSASVars; i++) {
			cout << task->variables[i].name << "=" << task->values[fs->state[i]].name << endl;
		}
	*/
}

bool Evaluator::findOpenNode(std::shared_ptr<LandmarkCheck> l)
{
	for (unsigned int i = 0; i < openNodes.size(); i++) {
		if (openNodes[i] == l) return true;
	}
	return false;
}

Evaluator::Evaluator()
{
}

// Destroyer
Evaluator::~Evaluator()
{
	//delete[] usefulActions;
}

// Evaluator initialization
void Evaluator::initialize(std::shared_ptr<TState> state, std::shared_ptr<SASTask> task, std::vector<std::shared_ptr<SASAction>>* a, bool forceAtEndConditions) {
    this->task = task;
	numericConditionsOrConditionalEffects = false;
	for (std::shared_ptr<SASAction> a : task->actions) {
		if (a->startNumCond.size() > 0 || a->overNumCond.size() > 0 || a->endNumCond.size() > 0) {
			numericConditionsOrConditionalEffects = true;
			break;
		}
		if (a->conditionalEff.size() > 0) {
			numericConditionsOrConditionalEffects = true;
			break;
		}
	}
	tilActions = a;
	landmarks = std::make_unique<LandmarkHeuristic>();
	if (state == nullptr) landmarks->initialize(task, a);
	else landmarks->initialize(state, task, a);
	//cout << ";" << landmarks->getNumNodes() << " landmarks (" << landmarks->getNumInformativeNodes() << " informative)" << endl;
	if (landmarks->getNumInformativeNodes() <= 0) {
		landmarks = nullptr;
		SIGNIFICATIVE_LANDMARKS = false;
	}
	else {
		SIGNIFICATIVE_LANDMARKS = true;
	}
}

// Calculates the frontier state of a given plan. This state is stored in the plan (p->fs)
void Evaluator::calculateFrontierState(std::shared_ptr<Plan> p)
{
	//p->numUsefulActions = 0;
	planComponents.calculate(p);
	p->fs = std::make_shared<TState>(task);			// Make a copy of the initial state
	calculateFrontierState(p->fs, p);
}
