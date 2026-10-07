// Skater: one dynamic box body (rider+board mass) held above the ground by four
// raycast "wheel" springs. All skating meaning lives here, in project JS.
import {physics, world, console, time, scenes, ui, profiler} from 'judas';
import {V, add, sub, mul, dot, cross, len, norm, reject, clamp, UP, rotate, qmul, qconj, qaxis,
  qfromto, qlook, qdeltaVec, angleBetween, fmt} from './vec.js';
import {Controls} from './controls.js';
import {game, TRICKS} from './game.js';
import {Presentation} from './presentation.js';
import {SkateCamera} from './camera.js';
import {Sfx} from './sfx.js';

export const properties = {
  deckVisual: {type: 'entity', default: null},
  riderVisual: {type: 'entity', default: null},
  autopilot: {type: 'string', default: ''},
  telemetry: {type: 'boolean', default: false},
  runSeconds: {type: 'number', default: 180},
  startSpeed: {type: 'number', default: 0},
  delay: {type: 'number', default: 0},
  streamLoad: {type: 'number', default: 120},
  streamRetain: {type: 'number', default: 160},   // scenario: hold at spawn, then launch at startSpeed
  rest: {type: 'number', default: 0.26},       // suspension rest length below box bottom (m)
  stiffness: {type: 'number', default: 3700},  // per wheel N/m
  damping: {type: 'number', default: 380},     // per wheel N s/m
  pushSpeed: {type: 'number', default: 7.5},   // push won't accelerate beyond this (m/s)
  popSpeed: {type: 'number', default: 4.4},
  turnRate: {type: 'number', default: 2.3},
  grindSnap: {type: 'number', default: 0.55}
};

const HALF = V(0.10, 0.04, 0.24);
const WHEELS = [V(0.09, -0.04, -0.26), V(-0.09, -0.04, -0.26), V(0.09, -0.04, 0.26), V(-0.09, -0.04, 0.26)];
const G = 9.81;
const INVERT_RATE = 7.5;
const BOOST = {time: 0.6, accel: 5.0, cap: 12.0, cooldown: 1.0};   // speed-boost kick push   // rad/s: one full flip in ~0.84 s of air
const FLIPS = {kick: ['Kickflip', 'x', 1], heel: ['Heelflip', 'x', -1], shove: ['Pop Shove-it', 'y', 1],
  hard: ['Hardflip', 'xy', 1], varial: ['Varial Kickflip', 'xy', -1], impossible: ['Impossible', 'z', 1]};

export default class Skater {
  constructor({entity, properties}) {
    this.e = entity; this.p = properties;
    this.state = {checkpoint: null};
    this.mode = 'ride';          // ride | grind | bail
    this.grounded = false; this.wasGrounded = false;
    this.contacts = []; this.n = UP; this.visualDrop = 0.3;
    this.air = null; this.grind = null; this.manual = null; this.bailT = 0;
    this.crouch = 0; this.pushT = 0; this.pushing = false; this.coyote = 0; this.comboGrace = -1;
    this.trick = null; this.grab = null; this.switchStance = false; this.safe = [];
    this.tel = {t: 0, normalJumps: 0, launches: 0, landings: 0, bails: 0, maxSpeed: 0, steps: 0};
    this.grindHeldBuffer = 0; this.steer = 0; this.lean = 0; this.boostT = 0; this.boostCooldown = 0;
  }

  // New game: rules reset, optional checkpoint/scenario launch.
  start() {
    this.init();
    game.reset(this.p.runSeconds);
    const t = this.e.transform;
    if (this.state.checkpoint) this.respawn(this.state.checkpoint);
    this.launched = !(this.p.delay > 0);
    if (this.launched && this.p.startSpeed) this.e.velocity = mul(rotate(t.rotation, V(0, 0, -1)), this.p.startSpeed);
    console.log(`SKATER start rails=${this.rails.length} autopilot=${this.ctl.auto}`);
  }

  // M61 slot load: the engine restored body pose/velocity and this.state; rebuild the
  // module-level rules from state instead of starting a new run.
  restore() {
    this.init();
    game.load(this.state.game);
    this.launched = true;
    console.log(`SKATER restore score=${game.score} time=${game.timeLeft.toFixed(1)} rails=${this.rails.length} p=${fmt(this.e.transform.position)} v=${fmt(this.e.velocity)}`);
  }

