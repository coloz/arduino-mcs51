import {readFileSync,writeFileSync,mkdirSync,existsSync} from 'node:fs';
import {resolve,dirname,join,sep} from 'node:path';
import {fileURLToPath} from 'node:url';
export const root=resolve(dirname(fileURLToPath(import.meta.url)),'..');
export function syncCore({check=false}={}) {
  const manifest=JSON.parse(readFileSync(join(root,'cores/STC/sdk-sources.json')));
  let count=0,changed=0;
  for(const group of [manifest.shared_runtime,manifest.shared_arduino_api]) {
    const source=resolve(root,group.source),destination=resolve(root,group.destination);
    if(!destination.startsWith(root+sep))throw Error('Destination outside platform');
    for(const name of group.files) {
      if(!/^(?:(?:include|src)\/)?[\w.]+$/.test(name))throw Error('Invalid inventory path: '+name);
      const data=readFileSync(join(source,name)),target=join(destination,name);
      count++;
      if(existsSync(target)&&readFileSync(target).equals(data))continue;
      if(check)throw Error('Shared source differs: '+target);
      mkdirSync(dirname(target),{recursive:true});writeFileSync(target,data);changed++;
    }
  }
  console.log(`PASS: ${count} shared SDK/API files; ${changed} synchronized`);
}
if(process.argv[1]&&resolve(process.argv[1])===fileURLToPath(import.meta.url))syncCore({check:process.argv.includes('--check')});
