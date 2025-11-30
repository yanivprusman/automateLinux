#ifndef DIRHISTORY_H
#define DIRHISTORY_H

#include "common.h"
#include "KVTable.h"
#include <string>
#include <fstream>

class DirHistory {
    
public:
    string defaultDir = "/home/yaniv/coding";
    int numberOfPointers();
    void resetToBeginningState();
};
#endif // DIRHISTORY_H