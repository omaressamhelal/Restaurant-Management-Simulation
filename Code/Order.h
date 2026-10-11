#pragma once

class Chef;

enum OrderType { ODG, ODN, OT, OVC, OVN, OVG, OCOMBO };

class Order
{
private:
    int       id;
    OrderType type;
    int       size;
    double    price;
    double    distance;

    // OD-only fields
    int       numSeats;
    int       orderDuration;
    bool      canShare;

    // NEW: COMBO-only fields
    int       reqChefs;
    int       reqScooters;

    int       arrivalTime;
    int       assignTime;
    int       readyTime;
    int       serviceStart;
    int       finishTime;

    // Multi-Chef Support
    Chef* assignedChefs[32];
    int       numAssignedChefs;
    int       readyAtTime;

public:
    Order(int id, OrderType type, int size, double price, double distance,
        int arrivalTime, int numSeats = 0, int orderDuration = 0, bool canShare = false,
        int reqChefs = 0, int reqScooters = 0)
        : id(id), type(type), size(size), price(price), distance(distance),
        numSeats(numSeats), orderDuration(orderDuration), canShare(canShare),
        reqChefs(reqChefs), reqScooters(reqScooters),
        arrivalTime(arrivalTime), assignTime(-1), readyTime(-1),
        serviceStart(-1), finishTime(-1), numAssignedChefs(0), readyAtTime(-1)
    {
        for (int i = 0; i < 4; i++) assignedChefs[i] = nullptr;
    }

    int        getID()            const { return id; }
    OrderType  getType()          const { return type; }
    int        getSize()          const { return size; }
    double     getPrice()         const { return price; }
    double     getDistance()      const { return distance; }
    int        getNumSeats()      const { return numSeats; }
    int        getOrderDuration() const { return orderDuration; }
    bool       getCanShare()      const { return canShare; }
    int        getReqChefs()      const { return reqChefs; }
    int        getReqScooters()   const { return reqScooters; }
    int        getArrivalTime()   const { return arrivalTime; }
    int        getAssignTime()    const { return assignTime; }
    int        getReadyTime()     const { return readyTime; }
    int        getServiceStart()  const { return serviceStart; }
    int        getFinishTime()    const { return finishTime; }
    int        getReadyAtTime()   const { return readyAtTime; }

    // Legacy support for single-chef code
    Chef* getAssignedChef()  const { return assignedChefs[0]; }

    // Multi-chef getters
    int        getNumAssignedChefs() const { return numAssignedChefs; }
    Chef* getAssignedChef(int idx) const { return assignedChefs[idx]; }

    void setPrice(double p) { price = p; }
    void setType(OrderType t) { type = t; }
    void setAssignTime(int t) { assignTime = t; }
    void setReadyTime(int t) { readyTime = t; }
    void setServiceStart(int t) { serviceStart = t; }
    void setFinishTime(int t) { finishTime = t; }
    void setReadyAtTime(int t) { readyAtTime = t; }

    // Legacy setter
    void setAssignedChef(Chef* c) {
        numAssignedChefs = 0;
        assignedChefs[0] = c;
        if (c) numAssignedChefs = 1;
    }

    // Multi-chef setters
    void addAssignedChef(Chef* c) {
        if (numAssignedChefs < 4) assignedChefs[numAssignedChefs++] = c;
    }
    void clearAssignedChefs() {
        for (int i = 0; i < 4; i++) assignedChefs[i] = nullptr;
        numAssignedChefs = 0;
    }

    bool isDineIn()    const { return type == ODG || type == ODN; }
    bool isDelivery()  const { return type == OVC || type == OVN || type == OVG || type == OCOMBO; }
    bool isTakeaway()  const { return type == OT; }
    bool needsCSChef() const { return type == ODG || type == OVG || type == OCOMBO; }
    bool isCooking()   const { return numAssignedChefs > 0; }

    const char* getTypeLabel() const
    {
        switch (type)
        {
        case ODG: return "ODG"; case ODN: return "ODN"; case OT:  return "OT";
        case OVC: return "OVC"; case OVN: return "OVN"; case OVG: return "OVG";
        case OCOMBO: return "OCOMBO";
        default:  return "???";
        }
    }
};