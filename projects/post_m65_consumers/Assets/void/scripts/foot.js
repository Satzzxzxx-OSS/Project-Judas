// Pilot on foot: CharacterMotor walking on spherical planets (motor gravity/up),
// first-person look. Disabled while flying the ship.
import {world} from 'judas';
import {input} from './collection_input.js';
import {G,intent} from './shared.js';
import {add,sub,mul,dot,length,norm,tangent,clamp,damp,qm,axis,rotate,lookRotation} from './math.js';

const T={walk:4.5,run:8,accel:30,airAccel:6,jump:5.2,sens:0.0022};

export default class Foot {
 constructor({entity}){this.entity=entity;this.state={yaw:0,pitch:0,suit:100};this.stepDist=0;G.foot=this;}
 get active(){return G.mode==='foot';}
 damage(n){if(G.game?.over)return;this.state.suit=Math.max(0,this.state.suit-n);G.game?.sfx('hit',0.8);if(this.state.suit<=0)G.game?.pilotKilled();}

 /** Leave the ship: place the pilot, re-enable the motor, keep looking where the ship pointed. */
 placeAt(position,heading,up){
  const m=this.entity.character;
  // Body frame: +Y along local up; yaw stored separately so the view keeps the ship's heading.
  const rot=lookRotation(tangent(heading,up),up);
  this.entity.transform={position,rotation:rot};
  m.enabled=true;this.pendingVelocity={x:0,y:0,z:0};
  this.state.yaw=0;this.state.pitch=-0.1;
 }
 board(){const m=this.entity.character;if(m)m.enabled=false;this.entity.transform={position:{x:0,y:-3000,z:0}};}

 update(dt){
  if(!this.active||G.game?.paused)return;
  this.state.yaw-=input.axis('look_x')*T.sens+input.axis('look_stick_x')*2.4*dt;
  this.state.pitch=clamp(this.state.pitch-input.axis('look_y')*T.sens-input.axis('look_stick_y')*2.0*dt,-1.5,1.5);
 }
 fixedUpdate(dt){
  if(!this.active)return;
  const m=this.entity.character;if(!m)return;
  const st=m.state,up=st.up,rot=this.entity.transform.rotation;
  if(this.pendingVelocity){m.velocity=this.pendingVelocity;this.pendingVelocity=null;return;}
  const I=intent();
  if(G.auto&&I.yawRate)this.state.yaw+=I.yawRate*dt;
  const heading=qm(rot,axis({x:0,y:1,z:0},this.state.yaw));
  const fwd=norm(tangent(rotate(heading,{x:0,y:0,z:-1}),up)),right=norm(tangent(rotate(heading,{x:1,y:0,z:0}),up));
  let dir=add(mul(fwd,I.my),mul(right,I.mx));if(length(dir)>1)dir=norm(dir);
  const speed=I.boost?T.run:T.walk;let v=m.velocity;
  if(st.supported){
   const own=tangent(sub(v,st.supportVelocity),up),change=sub(mul(dir,speed),own),cl=length(change),mx=T.accel*dt;
   v=add(st.supportVelocity,cl>mx?add(own,mul(change,mx/cl)):mul(dir,speed));
   if(I.ascend&&!this.jumpHeld)v=add(v,mul(up,T.jump));
   this.stepDist+=length(own)*dt;if(this.stepDist>(I.boost?2.2:1.6)){this.stepDist=0;try{this.entity.playAudioOneShot();}catch(e){/* loading/capacity */}}
  }else if(length(dir)>0){v=add(v,mul(dir,T.airAccel*dt));}
  this.jumpHeld=I.ascend;
  m.velocity=v;
 }
 cameraPose(){
  const t=this.entity.presentedTransform;
  const rot=qm(qm(t.rotation,axis({x:0,y:1,z:0},this.state.yaw)),axis({x:1,y:0,z:0},this.state.pitch));
  return {position:add(t.position,rotate(t.rotation,{x:0,y:0.7,z:0})),rotation:rot,fov:72};
 }
}
