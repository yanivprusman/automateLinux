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
    static int deleteByPrefix(const string& prefix);
    static int deleteEntry(const string& key);
    static int createDB();
    static int countKeysByPrefix(const string& prefix);
    static string get(const string& key);
    static std::vector<std::pair<string, string>> getAll();
    static std::vector<std::pair<string, string>> getByPrefix(const string& prefix);
};

#endif // KVTABLE_H