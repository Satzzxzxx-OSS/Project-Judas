/** Current JudasJS through M53; reviewed against ScriptSystem.cpp based on M52
 * 3c52b13765a0721fa6a1fea0038505326359326b. Tooling only, no TS runtime.
 * See JUDASJS.md. Ordinary returned objects are detached snapshots.
 */
declare module "judas" {
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
  export interface AudioInfo { enabled: boolean; playing: boolean; requested: boolean }
  export interface ParticleSettingsPatch { enabled?: boolean; rate?: number }
  export interface CastHit {
    entity: Entity | null; entityId: EntityId; bodyId: number;
    point: Vec3; normal: Vec3; distance: number; fraction: number;
    primitiveIndex: number; initialOverlap: boolean; shape: "sphere" | "box" | "terrain";
  }
  export interface LegacySweepHit { hit: boolean; distance: number; normal: Vec3; entityId: EntityId }
  export interface FluidSample { immersion: number; density: number; velocity: Vec3; acceleration: Vec3 }
  /** Plain wrappers reacquire native state. Treat the writable ID as opaque. */
  export class Entity {
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
  export const scenes: {
    readonly current: string;
    readonly registered: string[];
    load(name: string): boolean;
    reload(): boolean;
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
}
