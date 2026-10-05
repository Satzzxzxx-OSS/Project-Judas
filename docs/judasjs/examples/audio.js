import {audio} from 'judas';
export default class {
  /** @param {import('judas').ScriptContext} context */
  constructor({entity}) {this.entity=entity;this.state={requested:false,stopped:false,settings:false,seek:false,oneShot:false,group:false,pausedDrop:false};this.frames=0;}
  start() {this.state.group=audio.group("master")!==null&&audio.setGroup("master",{gain:.8},.1)===true&&audio.diagnostics!==null;this.state.settings=this.entity.setAudio({volume:.15,loop:true,doppler:0,send:0});this.state.requested=this.entity.playAudio();}
  update() {
    this.frames++;
    if(this.frames===2){this.entity.pauseAudio();this.state.seek=this.entity.seekAudio(.1);this.state.oneShot=this.entity.playAudioOneShot();}
    if(this.frames===3){this.entity.resumeAudio();audio.setGroup("master",{paused:true});this.state.pausedDrop=this.entity.playAudioOneShot()===false;audio.setGroup("master",{paused:false});}
    if(this.frames===4)this.state.stopped=this.entity.stopAudio();
  }
}
