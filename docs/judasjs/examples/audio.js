export default class {
  /** @param {import('judas').ScriptContext} context */
  constructor({entity}) {this.entity=entity;this.state={requested:false,stopped:false};this.frames=0;}
  start() {this.state.requested=this.entity.playAudio();}
  update() {
    this.frames++;
    if(this.frames===2)this.entity.pauseAudio();
    if(this.frames===3)this.entity.resumeAudio();
    if(this.frames===4)this.state.stopped=this.entity.stopAudio();
  }
}
