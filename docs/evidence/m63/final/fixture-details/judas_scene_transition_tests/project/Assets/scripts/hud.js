import {ui,input,world,scenes,session} from 'judas';
export default class {
 start(){this.hud=ui.get('game_ui');this.hud.modal=false;ui.debugOverlayVisible=false;
  for(const id of ['main','options','pause'])this.hud.get(id).visible=false;
  this.hud.get('hud').visible=true;
  this.hud.get('hud_title').text=scenes.current==='Scenes/a.judas'?'SCENE A / GREEN GATE':'SCENE B / PURPLE GATE';
  this.hud.get('hud_help').text='E: send courier / return | P: spawn | R: real reload\nEscape: scene menu';
  this.hud.get('resume').text='Return to Scene A';this.hud.get('pause_options').text='Reload this scene';}
 spawn(){return world.spawnPrefab('42424242424242424242424242424203',{position:{x:-4,y:1,z:0},rotation:{w:1,x:0,y:0,z:0},scale:{x:1,y:1,z:1}})}
 update(){if(input.pressed('spawn_prefab'))this.spawn();
  if(input.pressed('interact')){if(scenes.current==='Scenes/a.judas')this.spawn();else scenes.load('Scenes/a.judas');}}
 uiUpdate(){this.hud.get('counter').text=`${scenes.current}
Session key: ${session.get('hasKey')||false} | Visits: ${session.get('visits')||0}`;
  if(input.pressed('pause')){this.hud.modal=!this.hud.modal;this.hud.get('pause').visible=this.hud.modal;}}
 onUI(e){if(e.type==='back'){this.hud.modal=false;this.hud.get('pause').visible=false;}
  if(e.type!=='click')return;
  if(e.element==='resume')scenes.load('Scenes/a.judas');
  if(e.element==='pause_options')scenes.reload();if(e.element==='pause_quit')ui.quit();}
}
