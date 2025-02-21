/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Removes complex features from the ParsedTask, such   */
/* as universal and existential quantifiers,            */
/* disjunctions, etc.                                   */
/********************************************************/

#include "preprocess.h"
#include "../utils/utils.h"
using namespace std;

// Creates a new preprocessor
Preprocess::Preprocess(std::unique_ptr<ParsedTask> &parsedTask): task(parsedTask) {
}

// Disposes the preprocessor
Preprocess::~Preprocess() {
}

// Starts the preprocess and returns the preprocessed task
void Preprocess::preprocessTask(std::unique_ptr<PreprocessedTask> &prepTaskOut)
{
    prepTask = std::make_unique<PreprocessedTask>(task);
    preprocessOperators();
    //cout << prepTask->toString() << endl;
    //throwError(prepTask->toString());
    prepTaskOut = std::move(prepTask);
    return;
}

// Operators preprocessing
void Preprocess::preprocessOperators() {
    for (unsigned int i = 0; i < task->actions.size(); i++) {
        //cout << task->actions[i].toString(task->functions, task->objects, task->types) << endl;
        FeatureList features = { }; // Initialize to zero
        checkPreconditionFeatures(task->actions[i].precondition, &features);
        checkEffectFeatures(task->actions[i].effect, &features);
        preprocessAction(task->actions[i], &features, false, false);
        //cout << task->actions[i].toString(task->functions, task->objects, task->types) << endl;
    }
    for (unsigned int i = 0; i < task->durativeActions.size(); i++) {
        FeatureList features = { }; // Initialize to zero
        checkPreconditionFeatures(task->durativeActions[i].condition, &features);
        checkEffectFeatures(task->durativeActions[i].effect, &features);
        preprocessAction(task->durativeActions[i], &features, false, false);
    }
    Action goalAction;
    goalAction.index = -1;
    goalAction.name = "#goal";
    goalAction.precondition = task->goal;
    goalAction.effect.type = ET_AND;
    FeatureList features = { }; // Initialize to zero
    checkPreconditionFeatures(goalAction.precondition, &features);
    preprocessAction(goalAction, &features, false, true);
}

// Checks the extended features in an action precondition
void Preprocess::checkPreconditionFeatures(Precondition &prec, FeatureList* features) {
    switch (prec.type) {
    case PT_AND:
        for (unsigned int i = 0; i < prec.terms.size(); i++)
            checkPreconditionFeatures(prec.terms[i], features);
        break;
    case PT_NOT:
        checkPreconditionFeatures(prec.terms[0], features);
        break;
    case PT_OR:
        features->disjunctionPrec++;
        for (unsigned int i = 0; i < prec.terms.size(); i++)
            checkPreconditionFeatures(prec.terms[i], features);
        break;
    case PT_IMPLY:
        features->implicationPrec++;
        checkPreconditionFeatures(prec.terms[0], features);
        checkPreconditionFeatures(prec.terms[1], features);
        break;
    case PT_EXISTS:
        features->existentialQuantifierPrec++;
        checkPreconditionFeatures(prec.terms[0], features);
        break;
    case PT_FORALL:
        features->universalQuantifierPrec++;
        checkPreconditionFeatures(prec.terms[0], features);
        break;
    default:;
    }
}

// Checks the extended features in a durative-action precondition
void Preprocess::checkPreconditionFeatures(DurativeCondition &prec, FeatureList* features) {
    switch (prec.type) {
    case CT_AND:
        for (unsigned int i = 0; i < prec.conditions.size(); i++)
            checkPreconditionFeatures(prec.conditions[i], features);
        break;
    case CT_GOAL:
        checkGoalFeatures(prec.goal, features, true);
        break;
    case CT_FORALL:
        features->universalQuantifierPrec++;
        checkPreconditionFeatures(prec.conditions[0], features);
        break;
    case CT_PREFERENCE:;
    }
}

// Checks the extended features in a goal description
void Preprocess::checkGoalFeatures(GoalDescription &goal, FeatureList* features, bool prec) {
    switch (goal.type) {
    case GD_AND:
        for (unsigned int i = 0; i < goal.terms.size(); i++)
            checkGoalFeatures(goal.terms[i], features, prec);
        break;
    case GD_NOT:
        checkGoalFeatures(goal.terms[0], features, prec);
        break;
    case GD_OR:
        if (prec) features->disjunctionPrec++;
        else features->disjunctionEff++;
        for (unsigned int i = 0; i < goal.terms.size(); i++)
            checkGoalFeatures(goal.terms[i], features, prec);
        break;
    case GD_IMPLY:
        if (prec) features->implicationPrec++;
        else features->implicationEff++;
        checkGoalFeatures(goal.terms[0], features, prec);
        checkGoalFeatures(goal.terms[1], features, prec);
        break;
    case GD_EXISTS:
        if (prec) features->existentialQuantifierPrec++;
        else features->existentialQuantifierEff++;
        checkGoalFeatures(goal.terms[0], features, prec);
        break;
    case GD_FORALL:
        if (prec) features->universalQuantifierPrec++;
        else features->universalQuantifierEff++;
        checkGoalFeatures(goal.terms[0], features, prec);
        break;
    default:;
    }
}
    
// Checks the extended features in an action effect
void Preprocess::checkEffectFeatures(Effect &eff, FeatureList* features) {
    switch (eff.type) {
    case ET_AND:
        for (unsigned int i = 0; i < eff.terms.size(); i++)
            checkEffectFeatures(eff.terms[i], features);
        break;
    case ET_NOT:
        checkEffectFeatures(eff.terms[0], features);
        break;
    case ET_FORALL:
        features->universalQuantifierEff++;
        checkEffectFeatures(eff.terms[0], features);
        break;
    case ET_WHEN:
        features->conditionalEff++;
        checkGoalFeatures(eff.goal, features, false);
        checkEffectFeatures(eff.terms[0], features);
        break;
    default:;
    }
}

// Checks the extended features in a duractive-action effect
void Preprocess::checkEffectFeatures(DurativeEffect &eff, FeatureList* features) {
    switch (eff.type) {
    case DET_AND:
        for (unsigned int i = 0; i < eff.terms.size(); i++)
            checkEffectFeatures(eff.terms[i], features);
        break;
    case DET_FORALL:
        features->universalQuantifierEff++;
        checkEffectFeatures(eff.terms[0], features);
        break;
    case DET_WHEN:
        features->conditionalEff++;
        checkGoalFeatures(eff.condition, features, false);
        break;
    case DET_TIMED_EFFECT:
    case DET_ASSIGNMENT:;
    }
}

// Checks the extended features in a duractive condition
void Preprocess::checkGoalFeatures(DurativeCondition &goal, FeatureList* features, bool prec) {
    switch (goal.type) {
    case CT_AND:
        for (unsigned int i = 0; i < goal.conditions.size(); i++)
            checkGoalFeatures(goal.conditions[i], features, prec);
        break;
    case CT_GOAL:
        checkGoalFeatures(goal.goal, features, prec);
        break;
    case CT_FORALL:
        if (prec) features->universalQuantifierPrec++;
        else features->universalQuantifierEff++;
        checkGoalFeatures(goal.conditions[0], features, prec);
        break;
    case CT_PREFERENCE:; 
    }
}
    
// Processes an action and stores the result in a vector of simplified operators
void Preprocess::preprocessAction(Action a, FeatureList* features, bool isTIL, bool isGoal) {
    if (features->universalQuantifierPrec > 0 || features->existentialQuantifierPrec > 0)
        removeQuantifiers(a.precondition, a.parameters.size());
    if (features->universalQuantifierEff > 0 || features->existentialQuantifierEff > 0)
        removeQuantifiers(a.effect, a.parameters.size());
    if (features->implicationPrec > 0)
        removeImplications(a.precondition);
    if (features->implicationEff > 0)
        removeImplications(a.effect);
    //cout << a.toString(task->functions, task->objects, task->types) << endl;
    preconditionOptimization(&a.precondition, nullptr, 0, a);
    conjuctionOptimization(&a.effect);
    effectOptimization(&a.effect, nullptr, 0, a);
    buildOperators(a, isTIL, isGoal);
}

// Processes a durative action and stores the result in a vector of simplified operators
void Preprocess::preprocessAction(DurativeAction a, FeatureList* features, bool isTIL, bool isGoal) {
    if (features->universalQuantifierPrec > 0 || features->existentialQuantifierPrec > 0)
        removeQuantifiers(a.condition, a.parameters.size());
    if (features->universalQuantifierEff > 0 || features->existentialQuantifierEff > 0)
        removeQuantifiers(a.effect, a.parameters.size());
    if (features->implicationPrec > 0)
        removeImplications(a.condition);
    if (features->implicationEff > 0)
        removeImplications(a.effect);
    preconditionOptimization(&a.condition, nullptr, 0, a);
    conjuctionOptimization(&a.effect);
    effectOptimization(&a.effect, nullptr, 0, a);
    buildOperators(a, isTIL, isGoal);
    /*
    vector<DurativeAction> aList;
    if (features->conditionalEff > 0) {
        removeConditionalEffects(a, aList);
    } else {
        aList.push_back(a);
    }
    for (unsigned int i = 0; i < aList.size(); i++)
        buildOperators(aList[i], isGoal, i);
    */
}
    
