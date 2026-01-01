#include "MySQLManager.h"
#include "Globals.h"
#include "Utils.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <random>
#include <sys/socket.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

pid_t MySQLManager::mysqlPid = -1;
std::string MySQLManager::mysqlSocket = "";
int MySQLManager::mysqlPort = 0;
std::string MySQLManager::mysqlUser = "automate_user";
std::string MySQLManager::mysqlPassword =
    "automate_password"; // Simple password for local access
std::string MySQLManager::mysqlDatabase = "automate_db";

int MySQLManager::findFreePort() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return 3307; // Fallback

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = 0; // Let OS choose

  if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    close(sock);
    return 3307;
  }

  socklen_t len = sizeof(sin);
  if (getsockname(sock, (struct sockaddr *)&sin, &len) < 0) {
    close(sock);
    return 3307;
  }

  int port = ntohs(sin.sin_port);
  close(sock);
  return port;
}

void MySQLManager::generateMySQLConfigFile(int port, const std::string &dataDir,
                                           const std::string &socketPath,
                                           const std::string &configFile) {
  std::ofstream ofs(configFile);
  ofs << "[mysqld]" << std::endl;
  ofs << "datadir=" << dataDir << "/data" << std::endl;
  ofs << "socket=" << socketPath << std::endl;
  ofs << "port=" << port << std::endl;
  ofs << "log-error=" << dataDir << "/logs/error.log" << std::endl;
  ofs << "pid-file=" << dataDir << "/mysqld.pid" << std::endl;
  // Essential settings for isolation and specific paths
  ofs << "lc-messages-dir=/usr/share/mysql" << std::endl;
  ofs << "skip-networking=0" << std::endl; // Allow TCP/IP for our client
  ofs << "bind-address=127.0.0.1" << std::endl;
  ofs.close();
}

void MySQLManager::initializeMySQLDataDir(const std::string &mysqldPath,
                                          const std::string &dataDir,
                                          const std::string &configFile) {
  std::string initCmd =
      mysqldPath + " --defaults-file=" + configFile +
      " --initialize-insecure --user=" + std::string(getenv("USER"));
  logToFile("MySQLManager: Initializing data dir: " + initCmd);

  // Create necessary directories
  std::filesystem::create_directories(dataDir + "/data");
  std::filesystem::create_directories(dataDir + "/logs");

  // Execute initialization
  std::string output = executeCommand((initCmd + " 2>&1").c_str());
  logToFile("MySQLManager: Init output: " + output);
}

void MySQLManager::startMySQLServer(const std::string &mysqldPath,
                                    const std::string &configFile) {
  pid_t pid = fork();
  if (pid == 0) {
    // Child process
    // Redirect stdout/stderr to log file to avoid cluttering daemon output if
    // needed For now, we rely on MySQL's internal logging configured in my.cnf
    std::string defaultsArg = "--defaults-file=" + configFile;
    execl(mysqldPath.c_str(), "mysqld", defaultsArg.c_str(), "--user",
          getenv("USER"), (char *)NULL);
    exit(1);
  } else if (pid > 0) {
    mysqlPid = pid;
    logToFile("MySQLManager: Started MySQL server with PID " +
              std::to_string(pid));
  } else {
    logToFile("MySQLManager: Failed to fork MySQL process", 0xFFFFFFFF);
  }
}

bool MySQLManager::isServerRunning(const std::string &socketPath) {
  return std::filesystem::exists(socketPath);
}

void MySQLManager::createDatabaseAndUser(int port,
                                         const std::string &socketPath) {
  try {
    sql::Driver *driver = get_driver_instance();
    // Connect as root (no password because of initialize-insecure)
    // We use the socket to connect initially to ensure we are talking to OUR
    // instance But Connector/C++ with TCP might be easier if socket connection
    // is tricky with exact strings. Let's try TCP on localhost with the port we
    // assigned.

    std::string host = "tcp://127.0.0.1:" + std::to_string(port);
    std::unique_ptr<sql::Connection> con(driver->connect(host, "root", ""));

    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    stmt->execute("CREATE DATABASE IF NOT EXISTS " + mysqlDatabase);
    stmt->execute("CREATE USER IF NOT EXISTS '" + mysqlUser +
                  "'@'localhost' IDENTIFIED BY '" + mysqlPassword + "'");
    stmt->execute("GRANT ALL PRIVILEGES ON " + mysqlDatabase + ".* TO '" +
                  mysqlUser + "'@'localhost'");
    stmt->execute("FLUSH PRIVILEGES");

    // Connect to the database to create tables
    con->setSchema(mysqlDatabase);
    std::unique_ptr<sql::Statement> tableStmt(con->createStatement());

    // 1. Terminal History Table
    tableStmt->execute("CREATE TABLE IF NOT EXISTS terminal_history ("
                       "tty INT, "
                       "entry_index INT, "
                       "path TEXT NOT NULL, "
                       "PRIMARY KEY (tty, entry_index))");

    // 2. Terminal Sessions (TTY pointers)
    tableStmt->execute("CREATE TABLE IF NOT EXISTS terminal_sessions ("
                       "tty INT PRIMARY KEY, "
                       "history_index INT NOT NULL)");

    // 3. Automation Configs (Macros and Filters)
    tableStmt->execute("CREATE TABLE IF NOT EXISTS automation_configs ("
                       "config_key VARCHAR(255) PRIMARY KEY, "
                       "config_value JSON NOT NULL)");

    // 4. Device Registry
    tableStmt->execute("CREATE TABLE IF NOT EXISTS device_registry ("
                       "device_type VARCHAR(50) PRIMARY KEY, "
                       "path TEXT NOT NULL)");

    // 5. System Settings
    tableStmt->execute("CREATE TABLE IF NOT EXISTS system_settings ("
                       "setting_key VARCHAR(255) PRIMARY KEY, "
                       "setting_value TEXT NOT NULL)");

    logToFile(
        "MySQLManager: Database, user, and tables configured successfully.");
  } catch (sql::SQLException &e) {
    logToFile("MySQLManager: Error configuring database: " +
                  std::string(e.what()),
              0xFFFFFFFF);
  }
}

