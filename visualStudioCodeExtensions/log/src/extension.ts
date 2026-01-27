import * as vscode from 'vscode';
import * as fs from 'fs';
import * as path from 'path';
// export function activate(context: vscode.ExtensionContext) {
//     console.log('Your extension "log" is now active!');
//     const logFilePath = '/opt/automateLinux/data/combined.log';
//     vscode.workspace.openTextDocument(logFilePath).then(doc => {
//         vscode.window.showTextDocument(doc, { preview: false });
// 		const watcher = fs.watch(logFilePath, (eventType) => {
// 			if (eventType === 'change') {
// 				const doc = vscode.workspace.textDocuments.find(d => d.uri.fsPath === logFilePath);
// 				if (doc) {
// 					fs.readFile(logFilePath, 'utf8', (err, data) => {
// 						if (!err) {
// 							const edit = new vscode.WorkspaceEdit();
// 							const fullRange = new vscode.Range(
// 								doc.positionAt(0),
// 								doc.positionAt(doc.getText().length)
// 							);
// 							edit.replace(doc.uri, fullRange, data);
// 							vscode.workspace.applyEdit(edit);
// 						}
// 					});
// 				}
// 			}
// 		});
//         context.subscriptions.push({ dispose: () => watcher.close() });
//     });
// }

// export function activate(context: vscode.ExtensionContext) {
//     const logFilePath = '/opt/automateLinux/data/combined.log';
//     vscode.workspace.openTextDocument(logFilePath).then(doc => {
//         vscode.window.showTextDocument(doc, { preview: false });
//     });
//     const watcher = vscode.workspace.createFileSystemWatcher(logFilePath);
//     context.subscriptions.push(watcher);
// }

export function activate(context: vscode.ExtensionContext) {
    const logFilePath = '/opt/automateLinux/data/combined.log';

    const disposable = vscode.commands.registerCommand('log.open', async () => {
        const doc = await vscode.workspace.openTextDocument(logFilePath);
        await vscode.window.showTextDocument(doc, { preview: false });

        const watcher = fs.watch(logFilePath, (eventType) => {
            if (eventType === 'change') {
                const doc = vscode.workspace.textDocuments.find(d => d.uri.fsPath === logFilePath);
                if (doc) {
                    fs.readFile(logFilePath, 'utf8', (err, data) => {
                        if (!err) {
                            const edit = new vscode.WorkspaceEdit();
                            const fullRange = new vscode.Range(
                                doc.positionAt(0),
                                doc.positionAt(doc.getText().length)
                            );
                            edit.replace(doc.uri, fullRange, data);
                            vscode.workspace.applyEdit(edit);
                        }
                    });
                }
            }
        });
        context.subscriptions.push({ dispose: () => watcher.close() });
    });

    context.subscriptions.push(disposable);
}

export function deactivate() {}

