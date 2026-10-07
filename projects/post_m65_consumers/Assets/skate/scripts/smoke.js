// Audit smoke probe: reports what the engine sees of the imported rider/board.
import {world, time, console, ui} from 'judas';
export const properties = {quitAfter: {type: 'number', default: 12}};
export default class {
  constructor({entity, properties}) { this.entity = entity; this.props = properties; this.logged = false; }
  update() {
    world.setView({position: {x: 0, y: 1.0, z: -0.6}}, 60);
    const a = this.entity.animation;
    if (!this.logged && a && a.info.ready) {
      const info = a.info;
      console.log('SMOKE clips', JSON.stringify(info.clips));
      console.log('SMOKE joints', info.joints.length, JSON.stringify(info.joints));
      console.log('SMOKE error', info.error);
      a.play('Ride');
      this.logged = true;
    }
    if (a && !a.info.ready && time.elapsed > 5 && !this.logged) {
      console.log('SMOKE not ready', JSON.stringify(a.info));
      this.logged = true;
    }
    if (time.elapsed > this.props.quitAfter) ui.quit();
  }
}