// Removes all quantifiers from a precondition
void Preprocess::removeQuantifiers(Precondition &prec, unsigned int numParameters) {
    switch (prec.type) {
    case PT_AND:
    case PT_OR:
        for (unsigned int i = 0; i < prec.terms.size(); i++)
            removeQuantifiers(prec.terms[i], numParameters);
        break;
    case PT_NOT:
        removeQuantifiers(prec.terms[0], numParameters);
        break;
    case PT_IMPLY:
        removeQuantifiers(prec.terms[0], numParameters);
        removeQuantifiers(prec.terms[1], numParameters);
        break;
    case PT_EXISTS:
        prec.type = PT_OR;
        replaceQuantifierParameter(prec, prec.terms[0], 0, numParameters);
        prec.terms.erase(prec.terms.begin());
        for (unsigned int i = 0; i < prec.terms.size(); i++)
            removeQuantifiers(prec.terms[i], numParameters + prec.parameters.size());
        break;
    case PT_FORALL:
        prec.type = PT_AND;
        replaceQuantifierParameter(prec, prec.terms[0], 0, numParameters);
        prec.terms.erase(prec.terms.begin());
        for (unsigned int i = 0; i < prec.terms.size(); i++)
            removeQuantifiers(prec.terms[i], numParameters + prec.parameters.size());
        break;
    default:;
    }
}

// Removes all quantifiers from a precondition
void Preprocess::removeQuantifiers(DurativeCondition &prec, unsigned int numParameters) {
    switch (prec.type) {
    case CT_AND:
        for (unsigned int i = 0; i < prec.conditions.size(); i++)
            removeQuantifiers(prec.conditions[i], numParameters);
        break;
    case CT_GOAL:
        removeQuantifiers(prec.goal, numParameters);
        break;
    case CT_FORALL:
        prec.type = CT_AND;
        replaceQuantifierParameter(prec, prec.conditions[0], 0, numParameters);
        prec.conditions.erase(prec.conditions.begin());
        for (unsigned int i = 0; i < prec.conditions.size(); i++)
            removeQuantifiers(prec.conditions[i], numParameters + prec.parameters.size());
        break;
    case CT_PREFERENCE:;
    }
}
    
// Removes all quantifiers from an effect
void Preprocess::removeQuantifiers(Effect &eff, unsigned int numParameters) {
    //cout << eff.toString(borrar->parameters, borrar->parameters, task->functions, task->objects, task->types) << endl;
    switch (eff.type) {
    case ET_AND:
        for (unsigned int i = 0; i < eff.terms.size(); i++)
            removeQuantifiers(eff.terms[i], numParameters);
        break;
    case ET_NOT:
        removeQuantifiers(eff.terms[0], numParameters);
        break;
    case ET_WHEN:
        removeQuantifiers(eff.goal, numParameters);
        removeQuantifiers(eff.terms[0], numParameters);
        break;
    case ET_FORALL:
        eff.type = ET_AND;
        replaceQuantifierParameter(eff, eff.terms[0], 0, numParameters);
        eff.terms.erase(eff.terms.begin());
        //cout << eff.toString(borrar->parameters, borrar->parameters, task->functions, task->objects, task->types) << endl;
        for (unsigned int i = 0; i < eff.terms.size(); i++)
            removeQuantifiers(eff.terms[i], numParameters + eff.parameters.size());
        break;
    default:;
    }
}

// Removes all quantifiers from an effect
void Preprocess::removeQuantifiers(DurativeEffect &eff, unsigned int numParameters) {
    switch (eff.type) {
    case DET_AND:
        for (unsigned int i = 0; i < eff.terms.size(); i++)
            removeQuantifiers(eff.terms[i], numParameters);
        break;
    case DET_FORALL:
        eff.type = DET_AND;
        replaceQuantifierParameter(eff, eff.terms[0], 0, numParameters);
        eff.terms.erase(eff.terms.begin());
        for (unsigned int i = 0; i < eff.terms.size(); i++)
            removeQuantifiers(eff.terms[i], numParameters + eff.parameters.size());
        break;
    case DET_WHEN:
        removeQuantifiers(eff.condition, numParameters);
        break;
    case DET_TIMED_EFFECT:
    case DET_ASSIGNMENT:;
    }
}
    
// Removes all quantifiers from a goal description
void Preprocess::removeQuantifiers(GoalDescription &goal, unsigned int numParameters) {
    switch (goal.type) {
    case GD_AND:
    case GD_OR:
        for (unsigned int i = 0; i < goal.terms.size(); i++)
            removeQuantifiers(goal.terms[i], numParameters);
        break;
    case GD_NOT:
        removeQuantifiers(goal.terms[0], numParameters);
        break;
    case GD_IMPLY:
        removeQuantifiers(goal.terms[0], numParameters);
        removeQuantifiers(goal.terms[1], numParameters);
        break;
    case GD_EXISTS:
        goal.type = GD_OR;
        replaceQuantifierParameter(goal, goal.terms[0], 0, numParameters);
        goal.terms.erase(goal.terms.begin());
        for (unsigned int i = 0; i < goal.terms.size(); i++)
            removeQuantifiers(goal.terms[i], numParameters + goal.parameters.size());
        break;
    case GD_FORALL:
        goal.type = GD_AND;
        replaceQuantifierParameter(goal, goal.terms[0], 0, numParameters);
        goal.terms.erase(goal.terms.begin());
        for (unsigned int i = 0; i < goal.terms.size(); i++)
            removeQuantifiers(goal.terms[i], numParameters + goal.parameters.size());
        break;
    default:;
    }
}

// Replaces the parameters of a quantifier by the task object that matches
void Preprocess::replaceQuantifierParameter(Precondition &prec, Precondition term, 
        unsigned int paramNumber, unsigned int numParameters) {
    Variable &param = prec.parameters[paramNumber];
    for (unsigned int i = 0; i < task->objects.size(); i++) {
        if (task->compatibleTypes(task->objects[i].types, param.types)) {
            Precondition updatedTerm = term;
            replaceParameter(updatedTerm, paramNumber + numParameters, i);
            if (paramNumber + 1 < prec.parameters.size())
                replaceQuantifierParameter(prec, updatedTerm, paramNumber + 1, numParameters);
            else
                prec.terms.push_back(updatedTerm);
        }
    }
}

// Replaces the parameters of a quantifier by the task object that matches
void Preprocess::replaceQuantifierParameter(DurativeCondition &prec, DurativeCondition term, 
        unsigned int paramNumber, unsigned int numParameters) {
    Variable &param = prec.parameters[paramNumber];
    for (unsigned int i = 0; i < task->objects.size(); i++) {
        if (task->compatibleTypes(task->objects[i].types, param.types)) {
            DurativeCondition updatedTerm = term;
            replaceParameter(updatedTerm, paramNumber + numParameters, i);
            if (paramNumber + 1 < prec.parameters.size())
                replaceQuantifierParameter(prec, updatedTerm, paramNumber + 1, numParameters);
            else
                prec.conditions.push_back(updatedTerm);
        }
    }
}
        
// Replaces the parameters of a quantifier by the task object that matches
void Preprocess::replaceQuantifierParameter(Effect &eff, Effect term, 
        unsigned int paramNumber, unsigned int numParameters) {
    Variable &param = eff.parameters[paramNumber];
    for (unsigned int i = 0; i < task->objects.size(); i++) {
        if (task->compatibleTypes(task->objects[i].types, param.types)) {
            Effect updatedTerm = term;
            replaceParameter(updatedTerm, paramNumber + numParameters, i);
            if (paramNumber + 1 < eff.parameters.size())
                replaceQuantifierParameter(eff, updatedTerm, paramNumber + 1, numParameters);
            else
                eff.terms.push_back(updatedTerm);
        }
    }
}

// Replaces the parameters of a quantifier by the task object that matches
void Preprocess::replaceQuantifierParameter(DurativeEffect &eff, DurativeEffect term, 
        unsigned int paramNumber, unsigned int numParameters) {
    Variable &param = eff.parameters[paramNumber];
    for (unsigned int i = 0; i < task->objects.size(); i++) {
        if (task->compatibleTypes(task->objects[i].types, param.types)) {
            DurativeEffect updatedTerm = term;
            replaceParameter(updatedTerm, paramNumber + numParameters, i);
            if (paramNumber + 1 < eff.parameters.size())
                replaceQuantifierParameter(eff, updatedTerm, paramNumber + 1, numParameters);
            else
                eff.terms.push_back(updatedTerm);
        }
    }
}
    
// Replaces the parameters of a quantifier by the task object that matches
void Preprocess::replaceQuantifierParameter(GoalDescription &goal, GoalDescription term, 
        unsigned int paramNumber, unsigned int numParameters) {
    Variable &param = goal.parameters[paramNumber];
    for (unsigned int i = 0; i < task->objects.size(); i++) {
        if (task->compatibleTypes(task->objects[i].types, param.types)) {
            GoalDescription updatedTerm = term;
            replaceParameter(updatedTerm, paramNumber + numParameters, i);
            if (paramNumber + 1 < goal.parameters.size())
                replaceQuantifierParameter(goal, updatedTerm, paramNumber + 1, numParameters);
            else
                goal.terms.push_back(updatedTerm);
        }
    }       
}
 
