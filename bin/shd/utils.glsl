float LinearizeDepth(float depth, mat4 projection)
{
    return (0.5 * projection[3][2]) / (depth + 0.5 * projection[2][2] - 0.5);
}

vec4 PixelToProjection(vec2 screenCoord, float depth)
{
    // we use DX depth range [0,1], for GL where depth is [-1,1], we would need depth * 2 - 1 too
    return vec4(screenCoord * 2.0f - 1.0f, depth * 2.0f - 1.0f, 1.0f);
}

vec4 PixelToView(vec2 screenCoord, float depth, mat4 invProjection)
{
    vec4 projectionSpace = PixelToProjection(screenCoord, depth);
    vec4 viewSpace = invProjection * projectionSpace;
    viewSpace /= viewSpace.w;
    return viewSpace;
}

vec4 PixelToWorld(vec2 screenCoord, float depth, mat4 invView, mat4 invProjection)
{
    vec4 viewSpace = PixelToView(screenCoord, depth, invProjection);
    return invView * viewSpace;
}

/*
    PCG 3d hash http://www.jcgt.org/published/0009/03/02/
    Visualize with:
        uvec3 rnd = hash_pcg3d(uvec3(voxelPos));
        out_Color = vec4(vec3(rnd) * (1.0/float(0xffffffffu)), 1.0);
*/
uvec3 hash_pcg3d(uvec3 v)
{
    v = v * 1664525u + 1013904223u;
    v.x += v.y*v.z;
    v.y += v.z*v.x;
    v.z += v.x*v.y;
    v ^= v >> 16u;
    v.x += v.y*v.z;
    v.y += v.z*v.x;
    v.z += v.x*v.y;
    return v;
}

// Spherical fibonacci https://dl.acm.org/doi/10.1145/2816795.2818131
// To generate a nearly uniform point distribution on the unit sphere of size N, do
// for (float i = 0.0; i < N; i += 1.0) {
//         float3 point = sphericalFibonacci(i,N);
// }
// The points go from y = +1 down to y = -1 in a spiral.
// To generate samples on the +y hemisphere, just stop before i > N/2.
vec3 SphericalFibonacci(float i, float n)
{
    const float C_PI = 3.14159265359f;
    const float C_PHI = sqrt(5.0f) * 0.5f + 0.5f;
    float mf = i * (C_PHI - 1) + (-floor(i*(C_PHI - 1)));
	float phi = 2 * C_PI * mf;
	float cosTheta = 1.0f - (2.0f * i + 1.0f) * (1.0f / n);
	float sinTheta = sqrt(clamp(1.0f - cosTheta * cosTheta, 0.0f, 1.0f));
	return vec3(
		cos(phi) * sinTheta,
		cosTheta,
        sin(phi) * sinTheta
		);
}

// Octahedron-normal vectors
vec2 OctWrap(vec2 v)
{
    return (1.0 - abs(v.yx)) * (all(greaterThan(v.xy, vec2(0.0))) ? 1.0 : -1.0);
}
 
 // Octahedron-normal vectors
vec2 EncodeOctahedronNormal(vec3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    n.xy = n.z >= 0.0 ? n.xy : OctWrap(n.xy);
    n.xy = n.xy * 0.5 + 0.5;
    return n.xy;
}
 
// Octahedron-normal vectors
vec3 DecodeOctahedronNormal(vec2 f)
{
    f = f * 2.0 - 1.0;
    // https://twitter.com/Stubbesaurus/status/937994790553227264
    vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = clamp(-n.z, 0.0f, 1.0f);
    n.xy += all(greaterThan(n.xy, vec2(0.0))) ? vec2(-t,-t) : vec2(t,t);
    return normalize(n);
}