import { exec } from 'child_process';
import * as path from 'path';

export async function findGitRepoRoot(filePath: string): Promise<string | null> {
    return new Promise((resolve) => {
        const dir = path.dirname(filePath);
        exec('git rev-parse --show-toplevel', { cwd: dir }, (error, stdout, stderr) => {
            if (error || stderr) {
                resolve(null);
            } else {
                resolve(stdout.trim());
            }
        });
    });
}
