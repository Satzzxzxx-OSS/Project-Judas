declare module "judas" {
  export interface Vec3 { x:number; y:number; z:number }
  export interface Quat extends Vec3 { w:number }
  export interface Transform { position:Vec3; rotation:Quat; scale:Vec3 }
  export interface QueryFilter { includeLayers?:string[]; excludeLayers?:string[]; requiredTags?:string[]; excludedTags?:string[]; ignored?:Entity[]; includeSensors?:boolean }
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
  export const world:{readonly viewRay:{origin:Vec3;direction:Vec3}|null;entity:typeof entity;queryTags(required?:string[],excluded?:string[]):Entity[];
    spawnPrefab(asset:string,transform:Partial<Transform>):Entity;
    overlap(min:Vec3,max:Vec3,filter?:QueryFilter):Entity[];
    sweepCapsule(from:Vec3,displacement:Vec3,rotation?:Quat,filter?:QueryFilter):{hit:boolean;distance:number;normal:Vec3;entityId:string}};
  export const input:{held(name:string):boolean;pressed(name:string):boolean;released(name:string):boolean;axis(name:string):number};
  export const time:{readonly elapsed:number;readonly delta:number;readonly fixed:boolean};
  export const console:{log(...values:unknown[]):void};
}

// M41 extends the same virtual module; event handlers are synchronous.
declare module 'judas' {
  export interface UIEvent { document:string; element:string; type:'click'|'change'|'focus'|'back'; value:number; }
  export class UIElement { readonly handle:number; readonly id:string; text:string; visible:boolean; enabled:boolean; texture:string; value:number; }
  export class UIDocument { readonly handle:number; visible:boolean; enabled:boolean; modal:boolean; get(id:string):UIElement; show():void; hide():void; unload():void; }
  export const ui:{get(name:string):UIDocument|null;load(asset:string,name:string):UIDocument;quit():void;debugOverlayVisible:boolean};
}

// M42 fixed-step contact snapshots. Normals point toward the recipient.
declare module 'judas' {
  interface ContactEvent {
    other: Entity;
    point: {x:number;y:number;z:number};
    normal: {x:number;y:number;z:number};
    relativeVelocity: {x:number;y:number;z:number};
    normalImpulse: number|null;
  }
  interface Entity { setColliderEnabled(enabled:boolean): boolean; }
}

declare module "judas" {
/** M43: project-relative registered scenes; first valid request wins this frame. */
export const scenes: {
    readonly current: string;
    readonly registered: string[];
    load(scene: string): boolean;
    reload(): boolean;
};
/** Detached, bounded JSON values. Survives scene changes, ends with Play/session. */
export const session: {
    get(key: string): unknown;
    set(key: string, value: unknown): void;
    delete(key: string): void;
};
}

// M44: explicit read-only snapshot queries, independent of physical collision masks.
declare module 'judas' {
  export interface CastPose {position:Vec3;rotation?:Quat}
  export interface CastHit {entity:Entity|null;entityId:string;bodyId:number;point:Vec3;normal:Vec3;
    distance:number;fraction:number;initialOverlap:boolean;primitiveIndex:number;shape:'sphere'|'box'|'terrain'}
  export const physics:{
    raycast(origin:Vec3,direction:Vec3,maximum:number,filter?:QueryFilter):CastHit|null;
    sphereCast(origin:Vec3,radius:number,direction:Vec3,maximum:number,filter?:QueryFilter):CastHit|null;
    capsuleCast(pose:CastPose,radius:number,halfHeight:number,direction:Vec3,maximum:number,filter?:QueryFilter):CastHit|null;
    boxCast(pose:CastPose,halfExtents:Vec3,direction:Vec3,maximum:number,filter?:QueryFilter):CastHit|null;
  };
}
