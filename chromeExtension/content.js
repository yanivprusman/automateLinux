// Content script for ChatGPT - handles focus requests
console.log("[AutomateLinux] ChatGPT content script loaded");

// Listen for focus requests from background script
chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
    if (message.action === "focusTextBox") {
        console.log("[AutomateLinux] Received focus request");

        // ChatGPT uses #prompt-textarea for the main input
        const textBox = document.querySelector('#prompt-textarea');

        if (textBox) {
            textBox.focus();
            console.log("[AutomateLinux] Text box focused successfully");
            sendResponse({ success: true });
        } else {
            console.log("[AutomateLinux] Text box not found");
            sendResponse({ success: false, error: "Text box #prompt-textarea not found" });
        }
    }
    return true; // Keep channel open for async response
});
