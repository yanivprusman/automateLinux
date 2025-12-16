import * as vscode from 'vscode';
import { exec } from 'child_process';
import { promisify } from 'util';

const execAsync = promisify(exec);
export function activate(context: vscode.ExtensionContext) {
    const disposable = vscode.commands.registerCommand('daemon.getDaemonPID', async () => {
        try {
            const automateDir = process.env.AUTOMATE_LINUX_DIR;
            if (!automateDir) {
                vscode.window.showErrorMessage('AUTOMATE_LINUX_DIR is not set');
                return;
            }
            const scriptPath = `${automateDir}/daemon/getDaemonPID.sh`;
            const { stdout } = await execAsync(scriptPath);
            const pid = stdout.trim();
            if (!pid) {
                vscode.window.showErrorMessage('Daemon is not running');
                return;
            }
            vscode.window.showInformationMessage(`Daemon PID set to ${pid}`);
            return pid;
            // return parseInt(pid, 10);  
        } catch (err) {
            vscode.window.showErrorMessage(`Failed to get daemon PID: ${err}`);
            return undefined;
        }
    });
    context.subscriptions.push(disposable);
}

export function deactivate() {}