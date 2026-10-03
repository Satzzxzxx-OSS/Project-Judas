import {ui,input,scenes,session} from 'judas';
import {round,targets,navigators} from './round.js';
export default class {
 constructor(player){this.player=player;this.doc=ui.get('range_ui');}
 start(){ui.debugOverlayVisible=false;this.doc.modal=false;this.doc.get('pause').visible=false;input.pointerCapture=true;}
 menu(open){this.doc.modal=open;this.doc.get('pause').visible=open;input.pointerCapture=!open;}
 update(){if(input.pressed('pause'))this.menu(!this.doc.modal);
  this.doc.get('score').text=`SCORE ${round.score}     HITS ${round.hits} / ${round.shots} SHOTS`;
  let ready=0;for(const t of targets.values())if(t.state.ready)ready++;
  this.doc.get('progress').text=`${round.unique} / ${targets.size} UNIQUE  |  ${ready} READY  |  ${this.player.camera.third?'THIRD':'FIRST'} PERSON | ${[...navigators.values()].filter(n=>!n.state.defeated).length} CHASERS  |  BEST ${session.get('rangeBest')||0}`;
  this.doc.get('message').text=round.message;
  this.doc.get('hitmark').visible=round.flash>0;
  this.player.state.paused=this.doc.modal;
 }
 event(e){if(e.document!=='range_ui')return;
  if(e.type==='back'||e.type==='click'&&e.element==='resume')this.menu(false);
  if(e.type==='click'&&e.element==='restart')scenes.reload();
  if(e.type==='click'&&e.element==='quit')ui.quit();
 }
}
