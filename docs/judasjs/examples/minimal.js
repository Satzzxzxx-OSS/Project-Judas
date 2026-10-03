import {time, console} from 'judas';
export const properties = {label: {type: 'string', default: 'Example'}};
export default class {
  /** @param {import('judas').ScriptContext} context */
  constructor({entity, properties}) { this.entity=entity; this.props=properties; this.state={started:0,frames:0,steps:0}; }
  start() { this.state.started++; console.log(this.props.label); }
  update() { this.state.frames++; }
  fixedUpdate() { if (time.fixed) this.state.steps++; }
  destroy() { console.log('Ending', this.entity.id); }
}
