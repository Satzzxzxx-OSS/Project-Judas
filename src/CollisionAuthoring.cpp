#include "CollisionAuthoring.h"
#include "ResourceManager.h"
bool ExtendedCompound(const SceneBodyComponent& b){for(auto& c:b.compoundBoxes)if(c.rotation!=glm::quat(1,0,0,0)||c.type!=ShapeType::Box||!c.assetId.empty())return true;return false;}
bool ResolveBodyCollision(const SceneBodyComponent& b,ResourceManager* resources,Shape& out,std::string& error){
 auto load=[&](const std::string& id,bool convex)->std::shared_ptr<const CollisionAsset>{if(!resources){error="cooked collision requires project ResourceManager";return nullptr;}auto a=resources->RequireCollision(id,error);if(a&&a->convex!=convex){error="cooked collision kind does not match authored body";return nullptr;}return a;};
 if(b.shape==SceneShape::ConvexHull||b.shape==SceneShape::TriangleMesh){auto asset=load(b.collisionAsset,b.shape==SceneShape::ConvexHull);if(!asset)return false;out=Shape::Cooked(asset,b.collisionAsset);return true;}
 if(b.shape==SceneShape::Compound){auto children=b.compoundBoxes;for(auto& c:children)if(c.type==ShapeType::ConvexHull){c.asset=load(c.assetId,true);if(!c.asset)return false;}out=Shape::Compound(std::move(children));return true;}
 error="body is not a cooked/compound shape";return false;
}

bool ValidateCollisionFluid(const SceneObject& o,bool legacyParticleWater,std::string& error){
 if(!o.body)return true;
 const auto& b=*o.body;
 bool unsupported=b.shape==SceneShape::TriangleMesh||b.shape==SceneShape::ConvexHull;
 for(auto& child:b.compoundBoxes)unsupported|=child.type!=ShapeType::Box;
 if(unsupported&&(legacyParticleWater||o.liquidContainer||!b.fluidCavities.empty())){
  error="cooked/mixed collision has no legacy particle-fluid volume/contact proxy; use supported primitive geometry";return false;
 }
 return true;
}
