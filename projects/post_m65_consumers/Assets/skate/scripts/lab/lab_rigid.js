// LAB variant R / R2: dynamic box body. R adds 4-ray spring/damper suspension;
// R2 (suspension=false) lets the box slide directly on the colliders.
// Autopilot: hold a heading, maintain an entry speed until the feature, then coast.
import {physics, console, time} from 'judas';
import {V, add, sub, mul, dot, cross, len, norm, reject, UP, rotate, qdeltaVec, qfromto, qmul, clamp, fmt} from '../vec.js';

export const properties = {
  label: {type: 'string', default: 'R'},
  suspension: {type: 'boolean', default: true},
  entrySpeed: {type: 'number', default: 7},
  holdUntil: {type: 'number', default: 1.0},   // seconds of speed holding
  log: {type: 'boolean', default: true}
};

const WHEELS = [V(0.09, 0, -0.26), V(-0.09, 0, -0.26), V(0.09, 0, 0.26), V(-0.09, 0, 0.26)];
const HALF_Y = 0.03, REST = 0.10, K = 3700, C = 320, PROBE = 0.35;

export default class {
  constructor({entity, properties}) {
    this.e = entity; this.p = properties;
    this.t = 0; this.prevN = null; this.airborne = 0; this.events = []; this.maxE = 0;
    this.lastGrounded = true; this.launches = 0; this.normalJumps = 0; this.jitter = 0;
  }
  start() {
    const fwd = rotate(this.e.transform.rotation, V(0, 0, -1));
    this.e.velocity = mul(fwd, this.p.entrySpeed);
    this.filter = {ignored: [this.e], excludeLayers: ['Board', 'Ragdoll']};
  }
  fixedUpdate(dt) {
    this.t += dt;
    const tr = this.e.transform, q = tr.rotation, pos = tr.position;
    const up = rotate(q, UP), fwd = rotate(q, V(0, 0, -1));
    let v = this.e.velocity;
    const m = this.e.mass;
    // Wheel probes.
    const hits = [];
    for (const w of WHEELS) {
      const wp = add(pos, rotate(q, add(w, V(0, -HALF_Y, 0))));
      const origin = add(wp, mul(up, 0.15));
      const h = physics.raycast(origin, mul(up, -1), PROBE + 0.15, this.filter);
      if (h) hits.push({wp, h, d: h.distance - 0.15});
    }
    const grounded = hits.length >= 2;
    let n = UP;
    if (hits.length >= 3) {
      // Plane fit through contact points (smooths facet seams), oriented to board up.
      const c = hits.map(x => x.h.point);
      const a = sub(c[0], c[hits.length - 1]), b = sub(c[1], c[hits.length >= 4 ? 2 : 2]);
      let pn = norm(cross(a, b));
      if (dot(pn, up) < 0) pn = mul(pn, -1);
      n = pn;
    } else if (hits.length) n = hits[0].h.normal;
    if (this.p.suspension) {
      for (const x of hits) {
        if (x.d > REST) continue;
        const pv = add(v, cross(this.e.angularVelocity, sub(x.wp, pos)));
        const closing = -dot(pv, up);
        const f = Math.max(0, K * (REST - x.d) + C * closing);
        this.e.applyImpulseAtPoint(mul(up, f * dt), x.wp);
      }
    }
    v = this.e.velocity;  // re-read: suspension impulses changed it
    if (grounded) {
      // Lateral grip with carve (speed preserved), rolling resistance, drag.
      const fs = norm(reject(fwd, n)), rs = norm(cross(fs, n));
      const vn = dot(v, n), vf = dot(v, fs), vl = dot(v, rs);
      const speed = Math.hypot(vf, vl);
      let nvf = Math.sign(vf || 1) * speed;
      nvf -= Math.sign(nvf) * Math.min(Math.abs(nvf), (0.12 + 0.004 * speed * speed) * dt);
      if (this.t < this.p.holdUntil) nvf = this.p.entrySpeed;
      v = add(add(mul(fs, nvf), mul(n, vn)), mul(rs, vl * 0.05));
      this.e.velocity = v;
      // Orientation: align up with n, keep yaw.
      const align = qdeltaVec(q, qmul(qfromto(up, n), q));
      this.e.angularVelocity = mul(align, 14);
      if (this.prevN) {
        const da = Math.acos(clamp(dot(this.prevN, n), -1, 1));
        if (da > 0.12) this.normalJumps++;
        this.jitter += da;
      }
      this.prevN = n;
    } else {
      this.e.angularVelocity = mul(this.e.angularVelocity, 0.98);
      this.prevN = null;
    }
    if (!grounded && this.lastGrounded) this.launches++;
    this.lastGrounded = grounded;
    const energy = 0.5 * dot(v, v) + 9.81 * pos.y;
    if (this.p.log && Math.round(this.t * 60) % 6 === 0)
      console.log(`TEL ${this.p.label} t=${this.t.toFixed(2)} p=${fmt(pos)} v=${len(v).toFixed(2)} vy=${v.y.toFixed(2)} g=${grounded ? 1 : 0} n=${fmt(n)} up=${fmt(up)} E=${energy.toFixed(2)} hits=${hits.length}`);
    if (Math.round(this.t * 60) === 60 * 9)
      console.log(`SUMMARY ${this.p.label} launches=${this.launches} normalJumps=${this.normalJumps} jitter=${this.jitter.toFixed(2)} finalE=${energy.toFixed(2)} pos=${fmt(pos)}`);
  }
}
