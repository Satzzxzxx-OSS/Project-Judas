#!/usr/bin/env python3
"""Focused current-game scenarios, using physical M35 input and normal Application.

Temporary copies contain only documented spawn/property/probe changes. Final
consumer content stays untouched. Keep every run in its own evidence directory.
"""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1];P=ROOT/'projects/post_m65_consumers';E=ROOT/'docs/evidence/post_m65_consumers'
def block(scene,id):return re.search(r'\nobject '+str(id)+r' "[^\n]+\n.*?\nend\n',scene,re.S)
def props(path,id,index,values):
    s=path.read_text();m=block(s,id);assert m,id
    text=m[0];line=re.search(r'  script\.'+str(index)+r'\.properties (.*)',text)
    data=json.loads(json.loads(line[1]));data.update(values)
    text=text[:line.start()]+f'  script.{index}.properties '+json.dumps(json.dumps(data))+text[line.end():]
    path.write_text(s[:m.start()]+text+s[m.end():])
def position(path,id,p):
    s=path.read_text();m=block(s,id);assert m
    text=re.sub(r'  position [^\n]+','  position '+' '.join(map(str,p)),m[0],count=1);path.write_text(s[:m.start()]+text+s[m.end():])
def probe(p,scene,source):
    path=p/'Assets/collection/review_probe.js';path.write_text(source)
    Path(str(path)+'.judasmeta').write_text('JudasAssetMeta 1\nid "f1246803e77aee1136349c664b593651"\ntype script\nsource ""\n')
    s=scene.read_text();m=block(s,1000000);text=m[0].replace('  scripts 1','  scripts 2').replace('\nend\n','\n  script.1.id 2\n  script.1.asset "f1246803e77aee1136349c664b593651"\n  script.1.enabled true\n  script.1.properties "{}"\nend\n');scene.write_text(s[:m.start()]+text+s[m.end():])

