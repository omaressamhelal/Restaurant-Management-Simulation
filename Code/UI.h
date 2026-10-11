#pragma once

class Restaurant;

class UI
{
private:
    char inputFile[256];
    char outputFile[256];

public:
    UI();

    int  readMode();
    void readFilenames();

    const char* getInputFile()  const { return inputFile;  }
    const char* getOutputFile() const { return outputFile; }

    void printScreen(Restaurant* pRest) const;
    void waitForEnter()  const;
    void waitOneSecond() const;
};
