// Endpoint exclusion for coarse particles squeezed into overlapping collision
// margins. This is position projection, not liquid transport: matter is retained
// and the displacement is excluded from reconstructed velocity just like other
// solid push-out. Only encountered box primitives provide escape planes.
double GeometryRoundoff(const glm::vec3& point, const glm::vec3& center, float extent) {
    return 16.0*std::numeric_limits<float>::epsilon()*
        std::max(1.0,glm::length(glm::dvec3(point))+glm::length(glm::dvec3(center))+double(extent));
}
double BoxSeparation(const glm::vec3& point,const PreparedBox& box,float radius) {
    const glm::vec3 local=box.inverseEnd*(point-box.end.position);
    const glm::vec3 gap=glm::abs(local)-(box.halfExtents+glm::vec3(radius));
    return std::max({double(gap.x),double(gap.y),double(gap.z)});
}
bool InsidePreparedSolid(const glm::vec3& point,const std::vector<PreparedBox>& boxes,
                         const std::vector<PreparedSphere>& spheres,
                         const std::vector<PreparedTerrain>& terrains,float radius) {
    for(const auto& box:boxes)
        if(BoxSeparation(point,box,radius)<-GeometryRoundoff(point,box.end.position,glm::length(box.halfExtents)+radius))return true;
    for(const auto& sphere:spheres)
        if(double(glm::distance(point,sphere.end.position))-sphere.radius-radius<
           -GeometryRoundoff(point,sphere.end.position,sphere.radius+radius))return true;
    for(const auto& terrain:terrains) {
        const auto local=terrain.inverseEnd*(point-terrain.end.position);
        if(glm::dot(local,local)<=kEpsilon*kEpsilon)return true;
        if(double(terrain.surface->Sample(local).signedDistance)-radius<
           -GeometryRoundoff(point,terrain.end.position,terrain.surface->BoundRadius()+radius))return true;
    }
    return false;
}
bool OutsidePreparedSolids(const glm::vec3& point,const std::vector<PreparedBox>& boxes,
                           const std::vector<PreparedSphere>& spheres,
                           const std::vector<PreparedTerrain>& terrains,float radius) {
    for(const auto& box:boxes)if(BoxSeparation(point,box,radius)<0)return false;
    for(const auto& sphere:spheres)if(glm::distance(point,sphere.end.position)<sphere.radius+radius)return false;
    for(const auto& terrain:terrains) {
        const auto local=terrain.inverseEnd*(point-terrain.end.position);
        if(glm::dot(local,local)<=kEpsilon*kEpsilon || terrain.surface->Sample(local).signedDistance<radius)return false;
    }
    return true;
}
bool EscapeEncounteredBoxes(const glm::vec3& reference,glm::vec3& point,
                            const std::vector<BoxContact>& contacts,
                            const std::vector<PreparedBox>& boxes,
                            const std::vector<PreparedSphere>& spheres,
                            const std::vector<PreparedTerrain>& terrains,float radius,
                            std::size_t& candidateCount) {
    struct Plane {glm::dvec3 n;double offset;};
    std::vector<const PreparedBox*> encountered;
    for(const auto& contact:contacts)if(contact.box &&
        std::find(encountered.begin(),encountered.end(),contact.box)==encountered.end())encountered.push_back(contact.box);
    std::vector<Plane> planes;planes.reserve(6*encountered.size());
    for(const auto* box:encountered)for(int axis=0;axis<3;++axis)for(int sign:{-1,1}) {
        glm::vec3 localNormal(0);localNormal[axis]=float(sign);
        const auto n=glm::normalize(glm::dvec3(box->end.rotation*localNormal));
        const double margin=GeometryRoundoff(reference,box->end.position,glm::length(box->halfExtents)+radius);
        planes.push_back({n,glm::dot(n,glm::dvec3(box->end.position))+box->halfExtents[axis]+radius+2*margin});
    }
    const glm::dvec3 origin(reference);
    double best=std::numeric_limits<double>::infinity();glm::vec3 selected(0);
    const auto consider=[&](const glm::dvec3& candidate) {
        ++candidateCount;
        if(!std::isfinite(candidate.x)||!std::isfinite(candidate.y)||!std::isfinite(candidate.z))return;
        const glm::vec3 represented(candidate);
        if(!std::isfinite(represented.x)||!std::isfinite(represented.y)||!std::isfinite(represented.z)||
           !OutsidePreparedSolids(represented,boxes,spheres,terrains,radius))return;
        const auto displacement=glm::dvec3(represented)-origin;
        const double distance=glm::dot(displacement,displacement);
        if(distance<best){best=distance;selected=represented;}
    };
    // Closest points in a 3-D polyhedral exterior have at most three
    // linearly independent active planes. Gram rank tolerance is dimensionless
    // double roundoff, never a scene/body/contact-gap threshold.
    constexpr double rankTolerance=64*std::numeric_limits<double>::epsilon();
    for(std::size_t a=0;a<planes.size();++a) {
        const auto& pa=planes[a];const double ra=pa.offset-glm::dot(pa.n,origin);
        consider(origin+pa.n*ra);
        for(std::size_t b=a+1;b<planes.size();++b) {
            const auto& pb=planes[b];const double rb=pb.offset-glm::dot(pb.n,origin);
            const double ab=glm::dot(pa.n,pb.n),determinant=1-ab*ab;
            if(determinant<=rankTolerance)continue;
            consider(origin+pa.n*((ra-ab*rb)/determinant)+pb.n*((rb-ab*ra)/determinant));
            for(std::size_t c=b+1;c<planes.size();++c) {
                const auto& pc=planes[c];const double rc=pc.offset-glm::dot(pc.n,origin);
                const double ac=glm::dot(pa.n,pc.n),bc=glm::dot(pb.n,pc.n);
                const glm::dmat3 gram(glm::dvec3(1,ab,ac),glm::dvec3(ab,1,bc),glm::dvec3(ac,bc,1));
                if(glm::determinant(gram)<=rankTolerance)continue;
                const glm::dvec3 coefficients=glm::inverse(gram)*glm::dvec3(ra,rb,rc);
                consider(origin+pa.n*coefficients.x+pb.n*coefficients.y+pc.n*coefficients.z);
            }
        }
    }
    if(!std::isfinite(best))return false;
    point=selected;return true;
}
BoxContact BoxFaceContact(const glm::vec3& point,const PreparedBox& box,int axis,float sign,float substepTime) {
    BoxContact result;result.hit=true;result.owner=box.owner;result.projectedPosition=point;result.box=&box;
    glm::vec3 localNormal(0);localNormal[axis]=sign;result.normal=box.end.rotation*localNormal;
    const glm::vec3 local=box.inverseEnd*(point-box.end.position);
    const glm::vec3 surface=glm::clamp(local,-box.halfExtents,box.halfExtents);
    result.point=box.end.position+box.end.rotation*surface;
    result.wallVelocity=(result.point-(box.start.position+box.start.rotation*surface))/substepTime;
    result.boxAxis=axis;
    return result;
}

