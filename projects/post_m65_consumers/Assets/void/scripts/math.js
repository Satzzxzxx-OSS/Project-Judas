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
export const qconj=q=>({w:q.w,x:-q.x,y:-q.y,z:-q.z});
export const qnorm=q=>{const l=Math.hypot(q.w,q.x,q.y,q.z)||1;return {w:q.w/l,x:q.x/l,y:q.y/l,z:q.z/l};};
export const slerp=(a,b,t)=>{let d=a.w*b.w+a.x*b.x+a.y*b.y+a.z*b.z;if(d<0){b={w:-b.w,x:-b.x,y:-b.y,z:-b.z};d=-d;}
 if(d>0.9995)return qnorm({w:a.w+(b.w-a.w)*t,x:a.x+(b.x-a.x)*t,y:a.y+(b.y-a.y)*t,z:a.z+(b.z-a.z)*t});
 const th=Math.acos(d),s=Math.sin(th),wa=Math.sin((1-t)*th)/s,wb=Math.sin(t*th)/s;return {w:a.w*wa+b.w*wb,x:a.x*wa+b.x*wb,y:a.y*wa+b.y*wb,z:a.z*wa+b.z*wb};};
/** Rotation whose -Z points along fwd and +Y is as close as possible to up. */
export const lookRotation=(fwd,up)=>{
 const f=norm(fwd);let r=cross(f,up);if(length(r)<1e-4)r=cross(f,Math.abs(f.y)<0.9?{x:0,y:1,z:0}:{x:1,y:0,z:0});r=norm(r);const u=cross(r,f);
 // basis columns: X=r, Y=u, Z=-f
 const m00=r.x,m01=u.x,m02=-f.x,m10=r.y,m11=u.y,m12=-f.y,m20=r.z,m21=u.z,m22=-f.z,tr=m00+m11+m22;
 let q;if(tr>0){const s=Math.sqrt(tr+1)*2;q={w:s/4,x:(m21-m12)/s,y:(m02-m20)/s,z:(m10-m01)/s};}
 else if(m00>m11&&m00>m22){const s=Math.sqrt(1+m00-m11-m22)*2;q={w:(m21-m12)/s,x:s/4,y:(m01+m10)/s,z:(m02+m20)/s};}
 else if(m11>m22){const s=Math.sqrt(1+m11-m00-m22)*2;q={w:(m02-m20)/s,x:(m01+m10)/s,y:s/4,z:(m12+m21)/s};}
 else{const s=Math.sqrt(1+m22-m00-m11)*2;q={w:(m10-m01)/s,x:(m02+m20)/s,y:(m12+m21)/s,z:s/4};}
 return qnorm(q);};
