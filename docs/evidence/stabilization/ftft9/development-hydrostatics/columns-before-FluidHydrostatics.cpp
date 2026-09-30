#include "FluidHydrostatics.h"
#include "GravityField.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <glm/gtc/quaternion.hpp>

namespace {
bool Finite(const glm::vec3& v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
void RequireSize(const glm::vec3& h) {
    if (!Finite(h) || h.x <= 0 || h.y <= 0 || h.z <= 0) throw std::invalid_argument("fluid volume requires finite positive dimensions");
}
void AppendBox(FluidVolumeQuadrature& out, const glm::vec3& center, const glm::vec3& h, unsigned n) {
    const glm::vec3 small = h / static_cast<float>(n);
    const double v = 8.0 * h.x * h.y * h.z;
    const float weight = static_cast<float>(v / (static_cast<double>(n) * n * n));
    for (unsigned z = 0; z < n; ++z) for (unsigned y = 0; y < n; ++y) for (unsigned x = 0; x < n; ++x) {
        const glm::vec3 p = center - h + glm::vec3(2*x+1, 2*y+1, 2*z+1) * small;
        out.samples.push_back({p, small, 0, weight});
    }
    out.totalVolume += static_cast<float>(v);
    out.boundRadius = std::max(out.boundRadius, glm::length(glm::abs(center) + h));
}
struct Box { glm::vec3 lo, hi; };
std::vector<Box> Subtract(const Box& a, const Box& b) {
    const glm::vec3 lo = glm::max(a.lo,b.lo), hi = glm::min(a.hi,b.hi);
    if (lo.x >= hi.x || lo.y >= hi.y || lo.z >= hi.z) return {a};
    std::vector<Box> out;
    auto append = [&](glm::vec3 l, glm::vec3 h) { if (l.x<h.x && l.y<h.y && l.z<h.z) out.push_back({l,h}); };
    append(a.lo, {lo.x,a.hi.y,a.hi.z}); append({hi.x,a.lo.y,a.lo.z},a.hi);
    append({lo.x,a.lo.y,a.lo.z},{hi.x,lo.y,a.hi.z}); append({lo.x,hi.y,a.lo.z},{hi.x,a.hi.y,a.hi.z});
    append({lo.x,lo.y,a.lo.z},{hi.x,hi.y,lo.z}); append({lo.x,lo.y,hi.z},{hi.x,hi.y,a.hi.z});
    return out;
}
}

FluidVolumeQuadrature MakeBoxFluidVolume(const glm::vec3& halfExtents, unsigned n) {
    RequireSize(halfExtents);
    if (!n) throw std::invalid_argument("fluid volume sample count is zero");
    FluidVolumeQuadrature out; AppendBox(out,glm::vec3(0),halfExtents,n); return out;
}
FluidVolumeQuadrature MakeSphereFluidVolume(float radius, unsigned layers, unsigned pairs) {
    if (!(radius > 0) || !std::isfinite(radius) || !layers || !pairs) throw std::invalid_argument("invalid fluid sphere quadrature");
    constexpr double pi = 3.1415926535897932384626433832795;
    FluidVolumeQuadrature out;
    out.totalVolume = static_cast<float>((4.0/3.0)*pi*radius*radius*radius); out.boundRadius=radius;
    const float weight = out.totalVolume / static_cast<float>(2.0*layers*pairs);
    const float footprint = 0.5f * std::cbrt(weight);
    for (unsigned l=0;l<layers;++l) {
        const float r = radius * static_cast<float>(std::cbrt((l+0.5)/layers));
        for (unsigned k=0;k<pairs;++k) {
            const double y=(k+0.5)/pairs, azimuth=k*pi*(3.0-std::sqrt(5.0));
            const double radial=std::sqrt(1-y*y);
            const glm::vec3 p=r*glm::vec3(radial*std::cos(azimuth),y,radial*std::sin(azimuth));
            out.samples.push_back({p,glm::vec3(0),footprint,weight});
            out.samples.push_back({-p,glm::vec3(0),footprint,weight});
        }
    }
    return out;
}
FluidVolumeQuadrature MakeCompoundFluidVolume(const std::vector<CompoundBox>& boxes, unsigned n) {
    if (!n) throw std::invalid_argument("fluid volume sample count is zero");
    std::vector<Box> partition;
    for (const auto& box:boxes) {
        RequireSize(box.halfExtents);
        if (!Finite(box.localCenter)) throw std::invalid_argument("nonfinite compound center");
        std::vector<Box> pending{{box.localCenter-box.halfExtents,box.localCenter+box.halfExtents}};
        for (const auto& covered:partition) {
            std::vector<Box> next;
            for (const auto& piece:pending) { const auto rest=Subtract(piece,covered); next.insert(next.end(),rest.begin(),rest.end()); }
            pending=std::move(next);
        }
        partition.insert(partition.end(),pending.begin(),pending.end());
    }
    FluidVolumeQuadrature out;
    for (const auto& box:partition) AppendBox(out,(box.lo+box.hi)*0.5f,(box.hi-box.lo)*0.5f,n);
    return out;
}

std::size_t FluidHydrostaticField::Hash::operator()(const Key& k) const {
    auto mix=[](std::uint64_t x) { x ^= x>>30; x *= UINT64_C(0xbf58476d1ce4e5b9); x ^= x>>27; x *= UINT64_C(0x94d049bb133111eb); return x^(x>>31); };
    return static_cast<std::size_t>(mix(static_cast<std::uint64_t>(k.x)) ^ mix(static_cast<std::uint64_t>(k.y)+UINT64_C(0x9e3779b97f4a7c15)) ^ mix(static_cast<std::uint64_t>(k.z)+UINT64_C(0x3c6ef372fe94f82a)));
}
FluidHydrostaticField::Key FluidHydrostaticField::Cell(const glm::vec3& p) const {
    return {static_cast<std::int64_t>(std::floor(static_cast<double>(p.x)/m_cellSize)),static_cast<std::int64_t>(std::floor(static_cast<double>(p.y)/m_cellSize)),static_cast<std::int64_t>(std::floor(static_cast<double>(p.z)/m_cellSize))};
}
void FluidHydrostaticField::Build(const std::vector<FluidParticle>& particles,float density,const std::vector<bool>* excluded) {
    if (!(density>0) || !std::isfinite(density) || (excluded && excluded->size()!=particles.size())) throw std::invalid_argument("invalid hydrostatic field density or exclusion mask");
    m_particles.clear(); m_cells.clear(); m_density=density; m_maxSpacing=0;
    m_min=glm::vec3(std::numeric_limits<float>::max()); m_max=-m_min;
    for (std::size_t i=0;i<particles.size();++i) {
        if (excluded && (*excluded)[i]) continue;
        const auto& p=particles[i];
        if (!(p.mass>0) || !std::isfinite(p.mass) || !Finite(p.position) || !Finite(p.velocity) || !Finite(p.acceleration)) throw std::invalid_argument("invalid particle in hydrostatic field");
        const float volume=p.mass/density, spacing=std::cbrt(volume);
        m_particles.push_back({p.position,p.velocity,p.acceleration,spacing,volume});
        m_maxSpacing=std::max(m_maxSpacing,spacing);
        m_min=glm::min(m_min,p.position-glm::vec3(spacing*0.5f)); m_max=glm::max(m_max,p.position+glm::vec3(spacing*0.5f));
    }
    m_cellSize=m_maxSpacing>0 ? 2*m_maxSpacing : 1;
    for (std::size_t i=0;i<m_particles.size();++i) m_cells[Cell(m_particles[i].position)].push_back(i);
}
FluidFieldSample FluidHydrostaticField::Query(const glm::vec3& point,const glm::vec3& direction,float halfHeight,float radius) const {
    FluidFieldSample out; out.density=m_density;
    if (m_particles.empty()) return out;
    if (!Finite(point) || !Finite(direction) || !(glm::length(direction)>0) || !std::isfinite(halfHeight) || halfHeight<0 || !std::isfinite(radius) || radius<0) throw std::invalid_argument("invalid hydrostatic query");
    const glm::dvec3 up=glm::normalize(glm::dvec3(direction));
    const double reach=radius+2.0*m_maxSpacing, reach2=reach*reach;
    const float extent=static_cast<float>(reach+halfHeight+m_maxSpacing);
    if (glm::any(glm::lessThan(point+glm::vec3(extent),m_min)) || glm::any(glm::greaterThan(point-glm::vec3(extent),m_max))) return out;
    struct Interval { double lo,hi; };
    std::vector<Interval> intervals;
    double bulkWeight=0;
    glm::dvec3 bulkVelocity(0), bulkAcceleration(0);
    auto add=[&](std::size_t index) {
        const auto& p=m_particles[index];
        const glm::dvec3 d=glm::dvec3(p.position)-glm::dvec3(point);
        const double distance2=glm::dot(d,d);
        // Bulk flow is a continuous spatial average of the most recent
        // particle velocities/accelerations. Its compact kernel vanishes
        // with zero slope at the neighbourhood boundary; it is independent
        // of interval contact with this quadrature footprint. Otherwise a
        // zero-overlap entering row would abruptly receive full weight.
        if (distance2<reach2) {
            const double t=1.0-distance2/reach2, w=p.volume*t*t;
            bulkWeight+=w;
            bulkVelocity+=w*glm::dvec3(p.velocity);
            bulkAcceleration+=w*glm::dvec3(p.acceleration);
        }
        const double h=glm::dot(d,up), lateral2=std::max(0.0,distance2-h*h);
        if (lateral2>=reach2 || h-p.spacing*0.5>halfHeight || h+p.spacing*0.5< -halfHeight) return;
        intervals.push_back({h-p.spacing*0.5,h+p.spacing*0.5});
    };
    const Key lo=Cell(point-glm::vec3(extent)), hi=Cell(point+glm::vec3(extent));
    const double cells=(static_cast<double>(hi.x)-lo.x+1)*(static_cast<double>(hi.y)-lo.y+1)*(static_cast<double>(hi.z)-lo.z+1);
    // Equivalent brute scan prevents huge empty hash traversals for a large body.
    if (cells>4.0*m_particles.size()) for (std::size_t i=0;i<m_particles.size();++i) add(i);
    else for (auto z=lo.z;z<=hi.z;++z) for (auto y=lo.y;y<=hi.y;++y) for (auto x=lo.x;x<=hi.x;++x) {
        const auto it=m_cells.find({x,y,z}); if (it!=m_cells.end()) for (const auto i:it->second) add(i);
    }
    if (intervals.empty()) return out;
    std::stable_sort(intervals.begin(),intervals.end(),[](const Interval& a,const Interval& b) { return a.lo<b.lo; });
    double occupied=0;
    auto consume=[&](const Interval& group) {
        const double overlap=halfHeight>0 ? std::max(0.0,std::min(group.hi,static_cast<double>(halfHeight))-std::max(group.lo,-static_cast<double>(halfHeight))) : (group.lo<=0 && group.hi>=0 ? 1.0 : 0.0);
        occupied+=overlap;
    };
    Interval group=intervals.front();
    for (std::size_t i=1;i<intervals.size();++i) {
        const auto& next=intervals[i];
        if (next.lo<=group.hi) { group.hi=std::max(group.hi,next.hi); }
        else { consume(group); group=next; }
    }
    consume(group);
    if (occupied>0) {
        out.fraction=static_cast<float>(halfHeight>0 ? occupied/(2*halfHeight) : 1.0);
        if (bulkWeight>0) { out.velocity=glm::vec3(bulkVelocity/bulkWeight); out.acceleration=glm::vec3(bulkAcceleration/bulkWeight); }
    }
    return out;
}
FluidHydrostaticResult EvaluateFluidHydrostatics(const FluidHydrostaticField& field,const FluidVolumeQuadrature& volume,const BodyTransform& pose,const GravityField& gravity) {
    FluidHydrostaticResult out; out.totalVolume=volume.totalVolume; out.density=field.RestDensity(); out.submergedCentroid=pose.position;
    double wet=0; glm::dvec3 center(0),velocity(0),acceleration(0),force(0),torque(0);
    for (const auto& sample:volume.samples) {
        const glm::vec3 offset=pose.rotation*sample.localPosition, point=pose.position+offset, g=gravity.Sample(point);
        const glm::vec3 up=glm::dot(g,g)>0 ? -glm::normalize(g) : pose.rotation*glm::vec3(0,1,0);
        const glm::vec3 localUp=glm::conjugate(pose.rotation)*up;
        const float height=glm::dot(glm::abs(localUp),sample.localHalfExtents)+sample.localRadius;
        const auto fluid=field.Query(point,up,height,volume.boundRadius);
        const double v=static_cast<double>(sample.volume)*fluid.fraction;
        const glm::dvec3 f=-static_cast<double>(fluid.density)*v*(glm::dvec3(g)-glm::dvec3(fluid.acceleration));
        wet+=v; center+=v*glm::dvec3(point); velocity+=v*glm::dvec3(fluid.velocity); acceleration+=v*glm::dvec3(fluid.acceleration);
        force+=f; torque+=glm::cross(glm::dvec3(offset),f);
    }
    out.submergedVolume=static_cast<float>(wet); out.fraction=volume.totalVolume>0 ? static_cast<float>(wet/volume.totalVolume) : 0;
    out.buoyancyForce=glm::vec3(force); out.buoyancyTorque=glm::vec3(torque);
    if (wet>0) { out.submergedCentroid=glm::vec3(center/wet); out.fluidVelocity=glm::vec3(velocity/wet); out.fluidAcceleration=glm::vec3(acceleration/wet); }
    return out;
}