  init() {
    ui.debugOverlayVisible = false;
    this.ctl = new Controls(this.p.autopilot);
    game.skater = this;
    this.filter = {ignored: [this.e], excludeLayers: ['Board', 'Ragdoll', 'Pickup']};
    this.mass = this.e.mass;
    this.rails = this.loadRails();
    this.view = new Presentation(this);
    this.camera = new SkateCamera(this);
    this.sfx = new Sfx(this);
    if (!this.spawnFixed) {
      const t = this.e.transform;
      this.state.spawn = this.state.spawn || {position: t.position, rotation: t.rotation};
      this.spawnFixed = true;
    }
    this.spawn = this.state.spawn;
  }

  loadRails() {
    // M64: the grind line comes from the rail's own box collider (Entity.collider); the
    // long axis of the box is the rail. rail.js now only carries a display name/kind.
    const out = [];
    for (const r of world.queryTags(['grindable'])) {
      const col = r.collider;
      if (!col || col.type !== 'box' || !col.halfExtents) continue;
      const data = r.scriptState(1) || {};
      const t = r.transform, q = qmul(t.rotation, col.rotation);
      const h = col.halfExtents;
      const along = h.x >= h.z ? V(1, 0, 0) : V(0, 0, 1), halfLength = Math.max(h.x, h.z);
      const axis = rotate(q, along);
      const centre = add(t.position, rotate(t.rotation, col.position));
      const top = add(centre, rotate(q, V(0, h.y, 0)));
      out.push({entity: r, a: sub(top, mul(axis, halfLength)), b: add(top, mul(axis, halfLength)),
        axis, length: halfLength * 2, up: rotate(q, UP), kind: data.kind || 'rail', name: data.name || 'Rail'});
    }
    return out;
  }

  // Streaming policy (game JS): one spatial interest at the skater; when a region
  // becomes active its rails must be added to the grind cache.
  stream() {
    const pos = this.e.transform.position;
    try {
      scenes.setInterest('skater', pos, {load: this.p.streamLoad, retain: this.p.streamRetain, priority: 10});
    } catch (err) { if (!this.streamErr) { this.streamErr = true; console.log(`STREAM setInterest failed: ${err}`); } return; }
    const regions = scenes.regions;
    let changed = false;
    this.regionState = this.regionState || {};
    for (const r of regions) {
      const prev = this.regionState[r.id];
      if (prev !== r.state) {
        changed = true;
        console.log(`STREAM t=${this.tel.t.toFixed(2)} ${r.id} ${prev || '-'} -> ${r.state} entities=${r.entities} ` +
          `prepMs=${r.preparationMs.toFixed(2)} integMs=${r.integrationMs.toFixed(2)} largestUnitMs=${r.largestUnitMs.toFixed(2)} ` +
          `skater=${fmt(pos)} v=${len(this.e.velocity).toFixed(1)}`);
        this.regionState[r.id] = r.state;
      }
    }
    if (changed) this.rails = this.loadRails();
  }

  // ---------------------------------------------------------------- fixed step
  fixedUpdate(dt) {
    profiler.scope('Skate fixed', () => this.step(dt));
  }

  step(dt) {
    const ctl = this.ctl;
    if (!this.launched) {
      if (Math.round((this.delayT || 0) * 60) % 6 === 0) this.stream();
      this.delayT = (this.delayT || 0) + dt;
      this.e.velocity = V(); this.e.angularVelocity = V();
      if (this.delayT < this.p.delay) return;
      this.launched = true;
      this.e.velocity = mul(this.frame().fwd, this.p.startSpeed);
    }
    ctl.tick(dt, this.e.transform.position);
    if (ctl.quit) { this.report(); ui.quit(); }
    game.tick(dt);
    this.tel.t += dt; this.tel.steps++;
    if (this.tel.steps % 10 === 0) this.state.game = game.snapshot();
    if (this.tel.steps % 60 === 0 && this.p.telemetry) console.log(`POS t=${this.tel.t.toFixed(2)} p=${fmt(this.e.transform.position)} score=${game.score}`);
    if (this.tel.steps % 6 === 0) this.stream();
    if (this.finishRun > 0) { this.e.velocity = mul(this.e.velocity, 1 - 3 * dt); this.finishRun -= dt; if (this.finishRun <= 0) { this.state.checkpoint = null; this.respawn(this.spawn); } }
    this.steer = ctl.axis('steer'); this.lean = ctl.axis('lean');
    if (ctl.pressed('restart')) { scenes.reload(); return; }
    if (ctl.pressed('checkpoint') && this.grounded && this.mode === 'ride') {
      const t = this.e.transform;
      this.state.checkpoint = {position: t.position, rotation: t.rotation};
      game.say('Checkpoint set');
    }
    if (game.over) { this.coast(dt); return; }
    if (ctl.pressed('bail') && this.mode !== 'bail') { this.startBail('forced'); }
    if (this.mode === 'bail') { this.bailStep(dt); return; }
    if (this.mode === 'grind') { this.grindStep(dt); return; }
    this.rideStep(dt);
  }

