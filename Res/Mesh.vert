/**
* @file Mesh.vert
*/
#version 410
#extension AMD_gpu_shader_half_float : require
#extension AMD_gpu_shader_half_float_fetch : require

layout(location=0) in vec3 vPosition;
layout(location=1) in vec4 vColor;
layout(location=2) in vec2 vTexCoord;
layout(location=3) in vec3 vNormal;

layout(location=0) out vec4 outColor;
layout(location=1) out vec2 outTexCoord;
layout(location=2) out vec3 outNormal;
layout(location=3) out vec3 outPosition;

// global
uniform mat4x4 matVP;

// model local
uniform vec4 modelColor;

// per mesh
uniform mat4x4 matModel;
uniform mat3x3 matNormal;

// per primitive
uniform vec4 materialColor;

/**
* スプライト用頂点シェーダー.
*/
void main()
{
  outColor = vColor * modelColor;
  outTexCoord = vTexCoord;
  outNormal = matNormal * vNormal;
  outPosition = vec3(matModel * vec4(vPosition, 1.0));
  gl_Position = matVP * vec4(outPosition, 1.0);
}