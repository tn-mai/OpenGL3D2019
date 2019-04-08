#version 410

layout(location=0) in vec3 vPosition;
layout(location=1) in vec4 vColor;
layout(location=2) in vec2 vTexCoord;

layout(location=0) out vec2 outTexCoord;
layout(location=1) out vec4 outColor;

uniform mat4 matVP;

void main()
{
  outTexCoord = vTexCoord;
  outColor = vColor;
  gl_Position = matVP * vec4(vPosition.xy, 0, 1);
}