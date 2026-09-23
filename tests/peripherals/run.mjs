// Execute the production C drivers with emulated SFR/XFR and GPIO boundaries.
import {readFileSync,mkdirSync,writeFileSync} from 'node:fs';
import {resolve,join} from 'node:path';
import {spawnSync} from 'node:child_process';
const args=process.argv.slice(2), opt=(n,d)=>args.includes(n)?args[args.indexOf(n)+1]:d;
const root=resolve(import.meta.dirname,'../..'),out=join(root,'.build/peripheral-host-tests');mkdirSync(out,{recursive:true});
const clang=opt('--clang',join(process.env.LOCALAPPDATA,'Arduino15/packages/stc/tools/stcxx-toolchain/0.3.0/frontend/bin/clang.exe'));
const ld=opt('--ld',join(process.env.USERPROFILE,'.rustup/toolchains/stable-x86_64-pc-windows-msvc/lib/rustlib/x86_64-pc-windows-msvc/bin/gcc-ld/wasm-ld.exe'));
const devices=JSON.parse(readFileSync(join(root,'tools/variants/devices.json'))).devices;
const boards=readFileSync(join(root,'boards.txt'),'utf8').split('\n');
function run(exe,args) { const p=spawnSync(exe,args,{cwd:root,encoding:'utf8',windowsHide:true});if(p.error||p.status)throw new Error(p.error||p.stderr||p.stdout); }
const results=[];
for(const d of devices) for(const kind of ['serial1','serial','spi','wire','interrupts']) {
 const flags=boards.find(l=>l.startsWith(d.id+'.build.core_flags=')).slice((d.id+'.build.core_flags=').length).trim().split(/\s+/);
 const base=['--target=wasm32','-ffreestanding','-O2','-Wall','-Werror','-Wno-unused-variable','-Wno-unused-function','-Wno-unused-parameter','-DF_CPU=12000000UL',...flags,
  '-Itests/peripherals','-Icores/STC','-Icores/STC/hal','-Ilibraries/SPI/src','-Ilibraries/Wire/src','-Ivariants/'+boards.find(l=>l.startsWith(d.id+'.build.variant=')).split('=')[1].trim()];
 if(kind==='spi') {base.push('-DSTC_SPI_HOST_INTERRUPT_HOOKS');if(d.peripherals.spi_count)base.push('-DSTC_SPI_HOST_HARDWARE_HOOKS');}
 if(kind==='wire' && d.peripherals.i2c_count)base.push('-DSTC_WIRE_HOST_HARDWARE_HOOKS');
 const sources=[`tests/peripherals/${kind}.c`,...(kind==='spi'?['libraries/SPI/src/SPI.c']:kind==='wire'?['libraries/Wire/src/Wire.c']:[])];
 const objects=[];
 for(const [i,s]of sources.entries()) {const o=join(out,`${d.id}-${kind}-${i}.o`);objects.push(o);run(clang,[...base,'-c',s,'-o',o]);}
 const wasm=join(out,`${d.id}-${kind}.wasm`);run(ld,['--no-entry','--export=run_tests',...objects,'-o',wasm]);
 const {instance}=await WebAssembly.instantiate(readFileSync(wasm));const line=instance.exports.run_tests();
 if(line)throw new Error(`${d.id}: ${kind}.c:${line}`);
 results.push({board:d.id,test:kind,status:'pass'});console.log(`PASS ${d.id} ${kind}`);
}
writeFileSync(join(out,'results.json'),JSON.stringify({scope:'Emulated registers; not physical hardware',results},null,2)+'\n');
console.log(`${results.length} register-behavior suites passed.`);
