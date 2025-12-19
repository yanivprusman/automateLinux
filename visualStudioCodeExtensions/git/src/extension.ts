import * as vscode from 'vscode';
import * as path from 'path';
export function activate(context: vscode.ExtensionContext) {
	console.log('Congratulations, your extension "git" is now active!');
	const activeFileProvider = new ActiveFileProvider();
	vscode.window.createTreeView('activeFileView', { treeDataProvider: activeFileProvider });
	context.subscriptions.push(vscode.commands.registerCommand('git.showActiveFile', () => {
		activeFileProvider.refresh();
	}));
	context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(() => {
		activeFileProvider.refresh();
	}));
	activeFileProvider.refresh();
}

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
			return Promise.resolve([]); 
		} else {
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

