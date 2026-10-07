// Game rules (scoring, combos, goals, run timer). Pure project JS.
// Module globals are shared by every script in this world's VM; `game` is the
// single per-world rules object (rebuilt on scene reload).

export const TRICKS = {
  'Ollie': 50, 'Nollie': 60, 'Kickflip': 100, 'Heelflip': 100, 'Pop Shove-it': 80,
  'Hardflip': 150, 'Impossible': 150, 'Varial Kickflip': 180,
  'Indy': 150, 'Melon': 150, 'Nosegrab': 150, 'Tailgrab': 150,
  '50-50': 100, 'Boardslide': 120, '5-0': 110, 'Manual': 60, 'Nose Manual': 70,
  'Wallride': 150, 'Backflip': 400, 'Frontflip': 400, 'Barrel Roll': 350
};

export const GOALS = [
  {id: 'score', text: 'Score 8,000 points', target: 8000},
  {id: 'combo', text: 'Land a 1,500 point combo', target: 1500},
  {id: 'letters', text: 'Collect S-K-A-T-E', target: 5},
  {id: 'gap', text: 'Clear the Kicker Gap', target: 1},
  {id: 'grind', text: 'Grind 4 seconds in one combo', target: 4},
  {id: 'vert', text: 'Launch 1.5 m above a quarter-pipe lip', target: 1}
];

export class GameRules {
  constructor() { this.reset(60 * 3); }
  reset(runSeconds) {
    this.score = 0;
    this.best = 0;
    this.runSeconds = runSeconds;
    this.timeLeft = runSeconds;
    this.over = false;
    this.combo = null;
    this.lastCombo = null;        // {text, points, landed, age}
    this.message = '';
    this.messageAge = 0;
    this.letters = {};
    this.goals = {};
    this.gaps = {};
    this.bails = 0;
    this.events = [];             // audit trace
    this.stats = {tricks: 0, landed: 0, maxCombo: 0, airTime: 0, grindTime: 0};
  }
  // Plain-JSON snapshot for M61 slot saves (module globals are not serialized).
  snapshot() {
    return {score: this.score, best: this.best, runSeconds: this.runSeconds, timeLeft: this.timeLeft,
      over: this.over, letters: this.letters, goals: this.goals, bails: this.bails, stats: this.stats};
  }
  load(s) {
    this.reset(s && s.runSeconds || 180);
    if (!s) return;
    Object.assign(this, {score: s.score, best: s.best, timeLeft: s.timeLeft, over: s.over,
      letters: s.letters || {}, goals: s.goals || {}, bails: s.bails || 0, stats: s.stats || this.stats});
    this.say('Run restored');
  }
  tick(dt) {
    this.messageAge += dt;
    if (this.lastCombo) this.lastCombo.age += dt;
    if (!this.over && this.runSeconds > 0) {
      this.timeLeft = Math.max(0, this.timeLeft - dt);
      if (this.timeLeft === 0 && !this.combo) this.endRun();
    }
  }
  say(text) { this.message = text; this.messageAge = 0; }
  // A trick enters the current combo. `points` may be fractional (held tricks).
  add(name, points, unique = true) {
    if (this.over) return;
    if (!this.combo) this.combo = {tricks: [], points: 0, names: new Set(), grind: 0};
    const c = this.combo;
    const seen = c.names.has(name);
    const value = Math.round(points * (seen ? 0.5 : 1));   // repetition decays
    c.tricks.push({name, points: value});
    c.names.add(name);
    c.points += value;
    this.stats.tricks++;
    this.events.push(`trick ${name} ${value}`);
  }
  // Held tricks extend the last entry's points without a new multiplier step.
  extend(points) {
    if (!this.combo || !this.combo.tricks.length) return;
    const last = this.combo.tricks[this.combo.tricks.length - 1];
    last.points += points;
    this.combo.points += points;
  }
  get multiplier() { return this.combo ? Math.max(1, this.combo.tricks.length) : 0; }
  comboText() {
    if (!this.combo) return '';
    const names = this.combo.tricks.map(t => t.name);
    const shown = names.length > 6 ? ['…'].concat(names.slice(-6)) : names;
    return shown.join(' + ');
  }
  land() {
    if (!this.combo) return 0;
    const total = Math.round(this.combo.points * this.multiplier);
    this.score += total;
    this.stats.landed++;
    this.stats.maxCombo = Math.max(this.stats.maxCombo, total);
    if (total >= GOALS[1].target) this.complete('combo');
    if (this.combo.grind >= GOALS[4].target) this.complete('grind');
    if (this.score >= GOALS[0].target) this.complete('score');
    this.lastCombo = {text: this.comboText(), points: total, landed: true, age: 0};
    this.events.push(`land ${total}`);
    this.combo = null;
    if (this.timeLeft === 0) this.endRun();
    return total;
  }
  bail(reason) {
    this.bails++;
    this.events.push(`bail ${reason}`);
    if (this.combo) this.lastCombo = {text: this.comboText(), points: 0, landed: false, age: 0};
    this.combo = null;
    this.say(`Bail! (${reason})`);
    if (this.timeLeft === 0) this.endRun();
  }
  collect(letter) {
    if (this.letters[letter]) return;
    this.letters[letter] = true;
    this.say(`Letter ${letter}!`);
    if (Object.keys(this.letters).length >= 5) this.complete('letters');
  }
  gap(name, points) {
    if (this.combo) this.add(name, points);
    else { this.score += points; }
    this.say(`${name}! +${points}`);
    if (name === 'Kicker Gap') this.complete('gap');
  }
  complete(id) {
    if (this.goals[id]) return;
    this.goals[id] = true;
    const g = GOALS.find(x => x.id === id);
    this.say(`Goal complete: ${g ? g.text : id}`);
    this.events.push(`goal ${id}`);
  }
  endRun() {
    if (this.over) return;
    this.over = true;
    this.best = Math.max(this.best, this.score);
    this.events.push(`end ${this.score}`);
  }
}

export const game = new GameRules();
