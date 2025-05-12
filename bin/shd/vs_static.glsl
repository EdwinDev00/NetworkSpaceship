#version 430

layout(location=0) in vec3 in_Position;
layout(location=1) in vec3 in_Normal;
layout(location=2) in vec4 in_Tangent;
layout(location=3) in vec2 in_TexCoord_0;

layout(location=0) out vec3 out_WorldSpacePos;
layout(location=1) out vec3 out_Normal;
layout(location=2) out vec4 out_Tangent;
layout(location=3) out vec2 out_TexCoords;

uniform mat4 ViewProjection;
uniform mat4 Model;

invariant gl_Position;

void main()
{
	vec4 wPos = (Model * vec4(in_Position, 1.0f));
	out_WorldSpacePos = wPos.xyz;
	out_TexCoords = in_TexCoord_0;
	out_Tangent = vec4(normalize((Model * vec4(in_Tangent.xyz, 0)).xyz), in_Tangent.w);
    out_Normal = normalize((Model * vec4(in_Normal, 0)).xyz);
	gl_Position = ViewProjection * wPos;
}
