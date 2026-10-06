#!/usr/bin/env python3
"""M61 package proof. Only processes created here may be closed.
Two copies distinguish the unmodified shipped executable from instrumented
Application hosts used for pre-resume assertions. No test hosts ship in exports.
"""
import os, sys, json, shutil, subprocess, time, importlib.util, hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=Path(sys.argv[1]).resolve(); OUT.mkdir(parents=True,exist_ok=False)
spec=importlib.util.spec_from_file_location('owned_window',ROOT/'scripts/m60_package_smoke.py')
window=importlib.util.module_from_spec(spec);spec.loader.exec_module(window)
env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')}
env.update(SDL_AUDIODRIVER='dummy',JUDAS_WORLD_STATE='none',XDG_DATA_HOME=str(ROOT/'.cache/m61-final-data'))
records=[]
def run(name,args,cwd=ROOT,graphics=False):
 local=dict(env,JUDAS_PROFILE='1',JUDAS_PROFILE_OUTPUT=str(OUT/(name+'-profile.json')))
 if not graphics:local.update(SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1')
 start=time.monotonic();p=subprocess.run([str(x) for x in args],cwd=cwd,env=local,capture_output=True,text=True,timeout=240)
 (OUT/(name+'.log')).write_text(p.stdout+p.stderr)
 records.append({'name':name,'exit_code':p.returncode,'seconds':time.monotonic()-start,'cwd':str(cwd),'command':[str(x) for x in args]})
 (OUT/'COMMANDS.json').write_text(json.dumps(records,indent=2)+'\n');assert not p.returncode,name
 return p.stdout+p.stderr
def readonly(tree):
 for p in tree.rglob('*'):
  if p.is_file():p.chmod(0o555 if os.access(p,os.X_OK) else 0o444)
 for p in sorted([p for p in tree.rglob('*') if p.is_dir()],key=lambda p:len(p.parts),reverse=True):p.chmod(0o555)
 tree.chmod(0o555)
pairs=[('save_lab','judas_save_application_tests'),('streamed_range','judas_save_streaming_tests')]
if '--stream-only' in sys.argv:pairs=pairs[1:]
suffix='_'+OUT.name
for label,target in pairs:
 source=ROOT/'.cache'/('m61-export-'+label+suffix);moved=Path('/tmp')/('Judas_M61_'+label+suffix);instrumented=Path('/tmp')/('Judas_M61_assertions_'+label+suffix)
 assert not source.exists() and not moved.exists() and not instrumented.exists()
 run('export-'+label,[ROOT/'build/judas_export',ROOT/'projects'/label/(label+'.judasproj'),source])
 shutil.move(str(source),moved);shutil.copytree(moved,instrumented)
 shutil.copy2(ROOT/'build'/target,instrumented/target)
 expected={str(p.relative_to(moved)):hashlib.sha256(p.read_bytes()).hexdigest() for p in moved.rglob('*') if p.is_file()}
 assert not any('/Saves/' in '/'+p or p.endswith('.save') for p in expected)
 readonly(moved);readonly(instrumented)
 # This first read deliberately uses the slot written by the LOCAL focused pair:
 # same namespace/content despite exported game.judasproj and moved paths.
 run(label+'-local-to-export-read',[instrumented/target,instrumented/'game.judasproj','read',OUT/(label+'-local-to-export-read')],Path('/tmp'))
 for mode in ['write','read']+(['read-b'] if label=='streamed_range' else []):
  run(label+'-export-'+mode,[instrumented/target,instrumented/'game.judasproj',mode,OUT/(label+'-export-'+mode)],Path('/tmp'))
 # Normal, unmodified exported startup and normal close of precisely our PID.
 local=dict(env,JUDAS_PROFILE='1',JUDAS_PROFILE_OUTPUT=str(OUT/(label+'-shipping-profile.json')))
 local.pop('SDL_VIDEODRIVER',None);local.pop('LIBGL_ALWAYS_SOFTWARE',None)
 with (OUT/(label+'-shipping.log')).open('w') as log:
  process=subprocess.Popen([str(moved/'judas')],cwd='/tmp',env=local,stdout=log,stderr=subprocess.STDOUT)
  try:
   time.sleep(4);assert process.poll() is None;owned=window.close_window(process.pid);code=process.wait(timeout=20)
  finally:
   if process.poll() is None:process.terminate();process.wait(timeout=20)
 text=(OUT/(label+'-shipping.log')).read_text();assert code==0 and 'Project:' in text and 'Asset problem:' not in text and 'script asset=' not in text
 assert expected=={str(p.relative_to(moved)):hashlib.sha256(p.read_bytes()).hexdigest() for p in moved.rglob('*') if p.is_file()}
 records.append({'name':label+'-unmodified-shipping','exit_code':code,'cwd':'/tmp','moved':str(moved),'instrumented_copy':str(instrumented),'readonly':True,'own_closed_window':owned,'package_unchanged':True,'package_bytes':sum(p.stat().st_size for p in moved.rglob('*') if p.is_file())})
 (OUT/'COMMANDS.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps({'pass':True,'records':records},indent=2))
