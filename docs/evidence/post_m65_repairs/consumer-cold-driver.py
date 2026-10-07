#!/usr/bin/env python3
"""Two actual runtime processes using the same copied Skate project and public save APIs."""
import importlib.util,json,os,shutil,subprocess,tempfile
from pathlib import Path
R=Path(__file__).resolve().parents[3];E=Path(__file__).resolve().parent/'consumer-cold-save';E.mkdir(exist_ok=False)
spec=importlib.util.spec_from_file_location('fixtures',R/'scripts/post_m65_review.py');f=importlib.util.module_from_spec(spec);spec.loader.exec_module(f)
with tempfile.TemporaryDirectory(prefix='judas-consumer-cold-') as t:
 p=Path(t)/'project';shutil.copytree(f.P,p);scene=p/'Scenes/skate/park.judas'
 source="""import {saves,console,ui} from 'judas';
import {game} from '../skate/scripts/game.js';
export default class {
 constructor(){this.state={};this.request=0;this.issued=false;}
 start(){this.request=saves.refresh();console.log('COLD refresh '+this.request);}
 restore(){this.restored=true;console.log('COLD restored score='+game.score);if(game.score!==314)throw Error('consumer score not restored');ui.quit();}
 uiUpdate(){if(this.restored||!this.request)return;const s=saves.status(this.request);if(!s)return;
 if(s.state==='failed'||s.state==='cancelled')throw Error('cold operation '+s.state+' '+s.error);
 if(s.state==='completed'){
  if(!this.issued){this.issued=true;const exists=saves.exists('consumer-cold');console.log('COLD exists='+exists);if(exists)this.request=saves.load('consumer-cold');else{game.score=314;this.request=saves.save('consumer-cold');}}
  else{console.log('COLD committed score='+game.score);ui.quit();}
 }}
}
"""
 f.probe(p,scene,source);(E/'probe.js').write_text(source)
 data=R/'.cache/post-m65-repair-consumer-cold';assert not data.exists()
 env=dict(os.environ,SDL_VIDEODRIVER='offscreen',SDL_AUDIODRIVER='dummy',LIBGL_ALWAYS_SOFTWARE='1',XDG_DATA_HOME=str(data),JUDAS_ENGINE_ROOT=str(R))
 for name in ['write','read']:
  commands=E/(name+'-commands.txt');commands.write_text('FRAMES 360\nLOG_EVERY 60\nWAIT_SERVICES stream 15000 10\nWAIT_SERVICES save 15000 20\n')
  run=subprocess.run([str(R/'build/judas'),str(scene)],cwd='/tmp',env=dict(env,JUDAS_TEST_SCRIPT=str(commands)),capture_output=True,text=True,timeout=90)
  log=run.stdout+run.stderr;(E/(name+'.log')).write_text(log);print(name,run.returncode,flush=True);assert run.returncode==0
  assert ('COLD exists=false' in log and 'COLD committed score=314' in log) if name=='write' else ('COLD exists=true' in log and 'COLD restored score=314' in log)
 (E/'result.json').write_text(json.dumps({'write_exit':0,'read_exit':0,'same_authored_project':True,'same_save_namespace':str(data),'restored_game_score':314},indent=2)+'\n')
