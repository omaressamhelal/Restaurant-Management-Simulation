#pragma once
#include "Action.h"

class CancelAction : public Action
{
public:
    CancelAction(int actionTime, int orderID)
        : Action(actionTime, orderID) {}

    virtual void Act(Restaurant* pRest) override;
    virtual ~CancelAction() {}
};
