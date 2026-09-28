#pragma once
// FTFT4A internal arithmetic. IEEE binary64 round-to-nearest, no fast-math.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>
#include <stdexcept>

namespace contact_geometry {
    struct ArithmeticFailure : std::runtime_error {
        ArithmeticFailure():std::runtime_error("unresolved contact arithmetic range"){}
    };

    static_assert(sizeof(double)==sizeof(std::uint64_t) && std::numeric_limits<double>::is_iec559,
    "contact interval endpoints require IEEE binary64");
    // Exact adjacent representable values, equivalent to nextafter toward +/-inf.
    // Implemented directly to avoid a libm call at every interval endpoint.
    inline double Down(double x) {
        if (std::isnan(x) || x==-std::numeric_limits<double>::infinity()) return x;
        if (x==0) return -std::numeric_limits<double>::denorm_min();
        std::uint64_t bits;
        std::memcpy(&bits,&x,sizeof bits);
        if (x>0)--bits;
        else ++bits;
        std::memcpy(&x,&bits,sizeof x);
        return x;
    }

    inline double Up(double x) {
        if (std::isnan(x) || x==std::numeric_limits<double>::infinity()) return x;
        if (x==0) return std::numeric_limits<double>::denorm_min();
        std::uint64_t bits;
        std::memcpy(&bits,&x,sizeof bits);
        if (x>0)++bits;
        else --bits;
        std::memcpy(&x,&bits,sizeof x);
        return x;
    }

    struct Iv {
        double lo=0,hi=0;
        Iv()=default;
        Iv(double x):lo(x),hi(x){}
        Iv(double l,double h):lo(l),hi(h){}
    };

    inline Iv operator-(Iv a) {
        return {-a.hi,-a.lo};
    }

    inline Iv operator+(Iv a,Iv b) {
        if (a.lo==0 && a.hi==0) return b;
        if (b.lo==0 && b.hi==0) return a;
        if (a.lo==a.hi && b.lo==b.hi) {
            const double s=a.lo+b.lo, bv=s-a.lo;
            const double e=(a.lo-(s-bv))+(b.lo-bv);
            if (std::isfinite(s)) return {e<0?Down(s):s,e>0?Up(s):s};
        }

        return {Down(a.lo+b.lo),Up(a.hi+b.hi)};
    }

    inline Iv operator-(Iv a,Iv b) {
        return a+-b;
    }

    // Equivalent to ilogb(abs(a))+ilogb(abs(b)) >= -968 for the existing
    // guard. Normal operands use only the exponent fields. Subnormal/nonfinite
    // operands retain the original library calculation. No threshold changed.
    inline bool ResidualExponentGuard(double a, double b) {
        std::uint64_t ua, ub;
        std::memcpy(&ua, &a, sizeof ua);
        std::memcpy(&ub, &b, sizeof ub);
        const unsigned ea = unsigned((ua >> 52) & 0x7ffu);
        const unsigned eb = unsigned((ub >> 52) & 0x7ffu);
        if (ea > 0 && ea < 2047 && eb > 0 && eb < 2047)
            return ea + eb >= 1078u; // (ea-1023)+(eb-1023) >= -968.
        return std::ilogb(std::abs(a)) + std::ilogb(std::abs(b)) >= -968;
    }

