// Rooftop Run - first-person parkour controller.
// Judas supplies the CharacterMotor (collision-aware capsule motion) and
// geometric queries; every movement rule below is game code.
//
// Phases: look input in update(); all motion intent in fixedUpdate();
// camera in presentationUpdate() from the interpolated pose.
import {input,world,physics,ui,console} from 'judas';
import {add,sub,mul,dot,cross,length,norm,tangent,clamp,damp,lerp,qm,axis,rotate} from './math.js';
import {scenarios} from './autopilot.js';

/** Shared with course.js (same module instance within the scene's VM). */
export const link={runner:null};

export const properties={
 autopilot:{type:'string',default:''},  // test scenario name (see autopilot.js); empty = player input
 logEvery:{type:'number',default:0},     // >0: log JSON state every N fixed steps
 sensitivity:{type:'number',default:0.0022},
};

// Tuning (metres, seconds, m/s, m/s^2).
const T={
 runMax:8.4, strafeMax:5, backMax:3.5,
 accelLow:18, accelMid:6, accelHigh:3,   // speed build-up bands: <4, <6.5, above
 groundGrip:45, groundBrake:22,
 airAccel:7, airMax:5.5,
 jumpSpeed:5.1, coyote:0.12, jumpBuffer:0.15,
 gravityRise:1.0, gravityRiseReleased:1.8, gravityFall:1.35, terminal:45,
 standCentre:0.9, eyeStand:0.72, eyeCrouch:-0.18,
 vaultReach:1.4, ledgeReach:2.25, vaultDetect:1.1, ledgeDetect:0.55,
 wallrunMin:4.2, wallrunTime:1.25, wallrunGravity:0.42, wallrunProbe:1.0,
 wallJumpOut:4.2, wallJumpUp:5.2,
 climbSpeed:5.4, climbTime:0.5,
 slideBoost:1.4, slideMax:10, slideFriction:3.2, slideMinTime:0.45,
 hardLanding:9.6, fatalLanding:17.5, rollWindow:0.5,
};
// Authored non-spatial emitter entities (see tools/build_course.py).
const SFX={wind:'20',step:'21',land:'22',whoosh:'23'};
const STAND={halfHeight:0.6,offset:{x:0,y:0,z:0}};
const CROUCH={halfHeight:0.15,offset:{x:0,y:-0.45,z:0}};

export default class Runner {
 constructor({entity,properties}){
  this.entity=entity;this.props=properties;
  // Motor capsules participate in ordinary queries. Movement probes inspect
  // the environment, so exclude this instance with the normal safe-handle filter.
  this.queryFilter={ignored:[entity]};
  this.state={yaw:0,pitch:0};
  this.mode='air';this.speed=0;this.time=0;this.steps=0;
  this.sinceGround=1;this.sinceJumpPress=1;this.jumpLock=0;this.sinceSlidePress=1;
  this.crouched=false;this.climbUsed=false;this.wallCooldown=0;this.lastWall=null;
  this.tr=null;this.wr=null;this.cl=null;this.sl=null;
  this.stun=0;this.lastLanding=0;this.pending=null;this.yawTurn=0;
  // Presentation-only state.
  this.cam={eye:T.eyeStand,roll:0,dip:0,dipV:0,bob:0,fov:75,rollAnim:0,rollPhase:0,turn:0};
  this.logEvents=[];
  this.events=[];         // one-shot notifications for course.js / HUD (landing, death...)
  link.runner=this;
 }

 start(){
  const m=this.entity.character;
  // Gravity is applied by this script (variable jump/fall/wall-run gravity).
  m?.configure({gravityScale:0,...STAND});
  const t=this.entity.transform;this.spawn={position:t.position,yaw:this.state.yaw};
  if(this.props.autopilot){this.auto=scenarios[this.props.autopilot];
   if(!this.auto)console.log('autopilot: unknown scenario '+this.props.autopilot);
   else if(this.auto.spawn)this.respawn({x:this.auto.spawn[0],y:this.auto.spawn[1],z:this.auto.spawn[2]},this.auto.yaw||0);}
 }

