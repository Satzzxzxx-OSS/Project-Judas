// Attach this script to an entity with a baked Deformable component.
export default class {
  /** @param {import('judas').ScriptContext} context */
  constructor({entity}) { this.entity = entity; this.state = {submitted:false}; }
  fixedUpdate() {
    const fabric = this.entity.deformable;
    if (!fabric || !fabric.valid || this.state.submitted) return;
    fabric.impulse({x:0, y:0, z:0.02});
    this.state.submitted = true;
    // Force/impulse use a total quantity, not one quantity per simulation node.
    // fabric.force({x:0, y:0, z:1}, 'top');
    // A picked current-surface hit can be used with impulseAt(hit.location,...).
  }
}
