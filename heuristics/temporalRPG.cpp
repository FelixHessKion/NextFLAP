#include "temporalRPG.h"
#include <algorithm>
using namespace std;

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Temporal relaxed planning graph.						*/
/********************************************************/

//#define DEBUG_TEMPORALRPG_ON

void TemporalRPG::initialize(bool untilGoals, std::shared_ptr<SASTask> task, std::vector<std::shared_ptr<SASAction>>* tilActions) {
	verifyFluent = false;
	this->untilGoals = untilGoals;
	this->task = task;
	this->tilActions = tilActions;
	if (untilGoals) {
		for (unsigned int i = 0; i < task->goals.size(); i++) {
      std::shared_ptr<SASAction> goal = task->goals[i];
			for (unsigned int j = 0; j < goal->startCond.size(); j++)
				addGoalToAchieve(goal->startCond[j]);
			for (unsigned int j = 0; j < goal->overCond.size(); j++)
				addGoalToAchieve(goal->overCond[j]);
			for (unsigned int j = 0; j < goal->endCond.size(); j++)
				addGoalToAchieve(goal->endCond[j]);
		}
	}
	numActions = task->actions.size();
	visitedAction = std::make_unique<char[]>(numActions);
	for (int i = 0; i < numActions; i++)
		visitedAction[i] = 0;
	actionLevels = nullptr;
}

TemporalRPG::~TemporalRPG() {
}

void TemporalRPG::addGoalToAchieve(SASCondition& c) {
	TVarValue v = SASTask::getVariableValueCode(c.var, c.value);
	if (std::find(goalsToAchieve.begin(), goalsToAchieve.end(), v) == goalsToAchieve.end())
		goalsToAchieve.push_back(v);
}

void TemporalRPG::clearPriorityQueue() {
	while (qPNormal.size() > 0) {
		std::shared_ptr<FluentLevel> fl = std::dynamic_pointer_cast<FluentLevel>(qPNormal.poll());
	}
}

void TemporalRPG::build(std::shared_ptr<TState> state) {
	init(state);
	if (untilGoals && checkAcheivedGoals()) {
		clearPriorityQueue();
	}
	float auxLevel;
	while (qPNormal.size() > 0) {
		std::shared_ptr<FluentLevel> fl = std::dynamic_pointer_cast<FluentLevel>(qPNormal.poll());
		std::vector<std::shared_ptr<SASAction>>& req = task->requirers[fl->variable][fl->value];
#ifdef DEBUG_TEMPORALRPG_ON
		cout << "EXTR.: " << fl->toString(task) << ", " << req.size() << " requirers" << endl;
#endif
		for (unsigned int i = 0; i < req.size(); i++) {
			std::shared_ptr<SASAction> a = req[i];
			if (visitedAction[a->index] == 0) {
				if (verifyFluent && actionProducesFluent(a)) visitedAction[a->index] = 1;
				else {
					bool applicable = true;
					for (unsigned int j = 0; j < a->startCond.size(); j++) {
						auxLevel = getFirstGenerationTime(a->startCond[j].var, a->startCond[j].value);
						if (auxLevel < 0 || auxLevel > fl->level) {
							applicable = false;
							break; // Non applicable
						}
					}
					if (applicable) {
						for (unsigned int j = 0; j < a->overCond.size(); j++) {
							auxLevel = getFirstGenerationTime(a->overCond[j].var, a->overCond[j].value);
							if (auxLevel < 0 || auxLevel > fl->level) {
								applicable = false;
								break; // Non applicable
							}
						}
						if (applicable) {
#ifdef DEBUG_TEMPORALRPG_ON
							cout << "N.ACTION " << fl->level << ": " << a->name << endl;
#endif
							visitedAction[a->index] = 1;
							float effLevel = fl->level + EPSILON;
							for (unsigned j = 0; j < a->startEff.size(); j++) {
								TVariable v = a->startEff[j].var;
								TValue value = a->startEff[j].value;
								auxLevel = getFirstGenerationTime(v, value);
								if (auxLevel == -1 || auxLevel > effLevel) {
									firstGenerationTime[SASTask::getVariableValueCode(v, value)] = effLevel;
									qPNormal.add(std::make_shared<FluentLevel>(v, value, effLevel));
#ifdef DEBUG_TEMPORALRPG_ON
									cout << "* PROG: (" << task->variables[v].name << "," << task->values[value].name << ") -> " << effLevel << endl;
#endif
								}
							}
							effLevel += task->getActionDuration(a, state->minState.get());
							for (unsigned j = 0; j < a->endEff.size(); j++) {
								TVariable v = a->endEff[j].var;
								TValue value = a->endEff[j].value;
								auxLevel = getFirstGenerationTime(v, value);
								if (auxLevel == -1 || auxLevel > effLevel) {
									firstGenerationTime[SASTask::getVariableValueCode(v, value)] = effLevel;
									qPNormal.add(std::make_shared<FluentLevel>(v, value, effLevel));
#ifdef DEBUG_TEMPORALRPG_ON
									cout << "* PROG: (" << task->variables[v].name << "," << task->values[value].name << ") -> " << effLevel << endl;
#endif
								}
							}
						}
					}
				}
			}
			if (a->conditionalEff.size() > 0 && visitedAction[a->index] != 0) {
				for (SASConditionalEffect& e : a->conditionalEff) {
					bool applicable = true;
					for (SASCondition& c: e.startCond) {
						auxLevel = getFirstGenerationTime(c.var, c.value);
						if (auxLevel < 0 || auxLevel > fl->level) {
							applicable = false;
							break; // Non applicable
						}
					}
					if (applicable) {
						float effLevel = fl->level + EPSILON;
						for (SASCondition &c : e.startEff) {
							auxLevel = getFirstGenerationTime(c.var, c.value);
							if (auxLevel == -1 || auxLevel > effLevel) {
								firstGenerationTime[SASTask::getVariableValueCode(c.var, c.value)] = effLevel;
								qPNormal.add(std::make_shared<FluentLevel>(c.var, c.value, effLevel));
							}
						}
						effLevel += task->getActionDuration(a, state->minState.get());
						for (SASCondition &c : e.endEff) {
							auxLevel = getFirstGenerationTime(c.var, c.value);
							if (auxLevel == -1 || auxLevel > effLevel) {
								firstGenerationTime[SASTask::getVariableValueCode(c.var, c.value)] = effLevel;
								qPNormal.add(std::make_shared<FluentLevel>(c.var, c.value, effLevel));
							}
						}
					}
				}
			}
		}
		if (untilGoals && checkAcheivedGoals()) {
			clearPriorityQueue();
		}
	}
}

