// Content policy only. Physical failure never calls release or decrements health.
import {world,input,ui,scenes,saves,physics} from 'judas';
import {mul,add} from './math.js';
export const properties={radial:{type:'boolean',default:false},game:{type:'boolean',default:false},shotImpulse:{type:'number',default:100}};
export default class {
 constructor({properties}){this.props=properties;this.state={load:0,frameLoad:false,spawned:[],region:false,slot:'fracture-current',shots:0};this.pending=[];this.request=0;this.message='Aim at a plate and click. Brown faces are newly exposed material.';this.region=0;}
 start(){this.hud=ui.get('lab');if(this.hud){this.hud.get('pause').visible=false;this.hud.modal=false;}}
 restore(){this.start();if(this.state.region)this.region=scenes.requestRegion('annex');}
 update(){if(this.hud?.modal)return;for(const action of ['load','air','release','spawn','cape','region','push','adopt'])if(input.pressed(action))this.pending.push(action);
  if(input.pressed('flat'))scenes.load('Scenes/flat.judas');if(input.pressed('radial'))scenes.load('Scenes/radial.judas');if(input.pressed('game'))scenes.load('Scenes/game.judas');if(input.pressed('restart'))scenes.reload();}
 owners(){const ids=[...(this.props.game?['100']:['100','200','300','450','501']),...this.state.spawned];if(this.state.region){const e=scenes.resolveRegionEntity('annex',1);if(e)ids.push(e.id);}return ids.map(id=>world.entity(id)).filter(e=>e?.valid&&e.fracture);}
 fixedUpdate(){for(const action of this.pending){
   if(action==='load')this.state.load=(this.state.load+1)%3;
   if(action==='air')this.state.frameLoad=!this.state.frameLoad;
   if(action==='release')world.entity('300')?.deformable?.release('right');
   if(action==='spawn'&&this.state.spawned.length<6){const r=world.viewRay;if(r){const e=world.spawnPrefab('63636363636363636363636363630007',{position:add(r.origin,mul(r.direction,4)),rotation:{w:1,x:0,y:0,z:0},scale:{x:1,y:1,z:1}});this.state.spawned.push(e.id);}}
   if(action==='region'){if(this.region){scenes.releaseRegion(this.region);this.region=0;this.state.region=false;}else{this.region=scenes.requestRegion('annex');this.state.region=true;}}
   if(action==='adopt'){const e=scenes.resolveRegionEntity('annex',1);if(e){scenes.adopt(e,'root');this.state.spawned.push(e.id);this.message='Family adopted to root; release annex to unload it without resurrecting material.';}}
   if(action==='push'||action==='cape'){const r=world.viewRay;if(!r)continue;let nearest=null,owner=null;for(const e of this.owners()){const h=e.deformable.raycast(r.origin,r.direction,50);if(h&&(!nearest||h.distance<nearest.distance)){nearest=h;owner=e;}}
    const body=physics.raycast(r.origin,r.direction,50,{ignored:[world.entity('10')]});
    if(action==='cape'){if(owner&&nearest){const state=owner.fracture.state;const part=state.rigid?state.parts.find(p=>p.entity?.id===body?.entity?.id):null;if(part)owner.fracture.remove(part.index,state.revision);this.message='Explicit project cleanup removes a real physical piece.';}continue;}
    // Exactly one hit-point impulse; no damage counter or second equivalent force.
    if(nearest&&(!body||nearest.distance<=body.distance+.03)){owner.deformable.impulseAt(nearest.location,mul(r.direction,this.props.shotImpulse));this.message='One physical hit-point impulse applied.';}
    else if(body?.entity?.hasTag('pickup')){body.entity.applyImpulseAtPoint(mul(r.direction,this.props.shotImpulse),body.point);this.message='Ordinary rigid prop hit.';}
    else this.message=body?'Static environment hit; no impulse applied.':'No material hit.';
    this.state.shots++;
   }
  }this.pending=[];
  if(this.state.load&&!this.props.game)world.entity('200')?.deformable?.force({x:0,y:0,z:this.state.load===1?5:120},'right');
  if(this.state.frameLoad&&!this.props.game)world.entity('300')?.deformable?.force({x:0,y:-1200,z:0},'right');
 }
 uiUpdate(){if(!this.hud)return;if(input.pressed('pause')){this.hud.modal=!this.hud.modal;this.hud.get('pause').visible=this.hud.modal;input.pointerCapture=!this.hud.modal;}
  if(input.pressed('save_game'))this.command('save');if(input.pressed('load_game'))this.command('load');if(this.request){const s=saves.status(this.request);if(s){this.message=s.state+(s.error?' — '+s.error:'');if(['completed','failed','cancelled'].includes(s.state))this.request=0;}}
  let parts=0,broken=0,maximum=0,errors=[];for(const e of this.owners()){const f=e.fracture.state;parts+=f.parts.filter(p=>!p.removed).length;broken+=f.interfaces.filter(b=>b.broken).length;for(const b of f.interfaces)maximum=Math.max(maximum,b.tension,b.shear);if(f.error||e.deformable.state.error)errors.push(f.error||e.deformable.state.error);}
  this.hud.get('title').text=this.props.game?'FRACTURE RANGE / clear the barrier for the navigator':'FRACTURE LAB / '+(this.props.radial?'RADIAL':'UNIFORM')+' GRAVITY';
  this.hud.get('score').text=`${parts} live material parts | ${broken} failed interfaces | ${this.state.shots} physical shots`;
  this.hud.get('progress').text=`Beam load ${['off','yield (5 N)','fracture (120 N)'][this.state.load]} | frame load ${this.state.frameLoad?'1200 N':'off'} | peak traction ${maximum.toFixed(1)} Pa | ${scenes.regions.map(r=>r.id+': '+r.state+(r.pins.length?' (pinned live family)':'')).join(' / ')}`;
  this.hud.get('message').text=errors[0]||this.message;
 }
 command(op){if(this.request)return;try{this.request=op==='save'?saves.save(this.state.slot,{name:'Fracture lab state'}):op==='load'?saves.load(this.state.slot):saves.delete(this.state.slot);}catch(e){this.message=String(e);}}
 onUI(e){if(e.type==='back'||e.type==='click'&&e.element==='resume'){this.hud.modal=false;this.hud.get('pause').visible=false;input.pointerCapture=true;}if(e.type==='click'&&e.element==='restart')scenes.reload();if(e.type==='click'&&e.element==='quit')ui.quit();if(e.type==='click'&&['save','load','delete'].includes(e.element))this.command(e.element);}
 destroy(){if(this.region)scenes.releaseRegion(this.region);}
}
