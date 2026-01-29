#include "PeerManager.h"
#include "Constants.h"
#include "DatabaseTableManagers.h"
#include "Utils.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

// Forward declaration - implemented in DaemonServer.cpp
extern string getWgInterfaceIP();

PeerManager &PeerManager::getInstance() {
  static PeerManager instance;
  return instance;
}

void PeerManager::setRole(const string &role) {
  m_role = role;
  saveConfig();
}

void PeerManager::setPeerId(const string &id) {
  m_peerId = id;
  saveConfig();
}

void PeerManager::setLeaderAddress(const string &addr) {
  m_leaderAddress = addr;
  saveConfig();
}

string PeerManager::getRole() const { return m_role; }

string PeerManager::getPeerId() const { return m_peerId; }

string PeerManager::getLeaderAddress() const { return m_leaderAddress; }

bool PeerManager::isLeader() const { return m_role == PEER_ROLE_LEADER; }

bool PeerManager::connectToLeader() {
  if (isLeader()) {
    logToFile("Cannot connect to leader: this daemon is the leader", LOG_CORE);
    return false;
  }

  if (m_leaderAddress.empty()) {
    logToFile("Cannot connect to leader: no leader address configured",
              LOG_CORE);
    return false;
  }

  // Close existing connection if any
  if (m_leaderSocket >= 0) {
    close(m_leaderSocket);
    m_leaderSocket = -1;
  }

  m_leaderSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_leaderSocket < 0) {
    logToFile("Failed to create socket for leader connection: " +
                  string(strerror(errno)),
              LOG_CORE);
    return false;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PEER_TCP_PORT);

  if (inet_pton(AF_INET, m_leaderAddress.c_str(), &addr.sin_addr) <= 0) {
    logToFile("Invalid leader address: " + m_leaderAddress, LOG_CORE);
    close(m_leaderSocket);
    m_leaderSocket = -1;
    return false;
  }

  if (connect(m_leaderSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    logToFile("Failed to connect to leader at " + m_leaderAddress + ":" +
                  to_string(PEER_TCP_PORT) + ": " + strerror(errno),
              LOG_CORE);
    close(m_leaderSocket);
    m_leaderSocket = -1;
    return false;
  }

  m_connectedToLeader = true;
  logToFile("Connected to leader at " + m_leaderAddress, LOG_CORE);

  // Send registration message with peer info
  json regMsg;
  regMsg["command"] = COMMAND_REGISTER_PEER;
  regMsg["peer_id"] = m_peerId;
  regMsg["ip"] = getWgInterfaceIP();

  // Get hostname
  char hostname[256];
  if (gethostname(hostname, sizeof(hostname)) == 0) {
    regMsg["hostname"] = string(hostname);
  }

  sendToLeader(regMsg);

  // Wait for and consume the registration response to avoid it being
  // returned for subsequent commands
  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  ssize_t n = read(m_leaderSocket, buffer, sizeof(buffer) - 1);
  if (n > 0) {
    logToFile("Registration response: " + string(buffer), LOG_CORE);
  }

  return true;
}

void PeerManager::disconnectFromLeader() {
  if (m_leaderSocket >= 0) {
    close(m_leaderSocket);
    m_leaderSocket = -1;
  }
  m_connectedToLeader = false;
  logToFile("Disconnected from leader", LOG_CORE);
}

bool PeerManager::isConnectedToLeader() const { return m_connectedToLeader; }

bool PeerManager::connectToPeer(const string &peer_id, const string &ip) {
  // Check if already connected
  {
    lock_guard<mutex> lock(m_peersMutex);
    auto it = m_peers.find(peer_id);
    if (it != m_peers.end() && it->second.socket_fd >= 0) {
      return true; // Already connected
    }
  }

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    logToFile("Failed to create socket for peer " + peer_id + ": " +
                  string(strerror(errno)),
              LOG_CORE);
    return false;
  }

  // Set connection timeout
  struct timeval timeout;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PEER_TCP_PORT);

  if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
    logToFile("Invalid peer address: " + ip, LOG_CORE);
    close(sock);
    return false;
  }

  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    logToFile("Failed to connect to peer " + peer_id + " at " + ip + ":" +
                  to_string(PEER_TCP_PORT) + ": " + strerror(errno),
              LOG_CORE);
    close(sock);
    return false;
  }

  logToFile("Connected to peer " + peer_id + " at " + ip, LOG_CORE);

  // Register in memory
  {
    lock_guard<mutex> lock(m_peersMutex);
    PeerInfo info;
    info.peer_id = peer_id;
    info.ip_address = ip;
    info.is_online = true;
    info.socket_fd = sock;
    m_peers[peer_id] = info;
  }

  return true;
}

