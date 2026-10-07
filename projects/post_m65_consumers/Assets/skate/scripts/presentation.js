// Presentation: board mesh, rider placement/animation, ragdoll bails, sparks.
// Body-free entities follow the skater's interpolated pose in presentationUpdate.
import {world, console} from 'judas';
import {V, add, sub, mul, dot, len, norm, clamp, UP, rotate, qmul, qaxis, qslerp, fmt} from './vec.js';

const YAW180 = qaxis(UP, Math.PI);
// Toe height in the baked Ride pose. The final resolved pose now supplies
// live toe positions; the deck follows the lower planted foot instead of a
// clip-name/crossfade-fraction estimate. Push still lifts only one foot.
const REST_TOE_HEIGHT = 0.096;
const DECK_TOP = 0.108;
const RIDER_BACK = 0.02;   // feet centre is slightly behind the deck centre in the clip

function one(tag) {
  const list = world.queryTags([tag]);
  return list.length ? list[0] : null;
}

export class Presentation {
  constructor(skater) {
    this.s = skater;
    this.board = skater.p.deckVisual?.valid ? skater.p.deckVisual : one('deck_visual');
    this.rider = skater.p.riderVisual?.valid ? skater.p.riderVisual : one('rider');
    this.sparks = one('sparks');
    this.anim = this.rider ? this.rider.animation : null;
    this.ragdoll = this.rider ? this.rider.ragdoll : null;
    this.clip = '';
    this.leanLayer = false;
    this.landKick = 0;
    this.flipAngle = 0;
    this.sparkRate = 0;
    this.manualPitch = 0;
    this.lift = 0;
    if (!this.board || !this.rider) console.log('PRESENTATION missing board/rider entities');
  }

  setClip(name, fade = 0.18) {
    if (!this.anim || this.clip === name) return;
    if (!this.anim.info.ready) return;
    this.anim.crossFade(name, fade);
    this.clip = name;
  }

  updateLift() {
    if (!this.anim?.info.ready) return;
    const left = this.anim.jointTransform('mixamorig:LeftToeBase', 'model');
    const right = this.anim.jointTransform('mixamorig:RightToeBase', 'model');
    if (left && right) this.lift = Math.max(0, Math.min(left.position.y, right.position.y) - REST_TOE_HEIGHT);
  }

  present(dt) {
    const s = this.s;
    if (!this.board || !this.board.valid) return;
    const pose = s.e.presentedTransform;
    const up = rotate(pose.rotation, UP);
    // Deck pose: body pose dropped to the wheels' measured contact distance.
    let q = pose.rotation;
    this.landKick = Math.max(0, this.landKick - dt * 4);
    const target = s.manual ? (s.manual.nose ? -0.22 : 0.22) : 0;
    this.manualPitch += (target - this.manualPitch) * Math.min(1, dt * 10);
    if (Math.abs(this.manualPitch) > 1e-3) q = qmul(q, qaxis(V(1, 0, 0), this.manualPitch));
    let deckPos = sub(pose.position, mul(up, s.visualDrop));
    if (Math.abs(this.manualPitch) > 1e-3) {
      // Manual: pivot about the rear (or front) axle, not the deck centre.
      const pivotLocal = V(0, 0, this.manualPitch > 0 ? 0.26 : -0.26);
      const pivot = add(deckPos, rotate(pose.rotation, pivotLocal));
      deckPos = add(pivot, rotate(q, mul(pivotLocal, -1)));
    }
    this.updateLift();
    let boardQ = q;
    if (s.trick) {
      const k = clamp(s.trick.t / s.trick.dur, 0, 1), ang = k * Math.PI * 2 * s.trick.sign;
      const tr = s.trick.axis === 'x' ? qaxis(V(0, 0, 1), ang) : s.trick.axis === 'y' ? qaxis(UP, ang * 0.5)
        : s.trick.axis === 'z' ? qaxis(V(1, 0, 0), ang) : qmul(qaxis(UP, ang * 0.5), qaxis(V(0, 0, 1), ang));
      boardQ = qmul(q, tr);
    }
    // During a flip the board spins below the (tucked) feet; otherwise it stays under them.
    const liftBoard = this.lift - (s.trick ? 0.05 * Math.sin(clamp(s.trick.t / s.trick.dur, 0, 1) * Math.PI) : 0);
    if (s.mode !== 'bail' || !this.detached) {
      this.board.transform = {position: add(deckPos, mul(up, liftBoard + 0.03)), rotation: boardQ};
    } else {
      // Loose board after a bail: the physics box is the board now.
      this.board.transform = {position: sub(pose.position, mul(up, 0.06)), rotation: pose.rotation};
    }
    if (!this.rider || !this.rider.valid) return;
    if (this.ragdoll && this.ragdoll.active) return;   // ragdoll owns the rider pose
    // Rider stands on the deck (not on the flipping board), crouching on landings.
    const riderQ = qmul(q, YAW180);
    const riderPos = add(add(deckPos, mul(up, DECK_TOP + 0.03)), rotate(q, V(0, 0, RIDER_BACK)));
    this.rider.transform = {position: riderPos, rotation: riderQ};
    this.animate(dt);
  }