void MySQLManager::initializeAndStartMySQL() {
  std::string baseDir = directories.base + "data/mysql";
  std::string binDir = baseDir + "/bin";
  std::string confDir = baseDir + "/conf";
  std::string dataDir =
      baseDir; // Use baseDir as root for our custom mysql setup

  std::filesystem::create_directories(binDir);
  std::filesystem::create_directories(confDir);

  // Copy mysqld binary for AppArmor workaround
  std::string targetMysqld = binDir + "/mysqld";
  if (!std::filesystem::exists(targetMysqld)) {
    logToFile("MySQLManager: Copying /usr/sbin/mysqld to " + targetMysqld);
    std::filesystem::copy_file(
        "/usr/sbin/mysqld", targetMysqld,
        std::filesystem::copy_options::overwrite_existing);
    std::filesystem::permissions(targetMysqld,
                                 std::filesystem::perms::owner_read |
                                     std::filesystem::perms::owner_write |
                                     std::filesystem::perms::owner_exec,
                                 std::filesystem::perm_options::add);
  }

  // Find free port
  mysqlPort = findFreePort();
  logToFile("MySQLManager: Selected port " + std::to_string(mysqlPort));

  // Config paths
  std::string configFile = confDir + "/my.cnf";
  mysqlSocket = baseDir + "/mysql.sock";

  generateMySQLConfigFile(mysqlPort, dataDir, mysqlSocket, configFile);

  // Initialize if needed
  if (!std::filesystem::exists(dataDir + "/data/mysql")) {
    initializeMySQLDataDir(targetMysqld, dataDir, configFile);
  }

  // Check for orphaned process using PID file
  std::string pidFile = dataDir + "/mysqld.pid";
  if (std::filesystem::exists(pidFile)) {
    std::ifstream ifs(pidFile);
    int oldPid;
    if (ifs >> oldPid) {
      if (kill(oldPid, 0) == 0) { // Process exists
        logToFile("MySQLManager: Found orphaned MySQL process " +
                  std::to_string(oldPid) + ". Killing it.");
        kill(oldPid, SIGTERM);
        waitpid(oldPid, NULL, 0);
      }
    }
    std::filesystem::remove(pidFile);
  }

  // Start Server
  startMySQLServer(targetMysqld, configFile);

  // Wait for startup
  int attempts = 0;
  while (!isServerRunning(mysqlSocket) && attempts < 50) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    attempts++;
  }

  if (isServerRunning(mysqlSocket)) {
    logToFile("MySQLManager: MySQL Server is ready.");
    // Setup initial DB structure
    createDatabaseAndUser(mysqlPort, mysqlSocket);
  } else {
    logToFile("MySQLManager: Failed to start MySQL server within timeout.",
              0xFFFFFFFF);
  }
}

void MySQLManager::stopMySQL() {
  // Try to stop the one we started
  if (mysqlPid > 0) {
    logToFile("MySQLManager: Stopping MySQL server (PID " +
              std::to_string(mysqlPid) + ")");
    kill(mysqlPid, SIGTERM);
    waitpid(mysqlPid, NULL, 0);
    mysqlPid = -1;
  }

  // Also check the PID file just in case
  std::string pidFile = directories.base + "data/mysql/mysqld.pid";
  if (std::filesystem::exists(pidFile)) {
    std::ifstream ifs(pidFile);
    int oldPid;
    if (ifs >> oldPid) {
      if (kill(oldPid, 0) == 0) {
        logToFile(
            "MySQLManager: Stopping orphaned MySQL process from PID file " +
            std::to_string(oldPid));
        kill(oldPid, SIGTERM);
        waitpid(oldPid, NULL, 0);
      }
    }
    std::filesystem::remove(pidFile);
  }
}

sql::Connection *MySQLManager::getConnection() {
  try {
    sql::Driver *driver = get_driver_instance();
    std::string host = "tcp://127.0.0.1:" + std::to_string(mysqlPort);
    sql::Connection *con = driver->connect(host, mysqlUser, mysqlPassword);
    con->setSchema(mysqlDatabase);
    return con;
  } catch (sql::SQLException &e) {
    logToFile("MySQLManager: Failed to connect: " + std::string(e.what()),
              0xFFFFFFFF);
    return nullptr;
  }
}
