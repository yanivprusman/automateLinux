#ifndef PEER_MANAGER_H
#define PEER_MANAGER_H

#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

struct PeerInfo {
  std::string peer_id;
  std::string ip_address;
  std::string mac_address;
  std::string hostname;
  bool is_online;
  int socket_fd;  // -1 if not connected
};

class PeerManager {
public:
  static PeerManager &getInstance();

  // Configuration
  void setRole(const std::string &role);
  void setPeerId(const std::string &id);
  void setLeaderAddress(const std::string &addr);
  std::string getRole() const;
  std::string getPeerId() const;
  std::string getLeaderAddress() const;
  bool isLeader() const;

  // Connection management
  bool connectToLeader();
  void disconnectFromLeader();
  bool isConnectedToLeader() const;
  int getLeaderSocket() const;
  bool connectToPeer(const std::string &peer_id, const std::string &ip);

  // Peer tracking (for leader)
  void registerPeer(const std::string &peer_id, const std::string &ip,
                    const std::string &mac, const std::string &hostname,
                    int socket_fd);
  void unregisterPeer(const std::string &peer_id);
  void updatePeerStatus(const std::string &peer_id, bool online);
  std::vector<PeerInfo> listPeers() const;
  PeerInfo getPeerInfo(const std::string &peer_id) const;

  // Messaging
  bool sendToLeader(const json &message);
  bool sendToPeer(const std::string &peer_id, const json &message);
  void broadcastToWorkers(const json &message);

  // Persistence
  void loadConfig();
  void saveConfig();

private:
  PeerManager() = default;
  PeerManager(const PeerManager &) = delete;
  PeerManager &operator=(const PeerManager &) = delete;

  std::string m_role = "";       // "leader" or "worker"
  std::string m_peerId = "";     // This peer's identifier
  std::string m_leaderAddress = "";
  int m_leaderSocket = -1;
  bool m_connectedToLeader = false;

  std::map<std::string, PeerInfo> m_peers;  // For leader: tracks all workers
  mutable std::mutex m_peersMutex;
};

#endif // PEER_MANAGER_H
