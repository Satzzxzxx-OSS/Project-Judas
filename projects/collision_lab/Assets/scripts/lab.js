import {world,input,ui,scenes,saves,physics} from 'judas';
import {mul,add} from './math.js';
export default class {
 constructor(){this.state={spawned:[],region:false,slot:'collision-current',shots:0};this.pending=[];this.request=0;this.message='Green: true mesh | red: separate legacy facets | doorway is physically open.';this.region=0;}
 start(){this.hud=ui.get('lab');if(this.hud){this.hud.get('pause').visible=false;this.hud.modal=false;this.hud.get('controls').text='WASD/mouse | Space | click | G/T | P | 1 lab / 2 mesh / 3 boxes | V assist / C break | Esc';}}
 restore(){this.start();if(this.state.region)this.region=scenes.requestRegion('annex');}
 update(){if(this.hud?.modal)return;for(const action of ['spawn','region','push','adopt','cape'])if(input.pressed(action))this.pending.push(action);if(input.pressed('restart'))scenes.reload();for(const [a,s] of [['flat','flat'],['radial','skate-mesh'],['game','skate-boxes']])if(input.pressed(a))scenes.load('Scenes/'+s+'.judas');}
 fixedUpdate(){const c=world.entity('200')?.collider;this.state.inspectedHull=c?.type==='hull'&&!!c.asset&&c.vertexCount===8;const q=physics.closestPoint({x:0,y:3,z:0},10,{ignored:[world.entity('10')]});this.state.closest=!!q&&q.distance>=0&&q.entity?.valid;for(const action of this.pending){
  if(action==='spawn'&&this.state.spawned.length<12){const r=world.viewRay;if(r){const e=world.spawnPrefab("8a57f50d3904b764ea526e57d25aa554",{position:add(r.origin,mul(r.direction,3))});this.state.spawned.push(e.id);}}
  if(action==='region'){if(this.region){scenes.releaseRegion(this.region);this.region=0;this.state.region=false;}else{this.region=scenes.requestRegion('annex');this.state.region=true;}}
  if(action==='adopt'){const e=scenes.resolveRegionEntity('annex',2);if(e)scenes.adopt(e,'root');}
  if(action==='cape'){const f=world.entity('402')?.fracture;if(f){const s=f.state;const i=s.interfaces.find(i=>!i.broken);if(i)this.state.fragmentReleased=f.release(i.key,s.revision);}}
  if(action==='push'){const r=world.viewRay;if(r){const h=physics.raycast(r.origin,r.direction,60,{ignored:[world.entity('10')]});if(h?.entity?.hasTag('pickup')){h.entity.applyImpulseAtPoint(mul(r.direction,18),h.point);this.state.shots++;}}}
 }this.pending=[];}
 uiUpdate(){if(!this.hud)return;if(input.pressed('pause')){this.hud.modal=!this.hud.modal;this.hud.get('pause').visible=this.hud.modal;input.pointerCapture=!this.hud.modal;}if(input.pressed('save_game'))this.command('save');if(input.pressed('load_game'))this.command('load');if(this.request){const s=saves.status(this.request);if(s){this.message=s.state+(s.error?' — '+s.error:'');if(['completed','failed','cancelled'].includes(s.state))this.request=0;}}
  const r=world.viewRay;let text='No collider hit';if(r){const h=physics.raycast(r.origin,r.direction,60,{ignored:[world.entity('10')]});if(h?.entity){const c=h.entity.collider;const p=physics.closestPoint(r.origin,60,{ignored:[world.entity('10')]});if(p)world.entity('220').transform={position:p.point};text=`${c.type} | ${c.triangleCount} triangles | child ${h.childKey} / feature ${h.feature} | ${c.children.map(c=>c.key+':'+c.type).join(', ')}`;}}
  this.hud.get('title').text='M64 / COLLISION GEOMETRY LAB';this.hud.get('score').text=text;this.hud.get('progress').text=this.regions()+' | F3 annex / H adopt | F5/F8 saves (root) | F9 reload';this.hud.get('message').text=this.message;
 }
 regions(){try{return scenes.regions.map(r=>r.id+': '+r.state).join(' / ');}catch{return 'Root world — saves enabled (no additive manifest)';}}
 command(op){if(!this.request)try{this.request=op==='save'?saves.save(this.state.slot):op==='load'?saves.load(this.state.slot):saves.delete(this.state.slot);}catch(e){this.message=String(e);}}
 onUI(e){if(e.type==='back'||e.type==='click'&&e.element==='resume'){this.hud.modal=false;this.hud.get('pause').visible=false;input.pointerCapture=true;}if(e.type==='click'&&e.element==='restart')scenes.reload();if(e.type==='click'&&e.element==='quit')ui.quit();if(e.type==='click'&&['save','load','delete'].includes(e.element))this.command(e.element);}
 destroy(){if(this.region)scenes.releaseRegion(this.region);}
}
