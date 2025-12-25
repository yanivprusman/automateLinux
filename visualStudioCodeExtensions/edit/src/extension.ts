import * as vscode from 'vscode';
export function activate(context: vscode.ExtensionContext) {
	const disposable = vscode.commands.registerCommand('edit.deleteEmptyRow', () => {
		const editor = vscode.window.activeTextEditor;
	});
	context.subscriptions.push(disposable);
}
export function deactivate() {}
