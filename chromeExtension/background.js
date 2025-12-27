// Background service worker for tracking active tab
// Uses Native Messaging for high performance and reliability
const NATIVE_HOST_NAME = "com.automatelinux.tabtracker";

let nativePort = null;

function connectToNativeHost() {
    if (nativePort) {
        return;
    }

    console.log("[AutomateLinux] Connecting to native host...");
    nativePort = chrome.runtime.connectNative(NATIVE_HOST_NAME);

    nativePort.onMessage.addListener((message) => {
        console.log("[AutomateLinux] Received from native host:", message);
        if (message.action === "focusChatGPT") {
            focusChatGPTTextarea();
        }
    });

    nativePort.onDisconnect.addListener(() => {
        console.log("[AutomateLinux] Native host disconnected:", chrome.runtime.lastError);
        nativePort = null;
        // Attempt to reconnect after a delay
        setTimeout(connectToNativeHost, 5000);
    });
}

// Focus ChatGPT textarea by injecting script
async function focusChatGPTTextarea() {
    try {
        const [activeTab] = await chrome.tabs.query({ active: true, currentWindow: true });

        if (activeTab && activeTab.url && activeTab.url.includes("chatgpt.com")) {
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

            // If we successfully focused, send an acknowledgment back immediately
            if (results && results[0] && results[0].result) {
                console.log("[AutomateLinux] Focus successful/received, result:", results[0].result);
                sendToNativeHost({ action: "focusAck" });
            }
        }
    } catch (error) {
        console.error("[AutomateLinux] Error focusing ChatGPT:", error);
    }
}

// Send message to Native Host
function sendToNativeHost(message) {
    if (!nativePort) {
        connectToNativeHost();
    }

    if (nativePort) {
        try {
            nativePort.postMessage(message);
        } catch (error) {
            console.error("[AutomateLinux] Error sending to native host:", error);
        }
    } else {
        console.error("[AutomateLinux] Cannot send message: no native port");
    }
}

// Update the active tab URL
async function getAndSendActiveTabUrl() {
    try {
        const [activeTab] = await chrome.tabs.query({ active: true, currentWindow: true });

        if (activeTab && activeTab.url) {
            console.log("[AutomateLinux] Active tab URL:", activeTab.url);
            sendToNativeHost({ url: activeTab.url });
        }
    } catch (error) {
        console.error("[AutomateLinux] Error getting active tab:", error);
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
