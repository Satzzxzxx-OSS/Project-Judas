// Minimal repro probe: logs height/velocity of a body held by an authored, enabled slider joint.
import {console, ui} from 'judas';
import {fmt, len} from '../vec.js';
export const properties = {label: {type: 'string', default: 'S'}, quitAfter: {type: 'number', default: 4}};
export default class {
  constructor({entity, properties}) { this.e = entity; this.p = properties; this.t = 0; this.minY = 1e9; this.maxY = -1e9; }
  fixedUpdate(dt) {
    this.t += dt;
    const p = this.e.transform.position, v = this.e.velocity;
    if (this.t > 0.5) { this.minY = Math.min(this.minY, p.y); this.maxY = Math.max(this.maxY, p.y); }
    if (Math.round(this.t * 60) % 12 === 0) console.log(`SL ${this.p.label} t=${this.t.toFixed(2)} p=${fmt(p, 3)} v=${fmt(v, 3)}`);
    if (Math.round(this.t * 60) === Math.round(this.p.quitAfter * 60) - 2)
      console.log(`SLSUM ${this.p.label} yRange=[${this.minY.toFixed(3)}, ${this.maxY.toFixed(3)}] final=${fmt(p, 3)} |v|=${len(v).toFixed(3)}`);
    if (this.t > this.p.quitAfter) ui.quit();
  }
}
