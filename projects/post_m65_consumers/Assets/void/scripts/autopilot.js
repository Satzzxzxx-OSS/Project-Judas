// Optional original scripted pilot retained for reproducible mission scenarios.
// M65 also supports real logical harness input; this attract path drives the
// same intent object as the keyboard without changing the game rules.
// Behaviours: walkTo, board/exit, climb, flyTo, fight, land.
import {console,physics} from 'judas';
import {SYSTEM} from './system_data.js';
const console_log=s=>console.log(s);
import {G,V,gravityAt,nearestPlanet} from './shared.js';
import {add,sub,mul,dot,cross,length,norm,tangent,clamp,rotate} from './math.js';

const shipFrame=()=>{const t=G.ship.entity.transform,q=t.rotation;
 return {pos:t.position,v:G.ship.entity.velocity,fwd:rotate(q,{x:0,y:0,z:-1}),right:rotate(q,{x:1,y:0,z:0}),up:rotate(q,{x:0,y:1,z:0})};};
/** Turn the nose toward a world direction. */
function steer(dir,f){const x=dot(dir,f.right),y=dot(dir,f.up),z=dot(dir,f.fwd);
 return {yawRate:clamp(-Math.atan2(x,z)*7,-1.6,1.6),pitchRate:clamp(Math.atan2(y,z)*7,-1.9,1.9),angle:Math.atan2(Math.hypot(x,y),z)};}
/** Translate toward a point using thrusters only (orientation independent). */
function translate(target,f,maxSpeed,gain=0.5){
 let want=mul(sub(target,f.pos),gain);if(length(want)>maxSpeed)want=mul(norm(want),maxSpeed);
 const err=sub(want,f.v),k=0.6;
 return {mx:clamp(dot(err,f.right)*k,-1,1),my:clamp(dot(err,f.fwd)*k,-1,1),ascend:dot(err,f.up)>0.6,descend:dot(err,f.up)<-0.6};
}
const nearestRaider=p=>{let best=null,bd=1e9;for(const r of G.enemies.values()){if(!r.entity.valid)continue;const d=length(sub(r.entity.transform.position,p));if(d<bd){bd=d;best=r;}}return best;};

const aimInfo={v:''};
const B={
 wait:()=>({}),
 walkTo:(target,radius=1.5)=>()=>{const f=G.foot;const t=f.entity.transform,up=f.entity.character?.up||{x:0,y:1,z:0};
  const d=tangent(sub(target(),t.position),up);if(length(d)<radius)return {done:true};
  // face the target: compute yaw error in the pilot's heading frame
  const heading=rotate(t.rotation,{x:0,y:0,z:-1}),fwd0=norm(tangent(heading,up));
  const yawNow=f.state.yaw,c=Math.cos(yawNow),s=Math.sin(yawNow);
  const right0=norm({x:fwd0.y*up.z-fwd0.z*up.y,y:fwd0.z*up.x-fwd0.x*up.z,z:fwd0.x*up.y-fwd0.y*up.x});
  const fwd={x:fwd0.x*c-right0.x*s,y:fwd0.y*c-right0.y*s,z:fwd0.z*c-right0.z*s},right={x:right0.x*c+fwd0.x*s,y:right0.y*c+fwd0.y*s,z:right0.z*c+fwd0.z*s};
  const nd=norm(d);return {my:clamp(dot(nd,fwd),-1,1),mx:clamp(dot(nd,right),-1,1),boost:length(d)>6};},
 press:(key)=>{let done=false;return ()=>{if(done)return {done:true};done=true;return {[key]:true};};},
 climb:(alt)=>()=>{const f=shipFrame();if(nearestPlanet(f.pos).alt>alt)return {done:true};return {ascend:true};},
 flyTo:(target,radius=25,speed=70)=>()=>{const f=shipFrame(),to=sub(target(),f.pos),d=length(to);
  if(d<radius)return {done:true};let dir=norm(to);
  // Simple avoidance: if something blocks the path ahead, bend the course around it.
  const block=physics.sphereCast(f.pos,5,dir,Math.min(d,120),{ignored:[G.ship.entity]});
  if(block)dir=norm(add(dir,mul(block.normal,1.6)));
  const s=steer(dir,f);
  const fast=s.angle<0.35;return {...s,my:fast?1:0,boost:fast&&d>150&&speed>75};},
 fight:()=>()=>{const f=shipFrame(),r=nearestRaider(f.pos);if(!r)return {done:true};
  const rp=r.entity.transform.position,rv=r.entity.velocity,d=length(sub(rp,f.pos));
  const lead=add(rp,mul(sub(rv,f.v),d/240)),s=steer(norm(sub(lead,f.pos)),f);
  // Feed-forward the line-of-sight rotation rate so the nose tracks a circling target.
  const rel=sub(rp,f.pos),wl=mul(cross(rel,sub(rv,f.v)),1/Math.max(1,dot(rel,rel)));
  s.pitchRate+=dot(wl,f.right);s.yawRate+=dot(wl,f.up);
  // keep clear of planets while dogfighting
  const np=nearestPlanet(f.pos);const climb=np.alt<25;
  aimInfo.v=Math.round(s.angle*100)/100+'@'+Math.round(d);
  return {...s,my:d>110?1:d<45?-0.6:0.2,fire:s.angle<Math.max(0.05,2.5/d)&&d<260,ascend:climb};},
 land:(pad,up)=>()=>{const f=shipFrame();if(G.ship.landed)return {done:true};
  const P=pad(),U=up(),above=add(P,mul(U,SYSTEM.shipHalf[1]+1.0));
  const horiz=length(tangent(sub(f.pos,P),U)),h=dot(sub(f.pos,P),U);
  // Line up over the pad first, then descend while still correcting sideways drift.
  const target=horiz>6?add(P,mul(U,Math.max(h,12))):above;
  const tr=translate(target,f,horiz>6?18:5,0.8);
  return horiz>6?tr:{...tr,ascend:false,descend:h>2.5};},
};

