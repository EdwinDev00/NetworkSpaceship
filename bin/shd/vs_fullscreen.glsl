#version 430
layout(location=0) in vec3 in_Position;

layout(location=0) out vec2 out_TexCoords;

void main()
{
	gl_Position = vec4(in_Position.xyz, 1.0f);
	out_TexCoords = (in_Position.xy + vec2(1.0f)) * 0.5f;
}
