#version 430
layout(location=0) in vec3 in_Position;

layout(location=1) uniform mat4 InverseProjection;
layout(location=2) uniform mat4 InverseView;

layout(location=0) out vec3 out_SampleDir;

void main()
{
	vec3 viewPos = (InverseProjection * vec4(in_Position, 1.0f)).xyz;
    out_SampleDir = (mat3(InverseView) * viewPos).xyz;
	gl_Position = vec4(in_Position.xyz, 1.0f);
}
