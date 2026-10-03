import Rules from './rules.js';
import {input,ui,world,scenes} from 'judas';
export const properties={controlled:{type:'boolean',default:true},speed:{type:'number',default:4},launchSpeed:{type:'number',default:5},density:{type:'number',default:950}};
const add=(a,b)=>({x:a.x+b.x,y:a.y+b.y,z:a.z+b.z});
const mul=(a,s)=>({x:a.x*s,y:a.y*s,z:a.z*s});
const dot=(a,b)=>a.x*b.x+a.y*b.y+a.z*b.z;
const length=a=>Math.sqrt(dot(a,a));
const norm=a=>length(a)>1e-6?mul(a,1/length(a)):{x:0,y:0,z:0};
const tangent=(a,u)=>add(a,mul(u,-dot(a,u)));
const qm=(a,b)=>({w:a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z,x:a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,y:a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,z:a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w});
const axis=(a,t)=>({w:Math.cos(t/2),...mul(a,Math.sin(t/2))});
const rotate=(q,v)=>{const r=qm(qm(q,{w:0,...v}),{w:q.w,x:-q.x,y:-q.y,z:-q.z});return {x:r.x,y:r.y,z:r.z}};
export default class {
 constructor({entity,properties}){this.entity=entity;this.props=properties;this.state={yaw:0,pitch:0};this.elapsed=0;this.rules=new Rules(this);}
 start(){if(!this.props.controlled)return;input.pointerCapture=true;this.hud=ui.get('game_ui');ui.debugOverlayVisible=false;
  if(this.hud){this.hud.modal=false;for(const id of ['main','options','pause','image'])this.hud.get(id).visible=false;this.hud.get('hud').visible=true;
   this.hud.get('hud_title').text='M51 / ENGINE PRIMITIVES, JS GAME RULES';
   this.hud.get('hud_help').text='WASD + mouse / controller: move/look | Space: script launch\nF1: flat | F2: radial planet | F3: existing pool geometry\nG: door/pickup/drop | H: throw | V: camera | F: craft control | X: SAS\nZ: spawn | Y: remove | R: reload | Escape: pause';
   this.hud.get('pause_options').text='Workshop / flight / planet / pool';}
  this.camera();
 }
 camera(){if(this.rules.camera())return;const t=this.entity.transform;const rotation=qm(qm(t.rotation,axis({x:0,y:1,z:0},this.state.yaw)),axis({x:1,y:0,z:0},this.state.pitch));
  const eye=add(t.position,rotate(t.rotation,{x:0,y:.7,z:0}));
  world.setView({position:this.rules.third?add(eye,rotate(rotation,{x:0,y:1,z:4})):eye,rotation},70);}
 update(dt){if(!this.props.controlled)return;if(this.hud?.modal)return;
  this.state.yaw-=(input.axis('look_x')*.0021+input.axis('look_stick_x')*2.2*dt);
  this.state.pitch=Math.max(-1.55,Math.min(1.55,this.state.pitch-input.axis('look_y')*.0021-input.axis('look_stick_y')*2.2*dt));
  if(input.pressed('motor_flat'))scenes.load('Scenes/flat.judas');if(input.pressed('motor_planet'))scenes.load('Scenes/planet.judas');if(input.pressed('motor_pool'))scenes.load('Scenes/pool.judas');
  this.rules.update();
  if(input.pressed('spawn')){const t=this.entity.transform;t.position=add(t.position,rotate(t.rotation,{x:2,y:1,z:-2}));world.spawnPrefab('49494949494949494949494949494903',t);}
  if(input.pressed('reset'))scenes.reload();this.camera();
 }
 fixedUpdate(dt){if(this.props.controlled&&this.rules.fixedUpdate(dt))return;const motor=this.entity.character;if(!motor)return;this.elapsed+=dt;
  const state=motor.state,up=motor.up,pose=this.entity.transform;
  const heading=qm(pose.rotation,axis({x:0,y:1,z:0},this.state.yaw));
  const forward=norm(tangent(rotate(heading,{x:0,y:0,z:-1}),up)),right=norm(tangent(rotate(heading,{x:1,y:0,z:0}),up));
  let direction=this.props.controlled?add(mul(forward,input.axis('move_y')),mul(right,input.axis('move_x'))):mul(right,Math.sin(this.elapsed));
  if(length(direction)>1)direction=norm(direction);
  const desired=mul(direction,this.props.speed),support=state.supportVelocity;
  let velocity=motor.velocity;
  if(state.supported){const own=tangent(add(velocity,mul(support,-1)),up);const change=add(desired,mul(own,-1));const rate=length(direction)>0?20:28;
   velocity=add(support,add(own,mul(norm(change),Math.min(length(change),rate*dt))));
   if(this.props.controlled&&input.pressed('jump'))velocity=add(velocity,mul(up,this.props.launchSpeed));
  }else if(length(direction)>0){const speed=dot(tangent(velocity,up),norm(direction));if(speed<this.props.speed)velocity=add(velocity,mul(norm(direction),Math.min(8*dt,this.props.speed-speed)));}
  // Swimming is project behaviour using the SAME existing approximate field.
  // Gravity still enters once through the motor. Opposing acceleration below
  // is explicit, not a second hidden gravity model or particle collision push.
  const liquid=world.fluidSample(pose.position,up,.9,.3,right);this.immersion=liquid.immersion;
  if(liquid.immersion>0){const buoyancy=mul(add(motor.gravity,mul(liquid.acceleration,-1)),-liquid.density/this.props.density*liquid.immersion);
   let direction3=this.props.controlled?add(mul(rotate(qm(heading,axis({x:1,y:0,z:0},this.state.pitch)),{x:0,y:0,z:-1}),input.axis('move_y')),mul(right,input.axis('move_x'))):{x:0,y:0,z:0};
   if(this.props.controlled&&input.held('jump'))direction3=add(direction3,up);
   const relaxation=Math.exp(-2*liquid.immersion*dt);velocity=add(liquid.velocity,mul(add(velocity,mul(liquid.velocity,-1)),relaxation));
   motor.accelerate(add(buoyancy,mul(norm(direction3),4*liquid.immersion)));}
  motor.velocity=velocity;
 }
 uiUpdate(){if(!this.props.controlled||!this.hud)return;
  if(input.pressed('pause')){this.hud.modal=!this.hud.modal;this.hud.get('pause').visible=this.hud.modal;}
  const s=this.entity.character.state;this.hud.get('counter').text=`${scenes.current}\nCraft: ${!!this.rules.craft} | SAS: ${this.rules.sas} | Held: ${this.rules.held?.id||'none'}\nSupport: ${s.supported} | actual speed: ${length(s.velocity).toFixed(2)} m/s\nImmersion: ${(100*(this.immersion||0)).toFixed(0)}% | JS owns controls and launch`;}
 onUI(e){if(e.type==='back'||(e.type==='click'&&e.element==='resume')){this.hud.modal=false;this.hud.get('pause').visible=false;}
  if(e.type==='click'&&e.element==='pause_options')scenes.load(['Scenes/flat.judas','Scenes/flight.judas','Scenes/planet.judas','Scenes/pool.judas'][(['Scenes/flat.judas','Scenes/flight.judas','Scenes/planet.judas','Scenes/pool.judas'].indexOf(scenes.current)+1)%4]);
  if(e.type==='click'&&e.element==='pause_quit')ui.quit();}
 destroy(){if(this.props.controlled){input.pointerCapture=false;world.clearView();}}
}
