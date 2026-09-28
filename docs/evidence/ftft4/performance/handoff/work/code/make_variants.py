from pathlib import Path
import hashlib,json
root=Path(__file__).resolve().parents[2]
src=root/'input/src/ContactGeometryInternal.h'
out=root/'work/generated';out.mkdir(exist_ok=True)
s=src.read_text()
old='std::ilogb(std::abs(a.lo))+std::ilogb(std::abs(b.lo))>=-968'
assert s.count(old)==1
helper='''    // Equivalent to ilogb(abs(a))+ilogb(abs(b)) >= -968 for the existing
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

'''
a=s.replace('    inline Iv operator*(Iv a,Iv b) {',helper+'    inline Iv operator*(Iv a,Iv b) {').replace(old,'ResidualExponentGuard(a.lo,b.lo)')
start='''            d=w*w+x*x+y*y+z*z;
            n={
                V<T>{w*w+x*x-y*y-z*z,two*(x*y+w*z),two*(x*z-w*y)},
                V<T>{two*(x*y-w*z),w*w-x*x+y*y-z*z,two*(y*z+w*x)},
                V<T>{two*(x*z+w*y),two*(y*z-w*x),w*w-x*x-y*y+z*z}
            };'''
replacement='''            const T ww=w*w, xx=x*x, yy=y*y, zz=z*z;
            const T xy=x*y, wz=w*z, xz=x*z, wy=w*y, yz=y*z, wx=w*x;
            d=ww+xx+yy+zz;
            n={
                V<T>{ww+xx-yy-zz,two*(xy+wz),two*(xz-wy)},
                V<T>{two*(xy-wz),ww-xx+yy-zz,two*(yz+wx)},
                V<T>{two*(xz+wy),two*(yz-wx),ww-xx-yy+zz}
            };'''
assert a.count(start)==1
b=a.replace(start,replacement)
pair_original='''        Pair(const ContactPose&pa,const ContactPose&pb):a(pa.orientation),b(pb.orientation){
            denominator=a.d*b.d;
            delta=Add(Mul(Sub(Vector<T>(pa.position),Vector<T>(pb.position)),denominator),
            Sub(Mul(Transform(a.n,Vector<T>(pa.localCenter)),b.d),Mul(Transform(b.n,Vector<T>(pb.localCenter)),a.d)));
        }'''
pair_new=pair_original+'''
        // Research-only prepared input overload. Lifetime invalidation is NOT
        // integrated into PhysicsWorld by this helper.
        Pair(const ContactPose&pa,const ContactPose&pb,
             const Rotation<T>&ra,const Rotation<T>&rb):a(ra),b(rb){
            denominator=a.d*b.d;
            delta=Add(Mul(Sub(Vector<T>(pa.position),Vector<T>(pb.position)),denominator),
            Sub(Mul(Transform(a.n,Vector<T>(pa.localCenter)),b.d),Mul(Transform(b.n,Vector<T>(pb.localCenter)),a.d)));
        }'''
assert b.count(pair_original)==1
c=b.replace(pair_original,pair_new)
for name,text in [('original',s),('exponent',a),('hoisted',b),('prepared',c)]:
 (out/(name+'.h')).write_text(text.replace('namespace contact_geometry','namespace '+name))
(out/'GENERATION.json').write_text(json.dumps({'input_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'transforms':['replace exponent query only','hoist identical quaternion products','add prepared-rotation constructor (research only)']},indent=2)+'\n')
# This proposed production patch contains only two local arithmetic changes.
(root/'work/code/ContactGeometryInternal_candidate.h').write_text(b)
# Standard sign-selected endpoint extrema, mathematically identical for finite
# intervals. Keep original fallback for nonfinite endpoint inputs.
slow_mul='''        const std::array<double,4> v{a.lo*b.lo,a.lo*b.hi,a.hi*b.lo,a.hi*b.hi};
        return {Down(*std::min_element(v.begin(),v.end())),Up(*std::max_element(v.begin(),v.end()))};'''
fast_mul='''        if (std::isfinite(a.lo) && std::isfinite(a.hi) &&
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
''' + slow_mul
slow_div='''        const std::array<double,4> v{a.lo/b.lo,a.lo/b.hi,a.hi/b.lo,a.hi/b.hi};
        return {Down(*std::min_element(v.begin(),v.end())),Up(*std::max_element(v.begin(),v.end()))};'''
fast_div='''        if (std::isfinite(a.lo) && std::isfinite(a.hi) &&
            std::isfinite(b.lo) && std::isfinite(b.hi)) {
            // Convert a negative denominator to the same positive-domain case.
            if (b.hi < 0) { a=-a; b=-b; }
            double lo, hi;
            if (a.lo >= 0) { lo=a.lo/b.hi; hi=a.hi/b.lo; }
            else if (a.hi <= 0) { lo=a.lo/b.lo; hi=a.hi/b.hi; }
            else { lo=a.lo/b.lo; hi=a.hi/b.lo; }
            return {Down(lo),Up(hi)};
        }
''' + slow_div
assert c.count(slow_mul)==1 and c.count(slow_div)==1
signed=c.replace(slow_mul,fast_mul).replace(slow_div,fast_div)
(out/'sign_selected.h').write_text(signed.replace('namespace contact_geometry','namespace sign_selected'))
(root/'work/code/ContactGeometryInternal_candidate.h').write_text(signed.replace(pair_new,pair_original))
