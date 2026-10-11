#pragma once

class UI;

#include "priQueue.h"
#include "LinkedQueue.h"
#include "ArrayStack.h"
#include "CancellableQueue.h"
#include "CancellablePriQueue.h"
#include "BestFitTableQueue.h"
#include "InServiceRecord.h"
#include "Order.h"
#include "Table.h"
#include "Chef.h"
#include "Scooter.h"
#include "Action.h"
#include "RequestAction.h"
#include "CancelAction.h"

enum OutputMode { SILENT, INTERACTIVE, STEP_BY_STEP };

class Restaurant
{
private:

    priQueue<Action*> actionsQueue;

    LinkedQueue<Order*>   pendingODG;
    LinkedQueue<Order*>   pendingODN;
    LinkedQueue<Order*>   pendingOT;
    CancellableQueue      pendingOVC;
    LinkedQueue<Order*>   pendingOVN;
    priQueue<Order*>      pendingOVG;
    LinkedQueue<Order*>   pendingOCOMBO;

    priQueue<Order*>      cookingOD;
    priQueue<Order*>      cookingOT;
    CancellablePriQueue   cookingOV;
    priQueue<Order*>      cookingOCOMBO;

    LinkedQueue<Order*>   readyOD;
    LinkedQueue<Order*>   readyOT;
    CancellableQueue      readyOVC;
    LinkedQueue<Order*>   readyOVN;
    priQueue<Order*>      readyOVG;
    priQueue<Order*>      overwaitOVG;
    LinkedQueue<Order*>   readyOCOMBO;

    InServiceQueue        inServiceOrders;
    LinkedQueue<Order*>   cancelledOrders;
    ArrayStack<Order*>    finishedOrders;

    LinkedQueue<Chef*>    availableCS;
    LinkedQueue<Chef*>    availableCN;
    Chef**   allChefs;
    int      numCN, numCS, totalChefs;
    double   cnSpeed, csSpeed;

    priQueue<Scooter*>    availableScooters;
    priQueue<Scooter*>    scootersBack;
    priQueue<Scooter*>    maintenanceScooters;
    Scooter** allScooters;
    int       numScooters;
    double    scooterSpeed;
    int       scooterMainOrds;
    int       scooterMainDur;

    BestFitTableQueue availableTables;
    int               numTables;

    int        overwaitThreshold;
    int        currentTimestep;
    OutputMode outputMode;
    char       outputFilename[256];

    int totalOrdersServed;
    int totalOrdersCancelled;
    int totalOrdersRejected;
    int activeBreakdowns;

    void addAction(Action* pAction);
    Chef* getFreeChef(ChefType required);
    Scooter* getFreeScooter();
    void assignOrdersToChefs();
    void checkCookingOrders(int t);
    void assignReadyOrders(int t);
    void checkInServiceOrders(int t);
    void checkScootersBack(int t);
    void checkMaintenance(int t);
    void checkOverwaitOrders(int t);

public:
    Restaurant();
    ~Restaurant();

    void setOutputMode(OutputMode mode) { outputMode = mode; }
    void setOutputFilename(const char* fname)
    {
        int i = 0;
        while (fname[i] && i < 254) { outputFilename[i] = fname[i]; i++; }
        outputFilename[i] = '\0';
    }

    void loadFile(const char* filename);
    void runSimulation(UI* pUI = nullptr);
    void randomSimulate();
    void runTimestep(int t);
    void executeActions(int t);
    void printCurrentState();
    void printFinalReport()  const;

    void addNewOrder(Order* pOrder);
    void cancelOrder(int orderID);
};
