#!/usr/bin/env python3
"""Affected checks after the single frozen M65 gate; never repeats that gate."""
import hashlib,json,os,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'docs/evidence/m65/late-followup'
PROJECT=ROOT/'projects/m65_integration/m65_integration.judasproj'
NATIVE=['rigid_contact','rigid_motion','impact_timing','impact_solver','physics','joint','contact_cache','contact_lifecycle','touch_event','character_motor','fluid_rigid_coupling','fluid_boundary_contact','fluid_cavity_impact','liquid','deformable']
APPLICATION=['character_motor_application','localization_application','streaming_application','audio_application','save_application','save_streaming','deformable_application','collision_application','developer_application']
TARGETS=['judas','judas_editor','judas_export','judasjs_examples_tests','judas_developer_integration_tests','judas_developer_authoring_tests']+['judas_'+x+'_tests' for x in NATIVE+APPLICATION]
def run(name,command,env):
 start=time.perf_counter();log=OUT/(name+'.log')
 with log.open('w') as f:r=subprocess.run(command,cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=300)
 result={'name':name,'command':list(map(str,command)),'exit_code':r.returncode,'seconds':time.perf_counter()-start,'log':str(log.relative_to(ROOT))}
 if Path(command[0]).is_file():result['executable_sha256']=hashlib.sha256(Path(command[0]).read_bytes()).hexdigest()
 print(name,r.returncode,flush=True);return result

def main():
 OUT.mkdir(parents=True,exist_ok=True)
 assert not (OUT/'results.json').exists(),'Use a fresh evidence directory for another correction.'
 env=dict(os.environ,SDL_VIDEODRIVER='offscreen',SDL_AUDIODRIVER='dummy',LIBGL_ALWAYS_SOFTWARE='1',XDG_DATA_HOME=str(ROOT/'.cache/m65-late-data'))
 for k in list(env):
  if k.startswith(('JUDAS_','FTFT_')):del env[k]
 results=[]
 for x in NATIVE:results.append(run(x,[str(ROOT/'build'/('judas_'+x+'_tests'))],env))
 cases=[('core','judas_developer_integration_tests',[str(OUT/'core-run')]),('authoring','judas_developer_authoring_tests',[str(OUT/'authoring')]),('character_motor_application','judas_character_motor_application_tests',[]),('localization_application','judas_localization_application_tests',[str(OUT/'localization')]),('streaming_application','judas_streaming_application_tests',[str(ROOT/'projects/streamed_range/streamed_range.judasproj'),str(OUT/'streaming')]),('audio_application','judas_audio_application_tests',[str(ROOT/'projects/audio_lab/audio_lab.judasproj'),str(OUT/'audio')]),('collision_application','judas_collision_application_tests',[str(ROOT/'projects/collision_lab/collision_lab.judasproj'),'probe',str(OUT/'collision')]),('deformable_application','judas_deformable_application_tests',[str(ROOT/'projects/deformable_lab/deformable_lab.judasproj'),'probe',str(OUT/'deformable')])]
 for stem,project,exe in [('save','save_lab','judas_save_application_tests'),('save-streaming','streamed_range','judas_save_streaming_tests'),('developer',None,'judas_developer_application_tests')]:
  for mode in ['write','read']+(['stream','probe'] if stem=='developer' else []):cases.append((stem+'-'+mode,exe,[str(PROJECT if project is None else ROOT/f'projects/{project}/{project}.judasproj'),mode,str(OUT/(stem+'-'+mode))]))
 for name,exe,args in cases:results.append(run(name,[str(ROOT/'build'/exe)]+args,env))
 record={'original_gate':'docs/evidence/m65/final/RESULTS.json','reason':'sleep TOI ledger, motor acceleration departure, deterministic input fixture migration and strict harness expectations','runs':results,'all_pass':all(r['exit_code']==0 for r in results)}
 (OUT/'results.json').write_text(json.dumps(record,indent=2)+'\n');assert record['all_pass']
if __name__=='__main__':main()
