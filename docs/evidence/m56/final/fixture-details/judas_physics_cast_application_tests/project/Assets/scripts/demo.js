import {ui,input,world,physics,time,console} from 'judas';
export default class {
 constructor(){this.state={shape:false,spawns:0};this.spawned=[];}
 start(){this.hud=ui.get('game_ui');this.hud.modal=false;ui.debugOverlayVisible=false;
  for(const id of ['main','options','pause','image','pause_options'])this.hud.get(id).visible=false;
  this.hud.get('hud').visible=true;
  this.hud.get('hud_help').text='Aim at red / green. Blue filtered; yellow ignored.\nG: ray / sphere (r=0.4) | P: spawn | Esc: pause';
  this.spawn();}
 spawn(){const n=this.state.spawns++;let e=world.spawnPrefab('44444444444444444444444444444402',{
  position:world.viewRay?{x:world.viewRay.origin.x+world.viewRay.direction.x*3,y:world.viewRay.origin.y+world.viewRay.direction.y*3,z:world.viewRay.origin.z+world.viewRay.direction.z*3}:{x:n%3-1,y:1.5,z:-4-n},rotation:{w:1,x:0,y:0,z:0},scale:{x:1,y:1,z:1}});this.spawned.push(e);this.hud.get('hud_title').text=`GEOMETRIC QUERIES | Spawned: ${this.state.spawns}`;}
 update(){if(input.pressed('spawn_prefab'))this.spawn();if(input.pressed('interact'))this.state.shape=!this.state.shape;
  // Demo movement is project JS. No engine fixture/name branch exists.
  for(let i=0;i<this.spawned.length;i++){let e=this.spawned[i];if(e.valid)e.velocity={x:Math.cos(time.elapsed+i)*0.35,y:0,z:0};}
  const ray=world.viewRay;if(!ray)return;
  const filter={excludeLayers:['Excluded'],excludedTags:['IgnoreQuery'],ignored:[world.entity('5')]};
  const hit=this.state.shape?physics.sphereCast(ray.origin,.4,ray.direction,100,filter):physics.raycast(ray.origin,ray.direction,100,filter);
  this.hud.get('counter').text=hit?`${this.state.shape?'Sphere':'Ray'}: entity ${hit.entity.id} at ${hit.distance.toFixed(2)} m\nPoint ${hit.point.x.toFixed(2)}, ${hit.point.y.toFixed(2)}, ${hit.point.z.toFixed(2)}\nNormal ${hit.normal.x.toFixed(2)}, ${hit.normal.y.toFixed(2)}, ${hit.normal.z.toFixed(2)}`:`${this.state.shape?'Sphere':'Ray'}: no hit`;
  for(const [id,point] of [['7',hit?.point],['8',hit?{x:hit.point.x+hit.normal.x*.5,y:hit.point.y+hit.normal.y*.5,z:hit.point.z+hit.normal.z*.5}:null]]){
    const e=world.entity(id),t=e.transform;t.position=point||{x:0,y:-50,z:0};e.transform=t;
  }
 }
 uiUpdate(){if(input.pressed('pause')){this.hud.modal=!this.hud.modal;this.hud.get('pause').visible=this.hud.modal;}}
 onUI(e){if((e.type==='click'&&e.element==='resume')||e.type==='back'){this.hud.modal=false;this.hud.get('pause').visible=false;}if(e.type==='click'&&(e.element==='quit'||e.element==='pause_quit'))ui.quit();}
}