 /** Teleport (applied on the next fixed step). */
 respawn(position,yaw){this.pending={position,yaw};}

 // ------------------------------------------------------------- input
 update(dt){
  if(this.auto)return;
  const s=this.props.sensitivity;
  this.state.yaw-=input.axis('look_x')*s+input.axis('look_stick_x')*2.6*dt;
  this.state.pitch=clamp(this.state.pitch-input.axis('look_y')*s-input.axis('look_stick_y')*2.2*dt,-1.5,1.5);
 }
 readIntent(){
  if(this.auto){const i=this.autoIntent();return i;}
  return {mx:input.axis('move_x'),my:input.axis('move_y'),
   jumpPressed:input.pressed('jump'),jumpHeld:input.held('jump'),
   slidePressed:input.pressed('slide'),slideHeld:input.held('slide')};
 }
 autoIntent(){
  const steps=this.auto.steps,pos=this.entity.transform.position;
  if(this.autoIdx===undefined){this.autoIdx=-1;this.autoT=0;}
  let fresh=false;
  const cur=steps[this.autoIdx];
  if(this.autoIdx<0||(cur&&(this.time-this.autoT>=(cur.d??1e9)||(cur.z!==undefined&&pos.z<cur.z)))){
   this.autoIdx++;this.autoT=this.time;fresh=true;
   if(this.autoIdx<steps.length)console.log('AUTO step '+this.autoIdx+' '+JSON.stringify(steps[this.autoIdx]));
   else console.log('AUTO done '+JSON.stringify({t:this.time,p:pos,mode:this.mode}));
  }
  const o=steps[this.autoIdx];
  if(!o)return {mx:0,my:0,jumpPressed:false,jumpHeld:false,slidePressed:false,slideHeld:false};
  if(fresh&&o.yaw!==undefined)this.state.yaw=o.yaw;
  return {mx:o.mx||0,my:o.my||0,jumpPressed:fresh&&!!o.jump,jumpHeld:!!o.jump,slidePressed:fresh&&!!o.slide,slideHeld:!!o.slide};
 }

