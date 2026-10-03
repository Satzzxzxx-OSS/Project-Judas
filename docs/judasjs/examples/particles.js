export default class {
  /** @param {import('judas').ScriptContext} context */
  constructor({entity}) {this.entity=entity;this.state={configured:false,burst:false};}
  start() {this.state.configured=this.entity.setParticles({enabled:true,rate:8});this.state.burst=this.entity.burst(12);}
}
