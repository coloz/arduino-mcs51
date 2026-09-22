import {spawnSync} from 'node:child_process';
import {copyFileSync} from 'node:fs';
import {fileURLToPath} from 'node:url';
import {resolve, dirname} from 'node:path';
import {syncCore} from './sync-core-sdk.mjs';
syncCore({check:true});
const root = resolve(dirname(fileURLToPath(import.meta.url)), '..', 'tools/stcxx-driver');
const result = spawnSync('cargo', ['build', '--release', '--locked', '--offline'], {
  cwd: root, stdio: 'inherit', windowsHide: true,
});
if (result.status !== 0) process.exit(result.status ?? 1);
const name = process.platform === 'win32' ? 'stcxx.exe' : 'stcxx';
const target=process.env.CARGO_TARGET_DIR?resolve(root,process.env.CARGO_TARGET_DIR):resolve(root,'target');
copyFileSync(resolve(target, 'release', name), resolve(root, name));
