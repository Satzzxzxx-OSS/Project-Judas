// Gap detection: a takeoff sensor arms a named gap; reaching the landing sensor
// while still in the same airborne/grind sequence awards it.
import {console} from 'judas';
import {game} from './game.js';
export const properties = {
  gap: {type: 'string', default: 'Gap'},
  role: {type: 'string', default: 'takeoff'},  // takeoff | landing
  points: {type: 'number', default: 250}
};
export default class {
  constructor({entity, properties}) { this.e = entity; this.p = properties; }
  onTriggerEnter(ev) {
    if (!ev.other || !ev.other.valid || !ev.other.hasTag('board')) return;
    const sk = game.skater;
    if (!sk) return;
    if (this.p.role === 'finish') {
      if (!this.done) { this.done = true; game.gap(this.p.gap, this.p.points); sk.finishRun = 1.5; }
      console.log(`FINISH ${this.p.gap} speed=${Math.hypot(sk.e.velocity.x, sk.e.velocity.y, sk.e.velocity.z).toFixed(2)}`);
      return;
    }
    if (this.p.role === 'takeoff') {
      game.gaps[this.p.gap] = {armed: true, launches: sk.tel.launches};
      console.log(`GAPZONE ${this.p.gap} takeoff launches=${sk.tel.launches} grounded=${sk.grounded}`);
    } else {
      const g = game.gaps[this.p.gap];
      // Valid if no ground contact happened since the takeoff sensor (same launch count, still airborne/grinding).
      if (g && g.armed && sk.mode !== 'bail' && (!sk.grounded || sk.tel.launches > g.launches)) {
        const launchesSince = sk.tel.launches - g.launches;
        if (launchesSince <= 1 && !sk.grounded) game.gap(this.p.gap, this.p.points);
      }
      if (g) g.armed = false;
      console.log(`GAPZONE ${this.p.gap} landing grounded=${sk.grounded} armed=${!!(g && g.armed)} launches=${sk.tel.launches}`);
    }
  }
}
