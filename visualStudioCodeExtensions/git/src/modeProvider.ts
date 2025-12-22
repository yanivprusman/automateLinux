import * as vscode from 'vscode';

export class ModeProvider implements vscode.TreeDataProvider<ModeItem> {
  private _onDidChangeTreeData: vscode.EventEmitter<ModeItem | undefined> = new vscode.EventEmitter();
  readonly onDidChangeTreeData = this._onDidChangeTreeData.event;
  
  private modes: ModeItem[] = [
    new ModeItem('checkout', true),
    new ModeItem('diff', false)
  ];

  getTreeItem(element: ModeItem): vscode.TreeItem {
    const item = new vscode.TreeItem(element.label, vscode.TreeItemCollapsibleState.None);
    item.contextValue = 'modeItem';
    item.command = {
      command: 'git.toggleMode',
      title: 'Toggle Mode',
      arguments: [element]
    };
    // Radio button style: filled circle if checked, outline if not
    item.iconPath = element.checked
      ? new vscode.ThemeIcon('radio-tower')
      : new vscode.ThemeIcon('circle-outline');
    return item;
  }

  getChildren(): ModeItem[] {
    return this.modes;
  }

  getCheckedMode(): string {
    const checked = this.modes.find(m => m.checked);
    return checked?.label || 'checkout';
  }

  refresh() {
    this._onDidChangeTreeData.fire(undefined);
  }
}

export class ModeItem {
  constructor(public label: string, public checked: boolean) {}
}
