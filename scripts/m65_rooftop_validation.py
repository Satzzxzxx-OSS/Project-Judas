#!/usr/bin/env python3
"""Rooftop motor-query regression through normal logical input and public casts.

Only temporary project copies are instrumented. The game and engine are exercised
by the ordinary application loop; no private controller intent is assigned.
"""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'docs/evidence/m65/rooftop-wall-followup/verified'
PROJECT=ROOT/'projects/m65_integration'
CASES={
 'wallrun':((5.35,20.2,-42),180,'ACTION jump 1 60 68\n'),
 'walljump':((5.35,20.2,-42),130,'ACTION jump 1 60 68\nACTION jump 1 95 99\n'),
 'empty-side':((0,20.2,-30),100,'ACTION jump 1 60 68\n'),
 'climb':((0,20.2,-77),140,'ACTION jump 1 42 95\n'),
 'vault':((0,21,0),130,'ACTION jump 1 45 55\n'),
 'slide-stand':((0,24,-84),160,'ACTION slide 1 60 80\n'),
}
PROBE="""
  if(this.steps===62){
   const summarize=h=>h?{id:h.entity?.id,distance:h.distance,normal:h.normal,overlap:h.initialOverlap}:null;
   const pos=this.entity.transform.position,dir={x:1,y:0,z:0};
   console.log('PROBE '+JSON.stringify({raw:summarize(physics.raycast(pos,dir,1.4)),filtered:summarize(physics.raycast(pos,dir,1.4,this.queryFilter)),stand:this.canStand()}));
  }
"""
def main():
 OUT.mkdir(parents=True,exist_ok=False)
 assert not (OUT/'results.json').exists(),'Keep prior runs; choose a fresh evidence path.'
 env={k:v for k,v in os.environ.items() if not k.startswith(('JUDAS_','FTFT_'))}
 env.update(SDL_VIDEODRIVER='offscreen',SDL_AUDIODRIVER='dummy',LIBGL_ALWAYS_SOFTWARE='1')
 results={};checks={}
 with tempfile.TemporaryDirectory(prefix='judas-m65-rooftop-') as temp:
  scratch=Path(temp)
  for name,(pos,frames,actions) in CASES.items():
   p=scratch/name;shutil.copytree(PROJECT,p)
   project=p/'m65_integration.judasproj';project.write_text(project.read_text().replace('startup-scene "Scenes/integration.judas"','startup-scene "Scenes/rooftops.judas"'))
   scene=p/'Scenes/rooftops.judas';s=scene.read_text()
   s=s.replace('object 10 "Runner"\n  position 0 21.0 8','object 10 "Runner"\n  position '+' '.join(map(str,pos)))
   s=s.replace('script.0.properties "{}"','script.0.properties "{\\"logEvery\\":1}"',1);scene.write_text(s)
   runner=p/'Assets/scripts/runner.js';s=runner.read_text()
   if name=='wallrun':s=s.replace('this.time+=dt;this.steps++;','this.time+=dt;this.steps++;'+PROBE)
   # Observation only: expose current wall normal in the temporary log.
   s=s.replace('mode:this.mode,p:', 'mode:this.mode,wall:this.wr?.normal,p:');runner.write_text(s)
   harness=OUT/(name+'.txt');harness.write_text(f'FRAMES {frames}\nENTITY 10\nLOG_EVERY 1\nAXIS move_y 1 1 {frames}\n'+actions)
   r=subprocess.run([str(ROOT/'build/judas'),str(project)],cwd=ROOT,env=dict(env,JUDAS_TEST_SCRIPT=str(harness),XDG_DATA_HOME=str(scratch/'data')),capture_output=True,text=True,timeout=60)
   (OUT/(name+'.log')).write_text(r.stdout+r.stderr)
   states=[json.loads(line[4:]) for line in r.stdout.splitlines() if line.startswith('JS: {"t":')]
   probes=[json.loads(line[10:]) for line in r.stdout.splitlines() if line.startswith('JS: PROBE ')]
   results[name]={'exit_code':r.returncode,'modes':sorted({s['mode'] for s in states}),'events':[e for s in states for e in s['ev'].split(',') if e],'states':states,'probes':probes}
   checks[name+'_executes']=r.returncode==0 and len(states)>frames-3 and not any(x in r.stdout+r.stderr for x in ['script callback failed','TypeError:','ReferenceError:'])
 wall=[s for s in results['wallrun']['states'] if s['mode']=='wallrun']
 checks['real_wall_run']=len(wall)>30 and all(s['wall']['x']<-.99 for s in wall)
 checks['moves_toward_real_wall']=bool(wall) and wall[-1]['p'][0]>wall[0]['p'][0] and max(s['p'][0] for s in wall)<5.75
 checks['advances_along_wall']=bool(wall) and wall[0]['p'][2]-wall[-1]['p'][2]>5
 probe=results['wallrun']['probes'][0]
 checks['unfiltered_detects_query_capsule']=probe['raw']['id']=='10' and probe['raw']['overlap'] and probe['raw']['distance']==0
 checks['filtered_detects_actual_tower']=probe['filtered']['id']=='111' and not probe['filtered']['overlap'] and probe['filtered']['normal']['x']<-.99
 checks['stand_clearance_excludes_self']=probe['stand']
 jump=[s for s in results['walljump']['states'] if 'walljump' in s['ev']]
 checks['wall_jump_away']=len(jump)==1 and jump[0]['v'][0]<-4 and jump[0]['v'][1]>5
 checks['no_phantom_wall_in_open_space']='wallrun' not in results['empty-side']['modes'] and 'climb' not in results['empty-side']['modes']
 checks['climb_works']='climb' in results['climb']['events']
 checks['vault_works']='vault' in results['vault']['events']
 slide=results['slide-stand']['states']
 checks['slide_and_stand']='slide' in results['slide-stand']['events'] and any(s['cr'] for s in slide) and any(not s['cr'] for s in slide if s['t']>2)
 result={'checks':checks,'all_pass':all(checks.values()),'runs':results,'runtime_sha256':hashlib.sha256((ROOT/'build/judas').read_bytes()).hexdigest(),'runner_sha256':hashlib.sha256((PROJECT/'Assets/scripts/runner.js').read_bytes()).hexdigest(),'test_policy':'Normal M35 logical input. Spawn placement and telemetry only in isolated temporary copies; original consumer and current scene unchanged.'}
 (OUT/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(checks,indent=2));assert result['all_pass']
if __name__=='__main__':main()
