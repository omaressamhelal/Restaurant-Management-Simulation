#include <iostream>
#include <fstream>
#include <cstring>
#include <random>
#include <ctime>
#include <string>
using namespace std;

#include "Restaurant.h"
#include "UI.h"

static const double W_PRICE = 1.0;
static const double W_SIZE = 10.0;
static const double W_DIST = 0.02;

static OrderType parseOrderType(const char* token)
{
    if (strcmp(token, "ODG") == 0) return ODG;
    else if (strcmp(token, "ODN") == 0) return ODN;
    else if (strcmp(token, "OT") == 0) return OT;
    else if (strcmp(token, "OVC") == 0) return OVC;
    else if (strcmp(token, "OVN") == 0) return OVN;
    else if (strcmp(token, "OVG") == 0) return OVG;
    else if (strcmp(token, "OCOMBO") == 0) return OCOMBO;
    cout << "[WARNING] Unknown order type: '" << token << "'\n";
    return ODN;
}

Restaurant::Restaurant()
    : allChefs(nullptr), numCN(0), numCS(0), totalChefs(0),
    cnSpeed(0.0), csSpeed(0.0),
    allScooters(nullptr), numScooters(0),
    scooterSpeed(0.0), scooterMainOrds(0), scooterMainDur(0),
    numTables(0), overwaitThreshold(0), currentTimestep(0),
    outputMode(SILENT),
    totalOrdersServed(0), totalOrdersCancelled(0), totalOrdersRejected(0), activeBreakdowns(0)
{
    outputFilename[0] = '\0';
}

Restaurant::~Restaurant()
{
    {
        InServiceRecord rec;
        while (!inServiceOrders.isEmpty())
        {
            inServiceOrders.dequeue(rec);
            delete rec.order;
        }
    }

    Order* o = nullptr;
    int  pri = 0;

    while (!pendingODG.isEmpty()) { pendingODG.dequeue(o); delete o; }
    while (!pendingODN.isEmpty()) { pendingODN.dequeue(o); delete o; }
    while (!pendingOT.isEmpty()) { pendingOT.dequeue(o);  delete o; }
    while (!pendingOVC.isEmpty()) { pendingOVC.dequeue(o); delete o; }
    while (!pendingOVN.isEmpty()) { pendingOVN.dequeue(o); delete o; }
    while (!pendingOVG.isEmpty()) { pendingOVG.dequeue(o, pri); delete o; }
    while (!pendingOCOMBO.isEmpty()) { pendingOCOMBO.dequeue(o); delete o; }

    while (!cookingOD.isEmpty()) { cookingOD.dequeue(o, pri); delete o; }
    while (!cookingOT.isEmpty()) { cookingOT.dequeue(o, pri); delete o; }
    while (!cookingOV.isEmpty()) { cookingOV.dequeue(o, pri); delete o; }
    while (!cookingOCOMBO.isEmpty()) { cookingOCOMBO.dequeue(o, pri); delete o; }

    while (!readyOD.isEmpty()) { readyOD.dequeue(o);        delete o; }
    while (!readyOT.isEmpty()) { readyOT.dequeue(o);        delete o; }
    while (!readyOVC.isEmpty()) { readyOVC.dequeue(o);       delete o; }
    while (!readyOVN.isEmpty()) { readyOVN.dequeue(o);       delete o; }
    while (!readyOVG.isEmpty()) { readyOVG.dequeue(o, pri);  delete o; }
    while (!readyOCOMBO.isEmpty()) { readyOCOMBO.dequeue(o); delete o; }

    while (!finishedOrders.isEmpty()) { finishedOrders.pop(o);      delete o; }
    while (!cancelledOrders.isEmpty()) { cancelledOrders.dequeue(o); delete o; }

    {
        Table* t = nullptr;
        while ((t = availableTables.extractBestFit(0)) != nullptr) delete t;
    }

    for (int i = 0; i < numScooters; i++) delete allScooters[i];
    delete[] allScooters;

    for (int i = 0; i < totalChefs; i++) delete allChefs[i];
    delete[] allChefs;

    {
        Action* a = nullptr;
        while (!actionsQueue.isEmpty()) { actionsQueue.dequeue(a, pri); delete a; }
    }
}

void Restaurant::addAction(Action* pAction)
{
    actionsQueue.enqueue(pAction, -(pAction->getActionTime()));
}

void Restaurant::loadFile(const char* filename)
{
    ifstream fin(filename);
    if (!fin.is_open()) return;

    fin >> numCN >> numCS;
    totalChefs = numCN + numCS;
    fin >> cnSpeed >> csSpeed;

    allChefs = new Chef * [totalChefs];
    int chefID = 1;
    for (int i = 0; i < numCN; i++) {
        allChefs[i] = new Chef(chefID++, CN, cnSpeed);
        availableCN.enqueue(allChefs[i]);
    }
    for (int i = numCN; i < totalChefs; i++) {
        allChefs[i] = new Chef(chefID++, CS, csSpeed);
        availableCS.enqueue(allChefs[i]);
    }

    fin >> numScooters >> scooterSpeed >> scooterMainOrds >> scooterMainDur;
    allScooters = new Scooter * [numScooters];
    for (int i = 0; i < numScooters; i++) {
        allScooters[i] = new Scooter(i + 1, scooterSpeed);
        availableScooters.enqueue(allScooters[i], allScooters[i]->availPriority());
    }

    fin >> numTables;
    int created = 0, tableID = 1;
    while (created < numTables) {
        int cnt, cap; fin >> cnt >> cap;
        for (int i = 0; i < cnt; i++) availableTables.enqueue(new Table(tableID++, cap));
        created += cnt;
    }

    fin >> overwaitThreshold;
    int M = 0; fin >> M;

    for (int i = 0; i < M; i++) {
        char actionCode; fin >> actionCode;
        if (actionCode == 'Q') {
            char typStr[8]; int TQ, ID, SIZE; double price;
            fin >> typStr >> TQ >> ID >> SIZE >> price;
            OrderType otype = parseOrderType(typStr);
            double orderDistance = 0.0;
            int numSeats = 0, duration = 0, reqChefs = 0, reqScooters = 0;
            bool canShare = false;

            if (otype == ODG || otype == ODN) {
                char shareChar; fin >> numSeats >> duration >> shareChar;
                canShare = (shareChar == 'Y' || shareChar == 'y');
            }
            else if (otype == OCOMBO) {
                fin >> orderDistance >> reqChefs >> reqScooters;
            }
            else if (otype == OVC || otype == OVN || otype == OVG) {
                fin >> orderDistance;
            }
            addAction(new RequestAction(TQ, ID, otype, SIZE, price, orderDistance, numSeats, duration, canShare, reqChefs, reqScooters));
        }
        else if (actionCode == 'X') {
            int Tcancel, ID; fin >> Tcancel >> ID;
            addAction(new CancelAction(Tcancel, ID));
        }
    }
    fin.close();
}

