import {ui,input,scenes,session,localization,audio} from 'judas';
import {round,targets,navigators} from './round.js';
// Only presentation changes with language. Score/physics/weapon rules are untouched.
function message(){const s=round.message;const points=/^\+(\d+) - physical hit!$/.exec(s);if(points)return localization.format('range.hit',{points:Number(points[1])});
 const keys={'Hit all 12 plates. Every ready plate can score again.':'start','RANGE CLEARED! Keep practising or restart.':'clear','Impact - plate cooling down or non-scoring prop.':'impact','Miss - aim at a coloured plate.':'miss','Corridor blocker installed; agents repath.':'blocked','Corridor cleared; tiles update without rebaking.':'unblocked','Navigator defeated +250':'defeat','Navigator hit - one more shot':'enemy'};
 return keys[s]?localization.format('range.'+keys[s]):s;
}
export default class {
 constructor(player){this.player=player;this.doc=ui.get('range_ui');this.last='';}
 start(){ui.debugOverlayVisible=false;this.doc.modal=false;this.doc.get('pause').visible=false;input.pointerCapture=true;}
 menu(open){audio.setGroup("effects",{paused:open});this.doc.modal=open;this.doc.get('pause').visible=open;input.pointerCapture=!open;}
 nextLanguage(){const languages=localization.available;localization.setLocale(languages[(languages.indexOf(localization.locale)+1)%languages.length]);}
 update(){if(input.pressed('pause'))this.menu(!this.doc.modal);if(input.pressed('language_next'))this.nextLanguage();
  let ready=0;for(const t of targets.values())if(t.state.ready)ready++;
  const chasers=[...navigators.values()].filter(n=>!n.state.defeated).length,best=session.get('rangeBest')||0;
  const signature=[localization.revision,round.score,round.hits,round.shots,round.unique,targets.size,ready,this.player.camera.third,chasers,best,round.message].join('|');
  if(signature!==this.last){this.last=signature;this.doc.get('canvas').direction=localization.direction;
   this.doc.get('score').text=localization.format('range.score',{score:round.score,hits:round.hits,shots:round.shots});
   this.doc.get('progress').text=localization.format('range.progress',{count:ready,unique:round.unique,total:targets.size,view:this.player.camera.third?'third':'first',chasers,best});
   this.doc.get('message').text=message();this.doc.get('language').text=localization.format('range.language',{language:localization.locale});
  }
  this.doc.get('hitmark').visible=round.flash>0;this.player.state.paused=this.doc.modal;
 }
 event(e){if(e.document!=='range_ui')return;
  if(e.type==='back'||e.type==='click'&&e.element==='resume')this.menu(false);
  if(e.type==='click'&&e.element==='restart')scenes.reload();
  if(e.type==='click'&&e.element==='language')this.nextLanguage();
  if(e.type==='click'&&e.element==='quit')ui.quit();
 }
}
