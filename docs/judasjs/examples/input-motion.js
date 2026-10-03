import {input} from 'judas';
export const properties = {speed: {type: 'number', default: 2}};
export default class {
  /** @param {import('judas').ScriptContext<{speed:number}>} context */
  constructor({entity, properties}) {this.entity=entity;this.props=properties;this.state={moved:false};}
  /** @param {number} dt */
  fixedUpdate(dt) {
    const t=this.entity.transform; // Detached snapshot; assignment below is a teleport.
    const delta=input.axis('move_x')*this.props.speed*dt;
    t.position.x+=delta; this.entity.transform=t;
    this.state.moved ||= delta!==0;
  }
}
