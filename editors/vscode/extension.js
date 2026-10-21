// VS Code debug adapter glue for lwbasic (see docs/m5-dap-debugger.md).
//
// VS Code resolves a launch.json "type" through an installed extension, so this registers the
// "lwbasic" type and hands VS Code the adapter. The adapter speaks DAP over stdin/stdout, which
// DebugAdapterExecutable drives directly - no port, no proxy.
const vscode = require('vscode');
const fs = require('fs');
const path = require('path');

// Where lwbasic-dap.exe lives: the lwbasic.adapterPath setting, else the usual build output in the
// first workspace folder.
function adapterPath() {
  const configured = vscode.workspace.getConfiguration('lwbasic').get('adapterPath');
  if (configured) {
    return configured;
  }
  const folders = vscode.workspace.workspaceFolders;
  if (folders && folders.length > 0) {
    return path.join(folders[0].uri.fsPath, 'out', 'src', 'lwbasic-dap', 'Release', 'lwbasic-dap.exe');
  }
  return '';
}

class AdapterFactory {
  createDebugAdapterDescriptor() {
    const command = adapterPath();
    if (!command || !fs.existsSync(command)) {
      throw new Error(
        'lwbasic-dap.exe was not found. Build the interpreter, or set lwbasic.adapterPath to its path.'
      );
    }
    return new vscode.DebugAdapterExecutable(command, [], {
      cwd: path.dirname(command),
    });
  }
}

function activate(context) {
  context.subscriptions.push(
    vscode.debug.registerDebugAdapterDescriptorFactory('lwbasic', new AdapterFactory())
  );
}

function deactivate() {}

module.exports = { activate, deactivate };
