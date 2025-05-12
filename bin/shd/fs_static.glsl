#version 430

#include "shd/lights.glsl"

layout(location=0) in vec3 in_WorldSpacePos;
layout(location=1) in vec3 in_Normal;
layout(location=2) in vec4 in_Tangent;
layout(location=3) in vec2 in_TexCoords;

layout(location=0) out vec4 out_Color;

layout(location=0) uniform sampler2D BaseColorTexture;
layout(location=1) uniform sampler2D NormalTexture;
// GLTF 2.0 spec: metallicRoughness = Its green channel contains roughness values and its blue channel contains metalness values.
layout(location=2) uniform sampler2D MetallicRoughnessTexture;
layout(location=3) uniform sampler2D EmissiveTexture;
layout(location=4) uniform sampler2D OcclusionTexture;

layout(location=5) uniform vec4 BaseColorFactor;
layout(location=6) uniform vec4 EmissiveFactor;
layout(location=7) uniform float MetallicFactor;
layout(location=8) uniform float RoughnessFactor;
layout(location=9) uniform float AlphaCutoff;

layout(location=10) uniform vec4 CameraPosition;

layout(std430, binding = 3) readonly buffer VisiblePointLightIndicesBuffer
{
	VisibleIndex data[];
} visiblePointLightIndicesBuffer;

layout(std430, binding = 4) readonly buffer VisibleProbesIndicesBuffer
{
	VisibleIndex data[];
} visibleProbesIndicesBuffer;

// V = view vector, N = surface normal, P = fragment point in world space
vec3 CalculateGlobalLight(vec3 V, vec3 N, vec3 P, vec4 diffuseColor)
{
    float diffuse = max(dot(GlobalLightDirection, N), 0.0);
    vec4 shadowCoords = GlobalShadowMatrix * vec4(P, 1);
    shadowCoords.xyzw /= shadowCoords.w;
    shadowCoords = shadowCoords * 0.5f + 0.5f;
    float shadowDepth = texture(GlobalShadowMap,shadowCoords.xy).r;
    float geoDepth = shadowCoords.z;
    // bias based on incidence angle
    float bias = max(0.0003 * (1.0 - diffuse), 0.00035);  

    // simple PCF with 3x3 kernel for now
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(GlobalShadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float d = texture(GlobalShadowMap, shadowCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += geoDepth - bias > d ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;

    float shadowFactor = 1.0f - shadow;
    return shadowFactor * (GlobalLightColor * diffuse * diffuseColor.rgb);
}

vec3 CalcNormal(in vec4 tangent, in vec3 binormal, in vec3 normal, in vec3 bumpData)
{
    mat3 tangentViewMatrix = mat3(tangent.xyz, binormal.xyz, normal.xyz);
    return tangentViewMatrix * ((bumpData.xyz * 2.0f) - 1.0f);
}

const vec3 AmbientLight = vec3(0.15f);

void main()
{
	// Determine which tile this fragment belongs to
	ivec2 location = ivec2(gl_FragCoord.xy);
	ivec2 tileID = location / ivec2(TILE_SIZE, TILE_SIZE);
	uint index = tileID.y * NumTiles.x + tileID.x;

    vec4 baseColor = texture(BaseColorTexture, in_TexCoords).rgba * BaseColorFactor;
    baseColor = pow(baseColor, vec4(1.0f/2.2f));
    vec3 normal = texture(NormalTexture, in_TexCoords).xyz;
	vec2 metallicRoughness = texture(MetallicRoughnessTexture, in_TexCoords).xy;
	vec3 emissive = texture(EmissiveTexture, in_TexCoords).xyz;
	vec3 occlusion = texture(OcclusionTexture, in_TexCoords).xyz;
    
    vec3 V = normalize(CameraPosition.xyz - in_WorldSpacePos.xyz);
    vec3 binormal = cross(in_Normal, in_Tangent.xyz) * in_Tangent.w;
    vec3 N = (CalcNormal(in_Tangent, binormal, in_Normal, normal.xyz));

    vec3 light = AmbientLight;

    light += CalculateGlobalLight(V, N, in_WorldSpacePos, baseColor);

    uint offset = index * MaxTileLights;
    for (uint i = 0; i < MaxTileLights && visiblePointLightIndicesBuffer.data[offset + i].index != -1; i++)
	{
        uint lightIndex = visiblePointLightIndicesBuffer.data[offset + i].index;
		vec3 LightPos = pointLightPositionsBuffer.data[lightIndex].xyz;
        vec3 LightColor = pointLightColorsBuffer.data[lightIndex].rgb;
        float LightRadius = pointLightRadiiBuffer.data[lightIndex];
        
        vec3 L = LightPos - in_WorldSpacePos;

        float lightDistance = length(L);
        float x = lightDistance / LightRadius;
        float attenuation = -0.05 + 1.05/(1+23.0f*x*x);
        vec3 radiance = LightColor.rgb * max(attenuation, 0.0);
        
        float diffuse = max(dot(normalize(L), N), 0.0);
        
        light += diffuse * radiance;
    }

    out_Color = vec4(light.rgb * baseColor.rgb + emissive, 1.0f);
}