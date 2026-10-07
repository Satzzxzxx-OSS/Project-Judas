#!/usr/bin/env python3
"""Focused repair checks through the existing application and test harness."""
import importlib.util,json,os,subprocess,sys,tempfile,shutil
from pathlib import Path
R=Path(__file__).resolve().parents[1];sys.dont_write_bytecode=True
spec=importlib.util.spec_from_file_location('fixtures',R/'scripts/post_m65_review.py');f=importlib.util.module_from_spec(spec);spec.loader.exec_module(f)
mode=sys.argv[1];name=sys.argv[2];binary=Path(sys.argv[3]).resolve();out=R/'docs/evidence/post_m65_repairs/application'/name;out.mkdir(parents=True,exist_ok=False)
with tempfile.TemporaryDirectory(prefix='judas-repair-app-') as tmp:
 p=Path(tmp)/'project';shutil.copytree(f.P,p)
 if mode.startswith('resident-'):
  # Identical accepted resident content for both binaries. Ragdoll adaptation is a separate proof.
  original=subprocess.check_output(['git','show','26e6f089fde0f39d68659882f6204da3a453473e:projects/post_m65_consumers/Scenes/skate/park.judas'],cwd=R)
  (p/'Scenes/skate/park.judas').write_bytes(original)
 env=dict(os.environ,SDL_VIDEODRIVER=os.getenv('SDL_VIDEODRIVER','offscreen'),SDL_AUDIODRIVER='dummy',JUDAS_ENGINE_ROOT=str(R),JUDAS_PROFILE='1',JUDAS_PROFILE_OUTPUT=str(out/'profile.json'),XDG_DATA_HOME=str(R/'.cache/post-m65-repair-saves'/name))
 if mode=='harness':
  commands='FRAMES 240\nLOG_EVERY 10\n'
  for frame,action,scene in [(5,'skate','skate/park'),(60,'rooftop','rooftop/rooftops'),(100,'void','void/system'),(150,'menu','launcher'),(170,'skate','skate/park'),(200,'rooftop','rooftop/rooftops'),(220,'menu','launcher')]:
   commands+=f'ACTION collection_{action} 1 {frame} {frame+1}\nWAIT_SERVICES scene 10000 {frame+1}\nEXPECT_SCENE Scenes/{scene}.judas {frame+2}\n'
  commands+='WAIT_SERVICES stream 15000 8\n'
  for frame,action in [(50,'skate'),(90,'rooftop'),(130,'void')]:commands+=f'ACTION {action}_pause 1 {frame} {frame+1}\nEXPECT_PAUSED 1 {frame+2}\nSCREENSHOT {frame+2} {out}/{action}-paused.png\n'
  commands+='EXPECT_PAUSED 0 63\nEXPECT_PAUSED 0 103\nEXPECT_PAUSED 0 153\n'
  (out/'commands.txt').write_text(commands);env['JUDAS_TEST_SCRIPT']=str(out/'commands.txt');path=p/'post_m65_consumers.judasproj'
 elif mode.startswith('resident-'):
  scene=p/'Scenes/skate/park.judas'
  if mode=='resident-four':f.position(scene,10,(36.6,-18.43,-250))
  f.props(scene,10,0,{'startSpeed':0,'delay':1})
  f.probe(p,scene,"""import {scenes,ui,console} from 'judas';
export default class {constructor(){this.n=0;}start(){this.tokens=NREGIONS.map(name=>scenes.requestRegion(name));}presentationUpdate(){scenes.removeInterest('skater');}uiUpdate(){++this.n;if(this.n%60===0)console.log('RESIDENT '+JSON.stringify(scenes.regions.map(r=>({id:r.id,state:r.state,pins:r.pins}))));if(this.n===360)ui.quit();}}
""".replace("NREGIONS",json.dumps(["street-"+str(i) for i in range(4 if mode=="resident-four" else 1)])));path=scene
 elif mode=='save':
  scene=p/'Scenes/skate/park.judas';f.props(scene,40,0,{'saveAt':2,'loadAt':4});commands='FRAMES 480\nLOG_EVERY 60\nWAIT_SERVICES stream 15000 10\nWAIT_SERVICES save 15000 190\nWAIT_SERVICES save 15000 400\n';(out/'commands.txt').write_text(commands);env['JUDAS_TEST_SCRIPT']=str(out/'commands.txt');path=scene
 else:raise ValueError(mode)
 result=subprocess.run([str(binary),str(path)],cwd='/tmp',env=env,capture_output=True,text=True,timeout=300)
 (out/'run.log').write_text(result.stdout+result.stderr);(out/'result.json').write_text(json.dumps({'exit':result.returncode,'mode':mode,'runtime':str(binary)},indent=2)+'\n');print(name,result.returncode,flush=True);assert result.returncode==0
