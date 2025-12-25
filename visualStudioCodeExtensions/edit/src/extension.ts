import * as vscode from 'vscode';
export function activate(context: vscode.ExtensionContext) {
	const disposableCommandLine = vscode.commands.registerCommand('edit.commentCommandLine', async () => {
		const editor = vscode.window.activeTextEditor;
		if (!editor) {
			return;
		}

		const selection = editor.selection;
		const line = editor.document.lineAt(selection.active.line);
		const text = line.text;

		if (text.trimEnd().endsWith('\\')) {
			const trimmed = text.trim();
			// Check if already commented: starts with '`#' and has '`' before the last '\'
			const commentRegex = /^`#(.*)`\\(\s*)$/;
			const match = text.match(commentRegex);

			if (match) {
				// Uncomment
				const content = match[1];
				const padding = match[2];
				await editor.edit(editBuilder => {
					editBuilder.replace(line.range, content + '\\' + padding);
				});
			} else {
				// Comment
				const backslashIndex = text.lastIndexOf('\\');
				const content = text.substring(0, backslashIndex);
				const rest = text.substring(backslashIndex);
				await editor.edit(editBuilder => {
					editBuilder.replace(line.range, `\`#${content}\`${rest}`);
				});
			}
			// } else if (line.isEmptyOrWhitespace) {
			// 	await editor.edit(editBuilder => {
			// 		const range = line.rangeIncludingLineBreak;
			// 		editBuilder.delete(range);
			// 	});
		} else {
			await vscode.commands.executeCommand('editor.action.commentLine');
		}
	});
	context.subscriptions.push(disposableCommandLine);
}
export function deactivate() { }
