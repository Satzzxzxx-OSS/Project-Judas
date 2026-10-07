// Courier ship: a dynamic rigid body flown with script forces.
// Rotation: mouse/stick/roll set a target angular velocity (arcade handling).
// Translation: thrusters + optional flight assist (cancels drift and gravity).
import {input,world,physics,console} from 'judas';
import {SYSTEM} from './system_data.js';
import {G,V,intent,gravityAt,nearestPlanet} from './shared.js';
import {add,sub,mul,dot,cross,length,norm,tangent,clamp,damp,lerp,qm,axis,rotate,slerp} from './math.js';

const T={
 forward:30, reverse:16, strafe:18, vertical:20, boostMul:1.9,
 maxSpeed:75, boostSpeed:120, assistDamp:1.6,
 pitchRate:1.9, yawRate:1.6, rollRate:2.4, mouseSens:0.0028, angularResponse:9,
 fireInterval:0.11, boltSpeed:240, boltDamage:1,
 landProbe:1.6, landSpeed:2.2,
};

export default class Ship {
 constructor({entity}){
  this.entity=entity;this.state={hull:100,assist:true,cockpit:false};
  this.mouse={x:0,y:0};this.cooldown=0;this.side=1;this.landed=true;this.thrust=0;
  this.cam={rot:null,shake:0,fov:70};this.engineVol=-1;
  G.ship=this;
 }
 start(){this.exhaust=SYSTEM.ids.exhaust.map(id=>world.entity(id));}

 get piloted(){return G.mode==='ship';}
 damage(n,src='bolt'){
  if(G.game?.over)return;
  if(!this.piloted&&src==='bolt'){G.game?.sfx('hit',0.2);return;} // parked: docking shield holds
  if(G.game?.props.logEvery)console.log(`DMG ${n} ${src} hull=${Math.round(this.state.hull)}`);
  this.state.hull=Math.max(0,this.state.hull-n);this.sinceHit=0;this.cam.shake=Math.min(1,this.cam.shake+n*0.04);
  G.game?.sfx('hit',0.6);
  if(this.state.hull<=0)G.game?.shipDestroyed();
 }
 reset(position,rotation){
  this.entity.transform={position,rotation};this.entity.velocity={x:0,y:0,z:0};this.entity.angularVelocity={x:0,y:0,z:0};
  this.state.hull=100;this.landed=true;this.cam.rot=null;
 }

 update(dt){
  if(!this.piloted||G.game?.paused)return;
  this.mouse.x+=input.axis('look_x');this.mouse.y+=input.axis('look_y');
  this.stick={x:input.axis('look_stick_x'),y:input.axis('look_stick_y')};
 }

 fixedUpdate(dt){
  const e=this.entity,t=e.transform,q=t.rotation,pos=t.position;
  const fwd=rotate(q,{x:0,y:0,z:-1}),right=rotate(q,{x:1,y:0,z:0}),up=rotate(q,{x:0,y:1,z:0});
  const v=e.velocity,w=e.angularVelocity,mass=e.mass,g=gravityAt(pos),gmag=length(g),gUp=gmag>0?mul(g,-1/gmag):null;
  this.cooldown=Math.max(0,this.cooldown-dt);
  // Hull nanites: repair after 4 s without taking damage.
  this.sinceHit=(this.sinceHit||0)+dt;if(this.sinceHit>4&&this.state.hull<100)this.state.hull=Math.min(100,this.state.hull+6*dt);
  // Landed: something solid just below (relative to gravity) and nearly stopped.
  let ground=null;
  if(gUp){ground=physics.raycast(pos,mul(gUp,-1),T.landProbe+1,{ignored:[e]});}
  this.landed=!!ground&&ground.distance<T.landProbe+0.3&&length(v)<T.landSpeed;
  const I=this.piloted?intent():null;
  if(I&&I.assist){this.state.assist=!this.state.assist;G.game?.banner(this.state.assist?'FLIGHT ASSIST ON':'FLIGHT ASSIST OFF - Newtonian drift',1.4);}
  if(I&&I.camera)this.state.cockpit=!this.state.cockpit;

  // ---- rotation -------------------------------------------------------
  let rate={x:0,y:0,z:0};
  if(I){
   const mx=this.mouse.x,my=this.mouse.y;this.mouse.x=this.mouse.y=0;
   const st=this.stick||{x:0,y:0};
   const pitch=clamp(-my*T.mouseSens/dt,-T.pitchRate,T.pitchRate)-st.y*T.pitchRate+I.pitchRate;
   const yaw=clamp(-mx*T.mouseSens/dt,-T.yawRate,T.yawRate)-st.x*T.yawRate+I.yawRate;
   rate={x:pitch,y:yaw,z:-I.roll*T.rollRate};
   // Near the ground with no pitch/roll input: level out against gravity for easy landings.
   const near=nearestPlanet(pos);
   if(gUp&&near.alt<30&&Math.abs(I.roll)<0.1&&Math.abs(pitch)<0.05){
    const fix=cross(up,gUp);const k=near.alt<12?2.5:1.2;
    rate=add(rate,{x:dot(fix,right)*k,y:0,z:dot(fix,fwd)*-k});
   }
  }
  const target=add(add(mul(right,rate.x),mul(up,rate.y)),mul(fwd,-rate.z));
  if(I||!this.landed)e.angularVelocity=add(w,mul(sub(target,w),damp(I?T.angularResponse:2,dt)));

  // ---- translation ----------------------------------------------------
  let a={x:0,y:0,z:0};this.thrust=0;
  if(I){
   const boost=I.boost&&I.my>0;
   const local={x:I.mx*T.strafe,y:((I.ascend?1:0)-(I.descend?1:0))*T.vertical,z:-(I.my>0?I.my*T.forward*(boost?T.boostMul:1):I.my*T.reverse)};
   a=add(add(mul(right,local.x),mul(up,local.y)),mul(fwd,-local.z));
   this.thrust=clamp(length(local)/T.forward,0,2);
   if(this.state.assist){
    // Damp velocity along any ship axis that has no thrust input, and hover against gravity.
    const vl={x:dot(v,right),y:dot(v,up),z:dot(v,fwd)};
    if(Math.abs(I.mx)<0.05)a=sub(a,mul(right,vl.x*T.assistDamp));
    if(!I.ascend&&!I.descend)a=sub(a,mul(up,vl.y*T.assistDamp));
    if(Math.abs(I.my)<0.05)a=sub(a,mul(fwd,vl.z*T.assistDamp*0.6));
    if(gmag>0&&!(this.landed&&!I.ascend))a=sub(a,g);
   }
   const cap=boost?T.boostSpeed:T.maxSpeed,sp=length(v);
   if(sp>cap)a=sub(a,mul(v,(sp-cap)/sp*3));
   if(I.fire&&this.cooldown<=0)this.fire(pos,q,v,fwd);
  }else if(!this.landed&&gmag===0){a=mul(v,-0.6);} // parked in space: hold station
  // Soft system boundary (everything beyond ~500 m is outside the view range).
  const fromBelt=sub(pos,V(SYSTEM.belt)),bd=length(fromBelt);
  if(bd>360)a=sub(a,mul(norm(fromBelt),(bd-360)*0.8));
  if(length(a)>0)e.applyForce(mul(a,mass));
  this.debugAlt=nearestPlanet(pos);
 }