void Restaurant::addNewOrder(Order* pOrder)
{
    if (!pOrder) return;
    switch (pOrder->getType())
    {
    case ODG: pendingODG.enqueue(pOrder); break;
    case ODN: pendingODN.enqueue(pOrder); break;
    case OT:  pendingOT.enqueue(pOrder);  break;
    case OVC: pendingOVC.enqueue(pOrder); break;
    case OVN: pendingOVN.enqueue(pOrder); break;
    case OCOMBO: pendingOCOMBO.enqueue(pOrder); break;
    case OVG:
    {
        int priority = static_cast<int>(W_PRICE * pOrder->getPrice() + W_SIZE * pOrder->getSize() + W_DIST * pOrder->getDistance());
        pendingOVG.enqueue(pOrder, priority);
        break;
    }
    }
}

void Restaurant::cancelOrder(int orderID)
{
    Order* found = pendingOVC.cancel(orderID);
    if (!found) {
        found = cookingOV.cancel(orderID);
        if (found) {
            Chef* c = found->getAssignedChef();
            if (c) {
                if (c->getType() == CS) availableCS.enqueue(c);
                else                    availableCN.enqueue(c);
                found->setAssignedChef(nullptr);
                found->setReadyAtTime(-1);
            }
        }
    }
    if (!found) found = readyOVC.cancel(orderID);
    if (found) {
        found->setFinishTime(currentTimestep);
        cancelledOrders.enqueue(found);
        totalOrdersCancelled++;
    }
}

Chef* Restaurant::getFreeChef(ChefType required)
{
    Chef* c = nullptr;
    if (required == CS) { if (!availableCS.isEmpty()) availableCS.dequeue(c); }
    else { if (!availableCN.isEmpty()) availableCN.dequeue(c); }
    return c;
}

Scooter* Restaurant::getFreeScooter()
{
    if (availableScooters.isEmpty()) return nullptr;
    Scooter* sc = nullptr; int pri = 0;
    availableScooters.dequeue(sc, pri);
    return sc;
}

static void assignOneOrder(Order* pOrder, Chef* chef, int t, priQueue<Order*>& cookQ)
{
    pOrder->setAssignTime(t);
    double speed = chef->getSpeed();
    int cookTicks = (int)(pOrder->getSize() / speed);
    if (pOrder->getSize() > cookTicks * speed) cookTicks++;
    int readyAt = t + cookTicks;
    pOrder->setReadyAtTime(readyAt);
    pOrder->setAssignedChef(chef);
    cookQ.enqueue(pOrder, -readyAt);
}

static void assignOneOrder(Order* pOrder, Chef* chef, int t, CancellablePriQueue& cookQ)
{
    pOrder->setAssignTime(t);
    double speed = chef->getSpeed();
    int cookTicks = (int)(pOrder->getSize() / speed);
    if (pOrder->getSize() > cookTicks * speed) cookTicks++;
    int readyAt = t + cookTicks;
    pOrder->setReadyAtTime(readyAt);
    pOrder->setAssignedChef(chef);
    cookQ.enqueue(pOrder, -readyAt);
}

void Restaurant::assignOrdersToChefs()
{
    Order* pOrder = nullptr; Chef* chef = nullptr; int pri = 0;

    LinkedQueue<Order*> tempCombo;
    bool comboStarving = false;

    while (!pendingOCOMBO.isEmpty()) {
        pendingOCOMBO.dequeue(pOrder);
        int reqC = pOrder->getReqChefs();

        if (reqC > totalChefs) reqC = totalChefs;
        if (reqC <= 0) reqC = 1;

        LinkedQueue<Chef*> tempCS, tempCN; Chef* c = nullptr;
        int countCS = 0, countCN = 0;

        while (!availableCS.isEmpty()) { availableCS.dequeue(c); tempCS.enqueue(c); countCS++; }
        while (!availableCN.isEmpty()) { availableCN.dequeue(c); tempCN.enqueue(c); countCN++; }

        bool canFulfill = false;
        if (numCS > 0) {
            if (countCS >= 1 && (countCS + countCN) >= reqC) canFulfill = true;
        }
        else {
            if ((countCS + countCN) >= reqC) canFulfill = true;
        }

        if (canFulfill) {
            double totalSpeed = 0;

            if (numCS > 0) {
                tempCS.dequeue(c); pOrder->addAssignedChef(c); totalSpeed += c->getSpeed();
                reqC--;
            }

            for (int i = 0; i < reqC; i++) {
                if (!tempCS.isEmpty()) { tempCS.dequeue(c); }
                else { tempCN.dequeue(c); }
                pOrder->addAssignedChef(c); totalSpeed += c->getSpeed();
            }

            while (!tempCS.isEmpty()) { tempCS.dequeue(c); availableCS.enqueue(c); }
            while (!tempCN.isEmpty()) { tempCN.dequeue(c); availableCN.enqueue(c); }

            pOrder->setAssignTime(currentTimestep);
            int cookTicks = (int)(pOrder->getSize() / totalSpeed);
            if (pOrder->getSize() > cookTicks * totalSpeed) cookTicks++;
            if (cookTicks <= 0) cookTicks = 1;
            int readyAt = currentTimestep + cookTicks;
            pOrder->setReadyAtTime(readyAt);
            cookingOCOMBO.enqueue(pOrder, -readyAt);
        }
        else {
            while (!tempCS.isEmpty()) { tempCS.dequeue(c); availableCS.enqueue(c); }
            while (!tempCN.isEmpty()) { tempCN.dequeue(c); availableCN.enqueue(c); }
            tempCombo.enqueue(pOrder);
            comboStarving = true;
            break;
        }
    }
    while (!tempCombo.isEmpty()) { tempCombo.dequeue(pOrder); pendingOCOMBO.enqueue(pOrder); }

    if (comboStarving) return;

    while (!pendingODG.isEmpty()) { chef = getFreeChef(CS); if (!chef) break; pendingODG.dequeue(pOrder); assignOneOrder(pOrder, chef, currentTimestep, cookingOD); }
    while (!pendingODN.isEmpty()) { chef = getFreeChef(CN); if (!chef) chef = getFreeChef(CS); if (!chef) break; pendingODN.dequeue(pOrder); assignOneOrder(pOrder, chef, currentTimestep, cookingOD); }
    while (!pendingOT.isEmpty()) { chef = getFreeChef(CN); if (!chef) break; pendingOT.dequeue(pOrder); assignOneOrder(pOrder, chef, currentTimestep, cookingOT); }
    while (!pendingOVG.isEmpty()) { chef = getFreeChef(CS); if (!chef) break; pendingOVG.dequeue(pOrder, pri); assignOneOrder(pOrder, chef, currentTimestep, cookingOV); }
    while (!pendingOVC.isEmpty()) { chef = getFreeChef(CN); if (!chef) chef = getFreeChef(CS); if (!chef) break; pendingOVC.dequeue(pOrder); assignOneOrder(pOrder, chef, currentTimestep, cookingOV); }
    while (!pendingOVN.isEmpty()) { chef = getFreeChef(CN); if (!chef) break; pendingOVN.dequeue(pOrder); assignOneOrder(pOrder, chef, currentTimestep, cookingOV); }
}

