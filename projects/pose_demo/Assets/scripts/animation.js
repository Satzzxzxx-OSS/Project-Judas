import {ui,input,world} from 'judas';
export default class {
 constructor(){this.state={spawns:0};}
 start(){this.hud=ui.get('game_ui');this.hud.modal=false;ui.debugOverlayVisible=false;
  for(const id of ['main','options','pause','image','pause_options'])this.hud.get(id).visible=false;
  this.hud.get('hud').visible=true;
  this.hud.get('hud_title').text='POSE COMPOSITION — clips + masks + additive';
  this.hud.get('hud_help').text='Orange: controlled bar | Blue: independently looping, rotated Stretch\nG: crossfade | J: tip layer | K: additive elbow | C: speed | P: spawn | Esc: pause';
  this.a=world.entity('10').animation;this.b=world.entity('11').animation;}
 spawn(){const n=this.state.spawns++;return world.spawnPrefab('46464646464646464646464646464603',{position:{x:-3+(n%5)*1.5,y:0,z:-3},rotation:{w:1,x:0,y:0,z:0},scale:{x:1,y:1,z:1}});}
 update(){if(input.pressed('spawn_prefab'))this.spawn();if(this.a.info.ready){
   if(input.pressed('interact')){this.a.crossFade(this.a.info.clip==='Wave'?'Stretch':'Wave',.8);}
   if(input.pressed('pulse')){this.tip=!this.tip;this.a.layer('tip',{clip:'Stretch',mask:['Root/Elbow/Tip'],enabled:this.tip,weight:1});}
   if(input.pressed('collect_key')){this.additive=!this.additive;this.a.layer('elbowOffset',{clip:'Wave',referenceClip:'Wave',referenceTime:0,additive:true,mask:['Root/Elbow'],weight:.4,enabled:this.additive});}
   if(input.pressed('igniter'))this.a.speed=this.a.speed===1?2:1;
   this.hud.get('counter').text=`Orange: ${this.a.info.clip} | ${this.a.time.toFixed(2)}s | ${this.a.speed}x | fade ${this.a.info.transitionFraction.toFixed(2)} | mask ${!!this.tip} | additive ${!!this.additive} | ${this.a.playing?'playing':'paused/stopped'}\nBlue: ${this.b.info.clip} ${this.b.time.toFixed(2)}s | spawned: ${this.state.spawns}`;
  }else this.hud.get('counter').text='Loading skinned asset through normal resources…';}
 uiUpdate(){if(input.pressed('pause')){this.hud.modal=!this.hud.modal;this.hud.get('pause').visible=this.hud.modal;}}
 onUI(e){if((e.type==='click'&&e.element==='resume')||e.type==='back'){this.hud.modal=false;this.hud.get('pause').visible=false;}if(e.type==='click'&&(e.element==='quit'||e.element==='pause_quit'))ui.quit();}
}
