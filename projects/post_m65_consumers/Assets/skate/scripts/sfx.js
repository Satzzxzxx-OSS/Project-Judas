// Audio policy: one authored AudioEmitter per entity, so each sound is its own
// body-free entity found by tag and moved to the skater every frame.
import {world, console} from 'judas';
import {add, mul, len, clamp, V, dot, UP} from './vec.js';

const TAGS = {roll: 'sfx_roll', grind: 'sfx_grind', pop: 'sfx_pop', land: 'sfx_land', bail: 'sfx_bail',
  score: 'sfx_score'};

export class Sfx {
  constructor(skater) {
    this.s = skater;
    this.e = {};
    for (const k in TAGS) {
      const l = world.queryTags([TAGS[k]]);
      this.e[k] = l.length ? l[0] : null;
    }
    this.rollOn = false; this.grindOn = false; this.rollVol = -1;
  }
  oneShot(kind, volume = 1) {
    const e = this.e[kind];
    if (!e || !e.valid) return;
    e.setAudio({volume: clamp(volume, 0, 1)});
    e.playAudioOneShot();
  }
  grind(on) {
    const e = this.e.grind;
    if (!e || !e.valid || on === this.grindOn) return;
    this.grindOn = on;
    if (on) e.playAudio(); else e.stopAudio();
  }
  present() {
    const s = this.s;
    const pose = s.e.presentedTransform;
    for (const k in this.e) {
      const e = this.e[k];
      if (e && e.valid) e.transform = {position: pose.position};
    }
    const roll = this.e.roll;
    if (roll && roll.valid) {
      const v = s.e.velocity, speed = len(v);
      const on = s.mode === 'ride' && s.grounded && speed > 0.4;
      if (on !== this.rollOn) { this.rollOn = on; if (on) roll.playAudio(); else roll.pauseAudio(); }
      if (on) {
        const vol = clamp(speed / 9, 0.08, 1);
        if (Math.abs(vol - this.rollVol) > 0.03) {
          roll.setAudio({volume: vol, pitch: 0.75 + clamp(speed / 14, 0, 0.8)});
          this.rollVol = vol;
        }
        roll.setAudioVelocity(v);
      }
    }
  }
}
