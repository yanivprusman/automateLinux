#include "PortalManager.h"
#include <cstring>
#include <fstream>
#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Portal state for callbacks - using static globals is a bit messy but standard
// for GDBus C API callbacks In a more complex app we'd pass a context structure
static std::string g_session_handle;
static uint32_t g_node_id = 0;
static GMainLoop *g_portal_loop = nullptr;
static int g_portal_step = 0;

// Helper to pass context if needed, but for now we rely on the class instance
// managing the loop actually the static callback needs to write to these
// globals.

static void on_portal_response(GDBusConnection *conn, const gchar *sender,
                               const gchar *path, const gchar *iface,
                               const gchar *signal, GVariant *params,
                               gpointer data) {
  uint32_t response;
  GVariant *results;
  g_variant_get(params, "(u@a{sv})", &response, &results);

  if (response != 0) {
    if (response != 1) { // 1 is just cancelled, don't spam error
      std::cerr << "Portal: Error response in step " << g_portal_step
                << " (code " << response << ")" << std::endl;
    }
    g_main_loop_quit(g_portal_loop);
    return;
  }

  PortalManager *manager = static_cast<PortalManager *>(data);
  // We can't easily access the manager instance to call private methods from
  // here exactly without changing the callback signature or passing user_data.
  // We passed 'data' as user_data in signal_subscribe.

  // Actually, to keep it simple and consistent with the original code structure
  // (which worked), I will keep the token saving logic here but use a callback
  // or helper if I can, OR just access the path from a global or pass it
  // through. Let's rely on the `data` pointer being the `PortalManager`
  // instance if possible, but `saveRestoreToken` is private. For simplicity in
  // this port, I'll pass a std::string* or similar for the token path OR just
  // let the caller handle the persistent storage after retrieval? No, the
  // original logic did it inline.

  // Let's modify the flow: The callback extracts the token and puts it in a
  // global/result struct, and the `setupPortal` method (which has access to
  // `this`) saves it. This is cleaner.

  if (g_portal_step == 1) { // CreateSession
    const gchar *handle = nullptr;
    if (g_variant_lookup(results, "session_handle", "&s", &handle)) {
      g_session_handle = handle;
      // std::cout << "Portal: Session handle obtained: " << g_session_handle <<
      // std::endl;
    }
  } else if (g_portal_step == 3) { // Start
    // Extract restore token
    const gchar *restore_token = nullptr;
    if (g_variant_lookup(results, "restore_token", "&s", &restore_token)) {
      // We will store this in a static string to be picked up by the class
      // method after loop Or we can just print it? The original code saved it
      // immediately. I will use a simple file writer here if I have the path,
      // but better to let the main thread do it. Ill use a global to hold the
      // new token.
    }

    // We can't return the token easily from here to the main loop context
    // without a shared state object.
  }
}

// Rewriting to avoid complex static state management:
// The original code had logic inside the callback.
// I will keep the logic mostly as is but clean up the token saving.

struct PortalContext {
  std::string newToken;
  std::string tokenPath;
};

static PortalContext g_ctx;

static void on_portal_response_v2(GDBusConnection *conn, const gchar *sender,
                                  const gchar *path, const gchar *iface,
                                  const gchar *signal, GVariant *params,
                                  gpointer data) {
  uint32_t response;
  GVariant *results;
  g_variant_get(params, "(u@a{sv})", &response, &results);

  if (response != 0) {
    if (response != 1)
      std::cerr << "Portal error: " << response << std::endl;
    g_main_loop_quit(g_portal_loop);
    return;
  }

  if (g_portal_step == 1) {
    const gchar *handle = nullptr;
    if (g_variant_lookup(results, "session_handle", "&s", &handle)) {
      g_session_handle = handle;
    }
  } else if (g_portal_step == 3) {
    const gchar *rt = nullptr;
    if (g_variant_lookup(results, "restore_token", "&s", &rt)) {
      g_ctx.newToken = rt;
    }

    GVariant *stream_val =
        g_variant_lookup_value(results, "streams", G_VARIANT_TYPE("a(ua{sv})"));
    if (stream_val) {
      GVariantIter *iter;
      g_variant_get(stream_val, "a(ua{sv})", &iter);
      uint32_t node_id;
      GVariant *props;
      if (g_variant_iter_next(iter, "(u@a{sv})", &node_id, &props)) {
        g_node_id = node_id;
        g_variant_unref(props);
      }
      g_variant_iter_free(iter);
      g_variant_unref(stream_val);
    }
  }

  g_variant_unref(results);
  g_main_loop_quit(g_portal_loop);
}

