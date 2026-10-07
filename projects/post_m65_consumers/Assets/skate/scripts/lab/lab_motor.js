// LAB variant M: CharacterMotor capsule; script owns velocity (carve/rolling
// resistance), motor owns gravity + sweep/slide; ground found by our own raycast
// because the motor's `supported` flag is false whenever velocity has an uphill
// (departing) component.
import {physics, console, time} from 'judas';
import {V, add, sub, mul, dot, cross, len, norm, reject, UP, rotate, clamp, fmt} from '../vec.js';

export const properties = {
  label: {type: 'string', default: 'M'},
  entrySpeed: {type: 'number', default: 7},
  holdUntil: {type: 'number', default: 1.0},
  log: {type: 'boolean', default: true}
};

export default class {
  constructor({entity, properties}) {
    this.e = entity; this.p = properties; this.t = 0;
    this.launches = 0; this.lastGrounded = true; this.normalJumps = 0; this.prevN = null; this.jitter = 0;
    this.supportedCount = 0; this.groundedCount = 0;
  }
  start() {
    this.heading = rotate(this.e.transform.rotation, V(0, 0, -1));
    this.motor = this.e.character;
    this.filter = {excludeLayers: ['Board', 'Ragdoll']};
  }
  fixedUpdate(dt) {
    this.t += dt;
    const c = this.motor, st = c.state, pos = this.e.transform.position;
    const h = physics.raycast(add(pos, V(0, 0.2, 0)), V(0, -1, 0), 0.2 + 0.5 + 0.35, this.filter);
    // Capsule bottom is 0.5+0.25 below centre (halfHeight .5, radius .25) when offset 0.
    const grounded = !!h && h.distance < 0.2 + 0.75 + 0.12;
    const n = grounded ? h.normal : UP;
    let v = c.velocity;
    if (grounded) {
      this.groundedCount++;
      const fs = norm(reject(this.heading, n)), rs = norm(cross(fs, n));
      const vn = dot(v, n), vf = dot(v, fs), vl = dot(v, rs);
      const speed = Math.hypot(vf, vl);
      let nvf = Math.sign(vf || 1) * speed;
      nvf -= Math.sign(nvf) * Math.min(Math.abs(nvf), (0.12 + 0.004 * speed * speed) * dt);
      if (this.t < this.p.holdUntil) nvf = this.p.entrySpeed;
      v = add(add(mul(fs, nvf), mul(n, Math.min(vn, 0))), mul(rs, vl * 0.05));
      c.velocity = v;
      if (this.prevN) {
        const da = Math.acos(clamp(dot(this.prevN, n), -1, 1));
        if (da > 0.12) this.normalJumps++;
        this.jitter += da;
      }
      this.prevN = n;
    } else this.prevN = null;
    if (st.supported) this.supportedCount++;
    if (!grounded && this.lastGrounded) this.launches++;
    this.lastGrounded = grounded;
    const energy = 0.5 * dot(v, v) + 9.81 * pos.y;
    if (this.p.log && Math.round(this.t * 60) % 6 === 0)
      console.log(`TEL ${this.p.label} t=${this.t.toFixed(2)} p=${fmt(pos)} v=${len(v).toFixed(2)} vy=${v.y.toFixed(2)} g=${grounded ? 1 : 0} sup=${st.supported ? 1 : 0} n=${fmt(n)} E=${energy.toFixed(2)} col=${st.collided ? 1 : 0}`);
    if (Math.round(this.t * 60) === 60 * 9)
      console.log(`SUMMARY ${this.p.label} launches=${this.launches} normalJumps=${this.normalJumps} jitter=${this.jitter.toFixed(2)} supportedSteps=${this.supportedCount} groundedSteps=${this.groundedCount} finalE=${energy.toFixed(2)} pos=${fmt(pos)}`);
  }
}
