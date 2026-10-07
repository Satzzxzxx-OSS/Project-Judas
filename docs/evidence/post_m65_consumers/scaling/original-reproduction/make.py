import sys
n=int(sys.argv[1]); scripted=sys.argv[2]=='1'
L=['JudasScene 3','settings','  name "repro"','  world-origin 0 0 0','  sun-direction 0 1 0','  sun-color 1 1 1','  ambient 0.3 0.3 0.3','  fluid-scale 13','  fluid-update-rate-hz 20','  fluid-hydrostatic-drag-rate 2','  fidelity-policy none',f'  next-id {n+10}','end','']
if scripted:
    L+=['object 1 "Scripted"','  position 0 0 0','  rotation 1 0 0 0','  scale 1 1 1','  scripts 1','  script.0.id 1','  script.0.asset "dddd0000000000000000000000000001"','  script.0.enabled true','  script.0.properties "{}"','end','']
for i in range(n):
    L+=[f'object {i+5} "Box"',f'  position {i%20} 0 {i//20}','  rotation 1 0 0 0','  scale 1 1 1','  render box','  render.half-extents 0.2 0.2 0.2','  render.radius 0.5','  render.color 1 1 1','  render.alpha 1','  render.secondary-color 0.8 0.8 0.8','  render.secondary-alpha 1','  render.mesh-asset ""','  render.texture-asset ""','end','']
open('Scenes/main.judas','w').write('\n'.join(L))
