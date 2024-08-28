#ifndef SELECTOR_H
#define SELECTOR_H

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Plan selector   										*/
/********************************************************/

#include <vector>
#include "plan.h"

class SearchQueue {
private:
	const static unsigned int INITIAL_PQ_CAPACITY = 8192;
	std::vector<std::shared_ptr<Plan>> pq;

	void heapify(unsigned int gap);

public:
	SearchQueue();
	void add(std::shared_ptr<Plan> p);
	std::shared_ptr<Plan> poll();
	inline std::shared_ptr<Plan> peek() { return pq[1]; }
	inline int size() { return (int)pq.size() - 1; }
	inline std::shared_ptr<Plan> getPlanAt(unsigned int i) { return pq[i]; }
	void clear();
};

#endif // !SELECTOR_H