// Replaces a parameter with a given object in an effect
void Preprocess::replaceParameter(Effect &eff, unsigned int paramToReplace, unsigned int objectIndex) {
    switch (eff.type) {
    case ET_LITERAL:
    case ET_NEG_LITERAL:
        replaceParameter(eff.literal, paramToReplace, objectIndex);
        break;
    case ET_AND:
        for (unsigned int i = 0; i < eff.terms.size(); i++)
            replaceParameter(eff.terms[i], paramToReplace, objectIndex);
        break;
    case ET_NOT:
    case ET_FORALL:
        replaceParameter(eff.terms[0], paramToReplace, objectIndex);
        break;
    case ET_WHEN:
        replaceParameter(eff.goal, paramToReplace, objectIndex);
        replaceParameter(eff.terms[0], paramToReplace, objectIndex);
        break;
    case ET_ASSIGNMENT:
        replaceParameter(eff.assignment, paramToReplace, objectIndex);
        break;
    }
}

// Replaces a parameter with a given object in an effect
void Preprocess::replaceParameter(DurativeEffect &eff, unsigned int paramToReplace, unsigned int objectIndex) {
    switch (eff.type) {
    case DET_AND:
        for (unsigned int i = 0; i < eff.terms.size(); i++)
            replaceParameter(eff.terms[i], paramToReplace, objectIndex);
        break;
    case DET_TIMED_EFFECT:
        replaceParameter(eff.timedEffect, paramToReplace, objectIndex);
        break;
    case DET_FORALL:
        replaceParameter(eff.terms[0], paramToReplace, objectIndex);
        break;
    case DET_WHEN:
        replaceParameter(eff.condition, paramToReplace, objectIndex);
        replaceParameter(eff.timedEffect, paramToReplace, objectIndex);
        break;
    case DET_ASSIGNMENT:
        replaceParameter(eff.assignment, paramToReplace, objectIndex);
        break;
    }
}

// Replaces a parameter with a given object in an effect
void Preprocess::replaceParameter(AssignmentContinuousEffect &eff, unsigned int paramToReplace, unsigned int objectIndex) {
    replaceParameter(eff.fluent, paramToReplace, objectIndex);
    if (eff.contEff.product)
        replaceParameter(eff.contEff.numExp, paramToReplace, objectIndex);
}

// Replaces a parameter with a given object in an effect
void Preprocess::replaceParameter(TimedEffect &eff, unsigned int paramToReplace, unsigned int objectIndex) {
    switch (eff.type) {
    case TE_AND:
        for (unsigned int i = 0; i < eff.terms.size(); i++)
            replaceParameter(eff.terms[i], paramToReplace, objectIndex);
        break;
    case TE_NOT:
        replaceParameter(eff.terms[0], paramToReplace, objectIndex);
        break;
    case TE_LITERAL:
    case TE_NEG_LITERAL:
        replaceParameter(eff.literal, paramToReplace, objectIndex);
        break;
    case TE_ASSIGNMENT:    
        replaceParameter(eff.assignment, paramToReplace, objectIndex);
        break;
    case TE_OR:;
    }
}
        
// Replaces a parameter with a given object in a precondition
void Preprocess::replaceParameter(Precondition &term, unsigned int paramToReplace, unsigned int objectIndex) {
    switch (term.type) {
    case PT_LITERAL:
        replaceParameter(term.literal, paramToReplace, objectIndex);
        break;
    case PT_AND:
    case PT_OR:
        for (unsigned int i = 0; i < term.terms.size(); i++)
            replaceParameter(term.terms[i], paramToReplace, objectIndex);
        break;
    case PT_NOT:
    case PT_EXISTS:
    case PT_FORALL:
        replaceParameter(term.terms[0], paramToReplace, objectIndex);
        break;
    case PT_IMPLY:
        replaceParameter(term.terms[0], paramToReplace, objectIndex);
        replaceParameter(term.terms[1], paramToReplace, objectIndex);
        break;
    default:
        replaceParameter(term.goal, paramToReplace, objectIndex);
    }
}

// Replaces a parameter with a given object in a precondition
void Preprocess::replaceParameter(DurativeCondition &term, unsigned int paramToReplace, unsigned int objectIndex) {
    switch (term.type) {
    case CT_AND:
        for (unsigned int i = 0; i < term.conditions.size(); i++)
            replaceParameter(term.conditions[i], paramToReplace, objectIndex);
        break;
    case CT_GOAL:
    case CT_PREFERENCE:
        replaceParameter(term.goal, paramToReplace, objectIndex);
        break;
    case CT_FORALL:
        replaceParameter(term.conditions[0], paramToReplace, objectIndex);
        break;
    }
}
    
// Replaces a parameter with a given object in a term
void Preprocess::replaceParameter(Term &term, unsigned int paramToReplace, unsigned int objectIndex) {
    if (term.type == TERM_PARAMETER && term.index == paramToReplace) {
        term.type = TERM_CONSTANT;
        term.index = objectIndex;
    }
}
    
// Replaces a parameter with a given object in a literal
void Preprocess::replaceParameter(Literal &literal, unsigned int paramToReplace, unsigned int objectIndex) {
    for (unsigned int i = 0; i < literal.params.size(); i++)
        replaceParameter(literal.params[i], paramToReplace, objectIndex);
}

// Replaces a parameter with a given object in a goal description
void Preprocess::replaceParameter(GoalDescription &goal, unsigned int paramToReplace, unsigned int objectIndex) {
    switch (goal.type) {
    case GD_LITERAL:
    case GD_NEG_LITERAL:
        replaceParameter(goal.literal, paramToReplace, objectIndex);
        break;
    case GD_AND:
    case GD_OR:
        for (unsigned int i = 0; i < goal.terms.size(); i++)
            replaceParameter(goal.terms[i], paramToReplace, objectIndex);
        break;
    case GD_NOT:
    case GD_EXISTS:
    case GD_FORALL:
        replaceParameter(goal.terms[0], paramToReplace, objectIndex);
        break;
    case GD_IMPLY:
        replaceParameter(goal.terms[0], paramToReplace, objectIndex);
        replaceParameter(goal.terms[1], paramToReplace, objectIndex);
        break;
    case GD_F_CMP:
        for (unsigned int i = 0; i < goal.exp.size(); i++)
            replaceParameter(goal.exp[i], paramToReplace, objectIndex);
        break;
    case GD_EQUALITY:
    case GD_INEQUALITY:
        replaceParameter(goal.eqTerms[0], paramToReplace, objectIndex);
        replaceParameter(goal.eqTerms[1], paramToReplace, objectIndex);
        break;
    }
}

// Replaces a parameter with a given object in a numeric expression
void Preprocess::replaceParameter(NumericExpression &exp, unsigned int paramToReplace, unsigned int objectIndex) {
    if (exp.type == NET_FUNCTION)
        replaceParameter(exp.function, paramToReplace, objectIndex);
    else if (exp.type == NET_TERM)
        replaceParameter(exp.term, paramToReplace, objectIndex);
    else {
        for (unsigned int i = 0; i < exp.operands.size(); i++)
            replaceParameter(exp.operands[i], paramToReplace, objectIndex);
    }
}

// Replaces a parameter with a given object in a fluent assignment
void Preprocess::replaceParameter(FluentAssignment &fa, unsigned int paramToReplace, unsigned int objectIndex) {
    replaceParameter(fa.fluent, paramToReplace, objectIndex);
    replaceParameter(fa.exp, paramToReplace, objectIndex);
}

// Replaces a parameter with a given object in an effect expression
void Preprocess::replaceParameter(EffectExpression &exp, unsigned int paramToReplace, unsigned int objectIndex) {
    switch (exp.type) {
    case EE_TERM:
        replaceParameter(exp.term, paramToReplace, objectIndex);
        break;
    case EE_OPERATION:
        for (unsigned int i = 0; i < exp.operands.size(); i++)
            replaceParameter(exp.operands[i], paramToReplace, objectIndex);
        break;
    case EE_FLUENT:
        replaceParameter(exp.fluent, paramToReplace, objectIndex);
        break;
    default:;
    }
}

// Removes all implications from a precondition: P -> Q = NOT(P) OR Q
void Preprocess::removeImplications(Precondition &prec) {
    switch (prec.type) {
    case PT_AND:
    case PT_OR:
        for (unsigned int i = 0; i < prec.terms.size(); i++)
            removeImplications(prec.terms[i]);
        break;
    case PT_NOT:
        removeImplications(prec.terms[0]);
        break;
    case PT_IMPLY:
        {
            prec.type = PT_OR;
            Precondition &p = prec.terms[0];
            Precondition notP;
            notP.type = PT_NOT;
            notP.terms.push_back(p);
            prec.terms[0] = notP;
            removeImplications(prec.terms[0]);
            removeImplications(prec.terms[1]);
        }
        break;
    default:;
    }    
}

// Removes all implications from a precondition: P -> Q = NOT(P) OR Q
void Preprocess::removeImplications(DurativeCondition &prec) {
    switch (prec.type) {
    case CT_AND:
        for (unsigned int i = 0; i < prec.conditions.size(); i++)
            removeImplications(prec.conditions[i]);
        break;
    case CT_GOAL:
        removeImplications(prec.goal);
        break;
    case CT_FORALL:
    case CT_PREFERENCE:;
    } 
}

// Removes all implications from an effect: P -> Q = NOT(P) OR Q
void Preprocess::removeImplications(Effect &eff) {
    switch (eff.type) {
    case ET_AND:
        for (unsigned int i = 0; i < eff.terms.size(); i++)
            removeImplications(eff.terms[i]);
        break;
    case ET_NOT:
        removeImplications(eff.terms[0]);
        break;
    case ET_WHEN:
        removeImplications(eff.goal);
        removeImplications(eff.terms[0]);
        break;
    default:;
    }
}

