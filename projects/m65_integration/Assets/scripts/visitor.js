import {input,world,ui} from 'judas';
import {add,mul,rotate,axis,qm,tangent,norm} from './math.js';
export default class {constructor({entity}){this.entity=entity;this.state={yaw:0,pitch:0,steps:0,inputSeen:false};this.jump=false;}
 start(){input.pointerCapture=true;}restore(){this.start();}
 update(){if(ui.get('integration')?.modal)return;this.state.yaw-=input.axis('look_x')*.0022;this.state.pitch=Math.max(-1.35,Math.min(1.35,this.state.pitch-input.axis('look_y')*.0022));this.jump ||= input.pressed('jump');}
 fixedUpdate(){const c=this.entity.character,up=c.up,q=axis(up,this.state.yaw),f=rotate(q,{x:0,y:0,z:-1}),r=rotate(q,{x:1,y:0,z:0});const x=input.axis('move_x'),y=input.axis('move_y');
  let v=add(mul(r,x*5),mul(f,y*5));const old=c.velocity,vertical=add(old,mul(up,-(old.x*up.x+old.y*up.y+old.z*up.z)));if(!c.supported)v=add(v,add(old,mul(vertical,-1)));else v=add(v,c.supportVelocity);
  if(this.jump&&c.supported)v=add(v,mul(up,5));this.jump=false;c.velocity=v;this.state.steps++;this.state.inputSeen ||= !!(x||y);
 }
 presentationUpdate(){const t=this.entity.presentedTransform,up=this.entity.character.up,q=qm(axis(up,this.state.yaw),axis({x:1,y:0,z:0},this.state.pitch));world.setView({position:add(t.position,mul(up,.65)),rotation:q},65);}
 destroy(){input.pointerCapture=false;}}
