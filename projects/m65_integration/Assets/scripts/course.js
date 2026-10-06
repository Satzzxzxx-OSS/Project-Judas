// Rooftop Run - course rules: timer, checkpoints, falls, finish, HUD and pause menu.
// M65 integration copy: ordinary sensors report overlap; this JS retains checkpoint rules.
import {input,ui,scenes,session,console} from 'judas';
import {length,tangent} from './math.js';
import {link} from './runner.js';
import {course} from './course_data.js';

const inside=(p,b)=>p.x>=b.min[0]&&p.x<=b.max[0]&&p.y>=b.min[1]&&p.y<=b.max[1]&&p.z>=b.min[2]&&p.z<=b.max[2];
const clock=t=>{const m=Math.floor(t/60),s=t-m*60;return `${m}:${s<10?'0':''}${s.toFixed(2)}`;};

export default class Course {
 constructor({entity}){this.entity=entity;
  this.state={checkpoint:0,elapsed:0,running:false,finished:false,falls:0};
  this.banner={text:'',t:0};this.lastHud='';this.deathTimer=0;this.splits=[];}

 start(){
  link.course=this;this.doc=ui.get('hud');ui.debugOverlayVisible=false;
  if(this.doc){this.doc.modal=false;this.doc.get('menu').visible=false;}
  input.pointerCapture=!link.runner?.auto; // autopilot test runs never grab the mouse
  const p=this.entity.transform.position;this.origin=p;
  this.show('ROOFTOP RUN',2.5);
 }

 show(text,seconds=2){this.banner={text,t:seconds};}

 respawn(reason){
  const cp=course.checkpoints[this.state.checkpoint];
  link.runner?.respawn({x:cp.spawn[0],y:cp.spawn[1],z:cp.spawn[2]},cp.yaw);
  if(reason){this.state.falls++;this.show(reason,1.6);}
 }

 fixedUpdate(dt){
  const r=link.runner;if(!r)return;
  const p=this.entity.transform.position;
  // Runner notifications.
  for(const e of r.events){
   if(e.type==='death'){this.deathTimer=0.5;this.show('TOO HIGH - HARD FALL',1.4);}
   if(e.type==='hardlanding')this.show('HARD LANDING - roll with CTRL',1.2);
   if(e.type==='roll')this.show('ROLL',0.6);
  }
  r.events.length=0;
  if(this.deathTimer>0){this.deathTimer-=dt;if(this.deathTimer<=0)this.respawn('');}
  if(input.pressed('checkpoint')&&!this.state.finished)this.respawn('');
  if(input.pressed('restart'))scenes.reload();
  // Timer starts on first real movement away from the start.
  if(!this.state.running&&!this.state.finished&&this.origin&&length(tangent({x:p.x-this.origin.x,y:0,z:p.z-this.origin.z},{x:0,y:1,z:0}))>0.75)this.state.running=true;
  if(this.state.running)this.state.elapsed+=dt;
  // Falling off the roofs.
  if(p.y<course.killHeight){this.respawn('FELL - back to checkpoint');return;}
 }

 checkpoint(index){if(index<=this.state.checkpoint||index>=course.checkpoints.length)return;
  this.state.checkpoint=index;this.splits.push(this.state.elapsed);
  this.show(`CHECKPOINT - ${course.checkpoints[index].name}  ${clock(this.state.elapsed)}`,1.8);
  console.log(`checkpoint ${index} ${this.state.elapsed.toFixed(2)}`);
 }
 finish(){if(this.state.finished)return;this.state.finished=true;this.state.running=false;
  const best=session.get('rooftopBest'),t=this.state.elapsed,record=typeof best!=='number'||t<best;
  if(record)session.set('rooftopBest',t);this.openMenu('FINISHED',`Time ${clock(t)}${record?'  NEW BEST':''}\nBest ${clock(record?t:best)}   Falls ${this.state.falls}`);

 }

 openMenu(title,body){
  if(!this.doc)return;
  this.doc.get('menu_title').text=title;this.doc.get('menu_body').text=body;
  this.doc.get('resume').visible=!this.state.finished;
  this.doc.get('menu').visible=true;this.doc.modal=true;input.pointerCapture=false;
 }
 closeMenu(){if(!this.doc||this.state.finished)return;this.doc.get('menu').visible=false;this.doc.modal=false;input.pointerCapture=true;}

 uiUpdate(dt){if(input.pressed('lab')){scenes.load('Scenes/integration.judas');return;}
  if(!this.doc)return;
  if(input.pressed('pause')){
   if(this.doc.modal)this.closeMenu();
   else this.openMenu('PAUSED',`Run time ${clock(this.state.elapsed)}\nCheckpoint: ${course.checkpoints[this.state.checkpoint].name}`);
  }
  const r=link.runner,p=this.entity.transform.position;
  if(!this.doc.modal)this.banner.t=Math.max(0,this.banner.t-dt);
  const ch=this.entity.character,speed=ch?length(tangent(ch.velocity,{x:0,y:1,z:0})):0;
  const hint=course.hints.find(h=>inside(p,h));
  const best=session.get('rooftopBest');
  const mode=r?({wallrun:'WALL-RUN',climb:'WALL CLIMB',traverse:r.tr?.kind==='vault'?'VAULT':'CLIMB UP',slide:'SLIDE',dead:''}[r.mode]||''):'';
  const hud=[clock(this.state.elapsed),`${speed.toFixed(1)} m/s`,this.banner.t>0?this.banner.text:'',hint?hint.text:'',mode,
   `CP ${this.state.checkpoint+1}/${course.checkpoints.length}${typeof best==='number'?'   BEST '+clock(best):''}`].join('|');
  if(hud!==this.lastHud){this.lastHud=hud;const [a,b,c,d,e,f]=hud.split('|');
   this.doc.get('timer').text=a;this.doc.get('speed').text=b;this.doc.get('banner').text=c;this.doc.get('hint').text=d;
   this.doc.get('hint').visible=!!d;this.doc.get('state').text=e;this.doc.get('split').text=f;}
 }

 onUI(e){
  if(e.document!=='hud')return;
  if(e.type==='back'||(e.type==='click'&&e.element==='resume'))this.closeMenu();
  if(e.type==='click'&&e.element==='restart')scenes.reload();
  if(e.type==='click'&&e.element==='quit')ui.quit();
 }
}
