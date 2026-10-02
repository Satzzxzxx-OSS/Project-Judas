#!/usr/bin/env python3
"""Narrow rerun after the no-script bridge guard. Original gate kept intact."""
from pathlib import Path
import subprocess,os,json,time,hashlib,shutil
ROOT=Path(__file__).resolve().parents[4]
OUT=Path(__file__).resolve().parent
os.chdir(ROOT)
env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')}
env.update(SDL_VIDEODRIVER='offscreen',SDL_AUDIODRIVER='dummy',LIBGL_ALWAYS_SOFTWARE='1',JUDAS_WORLD_STATE='none')
result={'scope':'narrow no-script event bridge guard; no repeated full suite','commands':[]}
def run(name,command,extra=None):
 start=time.monotonic();p=subprocess.run(command,env=dict(env,**(extra or {})),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 (OUT/(name+'.log')).write_text(p.stdout)
 result['commands'].append({'name':name,'command':command,'exit_code':p.returncode,'seconds':time.monotonic()-start})
 (OUT/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n')
 if p.returncode:raise SystemExit(name+' failed')
 return p.stdout
run('build',['cmake','--build','build','--target','judas_lifecycle_tests','judas_touch_event_tests','judas_touch_application_tests','judas_editor','judas','-j','4'])
run('lifecycle',['./build/judas_lifecycle_tests'])
run('touch',['./build/judas_touch_event_tests','--output',str(OUT/'fixtures')])
run('application',['./build/judas_touch_application_tests','--output',str(OUT/'application')])
project=ROOT/'build/m42-editor-project';shutil.copytree(ROOT/'projects/touch_demo',project,dirs_exist_ok=True)
text=run('editor',['./build/judas_editor',str(project/'touch_demo.judasproj')],{'JUDAS_EDITOR_AUTOTEST':str(OUT/'editor')})
assert 'authored scene after play/stop is IDENTICAL' in text
script=OUT/'steps.txt';script.write_text('STEPS 3\nLOG_EVERY 1\n')
run('standalone',['./build/judas','projects/touch_demo/touch_demo.judasproj'],{'JUDAS_TEST_SCRIPT':str(script)})
changed=subprocess.check_output(['git','diff','--name-only'],text=True).splitlines()+subprocess.check_output(['git','ls-files','--others','--exclude-standard'],text=True).splitlines()
source={p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in sorted(set(changed)) if Path(p).is_file() and (p.startswith(('src/','tests/','scripts/','projects/touch_demo/')) or p=='CMakeLists.txt')}
(OUT/'SOURCE_SHA256.json').write_text(json.dumps(source,indent=2)+'\n')
result.update(overall_pass=True,human_visual_validation='PENDING; operator authority')
(OUT/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
