#pragma once
//------------------------------------------------------------------------------
/**
	@file cameramanager.h

	@copyright
	(C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

namespace Render
{
#define CAMERA_MAIN uint32_t('DFCM')

//------------------------------------------------------------------------------
/**
	Used to create a camera.
*/
struct CameraCreateInfo
{
	/// camera hash number. Ex. 'ABC1'
	uint32_t hash = 0;
	glm::mat4 view;
	glm::mat4 projection;
};

//------------------------------------------------------------------------------
/**
*/
struct Camera
{
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 const invView;
	glm::mat4 const invProjection;
	glm::mat4 const viewProjection;
	glm::mat4 const invViewProjection;
};

//------------------------------------------------------------------------------
/**
*/
namespace CameraManager
{
	/// create the singleton
	void Create();

	/// create a camera. The global camera manager handles the cameras.
	Camera* const CreateCamera(CameraCreateInfo const& info);

	void UpdateCamera(Camera* const camera);

	/// get a camera by hash
	Camera* const GetCamera(uint32_t CAMERA_HASH);

	void Destroy();
	void OnBeforeRender();
};

} // namespace Game
