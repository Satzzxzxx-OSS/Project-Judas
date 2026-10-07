// LAB isolation: frictionless dynamic box + 4 spring impulses only (no velocity
// rewrite, no angular override). Variant 'damping' and 'align' flags.
// Measures total mechanical energy incl. spring potential, every 0.1 s.
import {physics, console} from 'judas';
import {V, add, sub, mul, dot, cross, len, UP, rotate, qdeltaVec, qmul, qfromto, fmt} from '../vec.js';
export const properties = {
  label: {type: 'string', default: 'S'}, entrySpeed: {type: 'number', default: 6.5},
  damping: {type: 'number', default: 0}, align: {type: 'boolean', default: false},
  pointImpulse: {type: 'boolean', default: true}, worldUp: {type: 'boolean', default: false}
};
const WHEELS = [V(0.09, -0.04, -0.26), V(-0.09, -0.04, -0.26), V(0.09, -0.04, 0.26), V(-0.09, -0.04, 0.26)];
const K = 3700, REST = 0.26;
export default class {
  constructor({entity, properties}) { this.e = entity; this.p = properties; this.t = 0; this.Emax = -1e9; this.E0 = null; }
  start() {
    this.filter = {ignored: [this.e], excludeLayers: ['Board']};
    this.e.velocity = mul(rotate(this.e.transform.rotation, V(0, 0, -1)), this.p.entrySpeed);
  }
  fixedUpdate(dt) {
    this.t += dt;
    const tr = this.e.transform, q = tr.rotation, pos = tr.position, up = this.p.worldUp ? UP : rotate(q, UP);
    const v = this.e.velocity, w = this.e.angularVelocity, m = this.e.mass;
    let springE = 0;
    for (const wl of WHEELS) {
      const wp = add(pos, rotate(q, wl));
      const h = physics.raycast(add(wp, mul(up, 0.2)), mul(up, -1), REST + 0.2, this.filter);
      if (!h) continue;
      const d = h.distance - 0.2;
      if (d >= REST) continue;
      springE += 0.5 * K * (REST - d) * (REST - d);
      const pv = add(v, cross(w, sub(wp, pos)));
      const f = Math.max(0, K * (REST - d) - this.p.damping * dot(pv, up));
      if (this.p.pointImpulse) this.e.applyImpulseAtPoint(mul(up, f * dt), wp);
      else this.e.applyImpulse(mul(up, f * dt));
    }
    if (this.p.align) {
      const n = UP;
      this.e.angularVelocity = mul(qdeltaVec(q, qmul(qfromto(up, n), q)), 14);
    }
    // Rotational KE: 1/2 w.I.w using world inertia.
    let rot = 0;
    try { const I = this.e.inertiaWorld; const Iw = add(add(mul(I.x, w.x), mul(I.y, w.y)), mul(I.z, w.z)); rot = 0.5 * dot(w, Iw); } catch (e) {}
    const E = (0.5 * m * dot(v, v) + rot + m * 9.81 * pos.y + springE) / m;
    if (this.E0 === null && this.t > 0.5) this.E0 = E;
    this.Emax = Math.max(this.Emax, E);
    if (Math.round(this.t * 60) % 6 === 0)
      console.log(`SPR ${this.p.label} t=${this.t.toFixed(2)} y=${pos.y.toFixed(3)} v=${len(v).toFixed(2)} up=${fmt(up)} w=${len(w).toFixed(2)} E=${E.toFixed(2)}`);
    if (Math.round(this.t * 60) === 900)
      console.log(`SPRSUM ${this.p.label} E0=${this.E0.toFixed(2)} Emax=${this.Emax.toFixed(2)} Efinal=${E.toFixed(2)}`);
  }
}
