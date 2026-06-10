const os = require('os');
const fs = require('fs');
const path = require('path');

const platform = os.platform();
const arch = os.arch();

const platformMap = {
  'linux_x64': 'memspace-linux-x64',
  'linux_arm64': 'memspace-linux-arm64',
  'darwin_x64': 'memspace-darwin-x64',
  'darwin_arm64': 'memspace-darwin-arm64',
  'win32_x64': 'memspace-windows-x64'
};

const pkgName = platformMap[`${platform}_${arch}`];

if (!pkgName) {
  console.warn(`memspace: warning, no binary package available for ${platform} ${arch}`);
  process.exit(0);
}

const ext = platform === 'win32' ? '.exe' : '';

let binaryPath;
try {
  binaryPath = require.resolve(`${pkgName}/bin/memspace${ext}`);
} catch (err) {
  binaryPath = path.resolve(__dirname, '..', 'packages', pkgName, 'bin', `memspace${ext}`);
  if (!fs.existsSync(binaryPath)) {
    console.warn(`memspace: warning, binary not found for ${pkgName}. This is normal if optional dependencies failed to install.`);
    process.exit(0);
  }
}

if (platform !== 'win32') {
  try {
    fs.chmodSync(binaryPath, 0o755);
  } catch (err) {
    console.warn(`memspace: warning, could not chmod binary at ${binaryPath}`);
  }
}

console.log(`memspace: installed binary for ${platform} ${arch}`);