  frame() {
    const t = this.e.transform;
    return {pos: t.position, q: t.rotation, up: rotate(t.rotation, UP), fwd: rotate(t.rotation, V(0, 0, -1)),
      right: rotate(t.rotation, V(1, 0, 0))};
  }

  probe(f) {
    const hits = [];
    const reach = this.p.rest + 0.45;
    for (const w of WHEELS) {
      const wp = add(f.pos, rotate(f.q, w));
      const h = physics.raycast(add(wp, mul(f.up, 0.2)), mul(f.up, -1), reach + 0.2, this.filter);
      if (h) hits.push({wp, point: h.point, normal: h.normal, d: h.distance - 0.2, entity: h.entity});
    }
    return hits;
  }

  fitNormal(hits, up) {
    if (hits.length >= 3) {
      const c = hits.map(x => x.point);
      const a = sub(c[0], c[c.length - 1]), b = sub(c[1], c[2]);
      let n = norm(cross(a, b), up);
      if (dot(n, up) < 0) n = mul(n, -1);
      // Reject degenerate fits (e.g. two wheels on a rail top, two on the floor).
      const avg = norm(hits.reduce((s, x) => add(s, x.normal), V()), up);
      return dot(n, avg) > 0.8 ? n : avg;
    }
    return hits.length ? hits[0].normal : up;
  }

  rideStep(dt) {
    const f = this.frame();
    const hits = this.probe(f);
    const rest = this.p.rest;
    const contacts = hits.filter(x => x.d <= rest + 0.03);
    this.contacts = contacts;
    const grounded = contacts.length >= 2;
    const n = grounded ? this.fitNormal(contacts, f.up) : (hits.length ? this.fitNormal(hits, f.up) : UP);
    // (Spring suspension rejected: body-axis follower forces pumped energy; see journal J08.)
    let v = this.e.velocity;
    this.preImpactV = this.lastV || v;
    this.lastV = v;
    this.visualDrop = contacts.length ? contacts.reduce((s, x) => s + x.d, 0) / contacts.length + HALF.y : rest + HALF.y;
    if (grounded && this.prevN) {
      if (angleBetween(this.prevN, n) > 0.12) this.tel.normalJumps++;
    }
    this.prevN = grounded ? n : null;

    if (grounded && !this.wasGrounded && this.air) this.onLand(f, n, v);
    if (this.mode !== 'ride') { this.wasGrounded = grounded; return; }
    if (!grounded && this.wasGrounded && !this.air) this.onTakeoff(f, this.n, v, false);
    this.grounded = grounded; this.wasGrounded = grounded; this.n = n;

    if (grounded) this.groundControl(dt, f, n, v);
    else this.airControl(dt, f, v);

    // Grind requests (buffered so an early press still catches the rail).
    if (this.ctl.pressed('grind')) this.grindHeldBuffer = 0.3;
    this.grindHeldBuffer = Math.max(0, this.grindHeldBuffer - dt);
    if (this.grindHeldBuffer > 0 || (this.ctl.held('grind') && !grounded)) this.tryGrind(f);

    if (this.comboGrace >= 0) {
      this.comboGrace += dt;
      if (this.comboGrace > 0.3 && !this.manual) { this.comboGrace = -1; this.bank(); }
    }
    const speed = len(this.e.velocity);
    this.tel.maxSpeed = Math.max(this.tel.maxSpeed, speed);
    if (grounded && speed < 14 && !this.manual && dot(n, UP) > 0.95 && Math.round(this.tel.t * 2) !== this.lastSafe) {
      this.lastSafe = Math.round(this.tel.t * 2);
      this.safe.push({position: f.pos, rotation: f.q});
      if (this.safe.length > 8) this.safe.shift();
    }
    if (this.p.telemetry && this.tel.steps % 6 === 0) this.telemetry(f, n);
  }

