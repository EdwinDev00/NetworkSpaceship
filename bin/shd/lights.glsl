#define TILE_SIZE 32

struct VisibleIndex 
{
	int index;
};

layout(std430, binding = 0) readonly buffer PointLightPositionsBuffer
{
	vec4 data[];
} pointLightPositionsBuffer;

layout(std430, binding = 1) readonly buffer PointLightColorsBuffer
{
	vec4 data[];
} pointLightColorsBuffer;

layout(std430, binding = 2) readonly buffer PointLightRadiiBuffer
{
	float data[];
} pointLightRadiiBuffer;

layout(location=16) uniform sampler2D GlobalShadowMap;
layout(location=17) uniform mat4 GlobalShadowMatrix;
layout(location=18) uniform vec3 GlobalLightDirection;
layout(location=19) uniform vec3 GlobalLightColor;

// num tiles (work groups) in x,y for tiled forward binning
layout(location=20) uniform uvec2 NumTiles;

// number of lights in light buffers
uniform uint NumLights;

// Must be same as CPU side max lights constant
const uint MaxTileLights = 512;