 // ------------------------------------------------------------- frame helpers
 frame(up){
  const rot=this.entity.transform.rotation;
  const heading=qm(rot,axis({x:0,y:1,z:0},this.state.yaw));
  const fwd=norm(tangent(rotate(heading,{x:0,y:0,z:-1}),up)),right=norm(tangent(rotate(heading,{x:1,y:0,z:0}),up));
  return {fwd,right};
 }
 setCrouch(on){
  if(on===this.crouched)return true;
  const m=this.entity.character;
  if(!on&&!this.canStand())return false;
  m.configure(on?CROUCH:STAND);this.crouched=on;return true;
 }
 canStand(){
  const p=this.entity.transform.position,up=this.up;
  const hit=physics.capsuleCast({position:add(p,mul(up,0.03)),rotation:this.entity.transform.rotation},0.28,0.58,up,0.02,this.queryFilter);
  return !hit;
 }
 /** Looks for a climbable edge in `dir`. Returns {top,height,normal,point,distance} or null. */
 probeLedge(pos,dir,minRel,maxRel,detect){
  const up=this.up,rot=this.entity.transform.rotation;
  const feet=dot(pos,up)-(this.crouched?T.standCentre:T.standCentre);
  const face=physics.capsuleCast({position:pos,rotation:rot},0.27,this.crouched?0.15:0.6,dir,detect,this.queryFilter);
  if(!face||Math.abs(dot(face.normal,up))>0.5)return null;
  if(face.initialOverlap)return null;
  // Point just behind the face, above the reachable height; cast down onto the top.
  const inside=tangent(add(face.point,mul(dir,0.32)),up);
  const startH=feet+maxRel+0.35;
  const down=physics.raycast(add(inside,mul(up,startH)),mul(up,-1),maxRel+0.35-minRel+0.05,this.queryFilter);
  if(!down||down.initialOverlap||down.distance<0.02)return null;
  if(dot(down.normal,up)<0.75)return null;
  const top=startH-down.distance,rel=top-feet;
  if(rel<minRel||rel>maxRel)return null;
  // Room to stand on top?
  const standAt=add(add(inside,mul(up,top+T.standCentre+0.06)),mul(dir,0.1));
  if(physics.capsuleCast({position:standAt,rotation:rot},0.28,0.58,up,0.02,this.queryFilter))return null;
  // Room to rise from where we are to that height?
  const rise=top+T.standCentre+0.06-dot(pos,up);
  if(rise>0){const over=physics.capsuleCast({position:pos,rotation:rot},0.27,this.crouched?0.15:0.6,up,rise,this.queryFilter);if(over&&!over.initialOverlap)return null;}
  return {top,rel,normal:face.normal,distance:face.distance,point:face.point};
 }
 wallAt(pos,dir,dist){
  const hit=physics.raycast(pos,dir,dist,this.queryFilter);
  if(!hit||Math.abs(dot(hit.normal,this.up))>0.25||dot(hit.normal,dir)>-0.3)return null;
  return hit;
 }
 emit(type,data={}){this.events.push({type,...data});if(this.events.length>16)this.events.shift();this.logEvents.push(type);
  if(['jump','walljump','vault','ledge','wallrun','slide','climb'].includes(type))this.sfx(SFX.whoosh,type==='slide'?0.6:0.45);
  if(type==='roll')this.sfx(SFX.land,0.5);if(type==='hardlanding')this.sfx(SFX.land,1);}
 /** Fire-and-forget one-shot on an authored emitter entity. */
 sfx(id,volume){const e=world.entity(id);if(!e?.valid)return;
  try{e.setAudio({volume});e.playAudioOneShot();}
  catch(err){/* clip still loading or one-shot capacity full: skip this sound */}}

 // ------------------------------------------------------------- simulation
 fixedUpdate(dt){
  const m=this.entity.character;if(!m)return;
  this.time+=dt;this.steps++;
  const st=m.state;const up=norm(st.up);this.up=up;
  const g=st.gravity,gmag=length(g)||9.81,down=gmag>1e-3?mul(g,1/gmag):mul(up,-1);
  if(this.pending){const p=this.pending;this.pending=null;this.setCrouch(false)||(m.configure(STAND),this.crouched=false);
   this.entity.transform={position:p.position};m.velocity={x:0,y:0,z:0};
   this.state.yaw=p.yaw;this.state.pitch=0;this.speed=0;this.mode='air';this.tr=this.wr=this.cl=this.sl=null;
   this.stun=0;this.climbUsed=false;this.sinceGround=1;this.emit('respawn');return;}
  const I=this.readIntent();
  const pos=this.entity.transform.position;
  const {fwd,right}=this.frame(up);
  let wish=add(mul(fwd,I.my),mul(right,I.mx));const wishLen=Math.min(1,length(wish));wish=norm(wish);
  const fwdIn=I.my;

  this.sinceJumpPress=I.jumpPressed?0:this.sinceJumpPress+dt;
  this.sinceSlidePress=I.slidePressed?0:this.sinceSlidePress+dt;
  this.jumpLock=Math.max(0,this.jumpLock-dt);this.wallCooldown=Math.max(0,this.wallCooldown-dt);this.stun=Math.max(0,this.stun-dt);

  let vel=m.velocity;
  const supportV=st.supportVelocity;
  const supported=st.supported&&this.jumpLock<=0&&this.mode!=='traverse';
  const fallSpeed=-dot(vel,up);

  // ---- landing ----------------------------------------------------------
  if(supported&&(this.mode==='air'||this.mode==='wallrun'||this.mode==='climb')){
   this.landed(fallSpeed,I.slideHeld);
   if(this.mode==='dead')return;
  }
  if(supported){this.sinceGround=0;this.climbUsed=false;}else this.sinceGround+=dt;
  if(!supported&&(this.mode==='ground')){this.mode='air';
   // Walking off an edge: keep a gentle start to the fall instead of any
   // downward velocity picked up while the capsule rounded the corner.
   vel=add(tangent(vel,up),mul(up,Math.max(dot(vel,up),-1)));}
  if(!supported&&this.mode==='slide'){this.sl=null;this.mode='air';}

  const own=tangent(sub(vel,supportV),up);
  let hSpeed=length(own),vy=dot(vel,up);

  // ---- mode dispatch ----------------------------------------------------
  switch(this.mode){
   case 'traverse':{vel=this.stepTraverse(dt,pos,up,vel);break;}
   case 'wallrun':{vel=this.stepWallrun(dt,I,pos,up,vel,gmag);break;}
   case 'climb':{vel=this.stepClimb(dt,I,pos,up,vel,gmag,fwd);break;}
   case 'slide':{vel=this.stepSlide(dt,I,pos,up,vel,wish,supportV,st);break;}
   case 'dead':{vel={x:0,y:0,z:0};break;}
   default:{
    if(supported){this.mode='ground';vel=this.stepGround(dt,I,pos,up,own,hSpeed,wish,wishLen,fwdIn,fwd,supportV,st,down,gmag);}
    else vel=this.stepAir(dt,I,pos,up,vel,own,hSpeed,vy,wish,wishLen,fwdIn,fwd,right,gmag);
   }
  }
  // Crouched while walking out from under something: stand when possible.
  if(this.crouched&&this.mode!=='slide')this.setCrouch(false);
  m.velocity=vel;
  this.debugLog(st,pos,vel);
 }

