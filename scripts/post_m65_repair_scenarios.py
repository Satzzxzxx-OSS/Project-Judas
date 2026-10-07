#!/usr/bin/env python3
"""Reuse the accepted consumer fixtures, always writing NEW repair evidence."""
from pathlib import Path
import importlib.util,os,sys,re,json,shutil
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('review',ROOT/'scripts/post_m65_review.py');review=importlib.util.module_from_spec(spec);spec.loader.exec_module(review)
review.E=ROOT/'docs/evidence/post_m65_repairs/scenarios'
original=review.run
MODE=sys.argv[1]
def run(name,p,scene,events,frames,env,extra=None):
    extra=dict(extra or {})
    if os.getenv("JUDAS_STREAM_UNIT_TRACE"):extra["JUDAS_STREAM_UNIT_TRACE"]="1"
    if MODE.startswith('skate-ragdolls'):
        extra['JUDAS_SLEEP_TRACE']='1'
        extra['JUDAS_RESOURCE_MODE']='blocking' # Isolate physics; wait for the same imported rig before advancing its synthetic clock.
        # Preserve every authored rig/body/constraint parameter. Only select
        # the requested instance count from the identical 20-rig fixture.
        count=int(os.getenv('JUDAS_REPAIR_RIG_COUNT','20'));assert count in (1,10,20)
        text=scene.read_text()
        for i in range(count,20):
            match=review.block(text,1000200+i);assert match;text=text[:match.start()]+text[match.end():]
        scene.write_text(text)
        names=re.findall(r'  ragdoll.bone.\d+.joint "([^"]+)"',review.block(text,12)[0])
        crowd=p/'Assets/skate/scripts/crowd.js';source=crowd.read_text()
        code="""if(this.dropped&&Math.floor(time.elapsed)!==this.lastProbe){this.lastProbe=Math.floor(time.elapsed);this.state.bones=NAMES.map(key=>{const b=this.e.ragdoll.body(key);return b?.valid?{key,sleeping:b.sleeping,v:b.velocity,w:b.angularVelocity}:null;});}""".replace('NAMES',json.dumps(names))
        source=source.replace('  update() {','  update() {\n    '+code,1);crowd.write_text(source)
    result=original(name,p,scene,events,frames,env,extra)
    if MODE.startswith('skate-ragdolls'):shutil.copyfile(p/'Assets/skate/scripts/crowd.js',review.E/name/'crowd_probe.js')
    return result
review.run=run
review.main()
