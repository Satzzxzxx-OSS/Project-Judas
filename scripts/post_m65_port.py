#!/usr/bin/env python3
"""Reproducible consumer import. Originals are read-only; never copy old engines.

Run once into an absent project. Subsequent game-side review edits are deliberate
and must not be overwritten by a regeneration.
"""
from pathlib import Path
import hashlib, json, re, shlex, shutil, subprocess

ROOT = Path(__file__).resolve().parents[1]
P = ROOT / 'projects/post_m65_consumers'
E = ROOT / 'docs/evidence/post_m65_consumers'
SOURCES = {
    'skate': Path('/home/conner/Documents/GitHub/ClaudeJudasSkateGame/Judas/projects/skate_game'),
    'rooftop': Path('/home/conner/Documents/GitHub/ClaudesEdge/RooftopRun_Package'),
    'void': Path('/home/conner/Documents/GitHub/ClaudesEdge/VoidCourier_Package'),
}
def quote(x): return json.dumps(x, ensure_ascii=False)
def key(x): return hashlib.md5(('post-M65 collection:' + x).encode()).hexdigest()
def meta(p, kind):
    i = key(str(p.relative_to(P)))
    Path(str(p)+'.judasmeta').write_text(f'JudasAssetMeta 1\nid "{i}"\ntype {kind}\nsource ""\n')
    return i