const SCEN={
 mission:()=>{
  const home=()=>V(SYSTEM.homePad.pos),hUp=()=>({x:0,y:1,z:0});
  const ruby=()=>V(SYSTEM.rubyPad.pos),rUp=()=>V(SYSTEM.rubyPad.up);
  const ship=()=>G.ship.entity.transform.position;
  return [
   ['wait',B.wait,1],
   ['walk to ship',B.walkTo(ship,6),20],
   ['board',B.press('interact'),1],
   ['climb',B.climb(25),8],
   ['fly to belt',B.flyTo(()=>V(SYSTEM.belt),90),25],
   ['fight wave 1',B.fight(),90],
   ['fly to ruby',B.flyTo(()=>add(ruby(),mul(rUp(),40)),30),40],
   ['land ruby',B.land(ruby,rUp),40],
   ['settle',B.wait,1],
   ['exit',B.press('interact'),1],
   ['walk to core',B.walkTo(()=>V(SYSTEM.core),2.0),40],
   ['wait core',B.wait,1],
   ['walk to ship',B.walkTo(ship,6),40],
   ['board again',B.press('interact'),1],
   ['climb',B.climb(20),6],
   ['fight wave 2',B.fight(),120],
   ['fly home',B.flyTo(()=>add(home(),{x:0,y:40,z:0}),30),40],
   ['land home',B.land(home,hUp),40],
   ['done',B.wait,5],
  ];
 },
 // Screenshot setups: start already landed on Ruby, on foot.
 ruby_foot:()=>({start:'ruby',steps:[['look',B.wait,7],['walk to core',B.walkTo(()=>V(SYSTEM.core),2.0),20]]}),
 hover:()=>[['walk to ship',B.walkTo(()=>G.ship.entity.transform.position,6),20],['board',B.press('interact'),1],['climb',B.climb(25),8],['hold',B.wait,6]],
 fly:()=>[['walk',B.walkTo(()=>G.ship.entity.transform.position,6),20],['board',B.press('interact'),1],['climb',B.climb(25),8],
  ['fly to belt',B.flyTo(()=>V(SYSTEM.belt),90),25],['fight',B.fight(),90],['hold',B.wait,3]],
};

export function makeAutopilot(name){
 const make=SCEN[name];if(!make)return null;
 const made=make(),steps=Array.isArray(made)?made:made.steps;let i=0,t=0,fn=null;
 const ap={step:'',start:Array.isArray(made)?null:made.start,intent(){
  while(i<steps.length){
   const [label,factory,limit]=steps[i];
   if(!fn){fn=factory===B.wait?B.wait:factory;t=0;ap.step=label;console_log(`AUTO ${label}`);}
   t+=1/60;const out=fn();
   if(out.done||t>limit){if(t>limit&&factory!==B.wait)console_log(`AUTO timeout ${label}`);i++;fn=null;continue;}
   return out;
  }
  ap.step='finished';return {};
 },get aim(){return aimInfo.v;
 }};
 return ap;
}