SKATE_PROBE="""import {world,physics,console,time} from 'judas';
import {game} from '../skate/scripts/game.js';
const dist=(a,b)=>Math.hypot(a.x-b.x,a.y-b.y,a.z-b.z);
export default class {
 constructor(){this.n=0;this.state={};}
 uiUpdate(){++this.n;const s=game.skater,r=world.entity('12'),a=r?.animation;
  if(!s||!a?.info.ready)return;
  const left=a.jointTransform('mixamorig:LeftToeBase','model'),right=a.jointTransform('mixamorig:RightToeBase','model');
  if(left&&right){this.state.poseLift=s.view.lift;this.state.measuredLift=Math.max(0,Math.min(left.position.y,right.position.y)-.096);this.state.clip=a.info.clip;}
  if(this.n===180){const e=world.entity('1000001');e.setSocket(r,'mixamorig:LeftHand',{position:{x:.03,y:0,z:0}});this.socket=e;
   const ankle=a.jointTransform('mixamorig:LeftFoot','world'),hip=a.jointTransform('mixamorig:LeftUpLeg','world');
   this.target={x:ankle.position.x,y:ankle.position.y+.1,z:ankle.position.z};
   this.state.ikConfigured=a.limb('review-foot',{root:'mixamorig:LeftUpLeg',middle:'mixamorig:LeftLeg',end:'mixamorig:LeftFoot',target:this.target,pole:{x:hip.position.x,y:hip.position.y-.3,z:hip.position.z-.7},weight:1});
  }
  if(this.n===240){const ankle=a.jointTransform('mixamorig:LeftFoot','world');this.state.ikError=dist(ankle.position,this.target);
   const h=a.jointTransform('mixamorig:LeftHand','world',true);this.state.socketError=Math.abs(dist(this.socket.presentedTransform.position,h.position)-.03);
   a.removeLimb('review-foot');this.socket.clearSocket();
   const body=s.e,t=body.transform;this.joint=physics.createJoint(body,{type:'slider',bodyA:body,anchorB:{x:0,y:0,z:0},lower:-5,upper:5,limits:true});this.state.jointCreated=this.joint.valid;
  }
  if(this.n===260){this.joint.configure({anchorB:{x:0,y:0,z:0}});this.state.jointReanchored=this.joint.valid;this.joint.destroy();this.state.jointRetired=!this.joint.valid;
   this.material=s.e.physicalMaterial;s.e.setPhysicalMaterial('dad488c962a44aaba2adfba152ac50c2',{friction:.7,restitution:.05});this.state.materialRead=s.e.physicalMaterial;
   const root=s.e.transform.position;const hit=physics.raycast({x:root.x,y:root.y+1,z:root.z},{x:0,y:-1,z:0},2,{excludeLayers:['Ragdoll','Pickup']});this.state.hitMaterial=hit?.physicalMaterial??null;this.state.hitBody=hit?.entity?.id??null;
  }
  if(this.n===290)s.e.setPhysicalMaterial(null,{friction:this.material.friction,restitution:this.material.restitution});
  if(this.n%60===0)console.log('REVIEW '+JSON.stringify(this.state));
 }
}
"""
VOID_PROBE="""import {physics,world,console} from 'judas';
import {G,gravityAt} from '../void/scripts/shared.js';
import {SYSTEM} from '../void/scripts/system_data.js';
export default class {constructor(){this.n=0;this.state={};}uiUpdate(){++this.n;if(this.n===180){
 const e=G.foot.entity,p=e.transform.position,up=e.character.state.up,origin={x:p.x+up.x*1.2,y:p.y+up.y*1.2,z:p.z+up.z*1.2};
 const hit=physics.raycast(origin,{x:-up.x,y:-up.y,z:-up.z},6,{ignored:[world.entity(SYSTEM.ids.ship)]});
 this.state.motorHit=hit?.entity?.id===e.id;this.state.shape=hit?.shape;this.state.hit=hit?.entity?.id??null;this.state.distance=hit?.distance??null;this.state.gravity=gravityAt(p);this.state.motorGravity=e.character.state.gravity;
 const suit=G.foot.state.suit;G.bolts.fire('enemy',origin,{x:-up.x*80,y:-up.y*80,z:-up.z*80},world.entity(SYSTEM.ids.ship),7);this.state.suitBefore=suit;
 }if(this.n===190)this.state.suitAfter=G.foot.state.suit;if(this.n%60===0)console.log('REVIEW '+JSON.stringify(this.state));}}
"""
def run(name,p,scene,events,frames,env,extra=None):
    out=E/name;out.mkdir(parents=True,exist_ok=False);(out/'events.txt').write_text(events)
    command=[str(ROOT/'build/judas_consumer_review'),str(scene),str(out/'events.txt'),str(out),str(frames)]
    r=subprocess.run(command,cwd='/tmp',env=dict(env,**(extra or {})),capture_output=True,text=True,timeout=240)
    (out/'run.log').write_text(r.stdout+r.stderr)
    rows=[json.loads(x) for x in (out/'observations.jsonl').read_text().splitlines()] if (out/'observations.jsonl').exists() else []
    (out/'run.json').write_text(json.dumps({'exit_code':r.returncode,'frames':frames,'runtime_sha256':hashlib.sha256((ROOT/'build/judas').read_bytes()).hexdigest(),'observer_sha256':hashlib.sha256((ROOT/'build/judas_consumer_review').read_bytes()).hexdigest(),'temporary_scene_sha256':hashlib.sha256(scene.read_bytes()).hexdigest(),'last_observation':rows[-1] if rows else None},indent=2)+'\n')
    print(name,'exit',r.returncode,flush=True);return r,rows
