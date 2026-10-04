#include "Tangents.h"
#include "../third_party/mikktspace/mikktspace.h"
#include <cmath>
#include <glm/geometric.hpp>
namespace {
MeshData& Mesh(const SMikkTSpaceContext* c){return *static_cast<MeshData*>(c->m_pUserData);}
MeshVertex& Vertex(const SMikkTSpaceContext* c,int face,int corner){return Mesh(c).vertices[size_t(face)*3+corner];}
int Faces(const SMikkTSpaceContext* c){return int(Mesh(c).vertices.size()/3);}
int Corners(const SMikkTSpaceContext*,int){return 3;}
void Position(const SMikkTSpaceContext* c,float v[],int f,int k){auto p=Vertex(c,f,k).position;for(int i=0;i<3;++i)v[i]=p[i];}
void Normal(const SMikkTSpaceContext* c,float v[],int f,int k){auto n=Vertex(c,f,k).normal;for(int i=0;i<3;++i)v[i]=n[i];}
void UV(const SMikkTSpaceContext* c,float v[],int f,int k){auto uv=Vertex(c,f,k).uv;v[0]=uv.x;v[1]=uv.y;}
void Tangent(const SMikkTSpaceContext* c,const float v[],float sign,int f,int k){auto& vertex=Vertex(c,f,k);glm::vec3 t(v[0],v[1],v[2]);if(!std::isfinite(t.x)||!std::isfinite(t.y)||!std::isfinite(t.z)||glm::dot(t,t)<1e-10f){auto n=vertex.normal;t=glm::normalize(glm::cross(abs(n.x)<.9f?glm::vec3(1,0,0):glm::vec3(0,0,1),n));}vertex.tangent=glm::vec4(t,sign);}
}
bool GenerateMeshTangents(MeshData& m){
 if(!m.indices.empty()){std::vector<MeshVertex> vertices;std::vector<MeshSkinVertex> skin;vertices.reserve(m.indices.size());if(!m.skinVertices.empty())skin.reserve(m.indices.size());for(auto i:m.indices){if(i>=m.vertices.size())return false;vertices.push_back(m.vertices[i]);if(!m.skinVertices.empty())skin.push_back(m.skinVertices[i]);}m.vertices=std::move(vertices);m.skinVertices=std::move(skin);for(size_t i=0;i<m.indices.size();++i)m.indices[i]=unsigned(i);}
 if(m.vertices.empty()||m.vertices.size()%3)return false;
 for(size_t i=0;i<m.vertices.size();i+=3){auto n=glm::cross(m.vertices[i+1].position-m.vertices[i].position,m.vertices[i+2].position-m.vertices[i].position);n=glm::dot(n,n)>1e-12f?glm::normalize(n):glm::vec3(0,0,1);for(size_t k=0;k<3;++k)if(glm::dot(m.vertices[i+k].normal,m.vertices[i+k].normal)<1e-12f)m.vertices[i+k].normal=n;}
 SMikkTSpaceInterface callbacks{};callbacks.m_getNumFaces=Faces;callbacks.m_getNumVerticesOfFace=Corners;callbacks.m_getPosition=Position;callbacks.m_getNormal=Normal;callbacks.m_getTexCoord=UV;callbacks.m_setTSpaceBasic=Tangent;
 SMikkTSpaceContext context{&callbacks,&m};bool ok=genTangSpaceDefault(&context)!=0;
 return ok;
}
