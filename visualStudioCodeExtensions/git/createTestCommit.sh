#!/bin/bash
cd /tmp
rm -rf test-git-extension
mkdir test-git-extension
cd test-git-extension
git init
git config user.email "test@example.com"
git config user.name "Test User"
mkdir -p src config
echo "Initial content" > src/data.txt
git add src/data.txt
git commit -m "Initial: Add data.txt in src"
echo "Config content" > config/data.txt
git add config/data.txt
git commit -m "Add data.txt in config"
echo "Initial content - modified" >> src/data.txt
git add src/data.txt
git commit -m "Update src/data.txt"
echo "Config content - modified" >> config/data.txt
git add config/data.txt
git commit -m "Update config/data.txt"
rm src/data.txt
git add -A
git commit -m "Remove src/data.txt"
echo "Recreated content" > src/data.txt
git add src/data.txt
git commit -m "Recreate src/data.txt"
echo "Test repository created at /tmp/test-git-extension"
git log --oneline
