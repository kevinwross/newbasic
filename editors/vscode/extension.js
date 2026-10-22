// VS Code debug adapter glue for newbasic (see docs/m5-dap-debugger.md).
//
// VS Code resolves a launch.json "type" through an installed extension, so this registers the
// "newbasic" type and hands VS Code the adapter. The adapter speaks DAP over stdin/stdout, which
// DebugAdapterExecutable drives directly - no port, no proxy.
const vscode = require('vscode');
const fs = require('fs');
const path = require('path');

// Where newbasic-dap.exe lives: the newbasic.adapterPath setting, else the usual build output in the
// first workspace folder.
function adapterPath() {
  const configured = vscode.workspace.getConfiguration('newbasic').get('adapterPath');
  if (configured) {
    return configured;
  }
  const folders = vscode.workspace.workspaceFolders;
  if (folders && folders.length > 0) {
    return path.join(folders[0].uri.fsPath, 'out', 'src', 'newbasic-dap', 'Release', 'newbasic-dap.exe');
  }
  return '';
}

class AdapterFactory {
  createDebugAdapterDescriptor() {
    const command = adapterPath();
    if (!command || !fs.existsSync(command)) {
      throw new Error(
        'newbasic-dap.exe was not found. Build the interpreter, or set newbasic.adapterPath to its path.'
      );
    }
    // The adapter opens the interpreter's GUI window by default so PRINT output and graphics are
    // visible while debugging. --nowindow keeps a session headless (CI, SSH, or an explicit opt-out).
    // Time-travel snapshots are opt-in because they copy the VM at every line boundary.
    const config = vscode.workspace.getConfiguration('newbasic');
    const showWindow = config.get('showWindow', true);
    const history = config.get('history', false);
    const args = [];
    if (showWindow === false)
      args.push('--nowindow');
    if (history === true)
      args.push('--history');
    // Relative paths in the BASIC program (for example samples\accounting.dat)
    // must resolve from the workspace, not from the adapter executable's folder.
    const active = vscode.window.activeTextEditor;
    const activeFolder = active ? vscode.workspace.getWorkspaceFolder(active.document.uri) : undefined;
    const firstFolder =
      vscode.workspace.workspaceFolders && vscode.workspace.workspaceFolders.length > 0
        ? vscode.workspace.workspaceFolders[0]
        : undefined;
    const workingDirectory = activeFolder
      ? activeFolder.uri.fsPath
      : firstFolder
        ? firstFolder.uri.fsPath
        : path.dirname(command);
    return new vscode.DebugAdapterExecutable(command, args, {
      cwd: workingDirectory,
    });
  }
}

function activate(context) {
  context.subscriptions.push(
    vscode.debug.registerDebugAdapterDescriptorFactory('newbasic', new AdapterFactory())
  );
}

function deactivate() {}

module.exports = { activate, deactivate };