  groundControl(dt, f, n, v) {
    const ctl = this.ctl;
    const fs = norm(reject(f.fwd, n)), rs = norm(cross(fs, n));
    // Velocity-level ground constraint. While continuously grounded, transport last
    // step's tangential velocity onto the new surface (speed preserved through
    // facet seams/curvature) and add gravity along the slope. Fall back to the
    // solver's velocity when a collision changed it (walls, kerbs).
    const g = mul(physics.gravity(f.pos), dt);
    let base = v;
    if (this.vSet && !this.justLanded) {
      const expected = add(this.vSet, g);
      if (len(sub(v, expected)) < 0.8) {
        const t0 = reject(this.vSet, n);
        const mag = this.vSetT;
        base = add(len(t0) > 1e-4 ? mul(norm(t0), mag) : t0, reject(g, n));
      }
    }
    this.justLanded = false;
    const vf = dot(base, fs), vl = dot(base, rs);
    let speed = Math.hypot(vf, vl);
    let dir = vf >= 0 ? 1 : -1;
    if (Math.abs(vf) < 0.02 && dot(n, UP) > 0.9) dir = this.switchStance ? -1 : 1;
    let nv = dir * speed;
    // Rolling resistance + air drag (game tuning, not engine friction).
    nv -= Math.sign(nv) * Math.min(Math.abs(nv), (0.10 + 0.0035 * speed * speed) * dt);
    // Push: cyclical kicks while held, only on flat-ish ground and below push speed.
    this.pushing = false;
    if (ctl.held('push') && !this.manual && this.crouch === 0) {
      this.pushT += dt;
      const phase = this.pushT % 0.95;
      if (phase < 0.5 && Math.abs(nv) < this.p.pushSpeed && dot(n, UP) > 0.85) {
        nv += dir * 4.2 * dt;
        this.pushing = true;
      }
    } else this.pushT = 0.0;
    // Speed boost: a strong kick push (plays the Push clip), capped and on a cooldown.
    this.boostCooldown = Math.max(0, this.boostCooldown - dt);
    if (ctl.pressed('boost') && this.boostCooldown === 0 && !this.manual && this.crouch === 0) {
      this.boostT = BOOST.time; this.boostCooldown = BOOST.cooldown;
    }
    if (this.boostT > 0) {
      this.boostT = Math.max(0, this.boostT - dt);
      if (Math.abs(nv) < BOOST.cap) nv += dir * Math.min(BOOST.accel * dt, BOOST.cap - Math.abs(nv));
      this.pushing = true;
    }
    if (ctl.held('brake') && !this.manual) nv -= Math.sign(nv) * Math.min(Math.abs(nv), 5 * dt);
    const lateralKeep = this.manual ? 0.0 : 0.04;
    // Normal velocity: hold the ride height (one gentle P term, limited pull-down).
    const dAvg = this.contacts.reduce((sum, x) => sum + x.d, 0) / this.contacts.length;
    const vn = clamp((this.p.rest - dAvg) * 10, -0.6, 3.0);
    v = add(add(mul(fs, nv), mul(n, vn)), mul(rs, vl * lateralKeep));
    this.e.velocity = v;
    this.vSet = v;
    this.vSetT = Math.hypot(nv, vl * lateralKeep);
    // Steering about the surface normal + alignment of the deck to the fitted plane.
    const speedFactor = clamp(Math.abs(nv) / 10, 0, 1);
    const yaw = -this.steer * this.p.turnRate * (1 - 0.45 * speedFactor) * (this.manual ? 0.5 : 1);
    const align = qdeltaVec(f.q, qmul(qfromto(f.up, n), f.q));
    this.e.angularVelocity = add(mul(align, 14), mul(n, yaw));
    // Ollie: crouch while held, pop on release.
    this.coyote = 0.12;
    if (ctl.held('ollie')) this.crouch = Math.min(0.6, this.crouch + dt);
    else if (this.crouch > 0 || ctl.released('ollie')) { this.pop(f, n, 'Ollie'); return; }
    // Manual (hold grab on the ground while rolling).
    if (ctl.held('grab') && speed > 1.2) {
      if (!this.manual) {
        this.manual = {t: 0, balance: (Math.random() - 0.5) * 0.2, nose: this.lean > 0.3};
        game.add(this.manual.nose ? 'Nose Manual' : 'Manual', TRICKS[this.manual.nose ? 'Nose Manual' : 'Manual']);
        this.comboGrace = -1;
      }
      const m = this.manual;
      m.t += dt;
      m.balance += (m.balance * 1.6 + (Math.random() - 0.5) * 1.4 - this.lean * (m.nose ? -2.2 : 2.2)) * dt;
      game.extend(40 * dt);
      if (Math.abs(m.balance) > 1) { this.manual = null; this.startBail('manual'); }
    } else if (this.manual) {
      this.manual = null;
      this.comboGrace = 0;
    }
  }

