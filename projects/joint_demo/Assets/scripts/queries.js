import {ui,input,world,physics,console} from 'judas';
export default class {
 constructor(){this.state={motor:true,spawns:0};}
 start(){this.hud=ui.get('game_ui');this.hud.modal=false;ui.debugOverlayVisible=false;
  for(const id of ['main','options','pause','image','pause_options'])this.hud.get(id).visible=false;
  this.hud.get('hud').visible=true;
  this.hud.get('hud_title').text='RIGID JOINTS — Fixed / Hinge / Door / Ball / Slider';
  this.hud.get('hud_help').text='Left to right: fixed pair, free hinge, motor door, ball, slider.\nRear: oblique hinge. G: reverse motors | P: spawn fixed prefab pair | Esc: pause';
  world.entity('20').angularVelocity={x:0,y:0,z:1.5};world.entity('40').velocity={x:1.5,y:0,z:1};
  this.door=physics.joint(world.entity('31'));this.slider=physics.joint(world.entity('51'));this.spawn();}
 spawn(){const n=this.state.spawns++;world.spawnPrefab('44444444444444444444444444444402',{position:{x:4+(n%3)*1.5,y:4,z:-4},rotation:{w:1,x:0,y:0,z:0},scale:{x:1,y:1,z:1}});}
 update(){if(input.pressed('spawn_prefab'))this.spawn();if(input.pressed('interact')){this.state.motor=!this.state.motor;this.door.setMotor(this.state.motor?-.8:.8,15);this.slider.setMotor(this.state.motor?-.8:.8,15);}
  this.hud.get('counter').text=`Door angle: ${this.door.state.coordinate.toFixed(2)} rad | Slider: ${this.slider.state.coordinate.toFixed(2)} m\nPrefab assemblies: ${this.state.spawns}`;}
 uiUpdate(){if(input.pressed('pause')){this.hud.modal=!this.hud.modal;this.hud.get('pause').visible=this.hud.modal;}}
 onUI(e){if((e.type==='click'&&e.element==='resume')||e.type==='back'){this.hud.modal=false;this.hud.get('pause').visible=false;}if(e.type==='click'&&(e.element==='quit'||e.element==='pause_quit'))ui.quit();}
}
