#include "planComponents.h"

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Calculation of the list of steps of a plan (each     */
/* step is called plan component.)                      */
/********************************************************/

void PlanComponents::calculate(std::shared_ptr<Plan> base)
{
	if (base == nullptr) {
		basePlanComponents.clear();
		numSteps = 0;
	}
	else {
		calculate(base->parentPlan.lock());
		base->startPoint.copyInitialTime();
		base->endPoint.copyInitialTime();
		basePlanComponents.push_back(base);
		numSteps++;
		if (base->planUpdates != nullptr) {
			for (TPlanUpdate& u : *(base->planUpdates)) {
				TStep stepIndex = timePointToStep(u.timePoint);
				std::shared_ptr<Plan> step = basePlanComponents[stepIndex];
				if ((u.timePoint & 1) == 0) step->startPoint.updatedTime = u.newTime;
				else step->endPoint.updatedTime = u.newTime;
			}
		}
	}
}
