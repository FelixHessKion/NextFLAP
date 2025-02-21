#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Priority queue implementation.                       */
/********************************************************/

#include <vector>
#include <memory>

#define DEFAULT_PQ_CAPACITY	 250

class PriorityQueueItem {
public:
	virtual int compare(std::shared_ptr<PriorityQueueItem> other) = 0;
	virtual ~PriorityQueueItem() { }
};

class PriorityQueue {
private:
	std::vector<std::shared_ptr<PriorityQueueItem>> pq;		// Priority queue

	void heapify(unsigned int gap) {
		std::shared_ptr<PriorityQueueItem> aux = pq[gap];
		unsigned int child = gap << 1;
		while (child < pq.size()) {
			if (child != pq.size() - 1 && pq[child + 1]->compare(pq[child]) < 0)
				child++;
			if (pq[child]->compare(aux) < 0) {
				pq[gap] = pq[child];
				gap = child;
				child = gap << 1;
			}
			else break;
		}
		pq[gap] = aux;
	}

public:
	
	PriorityQueue() : PriorityQueue(DEFAULT_PQ_CAPACITY) { }

	PriorityQueue(unsigned int initialCapacity) {
		pq.reserve(initialCapacity);
		pq.push_back(nullptr);	// Position 0 empty
	}

	void add(std::shared_ptr<PriorityQueueItem> p) {
		unsigned int gap = (unsigned int)pq.size();
		pq.push_back(nullptr);
		while (gap > 1 && p->compare(pq[gap >> 1]) < 0) {
			pq[gap] = pq[gap >> 1];
			gap = gap >> 1;
		}
		pq[gap] = p;
	}
	
	inline int size() {
		return (int)pq.size() - 1;
	}
	
	inline std::shared_ptr<PriorityQueueItem> peek() {
		return pq[1];
	}
	
	std::shared_ptr<PriorityQueueItem> poll() {
		std::shared_ptr<PriorityQueueItem> next = pq[1];
		if (pq.size() > 2) {
			pq[1] = pq.back();
			pq.pop_back();
			heapify(1);
		}
		else if (pq.size() > 1) pq.pop_back();
		return next;
	}

	inline void clear() {
		pq.clear();
		pq.push_back(nullptr);	// Position 0 empty
	}

	inline std::shared_ptr<PriorityQueueItem> at(unsigned int i) {
		return pq[i];
	}

	void fix() {
		for (unsigned int i = (unsigned int)pq.size() / 2; i > 0; i--)
			heapify(i);
	}

	void fastRemove(unsigned int i) {
		pq[i] = pq.back();
		pq.pop_back();
	}
};

#endif
