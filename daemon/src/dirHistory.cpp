#include "dirHistory.h"

int DirHistory::numberOfPointers() {
    return kvTable.countKeysByPrefix(DIR_HISTORY_POINTER_PREFIX);
}

void DirHistory::resetToBeginningState() {
    

}
