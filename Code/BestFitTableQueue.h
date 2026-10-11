#pragma once
#include "priQueue.h"
#include "Table.h"

class BestFitTableQueue : public priQueue<Table*>
{
public:
    BestFitTableQueue() {}
    ~BestFitTableQueue() {}

    // Add this wrapper so Restaurant.cpp can still pass just 1 argument!
    void enqueue(Table* t)
    {
        // Project doc: tables prioritized by least no. of avail seats (Best fit)
        // We use negative free seats so smaller numbers (fewer seats) have higher priority
        int priority = -(t->getFreeSeats());
        priQueue<Table*>::enqueue(t, priority);
    }

    Table* extractBestFit(int orderSize)
    {
        if (isEmpty()) return nullptr;

        priNode<Table*>* prev = nullptr;
        priNode<Table*>* curr = head;
        priNode<Table*>* bestPrev = nullptr;
        priNode<Table*>* bestNode = nullptr;
        int              bestFit = -1;

        while (curr != nullptr)
        {
            int dummyPri = 0;
            Table* t = curr->getItem(dummyPri);
            int    seats = t->getFreeSeats();

            if (seats >= orderSize && (bestNode == nullptr || seats < bestFit))
            {
                bestPrev = prev;
                bestNode = curr;
                bestFit = seats;
            }

            prev = curr;
            curr = curr->getNext();
        }

        if (bestNode == nullptr) return nullptr;

        if (bestPrev == nullptr)
        {
            head = bestNode->getNext();
        }
        else
        {
            bestPrev->setNext(bestNode->getNext());
        }

        int finalDummy = 0;
        Table* result = bestNode->getItem(finalDummy);
        bestNode->setNext(nullptr);
        delete bestNode;
        return result;
    }
};