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
    const showWindow = vscode.workspace.getConfiguration('newbasic').get('showWindow', true);
    const args = showWindow === false ? ['--nowindow'] : [];
    return new vscode.DebugAdapterExecutable(command, args, {
      cwd: path.dirname(command),
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
