# Future capabilities

This roadmap describes **unimplemented features**, not the FTFT repair queue.
Current repair status is in [FTFT.md](FTFT.md). None of the entries below is
silently claimed by existing milestones.

| Capability | Current boundary | Future work |
|---|---|---|
| Live origin rebasing | Fixed double absolute origin with compact local float simulation is supported and verified by FTFT5. | Continuous travel through a rebasing region needs an explicit design covering physics, broadphase, contacts, attachments, player, fluids, rendering, interpolation and persistence. It is not currently implemented. |
| Deformable structures / crumple zones | Collision shapes and bodies are rigid. | A separate deformation/material model and budget. |
| High-fidelity cut-cell fluids | P1-C/P1-C-M/P1-PF are protected research prerequisites, not production integration. | Moving geometry and a complete validated liquid/solid solver. |
| Animation, AI and networking | No general feature claim is made here. | Independently scoped game-engine features. |
| Camera render-to-texture / portals | Not supplied by current camera presentation. | A separately scoped rendering feature. |

Small conventional games can use authored flat scenes without planetary or
universe systems. Future large-world work must preserve that option.
