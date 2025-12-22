import * as vscode from 'vscode';
import { exec } from 'child_process';
import { findGitRepoRoot } from './gitUtils';

export class CommitItem extends vscode.TreeItem {
	constructor(
		public readonly label: string,
		public readonly commitHash: string,
		public readonly authorDate: string,
		public readonly filePath: string,
		public readonly collapsibleState: vscode.TreeItemCollapsibleState = vscode.TreeItemCollapsibleState.None,
		public readonly isCurrentState: boolean = false,
		public readonly commitMessage: string = ''
	) {
		super(label, collapsibleState);
		if (isCurrentState) {
			this.description = '(current working state)';
			this.tooltip = `Current state of ${filePath}`;
			this.iconPath = new vscode.ThemeIcon('circle-filled');
		} else {
			this.description = `${authorDate} (${commitHash.substring(0, 7)})`;
			this.tooltip = `${label}\nHash: ${commitHash}\nDate: ${authorDate}\nFile: ${filePath}`;
			this.contextValue = 'commit';
		}
	}
}

export class ActiveFileCommitProvider implements vscode.TreeDataProvider<CommitItem> {
	private _onDidChangeTreeData: vscode.EventEmitter<CommitItem | undefined | void> = new vscode.EventEmitter<CommitItem | undefined | void>();
	readonly onDidChangeTreeData: vscode.Event<CommitItem | undefined | void> = this._onDidChangeTreeData.event;
	
	private currentDiffAnnotation: string = '';
	
	setCurrentDiffAnnotation(content: string): void {
		this.currentDiffAnnotation = content;
		this.refresh();
	}
	
	refresh(): void {
		this._onDidChangeTreeData.fire();
	}

	getTreeItem(element: CommitItem): vscode.TreeItem {
		return element;
	}

	async getChildren(element?: CommitItem): Promise<CommitItem[]> {
		if (element) {
			return [];
		}

		const editor = vscode.window.activeTextEditor;
		if (!editor || editor.document.uri.scheme !== 'file') {
			return [new CommitItem('No active file in a Git repository', '', '', '', vscode.TreeItemCollapsibleState.None)];
		}

		const filePath = editor.document.uri.fsPath;
		const repoRoot = await findGitRepoRoot(filePath);
		
		if (!repoRoot) {
			return [new CommitItem('File is not in a Git repository', '', '', '', vscode.TreeItemCollapsibleState.None)];
		}

		return new Promise((resolve) => {
			exec(`git --no-pager log --pretty=format:'%h %ad %s' --date=short -- "${filePath}"`, { cwd: repoRoot }, (error, stdout, stderr) => {
				if (error) {
					console.error(`Error executing git log: ${error.message}`);
					vscode.window.showErrorMessage(`Failed to get git log: ${error.message}`);
					resolve([new CommitItem('Error getting commit history', '', '', '', vscode.TreeItemCollapsibleState.None)]);
					return;
				}
				if (stderr) {
					console.warn(`Git log stderr: ${stderr}`);
				}

				const lines = stdout.trim().split('\n');
				if (lines.length === 0 || (lines.length === 1 && lines[0] === '')) {
					resolve([new CommitItem('No commit history found for this file', '', '', '', vscode.TreeItemCollapsibleState.None)]);
					return;
				}

				const commits = lines.map(line => {
					// Expected format: "%h %ad %s" -> hash date subject
					const parts = line.match(/^(\w+)\s+(\S+)\s+(.*)$/);
					if (parts && parts.length === 4) {
						const [, commitHash, authorDate, subject] = parts;
						return new CommitItem(subject, commitHash, authorDate, filePath, vscode.TreeItemCollapsibleState.None, false, subject);
					}
					return null;
				}).filter((item): item is CommitItem => item !== null);

				// Prepend "Current State" item
				const currentStateItem = new CommitItem('Current State', 'CURRENT', '', filePath, vscode.TreeItemCollapsibleState.None, true);
				const allItems = [currentStateItem, ...commits];
				resolve(allItems);
			});
		});
	}

	getParent(element: CommitItem): vscode.ProviderResult<CommitItem> {
		return null; // flat list has no parent
	}
}
