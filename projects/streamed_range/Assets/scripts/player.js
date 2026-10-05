// Game policy: speed, launch, look, fire and camera decisions are project JS.
import {input,world,scenes,session} from 'judas';
import Camera from './camera.js';
import Residency from './residency.js';
import Weapon from './weapon.js';
import HUD from './hud.js';
import {round} from './round.js';
import {add,mul,sub,dot,length,norm,tangent,qm,axis,rotate} from './math.js';
export const properties={speed:{type:'number',default:5},launchSpeed:{type:'number',default:5},shotImpulse:{type:'number',default:8}};
export default class {
 constructor({entity,properties}){this.entity=entity;this.props=properties;this.state={yaw:0,pitch:0,third:false,score:0,shots:0,hits:0,unique:0,lastHit:null,paused:false};}
 start(){this.camera=new Camera(this);this.weapon=new Weapon(this,this.props.shotImpulse);this.hud=new HUD(this);this.hud.start();this.camera.publish();this.residency=new Residency(this);this.residency.start();}
 update(dt){if(this.hud.doc.modal)return;this.residency.update();
  this.state.yaw-=input.axis('look_x')*.0021+input.axis('look_stick_x')*2.2*dt;
  this.state.pitch=Math.max(-1.45,Math.min(1.45,this.state.pitch-input.axis('look_y')*.0021-input.axis('look_stick_y')*2.2*dt));
  if(input.pressed('camera_toggle'))this.camera.third=!this.camera.third;
  if(input.pressed('restart'))scenes.reload();
 }
 fixedUpdate(dt){round.elapsed+=dt;round.flash=Math.max(0,round.flash-dt);this.weapon.tick(dt);
  const motor=this.entity.character,state=motor.state,up=motor.up,t=this.entity.transform;
  const heading=qm(t.rotation,axis({x:0,y:1,z:0},this.state.yaw));
  const forward=norm(tangent(rotate(heading,{x:0,y:0,z:-1}),up)),right=norm(tangent(rotate(heading,{x:1,y:0,z:0}),up));
  let direction=add(mul(forward,input.axis('move_y')),mul(right,input.axis('move_x')));if(length(direction)>1)direction=norm(direction);
  const desired=mul(direction,this.props.speed);let velocity=motor.velocity;
  if(state.supported){const own=tangent(sub(velocity,state.supportVelocity),up),change=sub(desired,own);
   velocity=add(state.supportVelocity,add(own,mul(norm(change),Math.min(length(change),25*dt))));
   if(input.pressed('jump'))velocity=add(velocity,mul(up,this.props.launchSpeed));
  }else if(length(direction)>0){const speed=dot(tangent(velocity,up),norm(direction));if(speed<this.props.speed)velocity=add(velocity,mul(norm(direction),Math.min(8*dt,this.props.speed-speed)));}
  motor.velocity=this.residency.constrain(velocity,dt);this.residency.fixedUpdate();
  if(input.pressed('fire'))this.weapon.fire();
  Object.assign(this.state,{third:this.camera.third,score:round.score,shots:round.shots,hits:round.hits,unique:round.unique,lastHit:this.weapon.report.hit,scored:this.weapon.report.scored});
  if(round.score>(session.get('rangeBest')||0))session.set('rangeBest',round.score);
 }
 // Presentation follows the SAME interpolated pose as the world renderer.
 // Queries/intent above deliberately keep using authoritative fixed-step poses.
 presentationUpdate(){
  const t=this.entity.presentedTransform,up=rotate(t.rotation,{x:0,y:1,z:0});
  const heading=qm(t.rotation,axis({x:0,y:1,z:0},this.state.yaw));
  const avatar=world.entity('11'),head=world.entity('12');
  const base=add(t.position,mul(up,this.camera.third?0:-100));
  if(avatar?.valid)avatar.transform={position:base,rotation:heading};
  if(head?.valid)head.transform={position:add(base,mul(up,.65)),rotation:heading};
  this.camera.publish(true);
 }
 uiUpdate(){this.hud.update();this.residency.uiUpdate();}
 onUI(e){this.hud.event(e);}
 destroy(){this.residency?.destroy();input.pointerCapture=false;world.clearView();}
}
