#include "linearizer.h"

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Calculates a linear order among the steps of the     */
/* plan.                                                */
/********************************************************/

void Linearizer::linearize(PlanComponents& planComponents)
{
	unsigned int numActions = planComponents.size();
	linearOrder.clear();
	linearOrder.reserve(((size_t)numActions) << 1);
	PriorityQueue timePoints((unsigned int)linearOrder.capacity());
	TTimePoint p = 0;
	for (unsigned int i = 0; i < numActions; i++) {
		std::shared_ptr<Plan> comp = planComponents.get(i);
		timePoints.add(std::make_shared<ScheduledTimepoint>(p++, comp->startPoint.updatedTime));
		timePoints.add(std::make_shared<ScheduledTimepoint>(p++, comp->endPoint.updatedTime));
	}
	while (timePoints.size() > 0) {
    std::shared_ptr<ScheduledTimepoint> tp = std::dynamic_pointer_cast<ScheduledTimepoint>(timePoints.poll());
		linearOrder.push_back(tp->point);
		//cout << tp->point << " [" << tp->scheduledTime << "]" << endl;
	}
}