  pop(f, n, name) {
    const strength = 0.75 + 0.25 * clamp(this.crouch / 0.35, 0, 1);
    this.crouch = 0;
    const v = add(this.e.velocity, mul(n, this.p.popSpeed * strength));
    this.e.velocity = v;
    this.onTakeoff(f, n, v, true);
    this.wasGrounded = false;
    game.add(this.switchStance ? 'Nollie' : name, TRICKS[name]);
    this.sfx.oneShot('pop');
    if (this.manual) this.manual = null;
    this.comboGrace = -1;
  }

  onTakeoff(f, n, v, popped) {
    const vert = dot(n, UP) < 0.45;
    this.air = {t: 0, popped, vert, startY: f.pos.y, peakY: f.pos.y, spin: 0, takeoffN: n, flips: 0,
      takeoffPos: f.pos, plane: norm(reject(n, UP), n)};
    this.grounded = false;
    this.tel.launches++;
    this.boostT = 0;
    if (this.manual) { this.manual = null; }
    this.comboGrace = -1;
  }

  airControl(dt, f, v) {
    this.vSet = null;
    const ctl = this.ctl, a = this.air || (this.air = {t: 0, popped: false, vert: false, startY: f.pos.y,
      peakY: f.pos.y, spin: 0, takeoffN: UP, flips: 0});
    a.t += dt;
    a.peakY = Math.max(a.peakY, f.pos.y);
    this.coyote -= dt;
    if (this.coyote > 0 && ctl.released('ollie') && this.crouch > 0) { this.pop(f, a.takeoffN, 'Ollie'); return; }
    if (!ctl.held('ollie')) this.crouch = 0;
    // Spin about world up; steer gives a gentle yaw too.
    // Whole-body inversions: hold Invert, lean W/S for front/backflip, steer A/D to barrel roll.
    // Rotation actually performed is integrated from the body's angular velocity.
    const inverting = ctl.held('invert');
    const w = this.e.angularVelocity;
    a.pitch = (a.pitch || 0) + dot(w, f.right) * dt;
    a.roll = (a.roll || 0) + dot(w, f.fwd) * dt;
    const spinInput = (ctl.held('spin_right') ? 1 : 0) - (ctl.held('spin_left') ? 1 : 0);
    const yawRate = -spinInput * 7.0 - (inverting ? 0 : this.steer * 0.7);   // gentle air steering (spins are Q/E)
    a.spin += yawRate * dt;
    // Attitude assist: level toward the predicted landing surface (not in vert air, not while inverting).
    let assist = V();
    if (inverting) {
      assist = add(mul(f.right, -this.lean * INVERT_RATE), mul(f.fwd, this.steer * INVERT_RATE));
    } else if (!a.vert) {
      const h = physics.raycast(f.pos, V(0, -1, 0), 30, this.filter);
      const target = h ? h.normal : UP;
      assist = mul(qdeltaVec(f.q, qmul(qfromto(f.up, target), f.q)), 3.5);
    }
    const yawAxis = a.vert ? f.up : UP;
    if (a.vert && a.plane) {
      // Vert assist (game policy): keep vert air in the takeoff wall plane so the
      // skater comes back down onto the ramp face.
      const off = dot(sub(f.pos, a.takeoffPos), a.plane), vn = dot(v, a.plane);
      this.e.velocity = add(v, mul(a.plane, clamp(-off * 3, -1.5, 1.5) - vn));
    }
    this.e.angularVelocity = add(assist, mul(yawAxis, yawRate));
    // Flip tricks (board-only visual rotation; the body is rider+board).
    if (this.trick) {
      this.trick.t += dt;
      if (this.trick.t >= this.trick.dur) { this.trick = null; }
    }
    if (ctl.pressed('flip') && !this.trick && !this.grab && a.t > 0.05) {
      const k = this.lean > 0.5 ? (this.steer > 0.5 ? 'varial' : 'hard') : this.lean < -0.5 ? 'shove'
        : this.steer < -0.5 ? 'heel' : this.steer > 0.5 && this.lean < -0.2 ? 'impossible' : 'kick';
      const [name, axis, sign] = FLIPS[k];
      this.trick = {name, axis, sign, t: 0, dur: 0.42};
      game.add(name, TRICKS[name]);
      a.flips++;
    }
    // Grabs: hold to keep, points accrue.
    if (ctl.held('grab') && !this.trick) {
      if (!this.grab) {
        const name = this.lean > 0.5 ? 'Nosegrab' : this.lean < -0.5 ? 'Tailgrab' : this.steer < -0.5 ? 'Melon' : 'Indy';
        this.grab = {name, t: 0};
        game.add(name, TRICKS[name]);
      }
      this.grab.t += dt;
      game.extend(100 * dt);
    } else this.grab = null;
    game.stats.airTime += dt;
    if (a.vert && a.peakY - a.startY > 1.5) game.complete('vert');
  }

