export const vec = (x,y,z) => ({x,y,z});
export const distance = (a,b) => Math.hypot(a.x-b.x,a.y-b.y,a.z-b.z);
