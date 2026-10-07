import {saves,console,ui} from 'judas';
import {game} from '../skate/scripts/game.js';
export default class {
 constructor(){this.state={};this.request=0;this.issued=false;}
 start(){this.request=saves.refresh();console.log('COLD refresh '+this.request);}
 restore(){this.restored=true;console.log('COLD restored score='+game.score);if(game.score!==314)throw Error('consumer score not restored');ui.quit();}
 uiUpdate(){if(this.restored||!this.request)return;const s=saves.status(this.request);if(!s)return;
 if(s.state==='failed'||s.state==='cancelled')throw Error('cold operation '+s.state+' '+s.error);
 if(s.state==='completed'){
  if(!this.issued){this.issued=true;const exists=saves.exists('consumer-cold');console.log('COLD exists='+exists);if(exists)this.request=saves.load('consumer-cold');else{game.score=314;this.request=saves.save('consumer-cold');}}
  else{console.log('COLD committed score='+game.score);ui.quit();}
 }}
}
