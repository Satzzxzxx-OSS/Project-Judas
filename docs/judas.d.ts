declare module "judas" {
  export interface Vec3 { x:number; y:number; z:number }
  export interface Quat extends Vec3 { w:number }
  export interface Transform { position:Vec3; rotation:Quat; scale:Vec3 }
  export interface QueryFilter { includeLayers?:string[]; excludeLayers?:string[]; requiredTags?:string[]; excludedTags?:string[]; ignored?:Entity[] }
  export class Entity {
    constructor(id:string);
    readonly id:string; readonly valid:boolean;
    transform:Transform; readonly parent:Entity|null; readonly children:Entity[];
    readonly classification:{renderLayer:number;collisionLayer:number;collisionMask:string};
    readonly camera:{enabled:boolean;width:number;height:number}|null;
    readonly audio:{enabled:boolean;playing:boolean;requested:boolean}|null;
    destroy():boolean; hasTag(tag:string):boolean; addTag(tag:string):boolean; removeTag(tag:string):boolean;
    scriptState(slot:number|string):object|null;
    applyForce(force:Vec3):void; applyImpulse(impulse:Vec3):void; applyTorque(torque:Vec3):void;
    velocity:Vec3; angularVelocity:Vec3;
    playAudio():boolean; stopAudio():boolean; pauseAudio():boolean; resumeAudio():boolean;
    setAudioEnabled(enabled:boolean):boolean; burst(count:number):boolean;
    setParticles(settings:{enabled?:boolean;rate?:number}):boolean;
    setCameraEnabled(enabled:boolean):boolean;
  }
  export function entity(id:string):Entity|null;
  export const world:{entity:typeof entity;queryTags(required?:string[],excluded?:string[]):Entity[];
    spawnPrefab(asset:string,transform:Partial<Transform>):Entity;
    overlap(min:Vec3,max:Vec3,filter?:QueryFilter):Entity[];
    sweepCapsule(from:Vec3,displacement:Vec3,rotation?:Quat,filter?:QueryFilter):{hit:boolean;distance:number;normal:Vec3;entityId:string}};
  export const input:{held(name:string):boolean;pressed(name:string):boolean;released(name:string):boolean;axis(name:string):number};
  export const time:{readonly elapsed:number;readonly delta:number;readonly fixed:boolean};
  export const console:{log(...values:unknown[]):void};
}
