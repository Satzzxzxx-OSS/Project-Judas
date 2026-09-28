#pragma once
// Algebraic interval filter for the SAME homogeneous quaternion OBB geometry.
// No tolerance, normalization or uncertainty->contact rule is added.
// Parameter Math is provided by including this body under a namespace below.

template<class T> struct SatWorkspace {
    const Pair<T>& p;
    std::array<std::array<T,3>,3> ab;
    T da2,db2;
    explicit SatWorkspace(const Pair<T>& pair):p(pair),da2(p.a.d*p.a.d),db2(p.b.d*p.b.d){
        for(int i=0;i<3;++i)for(int j=0;j<3;++j)ab[i][j]=Dot(p.a.n[i],p.b.n[j]);
    }
    T Evaluate(const glm::vec3&ha,const glm::vec3&hb,int axis,const V<T>&t)const{
        T s=Abs(Dot(p.delta,t));
        if(axis<3){
            for(int k=0;k<3;++k){
                const T own = k==axis ? da2 : T(0);
                s=s-T(double(ha[k]))*own*p.b.d-T(double(hb[k]))*Abs(ab[axis][k])*p.a.d;
            }
        }else if(axis<6){
            const int j=axis-3;
            for(int k=0;k<3;++k){
                const T own = k==j ? db2 : T(0);
                s=s-T(double(ha[k]))*Abs(ab[k][j])*p.b.d-T(double(hb[k]))*own*p.a.d;
            }
        }else{
            const int i=(axis-6)/3,j=(axis-6)%3;
            for(int k=0;k<3;++k){
                const T aa=k==i ? T(0) : p.a.d*Abs(ab[3-k-i][j]);
                const T bb=k==j ? T(0) : p.b.d*Abs(ab[i][3-k-j]);
                s=s-T(double(ha[k]))*aa*p.b.d-T(double(hb[k]))*bb*p.a.d;
            }
        }
        return s;
    }
};
