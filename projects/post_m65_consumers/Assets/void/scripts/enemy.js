// Raider: dynamic rigid body steered by forces. Circles its target, leads its
// shots, avoids planets/asteroids, and breaks into physical debris when destroyed.
import {world,physics} from 'judas';
import {SYSTEM} from './system_data.js';
import {G,V,PLANETS,gravityAt} from './shared.js';
import {add,sub,mul,dot,cross,length,norm,clamp} from './math.js';

const ASTEROIDS=SYSTEM.asteroids.map(a=>({c:V(a),r:a[3]}));
const T={hp:4,cruise:34,accel:26,orbit:48,fireRange:190,boltSpeed:130,damage:4,cooldown:[1.3,2.4]};

export default class Raider {
 constructor({entity}){
  this.entity=entity;this.state={hp:T.hp};this.phase=Math.random()*Math.PI*2;this.cool=1+Math.random()*1.5;
  this.home=null;this.flash=0;
 }
 start(){G.enemies.set(this.entity.id,this);this.home=this.entity.transform.position;}
 damage(n,point){
  this.state.hp-=n;this.flash=0.12;
  if(this.state.hp<=0&&!this.dead){this.dead=true;G.game?.raiderDestroyed(this);
   const p=this.entity.transform.position,v=this.entity.velocity;
   for(let i=0;i<5;i++){
    const off={x:(Math.random()-0.5)*3,y:(Math.random()-0.5)*2,z:(Math.random()-0.5)*3};
    const d=world.spawnPrefab(SYSTEM.prefabs.debris,{position:add(p,off),rotation:{w:1,x:0,y:0,z:0}});
    if(d?.valid)G.pendingDebris.set(d.id,{v:add(v,mul(norm(off),8+Math.random()*10)),w:{x:Math.random()*6-3,y:Math.random()*6-3,z:Math.random()*6-3}});
   }
   G.enemies.delete(this.entity.id);this.entity.destroy();}
 }
 target(){
  if(G.game?.over||G.game?.protectT>0)return null;
  if(G.mode==='ship'&&G.ship)return {p:G.ship.entity.transform.position,v:G.ship.entity.velocity,ship:true};
  if(G.foot&&G.game?.raidersHuntPilot)return {p:G.foot.entity.transform.position,v:G.foot.entity.character?.velocity||{x:0,y:0,z:0},ship:false};
  return null;
 }
 fixedUpdate(dt){
  if(this.dead)return;
  const e=this.entity,t=e.transform,pos=t.position,q=t.rotation,v=e.velocity,mass=e.mass;
  const fwd=norm(this.rotateZ(q));
  this.phase+=dt*0.35;this.cool-=dt;
  const tgt=this.target();
  let goal;
  if(tgt){
   // Orbit the target on a slowly turning ring, closing in to fire.
   const rel=sub(pos,tgt.p),dist=length(rel),radial=norm(rel);
   const side=norm(cross(radial,{x:Math.sin(this.phase),y:0.6,z:Math.cos(this.phase)}));
   goal=add(tgt.p,add(mul(radial,tgt.ship?T.orbit:T.orbit*1.4),mul(side,25)));
   // Lead the shot.
   // Bolts inherit this raider's velocity, so lead with the RELATIVE velocity.
   const lead=add(tgt.p,mul(sub(tgt.v,v),dist/T.boltSpeed)),aim=norm(sub(lead,pos));
   this.aim=aim;
   if(this.cool<=0&&dist<T.fireRange&&dot(fwd,aim)>0.97){
    const block=physics.raycast(pos,aim,dist-4,{ignored:[e]});
    if(!block||block.entity?.id===SYSTEM.ids.ship){
     // A little spread so raiders are dangerous but beatable.
     const j=0.035,shot=norm(add(aim,{x:(Math.random()-0.5)*j*2,y:(Math.random()-0.5)*j*2,z:(Math.random()-0.5)*j*2}));
     G.bolts?.fire('enemy',add(pos,mul(fwd,2.6)),add(v,mul(shot,T.boltSpeed)),e,T.damage);
     try{e.playAudioOneShot();}catch(err){/* loading/capacity */}
     this.cool=T.cooldown[0]+Math.random()*(T.cooldown[1]-T.cooldown[0]);
    }
   }
  }else{
   goal=add(this.home,{x:Math.cos(this.phase)*30,y:Math.sin(this.phase*0.7)*10,z:Math.sin(this.phase)*30});
   this.aim=norm(v.x||v.y||v.z?v:fwd);
  }
  let desired=mul(sub(goal,pos),0.9);if(length(desired)>T.cruise)desired=mul(norm(desired),T.cruise);
  let a=mul(sub(desired,v),1.6);
  // Avoid planets and asteroids.
  for(const pl of PLANETS){const d=sub(pos,pl.c),l=length(d);if(l<pl.r+18)a=add(a,mul(d,(pl.r+18-l)*3/l));}
  for(const as of ASTEROIDS){const d=sub(pos,as.c),l=length(d);if(l<as.r+7&&l>1e-3)a=add(a,mul(d,(as.r+7-l)*4/l));}
  if(length(a)>T.accel)a=mul(norm(a),T.accel);
  a=sub(a,gravityAt(pos)); // hover
  e.applyForce(mul(a,mass));
  // Turn the nose toward the aim direction.
  const axisV=cross(fwd,this.aim),s=length(axisV),c=dot(fwd,this.aim);
  const angle=Math.atan2(s,c);
  e.angularVelocity=s>1e-4?mul(axisV,Math.min(angle*4,3.2)/s):{x:0,y:0,z:0};
 }
 rotateZ(q){ // -Z of q
  return {x:-(2*(q.x*q.z+q.w*q.y)),y:-(2*(q.y*q.z-q.w*q.x)),z:-(1-2*(q.x*q.x+q.y*q.y))};
 }
 destroy(){G.enemies.delete(this.entity.id);}
}
