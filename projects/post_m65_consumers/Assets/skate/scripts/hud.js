// HUD, pause menu, results screen, music and locale policy (project JS).
import {ui,scenes,localization,session,world,console,saves} from 'judas';
import {input} from './collection_input.js';
import {game, GOALS} from './game.js';
import {clamp} from './vec.js';

// Scenario-only properties (evidence capture): force a locale / open the pause menu at a time.
export const properties = {locale: {type: 'string', default: ''}, pauseAt: {type: 'number', default: 0},
  nonModal: {type: 'boolean', default: false}, saveAt: {type: 'number', default: 0},
  loadAt: {type: 'number', default: 0}};
export default class Hud {
  constructor({entity, properties}) { this.e = entity; this.p = properties || {}; this.state = {}; this.cache = {}; this.revision = -1; }
  start() {
    this.doc = ui.get('hud');
    this.music = world.queryTags(['music'])[0] || null;
    if (this.music && this.music.valid) this.music.playAudio();
    const saved = session.get('locale');
    if (saved && saved !== localization.locale) localization.setLocale(saved);
    if (!this.doc) console.log('HUD document missing');
    if (this.p.locale) localization.setLocale(this.p.locale);
  }
  restore() { this.restored = true; this.start(); this.setPaused(false); }
  set(id, text) {
    if (this.cache[id] === text) return;
    this.cache[id] = text;
    this.doc.get(id).text = text;
  }
  show(id, on) {
    const key = id + '#v';
    if (this.cache[key] === on) return;
    this.cache[key] = on;
    this.doc.get(id).visible = on;
  }
  fmt(key, args) { return localization.format(key, args); }
  uiUpdate(dt) {
    if (!this.doc) return;
    if (localization.revision !== this.revision) { this.revision = localization.revision; this.cache = {}; }
    const paused = this.cache['pause#v'] === true;
    const runT = game.runSeconds - game.timeLeft;
    if (this.p.saveAt > 0 && !this.restored && !this.savedOnce && runT > this.p.saveAt) { this.savedOnce = true; this.saveRun(); }
    if (this.p.loadAt > 0 && !this.restored && !this.loadedOnce && runT > this.p.loadAt) {
      this.loadedOnce = true; session.set('loadedOnce', true); this.loadRun();
    }
    if (this.p.pauseAt > 0 && !this.pausedOnce && game.runSeconds - game.timeLeft > this.p.pauseAt) { this.pausedOnce = true; this.setPaused(true); }
    if (input.pressed('pause') && !game.over) this.setPaused(!paused);
    if (this.request) {
      const st = saves.status(this.request.id);
      const text = st ? `${this.request.op}: ${st.state}${st.error ? ' — ' + st.error : ''}` : '';
      this.set('save_status', text);
      if (st) console.log(`SAVES ${this.request.op} ${st.state}${st.error ? ' ' + st.error : ''}`);
      if (!st || ['completed', 'failed', 'cancelled'].includes(st.state)) this.request = null;
    }
    this.set('score', this.fmt('hud.score', {score: game.score}));
    const t = Math.ceil(game.timeLeft), mm = Math.floor(t / 60), ss = t % 60;
    this.set('timer', `${mm}:${ss < 10 ? '0' : ''}${ss}`);
    this.set('letters', ['S', 'K', 'A', 'T', 'E'].map(l => game.letters[l] ? l : '·').join(' '));
    const done = GOALS.filter(g => game.goals[g.id]).length;
    this.set('goals', this.fmt('hud.goals', {done, total: GOALS.length}) + '\n' +
      GOALS.map(g => `${game.goals[g.id] ? '✔' : '○'} ${g.text}`).join('\n'));
    this.set('message', game.messageAge < 2.5 ? game.message : '');
    if (game.combo) {
      this.set('combo', game.comboText());
      this.set('combo_points', this.fmt('hud.combo', {points: Math.round(game.combo.points), mult: game.multiplier}));
    } else if (game.lastCombo && game.lastCombo.age < 2.0) {
      this.set('combo', game.lastCombo.text);
      this.set('combo_points', game.lastCombo.landed ? `+${game.lastCombo.points}` : 'BAILED');
    } else { this.set('combo', ''); this.set('combo_points', ''); }
    const sk = game.skater;
    if (sk) {
      const meter = sk.grind ? sk.grind.balance : sk.manual ? sk.manual.balance : null;
      this.show('balance', meter !== null);
      if (meter !== null) this.doc.get('balance').value = clamp(0.5 + meter / 2, 0, 1);
      const v = sk.e.velocity, kmh = Math.round(Math.hypot(v.x, v.y, v.z) * 3.6);
      this.set('speed', this.fmt('hud.speed', {speed: kmh}));
    }
    if (game.over && this.cache['results#v'] !== true) {
      session.set('best', Math.max(session.get('best') || 0, game.score));
      this.set('results_body', this.fmt('results.body', {score: game.score, best: session.get('best'),
        goals: done, total: GOALS.length, bails: game.bails}));
      this.show('results', true);
      this.doc.modal = !this.p.nonModal;
      input.pointerCapture = false;
    }
  }
  // M61 slots: one quick slot; the engine captures the world at a safe boundary.
  saveRun() {
    try { this.request = {op: 'save', id: saves.save('quick', {name: `Score ${game.score}`, metadata: {score: game.score}})}; }
    catch (err) { this.set('save_status', `save: ${err}`); console.log(`SAVES save threw ${err}`); }
  }
  loadRun() {
    try { this.request = {op: 'load', id: saves.load('quick')}; }
    catch (err) { this.set('save_status', `load: ${err}`); console.log(`SAVES load threw ${err}`); }
  }
  setPaused(on) {
    this.show('pause', on);
    this.doc.modal = on && !this.p.nonModal;
    input.pointerCapture = !on;
    if (this.music && this.music.valid) { if (on) this.music.pauseAudio(); else this.music.resumeAudio(); }
  }
  onUI(e) {
    if (e.document !== 'hud' || e.type !== 'click') {
      if (e.document === 'hud' && e.type === 'back' && this.cache['pause#v']) this.setPaused(false);
      return;
    }
    if (e.element === 'resume') this.setPaused(false);
    else if (e.element === 'restart' || e.element === 'again') scenes.reload();
    else if (e.element === 'quit') ui.quit();
    else if (e.element === 'savegame') this.saveRun();
    else if (e.element === 'loadgame') this.loadRun();
    else if (e.element === 'language') {
      const next = localization.locale === 'en' ? 'es' : 'en';
      localization.setLocale(next);
      session.set('locale', next);
    }
  }
}
