import * as vscode from 'vscode';

export function activate(context: vscode.ExtensionContext) {
    console.log('Extension "common" is active');

    // Create TreeView
    const treeDataProvider = new ActivitiTreeProvider();
    const treeView = vscode.window.createTreeView('commonView', { treeDataProvider });

    // Command to launch an Activiti item
    const launchCommand = vscode.commands.registerCommand('common.launchActiviti', (label: string) => {
        vscode.window.showInformationMessage(`Launching ${label}...`);
    });
    context.subscriptions.push(launchCommand);

    // Command to execute the currently selected item
    const executeSelectedCommand = vscode.commands.registerCommand('common.executeSelectedActivitiItem', () => {
        const { selection } = treeView;
        if (selection.length > 0) {
            const item = selection[0] as ActivitiItem;
            vscode.commands.executeCommand('common.launchActiviti', item.label);
        } else {
            vscode.window.showInformationMessage('No Activiti item selected.');
        }
    });
    context.subscriptions.push(executeSelectedCommand);

    // Command to move down and execute the next item
    const executeNextItem = vscode.commands.registerCommand('common.executeNextActivitiItem', () => {
        treeDataProvider.getChildren().then(items => {
            const selection = treeView.selection;
            let index = 0;
            if (selection.length > 0) {
                index = items.findIndex(i => i.label === selection[0].label);
                index = Math.min(index + 1, items.length - 1); // next item
            }
            const nextItem = items[index];
            treeView.reveal(nextItem, { select: true, focus: true });
            vscode.commands.executeCommand('common.launchActiviti', nextItem.label);
        });
    });
    context.subscriptions.push(executeNextItem);

    // Trigger command on selection change (arrow keys or mouse)
    treeView.onDidChangeSelection(e => {
        if (e.selection.length > 0) {
            const item = e.selection[0] as ActivitiItem;
            vscode.commands.executeCommand('common.launchActiviti', item.label);
        }
    });

    // Focus first item on load
    treeDataProvider.getChildren().then(children => {
        if (children.length > 0) {
            treeView.reveal(children[0], { select: true, focus: true });
        }
    });

    context.subscriptions.push(treeView);
}

export function deactivate() {}

// ----------------- TreeView classes -----------------

class ActivitiTreeProvider implements vscode.TreeDataProvider<ActivitiItem> {
    getTreeItem(element: ActivitiItem): vscode.TreeItem {
        return element;
    }

    getChildren(): Thenable<ActivitiItem[]> {
        return Promise.resolve([
            new ActivitiItem('Element 1'),
            new ActivitiItem('Element 2')
        ]);
    }
}

class ActivitiItem extends vscode.TreeItem {
    constructor(label: string) {
        super(label);
        // No command assigned here; execution handled via selection
    }
}