void Restaurant::checkCookingOrders(int t)
{
    Order* pOrder = nullptr; int pri = 0;

    while (!cookingOCOMBO.isEmpty())
    {
        cookingOCOMBO.peek(pOrder, pri);
        if (-pri > t) break;
        cookingOCOMBO.dequeue(pOrder, pri);

        for (int i = 0; i < pOrder->getNumAssignedChefs(); i++) {
            Chef* c = pOrder->getAssignedChef(i);
            if (c->getType() == CS) availableCS.enqueue(c);
            else                    availableCN.enqueue(c);
        }
        pOrder->clearAssignedChefs();
        pOrder->setReadyTime(t);
        readyOCOMBO.enqueue(pOrder);
    }

    while (!cookingOD.isEmpty()) {
        cookingOD.peek(pOrder, pri); if (-pri > t) break;
        cookingOD.dequeue(pOrder, pri); Chef* c = pOrder->getAssignedChef();
        if (c->getType() == CS) availableCS.enqueue(c); else availableCN.enqueue(c);
        pOrder->setAssignedChef(nullptr); pOrder->setReadyTime(t); readyOD.enqueue(pOrder);
    }
    while (!cookingOT.isEmpty()) {
        cookingOT.peek(pOrder, pri); if (-pri > t) break;
        cookingOT.dequeue(pOrder, pri); availableCN.enqueue(pOrder->getAssignedChef());
        pOrder->setAssignedChef(nullptr); pOrder->setReadyTime(t); readyOT.enqueue(pOrder);
    }
    while (!cookingOV.isEmpty()) {
        cookingOV.peek(pOrder, pri); if (-pri > t) break;
        cookingOV.dequeue(pOrder, pri); Chef* c = pOrder->getAssignedChef();
        if (c->getType() == CS) availableCS.enqueue(c); else availableCN.enqueue(c);
        pOrder->setAssignedChef(nullptr); pOrder->setReadyTime(t);
        switch (pOrder->getType()) {
        case OVC: readyOVC.enqueue(pOrder); break;
        case OVN: readyOVN.enqueue(pOrder); break;
        case OVG: readyOVG.enqueue(pOrder, -t); break;
        }
    }
}

void Restaurant::assignReadyOrders(int t)
{

    {
        LinkedQueue<Order*> unmatched;
        Order* incoming = nullptr;
        while (!readyOD.isEmpty())
        {
            readyOD.dequeue(incoming);
            bool shared = false;
            if (incoming->getCanShare())
            {
                InServiceQueue tempIS;
                InServiceRecord rec;
                while (!inServiceOrders.isEmpty())
                {
                    inServiceOrders.dequeue(rec);
                    if (!shared && rec.assignedTable != nullptr && rec.order->getCanShare() && rec.assignedTable->getFreeSeats() >= incoming->getNumSeats())
                    {
                        rec.assignedTable->occupySeats(incoming->getNumSeats());
                        incoming->setServiceStart(t);
                        int tf = t + incoming->getOrderDuration();
                        incoming->setFinishTime(tf);
                        tempIS.enqueue(InServiceRecord(incoming, rec.assignedTable, nullptr, tf));
                        shared = true;
                    }
                    tempIS.enqueue(rec);
                }
                while (!tempIS.isEmpty()) { tempIS.dequeue(rec); inServiceOrders.enqueue(rec); }
            }
            if (!shared) unmatched.enqueue(incoming);
        }
        while (!unmatched.isEmpty())
        {
            unmatched.dequeue(incoming);
            Table* tbl = availableTables.extractBestFit(incoming->getNumSeats());
            if (!tbl) { readyOD.enqueue(incoming); continue; }
            tbl->occupySeats(incoming->getNumSeats());
            incoming->setServiceStart(t);
            int tf = t + incoming->getOrderDuration();
            incoming->setFinishTime(tf);
            inServiceOrders.enqueue(InServiceRecord(incoming, tbl, nullptr, tf));
        }
    }

    {
        LinkedQueue<Order*> notReady;
        Order* o = nullptr;
        while (!readyOT.isEmpty())
        {
            readyOT.dequeue(o);
            if ((t - o->getReadyTime()) >= 1)
            {
                o->setServiceStart(o->getReadyTime() + 1);
                o->setFinishTime(t);
                inServiceOrders.enqueue(InServiceRecord(o, nullptr, nullptr, t));
            }
            else notReady.enqueue(o);
        }
        while (!notReady.isEmpty()) { notReady.dequeue(o); readyOT.enqueue(o); }
    }

    bool comboScooterStarving = false;
    {
        LinkedQueue<Order*> tempWait; Order* o = nullptr;
        while (!readyOCOMBO.isEmpty()) {
            readyOCOMBO.dequeue(o);
            int reqS = o->getReqScooters();
            if (reqS > numScooters) reqS = numScooters;
            if (reqS <= 0) reqS = 1;

            Scooter* sc = nullptr; int pri = 0;
            int found = 0; Scooter** scArr = new Scooter * [reqS];

            while (found < reqS && !availableScooters.isEmpty()) {
                availableScooters.dequeue(sc, pri);
                scArr[found++] = sc;
            }

            if (found == reqS) {
                int maxTicks = 0;
                for (int i = 0; i < reqS; i++) {
                    int ticks = scArr[i]->computeTripTicks(o->getDistance());
                    if (ticks > maxTicks) maxTicks = ticks;
                }
                int finish = t + maxTicks;
                o->setServiceStart(t);
                o->setFinishTime(finish);

                InServiceRecord rec(o, nullptr, nullptr, finish);
                for (int i = 0; i < reqS; i++) {
                    scArr[i]->addDistance(o->getDistance());
                    scArr[i]->incrementOrdersCompleted();
                    rec.comboScooters[rec.numComboScooters++] = scArr[i];
                }
                inServiceOrders.enqueue(rec);
            }
            else {
                for (int i = 0; i < found; i++) availableScooters.enqueue(scArr[i], scArr[i]->availPriority());
                tempWait.enqueue(o);
                comboScooterStarving = true;
            }
            delete[] scArr;

            if (comboScooterStarving) break;
        }
        while (!tempWait.isEmpty()) { tempWait.dequeue(o); readyOCOMBO.enqueue(o); }
    }

    if (comboScooterStarving) return;

    auto assignScooterTo = [&](Order* o) -> bool
        {
            Scooter* sc = getFreeScooter();
            if (!sc) return false;
            sc->addDistance(o->getDistance());
            sc->incrementOrdersCompleted();
            o->setServiceStart(t);
            int tf = t + sc->computeTripTicks(o->getDistance());
            o->setFinishTime(tf);
            InServiceRecord rec(o, nullptr, sc, tf);

            if (activeBreakdowns < (numScooters - 1) && (rand() % 100 < 2)) {
                rec.needsRescue = true;
                activeBreakdowns++;
            }

            inServiceOrders.enqueue(rec);
            return true;
        };

    {
        priQueue<Order*> tempOver; Order* o = nullptr; int pri = 0;
        while (!overwaitOVG.isEmpty()) { overwaitOVG.dequeue(o, pri); if (!assignScooterTo(o)) tempOver.enqueue(o, pri); }
        while (!tempOver.isEmpty()) { tempOver.dequeue(o, pri); overwaitOVG.enqueue(o, pri); }
    }
    {
        CancellableQueue tempReady; Order* o = nullptr;
        while (!readyOVC.isEmpty()) { readyOVC.dequeue(o); if (!assignScooterTo(o)) tempReady.enqueue(o); }
        while (!tempReady.isEmpty()) { tempReady.dequeue(o); readyOVC.enqueue(o); }
    }
    {
        LinkedQueue<Order*> tempReady; Order* o = nullptr;
        while (!readyOVN.isEmpty()) { readyOVN.dequeue(o); if (!assignScooterTo(o)) tempReady.enqueue(o); }
        while (!tempReady.isEmpty()) { tempReady.dequeue(o); readyOVN.enqueue(o); }
    }
    {
        priQueue<Order*> tempReady; Order* o = nullptr; int pri = 0;
        while (!readyOVG.isEmpty()) { readyOVG.dequeue(o, pri); if (!assignScooterTo(o)) tempReady.enqueue(o, pri); }
        while (!tempReady.isEmpty()) { tempReady.dequeue(o, pri); readyOVG.enqueue(o, pri); }
    }
}

