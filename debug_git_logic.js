const { exec } = require('child_process');
const path = require('path');

const filePath = '/opt/automateLinux/terminal/bindings.sh';
const repoRoot = '/opt/automateLinux';

function run() {
    exec(`git --no-pager log --pretty=format:'%h %ad %s' --date=short -- "${filePath}"`, { cwd: repoRoot }, (error, stdout, stderr) => {
        if (error) {
            console.error(error);
            return;
        }
        const lines = stdout.trim().split('\n');
        const commits = lines.map(line => {
            const parts = line.match(/^(\w+)\s+(\S+)\s+(.*)$/);
            if (parts && parts.length === 4) {
                const [, commitHash, authorDate, subject] = parts;
                return { commitHash, subject };
            }
            return null;
        }).filter(x => x);

        const allItems = [{ commitHash: 'CURRENT', subject: 'Current State' }, ...commits];

        const targetHash = '6208525';
        const currentIndex = allItems.findIndex(c => c.commitHash === targetHash);

        console.log(`Target: ${targetHash} found at index ${currentIndex}`);

        if (currentIndex === -1) {
            console.log("Target not found!");
            return;
        }

        const prevIndex = currentIndex + 1; // "Previous Commit" (Older)
        const nextIndex = currentIndex - 1; // "Next Commit" (Newer)

        console.log(`Previous (Index + 1): ${allItems[prevIndex] ? allItems[prevIndex].commitHash : 'NONE'} (${allItems[prevIndex]?.subject})`);
        console.log(`Next     (Index - 1): ${allItems[nextIndex] ? allItems[nextIndex].commitHash : 'NONE'} (${allItems[nextIndex]?.subject})`);
    });
}

run();
