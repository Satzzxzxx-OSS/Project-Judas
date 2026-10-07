// Game controller: mission flow, boarding/exiting, camera ownership, HUD,
// world-space target markers, explosions, deaths/respawns and pause menu.
import {world,physics,ui,scenes,session,time,console} from 'judas';
import {input} from './collection_input.js';
import {SYSTEM} from './system_data.js';
import {G,V,PLANETS,intent,gravityAt,nearestPlanet} from './shared.js';
import {Bolts} from './bolts.js';
import {makeAutopilot} from './autopilot.js';
import {add,sub,mul,dot,cross,length,norm,tangent,clamp,qm,axis,rotate,lookRotation} from './math.js';

export const properties={autopilot:{type:'string',default:''},logEvery:{type:'number',default:0}};

const STAGES=[
 'Walk to your courier ship and board it  [F]',
 'Raiders in the asteroid belt! Destroy them',
 'Land on Ruby (the red planet) and recover the data core on foot',
 'AMBUSH! Get back to your ship and destroy the raiders',
 'Return to Terra and land on the home pad',
 'MISSION COMPLETE - free flight: raiders keep coming',
];
const HIDDEN={x:0,y:-5000,z:0};
const SFX={shot:SYSTEM.ids.shot,boom:SYSTEM.ids.boom,hit:SYSTEM.ids.hit,pickup:SYSTEM.ids.pickup};

export default class Game {
 constructor({entity,properties}){
  this.entity=entity;this.props=properties;
  this.state={stage:0,kills:0,deaths:0,elapsed:0,waveTotal:0,core:false};
  this.bannerText='';this.bannerT=0;this.respawnT=0;this.frozen=null;this.paused=false;this.steps=0;this.lastHud='';
  G.game=this;G.mode='foot';G.enemies.clear();G.pendingDebris.clear();
 }
 get over(){return this.respawnT>0;}
 get raidersHuntPilot(){ // only while the pilot is on foot down on Ruby
  if(this.state.stage!==3||this.protectT>0||!G.foot)return false;
  return length(sub(G.foot.entity.transform.position,PLANETS[1].c))<PLANETS[1].region;}

 start(){
  G.bolts=new Bolts();
  if(this.props.autopilot){G.auto=makeAutopilot(this.props.autopilot);if(!G.auto)console.log('unknown autopilot '+this.props.autopilot);}
  else G.auto=null;
  this.doc=ui.get('hud');ui.debugOverlayVisible=false;
  if(this.doc){this.doc.modal=false;this.doc.get('menu').visible=false;}
  input.pointerCapture=!G.auto;
  this.explosion=world.entity(SYSTEM.ids.explosion);this.core=world.entity(SYSTEM.ids.core);
  this.markers=SYSTEM.ids.markers.map(id=>world.entity(id));this.objMarker=world.entity(SYSTEM.ids.objective);
  this.banner('VOID COURIER',3);
 }

