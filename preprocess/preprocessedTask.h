#ifndef PREPROCESSED_TASK_H
#define PREPROCESSED_TASK_H

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Simplified task obtained from the parsedTask after   */
/* the preprocess.                                      */
/********************************************************/

#include "../parser/parsedTask.h"
#include "../utils/utils.h"

#include <memory>

class OpEquality {
public:
    bool equal;
    Term value1;
    Term value2;
    std::string toString(const std::vector<Object>& objects);
};

class OpFluent {
public:
    Literal variable;
    Term value;
    std::string toString(const std::vector<Function>& functions, const std::vector<Object>& objects);
    inline std::string getVarName(std::unique_ptr<unsigned int[]> &paramValues) {
        unsigned int index;
        std::string name(std::to_string(variable.fncIndex));
        for (unsigned int i = 0; i < variable.params.size(); i++) {
            index = variable.params[i].index;
            if (variable.params[i].type == TERM_PARAMETER) {
                if (paramValues[index] == MAX_UNSIGNED_INT) name += " ?" + std::to_string(index);
                else name += " " + std::to_string(paramValues[index]);
            }
            else name += " " + std::to_string(index);
        }
        return name;
    }
    inline std::string getValueName(std::unique_ptr<unsigned int[]> &paramValues) {
        if (value.type == TERM_PARAMETER) {
            if (paramValues[value.index] == MAX_UNSIGNED_INT) return "?" + std::to_string(value.index);
            else return std::to_string(paramValues[value.index]);
        }
        else return std::to_string(value.index);
    }
};

class OpPreference {
public:
    std::string name;
    GoalDescription preference;
    std::string toString(const std::vector<Variable>& opParameters, const std::vector<Variable>& controlVars, std::unique_ptr<ParsedTask> &task);
};

enum OpEffectExpressionType {
    OEET_NUMBER = 0, OEET_DURATION = 1, OEET_SHARP_T = 2,
    OEET_SHARP_T_PRODUCT = 3, OEET_FLUENT = 4, OEET_TERM = 5,
    OEET_SUM = 6, OEET_SUB = 7, OEET_DIV = 8, OEET_MUL = 9
};

class OpEffectExpression {
private:
    static OpEffectExpressionType getOperator(OperationType op) {
        switch (op) {
        case OT_SUM: return OEET_SUM;
        case OT_SUB: return OEET_SUB;
        case OT_DIV: return OEET_DIV;
        default:     return OEET_MUL;
        }
    }
    static OpEffectExpressionType getNumOperator(NumericExpressionType op) {
        switch (op) {
        case NET_SUM: return OEET_SUM;
        case NET_SUB: return OEET_SUB;
        case NET_DIV: return OEET_DIV;
        default:      return OEET_MUL;
        }
    }
public:
    OpEffectExpressionType type;
    float value;                                // if type == OEET_NUMBER
    std::vector<OpEffectExpression> operands;   // if type == OEET_SHARP_T_PRODUCT | OEET_SUM | OEET_SUB | OEET_DIV | OEET_MUL
    Literal fluent;                             // if type == OEET_FLUENT
    Term term;                                  // if type == OEET_TERM
    OpEffectExpression(EffectExpression& exp);
    OpEffectExpression(ContinuousEffect& exp);
    OpEffectExpression(NumericExpression& exp);
    std::string toString(const std::vector<Function>& functions, const std::vector<Object>& objects,
        const std::vector<Variable>& controlVars);
};

class OpEffect {
public:
    Assignment assignment;
    Literal fluent;
    OpEffectExpression exp;
    OpEffect(FluentAssignment& f);
    OpEffect(AssignmentContinuousEffect& f);
    std::string toString(const std::vector<Function>& functions, const std::vector<Object>& objects, 
        const std::vector<Variable>& controlVars);
};

class OpNumericPrec {
public:
    Comparator comparator;
    std::vector<OpEffectExpression> operands;
    OpNumericPrec();
    OpNumericPrec(GoalDescription& goal);
    std::string toString(const std::vector<Function>& functions, const std::vector<Object>& objects,
        const std::vector<Variable>& controlVars);
};

class OpCondition {
public:
    std::vector<OpFluent> prec;
    std::vector<OpNumericPrec> numericPrec;
    std::vector<OpFluent> eff;
    std::vector<OpEffect> numericEff;
};

class OpConditionalEffect {
public:
    OpCondition atStart;
    OpCondition atEnd;
};

class Operator {
public:
    std::string name;
    std::vector<Variable> parameters;
    std::vector<Variable> controlVars;
    std::vector<Duration> duration;
    OpCondition atStart;
    OpCondition atEnd;
    std::vector<OpConditionalEffect> condEffects;
    std::vector<OpFluent> overAllPrec;
    std::vector<OpNumericPrec> overAllNumericPrec;
    std::vector<OpEquality> equality;
    std::vector<OpPreference> preference;
    bool isGoal;
    bool isTIL;
    bool instantaneous;
    
    Operator(bool instantaneous, bool isTIL, bool isGoal) {
        this->instantaneous = instantaneous;
        this->isTIL = isTIL;
        this->isGoal = isGoal;
    }
    void addLiteralToPreconditions(Literal var, Term& value, TimeSpecifier time);
    void addEquality(Term& v1, Term& v2, bool equal);
    void addNumericPrecondition(GoalDescription goal, TimeSpecifier time);
    void addLiteralToEffects(Literal var, Term& value, TimeSpecifier time);
    void addNumericEffect(OpEffect eff, TimeSpecifier time);
    std::string toString(std::unique_ptr<ParsedTask> &task);
};

class PreprocessedTask {
private:
        
public:
    std::unique_ptr<ParsedTask> &task;
    std::vector<Operator> operators;
    PreprocessedTask(std::unique_ptr<ParsedTask> &parsedTask);
    ~PreprocessedTask();
    std::string toString();
};

#endif
