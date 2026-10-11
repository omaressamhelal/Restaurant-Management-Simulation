#pragma once
#include "Action.h"
#include "Order.h"

class RequestAction : public Action
{
private:
    OrderType orderType;
    int       orderSize;
    double    orderPrice;
    double    orderDistance;
    int       numSeats;
    int       orderDuration;
    bool      canShare;
    int       reqChefs;
    int       reqScooters;

public:
    RequestAction(int actionTime, int orderID, OrderType orderType,
        int orderSize, double orderPrice,
        double orderDistance = 0.0, int numSeats = 0,
        int orderDuration = 0, bool canShare = false,
        int reqChefs = 0, int reqScooters = 0)
        : Action(actionTime, orderID),
        orderType(orderType), orderSize(orderSize), orderPrice(orderPrice),
        orderDistance(orderDistance), numSeats(numSeats),
        orderDuration(orderDuration), canShare(canShare),
        reqChefs(reqChefs), reqScooters(reqScooters)
    {
    }

    OrderType getOrderType()     const { return orderType; }
    int       getOrderSize()     const { return orderSize; }
    double    getOrderPrice()    const { return orderPrice; }
    double    getOrderDistance() const { return orderDistance; }
    int       getNumSeats()      const { return numSeats; }
    int       getOrderDuration() const { return orderDuration; }
    bool      getCanShare()      const { return canShare; }
    int       getReqChefs()      const { return reqChefs; }
    int       getReqScooters()   const { return reqScooters; }

    virtual void Act(Restaurant* pRest) override;
    virtual ~RequestAction() {}
};