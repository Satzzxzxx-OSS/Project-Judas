import {input,world,ui,scenes,saves} from 'judas';
import {add,mul,sub,length,bounded,axis} from './math.js';
export const properties={radial:{type:'boolean',default:false}};
const prefab='6262626262626262626262626262000b';
export default class {
 constructor({properties}){this.props=properties;this.state={air:false,cape:true,load:false,elasticMode:0,time:0,slot:'deformable-lab',spawned:[]};this.pending=[];this.request=0;this.message='Aim at fabric and hold mouse to push it.';this.region=null;}
 start(){this.hud=ui.get('lab');}
 restore(){this.start();this.pending=[];this.region=null;}
 update(){if(!this.hud||this.hud.modal)return;
  for(const action of ['air','cape','load','elastic_mode','release','spawn','region'])if(input.pressed(action))this.pending.push(action);
  if(input.pressed('flat'))scenes.load('Scenes/flat.judas');if(input.pressed('radial'))scenes.load('Scenes/radial.judas');if(input.pressed('restart'))scenes.reload();
 }
 fixedUpdate(dt){this.state.time+=dt;
  const patch=id=>{const e=world.entity(id);return e?.valid?e.deformable:null;};
  for(const action of this.pending){
   if(action==='air'){this.state.air=!this.state.air;patch('100')?.setMaterial({airDrag:this.state.air?.5:0,airVelocity:{x:1,y:0,z:3}});}
   if(action==='cape')this.state.cape=!this.state.cape;
   if(action==='load')this.state.load=!this.state.load;
   if(action==='elastic_mode')this.state.elasticMode=(this.state.elasticMode+1)%3;
   if(action==='release'){patch('100')?.release('top');this.message='Curtain top released physically. F9 reload restores authoring.';}
   if(action==='spawn'){const r=world.viewRay;if(r){const spawned=world.spawnPrefab(prefab,{position:add(r.origin,mul(r.direction,3))});this.state.spawned.push(spawned.id);}}
   if(action==='region'){if(this.region){scenes.releaseRegion(this.region);this.region=null;this.message='Annex released: state suspends once.';}else{this.region=scenes.requestRegion('annex');this.message='Annex requested at far end of deck.';}}
  }this.pending=[];
  const character=world.entity('200'),proxy=world.entity('202');if(this.state.cape&&character?.valid){const t=this.state.time;character.transform={position:{x:6+Math.sin(t*.5),y:2,z:0},rotation:axis({x:0,y:1,z:0},Math.sin(t*.3)*.7)};proxy.transform=character.transform;}
  if(this.state.load){const elastic=patch('300');if(this.state.elasticMode===0)elastic?.force({x:0,y:-100,z:35},'right');else if(this.state.elasticMode===1)elastic?.force({x:-180,y:0,z:0},'right');else {elastic?.force({x:0,y:0,z:80},'upperRight');elastic?.force({x:0,y:0,z:-80},'lowerRight');}patch('301')?.force({x:0,y:0,z:150},'right');}
  const press=world.entity('310');if(press?.valid){const target={x:18,y:this.state.load?2.3:4,z:0};press.applyForce(mul(bounded(add(mul(sub(target,press.transform.position),20),mul(press.velocity,-7)),30),press.mass));}
  if(input.held('push')){const r=world.viewRay;if(r){let nearest=null,owner=null;const ids=['100','201','300','301','400','401','501',...this.state.spawned];for(const id of ids){const d=patch(id),h=d?.raycast(r.origin,r.direction,15);if(h&&(!nearest||h.distance<nearest.distance)){nearest=h;owner=d;}}if(nearest)owner.impulseAt(nearest.location,mul(r.direction,dt*3));}}
 }
 uiUpdate(){if(!this.hud)return;if(input.pressed('pause')){this.hud.modal=!this.hud.modal;this.hud.get('pause').visible=this.hud.modal;input.pointerCapture=!this.hud.modal;}
  if(input.pressed('save_game'))this.command('save');if(input.pressed('load_game'))this.command('load');if(this.request){const s=saves.status(this.request);if(s){this.message=s.state+(s.error?' — '+s.error:'');if(['completed','failed','cancelled'].includes(s.state))this.request=0;}}
  let nodes=0,contacts=0,sleep=0,errors=[];for(const id of ['100','201','300','301','400','401','501',...this.state.spawned]){const e=world.entity(id),s=e?.valid?e.deformable?.state:null;if(s){nodes+=s.nodes;contacts+=s.contacts;sleep+=s.sleeping?1:0;if(s.error)errors.push(s.error);}}
  const regions=scenes.regions.map(r=>`${r.id}: ${r.state}${r.pins.length?' pinned':''}`).join(' | ');
  this.hud.get('title').text=`DEFORMABLE LAB / ${this.props.radial?'ACTUAL RADIAL':'UNIFORM'} GRAVITY`;
  this.hud.get('score').text=`A BLUE curtain | B RED cape | C GREEN elastic / ORANGE yielding | D PURPLE cushion | E oblique`;
  this.hud.get('progress').text=`${nodes} nodes / ${contacts} contacts / ${sleep} sleeping | air ${this.state.air} | load ${this.state.load} / ${['bend','compress','twist'][this.state.elasticMode]} | ${regions}`;
  this.hud.get('message').text=errors.length?errors[0]:this.message;
 }
 command(op){if(this.request)return;try{this.request=op==='save'?saves.save(this.state.slot,{name:'Moving fabric + plastic dent'}):op==='load'?saves.load(this.state.slot):saves.delete(this.state.slot);}catch(e){this.message=String(e);}}
 onUI(e){if(e.type==='back'||(e.type==='click'&&e.element==='resume')){this.hud.modal=false;this.hud.get('pause').visible=false;input.pointerCapture=true;}if(e.type==='click'&&e.element==='restart')scenes.reload();if(e.type==='click'&&e.element==='quit')ui.quit();if(e.type==='click'&&['save','load','delete'].includes(e.element))this.command(e.element);}
 destroy(){if(this.region)scenes.releaseRegion(this.region);}
}