 landed(fallSpeed,slideHeld){
  if(this.mode==='traverse')return;
  const prevMode=this.mode;this.wr=null;this.cl=null;
  if(fallSpeed>T.fatalLanding){this.mode='dead';this.emit('death',{reason:'fall'});return;}
  if(fallSpeed>T.hardLanding){
   if(this.sinceSlidePress<T.rollWindow||slideHeld){this.mode='ground';this.cam.rollAnim=1;this.speed=Math.max(this.speed*0.95,5);this.emit('roll');}
   else{this.mode='ground';this.stun=0.45;this.speed*=0.15;this.cam.dipV-=2.6;this.emit('hardlanding');}
  }else{this.mode='ground';this.cam.dipV-=Math.min(1.6,fallSpeed*0.2);if(fallSpeed>2.5)this.sfx(SFX.land,clamp(fallSpeed/12,0.15,0.6));}
  this.lastLanding=fallSpeed;
 }

 stepGround(dt,I,pos,up,own,hSpeed,wish,wishLen,fwdIn,fwd,supportV,st,down,gmag){
  // The motor's support normal is a capsule contact normal (tilted at roof
  // edges); use the surface directly below for slope-following instead.
  const below=physics.raycast(pos,mul(up,-1),1.4,this.queryFilter);
  const n=below&&dot(below.normal,up)>0.6?below.normal:up;
  // Jump / vault (buffered press or coyote handled here and in air).
  if(this.sinceJumpPress<T.jumpBuffer&&this.stun<=0){
   const dir=hSpeed>1.5?norm(own):fwd;
   if(fwdIn>0.2){const ledge=this.probeLedge(pos,dir,0.35,T.vaultReach,T.vaultDetect+hSpeed*0.06);
    if(ledge){this.sinceJumpPress=1;return this.startTraverse(ledge,dir,'vault',Math.max(hSpeed,4.5),up);}}
   this.sinceJumpPress=1;return this.jump(own,up,supportV);
  }
  // Slide.
  if(this.sinceSlidePress<0.1&&hSpeed>4&&this.stun<=0){this.sinceSlidePress=1;return this.startSlide(own,hSpeed,up,supportV);}
  // Speed model: momentum builds while running forward, collapses when blocked.
  this.speed=Math.min(this.speed,hSpeed+2.5);
  const forwardness=wishLen>0.1?dot(wish,fwd):0;
  let cap=(forwardness>0.5?T.runMax:forwardness>-0.3?T.strafeMax:T.backMax)*wishLen;
  if(this.stun>0)cap=Math.min(cap,1.5);
  if(wishLen>0.1){
   if(this.speed<cap){const a=this.speed<4?T.accelLow:this.speed<6.5?T.accelMid:T.accelHigh;this.speed=Math.min(cap,this.speed+a*dt);}
   else this.speed=Math.max(cap,this.speed-T.groundBrake*dt);
  }else this.speed=Math.max(0,this.speed-T.groundBrake*dt);
  const dir=wishLen>0.1?wish:(hSpeed>0.05?norm(own):wish);
  let target=mul(dir,this.speed);
  // Move toward the target (strong grip), then lay it on the support plane.
  const change=sub(target,own),cl=length(change),maxStep=T.groundGrip*dt;
  let h=cl>maxStep?add(own,mul(change,maxStep/cl)):target;
  // Follow the slope only downhill. Uphill, keep horizontal intent and let the
  // motor slide up the surface: any upward velocity component makes the
  // CharacterMotor treat the step as departing its support.
  if(dot(n,up)>0.5){const hs=length(h);const onPlane=tangent(h,n);if(dot(onPlane,up)<0&&length(onPlane)>1e-4)h=mul(norm(onPlane),hs);}
  // Small downward bias keeps the motor's support probe engaged over edges/steps.
  return add(add(supportV,h),mul(up,-0.6));
 }

