#pragma once

class Scooter
{
private:
    int    id;
    double speed;
    double totalDistance;
    int    ordersCompleted;
    int    returnArrivalTime;
    int    mainFinishTime;

public:
    Scooter(int id, double speed)
        : id(id), speed(speed), totalDistance(0.0), ordersCompleted(0),
          returnArrivalTime(-1), mainFinishTime(-1) {}

    int    getID()                const { return id;                }
    double getSpeed()             const { return speed;             }
    double getTotalDistance()     const { return totalDistance;     }
    int    getOrdersCompleted()   const { return ordersCompleted;   }
    int    getReturnArrivalTime() const { return returnArrivalTime; }
    int    getMainFinishTime()    const { return mainFinishTime;    }

    void addDistance(double d)       { totalDistance += d;  }
    void incrementOrdersCompleted()  { ordersCompleted++;   }
    void resetOrdersCompleted()      { ordersCompleted = 0; }
    void setReturnArrivalTime(int t) { returnArrivalTime = t; }
    void setMainFinishTime(int t)    { mainFinishTime    = t; }

    int availPriority() const { return -(int)totalDistance;  }

    int backPriority()  const { return -returnArrivalTime;   }

    int maintPriority() const { return -mainFinishTime;      }

    int computeTripTicks(double distance) const
    {
        if (speed <= 0.0) return 0;
        int ticks = (int)(distance / speed);
        if (distance > ticks * speed) ticks++;
        return ticks;
    }

    bool isBrokenDown = false;
    void setBrokenDown(bool state) { isBrokenDown = state; }
    bool getBrokenDown() const { return isBrokenDown; }
};