// Removes all implications from an effect: P -> Q = NOT(P) OR Q
void Preprocess::removeImplications(DurativeEffect &eff) {
    switch (eff.type) {
    case DET_AND:
        for (unsigned int i = 0; i < eff.terms.size(); i++)
            removeImplications(eff.terms[i]);
        break;
    case DET_WHEN:
        removeImplications(eff.condition);
        break;
    case DET_TIMED_EFFECT:
    case DET_FORALL:
    case DET_ASSIGNMENT:;
    }
}
    
// Removes all implications from a goal description: P -> Q = NOT(P) OR Q
void Preprocess::removeImplications(GoalDescription &goal) {
    switch (goal.type) {
    case GD_AND:
    case GD_OR:
        for (unsigned int i = 0; i < goal.terms.size(); i++)
            removeImplications(goal.terms[i]);
        break;
    case GD_NOT:
        removeImplications(goal.terms[0]);
        break;
    case GD_IMPLY:
        {
            goal.type = GD_OR;
            GoalDescription &p = goal.terms[0];
            GoalDescription notP;
            notP.type = GD_NOT;
            notP.terms.push_back(p);
            goal.terms[0] = notP;
            removeImplications(goal.terms[0]);
            removeImplications(goal.terms[1]);
        }
        break;
    default:;
    }
}

// Removes negations and empty, unary ot nested conjunctions/disjunctions in a precondition
void Preprocess::preconditionOptimization(Precondition *prec, Precondition *parent, 
        unsigned int termNumber, Action &a) {
    unsigned int n = prec->terms.size();        
    switch (prec->type) {
        case PT_AND:
        case PT_OR:
            if (n == 0) prec->type = PT_AND;    // Empty conjuntion/disjunction
            else if (n == 1) {                  // Unary conjuntion/disjunction
                Precondition child = prec->terms[0];
                if (parent == nullptr) {
                    a.precondition = child;
                    preconditionOptimization(&a.precondition, nullptr, 0, a);
                } else {
                    parent->terms[termNumber] = child;
                    preconditionOptimization(&(parent->terms[termNumber]), parent, termNumber, a);
                }
            } else {                            // Multiple conjuntion/disjunction
                for (unsigned int i = 0; i < n; i++)
                    preconditionOptimization(&(prec->terms[i]), prec, i, a);
            }
            break;
        case PT_NOT:
            negationOptimization(prec, parent, termNumber, a);
            break;
        default:;
    }
}

// Removes negations and empty, unary ot nested conjunctions/disjunctions in a precondition
void Preprocess::preconditionOptimization(DurativeCondition *prec, DurativeCondition *parent, unsigned int termNumber, DurativeAction &a) {
    unsigned int n = prec->conditions.size();
    switch (prec->type) {
    case CT_AND:
        if (n == 1) {                       // Unary conjuntion/disjunction
            DurativeCondition child = prec->conditions[0];
            if (parent == nullptr) {
                a.condition = child;
                preconditionOptimization(&a.condition, nullptr, 0, a);
            } else {
                parent->conditions[termNumber] = child;
                preconditionOptimization(&(parent->conditions[termNumber]), parent, termNumber, a);
            }
        } else {                            // Multiple conjuntion/disjunction
            for (unsigned int i = 0; i < n; i++)
                preconditionOptimization(&(prec->conditions[i]), prec, i, a);
        }
        break;
    case CT_GOAL:
        goalOptimization(&(prec->goal), prec, nullptr, 0);
        break;
    case CT_FORALL:
    case CT_PREFERENCE:;
    }
}

// Removes negations: prec is a negation in a precondition
void Preprocess::negationOptimization(Precondition *prec, Precondition *parent, 
        unsigned int termNumber, Action &a) {
    Precondition &child = prec->terms[0];
    switch (child.type) {
    case PT_LITERAL:
        prec->type = PT_NEG_LITERAL;
        prec->literal = child.literal;
        prec->terms.clear();
        break;
    case PT_AND:    // ~(A ^ B) = ~A v ~B
    case PT_OR:     // ~(A v B) = ~A ^ ~B
        prec->type = child.type == PT_AND ? PT_OR : PT_AND;
        for (unsigned int i = 0; i < child.terms.size(); i++) {
            Precondition notP;
            notP.type = PT_NOT;
            notP.terms.push_back(child.terms[i]);
            prec->terms.push_back(notP);
        }
        prec->terms.erase(prec->terms.begin());
        for (unsigned int i = 0; i < prec->terms.size(); i++)
            negationOptimization(&(prec->terms[i]), prec, i, a);
        break;
    case PT_NOT:    // Double negation: ~~A = A
        if (parent == nullptr) {
            a.precondition = child.terms[0];
            preconditionOptimization(&a.precondition, nullptr, 0, a);
        } else {
            Precondition grandchild = child.terms[0];
            parent->terms[termNumber].type = grandchild.type;
            parent->terms[termNumber].goal = grandchild.goal;
            parent->terms[termNumber].literal = grandchild.literal;
            parent->terms[termNumber].preferenceName = grandchild.preferenceName;
            parent->terms[termNumber].parameters = grandchild.parameters;
            parent->terms[termNumber].terms.clear();
            for (int i = 0; i < grandchild.terms.size(); i++)
                parent->terms[termNumber].terms.push_back(grandchild.terms[i]);
            preconditionOptimization(&(parent->terms[termNumber]), parent, termNumber, a); 
        }
        break;
    case PT_F_CMP:
        prec->type = PT_F_CMP;
        prec->goal = prec->terms[0].goal;
        prec->goal.comparator = negateComparator(prec->goal.comparator);
        prec->terms.clear();
        break;
    case PT_EQUALITY:
        prec->type = PT_EQUALITY;
        prec->goal = prec->terms[0].goal;
        prec->goal.type = prec->goal.type == GD_EQUALITY ? GD_INEQUALITY : GD_EQUALITY;
        prec->terms.clear();
        break;
    default:;
    }
}

// Returns the negation of the given comparator
Comparator Preprocess::negateComparator(Comparator c) {
    switch (c) {
    case CMP_EQ:            return CMP_NEQ;
    case CMP_LESS:          return CMP_GREATER_EQ;
    case CMP_LESS_EQ:       return CMP_GREATER;
    case CMP_GREATER:       return CMP_LESS_EQ;
    case CMP_GREATER_EQ:    return CMP_LESS;
    default:                return CMP_EQ;
    }
}

// Removes negations and empty, unary or nested conjunctions/disjunctions in an effect
void Preprocess::effectOptimization(Effect *eff, Effect *parent, unsigned int termNumber, Action &a) {
    unsigned int n = eff->terms.size();        
    switch (eff->type) {
        case ET_AND:
            if (n == 1) {                // Unary conjuntion
                Effect child = eff->terms[0];
                if (parent == nullptr) {
                    a.effect = child;
                    effectOptimization(&a.effect, nullptr, 0, a);
                } else {
                    parent->terms[termNumber] = child;
                    effectOptimization(&(parent->terms[termNumber]), parent, termNumber, a);
                }
            } else {                            // Multiple conjuntion/disjunction
                for (unsigned int i = 0; i < n; i++)
                    effectOptimization(&(eff->terms[i]), eff, i, a);
            }
            break;
        case PT_NOT:
            negationOptimization(eff, parent, termNumber, a);
            break;
        case ET_WHEN:
            goalOptimization(&(eff->goal), nullptr, eff, nullptr, 0);
            effectOptimization(&(eff->terms[0]), eff, 0, a);
            break;
        default:;
    }
}

// Removes negations and empty, unary or nested conjunctions/disjunctions in an effect
void Preprocess::effectOptimization(DurativeEffect *eff, DurativeEffect *parent, unsigned int termNumber, DurativeAction &a) {
    unsigned int n = eff->terms.size();        
    switch (eff->type) {
        case DET_AND:
            if (n == 1) {                // Unary conjuntion
                DurativeEffect child = eff->terms[0];
                if (parent == nullptr) {
                    a.effect = child;
                    effectOptimization(&a.effect, nullptr, 0, a);
                } else {
                    parent->terms[termNumber] = child;
                    effectOptimization(&(parent->terms[termNumber]), parent, termNumber, a);
                }
            } else {                            // Multiple conjuntion/disjunction
                for (unsigned int i = 0; i < n; i++)
                    effectOptimization(&(eff->terms[i]), eff, i, a);
            }
            break;
        case DET_TIMED_EFFECT:
            effectOptimization(&(eff->timedEffect), nullptr, 0, eff);
            break;
        case DET_WHEN:
            effectOptimization(&(eff->timedEffect), nullptr, 0, eff);
            preconditionOptimization(&(eff->condition), nullptr, 0, eff);
            break;
        case DET_FORALL:    
        case DET_ASSIGNMENT:;
    }
}

// Removes negations and empty, unary or nested conjunctions/disjunctions in an effect
void Preprocess::effectOptimization(TimedEffect* eff, TimedEffect* parent, unsigned int termNumber, DurativeEffect *parentEff) {
    unsigned int n = eff->terms.size();        
    switch (eff->type) {
        case TE_AND:
        case TE_OR:
            if (n == 1) {                // Unary conjuntion
                TimedEffect child = eff->terms[0];
                if (parent == nullptr) {
                    parentEff->timedEffect = child;
                    effectOptimization(&(parentEff->timedEffect), nullptr, 0, parentEff);
                } else {
                    parent->terms[termNumber] = child;
                    effectOptimization(&(parent->terms[termNumber]), parent, termNumber, nullptr);
                }
            } else {                            // Multiple conjuntion/disjunction
                for (unsigned int i = 0; i < n; i++)
                    effectOptimization(&(eff->terms[i]), eff, i, nullptr);
            }
            break;
        case TE_NOT:
            negationOptimization(eff, parent, termNumber, parentEff);
            break;
        case TE_LITERAL:
        case TE_ASSIGNMENT:
        case TE_NEG_LITERAL:;
    }
}

