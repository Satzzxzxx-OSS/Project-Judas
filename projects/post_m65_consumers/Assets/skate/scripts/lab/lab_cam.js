// LAB observer camera: fixed side view of a lane, quits after a fixed time.
import {world, ui, time} from 'judas';
import {V, sub, norm, qlook, UP} from '../vec.js';
export const properties = {
  x: {type: 'number', default: 12}, y: {type: 'number', default: 4}, z: {type: 'number', default: -10},
  tx: {type: 'number', default: 0}, ty: {type: 'number', default: 1}, tz: {type: 'number', default: -12},
  fov: {type: 'number', default: 60}, quitAfter: {type: 'number', default: 10}
};
export default class {
  constructor({properties}) { this.p = properties; }
  start() { ui.debugOverlayVisible = false; }
  presentationUpdate() {
    const p = this.p, eye = V(p.x, p.y, p.z);
    world.setView({position: eye, rotation: qlook(sub(V(p.tx, p.ty, p.tz), eye), UP)}, p.fov);
  }
  update() { if (time.elapsed > this.p.quitAfter) ui.quit(); }
}