 // ------------------------------------------------------------ helpers
 sfx(name,volume){const e=world.entity(SFX[name]);if(!e?.valid)return;try{e.setAudio({volume});e.playAudioOneShot();}catch(err){/* loading or one-shot capacity */}}
 banner(text,seconds=2){this.bannerText=text;this.bannerT=seconds;}
 explode(pos,big=true){if(this.explosion?.valid){this.explosion.transform={position:pos};this.explosion.burst(big?160:60);}this.sfx('boom',big?0.9:0.5);}
 spawnRaiders(count,center,spread){
  for(let i=0;i<count;i++){
   const p=add(center,{x:(Math.random()-0.5)*spread,y:(Math.random()-0.5)*spread*0.5,z:(Math.random()-0.5)*spread});
   world.spawnPrefab(SYSTEM.prefabs.raider,{position:p,rotation:{w:1,x:0,y:0,z:0}});
  }
  this.state.waveTotal=count;this.waveKills=0;
 }
 setStage(n){this.state.stage=n;this.banner(n===5?'MISSION COMPLETE':STAGES[n].split(' - ')[0].split('!')[0],2.5);console.log(`stage ${n} t=${this.state.elapsed.toFixed(1)}`);
  if(n===1)this.spawnRaiders(3,V(SYSTEM.belt),40);
  if(n===3){const up=V(SYSTEM.rubyPad.up);this.spawnRaiders(3,add(V(SYSTEM.rubyPad.pos),mul(up,70)),50);}
  if(n===5)this.openMenu('MISSION COMPLETE',`Time ${this.clock(this.state.elapsed)}   Raiders destroyed ${this.state.kills}   Deaths ${this.state.deaths}\nResume for free flight.`);
 }
 raiderDestroyed(r){
  this.state.kills++;this.waveKills=(this.waveKills||0)+1;this.explode(r.entity.transform.position);
  this.banner('RAIDER DESTROYED',1.2);console.log(`kill ${this.state.kills}`);
 }
 shipDestroyed(){
  if(this.respawnT>0)return;
  this.explode(G.ship.entity.transform.position);this.frozen=this.lastPose;this.respawnT=3;this.banner('SHIP DESTROYED',3);console.log('ship destroyed');
 }
 pilotKilled(){if(this.respawnT>0)return;this.frozen=this.lastPose;this.respawnT=2.5;this.banner('PILOT DOWN',2.5);this.sfx('boom',0.5);console.log('pilot killed');}
 respawn(){
  this.state.deaths++;G.bolts.clear();
  G.ship.reset(V(SYSTEM.shipSpawn),{w:1,x:0,y:0,z:0});
  G.foot.state.suit=100;G.foot.placeAt(V(SYSTEM.pilotSpawn),{x:0,y:0,z:-1},{x:0,y:1,z:0});
  G.mode='foot';this.frozen=null;this.banner('Back at Terra base',2);this.protectT=5;
  // Reset the current wave so raiders don't camp the respawn point.
  const wave=this.state.stage===1?[V(SYSTEM.belt),40]:this.state.stage===3?[add(V(SYSTEM.rubyPad.pos),mul(V(SYSTEM.rubyPad.up),70)),50]:null;
  if(wave){const remaining=Math.max(1,G.enemies.size);for(const r of [...G.enemies.values()]){r.dead=true;r.entity.destroy();}G.enemies.clear();
   const kills=this.waveKills;this.spawnRaiders(remaining,wave[0],wave[1]);this.state.waveTotal=remaining+kills;this.waveKills=kills;}
 }
 clock(t){const m=Math.floor(t/60),s=t-m*60;return `${m}:${s<10?'0':''}${s.toFixed(1)}`;}

 // ------------------------------------------------------------ boarding
 tryInteract(){
  const ship=G.ship,foot=G.foot;
  if(G.mode==='foot'){
   const d=length(sub(foot.entity.transform.position,ship.entity.transform.position));
   if(d<8){foot.board();G.mode='ship';ship.cam.rot=null;this.sfx('pickup',0.4);this.banner('Systems online',1.2);
    if(this.state.stage===0)this.setStage(1);}
   return;
  }
  if(!ship.landed){this.banner('Land and slow down before exiting',1.5);return;}
  const t=ship.entity.transform,q=t.rotation,pos=t.position,g=gravityAt(pos),up=length(g)>0?norm(mul(g,-1)):rotate(q,{x:0,y:1,z:0});
  const right=rotate(q,{x:1,y:0,z:0}),fwd=rotate(q,{x:0,y:0,z:-1});
  for(const dir of [right,mul(right,-1),fwd,mul(fwd,-1)]){
   const side=norm(tangent(dir,up)),spot=add(add(pos,mul(side,SYSTEM.shipHalf[0]+1.4)),mul(up,0.4));
   const rot=lookRotation(tangent(fwd,up),up);
   if(physics.capsuleCast({position:spot,rotation:rot},0.32,0.62,up,0.05,{ignored:[ship.entity]}))continue;
   G.mode='foot';foot.placeAt(spot,fwd,up);this.sfx('pickup',0.3);return;
  }
  this.banner('No room to exit here',1.5);
 }

