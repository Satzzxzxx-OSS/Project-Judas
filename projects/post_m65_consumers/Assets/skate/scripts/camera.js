// Third-person skate camera policy (project JS): velocity-led chase with
// orbit input, vert-air hold, obstruction sphere-cast and bail framing.
import {world,physics} from 'judas';
import {input} from './collection_input.js';
import {V, add, sub, mul, dot, len, norm, reject, clamp, UP, rotate, qlook, lerp} from './vec.js';

export class SkateCamera {
  constructor(skater) {
    this.s = skater;
    this.dir = rotate(skater.e.transform.rotation, V(0, 0, -1));
    this.dir = norm(reject(this.dir, UP), V(0, 0, -1));
    this.orbitYaw = 0; this.orbitPitch = 0; this.idle = 0;
    this.far = false;
    this.eye = null; this.target = null; this.fov = 70;
    this.filter = {excludeLayers: ['Board', 'Ragdoll', 'Pickup'], excludedTags: ['rider']};
    input.pointerCapture = true;
  }

  input(dt) {
    const s = this.s;
    if (s.ctl.pressed && s.ctl.pressed('camera_mode')) this.far = !this.far;
    const dx = input.axis('look_x') * 0.003 + input.axis('look_stick_x') * 2.4 * dt;
    const dy = input.axis('look_y') * 0.002 + input.axis('look_stick_y') * 1.6 * dt;
    if (Math.abs(dx) + Math.abs(dy) > 1e-5) this.idle = 0; else this.idle += dt;
    this.orbitYaw -= dx;
    this.orbitPitch = clamp(this.orbitPitch + dy, -0.5, 0.9);
    if (this.idle > 1.2) {
      const k = Math.min(1, dt * 2);
      this.orbitYaw *= 1 - k;
      this.orbitPitch *= 1 - k;
    }
  }

  present(dt) {
    const s = this.s;
    const pose = s.e.presentedTransform;
    let focus = add(pose.position, V(0, 1.1, 0));
    if (s.mode === 'bail' && s.view.ragdoll && s.view.ragdoll.active) {
      const hips = s.view.ragdoll.body('mixamorig:Hips');
      if (hips && hips.valid) focus = add(hips.presentedTransform.position, V(0, 0.4, 0));
    }
    const v = s.e.velocity;
    const planar = reject(v, UP);
    const vertAir = s.mode === 'ride' && !s.grounded && s.air && s.air.vert;
    if (!vertAir && s.mode !== 'bail') {
      let want = len(planar) > 1.5 ? norm(planar) : norm(reject(rotate(pose.rotation, V(0, 0, -1)), UP), this.dir);
      // Rolling fakie: keep looking the way we travel.
      const rate = s.grounded ? 2.5 : 1.2;
      this.dir = norm(lerp(this.dir, want, Math.min(1, dt * rate)), this.dir);
    }
    const yawQ = {w: Math.cos(this.orbitYaw / 2), x: 0, y: Math.sin(this.orbitYaw / 2), z: 0};
    const dir = rotate(yawQ, this.dir);
    const dist = (this.far ? 6.5 : 4.2) + clamp(len(v) * 0.08, 0, 1.5);
    const height = (this.far ? 2.6 : 1.5) + this.orbitPitch * 2.5;
    let eye = add(sub(focus, mul(dir, dist)), V(0, height, 0));
    // Keep the camera out of the park geometry.
    const toEye = sub(eye, focus), d = len(toEye);
    const hit = physics.sphereCast(focus, 0.25, toEye, d, this.filter);
    if (hit) eye = add(focus, mul(norm(toEye), Math.max(0.6, hit.distance - 0.05)));
    // Smooth (frame-rate independent), faster when far away.
    if (!this.eye) { this.eye = eye; this.target = focus; }
    const k = 1 - Math.exp(-dt * 10);
    this.eye = lerp(this.eye, eye, k);
    this.target = lerp(this.target, focus, 1 - Math.exp(-dt * 16));
    const fov = 68 + clamp(len(v) - 5, 0, 10) * 1.2;
    this.fov += (fov - this.fov) * Math.min(1, dt * 3);
    world.setView({position: this.eye, rotation: qlook(sub(this.target, this.eye), UP)}, this.fov);
  }

  release() { input.pointerCapture = false; world.clearView(); }
}
