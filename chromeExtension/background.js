// Background service worker for tracking active tab
// Sends URL to daemon via native messaging

let currentTabUrl = "";

// Send URL to native messaging host (which forwards to daemon)
async function updateActiveTabUrl(url) {
    currentTabUrl = url;
    console.log("[AutomateLinux] Active tab URL:", url);

    try {
        const response = await chrome.runtime.sendNativeMessage(
            'com.automatelinux.tabtracker',
            { url: url }
        );
        console.log("[AutomateLinux] Sent to daemon:", response);
    } catch (error) {
        // Native messaging might not be set up yet
        console.log("[AutomateLinux] Native messaging error (expected on first run):", error.message);
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
        // Check if this is the active tab
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

console.log("[AutomateLinux] Tab tracker extension loaded");
