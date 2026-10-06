import pathlib,shutil,subprocess,os,json,time,hashlib
root=pathlib.Path.cwd();out=root/'docs/evidence/m65/public-proof-final';out.mkdir(exist_ok=True);scratch=pathlib.Path('/tmp/judas-m65-public-proof-project');shutil.copytree(root/'projects/m65_integration',scratch,dirs_exist_ok=True)
probe=r'''import {world,session,console} from 'judas';
import {add,rotate,axis,length,sub,norm} from './math.js';
function check(ok,name){if(!ok)throw new Error(name);console.log('PASS '+name);}
export default class {
 constructor(){this.n=0;this.targets=[];}
 start(){this.motor=world.entity('10');this.motor.transform={position:{x:-4,y:1,z:2}};}
 fixedUpdate(){this.n++;
  if(this.n===3){const a=world.entity('101');const q=axis(norm({x:1,y:2,z:3}),.65);a.transform={rotation:q};this.a=a;
   for(const [side,x] of [['Left',.1],['Right',-.1]]){const hip=a.animation.jointTransform(side+'Hip','world');const target=add(hip.position,rotate(q,{x,y:-1.5,z:.1}));a.animation.limb(side,{target,pole:add(hip.position,rotate(q,{x:0,y:0,z:2}))});this.targets.push([side,target]);}
  }
  if(this.n===4){for(const [side,target] of this.targets)check(length(sub(this.a.animation.jointTransform(side+'Foot','world').position,target))<.0001,'arbitrarily oriented '+side+' limb resolves world target');}
  if(this.n===10){check(session.get('m65.callback')==='destroy','normal motor/sensor callback ran');check(!this.motor.valid,'motor destroyed during callback retires safe handle');check(!world.entity('10').valid,'destroyed motor lookup does not alias another entity');check(session.get('m65.exit')===1,'destroying overlapped motor produces one exit');}
 }
}'''
sensor=r'''import {session} from 'judas';export default class {onTriggerEnter(e){if(e.other?.id==='10'){session.set('m65.callback','destroy');e.other.destroy();}}onTriggerExit(){session.set('m65.exit',(session.get('m65.exit')??0)+1);}}'''
(out/'probe.js').write_text(probe);(out/'sensor.js').write_text(sensor);(scratch/'Assets/scripts/lab.js').write_text(probe);(scratch/'Assets/scripts/lab_sensor.js').write_text(sensor);script=out/'harness.txt';script.write_text('FRAMES 25\nLOG_EVERY 0\n')
env=dict(os.environ,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',JUDAS_WORLD_STATE='none',JUDAS_TEST_SCRIPT=str(script));cmd=[str(root/'.cache/m65-before-clean-release/judas'),str(scratch/'m65_integration.judasproj')];start=time.monotonic()
with (out/'run.log').open('w') as f:p=subprocess.run(cmd,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=90)
(out/'results.json').write_text(json.dumps(dict(exit_code=p.returncode,seconds=time.monotonic()-start,command=cmd,script_sha256=hashlib.sha256(probe.encode()).hexdigest()),indent=2)+'\n');print(p.returncode);print((out/'run.log').read_text()[-2500:]);raise SystemExit(p.returncode)
