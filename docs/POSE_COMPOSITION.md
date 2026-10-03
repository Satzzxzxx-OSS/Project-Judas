# M47 pose composition

Current public JavaScript signatures, examples and lifetime rules: [JudasJS reference](JUDASJS.md). This document retains milestone architecture and evidence context.

Skeleton/clip assets remain immutable and shared. Each RuntimeWorld animation instance owns its playback clocks, mixer, ordered layers, external contributions, source pose and final resolved local pose. The Renderer only consumes the resulting skin palette.

The base mixer advances all live clip contributors. Crossfades use normalized weighted TRS sampling with shortest-path normalized quaternion interpolation. An interrupted fade retains the current weighted contributors and their clocks. Zero-duration fades replace immediately; completed sources retire. At most 16 interrupted outgoing contributors are supported; further interruptions fail explicitly until the transition finishes.

Authored layers resolve in list order after the base. Later override layers interpolate from the accumulated local pose. Empty masks affect all joints; explicit masks change only selected local transforms, while normal parent hierarchy still carries descendants. Stable keys are escaped imported node-name hierarchy paths; unique leaf names are accepted. Missing/ambiguous names fail. Renaming a source joint requires updating its mask; there is no retargeting.

Additive contributions explicitly compare a clip sample to rest or a named clip at referenceTime: local translation difference, relative quaternion rotation, multiplicative scale ratio. Zero reference scale is rejected. External non-clip producers submit copied validated PoseContribution values through RuntimeWorld::SetPoseContribution. They follow authored layers, sorted by priority and then source name. Removing a source resolves back to the other contributors. SetFinalPose remains a compatibility wrapper that submits an external contribution rather than bypassing resolution.

JavaScript: entity.animation.crossFade(clip,seconds), .layer(id,{clip,weight,enabled,mask,additive,referenceClip,referenceTime,speed,time}), .removeLayer(id), .info.transitioning/.transitionFraction/.joints/.layers. Existing playback APIs remain. Pause holds all clip clocks. Layer updates preserve an existing same-clip clock; use a new layer/clip to restart. Runtime layers do not rewrite authored state.

The animation inspector adds editable initial layers/masks/reference samples. Generic scene and prefab property serialization carries these fields. Optional authored layers contribute a tagged fingerprint extension under the existing schema 5; no-layer M46 data remains unchanged.

Demo: projects/pose_demo/pose_demo.judasproj. G crossfades, J toggles a tip-only Stretch layer, K toggles an additive elbow Wave offset, C changes speed, P spawns an independent prefab. No blend trees, state machines, IK, root-motion gameplay or active physics.
