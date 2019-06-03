/**
* @file Sprite.vert
*/
#version 410
#extension AMD_gpu_shader_half_float : require
#extension AMD_gpu_shader_half_float_fetch : require

layout(location=0) in vec3 vPosition;
layout(location=1) in vec4 vColor;
layout(location=2) in vec2 vTexCoord;
layout(location=3) in vec3 vNormal;
layout(location=4) in vec4 vWeights;
layout(location=5) in vec4 vJoints;

layout(location=0) out vec4 outColor;
layout(location=1) out vec2 outTexCoord;
layout(location=2) out vec3 outNormal;
layout(location=3) out vec3 outPosition;

// global
uniform mat4x4 matVP;

// per mesh
layout(std140) uniform MeshMatrixUniformData
{
  vec4 color;
  mat3x4 matModel[4]; // it must transpose.
  mat3x4 matNormal[4]; // w isn't ussing. no need to transpose.
  mat3x4 matBones[256]; // it must transpose.
} vd;

// per primitive
uniform vec4 materialColor;
uniform int meshIndex;

/**
* スプライト用頂点シェーダー.
*/
void main()
{
  outColor = vColor;
  outTexCoord = vTexCoord;
  mat4 matModel = mat4(transpose(vd.matModel[meshIndex]));
  outNormal = transpose(inverse(mat3(matModel))) * vNormal;
  outPosition = vec3(matModel * vec4(vPosition, 1.0));
  gl_Position = matVP * (matModel * vec4(vPosition, 1.0));
}