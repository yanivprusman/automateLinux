#ifndef MYSQLMANAGER_H
#define MYSQLMANAGER_H

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <mysql_connection.h>
#include <string>

class MySQLManager {
public:
  static void initializeAndStartMySQL();
  static void stopMySQL();
  static sql::Connection *getConnection();

private:
  static int findFreePort();
  static void generateMySQLConfigFile(int port, const std::string &dataDir,
                                      const std::string &socketPath,
                                      const std::string &configFile);
  static void initializeMySQLDataDir(const std::string &mysqldPath,
                                     const std::string &dataDir,
                                     const std::string &configFile);
  static void startMySQLServer(const std::string &mysqldPath,
                               const std::string &configFile);
  static void createDatabaseAndUser(int port, const std::string &socketPath);
  static bool isServerRunning(const std::string &socketPath);

  static pid_t mysqlPid;
  static std::string mysqlSocket;
  static int mysqlPort;
  static std::string mysqlUser;
  static std::string mysqlPassword;
  static std::string mysqlDatabase;
};

#endif // MYSQLMANAGER_H
