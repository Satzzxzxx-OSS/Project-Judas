// Grind rail label. The grind line itself is read from the rail's box collider
// (Entity.collider, M64); this script only publishes a display name/kind.
export const properties = {
  kind: {type: 'string', default: 'rail'},
  name: {type: 'string', default: 'Rail'}
};
export default class {
  constructor({properties}) { this.state = {kind: properties.kind, name: properties.name}; }
}
