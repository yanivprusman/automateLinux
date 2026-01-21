// Function to remove ad elements
function removeAds() {
  // Remove Google auto-placed divs
  document.querySelectorAll('.google-auto-placed').forEach(el => el.remove());

  // Remove all iframes likely to be ads (commonly from Google/DoubleClick)
  document.querySelectorAll('iframe').forEach(iframe => {
    const src = iframe.src || "";
    if (/doubleclick\.net|googlesyndication\.com|googleads\.g\.doubleclick\.net/.test(src)) {
      iframe.remove();
    }
  });
}

// Run once on page load
removeAds();

// Observe dynamically added content
const observer = new MutationObserver(removeAds);
observer.observe(document.body, { childList: true, subtree: true });
