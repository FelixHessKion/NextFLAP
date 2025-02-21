#ifndef PRINT_PLAN_H
#define PRINT_PLAN_H

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Class to plan printing on screen.                    */
/********************************************************/

#include "plan.h"
#include "z3Checker.h"

class PrintPlan {
public:
	static std::string actionName(std::shared_ptr<SASAction> a);
	static void print(std::shared_ptr<Plan> p, TControVarValues* cvarValues = nullptr);
	static float getMakespan(std::shared_ptr<Plan> p);
	static void rawPrint(std::shared_ptr<Plan> p, SASTask* task);
};

#endif // !PRINT_PLAN_H
