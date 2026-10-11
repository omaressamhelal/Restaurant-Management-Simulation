#include <iostream>
#include <thread>
#include <chrono>
using namespace std;

#include "UI.h"
#include "Restaurant.h"

UI::UI()
{
    inputFile[0]  = '\0';
    outputFile[0] = '\0';
}

int UI::readMode()
{
    int choice = 0;
    while (true)
    {
        cout << "\n========================================\n";
        cout << "  Restaurant Management Simulation\n";
        cout << "  Team 23  --  CMP S103  Spring 2026\n";
        cout << "========================================\n\n";
        cout << "Select Interface Mode:\n";
        cout << "  1. Interactive   (view each timestep; press ENTER to advance)\n";
        cout << "  2. Step-by-Step  (view each timestep; advances every 1 second)\n";
        cout << "  3. Silent        (run to completion; output file only)\n";
        cout << "  4. Random Test   (Stress Test: 500+ auto-generated orders)\n";
        cout << "  5. Exit\n";
        cout << "\nEnter choice [1-5]: ";

        cin >> choice;
        char ch;
        while (cin.get(ch) && ch != '\n') {}

        if (choice >= 1 && choice <= 5) return choice;
        cout << "[!] Please enter a number between 1 and 5.\n";
    }
}

void UI::readFilenames()
{
    cout << "\nEnter input filename  (e.g. input.txt):  ";
    cin >> inputFile;
    cout << "Enter output filename (e.g. output.txt): ";
    cin >> outputFile;
    char ch;
    while (cin.get(ch) && ch != '\n') {}
}

void UI::printScreen(Restaurant* pRest) const
{
    if (pRest) pRest->printCurrentState();
}

void UI::waitForEnter() const
{
    cout << "\nPRESS ANY KEY TO MOVE TO NEXT STEP!\n";
    cin.get();
}

void UI::waitOneSecond() const
{

    this_thread::sleep_for(chrono::seconds(1));
}
