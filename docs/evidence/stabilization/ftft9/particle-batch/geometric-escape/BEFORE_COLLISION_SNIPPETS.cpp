struct BoxContact {
    bool hit = false;
    BodyHandle owner;
    glm::vec3 projectedPosition{0.0f};
    glm::vec3 normal{0.0f};
    glm::vec3 point{0.0f};
    glm::vec3 wallVelocity{0.0f};
};

struct PreparedBox {
    BodyHandle owner;
    BodyTransform start;
    BodyTransform end;
    glm::quat inverseStart{1.0f, 0.0f, 0.0f, 0.0f};
    glm::quat inverseEnd{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 halfExtents{0.0f};
    glm::vec3 sweptCenter{0.0f};
    float sweptRadius = 0.0f;
};


void ApplySolidCollisions(std::size_t particleIndex, const glm::vec3& from,
                          std::vector<glm::vec3>& positions,
                          const std::vector<PreparedBox>& boxes,
                          const std::vector<PreparedSphere>& spheres,
                          const std::vector<PreparedTerrain>& terrains,
                          float radius, float substepTime,
                          std::vector<glm::vec3>& solidCorrections,
                          std::vector<std::vector<BoxContact>>& contacts) {
    auto retain = [&](const BoxContact& contact) {
        // Position projection and velocity response share every active plane.
        // Repeated density iterations update the same plane rather than losing
        // the floor when a later wall owns the final projection.
        auto& rows = contacts[particleIndex];
        for (auto& row : rows) {
            if (row.owner.id == contact.owner.id &&
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
}