 jump(own,up,supportV){
  this.jumpLock=0.12;this.mode='air';this.sinceGround=T.coyote; // no coyote re-jump, but wall-climb window stays open
  this.speed=Math.min(T.runMax+0.6,this.speed+0.25);
  this.emit('jump');
  return add(add(supportV,own),mul(up,T.jumpSpeed));
 }

 stepAir(dt,I,pos,up,vel,own,hSpeed,vy,wish,wishLen,fwdIn,fwd,right,gmag){
  // Coyote jump.
  if(this.sinceJumpPress<T.jumpBuffer&&this.sinceGround<T.coyote&&this.jumpLock<=0&&vy<=0.5){
   this.sinceJumpPress=1;return this.jump(own,up,{x:0,y:0,z:0});}
  // Ledge grab: automatic while pushing forward near the apex / descending.
  if(fwdIn>0.3&&vy<3.5){const dir=hSpeed>1.5&&dot(norm(own),fwd)>0.3?norm(own):fwd;
   const ledge=this.probeLedge(pos,dir,0.25,T.ledgeReach,T.ledgeDetect+hSpeed*0.03);
   if(ledge)return this.startTraverse(ledge,dir,'ledge',Math.max(hSpeed,3),up);}
  // Wall climb: jump into a wall head-on while holding jump + forward.
  if(!this.climbUsed&&I.jumpHeld&&fwdIn>0.5&&vy>-1.5&&this.sinceGround<0.6){
   const wall=this.wallAt(pos,fwd,0.75);
   if(wall&&dot(wall.normal,fwd)<-0.8){this.climbUsed=true;this.mode='climb';this.cl={t:0,normal:wall.normal};this.emit('climb');
    return add(mul(wall.normal,-0.5),mul(up,T.climbSpeed));}
  }
  // Wall-run: moving fast past a wall to either side while holding forward.
  if(fwdIn>0.3&&hSpeed>T.wallrunMin&&vy>-6&&this.wallCooldown<=0){
   const vdir=norm(own),side=norm(cross(up,vdir));
   for(const s of [side,mul(side,-1)]){
    const wall=this.wallAt(pos,s,T.wallrunProbe);if(!wall)continue;
    const n=wall.normal;if(Math.abs(dot(vdir,n))>0.75)continue;
    if(this.lastWall&&dot(this.lastWall,n)>0.95&&this.wallCooldown>-0.3)continue;
    const along=norm(tangent(tangent(own,up),n));
    this.mode='wallrun';this.wr={t:0,normal:n,along,speed:Math.max(hSpeed,6.8),side:dot(s,right)>0?1:-1};
    this.emit('wallrun');
    return add(add(mul(along,this.wr.speed),mul(n,-0.8)),mul(up,clamp(vy,1.8,3.2)));
   }
  }
  // Air control: steer, but never add speed beyond max(current, airMax).
  let h=own;
  if(wishLen>0.1){const along=dot(h,wish),limit=Math.max(T.airMax,hSpeed);
   if(along<limit){h=add(h,mul(wish,Math.min(T.airAccel*dt*wishLen,limit-along)));const l=length(h);if(l>Math.max(hSpeed,T.airMax))h=mul(h,Math.max(hSpeed,T.airMax)/l);}}
  const mult=vy>0?(I.jumpHeld?T.gravityRise:T.gravityRiseReleased):T.gravityFall;
  vy=Math.max(-T.terminal,vy-gmag*mult*dt);
  this.speed=Math.max(this.speed,Math.min(length(h),T.runMax+1));
  return add(h,mul(up,vy));
 }

