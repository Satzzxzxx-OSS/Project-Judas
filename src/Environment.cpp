#include "Environment.h"
#include "SceneFingerprint.h"
#include "PerformanceProfiler.h"
#include "../third_party/stb_image.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstring>
#include <glm/gtc/constants.hpp>
namespace {
float Radical(unsigned bits){bits=(bits<<16)|(bits>>16);bits=((bits&0x55555555u)<<1)|((bits&0xAAAAAAAAu)>>1);bits=((bits&0x33333333u)<<2)|((bits&0xCCCCCCCCu)>>2);bits=((bits&0x0F0F0F0Fu)<<4)|((bits&0xF0F0F0F0u)>>4);bits=((bits&0x00FF00FFu)<<8)|((bits&0xFF00FF00u)>>8);return float(bits)*2.3283064365386963e-10f;}
glm::vec3 Direction(float u,float v){float phi=u*glm::two_pi<float>(),theta=v*glm::pi<float>();return {sin(theta)*cos(phi),cos(theta),sin(theta)*sin(phi)};}
glm::mat3 Basis(glm::vec3 n){auto t=glm::normalize(glm::cross(abs(n.y)<.99f?glm::vec3(0,1,0):glm::vec3(1,0,0),n));return {t,glm::cross(n,t),n};}
glm::vec3 Sample(const EnvironmentLevel& l,glm::vec3 d){float u=atan2(d.z,d.x)/glm::two_pi<float>();u-=floor(u);float v=acos(glm::clamp(d.y,-1.f,1.f))/glm::pi<float>();float x=u*l.width-.5f,y=v*l.height-.5f;int ix=int(floor(x)),iy=int(floor(y));auto at=[&](int a,int b){a=(a%int(l.width)+int(l.width))%int(l.width);b=glm::clamp(b,0,int(l.height)-1);return l.pixels[size_t(b)*l.width+a];};return glm::mix(glm::mix(at(ix,iy),at(ix+1,iy),x-floor(x)),glm::mix(at(ix,iy+1),at(ix+1,iy+1),x-floor(x)),y-floor(y));}
glm::vec3 GGX(float x,float y,float rough){float a=std::max(.0025f,rough*rough),phi=glm::two_pi<float>()*x,c=sqrt((1-y)/(1+(a*a-1)*y)),s=sqrt(std::max(0.f,1-c*c));return {cos(phi)*s,sin(phi)*s,c};}
void U32(std::ostream& s,unsigned v){for(int k=0;k<4;++k)s.put(char((v>>(8*k))&255));}
void Float(std::ostream& s,float v){unsigned u;static_assert(sizeof(u)==sizeof(v));memcpy(&u,&v,4);U32(s,u);}
}
bool BakeEnvironment(const std::string& path,unsigned width,unsigned samples,EnvironmentData& out,std::string& error){
 JUDAS_PROFILE_SCOPE("Environment offline bake");
 if(width<16||width>512||(width&(width-1))||samples<32||samples>1024){error="environment width must be power-of-two 16..512, samples 32..1024";return false;}
 std::ifstream file(path,std::ios::binary);std::string source{std::istreambuf_iterator<char>(file),{}};
 if(source.empty()){error="cannot read HDR environment "+path;return false;}
 stbi_set_flip_vertically_on_load_thread(0);int w,h,c;float* rgb=stbi_loadf_from_memory(reinterpret_cast<const unsigned char*>(source.data()),int(source.size()),&w,&h,&c,3);
 if(!rgb){error="Radiance HDR decode failed";return false;}EnvironmentLevel original;original.width=w;original.height=h;original.pixels.resize(size_t(w)*h);bool valid=true;
 for(size_t i=0;i<original.pixels.size();++i){auto v=glm::vec3(rgb[i*3],rgb[i*3+1],rgb[i*3+2]);for(int k=0;k<3;++k)valid&=std::isfinite(v[k])&&v[k]>=0&&v[k]<=60000;original.pixels[i]=v;}stbi_image_free(rgb);if(!valid){error="environment radiance must be finite and within 0..60000 (RGB16F)";return false;}
 EnvironmentData result;result.sourceHash=SceneFingerprintSha256("Judas.EnvironmentBake.1:"+std::to_string(width)+":"+std::to_string(samples)+":"+source);
 unsigned levels=0;for(unsigned n=width;n;n>>=1)++levels;
 for(unsigned level=0;level<levels;++level){EnvironmentLevel map;map.width=std::max(1u,width>>level);map.height=std::max(1u,map.width/2);map.pixels.resize(size_t(map.width)*map.height);float rough=float(level)/(levels-1);
  for(unsigned y=0;y<map.height;++y)for(unsigned x=0;x<map.width;++x){auto n=Direction((x+.5f)/map.width,(y+.5f)/map.height);glm::vec3 total(0);float sum=0;auto basis=Basis(n);if(!level){total=Sample(original,n);sum=1;}else for(unsigned i=0;i<samples;++i){auto half=basis*GGX(float(i)/samples,Radical(i),rough);auto light=2*glm::dot(n,half)*half-n;float cosine=std::max(0.f,glm::dot(n,light));total+=Sample(original,light)*cosine;sum+=cosine;}map.pixels[size_t(y)*map.width+x]=total/std::max(sum,1e-6f);}
  result.specular.push_back(std::move(map));
 }
 auto& diffuse=result.diffuse;diffuse.width=32;diffuse.height=16;diffuse.pixels.resize(512);
 for(unsigned y=0;y<16;++y)for(unsigned x=0;x<32;++x){auto n=Direction((x+.5f)/32,(y+.5f)/16);auto basis=Basis(n);glm::vec3 total(0);for(unsigned i=0;i<samples;++i){float u=float(i)/samples,v=Radical(i),r=sqrt(u),phi=glm::two_pi<float>()*v;auto l=basis*glm::vec3(r*cos(phi),r*sin(phi),sqrt(1-u));total+=Sample(original,l);}diffuse.pixels[y*32+x]=total/float(samples);}
 result.brdfSize=64;result.brdf.resize(4096);
 for(unsigned y=0;y<64;++y)for(unsigned x=0;x<64;++x){float nv=(x+.5f)/64,rough=(y+.5f)/64;glm::vec3 v(sqrt(1-nv*nv),0,nv);glm::vec2 sum(0);for(unsigned i=0;i<samples;++i){auto half=GGX(float(i)/samples,Radical(i),rough);auto l=2*glm::dot(v,half)*half-v;float nl=std::max(0.f,l.z),nh=std::max(0.f,half.z),vh=std::max(0.f,glm::dot(v,half));if(nl>0){float a=rough*rough,a2=a*a;float visibility=2*nl*nv/std::max(float(nl*sqrt(nv*nv*(1-a2)+a2)+nv*sqrt(nl*nl*(1-a2)+a2)),1e-6f);float weight=visibility*vh/std::max(nh*nv,1e-6f),fc=pow(1-vh,5);sum+=glm::vec2((1-fc)*weight,fc*weight);}}result.brdf[y*64+x]=sum/float(samples);}
 out=std::move(result);error.clear();return true;
}
bool SaveEnvironment(const std::string& path,const EnvironmentData& d,std::string& e){
 // Validate the complete derived payload before opening/replacing a source asset.
 if(d.sourceHash.size()!=64){e="invalid environment content hash";return false;}
 std::ostringstream payload(std::ios::out|std::ios::binary);payload.write("JENV0001",8);payload.write(d.sourceHash.data(),64);U32(payload,unsigned(d.specular.size()));
 auto level=[&](const EnvironmentLevel& l){U32(payload,l.width);U32(payload,l.height);for(auto v:l.pixels){Float(payload,v.x);Float(payload,v.y);Float(payload,v.z);}};
 for(auto& l:d.specular)level(l);
 level(d.diffuse);U32(payload,d.brdfSize);for(auto v:d.brdf){Float(payload,v.x);Float(payload,v.y);}
 auto text=payload.str();EnvironmentData validated;if(!DecodeEnvironment(std::vector<unsigned char>(text.begin(),text.end()),validated,e))return false;
 std::ofstream file(path,std::ios::binary);file.write(text.data(),std::streamsize(text.size()));if(!file){e="cannot write environment "+path;return false;}e.clear();return true;
}
bool DecodeEnvironment(const std::vector<unsigned char>& bytes,EnvironmentData& out,std::string& e){size_t pos=0;bool ok=true;auto u32=[&](){if(pos+4>bytes.size()){ok=false;return 0u;}unsigned v=0;for(int i=0;i<4;++i)v|=unsigned(bytes[pos++])<<(8*i);return v;};auto f32=[&](){unsigned u=u32();float f;memcpy(&f,&u,4);if(!std::isfinite(f)||f<0||f>60000)ok=false;return f;};if(bytes.size()<76||memcmp(bytes.data(),"JENV0001",8)){e="invalid environment header/version";return false;}EnvironmentData d;pos=8;d.sourceHash.assign(reinterpret_cast<const char*>(bytes.data()+pos),64);pos+=64;unsigned count=u32();if(count<1||count>10){e="invalid environment mip count";return false;}auto level=[&](EnvironmentLevel& l){l.width=u32();l.height=u32();if(!l.width||!l.height||l.width>512||l.height>512||pos+size_t(l.width)*l.height*12>bytes.size()){ok=false;return;}l.pixels.resize(size_t(l.width)*l.height);for(auto& v:l.pixels){v.x=f32();v.y=f32();v.z=f32();}};d.specular.resize(count);for(auto& l:d.specular)level(l);level(d.diffuse);d.brdfSize=u32();if(!d.brdfSize||d.brdfSize>128||pos+size_t(d.brdfSize)*d.brdfSize*8>bytes.size())ok=false;if(ok){d.brdf.resize(size_t(d.brdfSize)*d.brdfSize);for(auto& v:d.brdf){v.x=f32();v.y=f32();}}if(!ok||pos!=bytes.size()){e="invalid/truncated environment payload";return false;}for(size_t i=1;i<d.specular.size();++i)if(d.specular[i].width!=std::max(1u,d.specular[i-1].width/2)||d.specular[i].height!=std::max(1u,d.specular[i-1].height/2)){e="invalid environment mip dimensions";return false;}out=std::move(d);e.clear();return true;}
bool LoadEnvironment(const std::string& path,EnvironmentData& d,std::string& e){std::ifstream s(path,std::ios::binary);std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(s),{}};return DecodeEnvironment(bytes,d,e);}