void Restaurant::checkInServiceOrders(int t)
{
    InServiceQueue tempIS;
    InServiceRecord rec;
    const int MAX_RELEASED = 64; Table* releasedTables[MAX_RELEASED]; int releasedCount = 0;
    auto tableAlreadyReleased = [&](Table* tbl) -> bool {
        for (int i = 0; i < releasedCount; i++) if (releasedTables[i] == tbl) return true; return false;
        };

    while (!inServiceOrders.isEmpty())
    {
        inServiceOrders.dequeue(rec);

        if (rec.assignedScooter && !rec.assignedScooter->getBrokenDown() && rec.needsRescue && rec.rescueMeetingTime == -1) {
            int totalTripTicks = rec.finishTime - rec.order->getServiceStart();
            if (t >= (rec.order->getServiceStart() + (totalTripTicks / 2)))
            {
                rec.assignedScooter->setBrokenDown(true);
            }
        }
        if (rec.assignedScooter && rec.assignedScooter->getBrokenDown() && rec.rescueScooter == nullptr) {
            Scooter* rescueSc = getFreeScooter();
            if (rescueSc) {
                rec.rescueScooter = rescueSc;
                int ticksElapsed = t - rec.order->getServiceStart();
                double currentDistance = (double)ticksElapsed * rec.assignedScooter->getSpeed();
                if (currentDistance > rec.order->getDistance()) currentDistance = rec.order->getDistance();
                rec.rescueMeetingTime = t + rescueSc->computeTripTicks(currentDistance);
            }
            tempIS.enqueue(rec); continue;
        }
        if (rec.needsRescue && rec.rescueScooter != nullptr && rec.rescueMeetingTime > 0) {
            if (t >= rec.rescueMeetingTime) {
                int ticksElapsed = t - rec.order->getServiceStart();
                double dist = rec.assignedScooter->getSpeed() * ticksElapsed;
                if (dist > rec.order->getDistance()) dist = rec.order->getDistance();
                rec.assignedScooter->setReturnArrivalTime(t + rec.assignedScooter->computeTripTicks(dist));
                rec.assignedScooter->setMainFinishTime(t + rec.assignedScooter->computeTripTicks(dist) + scooterMainOrds);
                rec.assignedScooter->setBrokenDown(false);
                scootersBack.enqueue(rec.assignedScooter, rec.assignedScooter->backPriority());

                rec.assignedScooter = rec.rescueScooter; rec.rescueScooter = nullptr;
                rec.needsRescue = false; rec.rescueMeetingTime = -1;
                activeBreakdowns--;

                double remaining = rec.order->getDistance() - dist;
                rec.finishTime = t + rec.assignedScooter->computeTripTicks(remaining > 0 ? remaining : 0);
                rec.order->setFinishTime(rec.finishTime);
            }
            tempIS.enqueue(rec); continue;
        }

        if (rec.finishTime <= t && !rec.needsRescue)
        {
            if (rec.assignedTable) {
                rec.assignedTable->releaseSeats(rec.order->getNumSeats());
                if (rec.assignedTable->isEmpty() && !tableAlreadyReleased(rec.assignedTable)) {
                    availableTables.enqueue(rec.assignedTable);
                    if (releasedCount < MAX_RELEASED) releasedTables[releasedCount++] = rec.assignedTable;
                }
            }

            if (rec.numComboScooters > 0) {
                for (int i = 0; i < rec.numComboScooters; i++) {
                    Scooter* sc = rec.comboScooters[i];
                    int arrivalT = t + sc->computeTripTicks(rec.order->getDistance());
                    sc->setReturnArrivalTime(arrivalT);
                    if (sc->getOrdersCompleted() >= scooterMainOrds) {
                        sc->setMainFinishTime(arrivalT + scooterMainDur);
                        sc->resetOrdersCompleted();
                    }
                    scootersBack.enqueue(sc, sc->backPriority());
                }
            }

            else if (rec.assignedScooter) {
                Scooter* sc = rec.assignedScooter;
                int arrivalT = t + sc->computeTripTicks(rec.order->getDistance());
                sc->setReturnArrivalTime(arrivalT);
                if (sc->getOrdersCompleted() >= scooterMainOrds) {
                    sc->setMainFinishTime(arrivalT + scooterMainDur);
                    sc->resetOrdersCompleted();
                }
                scootersBack.enqueue(sc, sc->backPriority());
            }

            rec.order->setFinishTime(t);
            if (!finishedOrders.push(rec.order)) delete rec.order;
            totalOrdersServed++;
        }
        else { tempIS.enqueue(rec); }
    }
    while (!tempIS.isEmpty()) { tempIS.dequeue(rec); inServiceOrders.enqueue(rec); }
}

