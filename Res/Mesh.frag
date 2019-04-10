/**
* @file Sprite.frag
*/
#version 410

layout(location=0) in vec4 inColor;
layout(location=1) in vec2 inTexCoord;
layout(location=2) in vec3 inNormal;

out vec4 fragColor;

uniform sampler2D texColor;

/**
* スプライト用フラグメントシェーダー.
*/
void main()
{
  fragColor = inColor * texture(texColor, inTexCoord);
}