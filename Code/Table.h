#pragma once

class Table
{
private:
    int id;
    int totalCapacity;
    int freeSeats;

public:
    Table(int id, int totalCapacity)
        : id(id), totalCapacity(totalCapacity), freeSeats(totalCapacity) {}

    int  getID()            const { return id;            }
    int  getTotalCapacity() const { return totalCapacity; }
    int  getFreeSeats()     const { return freeSeats;     }

    bool canFit(int orderSize) const { return freeSeats >= orderSize; }

    bool occupySeats(int count)
    {
        if (count > freeSeats) return false;
        freeSeats -= count;
        return true;
    }

    bool releaseSeats(int count)
    {
        if (freeSeats + count > totalCapacity) return false;
        freeSeats += count;
        return true;
    }

    bool isEmpty() const { return freeSeats == totalCapacity; }
};
