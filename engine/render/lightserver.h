#pragma once
//------------------------------------------------------------------------------
/**
	@file lightserver.h

	@copyright
	(C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "renderdevice.h"
#include "shaderresource.h"
#include "lightsources.h"
#include <vector>

#define CAMERA_SHADOW uint('GSHW')

namespace Render
{

namespace LightServer
{

	extern glm::vec3 globalLightDirection;
	extern glm::vec3 globalLightColor;

	enum class PointLightBuffer
	{
		POSITIONS,
		COLORS,
		RADII,
		VISIBLE_INDICES,
		NUM_BUFFERS
	};

	void Initialize();
	void UpdateWorkGroups(uint resolutionWidth, uint resolutionHeight);
	void OnBeforeRender();
	void Update(Render::ShaderProgramId pid);

	void BindPointLightBuffers();

    void DebugDrawPointLights();

    bool IsValid(PointLightId id);
	PointLightId CreatePointLight(glm::vec3 position, glm::vec3 color, float intensity, float radius);
	void DestroyPointLight(PointLightId id);

    void SetPosition(PointLightId id, glm::vec3 position);
    glm::vec3 GetPosition(PointLightId id);
    void SetColorAndIntensity(PointLightId id, glm::vec3 color, float intensity);
    glm::vec3 GetColorAndIntensity(PointLightId id);
    void SetRadius(PointLightId id, float radius);
    float GetRadius(PointLightId id);

	GLuint GetBuffer(PointLightBuffer buf);
	
	GLuint GetWorkGroupsX();
	GLuint GetWorkGroupsY();

	size_t GetNumPointLights();

	GLuint GetGlobalShadowMapHandle();
	GLuint GetGlobalShadowFramebuffer();
	uint GetShadowMapSize();

};
} // namespace Render
