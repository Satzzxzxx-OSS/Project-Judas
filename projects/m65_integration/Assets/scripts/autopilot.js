// Scripted input scenarios used ONLY for automated testing (runner.js
// `autopilot` property). The engine's JUDAS_TEST_SCRIPT harness cannot press
// project-named actions such as "jump"/"slide", so tests drive the same
// intent object the keyboard would.
//
// Each step: {d: seconds} and/or {z: advance when position.z < z}, plus
// held inputs my/mx (-1..1), jump/slide (pressed on entry, held for the
// step), yaw (absolute, set on entry).
const run=(z,extra={})=>({z,my:1,d:12,...extra});
export const scenarios={
 idle:{spawn:[0,21,8],steps:[{d:3}]},
 vault:{spawn:[0,21,8],steps:[run(-3.6),{d:0.15,my:1,jump:1},run(-12.6),{d:0.15,my:1,jump:1},{d:1.5,my:1}]},
 gap:{spawn:[0,21,-15.5],steps:[run(-19.4),{d:1.6,my:1,jump:1},{d:1}]},
 wallrun:{spawn:[5.0,20.2,-30],steps:[run(-47.5),{d:0.12,my:1,mx:0.8,jump:1},{z:-57,d:2,my:1,mx:0.3},{d:1.6,my:1,jump:1},{d:1}]},
 climb:{spawn:[0,20.2,-74],steps:[run(-80.6),{d:1.6,my:1,jump:1},{d:1,my:1}]},
 slide:{spawn:[0,24,-84],steps:[run(-90.2),{d:1.3,my:1,slide:1},{d:1,my:1}]},
 drop:{spawn:[0,24,-102],steps:[run(-109.5),{d:0.15,my:1,jump:1},{d:0.75,my:1},{d:0.6,my:1,slide:1},{d:1.2,my:1}]},
 ramp:{spawn:[0,18.4,-130],steps:[run(-141.6),{d:1.8,my:1,jump:1},run(-168)]},
 noslide:{spawn:[0,24,-84],steps:[{d:3,my:1}]},
 // Real-render screenshot timings (JUDAS_TERRAIN_SCREENSHOT fires at 10 s).
 shot_start:{spawn:[0,21,8],steps:[{d:20}]},
 shot_wallrun:{spawn:[5.0,20.2,-30],steps:[{d:7.75},run(-47.5),{d:0.12,my:1,mx:0.8,jump:1},{z:-57,d:2,my:1,mx:0.3}]},
 shot_slide:{spawn:[0,24,-84],steps:[{d:8.6},run(-90.2),{d:1.3,my:1,slide:1}]},
 shot_climb:{spawn:[0,20.2,-74],steps:[{d:8.6},run(-80.6),{d:1.6,my:1,jump:1}]},
 course:{spawn:[0,21,8],steps:[
  run(-3.6),{d:0.15,my:1,jump:1},run(-12.6),{d:0.15,my:1,jump:1},
  run(-19.4),{d:0.15,my:1,jump:1},{d:0.9,my:1},
  // Drift right toward the wall-run tower, then run along it.
  {z:-38,d:5,my:1,mx:0.5},run(-47.5),{d:0.12,my:1,mx:0.6,jump:1},{z:-56,d:2,my:1,mx:0.2},{d:1.0,my:1,jump:1},
  // Line up with the climb wall.
  {z:-70,d:4,my:1,mx:-0.6},run(-80.6),{d:1.6,my:1,jump:1},
  run(-90.2),{d:1.3,my:1,slide:1},
  run(-109.5),{d:0.15,my:1,jump:1},{d:0.75,my:1},{d:0.6,my:1,slide:1},
  run(-120.6),{d:0.15,my:1,jump:1},run(-126.6),{d:0.15,my:1,jump:1},
  run(-141.6),{d:1.8,my:1,jump:1},run(-168),{d:2}]},
};