 fire(pos,q,v,fwd){
  this.cooldown=T.fireInterval;this.side=-this.side;
  const muzzle=add(pos,rotate(q,{x:3.6*this.side,y:0,z:-2.4}));
  // Converge both cannons on whatever is under the reticle (or 180 m out).
  const probe=physics.raycast(pos,fwd,400,{ignored:[this.entity]});
  const range=probe?Math.max(15,probe.distance):180;
  const aim=add(pos,mul(fwd,range)),dir=norm(sub(aim,muzzle));
  G.bolts?.fire('player',muzzle,add(v,mul(dir,T.boltSpeed)),this.entity,T.boltDamage);
  G.game?.sfx('shot',0.35);
 }

 onCollisionEnter(ev){
  const impact=length(ev.relativeVelocity);
  if(impact>9)this.damage(Math.round((impact-9)*2.5),'collision '+(ev.other?.id)+' v='+impact.toFixed(1));
 }

 /** Presentation: engine audio/exhaust + camera pose for game.js. */
 presentationUpdate(dt){
  const vol=this.piloted?clamp(0.12+this.thrust*0.35,0,0.8):0;
  if(Math.abs(vol-this.engineVol)>0.02){this.engineVol=vol;this.entity.setAudio({volume:vol,pitch:clamp(0.8+this.thrust*0.4,0.8,1.6)});}
  const rateFx=this.piloted?Math.round(10+this.thrust*120):0;
  if(rateFx!==this.lastFx){this.lastFx=rateFx;for(const x of this.exhaust||[])if(x?.valid)x.setParticles({rate:rateFx});}
 }
 cameraPose(dt){
  const t=this.entity.presentedTransform,q=t.rotation,c=this.cam;
  dt=Math.min(dt,0.1);
  c.rot=c.rot?slerp(c.rot,q,damp(this.state.cockpit?40:7,dt)):q;
  c.shake=Math.max(0,c.shake-dt*1.5);
  const jitter=c.shake>0?{x:(Math.random()-0.5)*c.shake*0.5,y:(Math.random()-0.5)*c.shake*0.5,z:0}:{x:0,y:0,z:0};
  const speed=length(this.entity.velocity);
  c.fov=lerp(c.fov,clamp(68+speed*0.15,68,86),damp(3,dt));
  if(this.state.cockpit)return {position:add(t.position,rotate(q,add({x:0,y:1.05,z:-1.3},jitter))),rotation:q,fov:c.fov+4};
  const rot=qm(c.rot,axis({x:1,y:0,z:0},-0.07));
  return {position:add(t.position,rotate(c.rot,add({x:0,y:3.2,z:13},jitter))),rotation:rot,fov:c.fov};
 }
}