    inline Iv operator*(Iv a,Iv b) {
        if ((a.lo==0&&a.hi==0)||(b.lo==0&&b.hi==0)) return Iv(0);
        if (a.lo==1&&a.hi==1)return b;
        if (b.lo==1&&b.hi==1)return a;
        if (a.lo==-1&&a.hi==-1)return -b;
        if (b.lo==-1&&b.hi==-1)return -a;
        if (a.lo==a.hi && b.lo==b.hi) {
            const double p=a.lo*b.lo,e=std::fma(a.lo,b.lo,-p);
            if (std::isfinite(p) && std::abs(p)>=std::numeric_limits<double>::min() &&
            ResidualExponentGuard(a.lo,b.lo))
            return {e<0?Down(p):p,e>0?Up(p):p};
        }

        if (std::isfinite(a.lo) && std::isfinite(a.hi) &&
            std::isfinite(b.lo) && std::isfinite(b.hi)) {
            double lo, hi;
            if (a.lo >= 0) {
                if (b.lo >= 0) { lo=a.lo*b.lo; hi=a.hi*b.hi; }
                else if (b.hi <= 0) { lo=a.hi*b.lo; hi=a.lo*b.hi; }
                else { lo=a.hi*b.lo; hi=a.hi*b.hi; }
            } else if (a.hi <= 0) {
                if (b.lo >= 0) { lo=a.lo*b.hi; hi=a.hi*b.lo; }
                else if (b.hi <= 0) { lo=a.hi*b.hi; hi=a.lo*b.lo; }
                else { lo=a.lo*b.hi; hi=a.lo*b.lo; }
            } else {
                if (b.lo >= 0) { lo=a.lo*b.hi; hi=a.hi*b.hi; }
                else if (b.hi <= 0) { lo=a.hi*b.lo; hi=a.lo*b.lo; }
                else {
                    lo=std::min(a.lo*b.hi,a.hi*b.lo);
                    hi=std::max(a.lo*b.lo,a.hi*b.hi);
                }
            }
            if (std::isfinite(lo) && std::isfinite(hi)) return {Down(lo), Up(hi)};
        }
        const std::array<double,4> v{a.lo*b.lo,a.lo*b.hi,a.hi*b.lo,a.hi*b.hi};
        return {Down(*std::min_element(v.begin(),v.end())),Up(*std::max_element(v.begin(),v.end()))};
    }

    inline Iv operator/(Iv a,Iv b) {
        if (b.lo<=0 && b.hi>=0) throw ArithmeticFailure();
        if (b.lo==1&&b.hi==1)return a;
        if (std::isfinite(a.lo) && std::isfinite(a.hi) &&
            std::isfinite(b.lo) && std::isfinite(b.hi)) {
            // Convert a negative denominator to the same positive-domain case.
            if (b.hi < 0) { a=-a; b=-b; }
            double lo, hi;
            if (a.lo >= 0) { lo=a.lo/b.hi; hi=a.hi/b.lo; }
            else if (a.hi <= 0) { lo=a.lo/b.lo; hi=a.hi/b.hi; }
            else { lo=a.lo/b.lo; hi=a.hi/b.lo; }
            return {Down(lo),Up(hi)};
        }
        const std::array<double,4> v{a.lo/b.lo,a.lo/b.hi,a.hi/b.lo,a.hi/b.hi};
        return {Down(*std::min_element(v.begin(),v.end())),Up(*std::max_element(v.begin(),v.end()))};
    }

    inline Iv Abs(Iv a) {
        if (a.lo>=0)return a;
        if (a.hi<=0)return -a;
        return {0,std::max(-a.lo,a.hi)};
    }

    inline Iv Positive(Iv a) {
        return {std::max(a.lo,0.0),std::max(a.hi,0.0)};
    }

    inline Iv Sqrt(Iv a) {
        if (a.hi<0)throw ArithmeticFailure();
        return {a.lo<=0?0:Down(std::sqrt(a.lo)),Up(std::sqrt(std::max(a.hi,0.0)))};
    }

    inline double Abs(double a){
        return std::abs(a);
    }

    inline double Positive(double a){
        return std::max(a,0.0);
    }

    // Nonoverlapping expansion; no fixed precision silently declares zero.
    struct Exact {
        std::vector<double> e;
        Exact()=default;
        Exact(double a) {
            if (!std::isfinite(a))throw ArithmeticFailure();
            if (a!=0)e.push_back(a);
        }

        void Grow(double b) {
            std::vector<double> out;
            out.reserve(e.size()+1);
            double q=b;
            for (double a:e) {
                const double s=q+a,bv=s-q,err=(q-(s-bv))+(a-bv);
                if (!std::isfinite(s))throw ArithmeticFailure();
                if (err!=0)out.push_back(err);
                q=s;
            }

            if (q!=0)out.push_back(q);
            e=std::move(out);
        }

        int Sign()const {
            return e.empty()?0:(e.back()>0?1:-1);
        }

        long double Value()const {
            long double x=0;
            for (double a:e)x+=static_cast<long double>(a);
            return x;
        }
    };

    inline Exact operator-(Exact a){
        for (double& x:a.e)x=-x;
        return a;
    }

    inline Exact operator+(Exact a,const Exact& b){
        for (double x:b.e)a.Grow(x);
        return a;
    }

    inline Exact operator-(Exact a,const Exact& b){
        for (double x:b.e)a.Grow(-x);
        return a;
    }