 // ------------------------------------------------------------- traversal (vault / ledge)
 startTraverse(ledge,dir,kind,outSpeed,up){
  const rise=Math.max(0.1,ledge.top+T.standCentre+0.06-dot(this.entity.transform.position,up));
  this.mode='traverse';this.wr=null;this.cl=null;
  this.tr={kind,dir,t:0,phase:'rise',targetH:ledge.top+T.standCentre+0.06,outSpeed,
   riseSpeed:kind==='vault'?Math.max(5,rise/0.2):Math.max(3.6,rise/0.42),travel:0,overDist:kind==='vault'?1.15:0.55};
  this.cam.dipV+=kind==='vault'?-0.6:-1.0;
  this.emit(kind);
  return mul(up,this.tr.riseSpeed);
 }
 stepTraverse(dt,pos,up,vel){
  const tr=this.tr;tr.t+=dt;const h=dot(pos,up);
  if(tr.phase==='rise'){
   if(h<tr.targetH-0.01&&tr.t<0.8){const vy=Math.min(tr.riseSpeed,(tr.targetH-h)/dt);return add(mul(tr.dir,0.4),mul(up,vy));}
   tr.phase='over';
  }
  const actual=length(tangent(this.entity.character.actualDisplacement,up));
  tr.travel+=tr.phase==='over'&&tr.t2!==undefined?actual:0;tr.t2=(tr.t2||0)+dt;
  if(tr.travel>=tr.overDist||tr.t2>0.45){
   this.mode='air';this.tr=null;this.jumpLock=0;this.speed=Math.max(this.speed,tr.outSpeed*0.95);
   return add(mul(tr.dir,tr.outSpeed),mul(up,-0.5));
  }
  return mul(tr.dir,tr.kind==='vault'?tr.outSpeed:Math.min(tr.outSpeed,4));
 }

 // ------------------------------------------------------------- wall-run
 stepWallrun(dt,I,pos,up,vel,gmag){
  const wr=this.wr;wr.t+=dt;
  const wall=this.wallAt(pos,mul(wr.normal,-1),T.wallrunProbe+0.1);
  let vy=dot(vel,up);
  if(I.jumpPressed){ // wall jump: kick away from the wall, keep running momentum
   this.mode='air';this.wr=null;this.lastWall=wr.normal;this.wallCooldown=0.35;this.jumpLock=0.1;this.emit('walljump');
   this.speed=Math.max(this.speed,wr.speed);
   return add(add(mul(wr.along,wr.speed*0.92),mul(wr.normal,T.wallJumpOut)),mul(up,T.wallJumpUp));
  }
  if(!wall||wr.t>T.wallrunTime||I.my<0.2||vy<-4.5){
   this.mode='air';this.wr=null;this.lastWall=wr.normal;this.wallCooldown=0.3;
   return add(add(mul(wr.along,wr.speed),mul(wr.normal,1.2)),mul(up,vy));
  }
  wr.normal=wall.normal;wr.along=norm(tangent(wr.along,wall.normal));
  wr.speed=Math.max(T.wallrunMin,wr.speed-0.6*dt);
  vy-=gmag*T.wallrunGravity*(vy>0?1:0.6)*dt;
  return add(add(mul(wr.along,wr.speed),mul(wall.normal,-0.8)),mul(up,vy));
 }

