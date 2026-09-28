#pragma once
// This is NOT a GLM replacement and must never be used to build Judas.
// The imported arithmetic header only reads vec3/quat fields; these storage-only
// carriers permit testing those actual scalar template bodies without GLM.
namespace glm {
struct vec3 {
 float x=0,y=0,z=0;
 vec3()=default; explicit vec3(float a):x(a),y(a),z(a){}
 vec3(float a,float b,float c):x(a),y(b),z(c){}
 float operator[](int i)const{return i==0?x:i==1?y:z;}
};
struct quat {
 float w=1,x=0,y=0,z=0;
 quat()=default;quat(float a,float b,float c,float d):w(a),x(b),y(c),z(d){}
};
}
struct ContactPose {glm::vec3 position;glm::quat orientation;glm::vec3 localCenter;};
