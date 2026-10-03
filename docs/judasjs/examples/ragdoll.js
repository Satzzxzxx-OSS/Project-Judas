export default class {
  /** @param {import('judas').ScriptContext} context */
  constructor({entity}) {this.entity=entity;this.state={entered:false,left:false};this.steps=0;}
  fixedUpdate() {
    const a=this.entity.animation,r=this.entity.ragdoll;
    if(!a||!a.info.ready||!r)return;
    if(!this.state.entered){a.seek(.4);this.state.entered=r.enter();const tip=r.body('Root/Elbow/Tip');if(tip&&tip.valid)tip.applyImpulse({x:1,y:0,z:0});}
    if(++this.steps===8)this.state.left=r.leave(.4);
  }
}
