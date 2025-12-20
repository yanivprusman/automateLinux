import * as vscode from 'vscode';
import { exec } from 'child_process';
import * as path from 'path';

// This method is called when your extension is activated
export function activate(context: vscode.ExtensionContext) {
	console.log('Congratulations, your extension "git" is now active!');

	const activeFileProvider = new ActiveFileProvider();
	const activeFileTreeView = vscode.window.createTreeView('activeFileView', { treeDataProvider: activeFileProvider });

	context.subscriptions.push(vscode.commands.registerCommand('git.showActiveFile', () => {
		activeFileProvider.refresh();
	}));

	context.subscriptions.push(vscode.commands.registerCommand('git.checkoutFileFromCommit', async (commitHash: string, filePath: string) => {
		const editor = vscode.window.activeTextEditor;
		const currentFilePath = editor?.document.uri.fsPath;

		const repoRoot = await findGitRepoRoot(filePath);
		if (!repoRoot) {
			vscode.window.showErrorMessage('Not a Git repository.');
			return;
		}

		// Ensure the file is saved before checking out to avoid losing changes
		if (editor && editor.document.isDirty) {
			const saved = await editor.document.save();
			if (!saved) {
				vscode.window.showErrorMessage('Please save your changes before checking out a previous version.');
				return;
			}
		}

		exec(`git checkout ${commitHash} -- "${filePath}"`, { cwd: repoRoot }, (error, stdout, stderr) => {
			if (error) {
				vscode.window.showErrorMessage(`Failed to checkout file: ${error.message}`);
				return;
			}
			if (stderr) {
				vscode.window.showWarningMessage(`Git checkout warning: ${stderr}`);
			}
			vscode.window.showInformationMessage(`Successfully checked out ${path.basename(filePath)} from commit ${commitHash}.`);

			// Reopen the file to reflect changes, if it was the active file
			if (currentFilePath === filePath) {
				vscode.window.showTextDocument(vscode.Uri.file(filePath), { preview: false });
			}
		});
	}));

	context.subscriptions.push(vscode.commands.registerCommand('git.selectNextCommit', async () => {
		const currentSelection = activeFileTreeView.selection[0];
		if (currentSelection instanceof CommitItem) {
			const commits = await activeFileProvider.getChildren();
			const currentIndex = commits.findIndex(c => c.commitHash === currentSelection.commitHash);
			if (currentIndex !== -1 && currentIndex < commits.length - 1) {
				const nextCommit = commits[currentIndex + 1];
				activeFileTreeView.reveal(nextCommit, { select: true, focus: true });
				vscode.commands.executeCommand('git.checkoutFileFromCommit', nextCommit.commitHash, nextCommit.filePath);
			}
		}
	}));

	context.subscriptions.push(vscode.commands.registerCommand('git.selectPreviousCommit', async () => {
		const currentSelection = activeFileTreeView.selection[0];
		if (currentSelection instanceof CommitItem) {
			const commits = await activeFileProvider.getChildren();
			const currentIndex = commits.findIndex(c => c.commitHash === currentSelection.commitHash);
			if (currentIndex > 0) {
				const previousCommit = commits[currentIndex - 1];
				activeFileTreeView.reveal(previousCommit, { select: true, focus: true });
				vscode.commands.executeCommand('git.checkoutFileFromCommit', previousCommit.commitHash, previousCommit.filePath);
			}
		}
	}));

	context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(() => {
		activeFileProvider.refresh();
	}));

	activeFileProvider.refresh();
}

export function deactivate() {}

// Helper function to find the Git repository root
async function findGitRepoRoot(filePath: string): Promise<string | null> {
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

class CommitItem extends vscode.TreeItem {
	constructor(
		public readonly label: string, // Commit subject
		public readonly commitHash: string,
		public readonly authorDate: string,
		public readonly filePath: string,
		public readonly collapsibleState: vscode.TreeItemCollapsibleState = vscode.TreeItemCollapsibleState.None
	) {
		super(label, collapsibleState);
		this.description = `${authorDate} (${commitHash.substring(0, 7)})`;
		this.tooltip = `${label}\nHash: ${commitHash}\nDate: ${authorDate}\nFile: ${filePath}`;
		this.command = {
			command: 'git.checkoutFileFromCommit',
			title: 'Checkout File from Commit',
			arguments: [this.commitHash, this.filePath]
		};
	}
}

class ActiveFileProvider implements vscode.TreeDataProvider<CommitItem> {
	private _onDidChangeTreeData: vscode.EventEmitter<CommitItem | undefined | void> = new vscode.EventEmitter<CommitItem | undefined | void>();
	readonly onDidChangeTreeData: vscode.Event<CommitItem | undefined | void> = this._onDidChangeTreeData.event;

	refresh(): void {
		this._onDidChangeTreeData.fire();
	}

	getTreeItem(element: CommitItem): vscode.TreeItem {
		return element;
	}

	async getChildren(element?: CommitItem): Promise<CommitItem[]> {
		if (element) {
			return []; // Commit items don't have children
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
						return new CommitItem(subject, commitHash, authorDate, filePath, vscode.TreeItemCollapsibleState.None);
					}
					return null;
				}).filter((item): item is CommitItem => item !== null);

				resolve(commits);
			});
		});
	}
}