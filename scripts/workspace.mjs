import {existsSync, mkdirSync, cpSync, writeFileSync, symlinkSync, realpathSync} from 'node:fs';
import {resolve, dirname, join} from 'node:path';
import {fileURLToPath} from 'node:url';
export const root = resolve(dirname(fileURLToPath(import.meta.url)), '..');
export function options(args = process.argv.slice(2)) {
  const result = {};
  while (args.length) {
    const key = args.shift();
    if (!key.startsWith('--') || !args.length) throw new Error(`Expected --option value: ${key}`);
    result[key.slice(2)] = args.shift();
  }
  return result;
}
export function workspace(opts = {}) {
  if (process.platform !== 'win32') throw new Error('This initial toolchain lock is validated on Windows x64 only.');
  const installed = opts['arduino-data'] || join(process.env.LOCALAPPDATA, 'Arduino15');
  const toolchain = resolve(opts.toolchain || [
    join(root, '.build/toolchain/stcxx-toolchain'),
    join(installed, 'packages/stc/tools/stcxx-toolchain/0.3.0'),
  ].find(p => existsSync(join(p, 'sdcc/bin/sdcc.exe'))) || 'missing-toolchain');
  if (!existsSync(join(toolchain, 'sdcc/bin/sdcc.exe'))) throw new Error('Use --toolchain <stcxx-toolchain-0.3.0 directory>.');
  const driver = join(root, 'tools/stcxx-driver/stcxx.exe');
  if (!existsSync(driver)) throw new Error('Run node scripts/build-native-driver.mjs first.');
  const work = join(root, '.build');
  const hardware = join(work, 'hardware/stc/mcs51');
  mkdirSync(dirname(hardware), {recursive: true});
  if (!existsSync(hardware)) symlinkSync(root, hardware, 'junction');
  if (realpathSync(hardware).toLowerCase() !== realpathSync(root).toLowerCase()) throw new Error('Staging hardware path points at another project.');
  const data = join(work, 'data');
  const builtin = join(data, 'packages/builtin/tools');
  if (!existsSync(join(builtin, 'ctags'))) {
    const source = join(installed, 'packages/builtin/tools');
    if (!existsSync(source)) throw new Error('Arduino CLI builtin tools are missing; initialize Arduino CLI or pass --arduino-data.');
    cpSync(source, builtin, {recursive: true});
  }
  // Isolated offline indexes: do not modify the user's installed platforms.
  writeFileSync(join(data, 'package_index.json'), '{"packages":[]}\n');
  writeFileSync(join(data, 'library_index.json'), '{"libraries":[]}\n');
  const config = join(work, 'arduino-cli.yaml');
  writeFileSync(config, `directories:\n  user: ${JSON.stringify(work.replaceAll('\\', '/'))}\n  data: ${JSON.stringify(data.replaceAll('\\', '/'))}\n`);
  return {toolchain, config, work};
}

