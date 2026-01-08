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

// Helper function to annotate diff changes between two commits
async function annotateDiffChanges(fromCommit: string, toCommit: string, filePath: string, repoRoot: string): Promise<string> {
	return new Promise((resolve, reject) => {
		exec(`git --no-pager diff -U999 "${fromCommit}" "${toCommit}" -- "${filePath}"`,
			{ cwd: repoRoot, maxBuffer: 10 * 1024 * 1024 },
			(error, stdout, stderr) => {
				if (error) {
					console.error(`Failed to get diff: ${error.message}`);
					reject(error);
					return;
				}

				const lines = stdout.split('\n');
				const annotatedLines: string[] = [];

				for (const line of lines) {
					// Skip hunk headers, file headers
					if (line.startsWith('@@') || line.startsWith('---') || line.startsWith('+++')) {
						continue;
					}

					if (line.startsWith('-')) {
						// Removed line: strip '-' and append ' //remove'
						const content = line.substring(1);
						annotatedLines.push(content + ' //remove');
					} else if (line.startsWith('+')) {
						// Added line: strip '+', prefix with '//' and append ' //add'
						const content = line.substring(1);
						annotatedLines.push('//' + content + ' //add');
					} else if (line.startsWith(' ')) {
						// Context line: strip leading space
						const content = line.substring(1);
						annotatedLines.push(content);
					} else if (line === '') {
						// Empty line
						annotatedLines.push('');
					}
				}

				const annotatedContent = annotatedLines.join('\n');
				resolve(annotatedContent);
			}
		);
	});
}

