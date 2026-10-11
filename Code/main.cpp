#include "UI.h"
#include "Restaurant.h"
#include <iostream>

int main()
{
    UI ui;
    int choice = ui.readMode();

    if (choice == 5)
    {
        cout << "\nGoodbye!\n";
        return 0;
    }

    Restaurant restaurant;

    if (choice == 4)
    {
        restaurant.setOutputMode(INTERACTIVE);
        restaurant.setOutputFilename("random_output.txt");
        restaurant.randomSimulate();
        return 0;
    }

    ui.readFilenames();

    OutputMode mode;
    switch (choice)
    {
        case 1:  mode = INTERACTIVE;   break;
        case 2:  mode = STEP_BY_STEP;  break;
        default: mode = SILENT;        break;
    }

    restaurant.setOutputMode(mode);
    restaurant.setOutputFilename(ui.getOutputFile());
    restaurant.loadFile(ui.getInputFile());
    restaurant.runSimulation(&ui);

    return 0;
}