void TemporalRPG::init(std::shared_ptr<TState> state) {
	for (unsigned int i = 0; i < state->numSASVars; i++) {
		firstGenerationTime[SASTask::getVariableValueCode(i, state->state[i])] = 0;
	}
	if (verifyFluent) {
		firstGenerationTime[SASTask::getVariableValueCode(fluentToVerify.variable, fluentToVerify.value)] = -1;
	}
	for (int i = 0; i < numActions; i++) {
		std::shared_ptr<SASAction> a = task->actions[i];
		if (visitedAction[a->index] == 0)
			programAction(a, state);
	}
	if (tilActions != nullptr) {
		for (unsigned int i = 0; i < tilActions->size(); i++) {
			programAction(tilActions->at(i), state);
		}
	}
}

void TemporalRPG::programAction(std::shared_ptr<SASAction> a, std::shared_ptr<TState> state) {
	TVariable v;
	TValue value;
	float level, duration;
	bool insert = true;
	for (unsigned int j = 0; j < a->startCond.size(); j++) {
		if (state->state[a->startCond[j].var] != a->startCond[j].value) {
			insert = false;
			break;
		}
	}
	if (insert) {
		for (unsigned int j = 0; j < a->overCond.size(); j++) {
			if (state->state[a->overCond[j].var] != a->overCond[j].value) {
				insert = false;
				break;
			}
		}
	}
	if (insert) {
#ifdef DEBUG_TEMPORALRPG_ON
		cout << "Action: " << a->name << endl;
#endif
		if (a->index != MAX_UNSIGNED_INT) visitedAction[a->index] = 1;
		for (unsigned int j = 0; j < a->startEff.size(); j++) {
			v = a->startEff[j].var;
			value = a->startEff[j].value;
			level = getFirstGenerationTime(v, value);
			if (level == -1) {
				firstGenerationTime[SASTask::getVariableValueCode(v, value)] = EPSILON;
				qPNormal.add(std::make_shared<FluentLevel>(v, value, EPSILON));
#ifdef DEBUG_TEMPORALRPG_ON
				cout << "* PROG: (" << task->variables[v].name << "," << task->values[value].name << ") -> " << EPSILON << endl;
#endif
			}
		}

		duration = -1;

		for (SASConditionalEffect& e : a->conditionalEff) {
			bool hold = true;
			for (SASCondition &c : a->startCond) {
				if (state->state[c.var] != c.value) {
					hold = false;
					break;
				}
			}
			if (hold) {
				for (SASCondition &c : e.startEff) {
					level = getFirstGenerationTime(c.var, c.value);
					if (level == -1) {
						firstGenerationTime[SASTask::getVariableValueCode(c.var, c.value)] = EPSILON;
						qPNormal.add(std::make_shared<FluentLevel>(c.var, c.value, EPSILON));
					}
				}
				for (SASCondition &c : e.endEff) {
					level = getFirstGenerationTime(c.var, c.value);
					if (level == -1) {
						if (duration < 0) duration = EPSILON + task->getActionDuration(a, state->minState.get());
						firstGenerationTime[SASTask::getVariableValueCode(c.var, c.value)] = duration;
						qPNormal.add(std::make_shared<FluentLevel>(c.var, c.value, duration));
					}
				}
			}
		}

		for (unsigned int j = 0; j < a->endEff.size(); j++) {
			v = a->endEff[j].var;
			value = a->endEff[j].value;
			level = getFirstGenerationTime(v, value);
			if (level == -1) {
				if (duration < 0) duration = EPSILON + task->getActionDuration(a, state->minState.get());
				firstGenerationTime[SASTask::getVariableValueCode(v, value)] = duration;
				qPNormal.add(std::make_shared<FluentLevel>(v, value, duration));
#ifdef DEBUG_TEMPORALRPG_ON
				cout << "* PROG: (" << task->variables[v].name << "," << task->values[value].name << ") -> " << duration << endl;
#endif
			}
		}
	}
}

