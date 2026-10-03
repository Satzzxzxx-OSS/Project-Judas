export default class {
  /** @param {import('judas').ScriptContext} context */
  constructor({entity}) {this.entity=entity;this.state={ready:false,fade:false,layers:false};}
  update() {
    const a=this.entity.animation;if(!a||!a.info.ready||this.state.ready)return;
    a.loop=true;a.speed=.5;a.play('Wave');a.seek(.2);a.pause();a.resume();
    this.state.fade=a.crossFade('Stretch',.3);
    a.layer('tip',{clip:'Wave',weight:.5,mask:['Root/Elbow/Tip']});
    a.layer('add',{clip:'Wave',weight:.2,additive:true,referenceClip:'Wave',referenceTime:0,mask:['Root/Elbow']});
    this.state.layers=a.layers.length===2;a.removeLayer('tip');this.state.ready=true;
  }
}
