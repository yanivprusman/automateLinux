import * as vscode from 'vscode';
import { exec } from 'child_process';
import * as path from 'path';

export function activate(context: vscode.ExtensionContext) {
	const commitProvider = new ActiveFileCommitProvider();
	const treeView = vscode.window.createTreeView('activeFileCommitsView', { treeDataProvider: commitProvider });
	context.subscriptions.push(vscode.commands.registerCommand('git.showActiveFileCommits', () => {
		commitProvider.refresh();
	}));
	context.subscriptions.push(vscode.commands.registerCommand('git.checkoutFileFromCommit', async (commitHash: string, filePath: string) => {
		const editor = vscode.window.activeTextEditor;
		const currentFilePath = editor?.document.uri.fsPath;
		const repoRoot = await findGitRepoRoot(filePath);
		if (!repoRoot) {
			vscode.window.showErrorMessage('Not a Git repository.');
			return;
		}
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
			if (currentFilePath === filePath) {
				// vscode.window.showTextDocument(vscode.Uri.file(filePath), { preview: false });
			}
		});
		commitProvider.getChildren().then(children => {
			if (children.length > 0) {
				treeView.reveal(children[0], { select: true, focus: true });
			}
		});
	}));
	context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(() => {
		commitProvider.refresh();
	}));
	treeView.onDidChangeSelection(e => {
		if (e.selection.length > 0) {
			const item = e.selection[0] as CommitItem;
			lastCheckedOut[item.filePath] = item.commitHash;
			vscode.commands.executeCommand('git.checkoutFileFromCommit', item.commitHash, item.filePath);
		}
	});
	let lastCheckedOut: Record<string, string> = {};
	context.subscriptions.push(vscode.commands.registerCommand('git.checkoutFileFromPreviousCommit', async () => {
		const editor = vscode.window.activeTextEditor;
		if (!editor) return;

		const filePath = editor.document.uri.fsPath;
		const repoRoot = await findGitRepoRoot(filePath);
		if (!repoRoot) {
			vscode.window.showErrorMessage('Not a Git repository.');
			return;
		}

		const commits = await commitProvider.getChildren();
		if (commits.length === 0) {
			vscode.window.showInformationMessage('No commits found for this file.');
			return;
		}

		let currentHash = lastCheckedOut[filePath] || commits[0]?.commitHash;
		const currentIndex = commits.findIndex(c => c.commitHash === currentHash);
		const nextCommit = commits[currentIndex + 1];

		if (!nextCommit) {
			vscode.window.showInformationMessage('Already at the oldest commit.');
			return;
		}

		await vscode.commands.executeCommand('git.checkoutFileFromCommit', nextCommit.commitHash, filePath);
		lastCheckedOut[filePath] = nextCommit.commitHash;
		treeView.reveal(nextCommit, { select: true, focus: false });
	}));

	context.subscriptions.push(vscode.commands.registerCommand('git.checkoutFileFromNextCommit', async () => {
		const editor = vscode.window.activeTextEditor;
		if (!editor) return;

		const filePath = editor.document.uri.fsPath;
		const repoRoot = await findGitRepoRoot(filePath);
		if (!repoRoot) {
			vscode.window.showErrorMessage('Not a Git repository.');
			return;
		}

		const commits = await commitProvider.getChildren();
		if (commits.length === 0) {
			vscode.window.showInformationMessage('No commits found for this file.');
			return;
		}

		let currentHash = lastCheckedOut[filePath] || commits[0]?.commitHash;
		const currentIndex = commits.findIndex(c => c.commitHash === currentHash);
		const previousCommit = commits[currentIndex - 1];

		if (!previousCommit) {
			vscode.window.showInformationMessage('Already at the newest commit.');
			return;
		}

		await vscode.commands.executeCommand('git.checkoutFileFromCommit', previousCommit.commitHash, filePath);
		lastCheckedOut[filePath] = previousCommit.commitHash;
		treeView.reveal(previousCommit, { select: true, focus: false });
	}));

	commitProvider.refresh();
}

export function deactivate() {}

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
		public readonly label: string,
		public readonly commitHash: string,
		public readonly authorDate: string,
		public readonly filePath: string,
		public readonly collapsibleState: vscode.TreeItemCollapsibleState = vscode.TreeItemCollapsibleState.None
	) {
		super(label, collapsibleState);
		this.description = `${authorDate} (${commitHash.substring(0, 7)})`;
		this.tooltip = `${label}\nHash: ${commitHash}\nDate: ${authorDate}\nFile: ${filePath}`;
		// this.command = {
		// 	command: 'git.checkoutFileFromCommit',
		// 	title: 'Checkout File from Commit',
		// 	arguments: [this.commitHash, this.filePath]
		// };
	}
}

class ActiveFileCommitProvider implements vscode.TreeDataProvider<CommitItem> {
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
						return new CommitItem(subject, commitHash, authorDate, filePath, vscode.TreeItemCollapsibleState.None);
					}
					return null;
				}).filter((item): item is CommitItem => item !== null);
				resolve(commits);
			});
		});
	}
}