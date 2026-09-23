// Check actual linked MCS51 vector destinations against SDCC map symbols.
import {readFileSync,writeFileSync,existsSync} from 'node:fs';
import {resolve,join,basename} from 'node:path';
const root=resolve(import.meta.dirname,'..');
const args=process.argv.slice(2),opt=(n,d)=>args.includes(n)?args[args.indexOf(n)+1]:d;
const run=opt('--run','peripherals-qualified');
const overlays=opt('--overlays','').split(',').filter(Boolean);
const q=join(root,'.build/qualification');
const read=p=>JSON.parse(readFileSync(p,'utf8'));
const devices=read(join(root,'tools/variants/devices.json')).devices;
const records=new Map();
for(const name of [run,...overlays]) {
  const final=join(q,name,'results.json');
  const rows=existsSync(final)?read(final):readFileSync(join(q,name,'results.jsonl'),'utf8').trim().split('\n').filter(Boolean).map(JSON.parse);
  for(const r of rows) records.set([r.builder,r.board,r.case].join('|'),r);
}
function hexBytes(path) {
  const bytes=new Map();let base=0;
  for(const line of readFileSync(path,'utf8').trim().split(/\r?\n/)) {
    const b=Buffer.from(line.slice(1),'hex'),address=b.readUInt16BE(1);
    if(b[3]===0) for(let i=0;i<b[0];i++) bytes.set(base+address+i,b[4+i]);
    else if(b[3]===2||b[3]===4) base=b.readUInt16BE(4)*(b[3]===2?16:65536);
  }
  return bytes;
}
const results=[];
for(const r of records.values()) if(r.status==='pass') {
  const p=devices.find(d=>d.id===r.board).peripherals;
  const stem=basename(r.case)+(r.builder==='arduino'?'.ino':'');
  const mapPath=join(root,r.compilerBuild||r.build,stem+'.stcxx.map');
  const map=readFileSync(mapPath,'utf8');
  const bytes=hexBytes(join(root,r.build,stem+'.hex'));
  const vectors=[['stc_timer0_isr',1],['stc_uart1_isr',4]];
  if(p.uart_count>=2)vectors.push(['stc_uart2_isr',8]);
  if(p.uart_count>=4)vectors.push(['stc_uart3_isr',17],['stc_uart4_isr',18]);
  if(p.i2c_count)vectors.push(['stc_wire_slave_isr',24]);
  const checked=[];
  for(const [symbol,number]of vectors) {
    const m=map.match(new RegExp('C:\\s+([0-9A-Fa-f]+)\\s+_'+symbol+'\\b'));
    if(!m)throw Error(`Missing ISR symbol: ${r.board}/${r.case}/${symbol}`);
    const address=8*number+3,target=parseInt(m[1],16);
    if(bytes.get(address)!==2 || bytes.get(address+1)!==(target>>8) || bytes.get(address+2)!==(target&255))
      throw Error(`Incorrect vector: ${r.builder}/${r.board}/${r.case}/${symbol}`);
    checked.push({symbol,number,address,target});
  }
  results.push({builder:r.builder,board:r.board,case:r.case,checked});
}
const output=join(q,run,opt('--output','interrupt-vectors.json'));
writeFileSync(output,JSON.stringify({run,overlays,images:results.length,vectors:results.reduce((n,r)=>n+r.checked.length,0),results},null,2)+'\n',{flag:'wx'});
console.log(`${results.length} images: interrupt vectors match their linked ISR symbols. ${output}`);
