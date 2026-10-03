// This is project policy. Nothing here is a privileged engine/demo API.
import {input,world,physics} from 'judas';
const add=(a,b)=>({x:a.x+b.x,y:a.y+b.y,z:a.z+b.z});
const mul=(v,s)=>({x:v.x*s,y:v.y*s,z:v.z*s});
const sub=(a,b)=>add(a,mul(b,-1));
const dot=(a,b)=>a.x*b.x+a.y*b.y+a.z*b.z;
const length=v=>Math.sqrt(dot(v,v));
const bounded=(v,n)=>length(v)>n?mul(v,n/length(v)):v;
const qm=(a,b)=>({w:a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z,x:a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,y:a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,z:a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w});
const inverse=q=>({w:q.w,x:-q.x,y:-q.y,z:-q.z});
const rotate=(q,v)=>{const p=qm(qm(q,{w:0,...v}),inverse(q));return {x:p.x,y:p.y,z:p.z};};
const matrix=(m,v)=>add(add(mul(m.x,v.x),mul(m.y,v.y)),mul(m.z,v.z));
const zero=()=>({x:0,y:0,z:0});
function attitudeError(target,current){let q=qm(target,inverse(current));if(q.w<0)q={w:-q.w,x:-q.x,y:-q.y,z:-q.z};const n=Math.hypot(q.x,q.y,q.z);return n>1e-6?mul(q,2*Math.atan2(n,q.w)/n):zero();}
export default class Rules {
 constructor(controller){this.c=controller;this.held=null;this.craft=null;this.third=false;this.sas=false;this.attitude=null;this.doors=new Map();}
 ray(){const t=this.c.entity.transform;const y=this.c.state.yaw/2,p=this.c.state.pitch/2;const q=qm(qm(t.rotation,{w:Math.cos(y),x:0,y:Math.sin(y),z:0}),{w:Math.cos(p),x:Math.sin(p),y:0,z:0});return {origin:add(t.position,rotate(t.rotation,{x:0,y:.7,z:0})),direction:rotate(q,{x:0,y:0,z:-1}),rotation:q};}
 target(){const r=this.ray();return physics.raycast(r.origin,r.direction,5,{ignored:[this.c.entity]});}
 update(){
  if(this.held&&!this.held.valid)this.held=null;if(this.craft&&!this.craft.valid){this.craft=null;this.c.entity.character.enabled=true;}
  if(input.pressed('view_toggle'))this.third=!this.third;
  if(input.pressed('interact')){
   if(this.held)this.held=null;
   else {const hit=this.target();if(hit?.entity?.hasTag('pickup'))this.held=hit.entity;
    else if(hit?.entity?.hasTag('door')){const owner=world.entity('62'),joint=physics.joint(owner);if(joint){const open=!this.doors.get(hit.entity.id);this.doors.set(hit.entity.id,open);this.c.state.doorOpen=open;joint.setSpring(open?-1.2:0,80,20);}}}
  }
  if(input.pressed('throw')&&this.held){this.held.applyImpulse(mul(this.ray().direction,this.held.mass*8));this.held=null;}
  if(input.pressed('destroy')){const target=this.held||this.target()?.entity;if(target?.hasTag('pickup'))target.destroy();this.held=null;}
  if(input.pressed('control_toggle')){
   if(this.craft){const body=this.craft,t=body.transform;this.craft=null;this.c.entity.transform={position:add(t.position,rotate(t.rotation,{x:3,y:1,z:0})),rotation:t.rotation};this.c.entity.character.enabled=true;this.departure=body.velocity;}
   else {const hit=this.target();if(hit?.entity?.hasTag('craft')){this.held=null;this.craft=hit.entity;this.c.entity.character.enabled=false;}}
  }
  if(input.pressed('sas_toggle')&&this.craft){this.sas=!this.sas;this.attitude=this.craft.transform.rotation;}
  this.c.state.held=this.held?.id||null;this.c.state.piloting=!!this.craft;this.c.state.sas=this.sas;this.c.state.third=this.third;
 }
 fixedUpdate(){
  if(this.departure){this.c.entity.character.velocity=this.departure;this.departure=null;}
  if(this.held?.valid){const ray=this.ray(),target=add(ray.origin,mul(ray.direction,1.7));const velocity=this.c.entity.character.velocity;
   // Finite force holding: same bounded PD acceleration as the historical demo.
   const acceleration=bounded(add(mul(sub(target,this.held.transform.position),70),mul(sub(velocity,this.held.velocity),17)),70);
   this.held.applyForce(mul(acceleration,this.held.mass));
   const alpha=bounded(sub(mul(attitudeError(ray.rotation,this.held.transform.rotation),81),mul(this.held.angularVelocity,18)),100);
   this.held.applyTorque(matrix(this.held.inertiaWorld,alpha));
  }
  if(!this.craft?.valid)return false;
  const t=this.craft.transform,local={x:input.axis('move_x'),y:input.axis('move_z'),z:-input.axis('move_y')};
  this.craft.applyForce(rotate(t.rotation,mul(bounded(local,1),1200)));
  if(this.sas){const alpha=sub(mul(attitudeError(this.attitude,t.rotation),25),mul(this.craft.angularVelocity,10));this.craft.applyTorque(matrix(this.craft.inertiaWorld,alpha));}
  else this.craft.applyTorque(rotate(t.rotation,{x:input.axis('pitch')*450,y:input.axis('yaw')*450,z:input.axis('roll')*450}));
  // An explicit seat policy for a non-rigid motor entity, not rigid-body holding.
  this.c.entity.transform={position:add(t.position,rotate(t.rotation,{x:0,y:1,z:0})),rotation:t.rotation};return true;
 }
 camera(presented=false){if(!this.craft?.valid)return false;const t=presented?this.craft.presentedTransform:this.craft.transform;world.setView({position:add(t.position,rotate(t.rotation,this.third?{x:0,y:3,z:8}:{x:0,y:1,z:0})),rotation:t.rotation},70);return true;}
}
