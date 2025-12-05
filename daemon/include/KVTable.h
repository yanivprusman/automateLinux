#ifndef KVTABLE_H
#define KVTABLE_H

#include <sqlite3.h>
#include "common.h"

class KVTable {
    private:
        static sqlite3* db;
        static int create();
public:
    explicit KVTable();
    static int upsert(const string& key, const string& value);
    static int insertAt(const string& keyPrefix, int index, const string& value);
    static int createDB();
    static int countKeysByPrefix(const string& prefix);
    static string get(const string& key);
};

#endif // KVTABLE_H