  onLand(f, n, v) {
    this.justLanded = true;
    const a = this.air;
    this.air = null;
    this.tel.landings++;
    const planar = reject(v, n), speed = len(planar);
    const fs = norm(reject(f.fwd, n));
    const tilt = angleBetween(f.up, n);
    let reason = null;
    if (tilt > 0.75) reason = 'over-rotated';
    if (this.trick && this.trick.t < this.trick.dur * 0.8) reason = 'flip unfinished';
    if (!reason && speed > 2.0) {
      const ang = angleBetween(fs, planar);
      // Within 70 degrees of travel: clean; beyond 110: lands fakie/switch; between: sideways bail.
      if (ang > 1.92) this.switchStance = !this.switchStance;
      else if (ang > 1.22) reason = 'landed sideways';
    }
    this.trick = null;
    if (reason) { this.startBail(reason); return; }
    if (a) {
      // Inversions: count whole rotations (the last ~60 degrees are finished by the attitude assist).
      const inv = (angle, pos, neg) => {
        const n = Math.floor((Math.abs(angle) + 1.0) / (2 * Math.PI));   // landing must be upright anyway
        if (n < 1) return;
        const base = angle > 0 ? pos : neg;
        const name = n === 1 ? base : `${['', '', 'Double', 'Triple', 'Quad'][Math.min(n, 4)]} ${base}`;
        game.add(name, TRICKS[base] * n * (n > 1 ? 1.5 : 1));
      };
      inv(a.pitch || 0, 'Backflip', 'Frontflip');
      inv(a.roll || 0, 'Barrel Roll', 'Barrel Roll');
      const turns = Math.round(Math.abs(a.spin) / Math.PI);
      if (turns >= 1) game.add(`${a.spin > 0 ? 'FS' : 'BS'} ${turns * 180}`, [0, 100, 300, 600, 1000][Math.min(turns, 4)]);
      if (a.t > 1.0 && game.combo) game.extend(Math.round(a.t * 50));
    }
    if (this.grab) { this.grab = null; }
    this.sfx.oneShot('land', clamp(-dot(v, n) / 6, 0.3, 1));
    this.view.landed(clamp(-dot(v, n) / 6, 0, 1));
    this.comboGrace = 0;
  }

  bank() {
    const pts = game.land();
    if (pts > 0) this.sfx.oneShot('score');
  }

  // ---------------------------------------------------------------- grinding
  closestOnRail(r, p) {
    const ab = sub(r.b, r.a);
    const raw = dot(sub(p, r.a), ab) / dot(ab, ab);
    const t = clamp(raw, 0, 1);
    return {t, raw, point: add(r.a, mul(ab, t))};
  }

