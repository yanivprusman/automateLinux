import * as vscode from 'vscode';
import { exec } from 'child_process';
import { findGitRepoRoot } from './gitUtils';
import { ActiveFileCommitProvider, CommitItem } from './commitView';
import * as path from 'path';

export function activate(context: vscode.ExtensionContext) {
	const commitProvider = new ActiveFileCommitProvider();
	const treeView = vscode.window.createTreeView('activeFileCommitsView', { treeDataProvider: commitProvider });
	const treeView2 = vscode.window.createTreeView('activeFileCommitsView2', { treeDataProvider: commitProvider });
	let lastCheckedOut: Record<string, string> = {};
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
			const currentHash = lastCheckedOut[filePath];
			const currentItem = children.find(c => c.commitHash === currentHash);
			if (currentItem) {
				treeView.reveal(currentItem, { select: true, focus: true });
			}
		});
	}));
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

		lastCheckedOut[filePath] = nextCommit.commitHash;
		await vscode.commands.executeCommand('git.checkoutFileFromCommit', nextCommit.commitHash, filePath);
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

		lastCheckedOut[filePath] = previousCommit.commitHash;
		await vscode.commands.executeCommand('git.checkoutFileFromCommit', previousCommit.commitHash, filePath);
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
	commitProvider.refresh();
}

export function deactivate() {}