int PeerManager::getLeaderSocket() const { return m_leaderSocket; }

void PeerManager::registerPeer(const string &peer_id, const string &ip,
                               const string &mac, const string &hostname,
                               int socket_fd) {
  lock_guard<mutex> lock(m_peersMutex);
  PeerInfo info;
  info.peer_id = peer_id;
  info.ip_address = ip;
  info.mac_address = mac;
  info.hostname = hostname;
  info.is_online = true;
  info.socket_fd = socket_fd;
  m_peers[peer_id] = info;
  logToFile("Registered peer: " + peer_id + " (" + ip + ")", LOG_CORE);
}

void PeerManager::unregisterPeer(const string &peer_id) {
  lock_guard<mutex> lock(m_peersMutex);
  auto it = m_peers.find(peer_id);
  if (it != m_peers.end()) {
    if (it->second.socket_fd >= 0) {
      close(it->second.socket_fd);
    }
    m_peers.erase(it);
    logToFile("Unregistered peer: " + peer_id, LOG_CORE);
  }
}

void PeerManager::updatePeerStatus(const string &peer_id, bool online) {
  lock_guard<mutex> lock(m_peersMutex);
  auto it = m_peers.find(peer_id);
  if (it != m_peers.end()) {
    it->second.is_online = online;
  }
}

vector<PeerInfo> PeerManager::listPeers() const {
  lock_guard<mutex> lock(m_peersMutex);
  vector<PeerInfo> result;
  for (const auto &pair : m_peers) {
    result.push_back(pair.second);
  }
  return result;
}

PeerInfo PeerManager::getPeerInfo(const string &peer_id) const {
  lock_guard<mutex> lock(m_peersMutex);
  auto it = m_peers.find(peer_id);
  if (it != m_peers.end()) {
    return it->second;
  }
  return PeerInfo{"", "", "", "", false, -1};
}

bool PeerManager::sendToLeader(const json &message) {
  if (!m_connectedToLeader || m_leaderSocket < 0) {
    logToFile("Cannot send to leader: not connected", LOG_CORE);
    return false;
  }

  string msg = message.dump() + "\n";
  ssize_t sent = write(m_leaderSocket, msg.c_str(), msg.length());
  if (sent < 0) {
    logToFile("Failed to send to leader: " + string(strerror(errno)), LOG_CORE);
    disconnectFromLeader();
    return false;
  }
  return true;
}

bool PeerManager::sendToPeer(const string &peer_id, const json &message) {
  lock_guard<mutex> lock(m_peersMutex);
  auto it = m_peers.find(peer_id);
  if (it == m_peers.end() || it->second.socket_fd < 0) {
    logToFile("Cannot send to peer " + peer_id + ": not connected", LOG_CORE);
    return false;
  }

  string msg = message.dump() + "\n";
  ssize_t sent = write(it->second.socket_fd, msg.c_str(), msg.length());
  if (sent < 0) {
    logToFile("Failed to send to peer " + peer_id + ": " +
                  string(strerror(errno)),
              LOG_CORE);
    return false;
  }
  return true;
}

void PeerManager::broadcastToWorkers(const json &message) {
  lock_guard<mutex> lock(m_peersMutex);
  string msg = message.dump() + "\n";

  for (auto &pair : m_peers) {
    if (pair.second.socket_fd >= 0 && pair.second.is_online) {
      ssize_t sent =
          write(pair.second.socket_fd, msg.c_str(), msg.length());
      if (sent < 0) {
        logToFile("Failed to broadcast to " + pair.first, LOG_CORE);
      }
    }
  }
}

void PeerManager::loadConfig() {
  m_role = SettingsTable::getSetting("peer_role");
  m_peerId = SettingsTable::getSetting("peer_id");
  m_leaderAddress = SettingsTable::getSetting("peer_leader_address");
  logToFile("Peer config loaded: role=" + m_role + ", id=" + m_peerId +
                ", leader=" + m_leaderAddress,
            LOG_CORE);
}

void PeerManager::saveConfig() {
  if (!m_role.empty())
    SettingsTable::setSetting("peer_role", m_role);
  if (!m_peerId.empty())
    SettingsTable::setSetting("peer_id", m_peerId);
  if (!m_leaderAddress.empty())
    SettingsTable::setSetting("peer_leader_address", m_leaderAddress);
}
