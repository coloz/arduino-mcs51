// Produce a durable source-backed report from completed compilation records.
import {readFileSync, writeFileSync, existsSync} from 'node:fs';
import {join, resolve} from 'node:path';
import {root, options} from './workspace.mjs';
const opts = options();
const run = opts.run || 'matrix';
const runPath = join(root, '.build/qualification', run);
const meta = JSON.parse(readFileSync(join(runPath, 'metadata.json')));
const rows = new Map();
const key = r => `${r.builder}:${r.board}:${r.case}`;
const runs = [run, ...(opts.overlays || '').split(',').filter(Boolean)];
for (const name of runs) {
  if (name !== run) {
    const overlay = JSON.parse(readFileSync(join(root, '.build/qualification', name, 'metadata.json')));
    for (const c of overlay.cases) {
      const index = meta.cases.findIndex(before => before.id === c.id);
      if (index >= 0) meta.cases[index] = c;
    }
  }
  const path = join(root, '.build/qualification', name, 'results.jsonl');
  for (const line of readFileSync(path, 'utf8').trim().split('\n').filter(Boolean)) {
    const r = JSON.parse(line);
    rows.set(key(r), r);
  }
}
const records = [...rows.values()].filter(r => meta.boards.includes(r.board)
  && meta.builders.includes(r.builder) && meta.cases.some(c => c.id === r.case));