  animate(dt) {
    const s = this.s;
    if (!this.anim || !this.anim.info.ready) return;
    let clip = 'Ride';
    if (s.mode === 'grind') clip = 'Grind';
    else if (s.manual) clip = 'Manual';
    else if (s.grab) clip = 'Grab';
    else if (s.mode === 'ride' && !s.grounded && s.air && s.air.t > 0.08) clip = 'Air';
    else if (s.crouch > 0 || this.landKick > 0.4) clip = 'Crouch';
    else if (s.pushing) clip = 'Push';
    else if (len(s.e.velocity) > 0.4) clip = 'Cruise';
    this.setClip(clip, clip === 'Push' ? 0.25 : 0.15);
    // The boost is a faster, harder push.
    const speed = clip === 'Push' && s.boostT > 0 ? 1.4 : 1;
    if (this.anim.speed !== speed) this.anim.speed = speed;
    // Steering lean as an additive layer relative to the riding stance.
    const lean = clamp(s.steer, -1, 1);
    const want = s.mode === 'ride' && s.grounded && Math.abs(lean) > 0.05;
    if (want) {
      this.anim.layer('lean', {clip: lean < 0 ? 'LeanLeft' : 'LeanRight', referenceClip: 'Ride', additive: true,
        weight: Math.abs(lean), enabled: true});
      this.leanLayer = true;
    } else if (this.leanLayer) {
      this.anim.layer('lean', {weight: 0, enabled: false});
      this.leanLayer = false;
    }
  }

  landed(impact) { this.landKick = Math.max(this.landKick, impact); }

  sparksAt(point, speed) {
    if (!this.sparks || !this.sparks.valid) return;
    const rate = point ? clamp(40 + speed * 25, 0, 300) : 0;
    if (point) this.sparks.transform = {position: point};
    if (Math.abs(rate - this.sparkRate) > 1) {
      this.sparks.setParticles({rate});
      this.sparkRate = rate;
    }
  }

  bail(velocity) {
    this.detached = true;
    if (!this.ragdoll) return;
    try {
      this.ragdoll.enter();
      // Measure what the ragdoll inherited from the rider's (teleported) presentation motion.
      const hips = this.ragdoll.body('mixamorig:Hips');
      if (hips && hips.valid) {
        const inherited = hips.velocity;
        console.log(`RAGDOLL entered; hips inherited v=${fmt(inherited)} board v=${fmt(velocity)}`);
        // Give every mapped body the skater's momentum plus a forward tumble.
        for (const key of RAGDOLL_KEYS) {
          const b = this.ragdoll.body(key);
          if (b && b.valid) b.velocity = add(velocity, V(0, 1.2, 0));
        }
        hips.applyImpulse(mul(norm(velocity, V(0, 0, -1)), 25));
      }
    } catch (err) {
      console.log(`RAGDOLL enter failed: ${err}`);
    }
  }

  recover() {
    this.detached = false;
    if (this.ragdoll && this.ragdoll.active) this.ragdoll.leave(0.35);
    this.clip = '';
  }
}

export const RAGDOLL_KEYS = ['mixamorig:Hips', 'mixamorig:Spine1', 'mixamorig:Head', 'mixamorig:LeftArm',
  'mixamorig:LeftForeArm', 'mixamorig:RightArm', 'mixamorig:RightForeArm', 'mixamorig:LeftUpLeg',
  'mixamorig:LeftLeg', 'mixamorig:RightUpLeg', 'mixamorig:RightLeg', 'mixamorig:LeftFoot', 'mixamorig:RightFoot'];