 // ------------------------------------------------------------ simulation
 fixedUpdate(dt){
  this.steps++;
  if(G.auto?.start==='ruby'&&!this.startApplied){this.startApplied=true;
   const up=V(SYSTEM.rubyPad.up),pad=V(SYSTEM.rubyPad.pos),f=norm(tangent({x:0,y:0,z:-1},up));
   G.ship.reset(add(pad,mul(up,SYSTEM.shipHalf[1]+0.1)),lookRotation(f,up));
   const toCore=norm(tangent(sub(V(SYSTEM.core),pad),up));
   G.foot.placeAt(add(add(pad,mul(up,1.0)),mul(toCore,7)),toCore,up);this.state.stage=2;}
  if(this.respawnT>0){this.respawnT-=dt;G.bolts.tick(dt);if(this.respawnT<=0)this.respawn();this.log();return;}
  const I=intent();
  this.state.elapsed+=dt;this.protectT=Math.max(0,(this.protectT||0)-dt);
  if(I.interact)this.tryInteract();
  if(input.pressed('restart'))scenes.reload();
  G.bolts.tick(dt);
  const st=this.state;
  if(st.stage===1&&G.enemies.size===0&&this.waveKills>=st.waveTotal)this.setStage(2);
  if(st.stage===2&&G.mode==='foot'&&this.core?.valid){
   const d=length(sub(G.foot.entity.transform.position,V(SYSTEM.core)));
   if(d<2.6){st.core=true;this.core.transform={position:HIDDEN};this.sfx('pickup',0.9);this.setStage(3);}
  }
  if(st.stage===3&&G.enemies.size===0&&this.waveKills>=st.waveTotal)this.setStage(4);
  if(st.stage===4&&G.mode==='ship'&&G.ship.landed&&length(sub(G.ship.entity.transform.position,V(SYSTEM.homePad.pos)))<14)this.setStage(5);
  if(st.stage===5&&G.enemies.size===0&&!this.menuOpen)this.spawnRaiders(3,V(SYSTEM.belt),60);
  this.log();
 }
 log(){
  const n=this.props.logEvery;if(!(n>0)||this.steps%n)return;
  const r=x=>Math.round(x*10)/10,P=p=>[r(p.x),r(p.y),r(p.z)];
  const s=G.ship.entity,f=G.foot.entity;
  console.log(JSON.stringify({t:r(this.state.elapsed),stage:this.state.stage,mode:G.mode,ship:P(s.transform.position),sv:r(length(s.velocity)),
   landed:G.ship.landed,hull:Math.round(G.ship.state.hull),pilot:G.mode==='foot'?P(f.transform.position):null,suit:G.foot.state.suit,
   raiders:G.enemies.size,kills:this.state.kills,auto:G.auto?.step,bolts:G.bolts.stats,aim:G.auto?.aim,ids:[...G.enemies.keys()].join(',')}));
 }

 // ------------------------------------------------------------ presentation
 presentationUpdate(dt){
  let pose=this.frozen||(G.mode==='ship'?G.ship.cameraPose(dt):G.foot.cameraPose(dt));
  this.lastPose=pose;
  world.setView({position:pose.position,rotation:pose.rotation},pose.fov||70);
  const cam=pose.position,camUp=rotate(pose.rotation,{x:0,y:1,z:0});
  this.stars=this.stars||world.entity(SYSTEM.ids.starfield);if(this.stars?.valid)this.stars.transform={position:cam};
  const place=(m,p,base,spin)=>{
   if(!m?.valid)return;if(!p){m.transform={position:HIDDEN};return;}
   let d=sub(p,cam),l=length(d);let at=p;if(l>430){at=add(cam,mul(d,430/l));l=430;}
   const s=clamp(l/45,0.5,9)*base;
   m.transform={position:at,rotation:qm(axis({x:0,y:1,z:0},spin),axis({x:1,y:0,z:0},0.785)),scale:{x:s,y:s,z:s}};
  };
  const spin=time.elapsed*2;
  const raiders=[...G.enemies.values()];
  this.markers.forEach((m,i)=>{const r=raiders[i];place(m,r&&r.entity.valid?add(r.entity.presentedTransform.position,mul(camUp,3.5)):null,0.8,spin);});
  let obj=null;const st=this.state.stage;
  if(st===0||(G.mode==='foot'&&st!==2))obj=add(G.ship.entity.presentedTransform.position,mul(camUp,4));
  if(st===2)obj=G.mode==='foot'?add(V(SYSTEM.core),mul(V(SYSTEM.rubyPad.up),2.5)):add(V(SYSTEM.rubyPad.pos),mul(V(SYSTEM.rubyPad.up),6));
  if(st===4&&G.mode==='ship')obj=add(V(SYSTEM.homePad.pos),{x:0,y:6,z:0});
  place(this.objMarker,obj,1,-spin);
 }

