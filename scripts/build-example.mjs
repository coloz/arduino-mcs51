import {spawnSync} from 'node:child_process';
import {resolve, basename, join} from 'node:path';
import {root, options, workspace} from './workspace.mjs';
const opts = options();
const {toolchain, config, work} = workspace(opts);
const fqbn = opts.fqbn || 'stc:mcs51:stc8h8k64u';
if (!/^stc:mcs51:[a-z0-9_]+(?::.*)?$/.test(fqbn)) throw new Error('Expected stc:mcs51:<board> FQBN.');
const sketch = resolve(opts.sketch || join(root, 'examples/Blink'));
const build = resolve(opts.build || join(work, 'examples', fqbn.split(':')[2], basename(sketch)));
const result = spawnSync('arduino-cli', ['compile', '--config-file', config, '--fqbn', fqbn,
  '--build-path', build, '--build-property', `runtime.tools.stcxx-toolchain.path=${toolchain}`, sketch],
  {stdio: 'inherit', windowsHide: true});
if (result.status !== 0) process.exit(result.status ?? 1);
console.log(`HEX: ${join(build, basename(sketch) + '.ino.hex')}`);