void Restaurant::checkScootersBack(int t)
{
    priQueue<Scooter*> tempBack; Scooter* sc = nullptr; int pri = 0;
    while (!scootersBack.isEmpty()) {
        scootersBack.dequeue(sc, pri);
        if (t >= sc->getReturnArrivalTime()) {
            if (sc->getMainFinishTime() > t) maintenanceScooters.enqueue(sc, -(sc->getMainFinishTime()));
            else availableScooters.enqueue(sc, sc->backPriority());
        }
        else tempBack.enqueue(sc, pri);
    }
    while (!tempBack.isEmpty()) { tempBack.dequeue(sc, pri); scootersBack.enqueue(sc, pri); }
}

void Restaurant::checkMaintenance(int t)
{
    priQueue<Scooter*> tempMaint; Scooter* sc = nullptr; int pri = 0;
    while (!maintenanceScooters.isEmpty()) {
        maintenanceScooters.dequeue(sc, pri);
        if (t >= sc->getMainFinishTime()) availableScooters.enqueue(sc, sc->backPriority());
        else tempMaint.enqueue(sc, pri);
    }
    while (!tempMaint.isEmpty()) { tempMaint.dequeue(sc, pri); maintenanceScooters.enqueue(sc, pri); }
}

void RequestAction::Act(Restaurant* pRest)
{
    Order* pOrder = new Order(orderID, orderType, orderSize, orderPrice,
        orderDistance, actionTime,
        numSeats, orderDuration, canShare, reqChefs, reqScooters);
    pRest->addNewOrder(pOrder);
}

void CancelAction::Act(Restaurant* pRest) { pRest->cancelOrder(orderID); }

void Restaurant::executeActions(int t)
{
    currentTimestep = t; Action* a = nullptr; int pri = 0;
    while (!actionsQueue.isEmpty()) {
        actionsQueue.peek(a, pri); if (-pri != t) break;
        actionsQueue.dequeue(a, pri); a->Act(this); delete a;
    }
}

void Restaurant::runTimestep(int t)
{
    currentTimestep = t;
    executeActions(t);
    assignOrdersToChefs();
    checkCookingOrders(t);
    checkOverwaitOrders(t);
    assignReadyOrders(t);
    checkInServiceOrders(t);
    checkScootersBack(t);
    checkMaintenance(t);
}

void Restaurant::runSimulation(UI* pUI)
{
    if (outputMode == SILENT) cout << "Simulation Starts in Silent mode ...\n";
    const int MAX_T = 100000;
    for (currentTimestep = 1; currentTimestep <= MAX_T; currentTimestep++)
    {
        runTimestep(currentTimestep);
        if (outputMode == INTERACTIVE || outputMode == STEP_BY_STEP) {
            if (pUI) pUI->printScreen(this); else printCurrentState();
            if (outputMode == INTERACTIVE) { if (pUI) pUI->waitForEnter(); else cin.get(); }
            else { if (pUI) pUI->waitOneSecond(); }
        }

        bool done = actionsQueue.isEmpty() && pendingODG.isEmpty() && pendingODN.isEmpty() &&
            pendingOT.isEmpty() && pendingOVC.isEmpty() && pendingOVN.isEmpty() && pendingOVG.isEmpty() && pendingOCOMBO.isEmpty() &&
            cookingOD.isEmpty() && cookingOT.isEmpty() && cookingOV.isEmpty() && cookingOCOMBO.isEmpty() &&
            readyOD.isEmpty() && readyOT.isEmpty() && readyOVC.isEmpty() && readyOVN.isEmpty() && readyOVG.isEmpty() && readyOCOMBO.isEmpty() &&
            inServiceOrders.isEmpty() && scootersBack.isEmpty() && maintenanceScooters.isEmpty();
        if (done) break;
    }
    printFinalReport();
    if (outputMode == SILENT) cout << "Simulation ends, Output file created\n";
}

void Restaurant::randomSimulate()
{
    if (allChefs == nullptr)
    {
        numCN = 5; numCS = 4; totalChefs = numCN + numCS; cnSpeed = 3.0; csSpeed = 2.0; scooterSpeed = 150.0; numScooters = 8;
        scooterMainOrds = 10; scooterMainDur = 15; numTables = 15; overwaitThreshold = 10;
        allChefs = new Chef * [totalChefs]; int chefID = 1;
        for (int i = 0; i < numCN; i++) { allChefs[i] = new Chef(chefID++, CN, cnSpeed); availableCN.enqueue(allChefs[i]); }
        for (int i = numCN; i < totalChefs; i++) { allChefs[i] = new Chef(chefID++, CS, csSpeed); availableCS.enqueue(allChefs[i]); }
        allScooters = new Scooter * [numScooters];
        for (int i = 0; i < numScooters; i++) { allScooters[i] = new Scooter(i + 1, scooterSpeed); availableScooters.enqueue(allScooters[i], allScooters[i]->availPriority()); }
        int tableID = 1;
        for (int i = 0; i < 4; i++) availableTables.enqueue(new Table(tableID++, 4));
        for (int i = 0; i < 6; i++) availableTables.enqueue(new Table(tableID++, 6));
        for (int i = 0; i < 5; i++) availableTables.enqueue(new Table(tableID++, 8));
    }

    mt19937 rng(static_cast<unsigned int>(time(nullptr)));
    uniform_int_distribution<int>     distNumOrders(500, 1000);
    uniform_int_distribution<int>     distTQ(1, 200);
    uniform_int_distribution<int>     distType(0, 6);
    uniform_int_distribution<int>     distSize(1, 20);
    uniform_real_distribution<double> distPrice(10.0, 500.0);
    uniform_int_distribution<int>     distSeats(1, 8);
    uniform_int_distribution<int>     distDuration(5, 50);
    uniform_int_distribution<int>     distCanShare(0, 1);
    uniform_real_distribution<double> distDistance(300.0, 8000.0);
    uniform_int_distribution<int>     distCancelDelay(1, 50);
    uniform_int_distribution<int>     distChance(0, 99);

    const int numOrders = distNumOrders(rng);
    const OrderType typeMap[7] = { ODG, ODN, OT, OVC, OVN, OVG, OCOMBO };
    const int MAX_TRACK = 1000; int ovcIDs[MAX_TRACK], ovcTQs[MAX_TRACK]; int ovcCount = 0;

    for (int i = 0; i < numOrders; i++)
    {
        int ID = i; int TQ = distTQ(rng); OrderType otype = typeMap[distType(rng)];
        int sz = distSize(rng); double price = distPrice(rng); double dist = 0.0;
        int seats = 0, duration = 0, reqC = 0, reqS = 0; bool share = false;

        if (otype == ODG || otype == ODN) {
            seats = distSeats(rng); duration = distDuration(rng); share = (distCanShare(rng) == 1);
        }
        else if (otype == OCOMBO) {
            dist = distDistance(rng); reqC = 2 + (rand() % 3); reqS = 2 + (rand() % 2);
        }
        else if (otype == OVC || otype == OVN || otype == OVG) {
            dist = distDistance(rng);
        }
        addAction(new RequestAction(TQ, ID, otype, sz, price, dist, seats, duration, share, reqC, reqS));

        if (otype == OVC && ovcCount < MAX_TRACK) { ovcIDs[ovcCount] = ID; ovcTQs[ovcCount++] = TQ; }
    }

    int cancelGenerated = 0;
    for (int i = 0; i < ovcCount; i++) {
        if (distChance(rng) < 30) { addAction(new CancelAction(ovcTQs[i] + distCancelDelay(rng), ovcIDs[i])); cancelGenerated++; }
    }

    const int MAX_T = 200000;
    for (currentTimestep = 1; currentTimestep <= MAX_T; currentTimestep++)
    {
        runTimestep(currentTimestep);
        if (outputMode != SILENT && currentTimestep % 50 == 0) printCurrentState();

        bool done = actionsQueue.isEmpty() && pendingODG.isEmpty() && pendingODN.isEmpty() &&
            pendingOT.isEmpty() && pendingOVC.isEmpty() && pendingOVN.isEmpty() && pendingOVG.isEmpty() && pendingOCOMBO.isEmpty() &&
            cookingOD.isEmpty() && cookingOT.isEmpty() && cookingOV.isEmpty() && cookingOCOMBO.isEmpty() &&
            readyOD.isEmpty() && readyOT.isEmpty() && readyOVC.isEmpty() && readyOVN.isEmpty() && readyOVG.isEmpty() && readyOCOMBO.isEmpty() &&
            inServiceOrders.isEmpty() && scootersBack.isEmpty() && maintenanceScooters.isEmpty();
        if (done) break;
    }
    printFinalReport();
}