 // ------------------------------------------------------------ UI
 openMenu(title,body){if(!this.doc)return;this.menuOpen=true;this.doc.get('menu_title').text=title;this.doc.get('menu_body').text=body;
  this.doc.get('menu').visible=true;this.doc.modal=true;this.paused=true;input.pointerCapture=false;}
 closeMenu(){if(!this.doc)return;this.menuOpen=false;this.doc.get('menu').visible=false;this.doc.modal=false;this.paused=false;input.pointerCapture=!G.auto;}
 uiUpdate(dt){
  if(!this.doc)return;
  if(input.pressed('pause')){if(this.doc.modal)this.closeMenu();else this.openMenu('PAUSED',`${STAGES[this.state.stage]}\nTime ${this.clock(this.state.elapsed)}   Kills ${this.state.kills}   Deaths ${this.state.deaths}`);}
  if(!this.doc.modal)this.bannerT=Math.max(0,this.bannerT-dt);
  const ship=G.ship,foot=G.foot,st=this.state;
  const subject=G.mode==='ship'?ship.entity:foot.entity,pos=subject.transform.position;
  const near=nearestPlanet(pos),inG=length(gravityAt(pos))>0;
  const speed=G.mode==='ship'?length(ship.entity.velocity):length(foot.entity.character?.velocity||{x:0,y:0,z:0});
  let obj=STAGES[st.stage];if(st.stage===1||st.stage===3)obj+=`  (${this.waveKills||0}/${st.waveTotal})`;
  const status=G.mode==='ship'
   ?`SHIP  ${speed.toFixed(0)} m/s\n${near.planet.name} alt ${Math.max(0,near.alt).toFixed(0)} m${inG?'  (gravity)':'  (deep space)'}\nAssist ${ship.state.assist?'ON':'OFF'}  ${ship.landed?'LANDED':''}\nRaiders ${G.enemies.size}  Kills ${st.kills}`
   :`ON FOOT  ${near.planet.name}\nShip ${length(sub(pos,ship.entity.transform.position)).toFixed(0)} m away\nRaiders ${G.enemies.size}  Kills ${st.kills}`;
  const bar=n=>'\u2588'.repeat(Math.round(n/5)).padEnd(20,'\u2591');
  const hull=G.mode==='ship'?`HULL [${bar(ship.state.hull)}] ${Math.round(ship.state.hull)}%`:`SUIT [${bar(foot.state.suit)}] ${foot.state.suit}%`;
  let prompt='';
  if(this.respawnT<=0){
   if(G.mode==='foot'&&length(sub(pos,ship.entity.transform.position))<8)prompt='[F] Board ship';
   else if(G.mode==='ship'&&ship.landed)prompt='[F] Exit ship';
  }
  const controls=G.mode==='ship'
   ?'Mouse aim  W/S thrust  A/D strafe  SPACE/CTRL up/down  Q/E roll  SHIFT boost  LMB fire  X assist  V camera  F exit (landed)  ESC pause'
   :'WASD move  Mouse look  SPACE jump  SHIFT run  F board ship  ESC pause  BACKSPACE restart';
  const hud=[obj,status,hull,prompt,this.bannerT>0?this.bannerText:'',controls].join('\u0001');
  if(hud!==this.lastHud){this.lastHud=hud;const [a,b,c,d,e,f]=hud.split('\u0001');
   this.doc.get('objective').text=a;this.doc.get('status').text=b;this.doc.get('hull').text=c;this.doc.get('prompt').text=d;
   this.doc.get('banner').text=e;this.doc.get('controls').text=f;this.doc.get('reticle').visible=G.mode==='ship';}
 }
 onUI(e){
  if(e.document!=='hud')return;
  if(e.type==='back'||(e.type==='click'&&e.element==='resume'))this.closeMenu();
  if(e.type==='click'&&e.element==='restart')scenes.reload();
  if(e.type==='click'&&e.element==='quit')ui.quit();
 }
 destroy(){input.pointerCapture=false;world.clearView();G.game=null;}
}
