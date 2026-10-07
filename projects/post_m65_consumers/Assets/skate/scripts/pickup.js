// Collectible letter: an authored sensor. Spins, and hides itself when the
// skater's body enters it. (Trigger events reach the sensor's own scripts.)
import {console} from 'judas';
import {game} from './game.js';
import {qaxis, qmul, UP} from './vec.js';
export const properties = {letter: {type: 'string', default: 'S'}};
export default class {
  constructor({entity, properties}) { this.e = entity; this.letter = properties.letter; this.state = {taken: false}; }
  start() { this.base = this.e.transform; if (this.state.taken) this.hide(); }
  restore() { this.base = this.e.transform; if (this.state.taken) this.e.setColliderEnabled(false); }
  fixedUpdate(dt) {
    if (this.state.taken) return;
    const t = this.e.transform;
    this.e.transform = {rotation: qmul(qaxis(UP, dt * 2), t.rotation)};
  }
  onTriggerEnter(ev) {
    if (this.state.taken || !ev.other || !ev.other.valid || !ev.other.hasTag('board')) return;
    this.state.taken = true;
    game.collect(this.letter);
    this.hide();
    console.log(`PICKUP ${this.letter}`);
  }
  hide() {
    this.e.setColliderEnabled(false);
    const t = this.e.transform;
    this.e.transform = {position: {x: t.position.x, y: t.position.y - 50, z: t.position.z}};
  }
}
