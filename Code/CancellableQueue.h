#pragma once
#include "LinkedQueue.h"
#include "Order.h"

class CancellableQueue : public LinkedQueue<Order*>
{
public:
    CancellableQueue()  {}
    ~CancellableQueue() {}

    Order* cancel(int targetID)
    {
        if (isEmpty()) return nullptr;

        Node<Order*>* prev = nullptr;
        Node<Order*>* curr = frontPtr;

        while (curr != nullptr)
        {
            Order* candidate = curr->getItem();
            if (candidate->getID() == targetID)
            {
                if (prev == nullptr)
                {

                    frontPtr = curr->getNext();
                    if (frontPtr == nullptr) backPtr = nullptr;
                }
                else if (curr->getNext() == nullptr)
                {

                    prev->setNext(nullptr);
                    backPtr = prev;
                }
                else
                {

                    prev->setNext(curr->getNext());
                }

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
