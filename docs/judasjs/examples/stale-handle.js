import {world} from 'judas';
export const properties={prefab:{type:'string',default:'49494949494949494949494949494903'}};
export default class {
  /** @param {import('judas').ScriptContext<{prefab:string}>} context */
  constructor({entity,properties}){this.entity=entity;this.props=properties;this.state={invalid:false,throws:false,characterThrows:false,lookupNull:false,staleLookup:false};}
  start() {
    const e=world.spawnPrefab(this.props.prefab,this.entity.transform),motor=e.character;
    e.destroy();this.state.invalid=!e.valid;
    try{e.transform;}catch(error){this.state.throws=error instanceof ReferenceError;}
    try{if(motor)motor.state;}catch(error){this.state.characterThrows=error instanceof ReferenceError;}
    this.state.lookupNull=world.entity('0')===null;
    this.state.staleLookup=world.entity(e.id)?.valid===false;
  }
}
