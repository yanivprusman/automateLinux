#!/usr/bin/env node

import { Server } from "@modelcontextprotocol/sdk/server/index.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import {
  CallToolRequestSchema,
  ListToolsRequestSchema,
} from "@modelcontextprotocol/sdk/types.js";
import * as net from "node:net";

const UDS_PATH = "/run/automatelinux/automatelinux-daemon.sock";
const TIMEOUT_MS = 5000;

// Send command to daemon via UNIX socket
async function sendToDaemon(commandObj: Record<string, unknown>): Promise<string> {
  return new Promise((resolve, reject) => {
    const client = net.createConnection(UDS_PATH);
    let response = "";

    const timeout = setTimeout(() => {
      client.destroy();
      reject(new Error("Daemon connection timeout"));
    }, TIMEOUT_MS);

    client.on("connect", () => {
      client.write(JSON.stringify(commandObj) + "\n");
    });

    client.on("data", (data) => {
      response += data.toString();
      if (response.endsWith("\n")) {
        clearTimeout(timeout);
        client.destroy();
        resolve(response.trim());
      }
    });

    client.on("error", (err) => {
      clearTimeout(timeout);
      reject(err);
    });
  });
}

// Tool definitions for the daemon
const DAEMON_TOOLS = [
  // System tools
  {
    name: "daemon_ping",
    description: "Ping the daemon to check if it's running",
    inputSchema: { type: "object" as const, properties: {} },
    command: "ping",
    args: [],
  },
  {
    name: "daemon_version",
    description: "Get the daemon version (git commit count)",
    inputSchema: { type: "object" as const, properties: {} },
    command: "version",
    args: [],
  },
  {
    name: "daemon_help",
    description: "Get help and list of available daemon commands",
    inputSchema: { type: "object" as const, properties: {} },
    command: "help",
    args: [],
  },
  {
    name: "daemon_list_commands",
    description: "List all available daemon command names",
    inputSchema: { type: "object" as const, properties: {} },
    command: "listCommands",
    args: [],
  },

  // Port management
  {
    name: "daemon_list_ports",
    description: "List all port assignments managed by the daemon",
    inputSchema: { type: "object" as const, properties: {} },
    command: "listPorts",
    args: [],
  },
  {
    name: "daemon_get_port",
    description: "Get the assigned port for a specific app/service",
    inputSchema: {
      type: "object" as const,
      properties: {
        key: { type: "string", description: "App/service name (e.g., 'cad-dev', 'dashboard-bridge')" },
      },
      required: ["key"],
    },
    command: "getPort",
    args: ["key"],
  },
  {
    name: "daemon_set_port",
    description: "Assign a port to an app/service",
    inputSchema: {
      type: "object" as const,
      properties: {
        key: { type: "string", description: "App/service name" },
        value: { type: "number", description: "Port number to assign" },
      },
      required: ["key", "value"],
    },
    command: "setPort",
    args: ["key", "value"],
  },

  // App management
  {
    name: "daemon_list_apps",
    description: "List all registered apps",
    inputSchema: { type: "object" as const, properties: {} },
    command: "listApps",
    args: [],
  },
  {
    name: "daemon_app_status",
    description: "Get status of all apps or a specific app",
    inputSchema: {
      type: "object" as const,
      properties: {
        app: { type: "string", description: "Optional: specific app name (e.g., 'cad', 'pt')" },
      },
    },
    command: "appStatus",
    args: ["app"],
  },
  {
    name: "daemon_start_app",
    description: "Start an app in dev or prod mode",
    inputSchema: {
      type: "object" as const,
      properties: {
        app: { type: "string", description: "App name (e.g., 'cad', 'pt')" },
        mode: { type: "string", enum: ["dev", "prod"], description: "Mode to start (dev or prod)" },
      },
      required: ["app", "mode"],
    },
    command: "startApp",
    args: ["app", "mode"],
  },
  {
    name: "daemon_stop_app",
    description: "Stop an app",
    inputSchema: {
      type: "object" as const,
      properties: {
        app: { type: "string", description: "App name" },
        mode: { type: "string", enum: ["dev", "prod", "all"], description: "Mode to stop (dev, prod, or all)" },
      },
      required: ["app"],
    },
    command: "stopApp",
    args: ["app", "mode"],
  },
  {
    name: "daemon_restart_app",
    description: "Restart an app",
    inputSchema: {
      type: "object" as const,
      properties: {
        app: { type: "string", description: "App name" },
        mode: { type: "string", enum: ["dev", "prod"], description: "Mode to restart" },
      },
      required: ["app", "mode"],
    },
    command: "restartApp",
    args: ["app", "mode"],
  },
  {
    name: "daemon_build_app",
    description: "Build an app's C++ server component",
    inputSchema: {
      type: "object" as const,
      properties: {
        app: { type: "string", description: "App name" },
        mode: { type: "string", enum: ["dev", "prod"], description: "Build mode" },
      },
      required: ["app", "mode"],
    },
    command: "buildApp",
    args: ["app", "mode"],
  },
  {
    name: "daemon_deploy_to_prod",
    description: "Deploy dev changes to prod worktree",
    inputSchema: {
      type: "object" as const,
      properties: {
        app: { type: "string", description: "App name" },
        commit: { type: "string", description: "Optional: specific commit hash (defaults to latest dev)" },
      },
      required: ["app"],
    },
    command: "deployToProd",
    args: ["app", "commit"],
  },
  {
    name: "daemon_prod_status",
    description: "Check if prod worktree is clean or has uncommitted changes",
    inputSchema: {
      type: "object" as const,
      properties: {
        app: { type: "string", description: "App name" },
      },
      required: ["app"],
    },
    command: "prodStatus",
    args: ["app"],
  },

  // Database
  {
    name: "daemon_show_db",
    description: "Show database summary (terminal history, devices, settings)",
    inputSchema: { type: "object" as const, properties: {} },
    command: "showDB",
    args: [],
  },
  {
    name: "daemon_get_entry",
    description: "Get a setting entry by key from the database",
    inputSchema: {
      type: "object" as const,
      properties: {
        key: { type: "string", description: "Setting key to retrieve" },
      },
      required: ["key"],
    },
    command: "getEntry",
    args: ["key"],
  },
  {
    name: "daemon_upsert_entry",
    description: "Insert or update a setting entry in the database",
    inputSchema: {
      type: "object" as const,
      properties: {
        key: { type: "string", description: "Setting key" },
        value: { type: "string", description: "Setting value" },
      },
      required: ["key", "value"],
    },
    command: "upsertEntry",
    args: ["key", "value"],
  },

  // Input control
  {
    name: "daemon_enable_keyboard",
    description: "Enable keyboard input grabbing (daemon takes control of keyboard)",
    inputSchema: { type: "object" as const, properties: {} },
    command: "enableKeyboard",
    args: [],
  },
  {
    name: "daemon_disable_keyboard",
    description: "Disable keyboard input grabbing (release keyboard control)",
    inputSchema: { type: "object" as const, properties: {} },
    command: "disableKeyboard",
    args: [],
  },
  {
    name: "daemon_simulate_input",
    description: "Simulate keyboard input - type text or send raw key events",
    inputSchema: {
      type: "object" as const,
      properties: {
        string: { type: "string", description: "Text to type (simulates keypresses)" },
        type: { type: "number", description: "Event type (1=EV_KEY) for raw events" },
        code: { type: "number", description: "Key code for raw events" },
        value: { type: "number", description: "Key value (1=press, 0=release) for raw events" },
      },
    },
    command: "simulateInput",
    args: ["string", "type", "code", "value"],
  },
  {
    name: "daemon_get_macros",
    description: "Get all configured keyboard macros (JSON)",
    inputSchema: { type: "object" as const, properties: {} },
    command: "getMacros",
    args: [],
  },

  // Window management
  {
    name: "daemon_get_active_context",
    description: "Get current active window context (title, class, URL if browser)",
    inputSchema: { type: "object" as const, properties: {} },
    command: "getActiveContext",
    args: [],
  },
  {
    name: "daemon_list_windows",
    description: "List all tracked windows",
    inputSchema: { type: "object" as const, properties: {} },
    command: "listWindows",
    args: [],
  },
  {
    name: "daemon_activate_window",
    description: "Activate (focus) a window by its ID",
    inputSchema: {
      type: "object" as const,
      properties: {
        windowId: { type: "string", description: "Window ID to activate" },
      },
      required: ["windowId"],
    },
    command: "activateWindow",
    args: ["windowId"],
  },

  // Peer networking
  {
    name: "daemon_get_peer_status",
    description: "Show current peer configuration and connection status",
    inputSchema: { type: "object" as const, properties: {} },
    command: "getPeerStatus",
    args: [],
  },
  {
    name: "daemon_list_peers",
    description: "List all registered peers in the network",
    inputSchema: { type: "object" as const, properties: {} },
    command: "listPeers",
    args: [],
  },
  {
    name: "daemon_get_peer_info",
    description: "Get detailed info about a specific peer",
    inputSchema: {
      type: "object" as const,
      properties: {
        peer: { type: "string", description: "Peer ID (e.g., 'vps', 'desktop', 'laptop')" },
      },
      required: ["peer"],
    },
    command: "getPeerInfo",
    args: ["peer"],
  },
  {
    name: "daemon_exec_on_peer",
    description: "Execute a shell command on a remote peer",
    inputSchema: {
      type: "object" as const,
      properties: {
        peer: { type: "string", description: "Peer ID to execute on" },
        directory: { type: "string", description: "Working directory for the command" },
        shellCmd: { type: "string", description: "Shell command to execute" },
      },
      required: ["peer", "directory", "shellCmd"],
    },
    command: "execOnPeer",
    args: ["peer", "directory", "shellCmd"],
  },
  {
    name: "daemon_remote_pull",
    description: "Git pull automateLinux on a remote peer",
    inputSchema: {
      type: "object" as const,
      properties: {
        peer: { type: "string", description: "Peer ID" },
      },
      required: ["peer"],
    },
    command: "remotePull",
    args: ["peer"],
  },
  {
    name: "daemon_remote_bd",
    description: "Build daemon on a remote peer",
    inputSchema: {
      type: "object" as const,
      properties: {
        peer: { type: "string", description: "Peer ID" },
      },
      required: ["peer"],
    },
    command: "remoteBd",
    args: ["peer"],
  },
  {
    name: "daemon_remote_deploy_daemon",
    description: "Pull and build daemon on a remote peer",
    inputSchema: {
      type: "object" as const,
      properties: {
        peer: { type: "string", description: "Peer ID" },
      },
      required: ["peer"],
    },
    command: "remoteDeployDaemon",
    args: ["peer"],
  },

  // Generic command for any daemon command
  {
    name: "daemon_send_command",
    description: "Send any command to the daemon (for commands not covered by specific tools)",
    inputSchema: {
      type: "object" as const,
      properties: {
        command: { type: "string", description: "Command name (e.g., 'ping', 'listPorts')" },
        args: {
          type: "object",
          description: "Optional arguments as key-value pairs",
          additionalProperties: true,
        },
      },
      required: ["command"],
    },
    command: null, // Special handling
    args: [],
  },
];

