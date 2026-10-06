#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <stdexcept>
#include <type_traits>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// Explicit little-endian field encoding. Counts are checked BEFORE allocation;
// pointers, handles and C++ object layouts are never archive fields.
class SaveArchive {
public:
 static constexpr size_t Limit=64*1024*1024;
 std::string bytes;size_t position=0,allocated=0;bool reading=false;
 explicit SaveArchive(std::string input):bytes(std::move(input)),reading(true){if(bytes.size()>Limit)throw std::runtime_error("save exceeds 64 MiB");}
 SaveArchive()=default;
 void Require(bool condition,const std::string& error){if(!condition)throw std::runtime_error(error);}
 void Finish(){Require(!reading||position==bytes.size(),"trailing participant data");}
 template<class... T> void operator()(T&... values){(Field(values),...);}
 template<class T> void Field(T& value){
  if constexpr(std::is_same_v<T,bool>){uint8_t b=value?1:0;Field(b);Require(b<=1,"invalid saved boolean");value=b!=0;}
  else if constexpr(std::is_enum_v<T>){int32_t n=int32_t(value);Field(n);value=T(n);}
  else if constexpr(std::is_integral_v<T>){using U=std::make_unsigned_t<T>;U n=U(value);if(reading){Require(bytes.size()-position>=sizeof(T),"truncated save field");n=0;for(size_t i=0;i<sizeof(T);++i)n|=U(uint8_t(bytes[position++]))<<(8*i);value=T(n);}else{Require(bytes.size()+sizeof(T)<=Limit,"save exceeds 64 MiB");for(size_t i=0;i<sizeof(T);++i)bytes.push_back(char(n>>(8*i)));}}
  else if constexpr(std::is_floating_point_v<T>){using U=std::conditional_t<sizeof(T)==4,uint32_t,uint64_t>;U n=0;std::memcpy(&n,&value,sizeof(T));Field(n);if(reading)std::memcpy(&value,&n,sizeof(T));Require(std::isfinite(value),"nonfinite saved number");}
  else value.Save(*this);
 }
 void Field(std::string& s){uint32_t n=uint32_t(s.size());Field(n);Require(n<=8*1024*1024,"save string exceeds 8 MiB");if(reading){Require(n<=bytes.size()-position,"truncated save string");s=bytes.substr(position,n);position+=n;}else{Require(bytes.size()+n<=Limit,"save exceeds 64 MiB");bytes+=s;}}
 template<class T> void Field(std::vector<T>& v){uint32_t n=uint32_t(v.size());Field(n);Require(n<=65536,"save collection exceeds 65536");if(reading){Require(n<=bytes.size()-position,"impossible saved collection count");Require(size_t(n)<=Limit/sizeof(T)&&allocated+size_t(n)*sizeof(T)<=Limit,"save allocation budget exceeded");allocated+=size_t(n)*sizeof(T);v.resize(n);}for(auto& item:v)Field(item);}
 template<class K,class V> void Field(std::map<K,V>& m){uint32_t n=uint32_t(m.size());Field(n);Require(n<=65536,"save map exceeds 65536");if(reading){m.clear();for(uint32_t i=0;i<n;++i){K k{};V v{};Field(k);Field(v);Require(m.emplace(std::move(k),std::move(v)).second,"duplicate saved key");}}else for(auto& [key,v]:m){auto k=key;Field(k);Field(v);}}
 template<class T> void Field(std::set<T>& s){std::vector<T> v(s.begin(),s.end());Field(v);if(reading){s={v.begin(),v.end()};Require(s.size()==v.size(),"duplicate saved set value");}}
 template<class A,class B> void Field(std::pair<A,B>& p){Field(p.first);Field(p.second);}
 template<glm::length_t L,class T,glm::qualifier Q> void Field(glm::vec<L,T,Q>& v){for(int i=0;i<L;++i)Field(v[i]);}
 template<class T,glm::qualifier Q> void Field(glm::qua<T,Q>& q){Field(q.w);Field(q.x);Field(q.y);Field(q.z);Require(glm::dot(q,q)>1e-12,"degenerate saved rotation");}
 template<glm::length_t C,glm::length_t R,class T,glm::qualifier Q> void Field(glm::mat<C,R,T,Q>& m){for(int i=0;i<C;++i)Field(m[i]);}
};
