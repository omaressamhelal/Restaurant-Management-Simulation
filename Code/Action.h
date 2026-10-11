#pragma once

class Restaurant;

class Action
{
protected:
    int actionTime;
    int orderID;

public:
    Action(int actionTime, int orderID)
        : actionTime(actionTime), orderID(orderID) {}

    int getActionTime() const { return actionTime; }
    int getOrderID()    const { return orderID;    }

    virtual void Act(Restaurant* pRest) = 0;
    virtual ~Action() {}
};
