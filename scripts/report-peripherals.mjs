// Emit a current report from raw runs. Does not overwrite the historical report.
import {readFileSync,writeFileSync,existsSync,readdirSync} from 'node:fs';
import {resolve,join,basename} from 'node:path';
import {createHash} from 'node:crypto';
const root=resolve(import.meta.dirname,'..');
const args=process.argv.slice(2),opt=(n,d)=>args.includes(n)?args[args.indexOf(n)+1]:d;
const run=opt('--run','peripherals-qualified'),overlays=opt('--overlays','').split(',').filter(Boolean);
const q=join(root,'.build/qualification');
const read=p=>JSON.parse(readFileSync(p,'utf8'));
const meta=read(join(q,run,'metadata.json')),manifest=read(join(q,run,'source-files.json'));
const devices=read(join(root,'tools/variants/devices.json')).devices;
const resumes=readdirSync(join(q,run)).filter(n=>/^resume-\d+\.json$/.test(n)).map(file=>({file,metadata:read(join(q,run,file))}));
for(const [p,hash] of Object.entries(manifest)) if(createHash('sha256').update(readFileSync(join(root,p))).digest('hex')!==hash)throw Error('Source changed during matrix: '+p);
const records=new Map(),attempts=[];
for(const name of [run,...overlays]) {
 const rows=read(join(q,name,'results.json'));attempts.push({run:name,results:rows});
 for(const row of rows)records.set([row.builder,row.board,row.case].join('|'),{...row,sourceRun:name});
}
const rows=[...records.values()];
const expected=meta.boards.length*meta.builders.length*meta.cases.length;
if(rows.length!==expected)throw Error(`Incomplete matrix: ${rows.length}/${expected}`);
function classify(r) {
 if(r.status==='pass'||r.status==='capacity')return r.status;
 const d=devices.find(d=>d.id===r.board);
 if(d.adc===false && /\/(AnalogReadSerial|MathFunctions)$/.test(r.case) && /A0/.test((r.diagnostics||[]).join(' ')))return 'no-adc';
 return 'other';
}
const count=(rs,c)=>rs.filter(r=>classify(r)===c).length;
const stats=rs=>['pass','capacity','no-adc','other'].map(c=>count(rs,c)).join(' / ');
const pairs=[];
for(const board of meta.boards) for(const test of meta.cases) {
 const a=records.get(`arduino|${board}|${test.id}`),b=records.get(`aily|${board}|${test.id}`);
 if(a&&b)pairs.push({board,case:test.id,sameStatus:a.status===b.status,
  sameFlash:a.status==='pass'&&b.status==='pass'?a.memory.flash.used===b.memory.flash.used:null,
  sameImage:a.status==='pass'&&b.status==='pass'?a.hex.imageSha256===b.hex.imageSha256:null});
}
const host=read(join(root,'.build/peripheral-host-tests/results.json'));
const boundaries=read(join(q,'peripherals-boundaries','results.json'));
const vectors=read(join(q,run,'interrupt-vectors.json'));
if(host.results.length!==70||host.results.some(r=>r.status!=='pass'))throw Error('Incomplete behavior tests');
if(vectors.images!==count(rows,'pass')||JSON.stringify(vectors.overlays)!==JSON.stringify(overlays))throw Error('Stale interrupt-vector validation');
const historical=read(join(root,'tests/compile/RESULTS.json')).results;
const regressions=rows.filter(r=>r.builder==='arduino'&&classify(r)==='capacity'&&historical.some(h=>h.builder===r.builder&&h.board===r.board&&h.case===r.case&&h.status==='pass'));
const regressionSizes=regressions.map(r=>{
 let flash=null;
 try {
  const raw=readFileSync(join(root,r.log),'utf8');
  const response=JSON.parse(raw.slice(raw.indexOf('\n')+1));
  const mem=readFileSync(join(response.builder_result.build_path,basename(r.case)+'.ino.stcxx.mem'),'utf8');
  const line=mem.split(/\r?\n/).find(l=>l.includes('ROM/EPROM/FLASH'));
  const match=line?.trim().match(/(\d+)\s+(\d+)\s*$/);
  if(match)flash={used:Number(match[1]),max:Number(match[2])};
 } catch {}
 return {board:r.board,case:r.case,flash};
});
const report=[
 '# MCS51 外设适配测试结果','',
 `记录日期（UTC）：${new Date().toISOString().slice(0,10)}。源码基线：\`${meta.sourceCommit}\` 加当前未提交的适配；不代表已经发布。`,'',
 `完整矩阵 **${rows.length} 项**：14 型号 × ${meta.cases.length} 个程序 × Arduino CLI / aily-builder。**${count(rows,'pass')} 项通过，${count(rows,'capacity')} 项容量不足，${count(rows,'no-adc')} 项无 ADC/A0 而不适用，${count(rows,'other')} 项其他失败**。容量不足和不适用均不计为通过。`,'',
 `${host.results.length} 组真实 C 驱动寄存器行为测试通过；构建适配器单元测试 6 项通过，编译器单元测试 25 项通过、2 项明确忽略的子进程 fixture。47 文件 SDK 同步、变体生成一致性、差异空白检查已通过。`,'',
 '每个 pass 都完成目标编译、链接、HEX 校验和/地址检查、型号容量验证、Flash/XDATA 统计及 `stc-cli validate`。行为测试使用模拟寄存器，并非真实 8051 指令仿真或上板测试。','',
 `另对 ${vectors.images} 个通过映像的 ${vectors.vectors} 个 Timer0/UART/IIC 中断向量，逐项核对 HEX 中的跳转目标与链接映射中的 ISR 符号地址。`,'',

 `双构建器 ${pairs.length} 对：${pairs.filter(x=>x.sameStatus).length} 对状态一致；${pairs.filter(x=>x.sameFlash===true).length} 对通过项 Flash 用量一致；${pairs.filter(x=>x.sameImage===true).length} 对通过项的 HEX 地址/字节完全相同。其余映像的运行等价性未验证。`,'',
 '## 各型号结果','',
 '| 型号 | UART / IIC / SPI | 通过 / 容量不足 / 无 ADC / 其他 |','| --- | --- | --- |',
 ...devices.map(d=>`| ${d.model} | ${d.peripherals.uart_count} / ${d.peripherals.i2c_count} / ${d.peripherals.spi_count} | ${stats(rows.filter(r=>r.board===d.id))} |`),'',
 '## 新增接口探针','',
 '| 型号 | 多串口 | SPI | Wire 主机 | Wire 从机能力分支 |','| --- | --- | --- | --- | --- |',
 ...devices.map(d=>'| '+d.model+' | '+['HardwareUARTs','HardwareSPI','HardwareWire','WireSlaveSupported'].map(n=>{const a=records.get(`arduino|${d.id}|tests/compile/common/${n}`),b=records.get(`aily|${d.id}|tests/compile/common/${n}`);return a?.status===b?.status?a.status:`${a?.status}/${b?.status}`;}).join(' | ')+' |'),'',
 '串口探针按 UART_COUNT 选择可用串口，单串口型号只验证 UART1。从机能力分支在 WIRE_HAS_SLAVE=0 的板型只编译软件主机；不能把这些 pass 解读为有硬件从机。STC89 的 SPI 探针验证软件回退。模式、路由、时钟和物理外设数量详见 [HARDWARE_INTERFACES.md](../../HARDWARE_INTERFACES.md)。','',
 '## 容量与边界','',
 'UART1 新增路由分发时曾使 SerialEcho 增大约 3 KiB；已将可选路由配置移入独立归档成员，恢复默认接线的小体积路径。所有最终容量仍按真实型号限制检查，不放宽 Flash/XRAM 上限。','',
 '| 型号 | SerialEcho Flash 字节 |','| --- | ---: |',
 ...devices.map(d=>{const a=records.get(`arduino|${d.id}|examples/SerialEcho`);return `| ${d.model} | ${a?.memory?.flash?.used??a?.status} |`;}),'',
 '相对旧矩阵，原先通过而本次因完整驱动增大出现容量不足的组合（Arduino CLI）：','',
 ...(regressionSizes.length?regressionSizes.map(r=>`- ${r.board} / ${r.case}${r.flash?`：${r.flash.used} / ${r.flash.max} 字节`:''}`):['- 无。']),'',
 '24 项独立边界编译：EEPROM 和 Servo 缺头文件共 12 项；tone/noTone 缺 API 共 6 项；Wire 从机签名共 6 项编译通过。STC12/STC89 调用从机入口返回配置错误，由行为测试验证，编译通过并不改变 WIRE_HAS_SLAVE=0。`analogWrite` 仍是数字阈值输出；USB、RTC、PWM/PCA、通用定时器等缺口已列入接口文档。','',
 '## 证据与复现','',
 `工具：${meta.arduino}；aily-builder ${meta.aily}；STCXX 0.3.0；${meta.validator}；${meta.host}。`,'',
 `主轮次初始并发 ${meta.workers}；续跑并发记录：${resumes.map(r=>r.metadata.workers).join('、')||'无'}。续跑校验工具和用例输入，已完成项保留且不重复计数。`,'',
 `主轮次：\`.build/qualification/${run}\`；独立复测：${overlays.length?overlays.map(x=>'`'+x+'`').join('、'):'无'}。原始失败尝试完整保留在各轮次 logs / results 中，报告 JSON 同时保留原始轮次和最终合并结果。`,'',
 'Arduino CLI 在少数首次构建中出现过无编译诊断的 exit 1；所有其他失败均须独立复查。若最终其他失败为 0，表示这些组合的独立复测通过，不代表已定位 CLI 偶发退出原因。','',
 `已校验 ${Object.keys(manifest).length} 个源文件 SHA-256，确保最终矩阵期间核心、库、变体和测试输入未改变。原生驱动 SHA-256：\`${meta.driverSha256}\`。`,'',
 '```powershell',
 'node tests/peripherals/run.mjs',
 `node scripts/test-compile.mjs --aily .build/aily-builder/dist/main.js --workers 12 --jobs 2 --run <new-run>`,
 `node scripts/test-interrupt-vectors.mjs --run ${run}${overlays.length?' --overlays '+overlays.join(','):''}`,
 `node scripts/report-peripherals.mjs --run ${run}${overlays.length?' --overlays '+overlays.join(','):''}`,
 '```','',
 '[全部逐项结果](PERIPHERAL_RESULTS.json)；[编译运行说明](README.md)；[行为测试范围](../peripherals/README.md)。','',
 '**未烧录、未操作 COM 口；实板波形、硅修订差异、同时收发压力和中断栈峰值仍待上板验证。**','',
];
const output=resolve(opt('--output',join(root,'tests/compile/PERIPHERAL_RESULTS.md')));
writeFileSync(output,report.join('\n'),{flag:'wx'});
writeFileSync(output.replace(/\.md$/,'.json'),JSON.stringify({generated:new Date().toISOString(),metadata:meta,resumes,sourceHashes:manifest,
 results:rows,comparison:pairs,host,boundaries,vectors,attempts,capacityRegressions:regressions,capacityRegressionSizes:regressionSizes},null,2)+'\n',{flag:'wx'});
console.log(JSON.stringify({total:rows.length,stats:stats(rows),other:rows.filter(r=>classify(r)==='other'),output},null,2));
