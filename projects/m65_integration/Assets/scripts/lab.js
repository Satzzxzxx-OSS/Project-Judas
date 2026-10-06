import {world,input,ui,physics,scenes,saves,profiler} from 'judas';
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
   if(a==='spawn'){world.spawnPrefab("edd557e48f09c753ee1c26c75004d4d8",{position:{x:0,y:3,z:-12}});}
   if(a==='ragdoll'){if(r.ragdoll?.active)r.ragdoll.leave(.4);else r.ragdoll?.enter();}
   if(a==='material'){const b=world.entity('400');this.state.material=!this.state.material;b.setPhysicalMaterial(this.state.material?"0e4c611bce197d5058885d79cdea1e60":null,{friction:this.state.material?.9:.05,restitution:.05});b.applyImpulse({x:4,y:0,z:0});this.message='Physical material '+JSON.stringify(b.physicalMaterial);}
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
