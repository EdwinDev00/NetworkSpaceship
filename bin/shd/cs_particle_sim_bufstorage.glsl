#version 430

#include "shd/utils.glsl"

layout(std430, binding = 0) readonly buffer ReadBlockPositions
{
    vec4 ReadPosAndScale[];
};
layout(std430, binding = 1) readonly buffer ReadBlockColors
{
    vec4 ReadColors[];
};
layout(std430, binding = 2) readonly buffer ReadBlockVelocities
{
    vec4 ReadVelAndLifetime[];
};

layout(std430, binding = 3) writeonly buffer WriteBlockPositions
{
    vec4 WritePosAndScale[];
};
layout(std430, binding = 4) writeonly buffer WriteBlockColors
{
    vec4 WriteColors[];
};
layout(std430, binding = 5) writeonly buffer WriteBlockVelocities
{
    vec4 WriteVelAndLifetimes[];
};

layout(binding = 10) uniform EmitterBlock
{
	vec4 origin;
	vec4 dir;
	vec4 startColor;
	vec4 endColor;
	uint numParticles;
	float theta;
	float startSpeed;
	float endSpeed;
	float startScale;
	float endScale;
	float decayTime;
	float randomTimeOffsetDist;
	uint looping;
	uint fireOnce;
	uint emitterType; // 0 is spherical, 1 is from a circular disc with "dir" as normal and theta as spread.
	float discRadius; // only used if the emitterType is 1.
};

uniform float TimeStep;
uniform uvec3 Random;

float random(vec2 st)
{
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

vec3 GetConeDir(vec3 dir, float theta)
{
	uint maxId = uint(abs((-(float(numParticles) * cos(theta) - 1.0f) + 1.0f) / 2.0f));
	vec3 v = SphericalFibonacci(float(gl_GlobalInvocationID.x % maxId), float(numParticles - (gl_GlobalInvocationID.x / maxId)));

	vec3 ww = dir;
	vec3 uu = normalize(cross(ww, vec3(0,1,0)));
	vec3 vv = normalize(cross(uu, ww));
	mat3 m  = mat3(uu, vv, ww);
	return m*v;
}

vec2 RandomOnUnitCircle(float radius)
{
    float a = random(vec2(float(gl_GlobalInvocationID.x) / float(numParticles), TimeStep));
	float b = random(vec2(TimeStep, float(gl_GlobalInvocationID.x) / float(numParticles)));
	if (b < a)
	{
		float c = b;
		b = a;
		a = c;
	}
    return vec2(b * radius * cos( 2 * 3.14159265f * a / b ), b * radius * sin( 2 * 3.14159265f * a / b ));
}

#define THREADS_X 1024
#define THREADS_Y 1
#define THREADS_Z 1
layout(local_size_x = THREADS_X, local_size_y = THREADS_Y, local_size_z = THREADS_Z) in;
void main()
{
	uint pid = gl_GlobalInvocationID.x;
	if (pid >= numParticles)
		return;
	
	vec3 pos = ReadPosAndScale[pid].xyz;
	vec3 moveDir = ReadVelAndLifetime[pid].xyz;
	float lifetime = ReadVelAndLifetime[pid].w;
	
	lifetime -= TimeStep;
	
	if ((lifetime <= 0 && looping > 0) || fireOnce > 0)
	{
		// reset particle
		float rnd = random(vec2(float(gl_GlobalInvocationID.x) / float(numParticles), TimeStep));
		lifetime = decayTime - (rnd * randomTimeOffsetDist);
		pos = origin.xyz;
		if (emitterType == 0) // Sphere emitter
		{
			vec3 v = SphericalFibonacci(float(gl_GlobalInvocationID.x), float(numParticles));
			moveDir = v;
		}
		else if (emitterType == 1)// Disc emitter
		{
			vec2 emitFrom = RandomOnUnitCircle(discRadius);
			
			float rnd0 = random(vec2(float(gl_GlobalInvocationID.x) / float(numParticles), TimeStep));
			float rnd1 = random(vec2(TimeStep, float(gl_GlobalInvocationID.x) / float(numParticles)));
			float angle = rnd0 * 3.14159265f * 2.0f;
			float radius = rnd1;
			vec2 p = vec2(cos(angle)*radius, sin(angle)*radius);

			p = normalize(p) * sin(theta);
			vec3 v = vec3(p,cos(theta));

			vec3 ww = dir.xyz;
			vec3 uu = normalize(cross(ww, vec3(0,1,0)));
			vec3 vv = normalize(cross(uu, ww));
			mat3 m  = mat3(uu, vv, ww);
			moveDir = m*v;
			pos += m*vec3(emitFrom, 0.0f);
		}
	}

	float t = 1.0f - (lifetime / decayTime);

	vec3 velocity = mix(moveDir * startSpeed, moveDir * endSpeed, t);
	pos += velocity * TimeStep;
	
	float scale = mix(startScale, endScale, t);
	vec4 col = mix(startColor, endColor, t);

	WritePosAndScale[pid] = vec4(pos, scale);
	WriteVelAndLifetimes[pid] = vec4(moveDir,lifetime);
	WriteColors[pid] = col;
}