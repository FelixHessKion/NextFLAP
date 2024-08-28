#include "hLand.h"

//#define DEBUG_HLAND_ON

using namespace std;

/*******************************************/
/* LandmarkCheck                           */
/*******************************************/

LandmarkCheck::LandmarkCheck(LandmarkNode* n) {
	unsigned int numFluents = n->getNumFluents();
	single = numFluents == 1;
	for (unsigned int i = 0; i < numFluents; i++) {
		vars.push_back(n->getVariable(i));
		values.push_back(n->getValue(i));
	}
	checked = false;
}

void LandmarkCheck::addNext(std::shared_ptr<LandmarkCheck> n) {
	next.push_back(n);
}

void LandmarkCheck::addPrev(std::weak_ptr<LandmarkCheck> n) {
	prev.push_back(n);
}

string LandmarkCheck::toString(std::shared_ptr<SASTask> task, bool showNext) {
	std::string res = "(";
	res += task->variables[getVar()].name + "=" + task->values[getValue()].name;
	if (!single) {
		for (unsigned int i = 1; i < vars.size(); i++) {
			res += "," + task->variables[vars[i]].name + "=" + task->values[values[i]].name;
		}
	}
	res += ")";
	if (checked) res += " checked";
	res += " Next: " + to_string(next.size());
	if (showNext) {
		for (unsigned int i = 0; i < next.size(); i++) {
			res += "\n\t-> " + next[i]->toString(task, false);
		}
	}
	return res;
}

void LandmarkCheck::removePredecessor(std::shared_ptr<LandmarkCheck> n) {
	unsigned int i = 0;
	while (i < prev.size()) {
		if (prev[i].lock() == n) {
			//cout << "  Deleted predecessor" << endl;
			prev.erase(prev.begin() + i);
		}
		else {
			i++;
		}
	}
}

void LandmarkCheck::removeSuccessor(std::shared_ptr<LandmarkCheck> n) {
	unsigned int i = 0;
	while (i < next.size()) {
		if (next[i] == n) {
			next.erase(next.begin() + i);
		}
		else {
			i++;
		}
	}
}

bool LandmarkCheck::isGoal(std::shared_ptr<SASTask> task) {
	if (vars.size() != 1) return false;
	TVariable v = getVar();
	TValue value = getValue();
	for (unsigned int i = 0; i < task->goals.size(); i++) {
		std::shared_ptr<SASAction> g = task->goals[i];
		for (unsigned int j = 0; j < g->startCond.size(); j++)
			if (v == g->startCond[j].var && value == g->startCond[j].value) return true;
		for (unsigned int j = 0; j < g->overCond.size(); j++)
			if (v == g->overCond[j].var && value == g->overCond[j].value) return true;
		for (unsigned int j = 0; j < g->endCond.size(); j++)
			if (v == g->endCond[j].var && value == g->endCond[j].value) return true;
	}
	return false;
}

bool LandmarkCheck::isInitialState(std::shared_ptr<TState> state) {
	if (isSingle()) {
		return state->state[getVar()] == getValue();
	}
	else {
		for (unsigned int i = 0; i < vars.size(); i++) {
			if (state->state[vars[i]] == values[i])
				return true;
		}
		return false;
	}
}

bool LandmarkCheck::goOn(std::shared_ptr<TState> s) {
	for (unsigned int i = 0; i < vars.size(); i++) {
		if (s->state[vars[i]] == values[i])
			return true;
	}
	return false;
}


/*******************************************/
/* LandmarkHeuristic                       */
/*******************************************/

LandmarkHeuristic::LandmarkHeuristic() {
	this->task = nullptr;
}

LandmarkHeuristic::~LandmarkHeuristic() {
}

void LandmarkHeuristic::initialize(std::shared_ptr<SASTask> task, std::vector<std::shared_ptr<SASAction>>* tilActions) {
	this->task = task;
  std::shared_ptr<TState> state = std::make_shared<TState>(task);
	initialize(state, task, tilActions);
}

