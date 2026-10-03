import {ui,input,scenes} from 'judas';
export default class {
 start(){this.hud=ui.get('game_ui');this.hud.modal=false;ui.debugOverlayVisible=false;
  for(const id of ['main','options','pause','image'])this.hud.get(id).visible=false;
  this.hud.get('hud').visible=true;
  this.hud.get('hud_title').text=scenes.current==='Scenes/pool.judas'?'PRODUCTION FLUID / FLAT POOL':'PRODUCTION FLUID / RADIAL PLANET';
  this.hud.get('hud_help').text='WASD + mouse: move/look | Space: swim up / jump\nG: pick up/drop | H: throw | Aim down to dip the open bucket\nF1: flat pool | F2: planet | R: reset | Escape: pause';
  this.hud.get('counter').text='Orange block floats; purple block sinks.\nClear-sided bucket carries actual particles: dip, lift, tilt to pour.\nUse the steps at the near side to leave the basin.';
  this.hud.get('pause_options').text='Switch pool / planet';
 }
 uiUpdate(){if(input.pressed('fluid_pool'))scenes.load('Scenes/pool.judas');if(input.pressed('fluid_planet'))scenes.load('Scenes/planet.judas');
  if(input.pressed('pause')){this.hud.modal=!this.hud.modal;this.hud.get('pause').visible=this.hud.modal;}}
 onUI(e){if(e.type==='back'||(e.type==='click'&&e.element==='resume')){this.hud.modal=false;this.hud.get('pause').visible=false;}
  if(e.type==='click'&&e.element==='pause_options')scenes.load(scenes.current==='Scenes/pool.judas'?'Scenes/planet.judas':'Scenes/pool.judas');
  if(e.type==='click'&&e.element==='pause_quit')ui.quit();}
}