// Removes negations and empty, unary ot nested conjunctions/disjunctions in a precondition
void Preprocess::preconditionOptimization(DurativeCondition *prec, DurativeCondition *parent, unsigned int termNumber, DurativeEffect *parentEff) {
    unsigned int n = prec->conditions.size();
    switch (prec->type) {
    case CT_AND:
        if (n == 1) {                       // Unary conjuntion/disjunction
            DurativeCondition child = prec->conditions[0];
            if (parent == nullptr) {
                parentEff->condition = child;
                preconditionOptimization(&(parentEff->condition), nullptr, 0, parentEff);
            } else {
                parent->conditions[termNumber] = child;
                preconditionOptimization(&(parent->conditions[termNumber]), parent, termNumber, parentEff);
            }
        } else {                            // Multiple conjuntion/disjunction
            for (unsigned int i = 0; i < n; i++)
                preconditionOptimization(&(prec->conditions[i]), prec, i, parentEff);
        }
        break;
    case CT_GOAL:
        goalOptimization(&(prec->goal), prec, nullptr, 0);
        break;
    case CT_FORALL:
    case CT_PREFERENCE:;
    }
}

// Removes negations: eff is a negation in an effect
void Preprocess::negationOptimization(TimedEffect* eff, TimedEffect* parent, unsigned int termNumber, DurativeEffect *parentEff) {
    TimedEffect &child = eff->terms[0];
    switch (child.type) {
        case TE_AND:    // ~(A ^ B) = ~A v ~B
        case TE_OR:     // ~(A v B) = ~A ^ ~B
            eff->type = child.type == TE_AND ? TE_OR : TE_AND;
            for (unsigned int i = 0; i < child.terms.size(); i++) {
                TimedEffect notP;
                notP.type = TE_NOT;
                notP.terms.push_back(child.terms[i]);
                eff->terms.push_back(notP);
            }
            eff->terms.erase(eff->terms.begin());
            for (unsigned int i = 0; i < eff->terms.size(); i++)
                negationOptimization(&(eff->terms[i]), eff, i, parentEff);
            break;
        case TE_NOT:    // Double negation: ~~A = A
            if (parent != nullptr) {
                parent->terms[termNumber] = child.terms[0];
                effectOptimization(&(parent->terms[termNumber]), parent, termNumber, parentEff);
            } else {
                parentEff->timedEffect = child.terms[0];
                effectOptimization(&(parentEff->timedEffect), nullptr, 0, parentEff);
            }
            break;
        case TE_LITERAL:
            eff->type = TE_NEG_LITERAL;
            eff->literal = child.literal;
            eff->terms.clear();
            break;
        case TE_ASSIGNMENT:;
        case TE_NEG_LITERAL:;
    }
}
    

// Removes negations: eff is a negation in an effect
void Preprocess::negationOptimization(Effect *eff, Effect *parent, unsigned int termNumber, Action &a) {
    Effect &child = eff->terms[0];
    switch (child.type) {
    case ET_LITERAL:
        eff->type = ET_NEG_LITERAL;
        eff->literal = child.literal;
        eff->terms.clear();
        break;
    case ET_NOT:    // Double negation: ~~A = A
        if (parent == nullptr) {
            a.effect = child.terms[0];
            effectOptimization(&a.effect, nullptr, 0, a);
        } else {
            parent->terms[termNumber] = child.terms[0];
            effectOptimization(&(parent->terms[termNumber]), parent, termNumber, a); 
        }
        break;
    case ET_WHEN:
        effectOptimization(&(eff->terms[0]), eff, 0, a);
        break;
    default:;
    }
}
    
// Removes negations and empty, unary or nested conjunctions/disjunctions in a goal description
void Preprocess::goalOptimization(GoalDescription *goal, Precondition *parentPrec, Effect *parentEff, 
        GoalDescription *parent, unsigned int termNumber) {
    unsigned int n = goal->terms.size();        
    switch (goal->type) {
        case GD_AND:
        case GD_OR:
            if (n == 0) goal->type = GD_AND;    // Empty conjuntion/disjunction
            else if (n == 1) {                  // Unary conjuntion/disjunction
                GoalDescription child = goal->terms[0];
                if (parent != nullptr) {
                    parent->terms[termNumber] = child;
                    goalOptimization(&(parent->terms[termNumber]), nullptr, nullptr, parent, termNumber);
                } else if (parentPrec != nullptr) {
                    parentPrec->goal = child;
                    goalOptimization(&(parentPrec->goal), parentPrec, nullptr, nullptr, 0);
                } else {
                    parentEff->goal = child;
                    goalOptimization(&(parentEff->goal), nullptr, parentEff, nullptr, 0);
                }
            } else {                            // Multiple conjuntion/disjunction
                for (unsigned int i = 0; i < n; i++)
                    goalOptimization(&(goal->terms[i]), nullptr, nullptr, goal, i);
            }
            break;
        case GD_NOT:
            negationOptimization(goal, parentPrec, parentEff, parent, termNumber);
            break;
        default:;
    }
}

// Removes negations and empty, unary or nested conjunctions/disjunctions in a goal description
void Preprocess::goalOptimization(GoalDescription *goal, DurativeCondition *parentPrec,  
        GoalDescription *parent, unsigned int termNumber) {
    unsigned int n = goal->terms.size();        
    switch (goal->type) {
        case GD_AND:
        case GD_OR:
            if (n == 0) goal->type = GD_AND;    // Empty conjuntion/disjunction
            else if (n == 1) {                  // Unary conjuntion/disjunction
                GoalDescription child = goal->terms[0];
                if (parent != nullptr) {
                    parent->terms[termNumber] = child;
                    goalOptimization(&(parent->terms[termNumber]), nullptr, parent, termNumber);
                } else {
                    parentPrec->goal = child;
                    goalOptimization(&(parentPrec->goal), parentPrec, nullptr, 0);
                }
            } else {                            // Multiple conjuntion/disjunction
                for (unsigned int i = 0; i < n; i++)
                    goalOptimization(&(goal->terms[i]), nullptr, goal, i);
            }
            break;
        case GD_NOT:
            negationOptimization(goal, parentPrec, parent, termNumber);
            break;
        default:;
    }
}

// Removes negations: goal is a negation in a goal description
void Preprocess::negationOptimization(GoalDescription *goal, DurativeCondition *parentPrec, 
     GoalDescription *parent, unsigned int termNumber) {
    GoalDescription &child = goal->terms[0];
    switch (child.type) {
    case GD_LITERAL:
         //cout << "Child literal" << endl;
        goal->type = GD_NEG_LITERAL;
        goal->literal = child.literal;
        goal->time = child.time;
        goal->terms.clear();
        break;
    case GD_AND:    // ~(A ^ B) = ~A v ~B
    case GD_OR:     // ~(A v B) = ~A ^ ~B
        //cout << "Child and/or" << endl;
        goal->type = child.type == GD_AND ? GD_OR : GD_AND;
        for (unsigned int i = 0; i < child.terms.size(); i++) {
            GoalDescription notP;
            notP.type = GD_NOT;
            notP.terms.push_back(child.terms[i]);
            goal->terms.push_back(notP);
        }
        goal->terms.erase(goal->terms.begin());
        for (unsigned int i = 0; i < goal->terms.size(); i++)
            negationOptimization(&(goal->terms[i]), nullptr, nullptr, goal, i);
        break;
    case GD_NOT:    // Double negation: ~~A = A
        //cout << "Child not" << endl;
        if (parent != nullptr) {
            parent->terms[termNumber] = child.terms[0];
            goalOptimization(&(parent->terms[termNumber]), nullptr, nullptr, parent, termNumber);
        } else {
            parentPrec->goal = child.terms[0];
            goalOptimization(&(parentPrec->goal), parentPrec, nullptr, 0);
        }
        break;
    case GD_F_CMP:
         //cout << "Child cmp" << endl;
        goal->type = GD_F_CMP;
        goal->time = child.time;
        goal->exp = goal->terms[0].exp;
        goal->comparator = negateComparator(child.comparator);
        goal->terms.clear();
        break;
    case GD_EQUALITY:
         //cout << "Child equality" << endl;
        goal->type = GD_INEQUALITY;
        goal->time = child.time;
        goal->eqTerms = goal->terms[0].eqTerms;
        goal->terms.clear();
        break;
    default:;
    }
}

