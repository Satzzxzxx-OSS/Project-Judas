import {ui,input,world} from 'judas';
import {totals} from './events.js';
export default class {
 start(){this.hud=ui.get('game_ui');this.hud.modal=false;ui.debugOverlayVisible=false;
  this.hud.get('main').visible=false;this.hud.get('hud').visible=true;this.hud.get('options').visible=false;this.hud.get('pause').visible=false;
  this.hud.get('hud_help').text='P: spawn green prefab patrol | R: reset | blue is excluded from sensor';this.spawn();}
 spawn(){world.spawnPrefab('42424242424242424242424242424203',{position:{x:-4,y:1,z:0},rotation:{w:1,x:0,y:0,z:0},scale:{x:1,y:1,z:1}})}
 update(){if(input.pressed('spawn_prefab'))this.spawn();}
 uiUpdate(){if(!this.hud)return;this.hud.get('counter').text=`Collision ${totals.collisionEnter}/${totals.collisionStay}/${totals.collisionExit} | Trigger ${totals.triggerEnter}/${totals.triggerStay}/${totals.triggerExit}\n${totals.last}`;
 if(input.pressed('pause')){this.hud.modal=!this.hud.modal;this.hud.get('pause').visible=this.hud.modal;}}
 onUI(e){if(e.element==='resume'){this.hud.modal=false;this.hud.get('pause').visible=false;}if(e.element==='quit')ui.quit();if(e.type==='back'){this.hud.modal=false;this.hud.get('pause').visible=false;}}
}
