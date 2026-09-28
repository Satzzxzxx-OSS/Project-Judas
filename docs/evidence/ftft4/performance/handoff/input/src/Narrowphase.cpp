#include "Narrowphase.h"
#include <algorithm>
#include <array>
#include <cmath>
#include "RadialTerrain.h"
#include "ContactGeometryInternal.h"

int PrimitiveCount(const Shape& shape) {
    return shape.type == ShapeType::CompoundBoxes ? static_cast<int>(shape.boxes.size()) : 1;
}

PrimitivePose PrimitiveAt(const Shape& shape, const RigidBody& parent, int index) {
    if (shape.type != ShapeType::CompoundBoxes) return {shape, parent, parent.position, parent.orientation, glm::vec3(0.0f)};
    const CompoundBox& child = shape.boxes[static_cast<std::size_t>(index)];
    RigidBody childPose = parent;
    childPose.position = glm::vec3(glm::dvec3(parent.position) + ContactRotation(parent.orientation) * glm::dvec3(child.localCenter));
    return {Shape::Box(child.halfExtents), childPose, parent.position, parent.orientation, child.localCenter};
}

TerrainSample SampleTerrainAtWorld(const RadialTerrain& terrain, const RigidBody& body,
const glm::vec3& worldPoint) {
    const glm::quat inverseRotation = glm::conjugate(glm::normalize(body.orientation));
    TerrainSample sample = terrain.Sample(inverseRotation * (worldPoint - body.position));
    sample.surfacePoint = body.position + body.orientation * sample.surfacePoint;
    sample.outwardNormal = glm::normalize(body.orientation * sample.outwardNormal);
    return sample;
}

namespace {
    // Normals follow Contact's shape-A convention. For terrain A the normal
    // points *into* the terrain, so resolving dynamic B pushes it outward.
    // Each box contributes its corners and face centres: a hill can contact the
    // middle of a broad face even while all four corners clear the surface.
    ContactManifold TerrainVsPrimitive(const Shape& terrainShape, const RigidBody& terrainBody,
    const Shape& otherShape, const RigidBody& otherBody, float margin) {
        ContactManifold manifold;
        if (!terrainShape.terrain) return manifold;
        const RadialTerrain& terrain = *terrainShape.terrain;
        float boundingRadius = 0.0f;
        if (otherShape.type == ShapeType::Sphere) boundingRadius = otherShape.radius;
        else if (otherShape.type == ShapeType::Box) boundingRadius = glm::length(otherShape.halfExtents);
        else return manifold;
        if (glm::length(otherBody.position - terrainBody.position) >
        terrain.BoundRadius() + boundingRadius + margin) return manifold;
        if (otherShape.type == ShapeType::Sphere) {
            const TerrainSample sample = SampleTerrainAtWorld(terrain, terrainBody, otherBody.position);
            if (sample.signedDistance < otherShape.radius + margin) {
                Contact contact;
                contact.hit = true;
                contact.point = sample.surfacePoint;
                contact.normal = -sample.outwardNormal;
                contact.penetration = otherShape.radius - sample.signedDistance;
                manifold.Add(contact);
            }

            return manifold;
        }

        std::array<Contact, 14> candidates{};
        int count = 0;
        const glm::vec3 h = otherShape.halfExtents;
        const auto tryPoint = [&](const glm::vec3& localPoint) {
            const glm::vec3 worldPoint = otherBody.position + otherBody.orientation * localPoint;
            const TerrainSample sample = SampleTerrainAtWorld(terrain, terrainBody, worldPoint);
            if (sample.signedDistance >= margin) return;
            Contact contact;
            contact.hit = true;
            contact.point = sample.surfacePoint;
            contact.normal = -sample.outwardNormal;
            contact.penetration = -sample.signedDistance;
            candidates[static_cast<std::size_t>(count++)] = contact;
        };

        for (int x : {-1, 1})
        for (int y : {-1, 1})
        for (int z : {-1, 1})
        tryPoint(glm::vec3(x * h.x, y * h.y, z * h.z));
        for (int axis = 0; axis < 3; ++axis) {
            for (int sign : {-1, 1}) {
                glm::vec3 point(0.0f);
                point[axis] = sign * h[axis];
                tryPoint(point);
            }
        }

        std::sort(candidates.begin(), candidates.begin() + count,
        [](const Contact& a, const Contact& b) {
            return a.penetration > b.penetration;
        });
        for (int i = 0; i < std::min(count, 4); ++i) manifold.Add(candidates[static_cast<std::size_t>(i)]);
        return manifold;
    }
}