 // ------------------------------------------------------------- wall climb
 stepClimb(dt,I,pos,up,vel,gmag,fwd){
  const cl=this.cl;cl.t+=dt;
  const ledge=this.probeLedge(pos,mul(cl.normal,-1),0.25,T.ledgeReach,0.6);
  if(ledge)return this.startTraverse(ledge,mul(cl.normal,-1),'ledge',3,up);
  if(I.jumpPressed&&cl.t>0.08){ // turn-and-kick off the wall
   this.mode='air';this.cl=null;this.jumpLock=0.1;this.wallCooldown=0.3;this.lastWall=cl.normal;
   this.yawTurn=Math.PI;this.emit('walljump');
   return add(mul(cl.normal,4.8),mul(up,4.6));
  }
  const wall=this.wallAt(pos,mul(cl.normal,-1),0.8);
  if(!wall||cl.t>T.climbTime){this.mode='air';this.cl=null;return add(mul(cl.normal,0.3),mul(up,Math.min(dot(vel,up),T.climbSpeed*0.6)));}
  return add(mul(wall.normal,-0.6),mul(up,T.climbSpeed*(1-0.35*cl.t/T.climbTime)));
 }

 // ------------------------------------------------------------- slide / crouch
 startSlide(own,hSpeed,up,supportV){
  this.setCrouch(true);this.mode='slide';
  this.sl={t:0,dir:norm(own),speed:Math.min(T.slideMax,hSpeed+T.slideBoost)};
  this.emit('slide');
  return add(add(supportV,mul(this.sl.dir,this.sl.speed)),mul(up,-0.6));
 }
 stepSlide(dt,I,pos,up,vel,wish,supportV,st){
  const sl=this.sl;sl.t+=dt;
  // Slopes speed you up going down, slow you going up.
  const n=st.supportNormal,slope=dot(tangent(mul(up,-1),n),sl.dir);
  sl.speed=Math.max(0,sl.speed-T.slideFriction*dt+slope*9.81*dt);
  if(length(wish)>0.1){const turn=clamp(dot(wish,norm(tangent(sub(wish,mul(sl.dir,dot(wish,sl.dir))),up))),0,1)*1.2*dt;sl.dir=norm(add(sl.dir,mul(sub(wish,sl.dir),turn)));}
  const blocked=!this.canStand();
  if(this.sinceJumpPress<T.jumpBuffer&&!blocked){this.sinceJumpPress=1;this.setCrouch(false);this.mode='ground';this.speed=sl.speed;this.sl=null;
   return this.jump(mul(sl.dir,this.speed),up,supportV);}
  const done=(sl.t>T.slideMinTime&&(!I.slideHeld||sl.speed<3))||sl.speed<1.8;
  if(done){
   if(!blocked){this.setCrouch(false);this.mode='ground';this.speed=sl.speed;this.sl=null;}
   else sl.speed=Math.max(sl.speed,2.0); // keep crawling until there is headroom
  }
  let h=mul(sl.dir,sl.speed);
  if(dot(n,up)>0.5){const onPlane=tangent(h,n);if(dot(onPlane,up)<0)h=mul(norm(onPlane),sl.speed);}
  return add(add(supportV,h),mul(up,-0.6));
 }

