import {physics} from 'judas';
export default class {
  /** @param {import('judas').ScriptContext} context */
  constructor({entity}) {this.entity=entity;this.state={controlled:false};}
  fixedUpdate() {
    const joint=physics.joint(this.entity);if(!joint||!joint.valid)return;
    joint.configure({rotationalResistance:.05}); // Explicit passive resistance on free hinge/ball axes.
    joint.setEnabled(true);joint.setLimits(-.5,.5);joint.setMotor(.2,10);
    this.state.controlled=joint.valid&&joint.state.enabled;
  }
}
