#!/usr/bin/env python3
"""Unmodified read-only shipping export, unrelated cwd, owned-window close only."""
import os,sys,subprocess,time,json,hashlib,importlib.util
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
def module(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def main():
 package=Path(sys.argv[1]).resolve();out=Path(sys.argv[2]).resolve();out.mkdir(parents=True,exist_ok=False)
 metrics=module('m64_metrics',ROOT/'scripts/m64_focused.py');windows=module('owned_window',ROOT/'scripts/m60_package_smoke.py')
 expected={str(p.relative_to(package)):hashlib.sha256(p.read_bytes()).hexdigest() for p in package.rglob('*') if p.is_file()}
 for p in package.rglob('*'):
  if p.is_file():p.chmod(0o555 if os.access(p,os.X_OK) else 0o444)
 for p in sorted([p for p in package.rglob('*') if p.is_dir()],key=lambda p:len(p.parts),reverse=True):p.chmod(0o555)
 package.chmod(0o555)
 capture=ROOT/'.cache'/('m64-'+out.name+'-shipping.json')
 env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')};env.update(SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_DATA_HOME=str(ROOT/'.cache'/('m64-'+out.name+'-data')),JUDAS_WORLD_STATE='none',JUDAS_PROFILE='1',JUDAS_PROFILE_OUTPUT=str(capture))
 with (out/'shipping.log').open('w') as log:
  process=subprocess.Popen([str(package/'judas')],cwd='/tmp',env=env,stdout=log,stderr=subprocess.STDOUT)
  try:
   time.sleep(5);assert process.poll() is None;owned=windows.close_window(process.pid);code=process.wait(timeout=20)
  finally:
   if process.poll() is None:process.terminate();process.wait(timeout=20)
 text=(out/'shipping.log').read_text();assert code==0 and 'Project:' in text and 'Asset problem:' not in text and 'script asset=' not in text
 assert expected=={str(p.relative_to(package)):hashlib.sha256(p.read_bytes()).hexdigest() for p in package.rglob('*') if p.is_file()}
 profile=metrics.measurements(capture);profile['graphics']='native X11 desktop GL, unmodified visible shipping executable'
 frames=json.loads(capture.read_text())['frames'];intervals=sorted(f['interval_ns']/1e6 for f in frames if f['id']>40 and not f['startup'])
 assert intervals,'shipping capture needs completed real-time frames'
 profile['frame_interval']={'samples':len(intervals),'median_ms':intervals[len(intervals)//2],'p95_ms':intervals[min(len(intervals)-1,int(len(intervals)*.95))],'maximum_ms':intervals[-1],'approximate_median_fps':1000/intervals[len(intervals)//2]}
 profile['fixed_cap_reached_frames']=sum(f['fixed']['cap_reached'] for f in frames)
 result={'pass':True,'package':str(package),'cwd':'/tmp','readonly_package_unchanged':True,'closed_only_own_window':owned,'shipping_exit_code':code,'shipping':profile,'human_visual_acceptance':'PENDING operator'}
 (out/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'pass':True,'package':str(package),'shipping_frame':result['shipping']['frame']},indent=2))
if __name__=='__main__':main()