// Removes negations: goal is a negation in a goal description
void Preprocess::negationOptimization(GoalDescription *goal, Precondition *parentPrec, Effect *parentEff, 
        GoalDescription *parent, unsigned int termNumber) {
    GoalDescription &child = goal->terms[0];
    switch (child.type) {
    case GD_LITERAL:
        goal->type = GD_NEG_LITERAL;
        goal->literal = child.literal;
        goal->time = child.time;
        goal->terms.clear();
        break;
    case GD_AND:    // ~(A ^ B) = ~A v ~B
    case GD_OR:     // ~(A v B) = ~A ^ ~B
        goal->type = child.type == GD_AND ? GD_OR : GD_AND;
        for (unsigned int i = 0; i < child.terms.size(); i++) {
            GoalDescription notP;
            notP.type = GD_NOT;
            notP.terms.push_back(child.terms[i]);
            goal->terms.push_back(notP);
        }
        goal->terms.erase(goal->terms.begin());
        for (unsigned int i = 0; i < goal->terms.size(); i++)
            negationOptimization(&(goal->terms[i]), nullptr, nullptr, goal, i);
        break;
    case GD_NOT:    // Double negation: ~~A = A
        if (parent != nullptr) {
            parent->terms[termNumber] = child.terms[0];
            goalOptimization(&(parent->terms[termNumber]), nullptr, nullptr, parent, termNumber);
        } else if (parentPrec != nullptr) {
            parentPrec->goal = child.terms[0];
            goalOptimization(&(parentPrec->goal), parentPrec, nullptr, nullptr, 0);
        } else {
            parentEff->goal = child.terms[0];
            goalOptimization(&(parentEff->goal), nullptr, parentEff, nullptr, 0);
        }
        break;
    case GD_F_CMP:
        goal->type = GD_F_CMP;
        goal->time = child.time;
        goal->exp = goal->terms[0].exp;
        goal->comparator = negateComparator(child.comparator);
        goal->terms.clear();
        break;
    case GD_EQUALITY:
        goal->type = GD_INEQUALITY;
        goal->time = child.time;
        goal->eqTerms = goal->terms[0].eqTerms;
        goal->terms.clear();
        break;
    default:;
    }
}

// Checks if there are conditional effects
bool Preprocess::existingConditionalEffects(Effect &eff) {
    bool found = false;
    switch (eff.type) {
    case ET_AND:
        for (unsigned int i = 0; i < eff.terms.size() && !found; i++)
            found = existingConditionalEffects(eff.terms[i]);
        break;
    case ET_NOT:
        found = existingConditionalEffects(eff.terms[0]);
        break;
    case ET_WHEN:
        found = true;
        break;
    default:;
    }
    return found;
}

// Checks if there are conditional effects
bool Preprocess::existingConditionalEffects(DurativeEffect &eff) {
    bool found = false;
    switch (eff.type) {
    case DET_AND:
        for (unsigned int i = 0; i < eff.terms.size() && !found; i++)
            found = existingConditionalEffects(eff.terms[i]);
        break;
    case DET_WHEN:
        found = true;
        break;
    case DET_TIMED_EFFECT:
    case DET_FORALL:
    case DET_ASSIGNMENT:;
    }
    return found;
}
    
// Builds one operator (or more if there are disjunctions in the precondition) from the given action
// and stores them in prepTask->operators
void Preprocess::buildOperators(Action &a, bool isTIL, bool isGoal) {
    Operator op(true, isTIL, isGoal);
    vector<Operator> opList = buildOperatorPrecondition(a.precondition, a, &op);
    for (unsigned int i = 0; i < opList.size(); i++) {
        if (checkValidOperator(opList[i], a.parameters.size())) {
            terminateBuildingOperator(opList[i], a, getOpName(a.name, i, opList.size()));
        }
    }
}

// Builds one operator (or more if there are disjunctions in the precondition) from the given action
// and stores them in prepTask->operators
void Preprocess::buildOperators(DurativeAction &a, bool isTIL, bool isGoal) {
    Operator op(false, isTIL, isGoal);
    vector<Operator> opList = buildOperatorPrecondition(a.condition, a, &op);
    for (unsigned int i = 0; i < opList.size(); i++) {
        if (checkValidOperator(opList[i], a.parameters.size())) {
            opList[i].isGoal = isGoal;
            terminateBuildingOperator(opList[i], a, getOpName(a.name, i, opList.size()));
        }
    }
}
    
// Builds the operator precondition from the action precondition
vector<Operator> Preprocess::buildOperatorPrecondition(Precondition &prec, Action &a, Operator *op) {
    vector<Operator> opList;
    switch (prec.type) {
    case PT_LITERAL:
    case PT_NEG_LITERAL:
        {
            Term value;
            value.type = TERM_CONSTANT;
            value.index = prec.type == PT_LITERAL ? task->CONSTANT_TRUE : task->CONSTANT_FALSE;
            op->addLiteralToPreconditions(prec.literal, value, AT_START);
            opList.push_back(*op);
        }
        break;
    case PT_AND:
        if (prec.terms.size() < 1) opList.push_back(*op);
        else opList = buildOperatorPreconditionAnd(prec, a, op, 0);
        break;
    case PT_OR:
        if (prec.terms.size() == 1) {
            opList = buildOperatorPreconditionAnd(prec, a, op, 0);
        }
        else {
            for (unsigned int i = 0; i < prec.terms.size(); i++) {
                Operator newOp = *op;
                vector<Operator> partial = buildOperatorPrecondition(prec.terms[i], a, &newOp);
                opList.insert(opList.begin(), partial.begin(), partial.end());
            }
        }
        break;
    case PT_F_CMP:
        op->addNumericPrecondition(prec.goal, AT_START);
        opList.push_back(*op);
        break;
    case PT_EQUALITY:
        op->addEquality(prec.goal.eqTerms[0], prec.goal.eqTerms[1], prec.goal.type == GD_EQUALITY);
        opList.push_back(*op);
        break;
    case PT_PREFERENCE:
        {
            OpPreference pref;
            pref.name = prec.preferenceName;
            pref.preference = prec.goal;
            op->preference.push_back(pref);
            opList.push_back(*op);
        }
        break;
    case PT_GOAL:
        opList = buildOperatorPrecondition(prec.goal, a, op);
        break;
    default:;
    }
    return opList;
}

// Builds the operator precondition from the action precondition
vector<Operator> Preprocess::buildOperatorPrecondition(DurativeCondition &prec, DurativeAction &a, Operator *op) {
    vector<Operator> opList;
    //cout << "Aqui (" << prec.type << ") ";
    switch (prec.type) {
    case CT_AND:
        //cout << "and" << endl;
        if (prec.conditions.size() < 1) opList.push_back(*op);
        else opList = buildOperatorPreconditionAnd(prec, a, op, 0);
        break;
    case CT_GOAL:
        //cout << "goal" << endl; 
        opList = buildOperatorPrecondition(prec.goal, a, op);
        break;
    case CT_PREFERENCE:
        //cout << "pref" << endl; 
        {
            OpPreference pref;
            pref.name = prec.preferenceName;
            pref.preference = prec.goal;
            op->preference.push_back(pref);
            opList.push_back(*op);
        }
        break;
    default:;
    }
    //cout << " with size " << opList.size() << endl;
    return opList;
}
    
// Builds the operator precondition from the action precondition
vector<Operator> Preprocess::buildOperatorPrecondition(GoalDescription &goal, Action &a, Operator *op) {
    vector<Operator> opList;
    switch (goal.type) {
    case GD_LITERAL:
    case GD_NEG_LITERAL:
        {
            Term value;
            value.type = TERM_CONSTANT;
            value.index = goal.type == GD_LITERAL ? task->CONSTANT_TRUE : task->CONSTANT_FALSE;
            op->addLiteralToPreconditions(goal.literal, value, AT_START);
            opList.push_back(*op);
        }
        break;
    case GD_AND:
        if (goal.terms.size() < 1) opList.push_back(*op);
        else opList = buildOperatorPreconditionAnd(goal, a, op, 0);
        break;
    case GD_EQUALITY:
    case GD_INEQUALITY:
        op->addEquality(goal.eqTerms[0], goal.eqTerms[1], goal.type == GD_EQUALITY);
        opList.push_back(*op);
        break;
    case GD_OR:
        for (unsigned int i = 0; i < goal.terms.size(); i++) {
            Operator newOp = *op;
            vector<Operator> partial = buildOperatorPrecondition(goal.terms[i], a, &newOp);
            opList.insert(opList.begin(), partial.begin(), partial.end());
        }
        break;
    case GD_F_CMP:
        op->addNumericPrecondition(goal, AT_START);
        opList.push_back(*op);
        break;
    default:;
    }
    return opList;
}

// Builds the operator precondition from the action precondition
vector<Operator> Preprocess::buildOperatorPrecondition(GoalDescription &goal, DurativeAction &a, Operator *op) {
    vector<Operator> opList;
    switch (goal.type) {
    case GD_LITERAL:
    case GD_NEG_LITERAL:
        {
            Term value;
            value.type = TERM_CONSTANT;
            value.index = goal.type == GD_LITERAL ? task->CONSTANT_TRUE : task->CONSTANT_FALSE;
            op->addLiteralToPreconditions(goal.literal, value, goal.time);
            opList.push_back(*op);
        }
        break;
    case GD_AND:
        if (goal.terms.size() < 1) opList.push_back(*op);
        else opList = buildOperatorPreconditionAnd(goal, a, op, 0);
        break;
    case GD_EQUALITY:
    case GD_INEQUALITY:
        op->addEquality(goal.eqTerms[0], goal.eqTerms[1], goal.type == GD_EQUALITY);
        opList.push_back(*op);
        break;
    case GD_OR:
        for (unsigned int i = 0; i < goal.terms.size(); i++) {
            Operator newOp = *op;
            vector<Operator> partial = buildOperatorPrecondition(goal.terms[i], a, &newOp);
            opList.insert(opList.begin(), partial.begin(), partial.end());
        }
        break;
    case GD_F_CMP:
        op->addNumericPrecondition(goal, goal.time);
        opList.push_back(*op);
        break;
    default:;
    }
    return opList;
}
    
