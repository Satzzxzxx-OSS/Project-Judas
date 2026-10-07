// Small project math helpers (plain {x,y,z} / {x,y,z,w} objects, as the judas API uses).
export const V = (x = 0, y = 0, z = 0) => ({x, y, z});
export const add = (a, b) => ({x: a.x + b.x, y: a.y + b.y, z: a.z + b.z});
export const sub = (a, b) => ({x: a.x - b.x, y: a.y - b.y, z: a.z - b.z});
export const mul = (a, s) => ({x: a.x * s, y: a.y * s, z: a.z * s});
export const dot = (a, b) => a.x * b.x + a.y * b.y + a.z * b.z;
export const cross = (a, b) => ({x: a.y * b.z - a.z * b.y, y: a.z * b.x - a.x * b.z, z: a.x * b.y - a.y * b.x});
export const len = a => Math.sqrt(dot(a, a));
export const norm = (a, fallback = {x: 0, y: 1, z: 0}) => {
  const l = len(a);
  return l > 1e-9 ? mul(a, 1 / l) : {...fallback};
};
export const lerp = (a, b, t) => add(a, mul(sub(b, a), t));
export const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));
// Component of a perpendicular to unit n.
export const reject = (a, n) => sub(a, mul(n, dot(a, n)));
export const UP = {x: 0, y: 1, z: 0};

export const Q = (w = 1, x = 0, y = 0, z = 0) => ({w, x, y, z});
export const qmul = (a, b) => ({
  w: a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
  x: a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
  y: a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
  z: a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w
});
export const qconj = q => ({w: q.w, x: -q.x, y: -q.y, z: -q.z});
export const qnorm = q => {
  const l = Math.hypot(q.w, q.x, q.y, q.z) || 1;
  return {w: q.w / l, x: q.x / l, y: q.y / l, z: q.z / l};
};
export const qaxis = (axis, angle) => {
  const n = norm(axis), s = Math.sin(angle / 2);
  return {w: Math.cos(angle / 2), x: n.x * s, y: n.y * s, z: n.z * s};
};
export const rotate = (q, v) => {
  const p = qmul(qmul(q, {w: 0, x: v.x, y: v.y, z: v.z}), qconj(q));
  return {x: p.x, y: p.y, z: p.z};
};
// Shortest rotation taking unit a onto unit b.
export const qfromto = (a, b) => {
  const d = dot(a, b);
  if (d < -0.999999) {
    let axis = cross({x: 1, y: 0, z: 0}, a);
    if (len(axis) < 1e-6) axis = cross({x: 0, y: 0, z: 1}, a);
    return qaxis(axis, Math.PI);
  }
  const c = cross(a, b);
  return qnorm({w: 1 + d, x: c.x, y: c.y, z: c.z});
};
// Rotation whose local -Z is forward and +Y is (approximately) up.
export const qlook = (forward, up) => {
  const f = norm(forward), r = norm(cross(f, up), {x: 1, y: 0, z: 0}), u = cross(r, f);
  // Columns: X=r, Y=u, Z=-f
  const m00 = r.x, m01 = u.x, m02 = -f.x, m10 = r.y, m11 = u.y, m12 = -f.y, m20 = r.z, m21 = u.z, m22 = -f.z;
  const t = m00 + m11 + m22;
  let q;
  if (t > 0) {
    const s = Math.sqrt(t + 1) * 2;
    q = {w: 0.25 * s, x: (m21 - m12) / s, y: (m02 - m20) / s, z: (m10 - m01) / s};
  } else if (m00 > m11 && m00 > m22) {
    const s = Math.sqrt(1 + m00 - m11 - m22) * 2;
    q = {w: (m21 - m12) / s, x: 0.25 * s, y: (m01 + m10) / s, z: (m02 + m20) / s};
  } else if (m11 > m22) {
    const s = Math.sqrt(1 + m11 - m00 - m22) * 2;
    q = {w: (m02 - m20) / s, x: (m01 + m10) / s, y: 0.25 * s, z: (m12 + m21) / s};
  } else {
    const s = Math.sqrt(1 + m22 - m00 - m11) * 2;
    q = {w: (m10 - m01) / s, x: (m02 + m20) / s, y: (m12 + m21) / s, z: 0.25 * s};
  }
  return qnorm(q);
};
export const qslerp = (a, b, t) => {
  let d = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
  if (d < 0) { b = {w: -b.w, x: -b.x, y: -b.y, z: -b.z}; d = -d; }
  if (d > 0.9995) return qnorm({w: a.w + (b.w - a.w) * t, x: a.x + (b.x - a.x) * t, y: a.y + (b.y - a.y) * t, z: a.z + (b.z - a.z) * t});
  const th = Math.acos(d), s = Math.sin(th), wa = Math.sin((1 - t) * th) / s, wb = Math.sin(t * th) / s;
  return {w: a.w * wa + b.w * wb, x: a.x * wa + b.x * wb, y: a.y * wa + b.y * wb, z: a.z * wa + b.z * wb};
};
// Rotation vector (axis * angle) taking q0 to q1 (world frame): q1 = d * q0.
export const qdeltaVec = (q0, q1) => {
  let d = qmul(q1, qconj(q0));
  if (d.w < 0) d = {w: -d.w, x: -d.x, y: -d.y, z: -d.z};
  const s = Math.sqrt(Math.max(0, 1 - d.w * d.w));
  const angle = 2 * Math.atan2(s, d.w);
  if (s < 1e-6) return {x: 0, y: 0, z: 0};
  return {x: d.x / s * angle, y: d.y / s * angle, z: d.z / s * angle};
};
export const angleBetween = (a, b) => Math.acos(clamp(dot(norm(a), norm(b)), -1, 1));
export const fmt = (v, d = 2) => v && typeof v === 'object'
  ? `(${v.x.toFixed(d)},${v.y.toFixed(d)},${v.z.toFixed(d)})` : Number(v).toFixed(d);
