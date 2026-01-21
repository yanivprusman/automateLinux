#ifndef PORTAL_MANAGER_H
#define PORTAL_MANAGER_H

#include <cstdint>
#include <map>
#include <string>

// Manages XDG Desktop Portal session for screen capture
// Returns PipeWire node ID and FD for use with GStreamer pipewiresrc
class PortalManager {
public:
  struct Config {
    std::string appId;
    std::string tokenDir; // Directory to save restore tokens
  };

  PortalManager(const Config &config);
  ~PortalManager();

  // Initialize portal session and get PipeWire access
  // Returns true on success, fills node_id and pipewire_fd
  bool init();

  // Get PipeWire node ID (for pipewiresrc path property)
  uint32_t getNodeId() const { return m_node_id; }

  // Get PipeWire FD (for pipewiresrc fd property)
  int getPipeWireFd() const { return m_pipewire_fd; }

  // Check if portal session is active
  bool isActive() const { return m_active; }

private:
  bool setupPortal();
  std::string loadRestoreToken();
  void saveRestoreToken(const std::string &token);
  void clearRestoreToken();

  Config m_config;
  uint32_t m_node_id = 0;
  int m_pipewire_fd = -1;
  bool m_active = false;
  std::string m_session_handle;
};

#endif