export function activate(context: vscode.ExtensionContext) {
	const modeProvider = new ModeProvider();
	const modeTreeView = vscode.window.createTreeView('Mode', { treeDataProvider: modeProvider });
	context.subscriptions.push(
		vscode.commands.registerCommand('git.toggleMode', (item: ModeItem) => {
			// Mutually exclusive: uncheck all modes first
			const modes = modeProvider.getChildren();
			modes.forEach(mode => mode.checked = false);

			// Check only the selected one
			item.checked = true;
			modeProvider.refresh();

			// Set context for view visibility
			const isDiffMode = item.label === 'diff';
			vscode.commands.executeCommand('setContext', 'git-ext:diffMode', isDiffMode);
			console.log(`[git-ext] Mode changed to: ${item.label}`);
		})
	);
	const commitProvider = new ActiveFileCommitProvider();
	const fromCommitView = vscode.window.createTreeView('activeFileFromCommits', { treeDataProvider: commitProvider });
	const toCommitView = vscode.window.createTreeView('activeFileToCommits', { treeDataProvider: commitProvider });

	// Restore state from workspace storage
	let lastCheckedOut: Record<string, string> = context.workspaceState.get('lastCheckedOut') || {};

	const updateLastCheckedOut = (filePath: string, hash: string) => {
		if (lastCheckedOut[filePath] === hash) return;
		lastCheckedOut[filePath] = hash;
		context.workspaceState.update('lastCheckedOut', lastCheckedOut);
	};

	const getLastCheckedOutHash = (filePath: string): string | undefined => {
		let hash = lastCheckedOut[filePath];
		if (!hash) {
			// Try fuzzy match
			const lowerPath = filePath.toLowerCase();
			for (const key in lastCheckedOut) {
				if (key.toLowerCase() === lowerPath) {
					return lastCheckedOut[key];
				}
			}
		}
		return hash;
	};

	// Track files that are currently being checked out to avoid race conditions
	let checkoutInProgress: Set<string> = new Set();

	// Track selections for diff mode
	let selectedFromCommit: { hash: string; filePath: string } | null = null;
	let selectedToCommit: { hash: string; filePath: string } | null = null;

	context.subscriptions.push(vscode.commands.registerCommand('git.copyCommitHash', async (item: CommitItem) => {
		if (!item || !item.commitHash) {
			vscode.window.showErrorMessage('No commit hash to copy.');
			return;
		}
		await vscode.env.clipboard.writeText(item.commitHash);
		vscode.window.showInformationMessage(`Copied commit hash: ${item.commitHash.substring(0, 7)}`);
	}));

	context.subscriptions.push(vscode.commands.registerCommand('git._copyCommitMessage', async (item: CommitItem) => {
		if (!item || !item.commitMessage) {
			vscode.window.showErrorMessage('No commit message to copy.');
			return;
		}
		await vscode.env.clipboard.writeText(item.commitMessage);
		vscode.window.showInformationMessage(`Copied commit message: ${item.commitMessage}`);
	}));

	context.subscriptions.push(vscode.commands.registerCommand('git.applyDiffAnnotation', async () => {
		if (!selectedFromCommit || !selectedToCommit) {
			vscode.window.showErrorMessage('Please select both from and to commits for diff mode.');
			return;
		}

		const editor = vscode.window.activeTextEditor;
		if (!editor) {
			vscode.window.showErrorMessage('No active editor found.');
			return;
		}

		const filePath = editor.document.uri.fsPath;
		const repoRoot = await findGitRepoRoot(filePath);
		if (!repoRoot) {
			vscode.window.showErrorMessage('Not a Git repository.');
			return;
		}

		try {
			let annotatedContent: string;

			// Extract values after null check for type safety
			const fromCommitHash = selectedFromCommit.hash;
			const toCommitHash = selectedToCommit.hash;

			// Convert absolute path to relative path for git commands
			const relativePath = path.relative(repoRoot, filePath);

			// If both commits are the same, show the file content from that commit
			if (fromCommitHash === toCommitHash) {
				console.log(`[git-ext] Same commit selected: ${fromCommitHash.substring(0, 7)}`);
				annotatedContent = await new Promise((resolve, reject) => {
					exec(`git show "${fromCommitHash}:${relativePath}"`,
						{ cwd: repoRoot, maxBuffer: 10 * 1024 * 1024 },
						(error, stdout, stderr) => {
							if (error) {
								// File doesn't exist in this commit, treat as empty
								if (error.message.includes('exists on disk, but not in')) {
									console.log(`[git-ext] File did not exist in commit ${fromCommitHash.substring(0, 7)}, treating as empty`);
									resolve('');
								} else {
									console.error(`Failed to get file content: ${error.message}`);
									reject(error);
								}
								return;
							}
							resolve(stdout);
						}
					);
				});
			} else {
				console.log(`[git-ext] Applying diff annotation: ${fromCommitHash} → ${toCommitHash}`);
				annotatedContent = await annotateDiffChanges(
					fromCommitHash,
					toCommitHash,
					relativePath,
					repoRoot
				);
			}

			// Replace editor content with annotated diff or file content
			const edit = new vscode.WorkspaceEdit();
			const fullRange = new vscode.Range(
				editor.document.positionAt(0),
				editor.document.positionAt(editor.document.getText().length)
			);
			edit.replace(editor.document.uri, fullRange, annotatedContent);
			await vscode.workspace.applyEdit(edit);

			// Store annotation for display in tree and update view
			commitProvider.setCurrentDiffAnnotation(annotatedContent);

			const isSameCommit = fromCommitHash === toCommitHash;
			vscode.window.showInformationMessage(
				`Showing ${isSameCommit ? 'commit' : 'diff'} ${fromCommitHash.substring(0, 7)}${!isSameCommit ? ' → ' + toCommitHash.substring(0, 7) : ''}`
			);
		} catch (error) {
			vscode.window.showErrorMessage(`Failed to apply diff: ${error}`);
		}
	}));

	context.subscriptions.push(vscode.commands.registerCommand('git.checkoutFileFromCommit', async (commitHash: string, filePath: string) => {
		const editor = vscode.window.activeTextEditor;
		const currentFilePath = editor?.document.uri.fsPath;

		// Optimization: skip if already checked out
		if (getLastCheckedOutHash(filePath) === commitHash) {
			console.log(`[git-ext] Skipping checkout: ${commitHash} already current for ${filePath}`);
			return;
		}

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
			console.log(`[git-ext] Saving current state before checkout to ${commitHash}`);
			await saveFileState(filePath, repoRoot);
		}

		// Handle "Current State" special case
		if (commitHash === 'CURRENT') {
			const restored = await restoreFileState(filePath, repoRoot);
			if (restored) {
				updateLastCheckedOut(filePath, 'CURRENT');
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
				// File doesn't exist in this commit, treat as empty
				if (error.message.includes('pathspec') && error.message.includes('did not match')) {
					console.log(`[git-ext] File did not exist in commit ${commitHash}, clearing content`);
					// Write empty file
					fs.writeFileSync(filePath, '', 'utf-8');
					vscode.window.showInformationMessage(`File did not exist in commit ${commitHash.substring(0, 7)}, showing as empty.`);
				} else {
					vscode.window.showErrorMessage(`Failed to checkout file: ${error.message}`);
					return;
				}
			} else {
				if (stderr) {
					console.warn(`Git checkout warning: ${stderr}`);
				}
				vscode.window.showInformationMessage(`Successfully checked out ${path.basename(filePath)} from commit ${commitHash}.`);
			}

			// Update state AFTER successful checkout
			updateLastCheckedOut(filePath, commitHash);

			// Synchronize tree selection
			commitProvider.getChildren().then(children => {
				const currentItem = children.find(c => c.commitHash === commitHash);
				if (currentItem) {
					toCommitView.reveal(currentItem, { select: true });
				}
			});
		});
	}));

	context.subscriptions.push(vscode.commands.registerCommand('git.checkoutFileFromPreviousCommit', async () => {
		const editor = vscode.window.activeTextEditor;
		if (!editor) return;

		const filePath = editor.document.uri.fsPath;
		const commits = await commitProvider.getChildren();
		if (commits.length === 0) return;

		let currentHash = getLastCheckedOutHash(filePath) || commits[0]?.commitHash;
		const currentIndex = commits.findIndex(c => c.commitHash === currentHash);
		const nextCommit = commits[currentIndex + 1];

		console.log(`[git-ext] Previous (Down): Current: ${currentHash} (Index: ${currentIndex}), NextIndex: ${currentIndex + 1}, Target: ${nextCommit?.commitHash}`);

		if (!nextCommit) {
			vscode.window.showInformationMessage('Already at the oldest commit.');
			return;
		}

		await vscode.commands.executeCommand('git.checkoutFileFromCommit', nextCommit.commitHash, filePath);
	}));

	context.subscriptions.push(vscode.commands.registerCommand('git.checkoutFileFromNextCommit', async () => {
		const editor = vscode.window.activeTextEditor;
		if (!editor) return;

		const filePath = editor.document.uri.fsPath;
		const commits = await commitProvider.getChildren();
		if (commits.length === 0) return;

		let currentHash = getLastCheckedOutHash(filePath) || commits[0]?.commitHash;
		const currentIndex = commits.findIndex(c => c.commitHash === currentHash);
		const previousCommit = commits[currentIndex - 1];

		console.log(`[git-ext] Next (Up): Current: ${currentHash} (Index: ${currentIndex}), PrevIndex: ${currentIndex - 1}, Target: ${previousCommit?.commitHash}`);

		if (!previousCommit) {
			vscode.window.showInformationMessage('Already at the newest commit.');
			return;
		}

		await vscode.commands.executeCommand('git.checkoutFileFromCommit', previousCommit.commitHash, filePath);
	}));

	context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(() => {
		commitProvider.refresh();
	}));

	fromCommitView.onDidChangeSelection(e => {
		if (e.selection.length > 0) {
			const item = e.selection[0] as CommitItem;
			const currentMode = modeProvider.getCheckedMode();

			if (currentMode === 'checkout') {
				vscode.commands.executeCommand('git.checkoutFileFromCommit', item.commitHash, item.filePath);
			} else if (currentMode === 'diff') {
				selectedFromCommit = { hash: item.commitHash, filePath: item.filePath };
				if (selectedToCommit) {
					vscode.commands.executeCommand('git.applyDiffAnnotation');
				}
			}
		}
	});

	toCommitView.onDidChangeSelection(e => {
		if (e.selection.length > 0) {
			const item = e.selection[0] as CommitItem;
			const currentMode = modeProvider.getCheckedMode();

			if (currentMode === 'checkout') {
				vscode.commands.executeCommand('git.checkoutFileFromCommit', item.commitHash, item.filePath);
			} else if (currentMode === 'diff') {
				selectedToCommit = { hash: item.commitHash, filePath: item.filePath };
				if (selectedFromCommit) {
					vscode.commands.executeCommand('git.applyDiffAnnotation');
				}
			}
		}
	});

	context.subscriptions.push(vscode.workspace.onDidSaveTextDocument(async (document) => {
		if (document.uri.scheme !== 'file') return;

		const filePath = document.uri.fsPath;
		if (checkoutInProgress.has(filePath)) return;

		const repoRoot = await findGitRepoRoot(filePath);
		if (!repoRoot) return;

		let lastCheckout = getLastCheckedOutHash(filePath);

		if (lastCheckout && lastCheckout !== 'CURRENT') {
			console.log(`[git-ext] Persistence: Saving state for ${filePath} after save of commit version ${lastCheckout}`);
			await saveFileState(filePath, repoRoot);
		}
	}));

	commitProvider.refresh();
}

export function deactivate() { }
