import json
import os
import shutil

prefs_path = "/home/yaniv/coding/automateLinux/chrome/Default/Preferences"
backup_path = prefs_path + ".bak"

# 1. Backup
print(f"Backing up Preferences to {backup_path}...")
shutil.copy2(prefs_path, backup_path)

# 2. Load
with open(prefs_path, "r") as f:
    data = json.load(f)

# 3. Modify
extensions = data.get("extensions", {}).get("settings", {})
ids_to_remove = ["jffpflfepnpobcokpeodbmjlfjnbciif", "oidmgaidfdknnjomccocciajgbfhdjdj", "oabljmmfgfgbpblfgllkjoaejpmfmhff"]
path_to_clean = "/home/yaniv/coding/automateLinux/chromeExtension"

removed_ids = []
for ext_id in list(extensions.keys()):
    settings = extensions[ext_id]
    if ext_id in ids_to_remove or (isinstance(settings, dict) and settings.get("path") == path_to_clean):
        del extensions[ext_id]
        removed_ids.append(ext_id)

print(f"Removed extension IDs: {removed_ids}")

# 4. Save
with open(prefs_path, "w") as f:
    json.dump(data, f)

print("Preferences fixed successfully.")
