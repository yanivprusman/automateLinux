#ifndef KVTABLE_H
#define KVTABLE_H

#include "MySQLManager.h"
#include "common.h"
#include <vector>

class KVTable {
private:
  static sql::Connection *con;
  static void checkConnection();

public:
  explicit KVTable();
  ~KVTable();

  static int upsert(const std::string &key, const std::string &value);
  static int insertAt(const std::string &keyPrefix, int index,
                      const std::string &value);
  static int deleteByPrefix(const std::string &prefix);
  static int deleteEntry(const std::string &key);
  static int createDB();
  static int countKeysByPrefix(const std::string &prefix);
  static std::string get(const std::string &key);
  static std::vector<std::pair<std::string, std::string>> getAll();
  static std::vector<std::pair<std::string, std::string>>
  getByPrefix(const std::string &prefix);
};

#endif // KVTABLE_H