void ApplySolidCollisions(std::size_t particleIndex, const glm::vec3& from,
                          std::vector<glm::vec3>& positions,
                          const std::vector<PreparedBox>& boxes,
                          const std::vector<PreparedSphere>& spheres,
                          const std::vector<PreparedTerrain>& terrains,
                          float radius, float substepTime,
                          std::vector<glm::vec3>& solidCorrections,
                          std::vector<std::vector<BoxContact>>& contacts,
                          std::size_t& escapeCount,std::size_t& unresolvedCount,
                          std::size_t& candidateCount,float& maximumEscapeDistance) {
    const glm::vec3 reference=positions[particleIndex];
    auto retain = [&](const BoxContact& contact) {
        // Position projection and velocity response share every active plane.
        // Repeated density iterations update the same plane rather than losing
        // the floor when a later wall owns the final projection.
        auto& rows = contacts[particleIndex];
        for (auto& row : rows) {
            if (row.owner.id == contact.owner.id && row.box == contact.box &&
                glm::dot(row.normal, contact.normal) >= 1.0f - 8.0f * std::numeric_limits<float>::epsilon()) {
                row = contact;
                return;
            }
        }
        rows.push_back(contact);
    };
    // Two passes resolve the common floor/wall corner without introducing a
    // container-specific intersection solver.
    for (int pass = 0; pass < 2; ++pass) {
        for (const PreparedBox& box : boxes) {
            BoxContact contact = CollideBox(from, positions[particleIndex], box,
                                            radius, substepTime);
            if (!contact.hit) continue;
            contact.owner = box.owner;
            const glm::vec3 correction = contact.projectedPosition - positions[particleIndex];
            positions[particleIndex] = contact.projectedPosition;
            retain(contact);
            solidCorrections[particleIndex] += correction;
        }
        for (const PreparedSphere& sphere : spheres) {
            BoxContact contact = CollideSphere(from, positions[particleIndex], sphere,
                                               radius, substepTime);
            if (!contact.hit) continue;
            contact.owner = sphere.owner;
            const glm::vec3 correction = contact.projectedPosition - positions[particleIndex];
            positions[particleIndex] = contact.projectedPosition;
            retain(contact);
            solidCorrections[particleIndex] += correction;
        }
        for (const PreparedTerrain& terrain : terrains) {
            BoxContact contact = CollideTerrain(from, positions[particleIndex], terrain,
                                                radius, substepTime);
            if (!contact.hit) continue;
            contact.owner = terrain.owner;
            const glm::vec3 correction = contact.projectedPosition - positions[particleIndex];
            positions[particleIndex] = contact.projectedPosition;
            retain(contact);
            solidCorrections[particleIndex] += correction;
        }
    }
    auto& finalPoint=positions[particleIndex];auto& rows=contacts[particleIndex];
    if(InsidePreparedSolid(finalPoint,boxes,spheres,terrains,radius)) {
        const glm::vec3 before=finalPoint;
        if(EscapeEncounteredBoxes(reference,finalPoint,rows,boxes,spheres,terrains,radius,candidateCount)) {
            ++escapeCount;solidCorrections[particleIndex]+=finalPoint-before;
            maximumEscapeDistance=std::max(maximumEscapeDistance,glm::distance(finalPoint,before));
            std::vector<const PreparedBox*> encountered;
            for(const auto& row:rows)if(row.box &&
                std::find(encountered.begin(),encountered.end(),row.box)==encountered.end())encountered.push_back(row.box);
            rows.erase(std::remove_if(rows.begin(),rows.end(),[&](const BoxContact& row) {
                if(!row.box)return false;
                const double tolerance=GeometryRoundoff(finalPoint,row.box->end.position,glm::length(row.box->halfExtents)+radius);
                if(BoxSeparation(finalPoint,*row.box,radius)>tolerance)return true;
                const glm::vec3 local=row.box->inverseEnd*(finalPoint-row.box->end.position);
                const glm::vec3 half=row.box->halfExtents+glm::vec3(radius);
                // A different face of this primitive can remain active while
                // the old finite face patch is now provably far away.
                for(int axis=0;axis<3;++axis)if(axis!=row.boxAxis && std::abs(local[axis])>half[axis]+tolerance)return true;
                return false;
            }),rows.end());
            // Endpoint-active planes replace the escape primitive's old
            // contact locations; crossed but now clear box planes are absent.
            for(const auto* box:encountered) {
                const glm::vec3 local=box->inverseEnd*(finalPoint-box->end.position);
                const glm::vec3 half=box->halfExtents+glm::vec3(radius);
                const double tolerance=4*GeometryRoundoff(finalPoint,box->end.position,glm::length(box->halfExtents)+radius);
                for(int axis=0;axis<3;++axis) {
                    bool onFace=std::abs(double(std::abs(local[axis]))-half[axis])<=tolerance;
                    for(int other=0;other<3;++other)if(other!=axis)
                        onFace=onFace && std::abs(local[other])<=half[other]+tolerance;
                    if(onFace)retain(BoxFaceContact(finalPoint,*box,axis,local[axis]>=0?1.f:-1.f,substepTime));
                }
            }
        } else ++unresolvedCount;
    }

}
