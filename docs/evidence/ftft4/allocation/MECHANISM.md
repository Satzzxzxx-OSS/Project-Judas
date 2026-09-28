# FTFT4A-P bounded allocation/lifetime pass

## Observed sites and ownership

`ContactSolver::Clear` destroys all unordered-map nodes. `RegisterFrame`
allocates one replacement node per distinct contacting body on every step:
1,501 on the crate floor. Frame and constraint vectors already retain their
capacity. The map's pointer keys and frame values are valid for one solve
only; carrying those keys or derived values across generations is unsafe.

Use a solver-owned C++17 unsynchronized pool for node **storage**, while
still clearing every key/value every solve. Map nodes are returned to that
pool at Clear and rebuilt with current bodies; numerical frames are not
reused across steps. Pool, upstream counter, map destruction order is
explicit. No pool release is allowed while the map owns buckets/nodes.
Retained memory follows the maximum simultaneous node demand plus pool
chunk slack, not the number of elapsed steps; chunks are limited to 256
blocks and only blocks up to 64 bytes are pooled. Bucket growth and vector
growth remain genuine allocations. Pool storage is released with the solver.

`PhysicsWorld::Orientation` currently destroys prepared compound bounds on
any exact quaternion change. `PrepareShapeBounds` then allocates the same
child-vector size again. Retain that vector but invalidate its contents;
rebuild into it before use with precisely the same arithmetic. No consumers
retain child-vector references: bounds are returned by value, and contacts
hold parent orientation data only. Body shape is immutable for a slot
generation. Replacement/reuse resets the complete Body and its preparation.

Exact expansion vectors, lazy exact-pair objects, clipped manifold vectors
and fixture/oracle result sets have different lifetimes; this pass does not
pool or alter them. Diagnostics distinguish all-heap phase counters from
frame/compound cache growth and the unchanged lazy predicate work.

No numerical expression, threshold, contact order, anchor, friction rule or
impact timing is changed. The previous candidate and its evidence remain
in `../performance/`; `before-source.zip` preserves the starting source.
