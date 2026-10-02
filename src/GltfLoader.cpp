#define GLM_ENABLE_EXPERIMENTAL
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include "GltfLoader.h"
#include "SkeletalAnimation.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <memory>
#include <functional>
#include <set>
#include <cmath>
#include <stdexcept>
namespace {
bool Finite(glm::vec4 v){for(int i=0;i<4;++i)if(!std::isfinite(v[i]))return false;return true;}
glm::vec4 Read(cgltf_accessor* a,size_t i){cgltf_float v[4]={0,0,0,0};if(!a||!cgltf_accessor_read_float(a,i,v,4))throw std::runtime_error("invalid attribute/animation accessor");glm::vec4 result(v[0],v[1],v[2],v[3]);if(!Finite(result))throw std::runtime_error("non-finite attribute/animation");return result;}
}
bool ParseGltfMesh(const void* bytes,size_t size,MeshData& result,std::string& error){
 cgltf_options options{};cgltf_data* raw=nullptr;auto status=cgltf_parse(&options,bytes,size,&raw);if(status!=cgltf_result_success){error="glTF parse failed ("+std::to_string(status)+")";return false;}
 std::unique_ptr<cgltf_data,decltype(&cgltf_free)> data(raw,cgltf_free);
 try{
  // Self-contained files keep normal asset identity, async cancellation and export
  // independent of unregistered auxiliary files or arbitrary external paths.
  for(size_t i=0;i<data->buffers_count;++i)if(data->buffers[i].uri&&std::string(data->buffers[i].uri).rfind("data:",0)!=0)throw std::runtime_error("external glTF buffers unsupported; embed buffers or use GLB");
  if(cgltf_load_buffers(&options,data.get(),nullptr)!=cgltf_result_success||cgltf_validate(data.get())!=cgltf_result_success)throw std::runtime_error("invalid glTF buffers/accessors");
  if(data->skins_count!=1||data->nodes_count>128||data->skins[0].joints_count==0||data->skins[0].joints_count>48)throw std::runtime_error("requires one skin, 1..48 skin joints and <=128 hierarchy nodes");
  auto asset=std::make_shared<SkeletalAsset>();auto& skeleton=asset->skeleton;auto nodeIndex=[&](const cgltf_node* n){return int(n-data->nodes);};
  std::vector<unsigned char> visited(data->nodes_count,0);
  std::function<void(int)> visit=[&](int i){if(i<0||size_t(i)>=data->nodes_count)throw std::runtime_error("invalid hierarchy reference");if(visited[i]==1)throw std::runtime_error("cyclic skeleton");if(visited[i]==2)return;visited[i]=1;auto& n=data->nodes[i];if(n.parent)visit(nodeIndex(n.parent));visited[i]=2;skeleton.order.push_back(i);};
  for(size_t i=0;i<data->nodes_count;++i){const auto& n=data->nodes[i];JointTransform local;
   if(n.has_matrix){glm::mat4 m;cgltf_node_transform_local(&n,&m[0][0]);glm::vec3 skew;glm::vec4 perspective;if(!glm::decompose(m,local.scale,local.rotation,local.translation,skew,perspective)||glm::length(skew)>1e-5f)throw std::runtime_error("joint matrix contains unsupported shear");}
   else {if(n.has_translation)local.translation={n.translation[0],n.translation[1],n.translation[2]};if(n.has_scale)local.scale={n.scale[0],n.scale[1],n.scale[2]};if(n.has_rotation)local.rotation={n.rotation[3],n.rotation[0],n.rotation[1],n.rotation[2]};}
   if(!Finite({local.translation,0})||!Finite({local.scale,0})||!Finite({local.rotation.x,local.rotation.y,local.rotation.z,local.rotation.w})||glm::dot(local.rotation,local.rotation)<1e-12f)throw std::runtime_error("invalid rest transform");
   local.rotation=glm::normalize(local.rotation);skeleton.names.push_back(n.name?n.name:std::to_string(i));skeleton.parents.push_back(n.parent?nodeIndex(n.parent):-1);skeleton.rest.local.push_back(local);visit(int(i));
  }
  const auto& skin=data->skins[0];if(skin.inverse_bind_matrices&&skin.inverse_bind_matrices->count!=skin.joints_count)throw std::runtime_error("inverse bind count mismatch");
  for(size_t i=0;i<skin.joints_count;++i){glm::mat4 inverse(1);if(skin.inverse_bind_matrices&&!cgltf_accessor_read_float(skin.inverse_bind_matrices,i,&inverse[0][0],16))throw std::runtime_error("invalid inverse bind matrix");for(int c=0;c<4;++c)if(!Finite(inverse[c]))throw std::runtime_error("nonfinite inverse bind matrix");skeleton.skinNodes.push_back(nodeIndex(skin.joints[i]));skeleton.inverseBind.push_back(inverse);}
  cgltf_mesh* source=nullptr;for(size_t i=0;i<data->nodes_count;++i)if(data->nodes[i].mesh){if(source||data->nodes[i].skin!=&data->skins[0])throw std::runtime_error("requires a single skinned mesh node");source=data->nodes[i].mesh;}
  if(!source)throw std::runtime_error("missing skinned mesh");
  MeshData mesh;
  for(size_t p=0;p<source->primitives_count;++p){const auto& primitive=source->primitives[p];if(primitive.type!=cgltf_primitive_type_triangles||primitive.targets_count||primitive.has_draco_mesh_compression)throw std::runtime_error("requires uncompressed triangles without morph targets");
   cgltf_accessor *positions=nullptr,*normals=nullptr,*uv=nullptr,*joints=nullptr,*weights=nullptr;
   for(size_t a=0;a<primitive.attributes_count;++a){const auto& attr=primitive.attributes[a];if(attr.type==cgltf_attribute_type_position)positions=attr.data;else if(attr.type==cgltf_attribute_type_normal)normals=attr.data;else if(attr.type==cgltf_attribute_type_texcoord&&attr.index==0)uv=attr.data;else if(attr.type==cgltf_attribute_type_joints){if(attr.index!=0)throw std::runtime_error("more than four skin influences unsupported");joints=attr.data;}else if(attr.type==cgltf_attribute_type_weights){if(attr.index!=0)throw std::runtime_error("more than four skin influences unsupported");weights=attr.data;}}
   if(!positions||!joints||!weights||positions->type!=cgltf_type_vec3||joints->type!=cgltf_type_vec4||weights->type!=cgltf_type_vec4||joints->count!=positions->count||weights->count!=positions->count||(normals&&(normals->count!=positions->count||normals->type!=cgltf_type_vec3))||(uv&&(uv->count!=positions->count||uv->type!=cgltf_type_vec2)))throw std::runtime_error("incompatible skin vertex attributes");
   auto base=uint32_t(mesh.vertices.size());for(size_t i=0;i<positions->count;++i){MeshVertex v;MeshSkinVertex influence;v.position=glm::vec3(Read(positions,i));v.normal=normals?glm::vec3(Read(normals,i)):glm::vec3(0);v.uv=uv?glm::vec2(Read(uv,i)):glm::vec2(0);auto ji=Read(joints,i);influence.weights=Read(weights,i);float sum=0;for(int k=0;k<4;++k){if(ji[k]<0||ji[k]>=skin.joints_count||std::floor(ji[k])!=ji[k]||influence.weights[k]<0)throw std::runtime_error("invalid skin index/weight");influence.joints[k]=uint32_t(ji[k]);sum+=influence.weights[k];}if(!std::isfinite(sum)||sum<1e-8f)throw std::runtime_error("zero skin weights");influence.weights/=sum;mesh.vertices.push_back(v);mesh.skinVertices.push_back(influence);}
   size_t count=primitive.indices?primitive.indices->count:positions->count;if(count%3)throw std::runtime_error("invalid triangle count");for(size_t i=0;i<count;++i){size_t index=primitive.indices?cgltf_accessor_read_index(primitive.indices,i):i;if(index>=positions->count)throw std::runtime_error("mesh index out of range");mesh.indices.push_back(base+uint32_t(index));}
   if(!normals)for(size_t i=mesh.indices.size()-count;i<mesh.indices.size();i+=3){auto& a=mesh.vertices[mesh.indices[i]];auto& b=mesh.vertices[mesh.indices[i+1]];auto& c=mesh.vertices[mesh.indices[i+2]];auto n=glm::cross(b.position-a.position,c.position-a.position);a.normal+=n;b.normal+=n;c.normal+=n;}
  }
  for(auto& v:mesh.vertices)v.normal=glm::length(v.normal)>1e-8f?glm::normalize(v.normal):glm::vec3(0,1,0);
  std::set<std::string> clipNames;
  for(size_t i=0;i<data->animations_count;++i){const auto& animation=data->animations[i];AnimationClip clip;clip.name=animation.name?animation.name:"Clip "+std::to_string(i);if(!clipNames.insert(clip.name).second)throw std::runtime_error("duplicate animation names");std::set<std::pair<int,int>> channels;
   for(size_t c=0;c<animation.channels_count;++c){const auto& channel=animation.channels[c];if(!channel.target_node||channel.target_node->has_matrix)throw std::runtime_error("invalid animation target; matrix nodes cannot receive TRS tracks");AnimationTrack t;t.node=nodeIndex(channel.target_node);t.path=channel.target_path==cgltf_animation_path_type_translation?TrackPath::Translation:channel.target_path==cgltf_animation_path_type_rotation?TrackPath::Rotation:TrackPath::Scale;if(channel.target_path!=cgltf_animation_path_type_translation&&channel.target_path!=cgltf_animation_path_type_rotation&&channel.target_path!=cgltf_animation_path_type_scale)throw std::runtime_error("morph animation unsupported");if(!channels.insert({t.node,int(t.path)}).second)throw std::runtime_error("duplicate animation channel");
    const auto& sampler=*channel.sampler;t.interpolation=sampler.interpolation==cgltf_interpolation_type_step?TrackInterpolation::Step:sampler.interpolation==cgltf_interpolation_type_cubic_spline?TrackInterpolation::CubicSpline:TrackInterpolation::Linear;
    if(sampler.input->type!=cgltf_type_scalar||sampler.output->type!=(t.path==TrackPath::Rotation?cgltf_type_vec4:cgltf_type_vec3)||sampler.output->count!=sampler.input->count*(t.interpolation==TrackInterpolation::CubicSpline?3:1)||!sampler.input->count)throw std::runtime_error("invalid animation sample dimensions");
    for(size_t k=0;k<sampler.input->count;++k){float time=Read(sampler.input,k).x;if(time<0||(!t.times.empty()&&time<=t.times.back()))throw std::runtime_error("animation times must strictly increase");t.times.push_back(time);clip.duration=std::max(clip.duration,time);}
    for(size_t k=0;k<sampler.output->count;++k){auto v=Read(sampler.output,k);if(t.path==TrackPath::Rotation&&(t.interpolation!=TrackInterpolation::CubicSpline||k%3==1)&&glm::dot(v,v)<1e-12f)throw std::runtime_error("zero rotation key");if(t.path==TrackPath::Rotation&&t.interpolation!=TrackInterpolation::CubicSpline)v=glm::normalize(v);t.values.push_back(v);}clip.tracks.push_back(std::move(t));
   }asset->clips.push_back(std::move(clip));
  }
  if(mesh.vertices.empty())throw std::runtime_error("empty skinned mesh");
  mesh.skeletal=std::move(asset);result=std::move(mesh);error.clear();return true;
 }catch(const std::exception& e){error=std::string("glTF: ")+e.what();return false;}
}