def main():
    import sys
    mode=sys.argv[1];env={k:v for k,v in os.environ.items() if not k.startswith(('JUDAS_','FTFT_'))};env.update(SDL_VIDEODRIVER=os.getenv('SDL_VIDEODRIVER','offscreen'),SDL_AUDIODRIVER='dummy')
    with tempfile.TemporaryDirectory(prefix='judas-post-m65-') as tmp:
        p=Path(tmp)/'project';shutil.copytree(P,p)
        if mode.startswith('rooftop-'):
            scene=p/'Scenes/rooftop/rooftops.judas';props(scene,10,0,{'logEvery':1})
            if mode=='rooftop-events':
                position(scene,10,(5.35,20.2,-42));probe(p,scene,"""import {console} from 'judas';
export default class {constructor(){this.state={enter:0,stay:0,exit:0,triggers:0,valid:false};}
onCollisionEnter(e){++this.state.enter;this.state.valid=e.other?.valid===true;this.state.other=e.other?.id??null;}
onCollisionStay(e){++this.state.stay;}onCollisionExit(e){++this.state.exit;}onTriggerEnter(e){++this.state.triggers;}uiUpdate(){console.log('REVIEW '+JSON.stringify(this.state));}}
""");q=scene.read_text();m=block(q,1000000);review=re.search(r'  script.1.id 2\n  script.1.asset [^\n]+\n  script.1.enabled true\n  script.1.properties [^\n]+',m[0])[0].replace('script.1','script.2').replace('script.2.id 2','script.2.id 3');t=m[0].replace('scripts 2','scripts 1');t=re.sub(r'  script.1.id 2\n  script.1.asset [^\n]+\n  script.1.enabled true\n  script.1.properties [^\n]+\n','',t);q=q[:m.start()]+t+q[m.end():];m=block(q,10);t=m[0].replace('scripts 2','scripts 3').replace('\nend\n','\n'+review+'\nend\n');scene.write_text(q[:m.start()]+t+q[m.end():]);events='1 key:W 1\n60 key:Space 1\n68 key:Space 0\n';frames=180
            elif mode=='rooftop-wall':position(scene,10,(5.35,20.2,-42));events='1 key:W 1\n60 key:Space 1\n68 key:Space 0\n';frames=180
            elif mode=='rooftop-ramp':position(scene,10,(0,19.32,-134));events='60 key:W 1\n160 key:W 0\n';frames=180
            elif mode=='rooftop-checkpoint':position(scene,10,(0,20.2,-25.5));events='60 key:W 1\n120 key:W 0\n';frames=140
            else:props(scene,10,0,{'autopilot':'course'});events='';frames=2200
        elif mode=='void-query':
            scene=p/'Scenes/void/system.judas';probe(p,scene,VOID_PROBE);events='';frames=300
        elif mode=='void-input':
            scene=p/'Scenes/void/system.judas';props(scene,30,0,{'logEvery':60});events='1 key:W 1\n15 key:W 0\n60 key:F 1\n61 key:F 0\n180 key:Space 1\n260 key:Space 0\n300 mouse:Left 1\n430 mouse:Left 0\n450 key:V 1\n451 key:V 0\n';frames=600
        elif mode=='void-ruby':
            scene=p/'Scenes/void/system.judas';props(scene,30,0,{'autopilot':'ruby_foot','logEvery':60});events='';frames=1800
        elif mode=='void-mission':
            scene=p/'Scenes/void/system.judas';props(scene,30,0,{'autopilot':'mission','logEvery':60});events='';frames=6600
        elif mode=='skate-apis':
            scene=p/'Scenes/skate/park.judas';probe(p,scene,SKATE_PROBE)
            scene.write_text(scene.read_text()+'\nobject 1000001 "Review socket marker"\n  position 0 0 0\n  rotation 1 0 0 0\n  scale 1 1 1\nend\n')
            events='';frames=360
        elif mode=='skate-save':
            scene=p/'Scenes/skate/park.judas';props(scene,40,0,{'saveAt':2,'loadAt':4,'locale':'es'});events='90 key:W 1\n160 key:W 0\n';frames=480
        elif mode=='skate-stream-revisit':
            scene=p/'Scenes/skate/park.judas';position(scene,10,(36.6,-18.43,-250));props(scene,10,0,{'startSpeed':0,'delay':1,'telemetry':True})
            probe(p,scene,"""import {world,console} from 'judas';
import {game} from '../skate/scripts/game.js';
export default class {constructor(){this.n=0;this.state={};}fixedUpdate(){++this.n;
if(this.n===240||this.n===480){const s=game.skater, p=this.n===240?{x:0,y:3.28,z:23}:{x:36.6,y:-18.43,z:-250};s.e.transform={position:p};s.e.velocity={x:0,y:0,z:0};this.state.returning=this.n===480;console.log('REVIEW backtrack '+JSON.stringify(p));}}}
""");events='';frames=840
        elif mode=='skate-street':
            scene=p/'Scenes/skate/park.judas';position(scene,10,(36.6,-18.43,-250));props(scene,10,0,{'startSpeed':15,'delay':1,'telemetry':True});events='300 key:T 1\n301 key:T 0\n500 key:T 1\n501 key:T 0\n';frames=720
        elif mode.startswith('skate-ragdolls'):
            scene=p/'Scenes/skate/park.judas';s=scene.read_text();rig=block(s,12)[0];crowd=p/'Assets/skate/scripts/crowd.js';asset=re.search(r'id "([0-9a-f]{32})"',Path(str(crowd)+'.judasmeta').read_text())[1]
            # Same consumer's real 13-body imported humanoid. Add only a flat
            # benchmark floor outside its play obstacles to isolate settled cost.
            floor=block(s,2)[0].replace('object 2 ', 'object 1000100 ').replace('  position 0 -0.5 -12','  position 90 -0.5 0').replace('60 0.5 60','30 .5 30')
            s+=floor
            for i in range(20):
                r=rig.replace('object 12 ',f'object {1000200+i} ',1);r=re.sub(r'  position [^\n]+',f'  position {78+(i%5)*5} 2 {-8+(i//5)*5}',r,count=1)
                r=r.replace('\nend\n',f'\n  scripts 1\n  script.0.id 1\n  script.0.asset "{asset}"\n  script.0.enabled true\n  script.0.properties '+json.dumps(json.dumps({'clip':'Ride','ragdollAt':1}))+'\nend\n');s+=r
            scene.write_text(s);events='';frames=1800
            # A genuine impulse after settling, through the game's existing body handles.
            cs=crowd.read_text().replace('this.started = false;', 'this.state = {}; this.started = false;')
            cs=cs.replace('  update() {',"  update() {\n    if(this.e.id==='1000200'&&this.dropped){const b=this.e.ragdoll.body('mixamorig:Hips');if(time.elapsed>27.9&&!this.kicked){this.state.beforeSleep=b.sleeping;this.kicked=true;b.applyImpulse({x:20,y:3,z:0});this.state.afterSleep=b.sleeping;}if(this.kicked)this.state.speed=Math.hypot(b.velocity.x,b.velocity.y,b.velocity.z);}")
            crowd.write_text(cs)
        else:raise ValueError(mode)
        extra={'JUDAS_REVIEW_NO_SLEEP':'1'} if mode.endswith('reference') else {}
        if frames>1200:extra['JUDAS_REVIEW_CAPTURE_EVERY']='1200'
        if mode=='skate-apis':
            material=p/'Assets/collection/grippy.judasphysmat';material.write_text('JudasPhysicalMaterial 1\nfriction .7\nrestitution .05\n');Path(str(material)+'.judasmeta').write_text('JudasAssetMeta 1\nid \"dad488c962a44aaba2adfba152ac50c2\"\ntype physicalMaterial\nsource \"\"\n')
        r,rows=run(os.getenv('JUDAS_REVIEW_RUN_NAME',mode),p,scene,events,frames,env,extra)
        assert r.returncode==0,mode
        # Keep source for review/reproduction rather than manufacturing success.
        shutil.copyfile(scene,E/os.getenv('JUDAS_REVIEW_RUN_NAME',mode)/'fixture.judas')
        q=p/'Assets/collection/review_probe.js'
        if q.exists():shutil.copyfile(q,E/os.getenv('JUDAS_REVIEW_RUN_NAME',mode)/'review_probe.js')
if __name__=='__main__':main()
