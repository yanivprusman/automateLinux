import * as vscode from 'vscode';
import { exec } from 'child_process';
import { findGitRepoRoot } from './gitUtils';
import { ActiveFileCommitProvider, CommitItem } from './commitView';
import * as path from 'path';
import * as fs from 'fs';
import { ModeProvider, ModeItem } from './modeProvider';

// Helper function to save file state before checkout
async function saveFileState(filePath: string, repoRoot: string): Promise<void> {
	const stateDir = path.join(repoRoot, '.git-extension-state');
	if (!fs.existsSync(stateDir)) {
		fs.mkdirSync(stateDir, { recursive: true });
	}
	
	try {
		const fileContent = fs.readFileSync(filePath, 'utf-8');
		const fileHash = path.basename(filePath).replace(/[^a-zA-Z0-9]/g, '_');
		const statePath = path.join(stateDir, `${fileHash}.state`);
		fs.writeFileSync(statePath, fileContent, 'utf-8');
	} catch (error) {
		console.error(`Failed to save file state: ${error}`);
	}
}

// Helper function to restore file state
async function restoreFileState(filePath: string, repoRoot: string): Promise<boolean> {
	const stateDir = path.join(repoRoot, '.git-extension-state');
	const fileHash = path.basename(filePath).replace(/[^a-zA-Z0-9]/g, '_');
	const statePath = path.join(stateDir, `${fileHash}.state`);
	
	if (!fs.existsSync(statePath)) {
		return false;
	}
	
	try {
		const savedContent = fs.readFileSync(statePath, 'utf-8');
		fs.writeFileSync(filePath, savedContent, 'utf-8');
		return true;
	} catch (error) {
		console.error(`Failed to restore file state: ${error}`);
		return false;
	}
}

export function activate(context: vscode.ExtensionContext) {
	const modeProvider = new ModeProvider();
	const modeTreeView = vscode.window.createTreeView('Mode', { treeDataProvider: modeProvider });
	context.subscriptions.push(
        vscode.commands.registerCommand('git.toggleMode', (item: ModeItem) => {
            item.checked = !item.checked;
            modeProvider.refresh();
        })
    );
	const commitProvider = new ActiveFileCommitProvider();
	const treeView = vscode.window.createTreeView('activeFileCommitsView', { treeDataProvider: commitProvider });
	const treeView2 = vscode.window.createTreeView('activeFileCommitsView2', { treeDataProvider: commitProvider });
	
	let lastCheckedOut: Record<string, string> = {};
	// Track files that are currently being checked out to avoid race conditions
	let checkoutInProgress: Set<string> = new Set();
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

		// Save current state before checkout (unless we're restoring from saved state)
		if (commitHash !== 'CURRENT') {
			console.log(`[git-ext] Saving current state before checkout from ${commitHash}`);
			await saveFileState(filePath, repoRoot);
		}

		// Handle "Current State" special case
		if (commitHash === 'CURRENT') {
			const restored = await restoreFileState(filePath, repoRoot);
			if (restored) {
				vscode.window.showInformationMessage(`Restored ${path.basename(filePath)} to current working state.`);
			} else {
				vscode.window.showInformationMessage(`Current working state not available. File may not have been modified since last checkout.`);
			}
			return;
		}

		console.log(`[git-ext] Checking out ${filePath} from commit ${commitHash}`);
		
		// Mark checkout as in progress
		checkoutInProgress.add(filePath);
		
		exec(`git checkout ${commitHash} -- "${filePath}"`, { cwd: repoRoot }, (error, stdout, stderr) => {
			checkoutInProgress.delete(filePath);
			
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
	// Listen for file saves and update the current state
	context.subscriptions.push(vscode.workspace.onDidSaveTextDocument(async (document) => {
		if (document.uri.scheme !== 'file') {
			return;
		}

		const filePath = document.uri.fsPath;
		
		// Don't save state while a checkout is in progress
		if (checkoutInProgress.has(filePath)) {
			console.log(`[git-ext] Ignoring save during checkout for ${filePath}`);
			return;
		}

		const repoRoot = await findGitRepoRoot(filePath);
		
		if (!repoRoot) {
			return;
		}

		// Check if this file was recently checked out from a commit
		// Try exact match first, then try normalized path
		let lastCheckout = lastCheckedOut[filePath];
		
		// If no exact match, try to find by comparing file names
		if (!lastCheckout) {
			for (const trackedPath in lastCheckedOut) {
				if (trackedPath.toLowerCase() === filePath.toLowerCase()) {
					lastCheckout = lastCheckedOut[trackedPath];
					break;
				}
			}
		}

		// Only update the saved state if this file was checked out (not the initial 'CURRENT')
		if (lastCheckout && lastCheckout !== 'CURRENT') {
			// File was checked out from a commit and now has been saved
			// Update the current state to this new content
			console.log(`[git-ext] Saving state for ${filePath} after checkout from ${lastCheckout}`);
			await saveFileState(filePath, repoRoot);
		}
	}));
	commitProvider.refresh();
}

export function deactivate() {}
