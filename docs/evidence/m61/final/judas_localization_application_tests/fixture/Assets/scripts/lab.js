import {ui, input, localization, scenes} from 'judas';
export default class {
 constructor(){this.revision=-1;this.state={count:22};}
 start(){ui.debugOverlayVisible=false;input.pointerCapture=false;}
 uiUpdate(){const d=ui.get('text_lab');if(this.revision===localization.revision)return;this.revision=localization.revision;
  d.get('caption').direction=localization.direction;
  d.get('info').text=`${localization.locale} / ${localization.direction} / revision ${this.revision} · DejaVu → Noto Arabic / Devanagari / JP · demo translations`;
  d.get('dynamic').text=localization.format('lab.count',{count:this.state.count})+' · '+localization.number(1234567.89)+' · '+localization.format('lab.fallback');
 }
 onUI(e){if(e.type!=='click')return;if(e.element==='reload')localization.reload();else localization.setLocale(e.element);}
}