  tryGrind(f) {
    if (this.mode !== 'ride') return;
    const bottom = sub(f.pos, mul(f.up, HALF.y));
    let best = null;
    for (const r of this.rails) {
      const c = this.closestOnRail(r, bottom);
      if (r === this.lastRail && this.tel.t < this.lastRailUntil) continue;   // no instant re-grab
      const beyond = Math.max(-c.raw, c.raw - 1) * r.length;   // metres outside the rail ends
      if (beyond > 0.5) continue;
      if (beyond > 0) {   // past an end: only catch it when travelling back onto the rail
        const outward = c.raw < 0 ? sub(r.a, r.b) : sub(r.b, r.a);
        if (dot(this.e.velocity, outward) > 0) continue;
      }
      if (beyond > 0) c.point = add(r.a, mul(sub(r.b, r.a), clamp(c.raw, 0.02, 0.98)));
      const rel = sub(bottom, c.point);
      const vertical = dot(rel, r.up);
      const lateral = len(reject(rel, r.up));
      if (lateral > this.p.grindSnap || vertical < -0.3 || vertical > 0.9) continue;
      const score = lateral + Math.abs(vertical) * 0.5;
      if (!best || score < best.score) best = {r, c, score};
    }
    if (!best) return;
    const r = best.r, v = this.e.velocity;
    let along = dot(v, r.axis);
    const facing = dot(f.fwd, r.axis);
    if (Math.abs(along) < 1.0) along = (facing >= 0 ? 1 : -1) * 1.0;
    const dirSign = along >= 0 ? 1 : -1;
    const travel = mul(r.axis, dirSign);
    const ang = angleBetween(reject(f.fwd, r.up), travel);
    const slide = ang > 0.6 && ang < Math.PI - 0.6;
    const name = slide ? 'Boardslide' : (this.lean < -0.5 ? '5-0' : '50-50');
    const backwards = !slide && ang >= Math.PI / 2;
    this.grind = {r, travel, speed: Math.max(Math.abs(along), 2.5), name, slide, backwards, t: 0,
      balance: (Math.random() - 0.5) * 0.3, tPos: best.c.t};
    this.mode = 'grind';
    this.grindHeldBuffer = 0;
    this.trick = null; this.grab = null; this.manual = null; this.comboGrace = -1;
    if (this.air) { this.air = null; }
    game.add(name, TRICKS[name]);
    this.sfx.grind(true);
    console.log(`GRIND enter ${r.name} ${name} speed=${this.grind.speed.toFixed(2)}`);
  }

  grindStep(dt) {
    const g = this.grind, r = g.r, f = this.frame();
    g.t += dt;
    if (game.combo) game.combo.grind = (game.combo.grind || 0) + dt;
    game.stats.grindTime += dt;
    game.extend((g.slide ? 60 : 50) * dt);
    // Speed along the rail: gravity component minus grind friction.
    g.speed += dot(physics.gravity(this.e.transform.position), g.travel) * dt - 0.6 * dt;
    // Balance meter.
    g.balance += (g.balance * 1.3 + (Math.random() - 0.5) * 1.2 + this.steer * 2.0) * dt;
    const c = this.closestOnRail(r, sub(f.pos, mul(f.up, HALF.y)));
    const target = add(add(c.point, mul(r.up, 0.10 + HALF.y)), mul(r.axis, 0));
    const correction = mul(sub(target, f.pos), 12);
    const vel = add(mul(g.travel, g.speed), correction);
    this.e.velocity = vel;
    const fwd = g.slide ? norm(cross(r.up, g.travel)) : (g.backwards ? mul(g.travel, -1) : g.travel);
    const qt = qlook(fwd, r.up);
    this.e.angularVelocity = mul(qdeltaVec(f.q, qt), 12);
    this.visualDrop = 0.12 + HALF.y;
    this.view.sparksAt(c.point, g.speed);
    const offEnd = (c.raw <= 0.0 && dot(g.travel, sub(r.a, r.b)) > 0) || (c.raw >= 1.0 && dot(g.travel, sub(r.b, r.a)) > 0);
    if (this.ctl.pressed('ollie') || this.ctl.released('ollie')) {
      this.exitGrind(f, true);
      return;
    }
    if (Math.abs(g.balance) > 1) { this.exitGrind(f, false); this.startBail('lost balance'); return; }
    if (offEnd || g.speed < 0.4) this.exitGrind(f, false);
  }

  exitGrind(f, jump) {
    const g = this.grind;
    this.grind = null;
    this.lastRail = g.r; this.lastRailUntil = this.tel.t + 0.4;
    this.mode = 'ride';
    this.sfx.grind(false);
    this.view.sparksAt(null, 0);
    let v = mul(g.travel, Math.max(g.speed, 1.5));
    if (jump) { v = add(v, mul(UP, this.p.popSpeed * 0.9)); this.sfx.oneShot('pop'); }
    this.e.velocity = v;
    this.air = {t: 0, popped: jump, vert: false, startY: f.pos.y, peakY: f.pos.y, spin: 0, takeoffN: UP, flips: 0};
    this.wasGrounded = false;
    this.grindHeldBuffer = 0;
    console.log(`GRIND exit ${jump ? 'jump' : 'end'} t=${g.t.toFixed(2)}`);
  }