void LandmarkHeuristic::initialize(std::shared_ptr<TState> state, std::shared_ptr<SASTask> task, std::vector<std::shared_ptr<SASAction>>* tilActions) {
	this->task = task; 
  std::shared_ptr<Landmarks> landmarks = std::make_shared<Landmarks>(state, task, tilActions);
	landmarks->filterTransitiveOrders(task);
#ifdef DEBUG_HLAND_ON
	cout << landmarks->toString(task) << endl;
#endif
	unsigned int numLandNodes = landmarks->numNodes();
	//cout << ";" << numLandNodes << " landmarks" << endl;
	for (unsigned int i = 0; i < numLandNodes; i++) {
    std::shared_ptr<LandmarkCheck> l = std::make_shared<LandmarkCheck>(landmarks->getNode(i));
		nodes.push_back(l);
	}
	for (unsigned int i = 0; i < numLandNodes; i++) {
		LandmarkNode* ln = landmarks->getNode(i);
		unsigned int numAdj = ln->numAdjacents();
		for (unsigned int j = 0; j < numAdj; j++) {
			int adjIndex = ln->getAdjacent(j)->getIndex();
			nodes[ln->getIndex()]->addNext(nodes[adjIndex]);
			nodes[adjIndex]->addPrev(nodes[ln->getIndex()]);
		}
	}
	std::vector<std::shared_ptr<LandmarkCheck>> toDelete;
	for (unsigned int i = 0; i < numLandNodes; i++) {
		if (nodes[i]->numPrev() == 0) {
			addRootNode(nodes[i], state, &toDelete);
		}
	}
	for (unsigned int i = 0; i < toDelete.size(); i++) {
		std::shared_ptr<LandmarkCheck> n = toDelete[i];
		//cout << "Deleting node: " << n->toString(task, false) << endl;
		for (unsigned int j = 0; j < n->numPrev(); j++)
			n->getPrev(j).lock()->removeSuccessor(n);
		for (unsigned int j = 0; j < n->numNext(); j++)
			n->getNext(j)->removePredecessor(n);
		for (unsigned int j = 0; j < nodes.size(); j++) {
			if (nodes[j] == n) {
				nodes.erase(nodes.begin() + j);
				break;
			}
		}
	}
	unsigned int i = 0;
	while (i < rootNodes.size()) {
		//cout << rootNodes[i]->toString(task, false) << endl;
		if (hasRootPredecessor(rootNodes[i])) {
			rootNodes.erase(rootNodes.begin() + i);
		}
		else {
			i++;
		}
	}
	/*
	for (i = 0; i < rootNodes.size(); i++) {
		cout << "Root node: " << rootNodes[i]->toString(task, false) << endl;
	}*/
}

bool LandmarkHeuristic::hasRootPredecessor(std::shared_ptr<LandmarkCheck> n) {
	for (unsigned int i = 0; i < n->numPrev(); i++) {
		std::shared_ptr<LandmarkCheck> p = n->getPrev(i).lock();
		bool found = false;
		for (unsigned int j = 0; j < rootNodes.size(); j++) {
			if (rootNodes[j] == p) {
				found = true;
				break;
			}
		}
		if (found) return true;
		if (hasRootPredecessor(p)) return true;
	}
	return false;
}

void LandmarkHeuristic::addRootNode(std::shared_ptr<LandmarkCheck> n, std::shared_ptr<TState> state, std::vector<std::shared_ptr<LandmarkCheck>>* toDelete) {
	bool found = false;
	if (n->goOn(state)) {
		for (unsigned int i = 0; i < toDelete->size(); i++) {
			if (toDelete->at(i) == n) {
				found = true;
				break;
			}
		}
		if (!found) toDelete->push_back(n);
		for (unsigned int i = 0; i < n->numNext(); i++) {
			addRootNode(n->getNext(i), state, toDelete);
		}
	} else {
		for (unsigned int i = 0; i < rootNodes.size(); i++) {
			if (rootNodes[i] == n) {
				found = true;
				break;
			}
		}
		if (!found) rootNodes.push_back(n);
	}
}

void LandmarkHeuristic::uncheckNodes() {
	for (unsigned int i = 0; i < nodes.size(); i++) {
		//cout << nodes[i]->toString(task, false) << " unchecked" << endl;
		nodes[i]->uncheck();
	}
}

uint16_t LandmarkHeuristic::evaluate() {
	uint16_t h = 0;
	for (unsigned int i = 0; i < nodes.size(); i++) {
		if (!nodes[i]->isChecked())
			h++;
	}
	return h;
}

void LandmarkHeuristic::copyRootNodes(std::vector<std::shared_ptr<LandmarkCheck>>* v) {
	v->clear();
	v->insert(v->end(), rootNodes.begin(), rootNodes.end());
}

std::string LandmarkHeuristic::toString(std::shared_ptr<SASTask> task) {
	std::string res = "LANDMARKS:\n";
	for (unsigned int i = 0; i < nodes.size(); i++) {
		res += "* " + nodes[i]->toString(task, true) + "\n";
	}
	return res;
}


int LandmarkHeuristic::getNumInformativeNodes() {
	int n = 0;
	for (unsigned int i = 0; i < nodes.size(); i++) {
		if (nodes[i]->isSingle() && !nodes[i]->isGoal(task))
			n++;
	}
	return n;
}

