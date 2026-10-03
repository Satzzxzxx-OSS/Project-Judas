export default class {
  /** @param {import('judas').ScriptContext} context */
  constructor({entity}) {this.entity=entity;this.state={enters:0,stays:0,exits:0,triggers:0,otherValid:false,normal:{x:0,y:0,z:0}};}
  /** @param {import('judas').ContactEvent} event */
  onCollisionEnter(event) {this.state.enters++;this.state.otherValid=!!event.other&&event.other.valid;this.state.normal=event.normal;}
  onCollisionStay() {this.state.stays++;}
  onCollisionExit() {this.state.exits++;}
  onTriggerEnter() {this.state.enters++;this.state.triggers++;}
  onTriggerStay() {this.state.stays++;}
  onTriggerExit() {this.state.exits++;}
}
