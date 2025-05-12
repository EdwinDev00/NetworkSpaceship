#version 430
layout(location=0) in float dummy;

uniform mat4 viewProjection;
uniform vec4 v0pos;
uniform vec4 v1pos;
uniform vec4 v0color;
uniform vec4 v1color;

out vec4 fragColor;

void main()
{
	if (gl_VertexID == 0)
	{
		gl_Position = viewProjection * vec4(v0pos.xyz, 1.0f);
		fragColor = v0color;
	}
	else
	{
		gl_Position = viewProjection * vec4(v1pos.xyz, 1.0f);
		fragColor = v1color;
	}
}
