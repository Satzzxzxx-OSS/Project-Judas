import {physics} from 'judas';
export default class {
  /** @param {import('judas').ScriptContext} context */
  constructor({entity}) {this.entity=entity;this.state={ray:false,shape:false,point:{x:0,y:0,z:0},normal:{x:0,y:0,z:0}};}
  fixedUpdate() {
    const origin={x:0,y:5,z:0},down={x:0,y:-1,z:0}; // Explicit flat example, not engine universal up.
    const filter={includeLayers:['Default'],ignored:[this.entity]};
    const hit=physics.raycast(origin,down,10,filter);
    this.state.ray=!!hit&&hit.entity!==null&&hit.entity.valid;
    const wide=physics.sphereCast(origin,.25,down,10,filter);
    this.state.shape=!!wide&&wide.distance>=0;
    if(hit){this.state.point=hit.point;this.state.normal=hit.normal;}
  }
}
