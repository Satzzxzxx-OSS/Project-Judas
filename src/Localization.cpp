#include "Localization.h"
#include "ResourceManager.h"
#include "AssetDatabase.h"
#include "PerformanceProfiler.h"
#include <unicode/locid.h>
#include <unicode/msgfmt.h>
#include <unicode/messagepattern.h>
#include <unicode/numfmt.h>
#include <unicode/ustring.h>
#include <unicode/uloc.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
namespace {
bool LocaleValid(const std::string& tag){if(tag.empty()||tag.size()>64)return false;UErrorCode e=U_ZERO_ERROR;auto l=icu::Locale::forLanguageTag(tag,e);return U_SUCCESS(e)&&!l.isBogus()&&l.toLanguageTag<std::string>(e)==tag&&U_SUCCESS(e);}
icu::UnicodeString US(const std::string& s){return icu::UnicodeString::fromUTF8(s);}
std::string UTF8(const icu::UnicodeString& s){std::string out;s.toUTF8String(out);return out;}
bool PatternArguments(const std::string& s,std::map<std::string,int>& arguments,std::string& error){
 UErrorCode e=U_ZERO_ERROR;icu::MessagePattern mp(US(s),nullptr,e);
 if(U_FAILURE(e)){error=u_errorName(e);return false;}
 for(int i=0;i<mp.countParts();++i)if(mp.getPartType(i)==UMSGPAT_PART_TYPE_ARG_START){
  if(mp.getPartType(i+1)!=UMSGPAT_PART_TYPE_ARG_NAME){error="named message arguments required";return false;}
  auto type=mp.getPart(i).getArgType();auto name=UTF8(mp.getSubstring(mp.getPart(i+1)));int need=0;
  if(type==UMSGPAT_ARG_TYPE_PLURAL||type==UMSGPAT_ARG_TYPE_SELECTORDINAL)need=1;
  else if(type==UMSGPAT_ARG_TYPE_SELECT)need=2;
  else if(type==UMSGPAT_ARG_TYPE_SIMPLE){if(UTF8(mp.getSubstring(mp.getPart(i+2)))!="number"){error="supported typed argument is number";return false;}need=1;}
  else if(type==UMSGPAT_ARG_TYPE_CHOICE){error="use named plural/select instead of legacy choice";return false;}
  auto& previous=arguments[name];if(previous&&need&&previous!=need){error="incompatible uses of argument "+name;return false;}previous=std::max(previous,need);
 }return true;
}
bool PatternValid(const std::string& s,const std::string& locale,std::string& error){UErrorCode e=U_ZERO_ERROR;UParseError p{};auto l=icu::Locale::forLanguageTag(locale,e);icu::MessageFormat fmt(US(s),l,p,e);if(U_FAILURE(e)){error="ICU MessageFormat at UTF-16 offset "+std::to_string(p.offset)+": "+u_errorName(e);return false;}std::map<std::string,int> arguments;return PatternArguments(s,arguments,error);}
}
bool ProjectLocalization::Validate(std::string& error)const{
 if(!LocaleValid(defaultLocale)||locales.size()>32||fonts.size()>16||(!locales.empty()&&!locales.count(defaultLocale))){error="invalid localization default/count";return false;}
 for(auto& id:fonts)if(!IsValidAssetId(id)){error="invalid fallback font ID";return false;}
 for(auto& [locale,entry]:locales){if(!LocaleValid(locale)||!IsValidAssetId(entry.catalog)||entry.fonts.size()>16||(!entry.fallback.empty()&&!locales.count(entry.fallback))){error="invalid locale/catalog/fallback "+locale;return false;}for(auto& f:entry.fonts)if(!IsValidAssetId(f)){error="invalid locale font ID";return false;}
  std::set<std::string> visited;std::string at=locale;while(!at.empty()){if(!visited.insert(at).second){error="locale fallback cycle";return false;}at=locales.at(at).fallback;}
 }error.clear();return true;
}
std::string ProjectLocalization::Encode()const{std::ostringstream s;s.imbue(std::locale::classic());s<<1<<' '<<std::quoted(defaultLocale)<<' '<<fonts.size();for(auto& f:fonts)s<<' '<<std::quoted(f);s<<' '<<locales.size();for(auto& [tag,e]:locales){s<<' '<<std::quoted(tag)<<' '<<std::quoted(e.catalog)<<' '<<std::quoted(e.fallback)<<' '<<e.fonts.size();for(auto& f:e.fonts)s<<' '<<std::quoted(f);}return s.str();}
bool ProjectLocalization::Parse(const std::string& text,ProjectLocalization& out,std::string& error){std::istringstream s(text);s.imbue(std::locale::classic());ProjectLocalization c;int v=0;size_t fonts=0,n=0;if(!(s>>v>>std::quoted(c.defaultLocale)>>fonts)||v!=1||fonts>16){error="malformed localization configuration";return false;}for(size_t i=0;i<fonts;++i){std::string id;if(!(s>>std::quoted(id))){error="malformed fallback font list";return false;}c.fonts.push_back(id);}if(!(s>>n)||n>32){error="malformed locale count";return false;}for(size_t i=0;i<n;++i){std::string tag;LocaleEntry e;if(!(s>>std::quoted(tag)>>std::quoted(e.catalog)>>std::quoted(e.fallback)>>fonts)||fonts>16||c.locales.count(tag)){error="malformed/duplicate locale";return false;}for(size_t j=0;j<fonts;++j){std::string id;if(!(s>>std::quoted(id))){error="malformed fallback font list";return false;}e.fonts.push_back(id);}c.locales.emplace(tag,std::move(e));}s>>std::ws;if(!s.eof()){error="trailing localization configuration data";return false;}if(!c.Validate(error))return false;out=std::move(c);return true;}
bool ParseCatalog(const std::string& text,Catalog& out,std::string& error){
 if(text.size()>2*1024*1024||!ValidTextUTF8(text,error)){error="catalog UTF-8/size: "+error;return false;}std::istringstream s(text);s.imbue(std::locale::classic());std::string magic;int version=0;Catalog c;if(!(s>>magic>>version>>std::quoted(c.locale))||magic!="JudasCatalog"||version!=1||!LocaleValid(c.locale)){error="invalid catalog header/BCP47 locale";return false;}
 while(true){s>>std::ws;if(s.eof())break;std::string key,message;if(!(s>>std::quoted(key)>>std::quoted(message))||key.empty()||key.size()>128||key.find_first_of("\r\n\0",0,3)!=std::string::npos||message.size()>16384||c.messages.size()>=4096||c.messages.count(key)){error="malformed/duplicate catalog key "+key;return false;}if(!PatternValid(message,c.locale,error)){error=key+": "+error;return false;}c.messages.emplace(std::move(key),std::move(message));}
 out=std::move(c);error.clear();return true;
}
bool LoadCatalog(const std::string& path,Catalog& out,std::string& error){std::ifstream f(path,std::ios::binary);if(!f){error="cannot read catalog "+path;return false;}return ParseCatalog(std::string(std::istreambuf_iterator<char>(f),{}),out,error);}
bool ValidateLocalizationAssets(const ProjectLocalization& c,const AssetDatabase& assets,std::string& error){if(!c.Validate(error))return false;auto check=[&](const std::string& id,AssetType type){auto* a=assets.Find(id);if(!a||a->missing||a->type!=type){error="missing/wrong-type localization asset "+id;return false;}return true;};for(auto& id:c.fonts)if(!check(id,AssetType::Font))return false;for(auto& [locale,e]:c.locales){if(!check(e.catalog,AssetType::Catalog))return false;Catalog cat;if(!LoadCatalog(assets.Find(e.catalog)->path,cat,error)||cat.locale!=locale){error="catalog locale disagrees with project: "+locale+" "+error;return false;}for(auto& f:e.fonts)if(!check(f,AssetType::Font))return false;}return true;}
struct LocalizationSession::Impl {
 ProjectLocalization config;ResourceManager* resources=nullptr;std::string current,pending;uint64_t revision=1;std::map<std::string,std::shared_ptr<const Catalog>> catalogs;std::set<std::string> refs;std::set<std::string> reported;
 struct Compiled {std::string pattern,locale;std::unique_ptr<icu::MessageFormat> value;std::map<std::string,int> arguments;};std::map<std::string,Compiled> messages;std::map<std::string,std::unique_ptr<icu::NumberFormat>> numbers;
 explicit Impl(ProjectLocalization c):config(std::move(c)),current(config.defaultLocale){}
 std::vector<std::string> Chain(const std::string& locale)const {std::vector<std::string> result;std::set<std::string> seen;auto add=[&](std::string tag){while(config.locales.count(tag)&&seen.insert(tag).second){result.push_back(tag);tag=config.locales.at(tag).fallback;}};add(locale);auto parent=locale;while(parent.find('-')!=std::string::npos){parent=parent.substr(0,parent.rfind('-'));add(parent);}add(config.defaultLocale);return result;}
 std::vector<std::string> FontIDs(const std::string& locale)const {std::vector<std::string> out;auto it=config.locales.find(locale);if(it!=config.locales.end())out=it->second.fonts;out.insert(out.end(),config.fonts.begin(),config.fonts.end());return out;}
 bool Prepare(const std::string& locale){if(!resources)return config.locales.empty();bool ready=true;std::map<std::string,std::shared_ptr<const Catalog>> next;
  for(auto& tag:Chain(locale)){auto& id=config.locales.at(tag).catalog;if(refs.insert(id).second)resources->AddRef(id);resources->RequestCatalog(id);auto c=resources->TryGetCatalog(id);if(!c||c->locale!=tag){if((c||resources->StateOf(id)==ResourceState::Failed)&&reported.size()<16&&reported.insert(id).second)std::fprintf(stderr,"Localization catalog %s: %s\n",id.c_str(),c?"header locale disagrees with configured tag":resources->ErrorOf(id).c_str());ready=false;continue;}next[tag]=c;}
  for(auto& id:FontIDs(locale)){if(refs.insert(id).second)resources->AddRef(id);resources->RequestFont(id);if(!resources->TryGetFont(id))ready=false;}
  if(ready){bool changed=false;for(auto& [tag,c]:next){auto it=catalogs.find(tag);if(it==catalogs.end()||it->second!=c){catalogs[tag]=c;changed=true;}}if(changed){messages.clear();++revision;}}return ready;
 }

};
LocalizationSession::LocalizationSession(ProjectLocalization c):m(std::make_unique<Impl>(std::move(c))){}LocalizationSession::~LocalizationSession(){if(m->resources)for(auto& id:m->refs)m->resources->ReleaseRef(id);}
void LocalizationSession::Bind(ResourceManager* r){if(r==m->resources)return;if(m->resources)for(auto& id:m->refs)m->resources->ReleaseRef(id);m->resources=r;m->refs.clear();m->catalogs.clear();m->messages.clear();m->numbers.clear();++m->revision;}
void LocalizationSession::Reload(){m->reported.clear();if(m->resources){for(auto& tag:m->Chain(m->current))m->resources->Invalidate(m->config.locales.at(tag).catalog);}Refresh();}
void LocalizationSession::Refresh(){auto desired=m->pending.empty()?m->current:m->pending;if(m->Prepare(desired)&&!m->pending.empty()){m->current=m->pending;m->pending.clear();m->messages.clear();m->numbers.clear();++m->revision;}}
bool LocalizationSession::SetLocale(const std::string& tag,std::string& error){if(!m->config.locales.count(tag)){error="locale not configured: "+tag;return false;}m->pending=tag;Refresh();error.clear();return true;}
const std::string& LocalizationSession::Locale()const{return m->current;}const ProjectLocalization& LocalizationSession::Configuration()const{return m->config;}uint64_t LocalizationSession::Revision()const{return m->revision;}
bool LocalizationSession::RTL()const{UErrorCode e=U_ZERO_ERROR;auto l=icu::Locale::forLanguageTag(m->current,e);return U_SUCCESS(e)&&uloc_isRightToLeft(l.getName());}
std::vector<std::shared_ptr<const TextFont>> LocalizationSession::Fonts(){Refresh();std::vector<std::shared_ptr<const TextFont>> out;if(m->resources)for(auto& id:m->FontIDs(m->current))if(auto f=m->resources->TryGetFont(id))out.push_back(f);return out;}
std::string LocalizationSession::Format(const std::string& key,const MessageArguments& args,std::string& error){
 JUDAS_PROFILE_SCOPE("Localization format");Refresh();if(key.empty()||key.size()>128||args.size()>32){error="message argument/key limits";return "["+key+"]";}
 std::string pattern,locale;for(auto& tag:m->Chain(m->current)){auto c=m->catalogs.find(tag);if(c==m->catalogs.end())continue;auto p=c->second->messages.find(key);if(p!=c->second->messages.end()){pattern=p->second;locale=tag;break;}}
 // Catalogs publish asynchronously; until the whole chain is published a lookup miss is
 // not evidence that the key is absent, so report it distinctly and without a warning.
 if(locale.empty()){bool published=true;for(auto& tag:m->Chain(m->current))if(!m->catalogs.count(tag))published=false;
  if(!published){error="localization catalog not yet published for key "+key;return "["+key+"]";}
  error="missing localization key "+key;static unsigned warnings=0;if(warnings++<16)std::fprintf(stderr,"Localization: %s\n",error.c_str());return "["+key+"]";}
 auto it=m->messages.find(key);if(it==m->messages.end()||it->second.pattern!=pattern||it->second.locale!=locale){if(m->messages.size()>=512)m->messages.clear();Impl::Compiled c;c.pattern=pattern;c.locale=locale;UErrorCode e=U_ZERO_ERROR;auto l=icu::Locale::forLanguageTag(locale,e);c.value=std::make_unique<icu::MessageFormat>(US(pattern),l,e);if(!PatternArguments(pattern,c.arguments,error))return "["+key+"!]";
  if(U_FAILURE(e)){error=u_errorName(e);return "["+key+"!]";}it=m->messages.insert_or_assign(key,std::move(c)).first;
 }
 for(auto& [name,type]:it->second.arguments){auto a=args.find(name);if(a==args.end()||(type==1&&!std::holds_alternative<double>(a->second))||(type==2&&!std::holds_alternative<std::string>(a->second))){error="missing/wrong message argument "+name;return "["+key+"!]";}}
 std::vector<icu::UnicodeString> names;std::vector<icu::Formattable> values;for(auto& [name,value]:args){names.push_back(US(name));if(auto* n=std::get_if<double>(&value)){if(!std::isfinite(*n)){error="nonfinite message number";return "["+key+"!]";}values.emplace_back(*n);}else {const auto& s=std::get<std::string>(value);if(s.size()>16384||!ValidTextUTF8(s,error)){error="invalid message string";return "["+key+"!]";}values.emplace_back(US(s));}}
 UErrorCode e=U_ZERO_ERROR;icu::UnicodeString result;it->second.value->format(names.data(),values.data(),int32_t(values.size()),result,e);if(U_FAILURE(e)){error=u_errorName(e);return "["+key+"!]";}auto out=UTF8(result);if(out.size()>65536){error="formatted message too long";return "["+key+"!]";}error.clear();return out;
}
std::string LocalizationSession::Number(double value,const NumberOptions& options,std::string& error){if(!std::isfinite(value)||options.minimumFraction<0||options.maximumFraction>12||options.maximumFraction<options.minimumFraction){error="invalid number options";return {};}
 auto key=m->current+char(options.minimumFraction)+char(options.maximumFraction)+char(options.grouping);if(!m->numbers.count(key)&&m->numbers.size()>=64)m->numbers.clear();auto& p=m->numbers[key];if(!p){UErrorCode e=U_ZERO_ERROR;auto locale=icu::Locale::forLanguageTag(m->current,e);p.reset(icu::NumberFormat::createInstance(locale,e));if(U_FAILURE(e)){error=u_errorName(e);return {};}p->setMinimumFractionDigits(options.minimumFraction);p->setMaximumFractionDigits(options.maximumFraction);p->setGroupingUsed(options.grouping);}icu::UnicodeString s;p->format(value,s);error.clear();return UTF8(s);}
