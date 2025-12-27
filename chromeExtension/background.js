// Background service worker for tracking active tab
// Uses HTTP bridge instead of native messaging to bypass Chrome sandbox issues
const BRIDGE_URL = "http://localhost:9223/active-tab";

// Send URL to HTTP bridge
async function updateActiveTabUrl(url) {
    console.log("[AutomateLinux] Active tab URL:", url);

    try {
        await fetch(BRIDGE_URL, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ url: url })
        });
    } catch (error) {
        // Suppress connection errors if bridge is down to avoid console spam
        // console.log("[AutomateLinux] Bridge error:", error.message);
    }
}

// Get and update the active tab URL
async function getAndSendActiveTabUrl() {
    try {
        const [activeTab] = await chrome.tabs.query({ active: true, currentWindow: true });

        if (activeTab && activeTab.url) {
            await updateActiveTabUrl(activeTab.url);
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

console.log("[AutomateLinux] Tab tracker extension loaded (HTTP mode)");
getAndSendActiveTabUrl();