// namespace
// Uniform manifold dispatcher: sphere-involving pairs always produce at
// most one contact point (wrapped in a 1-point manifold); box-vs-box uses
// the real multi-point manifold (see Contacts.h for why that one
// specifically needs more than one point).
ContactManifold ComputeContacts(const Shape& shapeA, const RigidBody& bodyA, const Shape& shapeB,
const RigidBody& bodyB, float margin) {
    ContactManifold manifold;
    if (shapeA.type == ShapeType::Terrain) {
        return TerrainVsPrimitive(shapeA, bodyA, shapeB, bodyB, margin);
    }

    if (shapeB.type == ShapeType::Terrain) {
        manifold = TerrainVsPrimitive(shapeB, bodyB, shapeA, bodyA, margin);
        for (int i = 0; i < manifold.count; ++i) manifold.points[i].normal = -manifold.points[i].normal;
        return manifold;
    }

    return PrimitiveContacts(shapeA, {bodyA.position, bodyA.orientation, glm::vec3(0)},
    shapeB, {bodyB.position, bodyB.orientation, glm::vec3(0)}, margin);
}

ContactManifold ComputeContacts(const PrimitivePose& a, const PrimitivePose& b, float margin) {
    if (a.shape.type == ShapeType::Terrain || b.shape.type == ShapeType::Terrain)
    return ComputeContacts(a.shape, a.body, b.shape, b.body, margin);
    return PrimitiveContacts(a.shape, {a.parentPosition, a.parentOrientation, a.parentLocalCenter},
    b.shape, {b.parentPosition, b.parentOrientation, b.parentLocalCenter}, margin);
}

namespace {
    using namespace contact_geometry;
    float LowerFloat(double x) {
        float f=static_cast<float>(x);
        return double(f)>x ? std::nextafter(f,-std::numeric_limits<float>::infinity()) : f;
    }

    float UpperFloat(double x) {
        float f=static_cast<float>(x);
        return double(f)<x ? std::nextafter(f,std::numeric_limits<float>::infinity()) : f;
    }

    Aabb Bound(const std::array<Iv,3>& lo,const std::array<Iv,3>& hi) {
        Aabb out;
        for (int k=0;k<3;++k){
            out.min[k]=LowerFloat(lo[k].lo);
            out.max[k]=UpperFloat(hi[k].hi);
        }

        return out;
    }

    Aabb BoxAabb(const glm::vec3& position, const glm::quat& orientation,
    const glm::vec3& offset, const glm::vec3& half) {
        Rotation<Iv> r(orientation);
        std::array<Iv,3> lo,hi;
        for (int k=0;k<3;++k){
            Iv center(double(position[k])),extent(0);
            for (int j=0;j<3;++j){
                const Iv entry=r.n[j][k]/r.d;
                center=center+entry*Iv(double(offset[j]));
                extent=extent+Abs(entry)*Iv(double(half[j]));
            }

            lo[k]=center-extent;
            hi[k]=center+extent;
        }

        return Bound(lo,hi);
    }

    Aabb SphereAabb(const glm::vec3& position,double radius) {
        std::array<Iv,3> lo,hi;
        for (int k=0;k<3;++k){
            lo[k]=Iv(double(position[k]))-Iv(radius);
            hi[k]=Iv(double(position[k]))+Iv(radius);
        }

        return Bound(lo,hi);
    }
}

// namespace
Aabb ShapeAabb(const Shape& shape, const glm::vec3& position, const glm::quat& orientation) {
    switch (shape.type){
        case ShapeType::Sphere:return SphereAabb(position,shape.radius);
        case ShapeType::Box:return BoxAabb(position,orientation,glm::vec3(0),shape.halfExtents);
        case ShapeType::CompoundBoxes:{
            Aabb bound{position,position};
            bool first=true;
            for (const auto& child:shape.boxes){
                const auto b=BoxAabb(position,orientation,child.localCenter,child.halfExtents);
                bound=first?b:bound.Union(b);
                first=false;
            }

            return bound;
        }

        case ShapeType::Terrain:return SphereAabb(position,shape.terrain?shape.terrain->BoundRadius():0.0);
        case ShapeType::Capsule:return SphereAabb(position,double(shape.radius)+double(shape.halfHeight));
    }

    return {position,position};
}

float ShapeBoundingRadius(const Shape& shape) {
    const auto length = [](const glm::vec3& v) {
        const auto iv = Vector<Iv>(v);
        return Sqrt(Dot(iv,iv));
    };

    switch (shape.type) {
        case ShapeType::Sphere: return shape.radius;
        case ShapeType::Box: return UpperFloat(length(shape.halfExtents).hi);
        case ShapeType::CompoundBoxes: {
            double r = 0.0;
            for (const CompoundBox& child : shape.boxes) {
                r = std::max(r, (length(child.localCenter) + length(child.halfExtents)).hi);
            }

            return UpperFloat(r);
        }

        case ShapeType::Terrain: return shape.terrain ? shape.terrain->BoundRadius() : 0.0f;
        case ShapeType::Capsule: return UpperFloat((Iv(double(shape.radius)) + Iv(double(shape.halfHeight))).hi);
    }

    return 0.0f;
}
