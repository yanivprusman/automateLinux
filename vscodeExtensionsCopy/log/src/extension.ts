import * as vscode from 'vscode';
import * as fs from 'fs';
import * as path from 'path';
export function activate(context: vscode.ExtensionContext) {
    console.log('Your extension "log" is now active!');
    const logFilePath = '/home/yaniv/coding/automateLinux/data/combined.log';
    vscode.workspace.openTextDocument(logFilePath).then(doc => {
        vscode.window.showTextDocument(doc, { preview: false });
        // const watcher = fs.watch(logFilePath, (eventType) => {
        //     if (eventType === 'change') {
        //         vscode.workspace.openTextDocument(logFilePath).then(newDoc => {
        //             const activeEditor = vscode.window.activeTextEditor;
        //             if (activeEditor && activeEditor.document.uri.fsPath === logFilePath) {
        //                 const edit = new vscode.WorkspaceEdit();
        //                 const fullRange = new vscode.Range(
        //                     newDoc.positionAt(0),
        //                     newDoc.positionAt(doc.getText().length)
        //                 );
        //                 edit.replace(doc.uri, fullRange, newDoc.getText());
        //                 vscode.workspace.applyEdit(edit);
        //             }
        //         });
        //     }
        // });
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
}
export function deactivate() {}
