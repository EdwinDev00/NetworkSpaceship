#version 430
#include "shd/utils.glsl"

layout(location=0) in vec2 in_TexCoords;

out vec4 out_Color;

layout(location=0) uniform sampler2D BaseColorTexture;
layout(location=1) uniform sampler2D NormalTexture;
layout(location=2) uniform sampler2D MetallicRoughnessTexture;
layout(location=3) uniform sampler2D EmissiveTexture;
layout(location=4) uniform sampler2D DepthStencil;
layout(location=16) uniform sampler2D GlobalShadowMap;

uniform vec4 CameraPosition;
uniform mat4 InvView;
uniform mat4 InvProjection;

uniform mat4 GlobalShadowMatrix;
uniform vec3 GlobalLightDirection;
uniform vec3 GlobalLightColor;

vec3 CalcNormal(in vec4 tangent, in vec3 binormal, in vec3 normal, in vec3 bumpData)
{
    mat3 tangentViewMatrix = mat3(tangent.xyz, binormal.xyz, normal.xyz);
    return tangentViewMatrix * ((bumpData.xyz * 2.0f) - 1.0f);
}

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
    float bias = max(0.0003 * (1.0 - dot(GlobalLightDirection, N)), 0.00035);  

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
    return shadowFactor * (GlobalLightColor * 8.0f) * (diffuseColor.rgb * diffuse);
}

vec3 CalculateF0(in vec3 color, in float metallic, const vec3 def)
{
    // F0 as 0.04 will usually look good for all dielectric (non-metal) surfaces
	// F0 = vec3(0.04);
	// for metallic surfaces we interpolate between F0 and the albedo value with metallic value as our lerp weight
	return mix(def, color.rgb, metallic);
}

void main()
{
	vec4 baseColor = texture(BaseColorTexture,in_TexCoords).rgba;
    vec3 normal = texture(NormalTexture, in_TexCoords).xyz;
	vec2 metallicRoughness = texture(MetallicRoughnessTexture, in_TexCoords).xy;
    vec3 emissive = texture(EmissiveTexture, in_TexCoords).rgb;
    float depth = texture(DepthStencil, in_TexCoords).r;
    
    vec3 worldSpacePos = PixelToWorld(in_TexCoords, depth, InvView, InvProjection).xyz;

	vec3 V = normalize(CameraPosition.xyz - worldSpacePos.xyz);
    vec3 N = normal;
    
    float NdotV = clamp(dot(N, V), 0, 1);
    vec3 F0 = CalculateF0(baseColor.rgb, metallicRoughness.r, vec3(0.04));

    
    vec3 light = vec3(0, 0, 0);
    light += CalculateGlobalLight(V, N, worldSpacePos, baseColor);
    light += emissive.rgb;
    
    float ambient = 0.002f;
    
    light += baseColor.rgb * (vec3(ambient));

    out_Color = vec4(pow(light.rgb, vec3(0.45454545f)), 1.0f);
    gl_FragDepth = depth;
}