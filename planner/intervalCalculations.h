#ifndef INTERVAL_CALC_H
#define INTERVAL_CALC_H

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Class for calculations with numeric intervals.       */
/********************************************************/

#include "../sas/sasTask.h"
#include "plan.h"

// Base class for numeric interval
class FluentIntervalData {
public:
    virtual TFloatValue getMinValue(TVariable v, int numState) = 0;
    virtual TFloatValue getMaxValue(TVariable v, int numState) = 0;
};

// Value change of a numeric variable
class TNumVarChange {
public:
    TVariable v;
    TFloatValue min, max;

    TNumVarChange(TVariable v, TFloatValue min, TFloatValue max) { this->v = v; this->min = min; this->max = max; }
};

// Class for calculations with numeric intervals
class IntervalCalculations {
private:
    std::shared_ptr<SASAction> a;
    std::vector<TInterval> fluentValues;
    TInterval duration;
    std::vector<TInterval> cvarValues;

    void evaluateExpression(SASNumericExpression* e, TFloatValue* min, TFloatValue* max);
    void constrainInterval(char comp, SASNumericExpression* e, TInterval* cvarInt);
    void calculateControlVarIntervals();
    void calculateDuration();
    void applyEffect(SASNumericEffect* e);
    void constrainAtStartFluent(TVariable v);
    void constrainAtEndFluent(TVariable v);

public:
    IntervalCalculations(std::shared_ptr<SASAction> a, int numState, FluentIntervalData* fluentData, std::shared_ptr<SASTask> task);
    bool supportedNumericStartConditions(std::shared_ptr<bool[]> holdCondEff);
    bool supportedNumericEndConditions(std::shared_ptr<bool[]> holdCondEff);
    bool supportedNumericConditions(SASConditionalEffect* e);
    void constrainAtStartFluents();
    void applyStartEffects(std::shared_ptr<Plan> p, std::shared_ptr<bool[]> holdCondEff);
    void applyStartEffects(std::vector<TNumVarChange>* v, std::shared_ptr<bool[]> holdCondEff);
    void applyEndEffects(std::shared_ptr<Plan> p, std::shared_ptr<bool[]> holdCondEff);
    void applyEndEffects(std::vector<TNumVarChange>* v, std::shared_ptr<bool[]> holdCondEff);
    void copyControlVars(std::shared_ptr<Plan> p);
    void copyDuration(std::shared_ptr<Plan> p);
    bool supportedCondition(SASNumericCondition* c);
};

#endif