    inline Exact operator*(const Exact& a,const Exact& b){
        Exact r;
        for (double x:a.e)for (double y:b.e){
            const double p=x*y;
            if (!std::isfinite(p)||std::abs(p)<std::numeric_limits<double>::min())throw ArithmeticFailure();
            const double err=std::fma(x,y,-p);
            // In the supported normal-product range FMA's residual is exact unless
            // it underflows. Reject that range conservatively, including zero residual.
            if (std::ilogb(std::abs(x))+std::ilogb(std::abs(y)) < -968)throw ArithmeticFailure();
            if (err!=0)r.Grow(err);
            r.Grow(p);
        }

        return r;
    }

    inline Exact Abs(Exact a){
        return a.Sign()<0?-a:a;
    }

    inline Exact Positive(Exact a){
        return a.Sign()<0?Exact(0):a;
    }

    template<class T> using V=std::array<T,3>;
    template<class T> using M=std::array<V<T>,3>;
    // columns
    template<class T> V<T> Add(const V<T>&a,const V<T>&b){
        return {a[0]+b[0],a[1]+b[1],a[2]+b[2]};
    }

    template<class T> V<T> Sub(const V<T>&a,const V<T>&b){
        return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};
    }

    template<class T> V<T> Mul(const V<T>&a,const T&s){
        return {a[0]*s,a[1]*s,a[2]*s};
    }

    template<class T> T Dot(const V<T>&a,const V<T>&b){
        return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
    }

    template<class T> V<T> Cross(const V<T>&a,const V<T>&b){
        return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
    }

    template<class T> V<T> Transform(const M<T>&m,const V<T>&v){
        return Add(Add(Mul(m[0],v[0]),Mul(m[1],v[1])),Mul(m[2],v[2]));
    }

    template<class T> V<T> Vector(const glm::vec3&v){
        return {T(double(v.x)),T(double(v.y)),T(double(v.z))};
    }

    template<class T> struct Rotation {
        M<T> n;
        T d;
        explicit Rotation(const glm::quat&q){
            const T w(double(q.w)),x(double(q.x)),y(double(q.y)),z(double(q.z)),two(2);
            const T ww=w*w, xx=x*x, yy=y*y, zz=z*z;
            const T xy=x*y, wz=w*z, xz=x*z, wy=w*y, yz=y*z, wx=w*x;
            d=ww+xx+yy+zz;
            n={
                V<T>{ww+xx-yy-zz,two*(xy+wz),two*(xz-wy)},
                V<T>{two*(xy-wz),ww-xx+yy-zz,two*(yz+wx)},
                V<T>{two*(xz+wy),two*(yz-wx),ww-xx-yy+zz}
            };
        }
    };

    template<class T> struct Pair {
        Rotation<T> a,b;
        V<T> delta;
        T denominator;
        Pair(const ContactPose&pa,const ContactPose&pb):a(pa.orientation),b(pb.orientation){
            denominator=a.d*b.d;
            delta=Add(Mul(Sub(Vector<T>(pa.position),Vector<T>(pb.position)),denominator),
            Sub(Mul(Transform(a.n,Vector<T>(pa.localCenter)),b.d),Mul(Transform(b.n,Vector<T>(pb.localCenter)),a.d)));
        }
    };

    template<class T> V<T> Axis(const Pair<T>&p,int index){
        if (index<3)return p.a.n[index];
        if (index<6)return p.b.n[index-3];
        return Cross(p.a.n[(index-6)/3],p.b.n[(index-6)%3]);
    }

    template<class T> T Sat(const Pair<T>&p,const glm::vec3&ha,const glm::vec3&hb,int axis){
        const auto t=Axis(p,axis);
        T s=Abs(Dot(p.delta,t));
        for (int k=0;k<3;++k)s=s-T(double(ha[k]))*Abs(Dot(p.a.n[k],t))*p.b.d-T(double(hb[k]))*Abs(Dot(p.b.n[k],t))*p.a.d;
        return s;
    }

    template<class T> T SpherePredicate(const Pair<T>&p,const T& r){
        return Dot(p.delta,p.delta)-r*r*p.denominator*p.denominator;
    }

    template<class T> T SphereBoxPredicate(const Pair<T>&p,const glm::vec3&half,const T& r){
        const T h=p.denominator*p.b.d;
        T s(0);
        for (int k=0;k<3;++k){
            const T a=Positive(Abs(Dot(p.b.n[k],p.delta))-T(double(half[k]))*h);
            s=s+a*a;
        }

        return s-r*r*h*h;
    }
}

// namespace contact_geometry