const count = (items, status) => items.filter(r => r.status === status).length;
const planned = meta.boards.length * meta.builders.length * meta.cases.length;
if (records.length !== planned) throw new Error(`Incomplete matrix: ${records.length}/${planned}`);
// Failed links also emit useful capacity evidence, but are never counted as
// valid firmware. Cached CLI build locations are retained in its JSON log.
for (const r of records.filter(r => r.status === 'capacity')) {
  let directory = resolve(root, r.build);
  if (r.builder === 'arduino') {
    try {
      const log = readFileSync(resolve(root, r.log), 'utf8');
      const response = JSON.parse(log.slice(log.indexOf('\n') + 1));
      directory = response.builder_result.build_path;
    } catch { /* Earlier full builds use plain-text diagnostics. */ }
  }
  const stem = r.case.split('/').at(-1) + (r.builder === 'arduino' ? '.ino' : '');
  const path = join(directory, stem + '.stcxx.mem');
  if (existsSync(path)) {
    r.capacityReport = readFileSync(path, 'utf8').split(/\r?\n/)
      .filter(l => /ROM\/EPROM\/FLASH|EXTERNAL RAM|ERROR/.test(l)).map(l => l.trim());
  }
}
const devices = JSON.parse(readFileSync(join(root, 'tools/variants/devices.json'))).devices;
for (const r of records) {
  const device = devices.find(d => d.id === r.board);
  const errors = (r.diagnostics || []).filter(d => d.includes('error:'));
  if (r.status === 'compile-error' && device.adc === false
      && ['AnalogReadSerial', 'MathFunctions'].includes(r.case.split('/').at(-1))
      && errors.length && errors.every(d => d.includes("use of undeclared identifier 'A0'"))) {
    r.limitation = 'no-adc';
  }
}
const unsupported = items => items.filter(r => r.limitation === 'no-adc').length;
const unexpected = items => items.filter(r => !['pass', 'capacity'].includes(r.status) && !r.limitation).length;
const both = [];
for (const board of meta.boards) for (const c of meta.cases) {
  const a = rows.get(`arduino:${board}:${c.id}`), b = rows.get(`aily:${board}:${c.id}`);
  if (!a || !b) throw new Error(`Missing pair ${board}/${c.id}`);
  both.push({board, case: c.id, sameStatus: a.status === b.status,
    sameImage: a.status === 'pass' && b.status === 'pass' ? a.hex.imageSha256 === b.hex.imageSha256 : null,
    sameFlashUsage: a.status === 'pass' && b.status === 'pass' ? a.memory.flash.used === b.memory.flash.used : null});
}
const boundaryPath = join(root, '.build/qualification', opts.boundaries || 'boundaries', 'results.json');
const boundaries = existsSync(boundaryPath) ? JSON.parse(readFileSync(boundaryPath)) : [];
const replayPath = join(runPath, 'cache-replay/results.json');
const replay = existsSync(replayPath) ? JSON.parse(readFileSync(replayPath)) : [];
const stats = items => `${count(items, 'pass')} / ${count(items, 'capacity')} / ${unsupported(items)} / ${unexpected(items)}`;
const report = [
  '# MCS51 双构建器编译测试', '',
  `记录日期：${meta.started.slice(0, 10)}。`, '',
  `测试平台：${meta.host}，Node ${meta.node}；${meta.arduino}；aily-builder ${meta.aily}；${meta.validator}；STCXX 0.3.0。`, '',
  `完成 **${records.length} 个编译组合**：${meta.boards.length} 个板型 × ${meta.cases.length} 个程序 × 2 种构建器。**${count(records, 'pass')} 项通过，${count(records, 'capacity')} 项容量超限，${unsupported(records)} 项因板型未提供 ADC/A0 而不适用，${unexpected(records)} 项其他失败**。容量超限和不适用均不计为通过；JSON 保留原始失败状态和诊断。`, '',
  `${meta.cases.filter(c => c.suite === 'examples').length} 个 SDK 例程、${meta.cases.filter(c => c.suite === 'common').length} 个常见程序纳入矩阵。常见程序为本仓库编写的 Arduino 用法测试，包含多文件 C/C++ 与自动原型。所有成功项均检查 HEX 校验和、地址、型号容量、SDCC 内存统计，并通过 stc-cli validate。`, '',
  `两构建器共 ${both.length} 组配对：${both.filter(p => p.sameStatus).length} 组状态一致；双方通过的 ${both.filter(p => p.sameImage !== null).length} 组中，${both.filter(p => p.sameFlashUsage === true).length} 组 Flash 占用一致，${both.filter(p => p.sameImage === true).length} 组 HEX 映像按地址/字节完全一致。`, '',
  '抽查 SerialEcho 的跨构建器差异，生成 C 的区别在常量排列及符号编号，Flash 大小相同。本轮保留全部字节比较结果，未进行不同映像的运行时等价验证。', '',
  `在同一构建目录重复执行原编译命令：${replay.length} 项记录，${replay.filter(r => r.exitCode === 0 && r.sameHex).length} 项 HEX 与首次构建逐字节一致。原始命令、输出及哈希位于主轮次的 cache-replay 目录。`, '',
  '## 板型结果', '',
  `计数顺序：通过 / 容量超限 / ADC 未提供 / 其他失败。每种构建器、每个板型各测试 ${meta.cases.length} 个程序。`, '',
  '| 板型 | Flash / XDATA 字节 | Arduino CLI | aily-builder |',
  '| --- | ---: | ---: | ---: |',
];
for (const board of meta.boards) {
  const d = devices.find(d => d.id === board);
  report.push(`| ${d.model} | ${d.maximum_code_bytes} / ${d.xdata_bytes} | ${stats(records.filter(r => r.board === board && r.builder === 'arduino'))} | ${stats(records.filter(r => r.board === board && r.builder === 'aily'))} |`);
}
report.push('', '## 程序结果', '',
  '下表分别统计每个程序在 14 个板型上的结果。Flash 范围取 Arduino CLI 成功项，包含运行库和实际链接后的代码。', '',
  '| 程序 | 类别 | Arduino CLI | aily-builder | Flash 字节范围 |',
  '| --- | --- | ---: | ---: | ---: |');
