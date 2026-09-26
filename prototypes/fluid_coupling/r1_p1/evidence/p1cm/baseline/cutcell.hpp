#pragma once
#include <algorithm>
#include <vector>
#include "vof.hpp"
namespace r1p1 {
struct BoundaryPatch { double area{}; Vec2 centroid{},normal{}; };
inline double rectIntersectionArea(Rect a,Rect b){return std::max(0.0,std::min(a.x1,b.x1)-std::max(a.x0,b.x0))*std::max(0.0,std::min(a.y1,b.y1)-std::max(a.y0,b.y0));}
inline double solidCellFraction(Rect solid,Rect cell){return rectIntersectionArea(solid,cell)/((cell.x1-cell.x0)*(cell.y1-cell.y0));}
inline double openVerticalFace(Rect solid,double x,double y0,double y1){double blocked=(x>=solid.x0&&x<=solid.x1)?std::max(0.0,std::min(y1,solid.y1)-std::max(y0,solid.y0)):0.0;return std::clamp(1.0-blocked/(y1-y0),0.0,1.0);}
inline double openHorizontalFace(Rect solid,double y,double x0,double x1){double blocked=(y>=solid.y0&&y<=solid.y1)?std::max(0.0,std::min(x1,solid.x1)-std::max(x0,solid.x0)):0.0;return std::clamp(1.0-blocked/(x1-x0),0.0,1.0);}
inline void appendVerticalPatches(Rect solid,Rect cell,std::vector<BoundaryPatch>&out){double y0=std::max(solid.y0,cell.y0),y1=std::min(solid.y1,cell.y1);if(y1<=y0)return;for(auto [x,n]:{std::pair<double,Vec2>{solid.x0,{1,0}},std::pair<double,Vec2>{solid.x1,{-1,0}}})if(x>=cell.x0&&x<=cell.x1)out.push_back({y1-y0,{x,.5*(y0+y1)},n});}
inline void appendHorizontalPatches(Rect solid,Rect cell,std::vector<BoundaryPatch>&out){double x0=std::max(solid.x0,cell.x0),x1=std::min(solid.x1,cell.x1);if(x1<=x0)return;for(auto [y,n]:{std::pair<double,Vec2>{solid.y0,{0,1}},std::pair<double,Vec2>{solid.y1,{0,-1}}})if(y>=cell.y0&&y<=cell.y1)out.push_back({x1-x0,{.5*(x0+x1),y},n});}
inline std::vector<BoundaryPatch> bodyPatchesInCell(Rect solid,Rect cell){std::vector<BoundaryPatch> p;appendVerticalPatches(solid,cell,p);appendHorizontalPatches(solid,cell,p);return p;}
}