 debugLog(st,pos,vel){
  const every=this.props.logEvery;if(!(every>0)||this.steps%every)return;
  const r=x=>Math.round(x*100)/100;
  console.log(JSON.stringify({t:r(this.time),mode:this.mode,p:[r(pos.x),r(pos.y),r(pos.z)],v:[r(vel.x),r(vel.y),r(vel.z)],sup:st.supported,spd:r(this.speed),cr:this.crouched,ev:this.logEvents.join(',')}));
  this.logEvents.length=0;
 }

 // ------------------------------------------------------------- camera
 presentationUpdate(dt){
  const t=this.entity.presentedTransform,c=this.cam,up=rotate(t.rotation,{x:0,y:1,z:0});
  dt=Math.min(dt,0.1);
  // Wall-climb turn: animate a quick 180 for the camera and heading.
  if(this.yawTurn!==0){const step=Math.sign(this.yawTurn)*Math.min(Math.abs(this.yawTurn),9*dt);this.state.yaw+=step;this.yawTurn-=step;}
  const m=this.entity.character,s=m?m.state:null;
  const hv=s?length(tangent(s.velocity,up)):0;
  c.eye=lerp(c.eye,this.crouched?T.eyeCrouch:T.eyeStand,damp(14,dt));
  // Landing dip spring.
  c.dipV+=(-c.dip*90-c.dipV*14)*dt;c.dip+=c.dipV*dt;
  // Head bob only while running on the ground.
  this.hud=this.hud||ui.get('hud');const paused=!!this.hud?.modal;
  const bobbing=!paused&&this.mode==='ground'&&hv>0.5;
  if(bobbing){const before=Math.floor(c.bob);c.bob+=dt*hv*1.15;if(Math.floor(c.bob)!==before)this.sfx(SFX.step,clamp(0.25+hv*0.04,0.25,0.6));}
  // Wind rushes louder with speed (any mode), silent while paused.
  const windSpeed=s?length(s.velocity):0,windVol=paused?0:clamp((windSpeed-3)/14,0,0.75);
  if(Math.abs(windVol-(c.windVol??-1))>0.02){c.windVol=windVol;world.entity(SFX.wind)?.setAudio({volume:windVol,pitch:clamp(0.85+windSpeed*0.025,0.85,1.5)});}
  const bobAmt=bobbing?Math.min(1,hv/8):0;c.bobAmt=lerp(c.bobAmt||0,bobAmt,damp(8,dt));
  const bobY=Math.abs(Math.sin(c.bob*Math.PI))*0.055*c.bobAmt-0.03*c.bobAmt,bobRoll=Math.sin(c.bob*Math.PI)*0.012*c.bobAmt;
  const targetRoll=this.mode==='wallrun'&&this.wr?-this.wr.side*0.2:this.mode==='slide'?0.06:0;
  c.roll=lerp(c.roll,targetRoll,damp(this.mode==='wallrun'?9:6,dt));
  // Landing roll: one forward somersault of the view.
  let rollPitch=0;if(c.rollAnim>0){c.rollAnim=Math.max(0,c.rollAnim-dt/0.6);const k=1-c.rollAnim;rollPitch=-2*Math.PI*(k*k*(3-2*k));}
  const fovTarget=clamp(74+(hv-4)*1.6,74,88)+(this.mode==='slide'?3:0);
  c.fov=lerp(c.fov,fovTarget,damp(4,dt));
  const yaw=qm(t.rotation,axis({x:0,y:1,z:0},this.state.yaw));
  let rot=qm(yaw,axis({x:1,y:0,z:0},this.state.pitch+rollPitch));
  rot=qm(rot,axis({x:0,y:0,z:1},c.roll+bobRoll));
  const eye=add(t.position,rotate(t.rotation,{x:0,y:c.eye+c.dip*0.25+bobY,z:0}));
  world.setView({position:eye,rotation:rot},c.fov);
 }

 destroy(){input.pointerCapture=false;world.clearView();if(link.runner===this)link.runner=null;}
}
