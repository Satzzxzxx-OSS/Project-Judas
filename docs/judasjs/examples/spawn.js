import {world} from 'judas';
export const properties = {prefab: {type:'string',default:'49494949494949494949494949494903'}};
export default class {
  /** @param {import('judas').ScriptContext<{prefab:string}>} context */
  constructor({entity,properties}) {this.entity=entity;this.props=properties;this.state={spawned:false};}
  start() {
    const t=this.entity.transform;t.position.x+=3;
    const root=world.spawnPrefab(this.props.prefab,t);
    this.state.spawned=root.valid&&root.id!==this.entity.id;
    // Cache wrapper outside saved state, not the wrapper inside this.state.
    this.spawned=root;
  }
}
