// VS Code debug adapter glue for newbasic (see docs/m5-dap-debugger.md).
//
// VS Code resolves a launch.json "type" through an installed extension, so this
// registers the "newbasic" type and hands VS Code the adapter. The adapter speaks
// DAP over stdin/stdout, which DebugAdapterExecutable drives directly - no port,
// no proxy.
const vscode = require('vscode');
const fs = require('fs');
const path = require('path');

let output = null;

function log(message) {
  if (output) output.appendLine(message);
}

function workspaceFolder() {
  const folders = vscode.workspace.workspaceFolders;
  return folders && folders.length > 0 ? folders[0].uri.fsPath : '';
}

function expandWorkspaceFolder(value, folder) {
  return value.replace(/\$\{workspaceFolder\}/g, folder || '');
}

// The build puts the adapter in the workspace-root bin directory. Keep the old
// CMake-tree locations as fallbacks for existing checkouts. On Windows the
// executable carries .exe.
function adapterCandidates(folder, platform = process.platform) {
  const exe = platform === 'win32' ? '.exe' : '';
  const root = folder || '';
  return [
    path.join(root, 'bin', 'Release', `newbasic-dap${exe}`),
    path.join(root, 'bin', 'Debug', `newbasic-dap${exe}`),
    path.join(root, 'build-sdl', 'src', 'newbasic-dap', `newbasic-dap${exe}`),
    path.join(root, 'build', 'src', 'newbasic-dap', `newbasic-dap${exe}`),
    path.join(root, 'out', 'src', 'newbasic-dap', 'Release', `newbasic-dap${exe}`),
    path.join(root, 'out', 'src', 'newbasic-dap', 'Debug', `newbasic-dap${exe}`),
  ];
}

function adapterPath() {
  const folder = workspaceFolder();
  const configured = vscode.workspace.getConfiguration('newbasic').get('adapterPath');
  if (configured) {
    const expanded = expandWorkspaceFolder(configured, folder);
    return path.isAbsolute(expanded) ? expanded : path.join(folder, expanded);
  }
  const candidates = adapterCandidates(folder);
  const found = candidates.find((candidate) => fs.existsSync(candidate));
  return found || '';
}

function isExecutable(file) {
  if (process.platform === 'win32')
    return true;
  try {
    fs.accessSync(file, fs.constants.X_OK);
    return true;
  } catch {
    return false;
  }
}

function isEmptyConfiguration(config) {
  return !config.type && !config.request && !config.name;
}

// First hook: defaults only. VS Code has not substituted ${file} and friends yet,
// so a path must never be inspected here.
function resolveDebugConfiguration(folder, config) {
  if (isEmptyConfiguration(config))
    return undefined;
  return config;
}

// Validate the launch after variables are substituted. Without this, a missing
// program or a non-executable adapter starts a session that dies immediately,
// which VS Code reports only as the opaque "Invalid debug adapter" error.
async function resolveDebugConfigurationWithSubstitutedVariables(folder, config) {
  if (isEmptyConfiguration(config))
    return undefined;

  const editor = vscode.window.activeTextEditor;
  const document = editor ? editor.document : undefined;
  const folderPath = folder ? folder.uri.fsPath : workspaceFolder();

  if (config.program) {
    let program = expandWorkspaceFolder(String(config.program), folderPath);
    if (!path.isAbsolute(program) && folderPath)
      program = path.join(folderPath, program);
    config.program = program;

    let isFile = false;
    try {
      isFile = fs.statSync(program).isFile();
    } catch {
      isFile = false;
    }
    const sameDocument = document && (document.uri.fsPath === program || document.isUntitled);

    if (isFile) {
      // An unsaved buffer would otherwise run the stale file on disk.
      if (document && document.uri.fsPath === program && document.isDirty) {
        log(`saving ${program} before launch`);
        await document.save();
      }
    } else if (sameDocument && (document.isUntitled || document.isDirty) && document.getText().trim()) {
      log(`program not found on disk (${program}); launching the active buffer as programText`);
      config.programText = document.getText();
      delete config.program;
    } else {
      const message = `newbasic: program not found: ${program}`;
      log(message);
      if (output) output.show(true);
      vscode.window.showErrorMessage(message);
      return undefined;
    }
  } else if (!config.programText) {
    if (document && document.getText().trim()) {
      config.programText = document.getText();
      log('launching the active buffer as programText');
    } else {
      const message = 'newbasic: no program. Open a .bas file or set "program"/"programText" in launch.json.';
      log(message);
      if (output) output.show(true);
      vscode.window.showErrorMessage(message);
      return undefined;
    }
  }

  log(`launch: type=${config.type} program=${config.program || '<inline>'} cwd=${folderPath}`);
  return config;
}

class AdapterFactory {
  createDebugAdapterDescriptor() {
    try {
      const folder = workspaceFolder();
      const configured = vscode.workspace.getConfiguration('newbasic').get('adapterPath');
      const candidates = configured
        ? [expandWorkspaceFolder(configured, folder)]
        : adapterCandidates(folder);
      const command = adapterPath();
      if (!command || !fs.existsSync(command)) {
        throw new Error(
            'newbasic-dap was not found. Looked in:\n' + candidates.join('\n') +
            '\nBuild the interpreter, or set newbasic.adapterPath to its full path.');
      }
      if (!isExecutable(command)) {
        throw new Error(
            `${command} is not executable. On Linux/macOS run: chmod +x "${command}"`);
      }

      // The adapter opens the interpreter's GUI window by default so PRINT output
      // and graphics are visible while debugging. --nowindow keeps a session
      // headless (CI, SSH, or an explicit opt-out). Time-travel snapshots are
      // opt-in because they copy the VM at every line boundary.
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

      log(`adapter: ${command} args=[${args.join(' ')}] cwd=${workingDirectory}`);
      return new vscode.DebugAdapterExecutable(command, args, {
        cwd: workingDirectory,
      });
    } catch (error) {
      log(`adapter error: ${error && error.stack ? error.stack : error}`);
      if (output) output.show(true);
      throw error;
    }
  }
}

function activate(context) {
  output = vscode.window.createOutputChannel('newbasic');
  context.subscriptions.push(output);
  log(`newbasic debugger on ${process.platform}/${process.arch}, VS Code ${vscode.version}`);

  context.subscriptions.push(
      vscode.debug.registerDebugConfigurationProvider('newbasic', {
        resolveDebugConfiguration,
        resolveDebugConfigurationWithSubstitutedVariables,
      }));
  context.subscriptions.push(
      vscode.debug.registerDebugAdapterDescriptorFactory('newbasic', new AdapterFactory()));
  log('registered debug configuration provider and adapter factory');
}

function deactivate() {}

module.exports = { activate, deactivate, adapterPath, adapterCandidates };