void Restaurant::printCurrentState()
{
    cout << "\nCurrent Timestep:" << currentTimestep << "\n";
    cout << "=============== Actions List ================\n";
    {
        priQueue<Action*>& aq = const_cast<priQueue<Action*>&>(actionsQueue); priQueue<Action*> tmp; Action* a = nullptr; int pri = 0, total = 0, printed = 0;
        while (!aq.isEmpty()) {
            aq.dequeue(a, pri); total++;
            if (printed < 10) {
                RequestAction* ra = dynamic_cast<RequestAction*>(a); CancelAction* ca = dynamic_cast<CancelAction*>(a);
                if (ra) cout << "[" << ra->getOrderType() << ", " << ra->getActionTime() << ", " << ra->getOrderID() << "] ";
                else if (ca) cout << "(X, " << ca->getActionTime() << ", " << ca->getOrderID() << ") ";
                printed++;
            }
            tmp.enqueue(a, pri);
        }
        if (printed) cout << "\n";
        cout << total << " actions remaining\n";
        while (!tmp.isEmpty()) { tmp.dequeue(a, pri); aq.enqueue(a, pri); }
    }

    auto printLQ = [](LinkedQueue<Order*>& q, const char* lbl) {
        if (q.isEmpty()) return; LinkedQueue<Order*> tmp; Order* o = nullptr; int cnt = 0;
        while (!q.isEmpty()) { q.dequeue(o); tmp.enqueue(o); cnt++; } cout << cnt << " " << lbl << ": ";
        while (!tmp.isEmpty()) { tmp.dequeue(o); cout << o->getID() << " "; q.enqueue(o); } cout << "\n";
        };
    auto printCQ = [](CancellableQueue& q, const char* lbl) {
        if (q.isEmpty()) return; CancellableQueue tmp; Order* o = nullptr; int cnt = 0;
        while (!q.isEmpty()) { q.dequeue(o); tmp.enqueue(o); cnt++; } cout << cnt << " " << lbl << ": ";
        while (!tmp.isEmpty()) { tmp.dequeue(o); cout << o->getID() << " "; q.enqueue(o); } cout << "\n";
        };
    auto printPQ = [](priQueue<Order*>& q, const char* lbl) {
        if (q.isEmpty()) return; priQueue<Order*> tmp; Order* o = nullptr; int pri = 0, cnt = 0;
        while (!q.isEmpty()) { q.dequeue(o, pri); tmp.enqueue(o, pri); cnt++; } cout << cnt << " " << lbl << ": ";
        while (!tmp.isEmpty()) { tmp.dequeue(o, pri); cout << o->getID() << " "; q.enqueue(o, pri); } cout << "\n";
        };

    cout << "--------------- Pending Orders IDs -----------------\n";
    printLQ(const_cast<LinkedQueue<Order*>&>(pendingOCOMBO), "OCOMBO");
    printLQ(const_cast<LinkedQueue<Order*>&>(pendingODG), "ODG");
    printLQ(const_cast<LinkedQueue<Order*>&>(pendingODN), "ODN");
    printLQ(const_cast<LinkedQueue<Order*>&>(pendingOT), "OT");
    printCQ(const_cast<CancellableQueue&>(pendingOVC), "OVC");
    printLQ(const_cast<LinkedQueue<Order*>&>(pendingOVN), "OVN");
    printPQ(const_cast<priQueue<Order*>&>(pendingOVG), "OVG");

    cout << "--------------- Available chefs IDs ----------------\n";
    {
        LinkedQueue<Chef*>& csCopy = const_cast<LinkedQueue<Chef*>&>(availableCS); LinkedQueue<Chef*> tmpCS; Chef* c = nullptr; int cnt = 0;
        while (!csCopy.isEmpty()) { csCopy.dequeue(c); tmpCS.enqueue(c); cnt++; } cout << cnt << " CS: ";
        while (!tmpCS.isEmpty()) { tmpCS.dequeue(c); cout << c->getID() << " "; csCopy.enqueue(c); } cout << "\n";
        LinkedQueue<Chef*>& cnCopy = const_cast<LinkedQueue<Chef*>&>(availableCN); LinkedQueue<Chef*> tmpCN; cnt = 0;
        while (!cnCopy.isEmpty()) { cnCopy.dequeue(c); tmpCN.enqueue(c); cnt++; } cout << cnt << " CN: ";
        while (!tmpCN.isEmpty()) { tmpCN.dequeue(c); cout << c->getID() << " "; cnCopy.enqueue(c); } cout << "\n";
    }

    cout << "--------------- Cooking orders [ID, chefID] --------\n";
    auto printCookQ = [](priQueue<Order*>& q, const char* lbl) {
        if (q.isEmpty()) return; priQueue<Order*> tmp; Order* o = nullptr; int pri = 0, cnt = 0;
        while (!q.isEmpty()) { q.dequeue(o, pri); tmp.enqueue(o, pri); cnt++; } cout << cnt << " " << lbl << ": ";
        while (!tmp.isEmpty()) {
            tmp.dequeue(o, pri);
            cout << "[" << o->getID();
            if (o->getNumAssignedChefs() > 1) { cout << ",MULTI] "; }
            else { Chef* c = o->getAssignedChef(); cout << "," << (c ? c->getID() : -1) << "] "; }
            q.enqueue(o, pri);
        } cout << "\n";
        };
    printCookQ(const_cast<priQueue<Order*>&>(cookingOCOMBO), "OCOMBO");
    printCookQ(const_cast<priQueue<Order*>&>(cookingOD), "OD");
    printCookQ(const_cast<priQueue<Order*>&>(cookingOT), "OT");
    {
        CancellablePriQueue& q = const_cast<CancellablePriQueue&>(cookingOV); CancellablePriQueue tmp; Order* o = nullptr; int pri = 0, cnt = 0;
        while (!q.isEmpty()) { q.dequeue(o, pri); tmp.enqueue(o, pri); cnt++; }
        if (cnt) { cout << cnt << " OV: "; while (!tmp.isEmpty()) { tmp.dequeue(o, pri); Chef* c = o->getAssignedChef(); cout << "[" << o->getID() << "," << (c ? c->getID() : -1) << "] "; q.enqueue(o, pri); } cout << "\n"; }
    }

    cout << "--------------- Ready Orders IDs -------------------\n";
    printLQ(readyOCOMBO, "OCOMBO_ready");
    printLQ(readyOD, "OD_ready");
    printLQ(readyOT, "OT_ready");
    printCQ(readyOVC, "OVC_ready");
    printLQ(readyOVN, "OVN_ready");
    printPQ(readyOVG, "OVG_ready");
    printPQ(overwaitOVG, "Overwait_OVG");

    cout << "--------------- Available scooters IDs -------------\n";
    {
        priQueue<Scooter*>& q = const_cast<priQueue<Scooter*>&>(availableScooters); priQueue<Scooter*> tmp; Scooter* sc = nullptr; int pri = 0, cnt = 0;
        while (!q.isEmpty()) { q.dequeue(sc, pri); tmp.enqueue(sc, pri); cnt++; } cout << cnt << " Scooters: ";
        while (!tmp.isEmpty()) { tmp.dequeue(sc, pri); cout << sc->getID() << " "; q.enqueue(sc, pri); } cout << "\n";
    }

    cout << "--------------- Available tables [ID, cap, free] ---\n";
    {
        BestFitTableQueue& tq = const_cast<BestFitTableQueue&>(availableTables); LinkedQueue<Table*> buf; Table* t2 = nullptr; int cnt = 0;
        while ((t2 = tq.extractBestFit(0)) != nullptr) { buf.enqueue(t2); cnt++; } cout << cnt << " tables: ";
        while (!buf.isEmpty()) { buf.dequeue(t2); cout << "[T" << t2->getID() << "," << t2->getTotalCapacity() << "," << t2->getFreeSeats() << "] "; tq.enqueue(t2); } cout << "\n";
    }

    cout << "--------------- In-Service [ID, S/T] ---------------\n";
    {
        InServiceQueue& isq = const_cast<InServiceQueue&>(inServiceOrders); InServiceQueue tmp; InServiceRecord rec; int cnt = 0;
        while (!isq.isEmpty()) { isq.dequeue(rec); tmp.enqueue(rec); cnt++; } cout << cnt << " Orders: ";
        while (!tmp.isEmpty()) {
            tmp.dequeue(rec);
            if (rec.numComboScooters > 0) cout << "[" << rec.order->getID() << ",ComboSc] ";
            else if (rec.assignedScooter) cout << "[" << rec.order->getID() << ",S" << rec.assignedScooter->getID() << "] ";
            else if (rec.assignedTable) cout << "[" << rec.order->getID() << ",T" << rec.assignedTable->getID() << "] ";
            else cout << "[" << rec.order->getID() << ",pickup] ";
            isq.enqueue(rec);
        }
        cout << "\n";
    }

    cout << "--------------- In-maintenance scooters IDs --------\n";
    {
        priQueue<Scooter*>& q = const_cast<priQueue<Scooter*>&>(maintenanceScooters); priQueue<Scooter*> tmp; Scooter* sc = nullptr; int pri = 0, cnt = 0;
        while (!q.isEmpty()) { q.dequeue(sc, pri); tmp.enqueue(sc, pri); cnt++; } cout << cnt << " scooters: ";
        while (!tmp.isEmpty()) { tmp.dequeue(sc, pri); cout << sc->getID() << " "; q.enqueue(sc, pri); } cout << "\n";
    }

    cout << "--------------- Scooters Back to Restaurant --------\n";
    {
        priQueue<Scooter*>& q = const_cast<priQueue<Scooter*>&>(scootersBack); priQueue<Scooter*> tmp; Scooter* sc = nullptr; int pri = 0, cnt = 0;
        while (!q.isEmpty()) { q.dequeue(sc, pri); tmp.enqueue(sc, pri); cnt++; } cout << cnt << " scooters: ";
        while (!tmp.isEmpty()) { tmp.dequeue(sc, pri); cout << sc->getID() << " "; q.enqueue(sc, pri); } cout << "\n";
    }

    cout << "--------------- Cancelled Orders IDs ---------------\n";
    {
        LinkedQueue<Order*>& q = const_cast<LinkedQueue<Order*>&>(cancelledOrders); LinkedQueue<Order*> tmp; Order* o = nullptr; int cnt = 0;
        while (!q.isEmpty()) { q.dequeue(o); tmp.enqueue(o); cnt++; } cout << cnt << " cancelled: ";
        while (!tmp.isEmpty()) { tmp.dequeue(o); cout << o->getID() << " "; q.enqueue(o); } cout << "\n";
    }

    cout << "--------------- Finished orders IDs ----------------\n";
    {
        ArrayStack<Order*> tmp; Order* o = nullptr; int cnt = 0;
        while (!finishedOrders.isEmpty()) { finishedOrders.pop(o); cout << o->getID() << " "; tmp.push(o); cnt++; }
        if (cnt) cout << "\n"; cout << cnt << " finished\n";
        while (!tmp.isEmpty()) { tmp.pop(o); finishedOrders.push(o); }
    }
}

