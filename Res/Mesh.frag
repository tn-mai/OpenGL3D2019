/**
* @file Sprite.frag
*/
#version 410
#extension AMD_gpu_shader_half_float : require
#extension AMD_gpu_shader_half_float_fetch : require

layout(location=0) in vec4 inColor;
layout(location=1) in vec2 inTexCoord;
layout(location=2) in vec3 inNormal;
layout(location=3) in vec3 inPosition;

out vec4 fragColor;

uniform sampler2D texColor;

/**
* スプライト用フラグメントシェーダー.
*/
void main()
{
  vec3 normal = inNormal;
  if (gl_FrontFacing == false) {
    normal *= -1;
  }

  fragColor = inColor * texture(texColor, inTexCoord);
  if (fragColor.a < 0.25) {
    discard;
  }
  vec3 vLight = normalize(vec3(1, -1.5, -1));
  float power = max(dot(normalize(normal), -vLight), 0.0) + 0.2;
  fragColor.rgb *= power;
}