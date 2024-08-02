#ifndef SAS_TRANSLATOR_H
#define SAS_TRANSLATOR_H

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022											*/
/********************************************************/
/* Translates the PDDL literals (boolean fluents) into  */
/* a set of object fluents.                             */
/********************************************************/

#include "../grounder/groundedTask.h"
#include "sasTask.h"
#include "mutexGraph.h"

#include <memory>

class LiteralTranslation {
public:
	std::unique_ptr<unsigned int[]> numericVariables;						// New indexes for numeric variables
	std::unique_ptr<unsigned int[]> sasVariables;							// Already defined SAS variables
	std::vector< std::vector<unsigned int> > literals;	// literal -> <sasVariable, sasValue>
	LiteralTranslation(unsigned int numVars);
	~LiteralTranslation();
};

class SASTranslator {
private:
    std::unique_ptr<GroundedTask> gTask;
    bool** mutex;
    std::unique_ptr<bool[]> actions;
    bool* isLiteral;
    bool* literalInFNA;
	bool* literalInF;
	bool* negatedLiteral;
    bool negatedPrecs;
	unsigned int numNewLiterals;
    unsigned int numVars;
    unsigned int numActions;
    std::unordered_map<unsigned long long, bool> mutexChanges;

	void getInitialStateLiterals();
    void clearMemory();
    void checkAction(GroundedAction *a);
    bool holdsCondition(const GroundedCondition *c, std::vector<unsigned int>* preconditions);
    void computeMutex(GroundedAction* a, const std::vector<unsigned int> preconditions, unsigned int startEndPrec/*,
		std::vector<bool>& holdCondEff*/);
    void splitMutex(std::shared_ptr<SASTask> sTask, bool onlyGenerateMutex);
	inline static int findInVector(unsigned int value, const std::vector<unsigned int>* v) {
        for (unsigned int i = 0; i < v->size(); i++)
            if ((*v)[i] == value) return (int) i;
        return -1;
    }
	inline static int literalInAtStartAdd(unsigned int value, std::vector<unsigned int>* add, unsigned int statAddEndEff) {
		for (unsigned int i = 0; i < statAddEndEff; i++)
			if ((*add)[i] == value) return (int)i;
		return -1;
	}
    inline unsigned long long mutexIndex(unsigned int v1, unsigned int v2) {
        if (v1 > v2) {
           unsigned int aux = v1;
           v1 = v2;
           v2 = aux;
        }
        return (((unsigned long long) v1) << 32) + v2;
    }
    inline void addMutex(unsigned int v1, unsigned int v2) {
        if (!mutex[v1][v2]) {
            mutex[v1][v2] = true;
            mutex[v2][v1] = true;
            unsigned long long code = mutexIndex(v1,v2);
            std::unordered_map<unsigned long long,bool>::const_iterator got = mutexChanges.find(code);
			if (got == mutexChanges.end() || !got->second) {
           		mutexChanges[code] = true;
		    }
        }
    }
    inline void deleteMutex(unsigned int v1, unsigned int v2) {
        if (mutex[v1][v2]) {
           mutex[v1][v2] = false;
           mutex[v2][v1] = false;
           unsigned long long code = mutexIndex(v1,v2);
		   std::unordered_map<unsigned long long,bool>::const_iterator got = mutexChanges.find(code);
		   if (got == mutexChanges.end() || got->second) {
           		mutexChanges[code] = false;
		   }
        }
    }
    void updateDomain(std::shared_ptr<SASTask> sTask, MutexGraph* graph, LiteralTranslation* trans);
    void simplifyDomain(std::shared_ptr<SASTask> sTask, LiteralTranslation* trans);
	void removeMultipleValues(std::shared_ptr<SASTask> sTask, LiteralTranslation* trans);
	void createNumericAndFiniteDomainVariables(std::shared_ptr<SASTask> sTask, LiteralTranslation* trans);
	void setInitialValuesForVariables(std::shared_ptr<SASTask> sTask, LiteralTranslation* trans);
	void createAction(GroundedAction* ga, std::shared_ptr<SASTask> sTask, LiteralTranslation* trans, bool isGoal);
	void generateControlVar(SASAction* a, GroundedControlVar* cv);
	void generateDuration(SASAction* a, GroundedDuration* gd, LiteralTranslation* trans);
	char generateComparator(int comp);
	char generateTime(int time);
	SASNumericExpression generateNumericExpression(GroundedNumericExpression* gn, LiteralTranslation* trans);
	SASNumericExpression generateNumericExpression(PartiallyGroundedNumericExpression* gn, LiteralTranslation* trans);
	char generateNumericExpressionType(int type);
	char generatePartiallyNumericExpressionType(int type);
	void generateCondition(GroundedCondition* cond, std::shared_ptr<SASTask> sTask, LiteralTranslation* trans, std::vector<SASCondition>* conditionSet);
	void generateEffect(std::vector<GroundedCondition>* effects, unsigned int effIndex, std::shared_ptr<SASTask> sTask, LiteralTranslation* trans, std::vector<SASCondition>* conditionSet);
	bool modifiedVariable(unsigned int sasVar, std::vector<GroundedCondition>* effects, unsigned int effIndex, LiteralTranslation* trans);
	void checkModifiedVariable(SASCondition* c, SASAction* a);
	void checkNegatedPreconditionLiterals(GroundedAction* a);
	SASNumericCondition generateNumericCondition(GroundedNumericCondition* cond, LiteralTranslation* trans, SASAction* a);
	SASNumericEffect generateNumericEffect(GroundedNumericEffect* cond, LiteralTranslation* trans);
	char generateAssignment(int assignment);
	SASPreference generatePreference(GroundedPreference* pref, std::shared_ptr<SASTask> sTask, LiteralTranslation* trans);
	SASGoalDescription generateGoalDescription(GroundedGoalDescription* gd, std::shared_ptr<SASTask> sTask, LiteralTranslation* trans);
	SASConstraint createConstraint(GroundedConstraint* gc, std::shared_ptr<SASTask> sTask, LiteralTranslation* trans);
	SASMetric createMetric(GroundedMetric* metric, LiteralTranslation* trans);
	void writeMutexFile();
	void removeActionsWithMutexConditions();
	bool hasMutexConditions(GroundedAction* a);
	bool isMutex(GroundedCondition &c1, GroundedCondition &c2);
	void translateMutex(std::shared_ptr<SASTask> sTask, LiteralTranslation* trans);
	bool checkConditionalEffectHolds(GroundedConditionalEffect& e);
	void classifyEffect(GroundedCondition& e, std::vector<unsigned int>& newA, std::vector<unsigned int>& add, 
		std::vector<unsigned int>& del);

public:
    void translate(std::unique_ptr<GroundedTask> &gTaskIn, bool onlyGenerateMutex, bool generateMutexFile, bool keepStaticData, std::shared_ptr<SASTask> sTaskOut);
};

#endif