  // ---------------------------------------------------------------- collisions / bails
  onCollisionEnter(ev) {
    if (this.mode !== 'ride' || !ev.other) return;
    // Hitting the ground deck-first while inverted: the wheel rays point away from the
    // surface and would never report a landing.
    if (this.air && dot(this.frame().up, ev.normal) < 0.4) { this.startBail('landed upside down'); return; }
    const approach = dot(ev.relativeVelocity, ev.normal);
    if (approach > 5.5 && Math.abs(ev.normal.y) < 0.6) this.startBail('slammed');
    else if (approach > 2.5 && Math.abs(ev.normal.y) < 0.6) this.sfx.oneShot('land', 0.4);
  }

  startBail(reason) {
    if (this.mode === 'bail') return;
    if (this.mode === 'grind') { this.grind = null; this.sfx.grind(false); this.view.sparksAt(null, 0); }
    this.mode = 'bail';
    this.bailT = 0;
    this.trick = null; this.grab = null; this.manual = null; this.air = null; this.comboGrace = -1;
    this.tel.bails++;
    game.bail(reason);
    this.sfx.oneShot('bail');
    this.view.bail(reason === 'slammed' && this.preImpactV ? this.preImpactV : this.e.velocity);
    console.log(`BAIL ${reason} at ${fmt(this.e.transform.position)} v=${len(this.e.velocity).toFixed(2)}`);
  }

  bailStep(dt) {
    this.bailT += dt;
    // The board body keeps rolling; colliders are frictionless so we damp it here.
    const f = this.frame();
    const hits = this.probe(f);
    if (hits.some(x => x.d < this.p.rest + 0.1)) this.e.velocity = mul(this.e.velocity, 1 - 1.5 * dt);
    if (this.bailT > 3.0 || (this.bailT > 1.2 && this.ctl.pressed('ollie'))) {
      this.respawn(this.state.checkpoint || this.safe[Math.max(0, this.safe.length - 3)] || this.spawn);
    }
  }

  respawn(at) {
    const up = rotate(at.rotation, UP);
    const level = qmul(qfromto(up, UP), at.rotation);
    this.e.transform = {position: add(at.position, V(0, 0.15, 0)), rotation: level};
    this.e.velocity = V(); this.e.angularVelocity = V();
    this.mode = 'ride'; this.air = null; this.wasGrounded = false; this.switchStance = false;
    this.view.recover();
    console.log(`RESPAWN ${fmt(at.position)}`);
  }

  coast(dt) {
    const f = this.frame();
    this.e.velocity = mul(this.e.velocity, 1 - 1.2 * dt);
    this.e.angularVelocity = mul(qdeltaVec(f.q, qmul(qfromto(f.up, UP), f.q)), 6);
  }

  // ---------------------------------------------------------------- frame phases
  update(dt) { this.camera.input(dt); }
  presentationUpdate(dt, alpha) {
    profiler.scope('Skate presentation', () => {
      this.view.present(dt);
      this.camera.present(dt);
      this.sfx.present(dt);
    });
  }

  telemetry(f, n) {
    const v = this.e.velocity;
    console.log(`TEL t=${this.tel.t.toFixed(2)} mode=${this.mode} g=${this.grounded ? 1 : 0} p=${fmt(f.pos)} v=${len(v).toFixed(2)} ` +
      `vy=${v.y.toFixed(2)} n=${fmt(n)} up=${fmt(f.up)} c=${this.contacts.length} drop=${this.visualDrop.toFixed(3)} ` +
      `E=${(0.5 * dot(v, v) + G * f.pos.y).toFixed(2)} score=${game.score} combo=${game.combo ? game.combo.points : 0}`);
  }

  report() {
    console.log(`REPORT ${JSON.stringify({tel: this.tel, score: game.score, bails: game.bails, stats: game.stats,
      goals: game.goals, events: game.events.slice(-40)})}`);
  }

  destroy() { this.camera && this.camera.release(); }
}
