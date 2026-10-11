#pragma once
#include "Order.h"
#include "Table.h"
#include "Scooter.h"
#include "LinkedQueue.h"

struct InServiceRecord
{
    Order* order;
    Table* assignedTable;
    Scooter* assignedScooter;
    int      finishTime;

    bool     needsRescue = false;
    int      breakdownTime = -1;
    Scooter* rescueScooter = nullptr;
    int      rescueMeetingTime = -1;

    Scooter* comboScooters[32];
    int      numComboScooters = 0;

    InServiceRecord(Order* o, Table* t, Scooter* s, int tf)
        : order(o), assignedTable(t), assignedScooter(s), finishTime(tf), numComboScooters(0) {
        for (int i = 0; i < 4; i++) comboScooters[i] = nullptr;
    }

    InServiceRecord()
        : order(nullptr), assignedTable(nullptr),
        assignedScooter(nullptr), finishTime(-1), numComboScooters(0) {
        for (int i = 0; i < 4; i++) comboScooters[i] = nullptr;
    }
};

typedef LinkedQueue<InServiceRecord> InServiceQueue;