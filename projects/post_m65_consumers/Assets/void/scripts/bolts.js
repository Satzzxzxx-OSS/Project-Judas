// Cannon bolts: pooled render-only entities moved by script. Each fixed step a
// bolt sweeps a ray over its travel; hits are resolved here (no physics body per bolt).
import {world,physics} from 'judas';
import {SYSTEM} from './system_data.js';
import {G} from './shared.js';
import {add,sub,mul,dot,length,norm,lookRotation} from './math.js';

const HIDDEN={x:0,y:-5000,z:0};

export class Bolts {
 constructor(){
  this.pools={player:SYSTEM.ids.bolts.map(id=>({id,active:false})),enemy:SYSTEM.ids.enemyBolts.map(id=>({id,active:false}))};
  this.spark=world.entity(SYSTEM.ids.spark);
  this.stats={fired:0,hits:0,raiderHits:0,last:''};
 }
 fire(team,origin,velocity,owner,damage){
  const b=this.pools[team].find(b=>!b.active);if(!b)return false;
  if(team==='player')this.stats.fired++;
  Object.assign(b,{active:true,pos:origin,vel:velocity,life:team==='player'?2.0:3.2,owner,damage,team});
  this.place(b);return true;
 }
 place(b){const e=world.entity(b.id);if(e?.valid)e.transform=b.active?{position:b.pos,rotation:lookRotation(b.vel,{x:0,y:1,z:0})}:{position:HIDDEN};}
 tick(dt){
  for(const team of ['player','enemy'])for(const b of this.pools[team]){
   if(!b.active)continue;
   b.life-=dt;const step=mul(b.vel,dt),dist=length(step);
   if(b.life<=0||dist<1e-6){b.active=false;this.place(b);continue;}
   const ignored=b.owner?.valid?[b.owner]:[];
   const hit=physics.raycast(b.pos,step,dist,{ignored});
   // Retained after actual M65 review: its query capsule is registered, but
   // the cast target path still treats it as a zero-size box. Keep the
   // original game-side approximation until that generic defect is fixed.
   if(team==='enemy'&&G.mode==='foot'&&G.foot){const p=G.foot.entity.transform.position;
    const t=Math.max(0,Math.min(1,dot(sub(p,b.pos),step)/(dist*dist))),c=add(b.pos,mul(step,t));
    if(length(sub(p,c))<0.75&&(!hit||t*dist<hit.distance)){G.foot.damage(b.damage);this.impact(c,1);b.active=false;this.place(b);continue;}}
   if(hit){this.resolve(b,hit);b.active=false;this.place(b);continue;}
   b.pos=add(b.pos,step);this.place(b);
  }
 }
 resolve(b,hit){
  const e=hit.entity,id=e?.id;
  if(b.team==='player'){this.stats.hits++;this.stats.last=`${id}:${hit.shape}`;if(G.enemies.has(id))this.stats.raiderHits++;}
  this.impact(hit.point,b.team==='player'?6:4);
  // Push whatever dynamic thing was hit (ship, raiders, debris); static scenery ignores it.
  if(e?.valid&&hit.shape!=='terrain'){
   try{e.applyImpulseAtPoint(mul(norm(b.vel),b.team==='player'?900:1500),hit.point);}catch(err){/* static body: nothing to push */}
  }
  if(b.team==='player'&&G.enemies.has(id))G.enemies.get(id).damage(b.damage,hit.point);
  if(b.team==='enemy'&&id===G.foot?.entity.id&&G.mode==='foot')G.foot.damage(b.damage);
  if(b.team==='enemy'&&id===SYSTEM.ids.ship)G.ship?.damage(b.damage);
 }
 impact(point,count){if(this.spark?.valid){this.spark.transform={position:point};this.spark.burst(count);}}
 clear(){for(const team of ['player','enemy'])for(const b of this.pools[team]){b.active=false;this.place(b);}}
}
