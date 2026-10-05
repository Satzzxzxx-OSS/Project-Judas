/** Current JudasJS through M60; reviewed against ScriptSystem.cpp based on M59 icon
 * 277e67d2b0811644b2122bae27a4301d6dd3c96b. Tooling only, no TS runtime.
 * See JUDASJS.md. Ordinary returned objects are detached snapshots.
 */
declare module "judas" {
  export interface LiquidState {entityId:EntityId;entity:Entity|null;enabled:boolean;equilibriumValid:boolean;container:boolean;material:string;density:number;volume:number;capacity:number;stableCapacity:number;coordinate:number;surface:LiquidSurfaceState|null}
  export interface LiquidSurfaceState {enabled:boolean;cells:number;faces:number;volume:number;iterations:number;retries:number;limitedFaces:number;residual:number;partitionError:number;stepSeconds:number}
  export interface LiquidAccounting {reservoirs:number;containers:number;detached:number;total:number;expected:number;error:number;tolerance:number}
  export interface LiquidSample {entityId:EntityId;entity:Entity|null;material:string;density:number;depth:number;coordinate:number;surfacePoint:Vec3;normal:Vec3;up:Vec3;velocity:Vec3}
  export class LiquidVolume {constructor(handle:string);handle:string;readonly valid:boolean;readonly state:LiquidState;set enabled(value:boolean);transferTo(destination:LiquidVolume,volume:number,options?:{sourcePoint?:Vec3;destinationPoint?:Vec3}):number;applyImpulse(point:Vec3,impulse:Vec3):boolean;set surfaceEnabled(value:boolean)}
  export const liquid:{sample(point:Vec3):LiquidSample|null;samplePresented(point:Vec3):LiquidSample|null;accounting(material?:string):LiquidAccounting;readonly errors:{entityId:EntityId;message:string}[];readonly connections:{entityId:EntityId;active:boolean}[];submerged(target:Entity):{volume:number;center:Vec3;buoyancy:Vec3}};
  export type EntityId = string;
  export type AssetId = string;
  export type JSONValue = null | boolean | number | string | JSONValue[] | { [key: string]: JSONValue };
  export interface Vec3 { x: number; y: number; z: number }
  export interface Quat extends Vec3 { w: number }
  export interface Transform { position: Vec3; rotation: Quat; scale: Vec3 }
  export interface TransformPatch { position?: Vec3; rotation?: Quat; scale?: Vec3 }
  export interface CastPose { position: Vec3; rotation?: Quat }
  export interface Ray { origin: Vec3; direction: Vec3 }
  export interface QueryFilter {
    includeLayers?: string[]; excludeLayers?: string[];
    requiredTags?: string[]; excludedTags?: string[];
    ignored?: Entity[]; includeSensors?: boolean;
  }
  export interface Classification { renderLayer: number; collisionLayer: number; collisionMask: string }
  export interface CameraInfo { enabled: boolean; width: number; height: number }
  export interface AudioInfo {
    enabled:boolean;playing:boolean;requested:boolean;ready:boolean;streamed:boolean;loop:boolean;starved:boolean;
    state:"loading"|"seeking"|"ready"|"playing"|"paused"|"ended"|"starved"|"failed";
    error:string|null;position:number|null;duration:number|null;bufferBytes:number;underruns:number;
    dopplerRatio:number;occlusionGain:number;cutoff:number;distanceGain:number;
  }
  export interface AudioSettingsPatch {
    loading?:"buffered"|"streamed";streamPageFrames?:number;group?:string;loop?:boolean;spatial?:boolean;
    volume?:number;pitch?:number;doppler?:number;send?:number;occlusion?:boolean;bypass?:boolean;
    occlusionLayers?:string[];occludedGain?:number;occludedCutoff?:number;
    referenceDistance?:number;maximumDistance?:number;rolloff?:number;attenuation?:"none"|"inverse"|"linear";
  }
  export interface AudioGroup {gain:number;mute:boolean;paused:boolean}
  export interface AudioDiagnostics {voices:number;streams:number;bufferedBytes:number;streamBytes:number;streamHighWater:number;pendingRetirements:number;reverbProcessors:number;underruns:number;decodedFrames:number;occlusionQueries:number;maximumDetachMilliseconds:number}
  export const audio:{group(name:string):AudioGroup|null;setGroup(name:string,settings:Partial<AudioGroup>,fadeSeconds?:number):boolean|null;readonly diagnostics:AudioDiagnostics|null};
  export interface ParticleSettingsPatch { enabled?: boolean; rate?: number }
  export interface CastHit {
    entity: Entity | null; entityId: EntityId; bodyId: number;
    point: Vec3; normal: Vec3; distance: number; fraction: number;
    primitiveIndex: number; initialOverlap: boolean; shape: "sphere" | "box" | "terrain";
  }
  export interface LegacySweepHit { hit: boolean; distance: number; normal: Vec3; entityId: EntityId }
  export interface FluidSample { immersion: number; density: number; velocity: Vec3; acceleration: Vec3 }
  /** Plain wrappers reacquire native state. Treat the writable ID as opaque. */
  export interface Appearance {linearRendering:boolean;exposure:number;environmentAsset:AssetId;environmentIntensity:number;environmentRotation:Quat;environmentBackground:boolean}
  export interface MaterialParameters {baseColor?:Vec3 & {a:number};metallic?:number;roughness?:number;emissive?:Vec3;emissiveIntensity?:number}
  export interface MaterialState {asset:AssetId;ready:boolean;model:"legacy"|"pbr"|"unlit";alphaMode:"opaque"|"mask"|"blend";baseColor:Vec3 & {a:number};metallic:number;roughness:number;emissive:Vec3;emissiveIntensity:number;overridden:boolean}
  export class Material {
    constructor(entityId:EntityId,slot?:number);
    entityId:EntityId;slot:number;
    readonly state:MaterialState;
    assign(asset:AssetId):boolean;
    set(parameters:MaterialParameters):boolean;
    clearOverrides():boolean;
  }
  export class Entity {
    material(slot?:number):Material;
    readonly liquid:LiquidVolume|null;
    constructor(id: string | number | bigint);
    id: EntityId;
    readonly valid: boolean;
    get transform(): Transform;
    /** Render-interpolated world pose in presentationUpdate; authoritative pose otherwise. */
    readonly presentedTransform: Transform;
    set transform(value: TransformPatch);
    readonly parent: Entity | null;
    readonly children: Entity[];
    setColliderEnabled(enabled: boolean): boolean;
    destroy(): boolean;
    hasTag(tag: string): boolean;
    addTag(tag: string): boolean;
    removeTag(tag: string): boolean;
    readonly classification: Classification;
    readonly animation: Animation | null;
    readonly navigation: NavigationAgent | null;
    readonly navigationObstacle: NavigationObstacleInfo | null;
    readonly navigationLink: NavigationLinkInfo | null;
    setNavigationEnabled(component: "agent" | "obstacle" | "link", enabled: boolean): boolean;
    readonly character: Character | null;
    readonly ragdoll: Ragdoll | null;
    readonly audio: AudioInfo | null;
    setAudio(settings:AudioSettingsPatch):boolean;
    seekAudio(seconds:number):boolean;
    setAudioVelocity(velocity?:Vec3|null):boolean;
    playAudioOneShot():boolean;
    setAudioEnabled(enabled: boolean): boolean;
    readonly camera: CameraInfo | null;
    scriptState(slot: string | number): JSONValue;
    applyForce(value: Vec3): void;
    applyImpulse(value: Vec3): void;
    /** World-space impulse (N s) at a world-space point (metres). */
    applyImpulseAtPoint(impulse: Vec3, point: Vec3): void;
    applyTorque(value: Vec3): void;
    readonly mass: number;
    readonly inertiaWorld: {x: Vec3; y: Vec3; z: Vec3};
    velocity: Vec3;
    angularVelocity: Vec3;
    playAudio(): boolean;
    stopAudio(): boolean;
    pauseAudio(): boolean;
    resumeAudio(): boolean;
    burst(count: number): boolean;
    setParticles(settings: ParticleSettingsPatch): boolean;
    setCameraEnabled(enabled: boolean): boolean;
  }
  /** Does not check existence; null for absent/falsy input or string "0". Use .valid. */
  export function entity(id: string | number | bigint | null | undefined): Entity | null;
  export const world: {
    entity: typeof entity;
    readonly appearance:Appearance;
    setAppearance(settings:Partial<Appearance>):boolean;
    setView(pose: TransformPatch, fov?: number): boolean;
    clearView(): boolean;
    fluidSample(point: Vec3, up: Vec3, halfHeight: number, radius: number, tangent: Vec3): FluidSample;
    readonly viewRay: Ray | null;
    queryTags(required?: string[], excluded?: string[]): Entity[];
    spawnPrefab(asset: AssetId, transform: TransformPatch): Entity;
    /** Conservative broadphase candidates, not exact overlap results. */
    overlap(min: Vec3, max: Vec3, filter?: QueryFilter): Entity[];
    /** Legacy fixed player-sized capsule; prefer physics.capsuleCast for explicit dimensions. */
    sweepCapsule(from: Vec3, displacement: Vec3, rotation?: Quat, filter?: QueryFilter): LegacySweepHit;
  };
  export interface CharacterSettingsPatch {
    radius?: number; halfHeight?: number; offset?: Vec3;
    stepHeight?: number; supportDistance?: number; skin?: number; maxSlopeDegrees?: number;
    gravityScale?: number; reorientationDegreesPerSecond?: number;
    interactionMass?: number; maxPushImpulse?: number;
    collisionLayer?: string; collisionMask?: string[]; requiredTags?: string[]; excludedTags?: string[];
  }
  export interface CharacterState {
    velocity: Vec3; actualDisplacement: Vec3; supportNormal: Vec3; supportVelocity: Vec3;
    gravity: Vec3; up: Vec3; supported: boolean; collided: boolean;
    supportEntityId: EntityId; supportEntity: Entity | null;
  }
  export interface NavigationFilter {
    profile?: number | string; includeAreas?: string[]; excludeAreas?: string[];
    /** Traversal costs >= 1, keyed by project area name. */
    costs?: { [area: string]: number };
  }
  export interface NavigationLocation { position: Vec3; surfaceId: EntityId; surface: Entity | null; area: number }
  export interface NavigationCorner { position: Vec3; linkId: EntityId; link: Entity | null; linkEnd: Vec3 }
  export interface NavigationPath { status: "complete" | "partial" | "failed"; corners: NavigationCorner[]; distance: number; revision: number; areas: number[] }
  export interface NavigationAgentState {
    stopped: boolean; hasDestination: boolean; destination: Vec3; steering: Vec3;
    remainingDistance: number; reached: boolean; nextCorner: Vec3 | null;
    onLink: boolean; linkId: EntityId; link: Entity | null; linkEnd: Vec3;
    /** Agent path corners have native linkId, not the top-level path's link wrappers. */
    path: Omit<NavigationPath, "corners"> & { corners: Omit<NavigationCorner, "link">[] };
  }
  export interface NavigationAgentPatch extends NavigationFilter { speed?: number; arrival?: number; repathSeconds?: number; avoidance?: boolean }
  export interface NavigationObstacleInfo { enabled: boolean; cylinder: boolean; halfExtents: Vec3; radius: number; height: number }
  export interface NavigationLinkInfo { enabled: boolean; bidirectional: boolean; start: Vec3; end: Vec3; area: number }
  export class NavigationAgent {
    constructor(id: EntityId);
    id: EntityId;
    readonly state: NavigationAgentState;
    set enabled(value: boolean);
    setDestination(point: Vec3): boolean;
    clear(): boolean;
    get stopped(): boolean;
    set stopped(value: boolean);
    readonly steering: Vec3;
    readonly remainingDistance: number;
    completeLink(): boolean;
    configure(settings: NavigationAgentPatch): boolean;
  }
  export const navigation: {
    sample(point: Vec3, range?: number, filter?: NavigationFilter): NavigationLocation | null;
    path(start: Vec3, end: Vec3, filter?: NavigationFilter): NavigationPath;
    raycast(start: Vec3, end: Vec3, filter?: NavigationFilter): NavigationLocation | null;
    readonly areas: {id: number; name: string}[];
    readonly profiles: {id: number; name: string; radius: number; height: number}[];
    readonly errors: {entityId: EntityId; message: string}[];
  };
  export class Character {
    constructor(id: EntityId);
    id: EntityId;
    readonly state: CharacterState;
    get velocity(): Vec3;
    set velocity(value: Vec3);
    readonly supported: boolean;
    readonly supportNormal: Vec3;
    readonly supportVelocity: Vec3;
    readonly actualDisplacement: Vec3;
    readonly gravity: Vec3;
    readonly up: Vec3;
    /** Setter only in JS; reading returns undefined. TS cannot enforce write-only access. */
    set enabled(value: boolean);
    configure(settings: CharacterSettingsPatch): boolean;
    accelerate(value: Vec3): boolean;
    ignore(entities: Entity[]): boolean;
  }
  export class Ragdoll {
    constructor(id: EntityId);
    id: EntityId;
    readonly active: boolean;
    enter(): boolean;
    leave(seconds?: number): boolean;
    /** Setter only; read is undefined. */
    set enabled(value: boolean);
    body(joint: string): Entity | null;
  }
  export interface ClipInfo { name: string; duration: number }
  export interface AnimationLayerInfo { id: string; clip: string; weight: number; enabled: boolean; additive: boolean }
  export interface AnimationInfo {
    ready: boolean; playing: boolean; loop: boolean; speed: number; time: number; clip: string;
    clips: ClipInfo[]; transitioning: boolean; transitionFraction: number; error: string;
    joints: string[]; layers: AnimationLayerInfo[];
  }
  export interface AnimationLayerPatch {
    clip?: string; referenceClip?: string; weight?: number; speed?: number; time?: number;
    referenceTime?: number; enabled?: boolean; additive?: boolean; mask?: string[];
  }
  export class Animation {
    constructor(entityId: EntityId);
    entityId: EntityId;
    readonly info: AnimationInfo;
    readonly clips: ClipInfo[];
    readonly playing: boolean;
    readonly time: number;
    speed: number;
    loop: boolean;
    play(clip?: string): boolean;
    pause(): boolean;
    resume(): boolean;
    stop(): boolean;
    seek(time: number): boolean;
    crossFade(clip: string, seconds?: number): boolean;
    readonly layers: AnimationLayerInfo[];
    layer(id: string, settings: AnimationLayerPatch): boolean;
    removeLayer(id: string): boolean;
  }
  export interface JointState { active: boolean; enabled: boolean; coordinate: number; motorImpulse: number; type: 0 | 1 | 2 | 3 }
  export class Joint {
    constructor(id: string | number | bigint);
    id: string;
    readonly valid: boolean;
    readonly state: JointState;
    setEnabled(enabled: boolean): boolean;
    setLimits(lower: number, upper: number, limits?: boolean): boolean;
    setMotor(speed: number, maxForce: number, motor?: boolean): boolean;
    setSpring(rest: number, stiffness: number, damping: number, spring?: boolean): boolean;
  }
  export const physics: {
    joint(owner: Entity): Joint | null;
    raycast(origin: Vec3, direction: Vec3, maximum: number, filter?: QueryFilter): CastHit | null;
    sphereCast(origin: Vec3, radius: number, direction: Vec3, maximum: number, filter?: QueryFilter): CastHit | null;
    capsuleCast(pose: CastPose, radius: number, halfHeight: number, direction: Vec3, maximum: number, filter?: QueryFilter): CastHit | null;
    boxCast(pose: CastPose, halfExtents: Vec3, direction: Vec3, maximum: number, filter?: QueryFilter): CastHit | null;
  };
  /** Opaque request token; only valid in the requesting played-world session. */
  export type RegionRequest = string;
  export interface RegionStatus { id:string;state:"unloaded"|"preparing"|"prepared"|"installing"|"active"|"unloading"|"cancelled"|"failed"|"blocked";error:string;pins:string[];visualReady:boolean;entities:number;installed:number;bytes:number;retained:number;demands:number;preparationMs:number;integrationMs:number;largestUnitMs:number;loadMs:number }
  export interface StreamingStats {active:number;pending:number;pendingBytes:number;liveBytes:number;retainedBytes:number;resourceResidentBytes:number;resourceCacheBudget:number;integrationMs:number;largestUnitMs:number}
  export const scenes: {
    readonly regions:RegionStatus[];
    readonly streamingStats:StreamingStats;
    requestRegion(name:string,options?:{preload?:boolean}):RegionRequest;
    regionStatus(token:RegionRequest):RegionStatus|null;
    activateRegion(token:RegionRequest):boolean;
    releaseRegion(token:RegionRequest):boolean;
    unloadRegion(name:string):boolean;
    owner(entity:Entity):string;
    resolveRegionEntity(name:string,local:string|number):Entity|null;
    pinRegion(name:string,reason:string,pin?:boolean):boolean;
    adopt(entity:Entity,name?:string):boolean;
    setInterest(name:string,position:Vec3,options:{load:number;retain:number;priority?:number}):boolean;
    removeInterest(name:string):void;
    readonly current: string;
    readonly registered: string[];
    load(name: string): boolean;
    reload(): boolean;
  };
  export interface NumberOptions { minimumFraction?: number; maximumFraction?: number; grouping?: boolean }
  export const localization: {
    readonly locale: string;
    readonly available: string[];
    readonly revision: number;
    readonly direction: "ltr" | "rtl";
    /** True means request queued. Current locale/revision change only after all resources validate. */
    setLocale(locale: string): boolean;
    format(key: string, args?: Record<string, string | number>): string;
    number(value: number, options?: NumberOptions): string;
    reload(): void;
  };
  export const session: { get(key: string): JSONValue; set(key: string, value: JSONValue): void; delete(key: string): void };
  export const input: { pointerCapture: boolean; held(name: string): boolean; pressed(name: string): boolean; released(name: string): boolean; axis(name: string): number };
  export const time: { readonly elapsed: number; readonly delta: number; readonly fixed: boolean };
  export const console: { log(...args: unknown[]): void };
  export class UIElement {
    constructor(handle: number, id: string);
    handle: number;
    id: string;
    text: string;
    /** Assigning text clears an authored textKey. Assigning textKey restores localization binding. */
    textKey: string;
    font: AssetId;
    direction: "auto" | "ltr" | "rtl";
    /** Empty on legacy numeric alignment; writes must use a named value. */
    get textAlignment(): "" | "left" | "right" | "center" | "start" | "end";
    set textAlignment(value: "left" | "right" | "center" | "start" | "end");
    visible: boolean;
    enabled: boolean;
    value: number;
    texture: AssetId;
  }
  export class UIDocument {
    constructor(handle: number);
    handle: number;
    get(id: string): UIElement;
    visible: boolean;
    enabled: boolean;
    modal: boolean;
    show(): void;
    hide(): void;
    unload(): void;
  }
  export const ui: {
    get(name: string): UIDocument | null;
    load(asset: AssetId, name: string): UIDocument;
    quit(): void;
    debugOverlayVisible: boolean;
  };
  export interface UIEvent { document: string; element: string; type: "click" | "change" | "focus" | "back"; value: number }
  export interface ContactEvent { other: Entity | null; point: Vec3; normal: Vec3; relativeVelocity: Vec3; normalImpulse: number | null }
  export type ScriptProperties = Record<string, number | boolean | string>;
  export type PropertySchema = Record<string,
    { type: "number"; default?: number } | { type: "boolean"; default?: boolean } | { type: "string"; default?: string }>;
  export interface ScriptContext<P extends ScriptProperties = ScriptProperties> { entity: Entity; properties: P }
  /** Structural tooling interface, not a runtime-exported base class. */
  export interface ScriptBehaviour {
    state?: JSONValue;
    start?(dt: number): void;
    update?(dt: number): void;
    fixedUpdate?(dt: number): void;
    uiUpdate?(dt: number): void;
    /** After fixed steps, before camera/audio/render; alpha is the renderer interpolation fraction. */
    presentationUpdate?(dt: number, alpha: number): void;
    destroy?(dt: number): void;
    onUI?(event: UIEvent): void;
    onCollisionEnter?(event: ContactEvent): void;
    onCollisionStay?(event: ContactEvent): void;
    onCollisionExit?(event: ContactEvent): void;
    onTriggerEnter?(event: ContactEvent): void;
    onTriggerStay?(event: ContactEvent): void;
    onTriggerExit?(event: ContactEvent): void;
  }
  /** Diagnostics only: never use measured timings as game/simulation input. */
  export const profiler: {
    /** Calls synchronously exactly once; preserves the callback result/exception, even when capture is off. */
    scope<T>(label: string, callback: () => T): T;
    /** Finite value; sum resets per captured outer frame, latest is newest timestamp, max is largest observation. */
    counter(label: string, value: number, mode?: "sum" | "latest" | "max"): void;
  };

}
