#pragma once

enum ChefType { CN, CS };

class Chef
{
private:
    int      id;
    ChefType type;
    double   speed;
    bool     busy;

public:
    Chef(int id, ChefType type, double speed)
        : id(id), type(type), speed(speed), busy(false) {}

    int      getID()    const { return id;    }
    ChefType getType()  const { return type;  }
    double   getSpeed() const { return speed; }
    bool     isBusy()   const { return busy;  }
    bool     isFree()   const { return !busy; }

    void startCooking()  { busy = true;  }
    void finishCooking() { busy = false; }

    const char* getTypeLabel() const { return (type == CN) ? "CN" : "CS"; }

    int computeCookTime(int orderSize) const
    {
        if (speed <= 0.0) return 0;
        int ticks = (int)(orderSize / speed);
        if (orderSize > ticks * speed) ticks++;
        return ticks;
    }
};