bool TemporalRPG::checkAcheivedGoals() {
	while (goalsToAchieve.size() > 0 && getFirstGenerationTime(goalsToAchieve[0]) >= 0) {
		goalsToAchieve[0] = goalsToAchieve[goalsToAchieve.size() - 1];
		goalsToAchieve.pop_back();
	}
	return goalsToAchieve.empty();
}

bool TemporalRPG::actionProducesFluent(std::shared_ptr<SASAction> a) {
	for (unsigned int i = 0; i < a->startEff.size(); i++) {
		if (a->startEff[i].var == fluentToVerify.variable && a->startEff[i].value == fluentToVerify.value)
			return true;
	}
	for (unsigned int i = 0; i < a->endEff.size(); i++) {
		if (a->endEff[i].var == fluentToVerify.variable && a->endEff[i].value == fluentToVerify.value)
			return true;
	}
	for (SASConditionalEffect& e : a->conditionalEff) {
		for (SASCondition& c : e.startEff) {
			if (c.var == fluentToVerify.variable && c.value == fluentToVerify.value)
				return true;
		}
		for (SASCondition& c : e.endEff) {
			if (c.var == fluentToVerify.variable && c.value == fluentToVerify.value)
				return true;
		}
	}
	return false;
}

void TemporalRPG::computeLiteralLevels() {
	clearPriorityQueue();
	fluentList.reserve(firstGenerationTime.size());
	int index = 0;
	for (auto it = firstGenerationTime.begin(); it != firstGenerationTime.end(); ++it, ++index) {
    std::shared_ptr<LMFluent> f = std::make_shared<LMFluent>();
		f->initialize(it->first, it->second, index);
		fluentList.push_back(f);
	}
	for (unsigned int i = 0; i < fluentList.size(); i++) {
		TVariable v = fluentList[i]->variable;
		TValue value = fluentList[i]->value;
		fluentIndex[SASTask::getVariableValueCode(v, value)] = fluentList[i]->index;
		qPNormal.add(std::make_shared<FluentLevel>(v, value, fluentList[i]->level));
	}
	float currentLevel = -1;
	int i = -1;
	while (qPNormal.size() > 0) {
		std::shared_ptr<FluentLevel> fl = std::dynamic_pointer_cast<FluentLevel>(qPNormal.poll());
		if (fl->level > currentLevel) {
#ifdef DEBUG_TEMPORALRPG_ON
			cout << "Level: " << fl->level << endl;
#endif
			fluentLevels.emplace_back();
			currentLevel = fl->level;
			fluentLevelIndex[currentLevel] = ++i;
		}
		fluentLevels[i].push_back(SASTask::getVariableValueCode(fl->variable, fl->value));
	}
}

void TemporalRPG::computeActionLevels(std::shared_ptr<TState> state) {
	actionLevels = std::make_unique<float[]>(numActions);
	for (int i = 0; i < numActions; i++) {
		actionLevels[i] = getActionLevel(task->actions[i], state);
#ifdef DEBUG_TEMPORALRPG_ON
		cout << "Action: " << task->actions[i].name << ", level " << actionLevels[i] << endl;
#endif
	}
	for (unsigned int i = 0; i < fluentList.size(); i++) {
    std::shared_ptr<LMFluent> f = fluentList[i];
		std::vector<std::shared_ptr<SASAction>>& p = task->producers[f->variable][f->value];
		for (unsigned int j = 0; j < p.size(); j++) {
			std::shared_ptr<SASAction> a = p[j];
			if (actionLevels[a->index] < f->level && actionLevels[a->index] >= 0) {
				f->producers.push_back(a);
			}
		}
	}
}

float TemporalRPG::getActionLevel(std::shared_ptr<SASAction> a, std::shared_ptr<TState> state) {
	float res = 0, level;
	for (unsigned int i = 0; i < a->startCond.size(); i++) {
		level = getFirstGenerationTime(a->startCond[i].var, a->startCond[i].value);
		if (level > res) res = level;
		else if (level == -1) return -1;
	}
	for (unsigned int i = 0; i < a->overCond.size(); i++) {
		level = getFirstGenerationTime(a->overCond[i].var, a->overCond[i].value);
		if (level > res) res = level;
		else if (level == -1) return -1;
	}
	float duration = task->getActionDuration(a, state->minState.get());
	for (unsigned int i = 0; i < a->endCond.size(); i++) {
		level = getFirstGenerationTime(a->endCond[i].var, a->endCond[i].value);
		if (level == -1) return -1;
		level -= duration;
		if (level > res) res = level;
	}
	return res;
}