PortalManager::PortalManager(const Config &config) : m_config(config) {}

PortalManager::~PortalManager() {
  if (m_pipewire_fd >= 0) {
    close(m_pipewire_fd);
    m_pipewire_fd = -1;
  }
}

std::string PortalManager::loadRestoreToken() {
  if (m_config.tokenDir.empty())
    return "";
  std::string path = m_config.tokenDir + "/restore_token";
  std::ifstream file(path);
  if (!file.is_open())
    return "";
  std::string token;
  std::getline(file, token);
  return token;
}

void PortalManager::saveRestoreToken(const std::string &token) {
  if (m_config.tokenDir.empty() || token.empty())
    return;

  // mkdir -p
  std::string cmd = "mkdir -p " + m_config.tokenDir;
  system(cmd.c_str());

  std::string path = m_config.tokenDir + "/restore_token";
  std::ofstream file(path);
  if (file.is_open()) {
    file << token;
  }
}

void PortalManager::clearRestoreToken() {
  if (m_config.tokenDir.empty())
    return;
  std::string path = m_config.tokenDir + "/restore_token";
  unlink(path.c_str());
}

bool PortalManager::init() { return setupPortal(); }

bool PortalManager::setupPortal() {
  GError *error = NULL;
  GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
  if (!conn) {
    std::cerr << "DBus connection failed" << std::endl;
    return false;
  }

  srand(time(NULL) ^ getpid());
  const char *dbus_unique_name = g_dbus_connection_get_unique_name(conn);
  std::string sender_clean =
      (dbus_unique_name[0] == ':') ? (dbus_unique_name + 1) : dbus_unique_name;
  for (char &c : sender_clean)
    if (c == '.')
      c = '_';

  g_portal_loop = g_main_loop_new(NULL, FALSE);
  g_ctx.tokenPath = m_config.tokenDir; // Not strictly needed inside callback
                                       // anymore due to flow change

  auto call_portal_step = [&](const char *method, GVariant *params,
                              const char *token) -> bool {
    char request_path[256];
    snprintf(request_path, sizeof(request_path),
             "/org/freedesktop/portal/desktop/request/%s/%s",
             sender_clean.c_str(), token);

    guint sub_id = g_dbus_connection_signal_subscribe(
        conn, "org.freedesktop.portal.Desktop",
        "org.freedesktop.portal.Request", "Response", request_path, NULL,
        G_DBUS_SIGNAL_FLAGS_NONE, on_portal_response_v2, NULL, NULL);

    GVariant *res = g_dbus_connection_call_sync(
        conn, "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop", "org.freedesktop.portal.ScreenCast",
        method, params, G_VARIANT_TYPE("(o)"), G_DBUS_CALL_FLAGS_NONE, -1, NULL,
        &error);

    if (!res) {
      std::cerr << "Method " << method
                << " failed: " << (error ? error->message : "unknown")
                << std::endl;
      g_dbus_connection_signal_unsubscribe(conn, sub_id);
      return false;
    }
    g_variant_unref(res);

    g_main_loop_run(g_portal_loop);
    g_dbus_connection_signal_unsubscribe(conn, sub_id);
    return true;
  };

  // 1. CreateSession
  g_portal_step = 1;
  char token1[32];
  snprintf(token1, sizeof(token1), "ph_c%d", rand() % 1000);

  auto buildCreateSession = [&](const std::string &restoreToken) {
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&b, "{sv}", "session_handle_token",
                          g_variant_new_string(token1));
    g_variant_builder_add(&b, "{sv}", "handle_token",
                          g_variant_new_string(token1));
    g_variant_builder_add(&b, "{sv}", "app_id",
                          g_variant_new_string(m_config.appId.c_str()));
    if (!restoreToken.empty()) {
      g_variant_builder_add(&b, "{sv}", "restore_token",
                            g_variant_new_string(restoreToken.c_str()));
    }
    return g_variant_new("(@a{sv})", g_variant_builder_end(&b));
  };

  std::string restore_token = loadRestoreToken();
  bool used_restore = !restore_token.empty();

  if (!call_portal_step("CreateSession", buildCreateSession(restore_token),
                        token1)) {
    if (used_restore) {
      // Retry without token
      std::cerr << "CreateSession failed with token, retrying..." << std::endl;
      clearRestoreToken();
      used_restore = false;
      if (!call_portal_step("CreateSession", buildCreateSession(""), token1))
        return false;
    } else {
      return false;
    }
  }

  if (g_session_handle.empty())
    return false;
  m_session_handle = g_session_handle;

  // 2. SelectSources
  // ALWAYS required, even for restored sessions (restored token acts as hint)
  g_portal_step = 2;
  char token2[32];
  snprintf(token2, sizeof(token2), "ph_s%d", rand() % 1000);
  GVariantBuilder b2;
  g_variant_builder_init(&b2, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&b2, "{sv}", "handle_token",
                        g_variant_new_string(token2));
  g_variant_builder_add(&b2, "{sv}", "types",
                        g_variant_new_uint32(1)); // 1 = Screen?
  g_variant_builder_add(&b2, "{sv}", "persist_mode",
                        g_variant_new_uint32(2)); // Persist

  if (!call_portal_step("SelectSources",
                        g_variant_new("(o@a{sv})", g_session_handle.c_str(),
                                      g_variant_builder_end(&b2)),
                        token2)) {
    return false;
  }

  // 3. Start
  g_portal_step = 3;
  g_ctx.newToken = ""; // Reset
  char token3[32];
  snprintf(token3, sizeof(token3), "ph_t%d", rand() % 1000);
  GVariantBuilder b3;
  g_variant_builder_init(&b3, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&b3, "{sv}", "handle_token",
                        g_variant_new_string(token3));

  if (!call_portal_step("Start",
                        g_variant_new("(os@a{sv})", g_session_handle.c_str(),
                                      "", g_variant_builder_end(&b3)),
                        token3)) {
    if (used_restore) {
      std::cerr << "Start failed with restored session, retrying clean..."
                << std::endl;
      clearRestoreToken();
      g_session_handle = "";
      return setupPortal(); // Recurse
    }
    return false;
  }

  if (g_node_id == 0)
    return false;
  m_node_id = g_node_id;

  // Save new token if present
  if (!g_ctx.newToken.empty()) {
    saveRestoreToken(g_ctx.newToken);
  }

  // 4. OpenPipeWireRemote
  GVariantBuilder b4;
  g_variant_builder_init(&b4, G_VARIANT_TYPE("a{sv}"));
  GUnixFDList *fd_list = NULL;
  GVariant *res = g_dbus_connection_call_with_unix_fd_list_sync(
      conn, "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
      "org.freedesktop.portal.ScreenCast", "OpenPipeWireRemote",
      g_variant_new("(o@a{sv})", g_session_handle.c_str(),
                    g_variant_builder_end(&b4)),
      G_VARIANT_TYPE("(h)"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &fd_list, NULL,
      &error);

  if (res && fd_list) {
    int32_t handle_index;
    g_variant_get(res, "(h)", &handle_index);
    m_pipewire_fd = g_unix_fd_list_get(fd_list, handle_index, NULL);
    g_object_unref(fd_list);
    g_variant_unref(res);
  } else {
    if (res)
      g_variant_unref(res);
    return false;
  }

  m_active = true;
  if (g_portal_loop) {
    g_main_loop_unref(g_portal_loop);
    g_portal_loop = nullptr;
  }
  return true;
}
