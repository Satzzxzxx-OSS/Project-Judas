// Spectator: plays an authored clip; optionally collapses into its ragdoll at a time
// (profiling workload for many animated/articulated entities).
import {time} from 'judas';
export const properties = {clip: {type: 'string', default: 'Walk'}, speed: {type: 'number', default: 1},
  ragdollAt: {type: 'number', default: 0}};
export default class {
  constructor({entity, properties}) { this.e = entity; this.p = properties; this.started = false; this.dropped = false; }
  update() {
    const a = this.e.animation;
    if (!this.started && a && a.info.ready) { a.play(this.p.clip); a.speed = this.p.speed; this.started = true; }
    if (this.p.ragdollAt > 0 && !this.dropped && time.elapsed > this.p.ragdollAt && this.e.ragdoll) {
      this.dropped = true;
      this.e.ragdoll.enter();
    }
  }
}
