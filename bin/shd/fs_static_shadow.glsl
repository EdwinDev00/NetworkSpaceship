#version 430
layout(location=3) in vec2 in_TexCoords;

layout(location=0) uniform sampler2D BaseColorTexture;
uniform vec4 BaseColorFactor;
uniform float AlphaCutoff;

void main()
{
	vec4 diffuseColor = texture(BaseColorTexture, in_TexCoords).rgba * BaseColorFactor;
	if (diffuseColor.a <= AlphaCutoff)
		discard; // do not write depth
    return;
}