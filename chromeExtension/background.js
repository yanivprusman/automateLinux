// Background service worker for tracking active tab
// Uses HTTP bridge instead of native messaging to bypass Chrome sandbox issues
const BRIDGE_URL = "http://localhost:9223/active-tab";
const EVENTS_URL = "http://localhost:9223/events";
const FOCUS_ACK_URL = "http://localhost:9223/focus-ack";

// Establish SSE connection for receiving commands
function connectToEvents() {
    const eventSource = new EventSource(EVENTS_URL);

    eventSource.onopen = () => {
        console.log("[AutomateLinux] Connected to SSE event stream");
    };

    eventSource.onmessage = (event) => {
        try {
            const message = JSON.parse(event.data);
            console.log("[AutomateLinux] Received event:", message);

            if (message.action === "focusChatGPT") {
                focusChatGPTTextarea();
            }
        } catch (error) {
            console.error("[AutomateLinux] Error parsing event:", error);
        }
    };

    eventSource.onerror = () => {
        // console.log("[AutomateLinux] SSE connection error (bridge might be down). Retrying...");
        eventSource.close();
        setTimeout(connectToEvents, 5000);
    };
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
                console.log("[AutomateLinux] Sending ack to:", FOCUS_ACK_URL);
                fetch(FOCUS_ACK_URL, {
                    method: 'POST',
                    mode: 'cors',
                    cache: 'no-cache'
                }).then(r => {
                    console.log("[AutomateLinux] Ack response:", r.status);
                }).catch(err => {
                    console.error("[AutomateLinux] Ack fetch failed:", err);
                });
            }
        }
    } catch (error) {
        console.error("[AutomateLinux] Error focusing ChatGPT:", error);
    }
}

// Send URL to HTTP bridge
async function updateActiveTabUrl(url) {
    console.log("[AutomateLinux] Active tab URL:", url);

    try {
        const response = await fetch(BRIDGE_URL, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ url: url })
        });
        if (!response.ok) {
            console.error("[AutomateLinux] Bridge response error:", response.status);
        }
    } catch (error) {
        console.error("[AutomateLinux] Bridge fetch failed:", error.message);
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
connectToEvents();
getAndSendActiveTabUrl();
