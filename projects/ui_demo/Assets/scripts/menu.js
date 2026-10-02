import {ui,input,world} from 'judas';
export const properties={title:{type:'string',default:'PROJECT-OWNED HUD'}};
export default class Menu {
 constructor({entity,properties}){this.entity=entity;this.properties=properties;this.returnPanel='main';this.started=false;}
 start(){ui.debugOverlayVisible=false;this.doc=ui.get('game_ui');this.doc.get('hud_title').text=this.properties.title;}
 panel(name){for(const id of ['main','pause','options'])this.doc.get(id).visible=id===name;this.doc.modal=!!name;this.doc.get('hud').visible=this.started&&this.doc.get('toggle').value>0.5;}
 uiUpdate(){if(input.pressed('pause')&&!this.doc.modal)this.panel('pause');}
 update(){this.doc.get('counter').text=`Spawned props: ${world.queryTags(['spawned']).length}`;}
 onUI(e){if(e.document!=='game_ui')return;
  if(e.type==='back'){if(this.doc.get('options').visible)this.panel(this.returnPanel);else if(this.started)this.panel('');return;}
  if(e.type==='change'){if(e.element==='toggle')this.doc.get('hud').visible=this.started&&e.value>.5;if(e.element==='strength'){this.doc.get('level').text=`Demo effect strength: ${Math.round(e.value*100)}%`;for(const prop of world.queryTags(['spawned']))prop.setParticles({rate:8*e.value});}return;}
  if(e.type!=='click')return;
  if(e.element==='start'){this.started=true;this.panel('');}
  if(e.element==='resume')this.panel('');
  if(e.element==='main_options'||e.element==='pause_options'){this.returnPanel=e.element==='main_options'?'main':'pause';this.panel('options');}
  if(e.element==='back')this.panel(this.returnPanel);
  if(e.element==='quit'||e.element==='pause_quit')ui.quit();
 }
}