// Create MCP server
const server = new Server(
  {
    name: "automatelinux-daemon",
    version: "1.0.0",
  },
  {
    capabilities: {
      tools: {},
    },
  }
);

// List available tools
server.setRequestHandler(ListToolsRequestSchema, async () => {
  return {
    tools: DAEMON_TOOLS.map((tool) => ({
      name: tool.name,
      description: tool.description,
      inputSchema: tool.inputSchema,
    })),
  };
});

// Handle tool calls
server.setRequestHandler(CallToolRequestSchema, async (request) => {
  const { name, arguments: args } = request.params;

  try {
    // Find the tool definition
    const tool = DAEMON_TOOLS.find((t) => t.name === name);
    if (!tool) {
      throw new Error(`Unknown tool: ${name}`);
    }

    // Build command object
    let commandObj: Record<string, unknown>;

    if (name === "daemon_send_command") {
      // Generic command handler
      const cmdArgs = args as { command: string; args?: Record<string, unknown> };
      commandObj = { command: cmdArgs.command, ...cmdArgs.args };
    } else {
      // Specific tool handler
      commandObj = { command: tool.command };
      const toolArgs = args as Record<string, unknown>;

      // Add arguments that have values
      for (const arg of tool.args) {
        if (toolArgs[arg] !== undefined && toolArgs[arg] !== null && toolArgs[arg] !== "") {
          commandObj[arg] = toolArgs[arg];
        }
      }
    }

    // Send to daemon
    const response = await sendToDaemon(commandObj);

    // Try to parse as JSON for nicer formatting
    let formattedResponse = response;
    try {
      const parsed = JSON.parse(response);
      formattedResponse = JSON.stringify(parsed, null, 2);
    } catch {
      // Keep as plain text if not JSON
    }

    return {
      content: [
        {
          type: "text",
          text: formattedResponse,
        },
      ],
    };
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    return {
      content: [
        {
          type: "text",
          text: `Error: ${message}`,
        },
      ],
      isError: true,
    };
  }
});

// Main entry point
async function main() {
  const transport = new StdioServerTransport();
  await server.connect(transport);
  console.error("automateLinux daemon MCP server running on stdio");
}

main().catch((error) => {
  console.error("Server error:", error);
  process.exit(1);
});
