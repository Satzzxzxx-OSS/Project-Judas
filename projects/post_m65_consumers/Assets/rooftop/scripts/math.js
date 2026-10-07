// Small vector / quaternion helpers (plain objects, like the rest of JudasJS).
export const v=(x=0,y=0,z=0)=>({x,y,z});
export const add=(a,b)=>({x:a.x+b.x,y:a.y+b.y,z:a.z+b.z});
export const sub=(a,b)=>({x:a.x-b.x,y:a.y-b.y,z:a.z-b.z});
export const mul=(a,s)=>({x:a.x*s,y:a.y*s,z:a.z*s});
export const dot=(a,b)=>a.x*b.x+a.y*b.y+a.z*b.z;
export const cross=(a,b)=>({x:a.y*b.z-a.z*b.y,y:a.z*b.x-a.x*b.z,z:a.x*b.y-a.y*b.x});
export const length=a=>Math.sqrt(dot(a,a));
export const norm=a=>{const l=length(a);return l>1e-6?mul(a,1/l):{x:0,y:0,z:0};};
/** Component of a perpendicular to unit vector u. */
export const tangent=(a,u)=>sub(a,mul(u,dot(a,u)));
export const lerp=(a,b,t)=>a+(b-a)*t;
export const clamp=(x,lo,hi)=>Math.max(lo,Math.min(hi,x));
/** Frame-rate independent exponential approach factor. */
export const damp=(rate,dt)=>1-Math.exp(-rate*dt);
export const qm=(a,b)=>({w:a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z,x:a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,y:a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,z:a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w});
export const axis=(a,t)=>({w:Math.cos(t/2),...mul(a,Math.sin(t/2))});
export const rotate=(q,p)=>{const r=qm(qm(q,{w:0,...p}),{w:q.w,x:-q.x,y:-q.y,z:-q.z});return {x:r.x,y:r.y,z:r.z};};
export const IDENTITY={w:1,x:0,y:0,z:0};
