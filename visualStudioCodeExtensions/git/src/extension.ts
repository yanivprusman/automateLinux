// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import * as vscode from 'vscode';
import * as path from 'path';

// This method is called when your extension is activated
// Your extension is activated the very first time the command is executed
export function activate(context: vscode.ExtensionContext) {

	// Use the console to output diagnostic information (console.log) and errors (console.error)
	// This line of code will only be executed once when your extension is activated
	console.log('Congratulations, your extension "git" is now active!');

	const activeFileProvider = new ActiveFileProvider();
	vscode.window.createTreeView('activeFileView', { treeDataProvider: activeFileProvider });

	context.subscriptions.push(vscode.commands.registerCommand('git.showActiveFile', () => {
		activeFileProvider.refresh();
	}));

	context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(() => {
		activeFileProvider.refresh();
	}));

	// Initial refresh to show the active file when the extension activates
	activeFileProvider.refresh();
}

// This method is called when your extension is deactivated
export function deactivate() {}

class ActiveFileItem extends vscode.TreeItem {
	constructor(
		public readonly label: string,
		public readonly resourceUri?: vscode.Uri,
		public readonly collapsibleState: vscode.TreeItemCollapsibleState = vscode.TreeItemCollapsibleState.None
	) {
		super(label, collapsibleState);
		if (resourceUri) {
			this.tooltip = resourceUri.fsPath;
			this.description = path.basename(resourceUri.fsPath);
		}
	}
}

class ActiveFileProvider implements vscode.TreeDataProvider<ActiveFileItem> {
	private _onDidChangeTreeData: vscode.EventEmitter<ActiveFileItem | undefined | void> = new vscode.EventEmitter<ActiveFileItem | undefined | void>();
	readonly onDidChangeTreeData: vscode.Event<ActiveFileItem | undefined | void> = this._onDidChangeTreeData.event;

	refresh(): void {
		this._onDidChangeTreeData.fire();
	}

	getTreeItem(element: ActiveFileItem): vscode.TreeItem {
		return element;
	}

	getChildren(element?: ActiveFileItem): Thenable<ActiveFileItem[]> {
		if (element) {
			return Promise.resolve([]); // No children for file items
		} else {
			// Root of the tree, show the active file
			const editor = vscode.window.activeTextEditor;
			if (editor && editor.document.uri.scheme === 'file') {
				const filePath = editor.document.uri;
				return Promise.resolve([new ActiveFileItem(path.basename(filePath.fsPath), filePath)]);
			} else {
				return Promise.resolve([new ActiveFileItem('No active file')]);
			}
		}
	}
}

