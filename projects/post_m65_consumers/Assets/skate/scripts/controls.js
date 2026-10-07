// Logical controls. Live play reads the project input map. An optional
// "autopilot" program (game feature: attract/demo mode, also used for the
// audit's reproducible scenarios) replaces the live source.
//
// Program syntax: "t:cmd;t:cmd" with t in seconds since start, cmd one of
//   name=value  (axis/held value until changed; e.g. push=1, steer=-0.5)
//   name        (one-step press: held for one fixed step, pressed edge)
//   name~dur    (press and hold for dur seconds, then release edge)
//   quit        (request application quit; profiling runs end here)
// A command may be position-triggered instead of timed: "@z<3.2:ollie~0.2"
// (axis x|y|z, comparator < or >) fires once when the skater first satisfies it.
import {time} from 'judas';
import {input} from './collection_input.js';

const AXES = ['steer', 'lean'];

export class Controls {
  constructor(program) {
    this.auto = !!program;
    this.events = [];
    this.values = {};
    this.prev = {};
    this.edges = {};
    this.releaseAt = {};
    this.quit = false;
    this.t = 0;
    this.triggers = [];
    if (program) {
      for (const part of program.split(';')) {
        const s = part.trim();
        if (!s) continue;
        const i = s.indexOf(':');
        const when = s.slice(0, i).trim(), cmd = s.slice(i + 1).trim();
        if (when[0] === '@') this.triggers.push({axis: when[1], op: when[2], value: parseFloat(when.slice(3)), cmd});
        else this.events.push({t: parseFloat(when), cmd});
      }
      this.events.sort((a, b) => a.t - b.t);
    }
  }
  // Called once per fixed step before reads.
  tick(dt, pos) {
    if (!this.auto) return;
    this.t += dt;
    this.prev = {...this.values};
    for (const k in this.releaseAt) if (this.t >= this.releaseAt[k]) { this.values[k] = 0; delete this.releaseAt[k]; }
    for (const k in this.edges) if (this.edges[k] === 'tap') this.values[k] = 0;
    this.edges = {};
    if (pos) {
      for (const tr of this.triggers) {
        if (tr.done) continue;
        const v = pos[tr.axis];
        if (tr.op === '<' ? v < tr.value : v > tr.value) { tr.done = true; this.events.unshift({t: this.t, cmd: tr.cmd}); }
      }
    }
    while (this.events.length && this.events[0].t <= this.t) {
      const {cmd} = this.events.shift();
      if (cmd === 'quit') { this.quit = true; continue; }
      if (cmd.includes('=')) { const [k, v] = cmd.split('='); this.values[k] = parseFloat(v); }
      else if (cmd.includes('~')) { const [k, d] = cmd.split('~'); this.values[k] = 1; this.releaseAt[k] = this.t + parseFloat(d); }
      else { this.values[cmd] = 1; this.edges[cmd] = 'tap'; }
    }
  }
  axis(name) {
    if (this.auto) return this.values[name] || 0;
    return input.axis(name);
  }
  held(name) {
    if (this.auto) return (this.values[name] || 0) > 0.5;
    return input.held(name);
  }
  pressed(name) {
    if (this.auto) return (this.values[name] || 0) > 0.5 && !((this.prev[name] || 0) > 0.5);
    return input.pressed(name);
  }
  released(name) {
    if (this.auto) return !((this.values[name] || 0) > 0.5) && (this.prev[name] || 0) > 0.5;
    return input.released(name);
  }
}
