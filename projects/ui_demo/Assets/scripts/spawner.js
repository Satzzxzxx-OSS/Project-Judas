import {input,world,console} from 'judas';
import {vec} from './helpers.js';
export const properties={prefab:{type:'string',default:'40404040404040404040404040404010'},action:{type:'string',default:'spawn_prefab'}};
export default class Spawner {
 constructor({entity,properties}){this.entity=entity;this.properties=properties;this.state={spawned:0};}
 update(){if(input.pressed(this.properties.action)){
   const root=world.spawnPrefab(this.properties.prefab,{position:vec(3+(this.state.spawned%3),2,-this.state.spawned%3)});
   this.state.spawned++;console.log('Spawned independent scripted hierarchy',root.id);
   // This is the existing conservative AABB candidate query, not a new raycast.
   const candidates=world.overlap(vec(2,0,-4),vec(7,5,2),{includeLayers:['Default'],requiredTags:['spawned']});
   console.log('Tagged physics candidates:',candidates.length);
 }}
}
