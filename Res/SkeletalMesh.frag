/**
* @file Sprite.frag
*/
#version 410

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
  vec3 vLight = normalize(vec3(1, -2, -1));
  float power = dot(normalize(inNormal), -vLight);
  fragColor = inColor * texture(texColor, inTexCoord);
  fragColor.rgb *= power;
}