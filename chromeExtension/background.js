// Background service worker for tracking active tab
// Uses Native Messaging for high performance and reliability
const NATIVE_HOST_NAME = "com.automatelinux.tabtracker";

let nativePort = null;
let isNativeConnecting = false;
let messageSeq = 0; // Track message sequence for debugging
let stats = {
    messagesSent: 0,
    messagesReceived: 0,
    reconnects: 0,
    lastConnectTime: null,
    lastMessageTime: null
};

function getTimestamp() {
    return new Date().toISOString();
}

function connectToNativeHost() {
    if (nativePort || isNativeConnecting) {
        return;
    }

    isNativeConnecting = true;
    const connectTime = getTimestamp();
    console.log(`[${connectTime}] [AutomateLinux] Connecting to native host...`);
    try {
        const port = chrome.runtime.connectNative(NATIVE_HOST_NAME);

        port.onMessage.addListener((message) => {
            stats.messagesReceived++;
            stats.lastMessageTime = getTimestamp();
            console.log(`[${stats.lastMessageTime}] [AutomateLinux] Received from native host:`, message);
            if (message.action === "focusChatGPT") {
                focusChatGPTTextarea();
            }
        });

        port.onDisconnect.addListener(() => {
            const error = chrome.runtime.lastError ? chrome.runtime.lastError.message : "Unknown error";
            const disconnectTime = getTimestamp();
            console.warn(`[${disconnectTime}] [AutomateLinux] Native host disconnected:`, error);
            console.log(`[${disconnectTime}] [AutomateLinux] Stats:`, stats);
            nativePort = null;
            isNativeConnecting = false;
            stats.reconnects++;

            // Retry connection lazily
            setTimeout(connectToNativeHost, 5000);
        });

        nativePort = port;
        isNativeConnecting = false;
        stats.lastConnectTime = connectTime;
        console.log(`[${connectTime}] [AutomateLinux] Native host connected`);

        // CRITICAL: Register with daemon immediately
        nativePort.postMessage({ command: "registerNativeHost", timestamp: connectTime, seq: messageSeq++ });
        stats.messagesSent++;
        console.log(`[${connectTime}] [AutomateLinux] Sent registration to daemon (seq: ${messageSeq - 1})`);
    } catch (e) {
        console.error(`[${getTimestamp()}] [AutomateLinux] Connection error:`, e);
        isNativeConnecting = false;
    }
}

// Focus ChatGPT textarea by injecting script
async function focusChatGPTTextarea() {
    const startTime = Date.now();
    const timestamp = getTimestamp();
    try {
        const [activeTab] = await chrome.tabs.query({ active: true, currentWindow: true });

        // Always send ACK - either success or a notification that we're not on ChatGPT
        if (activeTab && activeTab.url && activeTab.url.includes("chatgpt.com")) {
            console.log(`[${timestamp}] [AutomateLinux] On chatgpt.com, attempting focus...`);
            const results = await chrome.scripting.executeScript({
                target: { tabId: activeTab.id },
                func: () => {
                    const textarea = document.querySelector('#prompt-textarea') || document.querySelector('div[contenteditable="true"]');
                    if (textarea) {
                        textarea.focus();
                        console.log("[AutomateLinux] Focused ChatGPT textarea");
                        return true;
                    }
                    return false;
                }
            });

            const elapsed = Date.now() - startTime;
            // Send ACK with success status
            if (results && results[0] && results[0].result) {
                console.log(`[${getTimestamp()}] [AutomateLinux] Focus successful (${elapsed}ms), sending ACK`);
                sendToNativeHost({ action: "focusAck", success: true, elapsed });
            } else {
                console.log(`[${getTimestamp()}] [AutomateLinux] Focus failed (${elapsed}ms), sending ACK`);
                sendToNativeHost({ action: "focusAck", success: false, elapsed });
            }
        } else {
            // Not on ChatGPT - send ACK immediately so daemon doesn't timeout
            const url = activeTab?.url || "unknown";
            const elapsed = Date.now() - startTime;
            console.log(`[${timestamp}] [AutomateLinux] Not on chatgpt.com (on ${url}), sending immediate ACK (${elapsed}ms)`);
            sendToNativeHost({ action: "focusAck", success: false, notChatGPT: true, url, elapsed });
        }
    } catch (error) {
        const elapsed = Date.now() - startTime;
        console.error(`[${getTimestamp()}] [AutomateLinux] Error focusing ChatGPT (${elapsed}ms):`, error);
        // Send ACK even on error
        sendToNativeHost({ action: "focusAck", success: false, error: error.message, elapsed });
    }
}

// Send message to Native Host
function sendToNativeHost(message) {
    if (!nativePort) {
        connectToNativeHost();
    }

    if (nativePort) {
        try {
            // Add metadata to all messages
            const enrichedMessage = {
                ...message,
                timestamp: getTimestamp(),
                seq: messageSeq++
            };
            nativePort.postMessage(enrichedMessage);
            stats.messagesSent++;
            stats.lastMessageTime = enrichedMessage.timestamp;
            console.log(`[${enrichedMessage.timestamp}] [AutomateLinux] Sent (seq ${enrichedMessage.seq}):`, message);
        } catch (error) {
            console.error(`[${getTimestamp()}] [AutomateLinux] Error sending to native host:`, error);
        }
    } else {
        console.error(`[${getTimestamp()}] [AutomateLinux] Cannot send message: no native port`);
    }
}

// Update the active tab URL
async function getAndSendActiveTabUrl() {
    try {
        const [activeTab] = await chrome.tabs.query({ active: true, currentWindow: true });

        if (activeTab && activeTab.url) {
            console.log(`[${getTimestamp()}] [AutomateLinux] Active tab URL:`, activeTab.url);
            sendToNativeHost({ url: activeTab.url });
        }
    } catch (error) {
        console.error(`[${getTimestamp()}] [AutomateLinux] Error getting active tab:`, error);
    }
}

// Listen for tab activation (user switches tabs)
chrome.tabs.onActivated.addListener(async (activeInfo) => {
    await getAndSendActiveTabUrl();
});

// Listen for tab updates (URL changes in current tab)
chrome.tabs.onUpdated.addListener(async (tabId, changeInfo, tab) => {
    if (changeInfo.url) {
        const [activeTab] = await chrome.tabs.query({ active: true, currentWindow: true });
        if (activeTab && activeTab.id === tabId) {
            await getAndSendActiveTabUrl();
        }
    }
});

// Listen for window focus changes  
chrome.windows.onFocusChanged.addListener(async (windowId) => {
    if (windowId !== chrome.windows.WINDOW_ID_NONE) {
        await getAndSendActiveTabUrl();
    }
});

// Initialize on startup
chrome.runtime.onStartup.addListener(async () => {
    await getAndSendActiveTabUrl();
});

// Initialize on install/update
chrome.runtime.onInstalled.addListener(async () => {
    await getAndSendActiveTabUrl();
});

console.log("[AutomateLinux] Tab tracker extension loaded (Native Messaging mode)");
connectToNativeHost();
getAndSendActiveTabUrl();