def main():
    assert not P.exists(), 'Refuse to overwrite the reviewed port.'
    P.mkdir(parents=True); E.mkdir(parents=True, exist_ok=True)
    provenance = {}; projects = {}; remaps = {}; inputs = {}; ui = {}
    for game, source in SOURCES.items():
        files = [f for d in ['Assets','Scenes'] for f in (source/d).rglob('*') if f.is_file()]
        provenance[game] = {'source':str(source),'files':{str(f.relative_to(source)):hashlib.sha256(f.read_bytes()).hexdigest() for f in files}}
        project = source/('skate_game.judasproj' if game=='skate' else 'game.judasproj')
        projects[game] = {shlex.split(line)[0]:shlex.split(line)[1] for line in project.read_text().splitlines()[1:] if line.strip()}
        shutil.copytree(source/'Assets', P/'Assets'/game)
        shutil.copytree(source/'Scenes', P/'Scenes'/game)
        ids = {}
        for f in (P/'Assets'/game).rglob('*.judasmeta'):
            m = re.search(r'id "([0-9a-f]{32})"',f.read_text())
            if m: ids[m[1]] = key(game+':'+m[1])
        remaps[game] = ids
        # Only textual files: glTF BIN/GLB/audio/image data remains byte-identical.
        for d in [P/'Assets'/game,P/'Scenes'/game]:
            for f in d.rglob('*'):
                if not f.is_file(): continue
                try: s = f.read_text()
                except UnicodeDecodeError: continue
                s = re.sub(r'\b[0-9a-f]{32}\b', lambda m:ids.get(m[0],m[0]),s)
                s = s.replace('Scenes/',f'Scenes/{game}/')
                if f.suffix=='.js':
                    def imported(m):
                        names=[x.strip() for x in m[1].split(',')]
                        if 'input' not in names: return m[0]
                        names.remove('input')
                        return (('import {'+','.join(names)+"} from 'judas';\n") if names else '')+"import {input} from './collection_input.js';"
                    s = re.sub(r"import\s*\{([^}]+)\}\s*from\s*['\"]judas['\"];?",imported,s)
                f.write_text(s)
        facade=P/'Assets'/game/'scripts/collection_input.js'
        facade.write_text("// Preserve this game's logical bindings inside the shared project.\nimport {input as engine} from 'judas';\nconst name=n=>n.startsWith('ui_')?n:'"+game+"_'+n;\nexport const input={axis:n=>engine.axis(name(n)),held:n=>engine.held(name(n)),pressed:n=>engine.pressed(name(n)),released:n=>engine.released(name(n)),get pointerCapture(){return engine.pointerCapture;},set pointerCapture(v){engine.pointerCapture=v;}};\n")
        meta(facade,'script')
        tokens=shlex.split(projects[game]['input-map']); assert tokens.pop(0)=='1'; count=int(tokens.pop(0))
        for _ in range(count):
            name,axis,n=tokens[:3];tokens=tokens[3:]; bindings=[]
            for __ in range(int(n)): bindings.append(tuple(tokens[:3]));tokens=tokens[3:]
            if name.startswith('ui_'):
                entry=ui.setdefault(name,[axis,[]]);entry[1].extend(b for b in bindings if b not in entry[1])
            else: inputs[game+'_'+name]=[axis,bindings]
        assert not tokens
    common=P/'Assets/collection';common.mkdir()
    script=common/'collection.js'
    script.write_text("""// Collection navigation is project content, not an engine game mode.
import {input,scenes,ui,world} from 'judas';
export const properties={launcher:{type:'boolean',default:false}};
const scenesByKey={collection_skate:'Scenes/skate/park.judas',collection_rooftop:'Scenes/rooftop/rooftops.judas',collection_void:'Scenes/void/system.judas',collection_menu:'Scenes/launcher.judas'};
export default class {
 constructor({properties}){this.launcher=properties.launcher;}
 start(){if(this.launcher){input.pointerCapture=false;ui.debugOverlayVisible=false;world.setView({position:{x:0,y:1,z:5},rotation:{w:1,x:0,y:0,z:0}},70);}}
 uiUpdate(){for(const [action,scene] of Object.entries(scenesByKey))if(input.pressed(action)){scenes.load(scene);return;}}
 onUI(e){if(e.type==='click'&&e.document==='collection'){const scene=scenesByKey['collection_'+e.element];if(scene)scenes.load(scene);if(e.element==='quit')ui.quit();}}
 destroy(){if(this.launcher)world.clearView();}
}
""")
    scriptid=meta(script,'script')
    font=next((P/'Assets/skate').rglob('*.ttf'))
    fontid=re.search(r'id "([0-9a-f]{32})"',Path(str(font)+'.judasmeta').read_text())[1]
    layout={'kind':'ui','reference':[1280,720],'elements':[
        {'id':'root','kind':'canvas','relativeSize':[1,1],'background':[.04,.06,.10,.99]},
        {'id':'title','parent':'root','kind':'text','anchorMin':[.5,.1],'align':[.5,0],'size':[900,65],'text':'JUDAS | THREE REAL GAMES','font':fontid,'fontSize':34,'textAlign':[.5,.5]},
        {'id':'subtitle','parent':'root','kind':'text','anchorMin':[.5,.22],'align':[.5,0],'size':[960,70],'text':'Post-M65 consumer review\nF1 Skate   F2 Rooftop Run   F3 Void Courier   F4 Return here','font':fontid,'fontSize':20,'textAlign':[.5,.5]},
    ]}
    for i,(ident,label) in enumerate([('skate','SKATE  |  tricks, grinds and a streamed street'),('rooftop','ROOFTOP RUN  |  parkour time trial'),('void','VOID COURIER  |  planets, flight and combat'),('quit','QUIT')]):
        layout['elements'].append({'id':ident,'parent':'root','kind':'button','anchorMin':[.5,.36+i*.13],'align':[.5,0],'size':[760,72],'text':label,'font':fontid,'fontSize':24,'textAlign':[.5,.5],'background':[.14,.22,.34,1]})
    for element in layout['elements']:
        if 'anchorMin' in element: element['anchorMax']=element['anchorMin']
    structured=common/'menu.author.json';structured.write_text(json.dumps(layout,indent=2)+'\n')
    subprocess.run([str(ROOT/'build/judas_scene_author'),'--structured',str(structured),str(common/'menu.judasui')],check=True)
    uid=meta(common/'menu.judasui','ui')
    def controller(launcher=False):
        return f'\nobject 1000000 "Collection navigation (F1-F4)"\n  position 0 0 0\n  rotation 1 0 0 0\n  scale 1 1 1\n  scripts 1\n  script.0.id 1\n  script.0.asset "{scriptid}"\n  script.0.enabled true\n  script.0.properties '+quote(json.dumps({'launcher':launcher}))+'\n'+(f'  ui.asset "{uid}"\n  ui.name "collection"\n  ui.enabled true\n' if launcher else '')+'end\n'
    for game,start in [('skate','park'),('rooftop','rooftops'),('void','system')]:
        scene=P/f'Scenes/{game}/{start}.judas';s=scene.read_text();s=re.sub(r'next-id \d+','next-id 1000001',s);scene.write_text(s+controller())
    (P/'Scenes/launcher.judas').write_text('JudasScene 3\nsettings\n  name "Three Games"\n  world-origin 0 0 0\n  sun-direction 0 1 0\n  sun-color 1 1 1\n  ambient .3 .3 .3\n  fluid-scale 13\n  fidelity-policy none\n  next-id 1000001\n  background-color 0.04 0.06 0.10\nend\n'+controller(True))
    inputs.update(ui)
    for i,name in enumerate(['skate','rooftop','void','menu'],1): inputs['collection_'+name]=['0',[(f'key:F{i}','1','0')]]
    imap='1 '+str(len(inputs))+' '
    for name,(axis,bindings) in inputs.items(): imap+=quote(name)+' '+axis+' '+str(len(bindings))+' '+''.join(quote(b[0])+' '+b[1]+' '+b[2]+' ' for b in bindings)
    config={'name':'Judas Three Games','startup-scene':'Scenes/launcher.judas','assets-dir':'Assets','scenes-dir':'Scenes','saves-dir':'Saves','legacy-gameplay':'false','input-map':imap}
    for name in ['classification','audio-groups','world-manifest','localization']:
        config[name]=re.sub(r'\b[0-9a-f]{32}\b',lambda m:remaps['skate'].get(m[0],m[0]),projects['skate'][name])
    export=['Scenes/launcher.judas','Scenes/skate/park.judas','Scenes/rooftop/rooftops.judas','Scenes/void/system.judas']+[f'Scenes/skate/street-{i}.judas' for i in range(5)]
    config['export-scenes']=' '.join(quote(x) for x in export)
    (P/'post_m65_consumers.judasproj').write_text('JudasProject 1\n'+''.join(name+' '+quote(value)+'\n' for name,value in config.items()))
    provenance['asset_id_remapping']=remaps
    (E/'import-provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
    for src,name in [('/home/conner/Documents/GitHub/ClaudeJudasSkateGame/SKATE_GAME_ENGINE_AUDIT.md','skate-original-audit.md'),('/home/conner/Documents/GitHub/ClaudeJudasSkateGame/audit/LATEST_ENGINE_FOLLOWUP.md','skate-m64-followup.md'),('/home/conner/Documents/GitHub/ClaudesEdge/notes/REPORT.md','rooftop-original-report.md'),('/home/conner/Documents/GitHub/ClaudesEdge/notes/SPACE_REPORT.md','void-original-report.md')]:shutil.copyfile(src,E/name)
    print(P)
if __name__=='__main__':main()
