import {world} from 'judas';
export default class {
 /** @param {{entity:import('judas').Entity}} context */
 constructor({entity}){this.entity=entity;this.state={changed:false,reverted:false,appearance:false,stale:false};}
 start(){const material=this.entity.material();material.set({roughness:.8,baseColor:{x:.1,y:.2,z:.3,a:1}});this.state.changed=material.state.overridden&&Math.abs(material.state.roughness-.8)<1e-6;material.clearOverrides();this.state.reverted=!material.state.overridden;const initial=world.appearance;const changed=world.setAppearance({exposure:1.5});this.state.appearance=changed&&world.appearance.exposure===1.5;world.setAppearance(initial);const other=world.spawnPrefab("49494949494949494949494949494903",this.entity.transform);if(other){const handle=other.material();other.destroy();try{handle.state;}catch(error){this.state.stale=error instanceof ReferenceError;}}}
}
