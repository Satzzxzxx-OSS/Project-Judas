#!/usr/bin/env python3
"""Current consumer integration assets through the shared named authoring path."""
import m65_create_demo as m
from pathlib import Path
import json,subprocess
m.asset('prefabs/rider.judasprefab',kind='prefab')
m.asset('worlds/integration.judasworld','JudasWorld 1\nbudget 2 8 2 8388608 8388608 8388608 2097152\nregion "annex" "Scenes/annex.judas" 30 0 0 0 0 0 1 10 10 10 1 "snapshot" 262144 ""\n','world')
m.asset('scripts/reference.js',"""export const properties={target:{type:'entity',default:null}};
export default class {constructor({properties}){this.target=properties.target;this.state={seen:false};}fixedUpdate(){this.state.seen=this.target?.valid===true;}}
""")
m.asset('scripts/visitor.js',"""import {input,world,ui} from 'judas';
import {add,mul,rotate,axis,qm,tangent,norm} from './math.js';
export default class {constructor({entity}){this.entity=entity;this.state={yaw:0,pitch:0,steps:0,inputSeen:false};this.jump=false;}
 start(){input.pointerCapture=true;}restore(){this.start();}
 update(){if(ui.get('integration')?.modal)return;this.state.yaw-=input.axis('look_x')*.0022;this.state.pitch=Math.max(-1.35,Math.min(1.35,this.state.pitch-input.axis('look_y')*.0022));this.jump ||= input.pressed('jump');}
 fixedUpdate(){const c=this.entity.character,up=c.up,q=axis(up,this.state.yaw),f=rotate(q,{x:0,y:0,z:-1}),r=rotate(q,{x:1,y:0,z:0});const x=input.axis('move_x'),y=input.axis('move_y');
  let v=add(mul(r,x*5),mul(f,y*5));const old=c.velocity,vertical=add(old,mul(up,-(old.x*up.x+old.y*up.y+old.z*up.z)));if(!c.supported)v=add(v,add(old,mul(vertical,-1)));else v=add(v,c.supportVelocity);
  if(this.jump&&c.supported)v=add(v,mul(up,5));this.jump=false;c.velocity=v;this.state.steps++;this.state.inputSeen ||= !!(x||y);
 }
 presentationUpdate(){const t=this.entity.presentedTransform,up=this.entity.character.up,q=qm(axis(up,this.state.yaw),axis({x:1,y:0,z:0},this.state.pitch));world.setView({position:add(t.position,mul(up,.65)),rotation:q},65);}
 destroy(){input.pointerCapture=false;}}
""")
m.asset('scripts/lab_sensor.js',"""import {link} from './lab.js';
export default class {onTriggerEnter(e){if(e.other?.id==='10')link.lab?.checkpoint('ENTER');}onTriggerStay(e){if(e.other?.id==='10')link.lab.state.sensorStay++;}onTriggerExit(e){if(e.other?.id==='10')link.lab?.checkpoint('EXIT');}}
""")
mat=m.asset('physical/grippy.judasphysmat',kind='physicalMaterial');prefab=m.asset('prefabs/rider.judasprefab',kind='prefab')
m.asset('scripts/lab.js',"""import {world,input,ui,physics,scenes,saves,profiler} from 'judas';
import {add,mul,rotate,axis,length,sub} from './math.js';
export const link={lab:null};
export default class {constructor(){this.state={phase:0,sensorEnter:0,sensorStay:0,sensorExit:0,ikError:0,material:false};this.pending=[];this.token=0;this.request=0;this.message='Original IK rider + five articulated rigs. Rooftop game copied with permission.';}
 start(){link.lab=this;this.hud=ui.get('integration');ui.debugOverlayVisible=false;if(this.hud){this.hud.modal=false;this.hud.get('menu').visible=false;}}
 restore(){this.start();}
 checkpoint(type){if(type==='ENTER')this.state.sensorEnter++;else this.state.sensorExit++;this.message='Sensor checkpoint '+type+' (normal M42 callback)';}
 update(){if(this.hud?.modal)return;for(const a of ['wake','material','connect','reanchor','spawn','ragdoll','region','adopt'])if(input.pressed(a))this.pending.push(a);if(input.pressed('parkour'))scenes.load('Scenes/rooftops.judas');if(input.pressed('lab'))scenes.load('Scenes/integration.judas');if(input.pressed('restart'))scenes.reload();}
 fixedUpdate(dt){this.state.phase+=dt;const phase=this.state.phase,r=world.entity('101'),board=world.entity('100');
  // Board motion is project content. Feet consume resolved joint state, never fade estimates.
  const rotation=axis({x:0,y:0,z:1},Math.sin(phase)*.22);board.transform={rotation};
  if(r.animation?.info.ready&&!r.ragdoll?.active){const targets=[['Left',-.38],['Right',.38]].map(([side,x])=>[side,add(board.transform.position,rotate(rotation,{x,y:.1,z:0}))]);for(const [side,target] of targets)r.animation.limb(side,{target,pole:{x:5,y:1,z:3}});this.state.ikError=0;for(const [side,target] of targets){const foot=r.animation.jointTransform(side+'Foot','world');if(foot)this.state.ikError=Math.max(this.state.ikError,length(sub(foot.position,target)));}}
  const platform=world.entity('21');platform.transform={position:{x:-2+Math.sin(phase*.5)*2,y:1.5,z:-6},rotation:axis({x:0,y:1,z:0},Math.sin(phase*.4)*.25)};
  for(const a of this.pending){
   if(a==='wake'){for(const e of world.queryTags()){if(e.ragdoll?.active){const b=e.ragdoll.body('Root');b?.applyImpulse({x:0,y:6,z:3});}}this.message='Normal impulses wake connected articulations';}
   if(a==='spawn'){world.spawnPrefab(PREFAB,{position:{x:0,y:3,z:-12}});}
   if(a==='ragdoll'){if(r.ragdoll?.active)r.ragdoll.leave(.4);else r.ragdoll?.enter();}
   if(a==='material'){const b=world.entity('400');this.state.material=!this.state.material;b.setPhysicalMaterial(this.state.material?MATERIAL:null,{friction:this.state.material?.9:.05,restitution:.05});b.applyImpulse({x:4,y:0,z:0});this.message='Physical material '+JSON.stringify(b.physicalMaterial);}
   if(a==='connect'){const owner=world.entity('401'),j=physics.joint(owner);if(j)j.destroy();else physics.createJoint(owner,{type:'ball',bodyA:world.entity('400'),anchorA:{x:0,y:0,z:0},anchorB:{x:0,y:0,z:0}});this.message='Runtime attachment toggled';}
   if(a==='reanchor'){physics.joint(world.entity('401'))?.configure({anchorB:{x:0,y:1,z:0}});}
   if(a==='region'){if(this.token){scenes.releaseRegion(this.token);this.token=0;}else this.token=scenes.requestRegion('annex');}
   if(a==='adopt'){const e=scenes.resolveRegionEntity('annex',1);if(e)this.message=String(scenes.adopt(e,'root'));}
  }this.pending=[];const g=physics.gravity(world.entity('400').transform.position);this.state.gravity=length(g);profiler.counter('M65 IK end error metres',this.state.ikError);
 }
 uiUpdate(){if(!this.hud)return;if(input.pressed('pause')){this.hud.modal=!this.hud.modal;this.hud.get('menu').visible=this.hud.modal;input.pointerCapture=!this.hud.modal;}for(const [action,operation] of [['save_game','save'],['load_game','load']])if(input.pressed(action))this.request=saves[operation]('m65-current');if(this.request){const s=saves.status(this.request);if(s){this.message=s.state+' '+s.error;if(['completed','failed','cancelled'].includes(s.state))this.request=0;}}
  let asleep=0,total=0;for(const e of world.queryTags())if(e.collider){total++;asleep+=e.sleeping?1:0;}
  this.hud.get('status').text=`IK error ${this.state.ikError.toFixed(4)} m | sleeping ${asleep}/${total} | checkpoint ${this.state.sensorEnter}/${this.state.sensorStay}/${this.state.sensorExit}`;this.hud.get('message').text=this.message;
 }
 onUI(e){if(e.document!=='integration')return;if(e.type==='back'||(e.type==='click'&&e.element==='resume')){this.hud.modal=false;this.hud.get('menu').visible=false;input.pointerCapture=true;}if(e.type==='click'&&e.element==='rooftops')scenes.load('Scenes/rooftops.judas');if(e.type==='click'&&e.element==='restart')scenes.reload();if(e.type==='click'&&e.element==='quit')ui.quit();}}
""".replace('PREFAB',json.dumps(prefab)).replace('MATERIAL',json.dumps(mat)))
font=next((m.A/'fonts').glob('*.ttf'));fontid=json.loads('{}') if False else __import__('re').search(r'id "(.*?)"',Path(str(font)+'.judasmeta').read_text())[1]
elements=[{'id':'canvas','kind':'canvas','relativeSize':[1,1]},
 {'id':'title','parent':'canvas','kind':'text','offset':[25,12],'size':[900,40],'font':fontid,'text':'M65 — Developer Integration','fontSize':28},
 {'id':'controls','parent':'canvas','kind':'text','offset':[25,56],'size':[1210,60],'font':fontid,'fontSize':18,'wrap':True,'text':'WASD / mouse / Space | F1 lab / F2 Rooftop Run | G wake / P spawn | J connect / K re-anchor / M material | T region / Y adopt | F6 save / F7 load | Esc pause'},
 {'id':'status','parent':'canvas','kind':'text','offset':[25,120],'size':[1190,30],'font':fontid,'fontSize':18},
 {'id':'message','parent':'canvas','kind':'text','offset':[25,156],'size':[1190,50],'font':fontid,'fontSize':18,'wrap':True},
 {'id':'menu','parent':'canvas','kind':'panel','anchorMin':[.5,.5],'anchorMax':[.5,.5],'offset':[-250,-180],'size':[500,360],'background':[.04,.08,.13,.97],'flow':'vertical','padding':[30,25,30,25],'visible':False},
 {'id':'paused','parent':'menu','kind':'text','size':[440,50],'font':fontid,'text':'Paused — fixed simulation is stopped','fontSize':22}]
for id,text in [('resume','Resume'),('rooftops','Open Rooftop Run'),('restart','Reload lab'),('quit','Quit')]:elements.append({'id':id,'parent':'menu','kind':'button','size':[440,46],'font':fontid,'text':text,'background':[.17,.3,.42,1]})
source=m.P/'tools/integration_ui.json';source.parent.mkdir(exist_ok=True);source.write_text(json.dumps({'kind':'ui','elements':elements},indent=2)+'\n');m.asset('ui/integration.judasui',kind='ui');subprocess.run([m.ROOT/'build/judas_scene_author','--structured',source,m.A/'ui/integration.judasui'],check=True)
subprocess.run([m.ROOT/'build/judas_developer_author',m.P/'m65_integration.judasproj'],check=True)
