#ifndef LANDMARKS_H
#define LANDMARKS_H

#include <vector>
#include <queue>
#include <algorithm>
#include "../planner/state.h"
#include "temporalRPG.h"
#include <memory>

class LTNode;

class USet {				// Disjunctive landmark
public:
	int id;
	std::vector<std::shared_ptr<LMFluent>> fluentSet;
	std::shared_ptr<LTNode> node;
	int value;

	USet() { }
	USet(std::shared_ptr<USet> s) {
		id = s->id;
		for (unsigned int i = 0; i < s->fluentSet.size(); i++)
			fluentSet.push_back(s->fluentSet[i]);
		node = s->node;
		value = s->value;
	}
	void initialize(std::shared_ptr<LMFluent> l, int fncIndex) {
		id = fncIndex;
		fluentSet.push_back(l);
		node = nullptr;
		value = 0;
	}
	void addElement(std::shared_ptr<LMFluent> l) {
		if (!contains(l)) {
			fluentSet.push_back(l);
		}
	}
	bool contains(std::shared_ptr<LMFluent> l) {
		for (unsigned int i = 0; i < fluentSet.size(); i++) {
			if (fluentSet[i]->index == l->index) {
				return true;
			}
		}
		return false;
	}
	void calculateValue() {
		value = 0;
		for (unsigned int i = 0; i < fluentSet.size(); i++) {
			value += fluentSet[i]->index;
		}
	}
	bool matches(SASCondition& c) {
		for (unsigned int i = 0; i < fluentSet.size(); i++) {
			if (fluentSet[i]->variable == c.var && fluentSet[i]->value == c.value)
				return true;
		}
		return false;
	}
	bool isEqual(std::shared_ptr<USet> u) {
		if (id != u->id || fluentSet.size() != u->fluentSet.size()) return false;
		for (unsigned int i = 0; i < fluentSet.size(); i++) {
			if (fluentSet[i]->variable != u->fluentSet[i]->variable ||
				fluentSet[i]->value != u->fluentSet[i]->value)
				return false;
		}
		return true;
	}
	std::string toString(std::shared_ptr<SASTask> task) {
		std::string res = "[";
		if (!fluentSet.empty()) res += fluentSet[0]->toString(task);
		for (unsigned int i = 1; i < fluentSet.size(); i++) {
			res += "," + fluentSet[i]->toString(task);
		}
		return res + "]";
	}
};

class LTNode {				// Landmark node
private:
  std::shared_ptr<LMFluent> fluent;
	std::shared_ptr<USet> disjunction;
	bool singleLiteral;
	unsigned int index;

public:
	LTNode() { index = MAX_INT32; }
	LTNode(std::shared_ptr<LMFluent> f, unsigned int i) {
		fluent = std::make_shared<LMFluent>(*f);
		disjunction = nullptr;
		singleLiteral = true;
		index = i;
	}
	LTNode(std::shared_ptr<USet> u, unsigned int i) {
		fluent = nullptr;
		disjunction = u;
		singleLiteral = false;
		index = i;
	}
	inline unsigned int getIndex() { return index; }
	inline std::shared_ptr<LMFluent> getFluent() { return fluent; }
	inline std::shared_ptr<USet> getSet() { return disjunction; }
	inline bool single() { return singleLiteral; }
	std::string toString(std::shared_ptr<SASTask> task) {
		if (singleLiteral) return "Node " + std::to_string(index) + ": " + fluent->toString(task);
		else return "Node " + std::to_string(index) + ": " + disjunction->toString(task);
	}
};

class LMOrdering {			// Landmark ordering
public:
	std::shared_ptr<LTNode> node1;
	std::shared_ptr<LTNode> node2;
	int ordType;
	void initialize(std::shared_ptr<LTNode> l1, std::shared_ptr<LTNode> l2, int t) {
		node1 = l1;
		node2 = l2;
		ordType = t;
	}
	std::string toString(std::shared_ptr<SASTask> task) {
		return node1->toString(task) + " -> " + node2->toString(task);
	}
};

class LandmarkRPG {
private:
	std::shared_ptr<SASTask> task;
	std::unordered_map<TVarValue, bool> achievedFluent;
  std::unique_ptr<bool[]> achievedAction;
	std::unique_ptr<std::vector<TVarValue>> lastLevel;
	std::unique_ptr<std::vector<TVarValue>> newLevel;
	std::vector<TVarValue> remainingGoals;

	void initialize(std::shared_ptr<TState> s);
	void addGoal(SASCondition* c);
	inline bool fluentAchieved(TVariable v, TValue value) {
		return fluentAchieved(SASTask::getVariableValueCode(v, value));
	}
	inline bool fluentAchieved(TVarValue vv) {
		return achievedFluent.find(vv) != achievedFluent.end();
	}
	bool isExecutable(std::shared_ptr<SASAction> a, TVariable v, TValue value);
	bool isExecutable(std::shared_ptr<SASAction> a, std::vector<TVariable>* v, std::vector<TValue>* value);
	bool isExecutable(std::shared_ptr<SASAction> a);
	void addActionEffects(std::shared_ptr<SASAction> a);
	void swapLevels();
	void clearMemory();
	bool allowedAction(std::shared_ptr<SASAction> a, std::vector<std::shared_ptr<SASAction>>* actions);

public:
	bool verifyFluent(TVariable v, TValue value, std::shared_ptr<TState> s, std::shared_ptr<SASTask> task);
	bool verifyFluents(std::vector<TVariable>* v, std::vector<TValue>* value, std::shared_ptr<TState> s, std::shared_ptr<SASTask> task);
	bool verifyActions(std::vector<std::shared_ptr<SASAction>>* actions, std::shared_ptr<TState> s, std::shared_ptr<SASTask> task);
};

