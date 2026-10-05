// Requires registered en/ru catalogs containing lab.named/lab.count and a UI document.
import {localization, ui} from 'judas';
export default class {
 constructor(){this.state={supplementary:false,plural:false,named:false,invalid:false,nul:false,keyLimit:false,textWins:false};this.revision=-1;}
 start(){const doc=ui.get('text_lab');if(doc){doc.get('latin').text='Café e\u0301 😀';this.state.supplementary=doc.get('latin').text==='Café e\u0301 😀';const label=doc.get('latin');label.text='A\0😀';this.state.nul=label.text==='A\0😀';label.textKey='lab.caption';label.text='explicit';this.state.textWins=label.textKey==='';try{label.textKey='x'.repeat(129);}catch{this.state.keyLimit=true;}}
  this.state.named=localization.format('lab.named',{first:'first',second:'second'})==='second — first';
  try{localization.setLocale('not-configured');}catch{this.state.invalid=true;}
  localization.setLocale('ru');
 }
 uiUpdate(){if(this.revision===localization.revision)return;this.revision=localization.revision;
  this.state.plural=localization.format('lab.count',{count:22})==='22 цели';
 }
}