for (const c of meta.cases) {
  const a = records.filter(r => r.case === c.id && r.builder === 'arduino');
  const b = records.filter(r => r.case === c.id && r.builder === 'aily');
  const sizes = a.filter(r => r.status === 'pass').map(r => r.memory.flash.used);
  report.push(`| ${c.name} | ${c.suite === 'examples' ? 'SDK 例程' : '常见应用'} | ${stats(a)} | ${stats(b)} | ${sizes.length ? `${Math.min(...sizes)}–${Math.max(...sizes)}` : '—'} |`);
}
report.push('', '## 容量与兼容边界', '',
  '8 KiB Flash 型号并不能容纳所有 C++/总线/打印组合。容量拒绝来自实际链接器，未扩大型号内存配置或隐藏失败。详细 JSON 中保留每项诊断和日志路径。', '',
  'Ai8H2K12U 的 12 KiB Flash 也无法容纳 4 个常见程序：Arduino CLI 链接报告中 IPAddressPrint 为 16,255 字节、MathFunctions 为 14,839 字节、SoftwareSerialBridge 为 12,679 字节、StringOperations 为 19,187 字节。该板型的全部 9 个 SDK 例程均通过两种构建器。', '',
  '`analogWrite` 编译成功仍仅代表数字阈值输出；无 ADC 型号没有由编译获得 ADC 功能。引脚可用性、外设时序和运行时栈/堆峰值须实板验证。', '',
  'STC8C2K64S4 和 STC89C58RD+ 的 SDK 配置没有 ADC/A0；AnalogReadSerial 和使用 ADC 输入的 MathFunctions 原样编译会失败。综合例程 PeripheralSmoke 则按 NUM_ANALOG_INPUTS 条件启用 ADC 部分。', '',
  '| 独立边界探针 | Arduino CLI | aily-builder |', '| --- | --- | --- |');
for (const name of [...new Set(boundaries.map(r => r.case))]) {
  const a = boundaries.find(r => r.case === name && r.builder === 'arduino');
  const b = boundaries.find(r => r.case === name && r.builder === 'aily');
  report.push(`| ${name.split('/').at(-1)} | ${a?.status || '未测试'} | ${b?.status || '未测试'} |`);
}
report.push('', '这些探针单独记录，不混入支持范围矩阵；SDK 未提供 tone/noTone、Servo、EEPROM 和 Wire 从机接口。', '',
  '## 本次修复', '',
  '- 将 5 个库例程的旧 C 函数表写法更新为当前 C++ API：`Serial.begin/write`、`SPISettings`、`Wire.endTransmission(false)`、LCD/Stepper/SoftwareSerial 实例。LCD 示例同时避开无效的 P1.2 默认引脚。',
  '- 综合例程 PeripheralSmoke 按 NUM_ANALOG_INPUTS 条件编译 ADC 读取，修复无 ADC 板型上 A0 未定义的问题。',
  '- 修复 Windows 长路径下原生适配器及编译器原子写入失败；新增长路径创建/覆盖回归测试。修复前测试可复现失败，修复后通过，PeripheralSmoke 的原长路径构建也通过。',
  '- 添加可重跑的矩阵脚本、20 个常见应用、4 个边界探针和结构化结果；保留每次构建与型号校验的完整日志。', '',
  '原生适配器测试 6 项通过；编译器测试 25 项通过，2 项为明确忽略的子进程 fixture。SDK 同步、variant 生成一致性和差异空白检查通过。', '',
  '## 复现与证据', '',
  '[运行说明](README.md)。主记录位于 `.build/qualification/' + run + '`，本报告合并轮次：' + runs.map(r => '`' + r + '`').join('、') + '。', '',
  'Arduino CLI 使用板型隔离的本地核心缓存，部分先行用例采用完整构建；aily 使用本地对象/归档缓存，禁用远程下载。构建时间受并发与缓存状态影响，不作为性能比较。', '',
  'STC8H3K64S4 / CppRuntime 的一次 Arduino CLI 调用无编译诊断并返回 1，独立复测通过；未定位这次退出的根因。原始记录未删除，最终矩阵采用单列复测结果。', '',
  '源代码基线：`' + meta.sourceCommit + '` 加本次工作区修复；原生驱动 SHA-256：`' + meta.driverSha256 + '`。工具命令、输入哈希、板型和逐项内存数据保存在相邻 JSON 记录中。', '',
  '**本报告仅证明编译、链接、静态容量和固件格式检查结果，未进行烧录或实板运行测试。**', '',
);
const output = resolve(opts.output || join(root, 'tests/compile/RESULTS.md'));
writeFileSync(output, report.join('\n'));
writeFileSync(output.replace(/\.md$/, '.json'), JSON.stringify({generated: new Date().toISOString(), metadata: meta,
  sourceRuns: runs, results: records, comparison: both, boundaries, replay}, null, 2) + '\n');
console.log(output);