// Builds the operator precondition from a conjunction in the action precondition
vector<Operator> Preprocess::buildOperatorPreconditionAnd(Precondition &prec, Action &a, Operator *op, unsigned int numTerm) {
    vector<Operator> opList = buildOperatorPrecondition(prec.terms[numTerm], a, op);
    if (++numTerm >= prec.terms.size()) return opList;
    vector<Operator> result;
    for (unsigned int i = 0; i < opList.size(); i++) {
        vector<Operator> partial = buildOperatorPreconditionAnd(prec, a, &opList[i], numTerm);
        result.insert(result.end(), partial.begin(), partial.end());
    }
    return result;
}

// Builds the operator precondition from a conjunction in the action precondition
vector<Operator> Preprocess::buildOperatorPreconditionAnd(DurativeCondition &prec, DurativeAction &a, Operator *op, unsigned int numTerm) {
    vector<Operator> opList = buildOperatorPrecondition(prec.conditions[numTerm], a, op);
    if (++numTerm >= prec.conditions.size()) return opList;
    vector<Operator> result;
    for (unsigned int i = 0; i < opList.size(); i++) {
        vector<Operator> partial = buildOperatorPreconditionAnd(prec, a, &opList[i], numTerm);
        result.insert(result.end(), partial.begin(), partial.end());
    }
    return result;
}
    
// Builds the operator precondition from a conjunction in the action precondition
vector<Operator> Preprocess::buildOperatorPreconditionAnd(GoalDescription &goal, Action &a, Operator *op, unsigned int numTerm) {
    vector<Operator> opList = buildOperatorPrecondition(goal.terms[numTerm], a, op);
    if (++numTerm >= goal.terms.size()) return opList;
    vector<Operator> result;
    for (unsigned int i = 0; i < opList.size(); i++) {
        vector<Operator> partial = buildOperatorPreconditionAnd(goal, a, &opList[i], numTerm);
        result.insert(result.end(), partial.begin(), partial.end());
    }
    return result;
}

// Builds the operator precondition from a conjunction in the action precondition
vector<Operator> Preprocess::buildOperatorPreconditionAnd(GoalDescription &goal, DurativeAction &a, Operator *op, unsigned int numTerm) {
    vector<Operator> opList = buildOperatorPrecondition(goal.terms[numTerm], a, op);
    if (++numTerm >= goal.terms.size()) return opList;
    vector<Operator> result;
    for (unsigned int i = 0; i < opList.size(); i++) {
        vector<Operator> partial = buildOperatorPreconditionAnd(goal, a, &opList[i], numTerm);
        result.insert(result.end(), partial.begin(), partial.end());
    }
    return result;
}

// Checks if the operator is valid, i.e. it has no contradictory preconditions
bool Preprocess::checkValidOperator(Operator &op, unsigned int numParameters) {
    bool ok = true;
    std::unique_ptr<unsigned int[]> paramValues =  std::make_unique<unsigned int[]>(numParameters);
    std::unique_ptr<unsigned int[]> equivalences = std::make_unique<unsigned int[]>(numParameters);
    for (unsigned int i = 0; i < numParameters; i++) {
        paramValues[i] = MAX_UNSIGNED_INT;
        equivalences[i] = i;
    }
    if (!setParameterValues(paramValues, equivalences, op.equality)) ok = false;
    else if (!checkEqualities(paramValues, equivalences, op.equality, numParameters)) ok = false;
    else if (!checkPreconditions(paramValues, op.atStart.prec)) ok = false;
    else if (!checkPreconditions(paramValues, op.overAllPrec)) ok = false;
    else if (!checkPreconditions(paramValues, op.atEnd.prec)) ok = false;
    return ok;
}

// Sets the values of the parameters according to the equality constraints
bool Preprocess::setParameterValues(std::unique_ptr<unsigned int[]> &paramValues, std::unique_ptr<unsigned int[]> &equivalences, const std::vector<OpEquality> &equality) {
    unsigned int p1, p2;
    for (unsigned int i = 0; i < equality.size(); i++) {
        if (equality[i].equal) {
            p1 = equality[i].value1.index;
            p2 = equality[i].value2.index;
            if (equality[i].value1.type == TERM_PARAMETER && equality[i].value2.type == TERM_CONSTANT) {          // (= ?p X)
                if (paramValues[p1] != MAX_UNSIGNED_INT && paramValues[p1] != p2) return false;
                paramValues[p1] = p2;
            } else if (equality[i].value2.type == TERM_PARAMETER && equality[i].value1.type == TERM_CONSTANT) {   // (= X ?p)
                if (paramValues[p2] != MAX_UNSIGNED_INT && paramValues[p2] != p1) return false;
                paramValues[p2] = p1;
            } else if (equality[i].value1.type == TERM_PARAMETER) {                                     // (= ?p1 ?p2)
                if (p1 < p2) equivalences[p2] = p1;
                else equivalences[p1] = p2;
            } else {                                                                        // (= X Y)
                if (p1 != p2) return false;
            }
        }
    }
    return true;
}

// Checks if equalities hold in the operator
bool Preprocess::checkEqualities(std::unique_ptr<unsigned int[]> &paramValues, std::unique_ptr<unsigned int[]> &equivalences, 
    std::vector<OpEquality> &equality, unsigned int numParameters) {
    unsigned int p1, p2;
    for (unsigned int i = 0; i < numParameters; i++) {
        p1 = equivalences[i];
        if (p1 < i) {
            if (paramValues[p1] == MAX_UNSIGNED_INT) paramValues[p1] = paramValues[i];
            else if (paramValues[i] == MAX_UNSIGNED_INT) paramValues[i] = paramValues[p1];
            else if (paramValues[i] != paramValues[p1]) return false;
        }
    }
    for (unsigned int i = 0; i < equality.size(); i++) {
        p1 = equality[i].value1.index;
        p2 = equality[i].value2.index;
        if (!equality[i].equal) {
            if (equality[i].value1.type == TERM_PARAMETER && !equality[i].value2.type == TERM_PARAMETER) {          // (!= ?p X)
                if (paramValues[p1] != MAX_UNSIGNED_INT && paramValues[p1] == p2) return false;
            } else if (equality[i].value2.type == TERM_PARAMETER && !equality[i].value1.type == TERM_PARAMETER) {   // (!= X ?p)
                if (paramValues[p2] != MAX_UNSIGNED_INT && paramValues[p2] == p1) return false;
            } else if (equality[i].value1.type == TERM_PARAMETER) {                                     // (!= ?p1 ?p2)
                if (paramValues[p1] != MAX_UNSIGNED_INT && paramValues[p2] != MAX_UNSIGNED_INT
                    && paramValues[p1] == paramValues[p2]) return false;
            } else {                                                                        // (!= X Y)
                if (p1 == p2) return false;
            }
        } else {
            if (equality[i].value1.type == TERM_PARAMETER && equality[i].value2.type == TERM_PARAMETER) {          // (= ?p1 ?p2)
                if (paramValues[p1] != MAX_UNSIGNED_INT && paramValues[p2] != MAX_UNSIGNED_INT
                    && paramValues[p1] != paramValues[p2]) return false;
            }
        }
    }
    return true;
}

// Checks if preconditions hold in the operator
bool Preprocess::checkPreconditions(std::unique_ptr<unsigned int[]> &paramValues, vector<OpFluent> &precs) {
    if (precs.size() != 0) {
        unordered_map<string,string> map(32);
        for (unsigned int i = 0; i < precs.size(); i++) {
            string varName = precs[i].getVarName(paramValues);
            string valueName = precs[i].getValueName(paramValues);
            unordered_map<string,string>::const_iterator got = map.find(varName);
            if (got == map.end()) map[varName] = valueName;
            else if (valueName.compare(got->second) != 0) return false;
        }
    }
    return true;
}

// Terminates to build de operator (only its preconditions are built)
void Preprocess::terminateBuildingOperator(Operator &op, Action &a, std::string name) {
    op.name = name;
    op.parameters = a.parameters;
    NumericExpression durExp(EPSILON);
    op.duration.emplace_back(Symbol::EQUAL, durExp);
    buildOperatorEffect(op, a.effect);
    prepTask->operators.push_back(op);
    //cout << op.toString(task) << endl;
}

// Terminates to build de operator (only its preconditions are built)
void Preprocess::terminateBuildingOperator(Operator &op, DurativeAction &a, std::string name) {
    op.name = name;
    //cout << "Bulding " << op.name << endl;
    op.parameters = a.parameters;
    op.controlVars = a.controlVars;
    op.duration = a.duration;
    buildOperatorEffect(op, a.effect);
    prepTask->operators.push_back(op);
}

// Adds action effects to the operators
void Preprocess::buildOperatorEffect(Operator &op, Effect &effect) {
    switch (effect.type) {
    case ET_LITERAL:
    case ET_NEG_LITERAL:
        {
            Term value;
            value.type = TERM_CONSTANT;
            value.index = effect.type == ET_LITERAL ? task->CONSTANT_TRUE : task->CONSTANT_FALSE;
            op.addLiteralToEffects(effect.literal, value, AT_START);
        }
        break;
    case ET_AND:
        for (unsigned int i = 0; i < effect.terms.size(); i++)
            buildOperatorEffect(op, effect.terms[i]);
        break;
    case ET_ASSIGNMENT:
        {
            OpEffect e(effect.assignment);
            op.atStart.numericEff.push_back(e);
        }
        break;
    case ET_WHEN:
        buildOperatorEffect(op, effect.goal, effect.terms[0]);
        break;
    default:;
    }
}