class LandmarkTree {
private:
	std::shared_ptr<TState> state;
	TemporalRPG rpg;
	std::shared_ptr<SASTask> task;
	std::vector<int> fluentNode;
	std::vector<std::vector<std::shared_ptr<LMFluent>>> objs;
	std::vector< std::vector< std::shared_ptr<USet> > > disjObjs;
	bool** reasonableOrderings;
  std::unique_ptr<std::unique_ptr<bool[]>[]> matrix;
	bool** mutexMatrix;
	std::vector<LMOrdering> reasonableOrderingsGoalsList;

	void addGoalNode(SASCondition* c, std::shared_ptr<TState> state);
	void exploreRPG();
	void actionProcessing(std::vector<std::shared_ptr<SASAction>>* a, std::shared_ptr<LTNode> g, int level);
	void checkPreconditions(std::shared_ptr<SASAction> a, std::unique_ptr<int[]> &common);
	bool verify(std::shared_ptr<LMFluent> p);
	bool verify(std::vector<std::shared_ptr<LMFluent>>* v);
	bool verify(std::vector<std::shared_ptr<SASAction>>* a);
	void groupUSet(std::vector<std::shared_ptr<USet>>* res, std::vector<std::shared_ptr<LMFluent>>* u, std::vector<std::shared_ptr<SASAction>>* a);
	void analyzeSet(std::shared_ptr<USet> s, std::vector<std::shared_ptr<SASAction>>* a, std::vector<std::shared_ptr<USet>>* u1);
	int equalParameters(std::shared_ptr<LMFluent> l, std::vector<std::shared_ptr<LMFluent>>* actionFluents);
	std::shared_ptr<USet> findDisjObject(std::shared_ptr<USet> u, int level);
	void postProcessing();
	void getActions(std::vector<std::shared_ptr<SASAction>>* aList, std::shared_ptr<LMFluent> l1, std::shared_ptr<LMFluent> l2);

public:
	std::vector<std::shared_ptr<LTNode>> nodes;
	std::vector<LMOrdering> edges;

	LandmarkTree(std::shared_ptr<TState> state, std::shared_ptr<SASTask> task, std::vector<std::shared_ptr<SASAction>>* tilActions);
	~LandmarkTree();
};

class LandmarkNode {
private:
	int index;
	std::vector<TVariable> variables;
	std::vector<TValue> values;
	std::vector<LandmarkNode*> nextNodes;

public:
	LandmarkNode(int index, std::shared_ptr<LTNode> n) {
		this->index = index;
		if (n->single()) {
			variables.push_back(n->getFluent()->variable);
			values.push_back(n->getFluent()->value);
		}
		else {
			std::vector<std::shared_ptr<LMFluent>>* fs = &(n->getSet()->fluentSet);
			for (unsigned int i = 0; i < fs->size(); i++) {
				variables.push_back(fs->at(i)->variable);
				values.push_back(fs->at(i)->value);
			}
		}
	}
	void addAdjacent(LandmarkNode* nextNode) {
		std::vector<LandmarkNode*>::iterator it = find(nextNodes.begin(), nextNodes.end(), nextNode);
		if (it == nextNodes.end()) {
			nextNodes.push_back(nextNode);
		}
	}
	std::string toString(std::shared_ptr<SASTask> task) {
		std::string res = "Node " + std::to_string(index) + ": (" + task->variables[variables[0]].name
			+ "," + task->values[values[0]].name + ")";
		for (unsigned int i = 1; i < variables.size(); i++) {
			res += ",(" + task->variables[variables[i]].name + "," + task->values[values[i]].name + ")";
		}
		return res;
	}
	inline unsigned int numAdjacents() {
		return nextNodes.size();
	}
	inline LandmarkNode* getAdjacent(unsigned int i) {
		return nextNodes[i];
	}
	inline int getIndex() {
		return index;
	}
	void deleteAdjacent(unsigned int i) {
		nextNodes.erase(nextNodes.begin() + i);
	}
	unsigned int getNumFluents() {
		return variables.size();
	}
	TVariable getVariable(unsigned int i) {
		return variables[i];
	}
	TValue getValue(unsigned int i) {
		return values[i];
	}
};

class Landmarks {
private:
	std::vector<LandmarkNode> nodes;

	bool checkIndirectReachability(int orig, int dst);
	bool checkIndirectReachability(int orig, int current, int dst, std::vector<bool>* visited);

public:
	Landmarks(std::shared_ptr<TState> state, std::shared_ptr<SASTask> task, std::vector<std::shared_ptr<SASAction>>* tilActions);
	void filterTransitiveOrders(std::shared_ptr<SASTask> task);
	unsigned int numNodes() { return nodes.size(); }
	LandmarkNode* getNode(unsigned int index) { return &(nodes[index]); }
	std::string toString(std::shared_ptr<SASTask> task);
};

#endif
