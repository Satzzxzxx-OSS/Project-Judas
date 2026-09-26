#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

namespace r1p1 {
struct Vec2 { double x{}, y{}; };
struct Rect { double x0{}, y0{}, x1{}, y1{}; };
struct Poly { std::vector<Vec2> p; };
inline Vec2 operator+(Vec2 a,Vec2 b){return {a.x+b.x,a.y+b.y};}
inline Vec2 operator-(Vec2 a,Vec2 b){return {a.x-b.x,a.y-b.y};}
inline Vec2 operator*(Vec2 a,double s){return {a.x*s,a.y*s};}
inline double dot(Vec2 a,Vec2 b){return a.x*b.x+a.y*b.y;}
inline double cross(Vec2 a,Vec2 b){return a.x*b.y-a.y*b.x;}
inline double area(const Poly&q){double a=0;if(q.p.size()<3)return 0;for(size_t i=0;i<q.p.size();++i)a+=cross(q.p[i],q.p[(i+1)%q.p.size()]);return std::abs(a)*.5;}
inline Poly rectangle(Rect r){return {{{r.x0,r.y0},{r.x1,r.y0},{r.x1,r.y1},{r.x0,r.y1}}};}
inline Poly clip(const Poly&in,Vec2 n,double c,bool le=true){Poly out;if(in.p.empty())return out;auto f=[&](Vec2 p){double d=dot(n,p)-c;return le?d:-d;};for(size_t i=0;i<in.p.size();++i){Vec2 a=in.p[i],b=in.p[(i+1)%in.p.size()];double da=f(a),db=f(b);bool ia=da<=0,ib=db<=0;if(ia)out.p.push_back(a);if(ia!=ib)out.p.push_back(a+(b-a)*(da/(da-db)));}return out;}
inline Poly plic(Rect cell,double alpha,Vec2 normal){Poly box=rectangle(cell);double full=area(box);if(alpha<=0)return {};if(alpha>=1)return box;double lo=dot(normal,box.p[0]),hi=lo;for(Vec2 p:box.p){lo=std::min(lo,dot(normal,p));hi=std::max(hi,dot(normal,p));}for(int k=0;k<64;++k){double m=(lo+hi)*.5;if(area(clip(box,normal,m))<alpha*full)lo=m;else hi=m;}return clip(box,normal,(lo+hi)*.5);}
inline double areaInStrip(const Poly&p,bool xAxis,double lo,double hi){if(p.p.empty())return 0;Vec2 n1=xAxis?Vec2{-1,0}:Vec2{0,-1},n2=xAxis?Vec2{1,0}:Vec2{0,1};Poly q=clip(p,n1,-lo);q=clip(q,n2,hi);return area(q);}
}