void Restaurant::printFinalReport() const
{
    ofstream fout; bool writeFile = (outputFilename[0] != '\0');
    if (writeFile) { fout.open(outputFilename); if (!fout.is_open()) { cout << "[ERROR] Cannot open output file: " << outputFilename << "\n"; writeFile = false; } }

    cout << "\n============= SIMULATION COMPLETE =============\n"; cout << "Final timestep: " << currentTimestep << "\n\n";
    if (writeFile) fout << "Final timestep: " << currentTimestep << "\n\n";

    const char* header = "TF\tTQ\tTA\tTR\tTS\tTc\tID\tTi\tTw\tTserv\n";
    cout << header; if (writeFile) fout << header;

    ArrayStack<Order*>& fs = const_cast<ArrayStack<Order*>&>(finishedOrders);
    ArrayStack<Order*> tmp, tmp2; Order* o = nullptr;

    int countODG = 0, countODN = 0, countOT = 0, countOVC = 0, countOVN = 0, countOVG = 0, countOCOMBO = 0;
    double sumTi = 0, sumTc = 0, sumTw = 0, sumTs = 0;
    double totalChefBusyTicks = 0, totalScooterBusyTicks = 0;
    int numFinished = 0, overwaitCount = 0;

    while (!fs.isEmpty()) { fs.pop(o); tmp.push(o); }
    while (!tmp.isEmpty()) { tmp.pop(o); tmp2.push(o); }

    while (!tmp2.isEmpty()) {
        tmp2.pop(o);
        int TQ = o->getArrivalTime(), TA = o->getAssignTime(), TR = o->getReadyTime(), TS = o->getServiceStart(), TF = o->getFinishTime();
        int Ti = (TA - TQ) + (TS - TR), Tc = TR - TA;
        int Tw = Ti + Tc;
        int Tserv = TF - TS;

        cout << TF << "\t" << TQ << "\t" << TA << "\t" << TR << "\t" << TS << "\t" << Tc << "\t" << o->getID() << "\t" << Ti << "\t" << Tw << "\t" << Tserv << "\n";
        if (writeFile) fout << TF << "\t" << TQ << "\t" << TA << "\t" << TR << "\t" << TS << "\t" << Tc << "\t" << o->getID() << "\t" << Ti << "\t" << Tw << "\t" << Tserv << "\n";

        sumTi += Ti; sumTc += Tc; sumTw += Tw; sumTs += Tserv;
        totalChefBusyTicks += Tc;

        OrderType type = o->getType();
        if (type == ODG) countODG++;
        else if (type == ODN) countODN++;
        else if (type == OT) countOT++;
        else if (type == OVC) countOVC++;
        else if (type == OVN) countOVN++;
        else if (type == OVG) {
            countOVG++;
            if ((TS - TR) > overwaitThreshold) overwaitCount++;
        }
        else if (type == OCOMBO) countOCOMBO++;

        if (type == OVC || type == OVN || type == OVG) {
            totalScooterBusyTicks += (Tserv * 2);
        }
        else if (type == OCOMBO) {
            totalScooterBusyTicks += (Tserv * 2 * o->getReqScooters());
        }

        numFinished++;
        fs.push(o);
    }

    LinkedQueue<Order*>& cq = const_cast<LinkedQueue<Order*>&>(cancelledOrders);
    LinkedQueue<Order*> cTmp;
    int numCancelled = 0;
    while (!cq.isEmpty()) {
        cq.dequeue(o);
        OrderType type = o->getType();
        if (type == ODG) countODG++;
        else if (type == ODN) countODN++;
        else if (type == OT) countOT++;
        else if (type == OVC) countOVC++;
        else if (type == OVN) countOVN++;
        else if (type == OVG) countOVG++;
        else if (type == OCOMBO) countOCOMBO++;
        cTmp.enqueue(o);
        numCancelled++;
    }
    while (!cTmp.isEmpty()) { cTmp.dequeue(o); cq.enqueue(o); }

    int total = numFinished + numCancelled;

    cout << "\n--- Statistics ---\n"; if (writeFile) fout << "\n--- Statistics ---\n";

    auto printStat = [&](const std::string& s) {
        cout << s << "\n";
        if (writeFile) fout << s << "\n";
        };

    printStat("1. Total orders: " + to_string(total) + " [ODG:" + to_string(countODG) + " ODN:" + to_string(countODN) + " OT:" + to_string(countOT) + " OVC:" + to_string(countOVC) + " OVN:" + to_string(countOVN) + " OVG:" + to_string(countOVG) + " OCOMBO:" + to_string(countOCOMBO) + "]");
    printStat("2. Total chefs: " + to_string(totalChefs) + " [CN:" + to_string(numCN) + " CS:" + to_string(numCS) + "]");
    printStat("3. Total scooters: " + to_string(numScooters));

    if (total > 0) {
        double pF = 100.0 * numFinished / total;
        double pC = 100.0 * numCancelled / total;
        printStat("4. Finished: " + to_string(pF) + "%  Cancelled: " + to_string(pC) + "%");
    }

    if (numFinished > 0) {
        double pOverwait = 100.0 * overwaitCount / numFinished;
        printStat("5. Overwait OVG percentage (of finished): " + to_string(pOverwait) + "%");
        printStat("6. Averages: Ti=" + to_string(sumTi / numFinished) + " Tc=" + to_string(sumTc / numFinished) + " Tw=" + to_string(sumTw / numFinished) + " Tserv=" + to_string(sumTs / numFinished));
    }

    double simTime = currentTimestep;
    double chefUtil = (totalChefs > 0 && simTime > 0) ? (totalChefBusyTicks / (simTime * totalChefs)) * 100.0 : 0.0;
    double scooterUtil = (numScooters > 0 && simTime > 0) ? (totalScooterBusyTicks / (simTime * numScooters)) * 100.0 : 0.0;

    if (chefUtil > 100.0) chefUtil = 100.0;
    if (scooterUtil > 100.0) scooterUtil = 100.0;

    printStat("7. Scooters utilization: " + to_string(scooterUtil) + "%");
    printStat("8. Chefs utilization: " + to_string(chefUtil) + "%");

    if (writeFile) fout.close();
}

void Restaurant::checkOverwaitOrders(int t)
{
    priQueue<Order*> temp; Order* o = nullptr; int pri = 0;
    while (!readyOVG.isEmpty()) {
        readyOVG.dequeue(o, pri);
        if ((t - o->getReadyTime()) > overwaitThreshold) overwaitOVG.enqueue(o, -(o->getArrivalTime()));
        else temp.enqueue(o, pri);
    }
    while (!temp.isEmpty()) { temp.dequeue(o, pri); readyOVG.enqueue(o, pri); }
}