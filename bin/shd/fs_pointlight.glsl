#version 430

#include "shd/utils.glsl"

layout(location=0) in vec4 in_NDC;

out vec4 out_Color;

layout(location=0) uniform sampler2D BaseColorTexture;
layout(location=1) uniform sampler2D NormalTexture;
layout(location=2) uniform sampler2D MetallicRoughnessTexture;
layout(location=4) uniform sampler2D DepthStencil;

uniform vec4 CameraPosition;
uniform mat4 InvView;
uniform mat4 InvProjection;
uniform vec3 LightPos;
uniform vec3 LightColor;
uniform float LightRadius;

void main()
{
    vec2 texCoords = ((in_NDC.xy / in_NDC.w) + 1.0f) * 0.5f;
	vec4 baseColor = texture(BaseColorTexture,texCoords).rgba;
    vec3 normal = texture(NormalTexture, texCoords).xyz;
	vec2 metallicRoughness = texture(MetallicRoughnessTexture, texCoords).xy;
    float depth = texture(DepthStencil, texCoords).r;

    vec3 worldSpacePos = PixelToWorld(texCoords, depth, InvView, InvProjection).xyz;

    vec3 L = LightPos - worldSpacePos;
	vec3 V = normalize(CameraPosition.xyz - worldSpacePos.xyz);
    vec3 N = normal;
    
    float NdotV = clamp(dot(N, V), 0, 1);
    
    float lightDistance = length(L);
    float x = lightDistance / LightRadius;
    float attenuation = -0.05 + 1.05/(1+23.0f*x*x);
    //float attenuation = max(0, x);
    vec3 radiance = LightColor.rgb * attenuation;
    
    float diffuse = max(dot(L, N), 0.0);
    vec3 light = diffuse * radiance * baseColor.rgb;
    
    //out_Color = vec4(pow(light.rgb, vec3(0.45454545f)), 1.0f);
    out_Color = vec4(light.rgb, 1.0f);
    //out_Color = vec4(LightPos.xyz,1.0f);
}