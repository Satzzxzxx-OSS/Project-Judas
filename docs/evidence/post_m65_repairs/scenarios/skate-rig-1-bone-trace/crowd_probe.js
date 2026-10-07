// Spectator: plays an authored clip; optionally collapses into its ragdoll at a time
// (profiling workload for many animated/articulated entities).
import {time} from 'judas';
export const properties = {clip: {type: 'string', default: 'Walk'}, speed: {type: 'number', default: 1},
  ragdollAt: {type: 'number', default: 0}};
export default class {
  constructor({entity, properties}) { this.e = entity; this.p = properties; this.state = {}; this.started = false; this.dropped = false; }
  update() {
    if(this.dropped&&Math.floor(time.elapsed)!==this.lastProbe){this.lastProbe=Math.floor(time.elapsed);this.state.bones=["mixamorig:Hips", "mixamorig:Spine1", "mixamorig:Head", "mixamorig:LeftArm", "mixamorig:LeftForeArm", "mixamorig:RightArm", "mixamorig:RightForeArm", "mixamorig:LeftUpLeg", "mixamorig:LeftLeg", "mixamorig:LeftFoot", "mixamorig:RightUpLeg", "mixamorig:RightLeg", "mixamorig:RightFoot"].map(key=>{const b=this.e.ragdoll.body(key);return b?.valid?{key,sleeping:b.sleeping,v:b.velocity,w:b.angularVelocity}:null;});}
    if(this.e.id==='1000200'&&this.dropped){const b=this.e.ragdoll.body('mixamorig:Hips');if(time.elapsed>27.9&&!this.kicked){this.state.beforeSleep=b.sleeping;this.kicked=true;b.applyImpulse({x:20,y:3,z:0});this.state.afterSleep=b.sleeping;}if(this.kicked)this.state.speed=Math.hypot(b.velocity.x,b.velocity.y,b.velocity.z);}
    const a = this.e.animation;
    if (!this.started && a && a.info.ready) { a.play(this.p.clip); a.speed = this.p.speed; this.started = true; }
    if (this.p.ragdollAt > 0 && !this.dropped && time.elapsed > this.p.ragdollAt && this.e.ragdoll) {
      this.dropped = true;
      this.e.ragdoll.enter();
    }
  }
}
