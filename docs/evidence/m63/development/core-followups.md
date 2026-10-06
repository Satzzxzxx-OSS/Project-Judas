# Initial focused fixture corrections

The first core fixture called PhysicsWorld queries without Init(). Native gdb
located the crash in QueryBodiesInAabbInto. The fixture now initializes the
ordinary world; no runtime null-world behavior was changed to mask it.

The next run passed the physical threshold checks (5 N intact; 400 N failed at
4x4 and 8x6) but its rotated trajectory assertion used 1e-7 m despite GravityField
sampling being float. The follow-up prints the actual deviation and uses the
existing M62 1e-6 m comparison tolerance. The original logs remain present.
