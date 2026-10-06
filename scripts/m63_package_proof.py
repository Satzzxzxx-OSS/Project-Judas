#!/usr/bin/env python3
"""Native desktop proof; closes only the PID launched here.
The export stays unmodified and becomes read-only. Assertion hosts never ship.
"""
import os,sys,subprocess,time,json,shutil,hashlib,importlib.util
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
def module(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def main():
 package=Path(sys.argv[1]).resolve();out=Path(sys.argv[2]).resolve();out.mkdir(parents=True,exist_ok=False)
 focused=module('m63_measurements',ROOT/'scripts/m63_focused.py');windows=module('owned_window',ROOT/'scripts/m60_package_smoke.py')
 capture=ROOT/'.cache'/('m63-'+out.name);assert not capture.exists()
 env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')};env.update(SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_DATA_HOME=str(capture.parent/(capture.name+'-data')),JUDAS_WORLD_STATE='none')
 p=subprocess.run([str(ROOT/'build/judas_fracture_application_tests'),str(package/'game.judasproj'),'probe',str(capture)],cwd='/tmp',env=env,capture_output=True,text=True,timeout=180)
 (out/'native-application.log').write_text(p.stdout+p.stderr);assert p.returncode==0
 measurements=focused.measurements(capture/'profile.json');measurements['graphics']='native X11 desktop OpenGL driver; hidden assertion window';shutil.copy2(capture/'lab.png',out/'lab.png')
 expected={str(p.relative_to(package)):hashlib.sha256(p.read_bytes()).hexdigest() for p in package.rglob('*') if p.is_file()}
 for p in package.rglob('*'):
  if p.is_file():p.chmod(0o555 if os.access(p,os.X_OK) else 0o444)
 for p in sorted([p for p in package.rglob('*') if p.is_dir()],key=lambda p:len(p.parts),reverse=True):p.chmod(0o555)
 package.chmod(0o555)
 profile=capture.parent/(capture.name+'-shipping.json')
 env.update(JUDAS_PROFILE='1',JUDAS_PROFILE_OUTPUT=str(profile))
 with (out/'shipping.log').open('w') as log:
  process=subprocess.Popen([str(package/'judas')],cwd='/tmp',env=env,stdout=log,stderr=subprocess.STDOUT)
  try:
   time.sleep(5);assert process.poll() is None;owned=windows.close_window(process.pid);code=process.wait(timeout=20)
  finally:
   if process.poll() is None:process.terminate();process.wait(timeout=20)
 text=(out/'shipping.log').read_text();assert code==0 and 'Project:' in text and 'Asset problem:' not in text and 'script asset=' not in text
 assert expected=={str(p.relative_to(package)):hashlib.sha256(p.read_bytes()).hexdigest() for p in package.rglob('*') if p.is_file()}
 shipping=focused.measurements(profile);shipping['graphics']='native X11 unmodified shipping executable'
 result={'pass':True,'package':str(package),'cwd':'/tmp','readonly_package_unchanged':True,'closed_only_own_window':owned,'shipping_exit_code':code,'application':measurements,'shipping':shipping,'human_visual_acceptance':'PENDING operator'}
 (out/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'pass':True,'package':str(package),'application_frame':measurements['frame'],'shipping_frame':shipping['frame']},indent=2))
if __name__=='__main__':main()
