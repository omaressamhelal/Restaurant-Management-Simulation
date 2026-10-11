#pragma once
#include "priQueue.h"
#include "Order.h"

class CancellablePriQueue : public priQueue<Order*>
{
public:
    CancellablePriQueue()  {}
    ~CancellablePriQueue() {}

    Order* cancel(int targetID)
    {
        if (isEmpty()) return nullptr;

        priNode<Order*>* prev = nullptr;
        priNode<Order*>* curr = head;

        while (curr != nullptr)
        {
            int dummy = 0;
            Order* candidate = curr->getItem(dummy);

            if (candidate->getID() == targetID)
            {
                if (prev == nullptr)
                    head = curr->getNext();
                else if (curr->getNext() == nullptr)
                    prev->setNext(nullptr);
                else
                    prev->setNext(curr->getNext());

                curr->setNext(nullptr);
                delete curr;
                return candidate;
            }

            prev = curr;
            curr = curr->getNext();
        }

        return nullptr;
    }
};
