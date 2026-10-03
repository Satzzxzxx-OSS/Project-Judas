#pragma once
#include "GravityField.h"
#include "MeshData.h"
#include <array>
#include <string>
#include <vector>
// Authored physical cavity: a disjoint union of positive-volume tetrahedra.
// This is independent of the visual mesh. Coordinates and integration use doubles.
using LiquidTet = std::array<glm::dvec3,4>;
struct LiquidGeometry {std::vector<LiquidTet> cells;};
struct LiquidMoment {double volume=0;glm::dvec3 first{0};};
struct LiquidCurvePoint {double q=0,volume=0;};
struct LiquidBasinData {
 LiquidGeometry geometry;GravityEquilibrium equilibrium;
 std::vector<LiquidCurvePoint> curve;std::string fingerprint;
 double volumeTolerance=1e-5,heightTolerance=.002,coordinateError=0,capacity=0;
};
bool LoadLiquidGeometry(const std::string&,LiquidGeometry&,std::string&);
bool ValidateLiquidGeometry(const LiquidGeometry&,std::string&);
bool DecodeLiquidBasin(const std::vector<unsigned char>&,LiquidBasinData&,std::string&);
bool SaveLiquidBasin(const std::string&,const LiquidBasinData&,std::string&);
bool LoadLiquidBasin(const std::string&,LiquidBasinData&,std::string&);
LiquidGeometry LiquidBox(glm::dvec3 minimum,glm::dvec3 maximum);
LiquidMoment LiquidClip(const LiquidTet&,const std::array<double,4>& coordinates,double q,MeshData* cap=nullptr);
double LiquidCapacity(const LiquidGeometry&,const GravityEquilibrium&,double q);
double LiquidInverse(const LiquidBasinData&,double volume);
bool BakeLiquidBasin(const LiquidGeometry&,const GravityEquilibrium&,double volumeTolerance,double heightTolerance,const std::string& fingerprint,LiquidBasinData&,std::string&);
double LiquidCoordinate(const LiquidTet&,const GravityEquilibrium&,glm::dvec3 point,bool& inside);
LiquidMoment LiquidIntersection(const LiquidTet& body,const LiquidTet& basin,const GravityEquilibrium&,double q);
