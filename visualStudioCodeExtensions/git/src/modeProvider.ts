import * as vscode from 'vscode';

export class ModeProvider implements vscode.TreeDataProvider<ModeItem> {
  private _onDidChangeTreeData: vscode.EventEmitter<ModeItem | undefined> = new vscode.EventEmitter();
  readonly onDidChangeTreeData = this._onDidChangeTreeData.event;

  getTreeItem(element: ModeItem): vscode.TreeItem {
    const item = new vscode.TreeItem(element.label, vscode.TreeItemCollapsibleState.None);
    item.contextValue = 'modeItem';
    item.command = {
      command: 'git.toggleMode',
      title: 'Toggle Mode',
      arguments: [element]
    };
    item.iconPath = element.checked ? new vscode.ThemeIcon('check') : undefined;
    return item;
  }

  getChildren(): ModeItem[] {
    return [
      new ModeItem('checkout', true),
      new ModeItem('diff', false)
    ];
  }

  refresh() {
    this._onDidChangeTreeData.fire(undefined);
  }
}

export class ModeItem {
  constructor(public label: string, public checked: boolean) {}
}