// Adds action effects to the operators
void Preprocess::buildOperatorEffect(Operator &op, DurativeEffect &effect) {
    switch (effect.type) {
    case DET_AND:
        for (unsigned int i = 0; i < effect.terms.size(); i++)
            buildOperatorEffect(op, effect.terms[i]);
        break;
    case DET_TIMED_EFFECT:
        buildOperatorEffect(op, effect.timedEffect, AT_END);
        break;
    case DET_ASSIGNMENT:
        buildOperatorEffect(op, effect.assignment, AT_END);
        break;
    case DET_WHEN:
        buildOperatorEffect(op, effect.condition, effect.timedEffect);
        break;
    default:;
    }
}

// Adds action effects to the operators
void Preprocess::buildOperatorEffect(Operator &op, TimedEffect &effect, TimeSpecifier time) {
    TimeSpecifier t = effect.time != NONE ? effect.time : time;
    switch (effect.type) {
    case TE_AND:
        for (unsigned int i = 0; i < effect.terms.size(); i++)
            buildOperatorEffect(op, effect.terms[i], t);
        break;
    case TE_LITERAL:
    case TE_NEG_LITERAL:
        {
            Term value;
            value.type = TERM_CONSTANT;
            value.index = effect.type == TE_LITERAL ? task->CONSTANT_TRUE : task->CONSTANT_FALSE;
            op.addLiteralToEffects(effect.literal, value, t);
        }
        break;
    case TE_ASSIGNMENT:
        buildOperatorEffect(op, effect.assignment, t);
        break;
    default:;
    }
}

// Adds action effects to the operators
void Preprocess::buildOperatorEffect(Operator &op, AssignmentContinuousEffect &effect, TimeSpecifier time) {
    OpEffect e(effect);    
    op.addNumericEffect(e, time);
}    

// Adds action effects to the operators
void Preprocess::buildOperatorEffect(Operator &op, FluentAssignment &effect, TimeSpecifier time) {
    if (effect.type == AS_ASSIGN && effect.exp.type == EE_TERM) {
        op.addLiteralToEffects(effect.fluent, effect.exp.term, time);
    } else {
        OpEffect e(effect);
        op.addNumericEffect(e, time);
    }
}

// Adds a conditional effect to the operator
void Preprocess::buildOperatorEffect(Operator& op, GoalDescription& condition, Effect& effect)
{
    op.condEffects.emplace_back();
    OpConditionalEffect* cond = &op.condEffects.back();
    buildConditionalEffectCondition(&op, cond, &condition);
    buildConditionalEffectEffect(&op, cond, &effect);
}

// Adds a conditional effect to the operator
void Preprocess::buildOperatorEffect(Operator& op, DurativeCondition& condition, TimedEffect& effect)
{
    op.condEffects.emplace_back();
    OpConditionalEffect* cond = &op.condEffects.back();
    buildConditionalEffectCondition(&op, cond, &condition);
    buildConditionalEffectEffect(&op, cond, &effect, AT_END);
}

void Preprocess::buildConditionalEffectCondition(Operator* op, OpConditionalEffect* cond, GoalDescription* condition)
{
    switch (condition->type) {
    case GD_LITERAL:
    case GD_NEG_LITERAL:
    {
        Term value;
        value.type = TERM_CONSTANT;
        value.index = condition->type == GD_LITERAL ? task->CONSTANT_TRUE : task->CONSTANT_FALSE;
        OpFluent f;
        f.variable = condition->literal;
        f.value = value;
        if (condition->time == AT_END || condition->time == OVER_ALL) cond->atEnd.prec.push_back(f);
        else cond->atStart.prec.push_back(f);
    }
    break;
    case GD_AND:
    {
        for (int i = 0; i < (int)condition->terms.size(); i++) {
            buildConditionalEffectCondition(op, cond, &condition->terms[i]);
        }
    }
    break;
    case GD_F_CMP:
    {
        if (condition->comparator == CMP_EQ &&
            ((condition->exp[0].type == NET_FUNCTION && condition->exp[1].type == NET_TERM) ||
                (condition->exp[0].type == NET_TERM && condition->exp[1].type == NET_FUNCTION))) { // Fluent
            if (condition->exp[0].type == NET_FUNCTION) {
                OpFluent f;
                f.variable = condition->exp[0].function;
                f.value = condition->exp[1].term;
                cond->atStart.prec.push_back(f);
            }
            else {
                OpFluent f;
                f.variable = condition->exp[1].function;
                f.value = condition->exp[0].term;
                cond->atStart.prec.push_back(f);
            }
        }
        else {    // Comparator
            OpNumericPrec np;
            np.comparator = condition->comparator;
            for (unsigned int i = 0; i < condition->exp.size(); i++) {
                OpEffectExpression op(condition->exp[i]);
                np.operands.push_back(op);
            }
            cond->atStart.numericPrec.push_back(np);
        }
    }
    break;
    default:;
    }
}

void Preprocess::buildConditionalEffectCondition(Operator* op, OpConditionalEffect* cond, DurativeCondition* condition)
{
    switch (condition->type) {
    case CT_AND:
    {
        for (int i = 0; i < (int)condition->conditions.size(); i++) {
            buildConditionalEffectCondition(op, cond, &condition->conditions[i]);
        }
    }
    break;
    case CT_GOAL:
        buildConditionalEffectCondition(op, cond, &condition->goal);
        break;
    default:;
    }
}

void Preprocess::buildConditionalEffectEffect(Operator* op, OpConditionalEffect* cond, Effect* effect)
{
    switch (effect->type) {
    case ET_LITERAL:
    case ET_NEG_LITERAL:
    {
        Term value;
        value.type = TERM_CONSTANT;
        value.index = effect->type == ET_LITERAL ? task->CONSTANT_TRUE : task->CONSTANT_FALSE;
        OpFluent f;
        f.variable = effect->literal;
        f.value = value;
        cond->atStart.eff.push_back(f);
    }
    break;
    case ET_AND:
        for (unsigned int i = 0; i < effect->terms.size(); i++)
            buildConditionalEffectEffect(op, cond, &effect->terms[i]);
        break;
    case ET_ASSIGNMENT:
    {
        OpEffect e(effect->assignment);
        cond->atStart.numericEff.push_back(e);
    }
    break;
    default:;
    }
}

void Preprocess::buildConditionalEffectEffect(Operator* op, OpConditionalEffect* cond, TimedEffect* effect, TimeSpecifier time)
{
    TimeSpecifier t = effect->time != NONE ? effect->time : time;
    switch (effect->type) {
    case TE_AND:
        for (unsigned int i = 0; i < effect->terms.size(); i++)
            buildConditionalEffectEffect(op, cond, &effect->terms[i], t);
        break;
    case TE_LITERAL:
    case TE_NEG_LITERAL:
        {
            Term value;
            value.type = TERM_CONSTANT;
            value.index = effect->type == TE_LITERAL ? task->CONSTANT_TRUE : task->CONSTANT_FALSE;
            OpFluent f;
            f.variable = effect->literal;
            f.value = value;
            if (t == AT_END || t == NONE) cond->atEnd.eff.push_back(f);
            else cond->atStart.eff.push_back(f);
        }
        break;
    case TE_ASSIGNMENT:
        if (effect->assignment.type == AS_ASSIGN && effect->assignment.exp.type == EE_TERM) {
            OpFluent f;
            f.variable = effect->assignment.fluent;
            f.value = effect->assignment.exp.term;
            if (t == AT_END || t == NONE) cond->atEnd.eff.push_back(f);
            else cond->atStart.eff.push_back(f);
        }
        else {
            OpEffect e(effect->assignment);
            if (t == AT_END || t == NONE) cond->atEnd.numericEff.push_back(e);
            else cond->atStart.numericEff.push_back(e);
        }
        break;
    default:;
    }
}

std::string Preprocess::getOpName(std::string& name, int index, int size)
{
    if (size < 2) return name;
    return name + ":" + std::to_string(index + 1);
}

void Preprocess::conjuctionOptimization(Effect* eff)
{
    for (Effect& child : eff->terms) {
        conjuctionOptimization(&child);
    }
    if (eff->type == ET_AND) {
        std::vector<Effect> effectsToMove;   
        int i = 0;
        while (i < (int)eff->terms.size()) {
            Effect& child = eff->terms[i];
            if (child.type == eff->type) {
                effectsToMove.push_back(child);
                eff->terms.erase(eff->terms.begin() + i);
            }
            else i++;
        }
        for (Effect& child : effectsToMove)
            for (Effect& conjTerm: child.terms)
                eff->terms.push_back(conjTerm);
    }
}

void Preprocess::conjuctionOptimization(DurativeEffect* eff)
{
    for (DurativeEffect& child : eff->terms) {
        conjuctionOptimization(&child);
    }
    if (eff->type == DET_AND) {
        std::vector<DurativeEffect> effectsToMove;    
        int i = 0;
        while (i < (int)eff->terms.size()) {
            DurativeEffect& child = eff->terms[i];
            if (child.type == eff->type) {
                effectsToMove.push_back(child);
                eff->terms.erase(eff->terms.begin() + i);
            }
            else i++;
        }
        for (DurativeEffect& child : effectsToMove)
            eff->terms.push_back(child);
    }
}
