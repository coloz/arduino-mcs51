// Compile/link qualification only. No upload or physical-board claims.
import {spawn, spawnSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import {existsSync, mkdirSync, readdirSync, readFileSync, writeFileSync, appendFileSync, copyFileSync, unlinkSync} from 'node:fs';
import {basename, dirname, join, relative, resolve} from 'node:path';
import {root, options, workspace} from './workspace.mjs';

const opts = options();
const {toolchain, config, work} = workspace(opts);
const aily = resolve(opts.aily || process.env.AILY_BUILDER || '../aily-builder/dist/main.js');
const cli = opts.cli || 'arduino-cli';
const validator = resolve(opts.validator || '../stc-cli/target/release/stc-cli.exe');
const builders = (opts.builders || 'arduino,aily').split(',');
if (builders.some(b => !['arduino', 'aily'].includes(b))) throw new Error('Invalid --builders');
if (builders.includes('aily') && !existsSync(aily)) throw new Error('Pass --aily <aily-builder/dist/main.js>');
if (!existsSync(validator)) throw new Error('Pass --validator <stc-cli.exe>');
const devices = JSON.parse(readFileSync(join(root, 'tools/variants/devices.json'))).devices;
const boards = opts.boards ? opts.boards.split(',') : devices.map(d => d.id);
if (boards.some(b => !devices.some(d => d.id === b))) throw new Error('Unknown board');
const suites = (opts.suites || 'examples,common').split(',');
if (suites.some(s => !['examples', 'common', 'boundaries'].includes(s))) throw new Error('Unknown suite');
const jobs = Number(opts.jobs || 2);
const workers = Number(opts.workers || 4);
if (![jobs, workers].every(n => Number.isInteger(n) && n > 0 && n <= 16)) throw new Error('Invalid concurrency');
const runName = opts.run || new Date().toISOString().replace(/[:.]/g, '-');
if (!/^[a-zA-Z0-9_-]+$/.test(runName)) throw new Error('Invalid --run name');
const out = join(work, 'qualification', runName);
const resume = opts.resume === 'true';
const cliCache = opts['arduino-cache'] !== 'false';
if (existsSync(out) && !resume) throw new Error(`Run already exists; choose a new --run or --resume true: ${out}`);
if (resume && !existsSync(join(out, 'metadata.json'))) throw new Error('No run metadata to resume');
mkdirSync(out, {recursive: true});
mkdirSync(join(out, 'logs'), {recursive: true});

function discover(base, suite) {
  if (!existsSync(base)) return [];
  const result = [];
  for (const e of readdirSync(base, {withFileTypes: true})) {
    if (e.isDirectory()) result.push(...discover(join(base, e.name), suite));
    else if (e.name === `${basename(base)}.ino`) result.push({
      id: relative(root, base).replaceAll('\\', '/'), name: basename(base), sketch: base, suite,
    });
  }
  return result;
}
const cases = [
  ...discover(join(root, 'examples'), 'examples'),
  ...discover(join(root, 'libraries'), 'examples'),
  ...discover(join(root, 'tests/compile/common'), 'common'),
  ...discover(join(root, 'tests/compile/boundaries'), 'boundaries'),
].filter(c => suites.includes(c.suite) && (!opts.cases || opts.cases.split(',').includes(c.name)))
  .sort((a, b) => a.id.localeCompare(b.id));
if (!cases.length) throw new Error('No sketches selected');
const sha = data => createHash('sha256').update(data).digest('hex');
function version(command, args) {
  const p = spawnSync(command, args, {encoding: 'utf8', windowsHide: true});
  if (p.status !== 0) throw new Error(`${command}: ${p.error || p.stderr}`);
  return p.stdout.trim();
}
const metadata = {
  started: new Date().toISOString(), node: process.version, host: `${process.platform}/${process.arch}`,
  arduino: version(cli, ['version']),
  aily: builders.includes('aily') ? version(process.execPath, [aily, '--version']) : null,
  ailyEntry: builders.includes('aily') ? aily : null,
  ailyEntrySha256: builders.includes('aily') ? sha(readFileSync(aily)) : null,
  toolchain, driverSha256: sha(readFileSync(join(root, 'tools/stcxx-driver/stcxx.exe'))),
  platformSha256: sha(readFileSync(join(root, 'platform.txt'))),
  boardsSha256: sha(readFileSync(join(root, 'boards.txt'))),
  sourceCommit: version('git', ['-C', root, 'rev-parse', 'HEAD']),
  sourceStatus: version('git', ['-C', root, 'status', '--short']),
  validator: version(validator, ['--version']),
  boards, builders, jobs, workers,
  ailyArchiveCache: 'local-only',
  arduinoCache: cliCache,
  cases: cases.map(c => ({...c, sha256: sha(readFileSync(join(c.sketch, `${c.name}.ino`)))})),
};
if (resume) {
  const before = JSON.parse(readFileSync(join(out, 'metadata.json')));
  for (const key of ['driverSha256', 'platformSha256', 'boardsSha256', 'boards', 'builders', 'cases']) {
    if (JSON.stringify(before[key]) !== JSON.stringify(metadata[key])) throw new Error(`Cannot resume changed input: ${key}`);
  }
  writeFileSync(join(out, `resume-${Date.now()}.json`), JSON.stringify(metadata, null, 2) + '\n');
  metadata.started = before.started;
  if (existsSync(join(out, 'STOP'))) unlinkSync(join(out, 'STOP'));
} else writeFileSync(join(out, 'metadata.json'), JSON.stringify(metadata, null, 2) + '\n');

function execute(command, args, cwd, log, env = {}) {
  return new Promise(resolveResult => {
    const start = performance.now();
    appendFileSync(log, JSON.stringify({command, args, cwd, env}) + '\n');
    const child = spawn(command, args, {cwd, env: {...process.env, ...env}, windowsHide: true, stdio: ['ignore', 'pipe', 'pipe']});
    let output = '', timedOut = false;
    const record = data => { const s = data.toString(); output += s; appendFileSync(log, s); };
    child.stdout.on('data', record);
    child.stderr.on('data', record);
    child.on('error', e => record(String(e)));
    const timer = setTimeout(() => {
      timedOut = true;
      // Windows child compilers must be stopped together with their parent.
      spawnSync('taskkill', ['/PID', String(child.pid), '/T', '/F'], {windowsHide: true, stdio: 'ignore'});
    }, 300000);
    child.on('close', code => {
      clearTimeout(timer);
      resolveResult({code, timedOut, seconds: +( (performance.now() - start) / 1000).toFixed(3), output});
    });
  });
}

function inspectHex(path, capacity) {
  const bytes = new Map();
  let base = 0, eof = false;
  for (const line of readFileSync(path, 'utf8').trim().split(/\r?\n/)) {
    if (eof || !/^:[0-9a-f]+$/i.test(line) || (line.length - 1) % 2) throw new Error('Invalid HEX record');
    const b = Buffer.from(line.slice(1), 'hex');
    if (b.length !== b[0] + 5 || (b.reduce((s, v) => s + v, 0) & 255)) throw new Error('HEX checksum/length error');
    const addr = b.readUInt16BE(1), type = b[3];
    if (type === 0) {
      for (let i = 0; i < b[0]; i++) {
        const a = base + addr + i;
        if (a >= capacity || a > 0xffff) throw new Error(`HEX exceeds MCS51 model capacity: ${a}`);
        if (bytes.has(a) && bytes.get(a) !== b[4 + i]) throw new Error('Conflicting HEX records');
        bytes.set(a, b[4 + i]);
      }
    } else if (type === 1 && b[0] === 0) eof = true;
    else if ((type === 2 || type === 4) && b[0] === 2) base = b.readUInt16BE(4) * (type === 2 ? 16 : 65536);
    else if (![3, 5].includes(type)) throw new Error(`Unsupported HEX record ${type}`);
  }
  if (!eof || !bytes.has(0)) throw new Error('Missing HEX EOF/reset vector');
  const sorted = [...bytes].sort((a, b) => a[0] - b[0]);
  return {bytes: bytes.size, maxAddress: sorted.at(-1)[0], imageSha256: sha(JSON.stringify(sorted)), fileSha256: sha(readFileSync(path))};
}

function memory(path) {
  const s = readFileSync(path, 'utf8');
  const region = name => {
    const line = s.split(/\r?\n/).find(l => l.includes(name));
    if (!line) return null;
    const m = line.trim().match(/(\d+)\s+(\d+)\s*$/);
    return m ? {used: Number(m[1]), max: Number(m[2])} : null;
  };
  return {flash: region('ROM/EPROM/FLASH'), xdata: region('EXTERNAL RAM'),
    stackReservation: Number(s.match(/with (\d+) bytes available/)?.[1] || 0)};
}
function classify(output) {
  if (/Insufficient (ROM\/EPROM\/FLASH|EXTERNAL RAM|XRAM|RAM)|(?:code|xram|flash) (?:size|memory).*exceed|exceeds.*(?:flash|code|xram)/i.test(output)) return 'capacity';
  if (/file not found|No such file or directory/.test(output)) return 'missing-header';
  return 'compile-error';
}
const results = resume && existsSync(join(out, 'results.jsonl'))
  ? readFileSync(join(out, 'results.jsonl'), 'utf8').trim().split('\n').filter(Boolean).map(JSON.parse) : [];
let completed = results.length;
async function compile(builder, board, c) {
  const device = devices.find(d => d.id === board);
  const key = `${builder}__${board}__${c.id.replaceAll('/', '_')}`;
  // The SDCC backend still needs short command-line paths on Windows.
  const build = join(out, 'b', sha(key).slice(0, 12));
  const log = join(out, 'logs', `${key}.log`);
  mkdirSync(build, {recursive: true});
  const props = ['--build-property', `runtime.tools.stcxx-toolchain.path=${toolchain}`,
    '--build-property', `runtime.tools.stc-cli.path=${dirname(validator)}`,
    '--build-property', 'runtime.ide.version=10607'];
  const args = builder === 'arduino'
    ? ['compile', '--config-file', config, '--fqbn', `stc:mcs51:${board}`, '--jobs', String(jobs),
      ...(cliCache ? ['--json'] : ['--build-path', build]), ...props, c.sketch]
    : [aily, 'compile', join(c.sketch, `${c.name}.ino`), '--board', `stc:mcs51:${board}`,
      '--sdk-path', root, '--tools-path', join(work, 'data/packages'), '--build-path', build, ...props,
      '--archive-cloud-cache', join(work, 'qualification/archive-cache'),
      '--archive-cloud-cache-local-only', '--generate-archive-cloud-cache',
      '--no-fetch-archive-cloud-cache', '--jobs', String(jobs)];
  const r = await execute(builder === 'arduino' ? cli : process.execPath, args,
    builder === 'aily' ? resolve(dirname(aily), '..') : root, log,
    builder === 'arduino' && cliCache ? {
      ARDUINO_BUILD_CACHE_PATH: join(work, 'qc', sha(`${runName}:${board}`).slice(0, 8)),
      ARDUINO_BUILD_CACHE_COMPILATIONS_BEFORE_PURGE: '0',
    } : {});
  let compilerBuild = build;
  if (builder === 'arduino' && cliCache) {
    try {
      const response = JSON.parse(r.output);
      compilerBuild = response.builder_result?.build_path || build;
      r.output = (response.compiler_out || '') + '\n' + (response.compiler_err || '');
    } catch (e) { if (r.code === 0) { r.code = -1; r.output += `\nInvalid CLI JSON: ${e}`; } }
  }
  const item = {builder, board, case: c.id, suite: c.suite, exitCode: r.code, seconds: r.seconds,
    status: r.timedOut ? 'timeout' : r.code === 0 ? 'pass' : classify(r.output),
    build: relative(root, build).replaceAll('\\', '/'), log: relative(root, log).replaceAll('\\', '/')};
  if (item.status === 'pass') {
    try {
      const stem = c.name + (builder === 'arduino' ? '.ino' : '');
      if (compilerBuild !== build) {
        for (const ext of ['.hex', '.mem', '.elf']) copyFileSync(join(compilerBuild, stem + ext), join(build, stem + ext));
        item.compilerBuild = relative(root, compilerBuild).replaceAll('\\', '/');
      }
      const hex = join(build, stem + '.hex');
      item.hex = inspectHex(hex, device.maximum_code_bytes);
      item.memory = memory(join(build, stem + '.mem'));
      if (!item.memory.flash || !item.memory.xdata) throw new Error('Missing memory accounting');
      if (item.memory.flash.used > device.maximum_code_bytes || item.memory.xdata.used > device.xdata_bytes) throw new Error('Memory capacity exceeded');
      const v = await execute(validator, ['validate', '--expect', device.model, '--execution-mode', 'mcs51', '--file', hex], root, log);
      if (v.code !== 0) throw new Error(`stc-cli validation failed: ${v.output}`);
      item.validated = true;
    } catch (e) { item.status = 'artifact-error'; item.error = String(e); }
  } else {
    item.diagnostics = r.output.replace(/\x1b\[[0-9;]*m/g, '').split(/\r?\n/)
      .filter(l => /error:|error \d+|ASlink|not found|Insufficient|undefined|stcxx:/i.test(l)).slice(0, 12);
  }
  results.push(item);
  appendFileSync(join(out, 'results.jsonl'), JSON.stringify(item) + '\n');
  console.log(`[${++completed}/${builders.length * boards.length * cases.length}] ${builder} ${board} ${c.name}: ${item.status} (${r.seconds}s)`);
}
const groups = boards.flatMap(board => builders.map(builder => ({builder, board})));
await Promise.all(Array.from({length: Math.min(workers, groups.length)}, async () => {
  while (groups.length) {
    if (existsSync(join(out, 'STOP'))) break;
    const {builder, board} = groups.shift();
    for (const c of cases) {
      if (existsSync(join(out, 'STOP'))) break;
      if (results.some(r => r.builder === builder && r.board === board && r.case === c.id)) continue;
      await compile(builder, board, c);
    }
  }
}));
const counts = {};
for (const r of results) counts[r.status] = (counts[r.status] || 0) + 1;
const comparison = [];
for (const board of boards) for (const c of cases) {
  const pair = results.filter(r => r.board === board && r.case === c.id);
  if (pair.length === 2) comparison.push({board, case: c.id, sameStatus: pair[0].status === pair[1].status,
    sameImage: pair.every(r => r.hex && r.status === 'pass') ? pair[0].hex.imageSha256 === pair[1].hex.imageSha256 : null});
}
const summary = {started: metadata.started, finished: new Date().toISOString(),
  planned: builders.length * boards.length * cases.length, total: results.length, counts, comparison};
writeFileSync(join(out, 'summary.json'), JSON.stringify(summary, null, 2) + '\n');
writeFileSync(join(out, 'results.json'), JSON.stringify(results, null, 2) + '\n');
console.log(JSON.stringify({...summary, comparison: `${comparison.length} builder pairs`, output: out}, null, 2));
// Capacity limits and explicitly requested boundary probes remain visible failures.
process.exitCode = results.length === summary.planned && results.every(r => r.status === 'pass') ? 0 : 1;
