#pragma once
// Independent analytical area oracles. These never call vof.hpp geometry.
// All integration uses long double; fractions are converted only at the boundary.
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
namespace oracle {
using Real=long double;
struct Point{Real x,y;};
struct Box{Real x0,y0,x1,y1;};
using Polygon=std::vector<Point>;
inline Box cell(int n,int i,int j){return {Real(i)/n,Real(j)/n,Real(i+1)/n,Real(j+1)/n};}
inline double rectangleFraction(int n,int i,int j,Box r){
    Box c=cell(n,i,j);Real x=std::max(Real(0),std::min(c.x1,r.x1)-std::max(c.x0,r.x0));
    Real y=std::max(Real(0),std::min(c.y1,r.y1)-std::max(c.y0,r.y0));return double(x*y*n*n);
}
inline Polygon orientedBox(Real x,Real y,Real hx,Real hy,Real angle){
    Real co=std::cos(angle),si=std::sin(angle);Polygon out;
    for(Point p:Polygon{{-hx,-hy},{hx,-hy},{hx,hy},{-hx,hy}})out.push_back({x+co*p.x-si*p.y,y+si*p.x+co*p.y});
    return out;
}
// Exact piecewise-linear vertical integration for a convex polygon.
// Break at polygon vertices and at each edge crossing a horizontal cell boundary.
// Between breaks the vertical intersection length is linear, so trapezoids are exact.
inline Real polygonAreaInBox(const Polygon&p,Box c){
    Real xmin=p[0].x,xmax=xmin;for(Point v:p){xmin=std::min(xmin,v.x);xmax=std::max(xmax,v.x);}
    Real left=std::max(c.x0,xmin),right=std::min(c.x1,xmax);if(left>=right)return 0;
    std::vector<Real> cuts{left,right};auto add=[&](Real x){if(x>left&&x<right)cuts.push_back(x);};
    for(size_t k=0;k<p.size();++k){Point a=p[k],b=p[(k+1)%p.size()];add(a.x);if(a.y!=b.y)for(Real y:{c.y0,c.y1}){Real t=(y-a.y)/(b.y-a.y);if(t>0&&t<1)add(a.x+t*(b.x-a.x));}}
    std::sort(cuts.begin(),cuts.end());
    auto height=[&](Real x){Real lo=std::numeric_limits<Real>::infinity(),hi=-lo;
        for(size_t k=0;k<p.size();++k){Point a=p[k],b=p[(k+1)%p.size()];if(x<std::min(a.x,b.x)||x>std::max(a.x,b.x))continue;
            if(a.x==b.x){lo=std::min(lo,std::min(a.y,b.y));hi=std::max(hi,std::max(a.y,b.y));}
            else{Real y=a.y+(b.y-a.y)*(x-a.x)/(b.x-a.x);lo=std::min(lo,y);hi=std::max(hi,y);}}
        return std::max(Real(0),std::min(c.y1,hi)-std::max(c.y0,lo));};
    Real area=0;for(size_t k=1;k<cuts.size();++k){Real a=cuts[k-1],b=cuts[k];area+=(b-a)*(height(a)+height(b))/2;}return area;
}
inline double polygonFraction(int n,int i,int j,const Polygon&p){return double(polygonAreaInBox(p,cell(n,i,j))*n*n);}
// Integrate the circular vertical chord analytically, splitting where either cell
// y bound meets the circle. No subcell sampling or production polygon clipping.
inline Real diskAreaInBox(Real cx,Real cy,Real radius,Box c){
    Real left=std::max(c.x0,cx-radius),right=std::min(c.x1,cx+radius);
    if(left>=right||c.y1<=cy-radius||c.y0>=cy+radius)return 0;
    std::vector<Real> cuts{left,right};
    auto add=[&](Real x){if(x>left&&x<right)cuts.push_back(x);};
    for(Real y:{c.y0,c.y1}){Real dy=y-cy;if(std::abs(dy)<radius){Real s=std::sqrt(radius*radius-dy*dy);add(cx-s);add(cx+s);}}
    std::sort(cuts.begin(),cuts.end());
    auto primitive=[&](Real x){x=std::max(-radius,std::min(radius,x));return (x*std::sqrt(std::max(Real(0),radius*radius-x*x))+radius*radius*std::asin(x/radius))/2;};
    Real area=0;for(size_t k=1;k<cuts.size();++k){Real a=cuts[k-1],b=cuts[k],x=(a+b)/2-cx,s=std::sqrt(std::max(Real(0),radius*radius-x*x));
        Real top=std::min(c.y1-cy,s),bottom=std::max(c.y0-cy,-s);if(top<=bottom)continue;
        Real constant=0,root=0;
        if(c.y1-cy<s)constant+=c.y1-cy;else root+=1;
        if(c.y0-cy>-s)constant-=c.y0-cy;else root+=1;
        area+=constant*(b-a)+root*(primitive(b-cx)-primitive(a-cx));}
    return area;
}
inline double slotFraction(int n,int i,int j,double sx,double sy,double theta){
    // The pre-existing nontrivial fixture: radius .2 disk minus an internal
    // .05 x .15 rectangular slot, whose lower edge begins at the disk center.
    // The slot lies wholly within the disk under every prescribed rigid transform.
    Real co=std::cos(Real(theta)),si=std::sin(Real(theta)),cx=.5L+sx,cy=.5L+sy;
    Polygon slot=orientedBox(cx-.075L*si,cy+.075L*co,.025L,.075L,theta);
    Box c=cell(n,i,j);return double((diskAreaInBox(cx,cy,.2L,c)-polygonAreaInBox(slot,c))*n*n);
}
}
