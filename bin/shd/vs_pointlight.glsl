#version 430

layout(location=0) in vec3 in_Position;

layout(location=0) out vec4 out_NDC;

uniform mat4 ViewProjection;
uniform vec3 LightPos;
uniform float LightRadius;

void main()
{
	vec4 wPos = vec4((in_Position * LightRadius) + LightPos, 1.0f);
	vec4 ndc = ViewProjection * wPos;
	out_NDC = ndc;
	gl_Position = ndc;
}
