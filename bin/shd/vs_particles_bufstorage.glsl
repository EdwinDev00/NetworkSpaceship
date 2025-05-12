#version 430
uniform mat4 ViewProjection;
uniform mat4 BillBoardViewProjection;

uniform int ParticleOffset;

out vec4 Color;

layout(std430, binding = 0) readonly buffer Positions
{
    vec4 ReadPosAndScale[];
};
layout(std430, binding = 1) readonly buffer ReadBlockColors
{
    vec4 ReadColors[];
};

const vec3 TriangleBaseVertices[] = {
	vec3(0.5, 0.5, 0),
	vec3(-0.5, 0.5, 0),
	vec3(-0.5, -0.5, 0),
	vec3(0.5, 0.5, 0),
	vec3(-0.5, -0.5, 0),
	vec3(0.5, -0.5, 0)
};

void main()
{
	int localIndex = gl_VertexID % 6;
	int index1D = ParticleOffset + gl_VertexID / 6;
	vec4 translation = vec4(ReadPosAndScale[index1D].xyz, 1);
	float scale = ReadPosAndScale[index1D].w;
	vec4 projectedVertexPos = BillBoardViewProjection * vec4(TriangleBaseVertices[localIndex] * scale,0);
	gl_Position = ViewProjection * translation + projectedVertexPos;
	Color = ReadColors[index1D];
}
