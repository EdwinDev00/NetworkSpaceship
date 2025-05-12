#version 430
#include "shd/utils.glsl"
#include "shd/lights.glsl"

uniform mat4 View;
uniform mat4 Projection;
uniform mat4 ViewProjection;

uniform int NumPointLights;

layout(std430, binding = 3) writeonly buffer VisiblePointLightIndicesBuffer
{
	VisibleIndex data[];
} visiblePointLightIndicesBuffer;

layout(std430, binding = 4) writeonly buffer VisibleProbesIndicesBuffer
{
	VisibleIndex data[];
} visibleProbesIndicesBuffer;

// Uniforms
layout(location=30) uniform sampler2D DepthMap;

// Shared values between all the threads a group
shared uint minDepthInt;
shared uint maxDepthInt;
shared uint visiblePointLightCount;
shared vec4 frustumPlanes[6];
// Shared local storage for visible indices, will be written out to the global buffer at the end
shared int visiblePointLightIndices[MaxTileLights];

layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;

void main() 
{
	ivec2 location = ivec2(gl_GlobalInvocationID.xy);
	ivec2 itemID = ivec2(gl_LocalInvocationID.xy);
	ivec2 tileID = ivec2(gl_WorkGroupID.xy);
	ivec2 tileNumber = ivec2(gl_NumWorkGroups.xy);
	uint index = tileID.y * tileNumber.x + tileID.x;
	uint threadCount = TILE_SIZE * TILE_SIZE;
	uint pointLightPassCount = (NumPointLights + threadCount - 1) / threadCount;
	
	// Initialize shared global values for depth and light count
	if (gl_LocalInvocationIndex == 0) 
	{
		minDepthInt = 0xFFFFFFFF;
		maxDepthInt = 0;
		visiblePointLightCount = 0;
	}

	barrier();

	// Step 1: Calculate the minimum and maximum depth values (from the depth buffer) for this group's tile
	float maxDepth, minDepth;
	float depth = texelFetch(DepthMap, location, 0).r;
	depth = LinearizeDepth(depth, Projection);

	// Convert depth to uint so we can do atomic min and max comparisons between the threads
	uint depthInt = floatBitsToUint(depth);
	atomicMin(minDepthInt, depthInt);
	atomicMax(maxDepthInt, depthInt);

	barrier();

	// Step 2: One thread should calculate the frustum planes to be used for this tile
	if (gl_LocalInvocationIndex == 0) 
	{
		// Convert the min and max across the entire tile back to float
		minDepth = uintBitsToFloat(minDepthInt);
		maxDepth = uintBitsToFloat(maxDepthInt);

		// Steps based on tile scale
		vec2 negativeStep = (2.0 * vec2(tileID)) / vec2(tileNumber);
		vec2 positiveStep = (2.0 * vec2(tileID + ivec2(1, 1))) / vec2(tileNumber);

		// Set up starting values for planes using steps and min and max z values
		frustumPlanes[0] = vec4(1.0, 0.0, 0.0, 1.0 - negativeStep.x); // Left
		frustumPlanes[1] = vec4(-1.0, 0.0, 0.0, -1.0 + positiveStep.x); // Right
		frustumPlanes[2] = vec4(0.0, 1.0, 0.0, 1.0 - negativeStep.y); // Bottom
		frustumPlanes[3] = vec4(0.0, -1.0, 0.0, -1.0 + positiveStep.y); // Top
		frustumPlanes[4] = vec4(0.0, 0.0, -1.0, -minDepth); // Near
		frustumPlanes[5] = vec4(0.0, 0.0, 1.0, maxDepth); // Far

		// Transform the first four planes
		frustumPlanes[0] *= ViewProjection;
		frustumPlanes[0] /= length(frustumPlanes[0].xyz);
		frustumPlanes[1] *= ViewProjection;
		frustumPlanes[1] /= length(frustumPlanes[1].xyz);
		frustumPlanes[2] *= ViewProjection;
		frustumPlanes[2] /= length(frustumPlanes[2].xyz);
		frustumPlanes[3] *= ViewProjection;
		frustumPlanes[3] /= length(frustumPlanes[3].xyz);
		
		// Transform the depth planes
		frustumPlanes[4] *= View;
		frustumPlanes[4] /= length(frustumPlanes[4].xyz);
		frustumPlanes[5] *= View;
		frustumPlanes[5] /= length(frustumPlanes[5].xyz);
	}

	barrier();

	// Step 3: Cull lights.
	// Parallelize the threads against the lights now.
	// Can handle TILE_SIZE*TILE_SIZE in parallel. Anymore lights than that and additional passes are performed
	
	// Point Light Culling
	for (uint i = 0; i < pointLightPassCount; i++) 
	{
		// Get the lightIndex to test for this thread / pass. If the index is >= light count, then this thread can stop testing lights
		uint lightIndex = i * threadCount + gl_LocalInvocationIndex;
		
		if (lightIndex >= NumPointLights) 
		{
			break;
		}

		vec4 position = pointLightPositionsBuffer.data[lightIndex];
		float radius = pointLightRadiiBuffer.data[lightIndex];

		// We check if the light exists in our frustum
		float distance = 0.0;
		for (uint j = 0; j < 6; j++) 
		{
			distance = dot(position, frustumPlanes[j]) + radius;

			// If one of the tests fails, then there is no intersection
			if (distance <= 0.0)
				break;
		}

		// If greater than zero, then it is a visible light
		if (distance > 0.0) 
		{
			// Add index to the shared array of visible indices
			uint offset = atomicAdd(visiblePointLightCount, 1);
			visiblePointLightIndices[offset] = int(lightIndex);
		}
	} 
	
	barrier();
	
	// One thread should fill the global light buffer
	if (gl_LocalInvocationIndex == 0) 
	{
		/// Point Lights
		uint offset = index * MaxTileLights; // Determine position in global buffer
		for (uint i = 0; i < visiblePointLightCount; i++) 
		{
			visiblePointLightIndicesBuffer.data[offset + i].index = visiblePointLightIndices[i];
		}

		if (visiblePointLightCount < MaxTileLights) 
		{
			// Unless we have totally filled the entire array, mark it's end with -1
			// Final shader step will use this to determine where to stop (without having to pass the light count)
			visiblePointLightIndicesBuffer.data[offset + visiblePointLightCount].index = -1;
		}
	}
}
