#ifndef KVTABLE_H
#define KVTABLE_H

#include <sqlite3.h>
#include "common.h"

class KVTable {
public:
    explicit KVTable();
    int upsert(const string& key, const string& value);
    int createDB();
    int countKeysByPrefix(const string& prefix);
private:
    sqlite3* db;
    int create();
};

#endif // KVTABLE_H