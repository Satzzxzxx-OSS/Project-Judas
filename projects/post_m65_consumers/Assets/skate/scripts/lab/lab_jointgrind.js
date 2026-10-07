// LAB: constraint-based grinding. A slider joint (board <-> world rail line) is authored
// disabled; JS enables it when the board is over the rail and disables it at the end.
import {physics, world, console, ui} from 'judas';
import {V, add, mul, len, rotate, UP, fmt, angleBetween} from '../vec.js';
export const properties = {joint: {type: 'string', default: '50'}, speed: {type: 'number', default: 6},
  offset: {type: 'number', default: 0.0}, yawDeg: {type: 'number', default: 0}, label: {type: 'string', default: 'J'}};
export default class {
  constructor({entity, properties}) { this.e = entity; this.p = properties; this.t = 0; this.state = 'approach'; }
  start() { this.e.velocity = V(0, 0, -this.p.speed); this.j = physics.joint(world.entity(this.p.joint)); }
  fixedUpdate(dt) {
    this.t += dt;
    const tr = this.e.transform, p = tr.position, v = this.e.velocity;
    const fwd = rotate(tr.rotation, V(0, 0, -1));
    if (this.state === 'approach' && p.z < 1.5) { this.e.velocity = add(v, V(0, 4.0, 0)); this.state = 'air'; }
    if (this.state === 'air' && p.z < -0.3 && v.y < 0.5) {
      this.j.setEnabled(true); this.state = 'grind'; this.enterP = p; this.enterV = len(v);
      console.log(`JG ${this.p.label} enable at ${fmt(p)} v=${fmt(v)} yawErr=${(angleBetween(fwd, V(0,0,-1))*57.3).toFixed(1)}deg`);
    }
    if (this.state === 'grind') {
      if (Math.round(this.t * 60) % 6 === 0) console.log(`JG ${this.p.label} t=${this.t.toFixed(2)} p=${fmt(p)} v=${fmt(v)} |v|=${len(v).toFixed(2)} yawErr=${(angleBetween(fwd, V(0,0,-1))*57.3).toFixed(1)} coord=${this.j.state.coordinate.toFixed(3)} active=${this.j.state.active}`);
      if (p.z < -7.5) { this.j.setEnabled(false); this.state = 'done'; console.log(`JG ${this.p.label} release at ${fmt(p)} v=${fmt(v)}`); }
    }
    if (this.t > 4) ui.quit();
  }
}
