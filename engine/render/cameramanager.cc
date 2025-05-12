//------------------------------------------------------------------------------
//  cameramanager.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "cameramanager.h"
#include <unordered_map>

namespace Render
{
namespace CameraManager
{
	//------------------------------------------------------------------------------
	/**
		This struct needs to be structured exactly like the Camera struct, just
		with different qualifiers.
	*/
	struct CameraState
	{
		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 invView;
		glm::mat4 invProjection;
		glm::mat4 viewProjection;
		glm::mat4 invViewProjection;
	};

	/// cameramanager singleton state
	struct State
	{
		CameraState cameras[32];
		unsigned char numCameras = 0;

		std::unordered_map<uint32_t, uint32_t> cameraTable;
	};

	static State* state = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
CameraManager::Create()
{
	assert(state == nullptr);
	state = new(State);

	// setup default cameras
	CameraCreateInfo cameraInfo;
	cameraInfo.hash = CAMERA_MAIN;
	CreateCamera(cameraInfo);
}

CameraManager::CameraState DeriveCameraState(glm::mat4 view, glm::mat4 projection)
{
	CameraManager::CameraState camera;
	camera.view = view;
	camera.projection = projection;
	camera.invView = inverse(view);
	camera.invProjection = inverse(projection);
	camera.viewProjection = projection * view;
	camera.invViewProjection = inverse(camera.viewProjection);
	return camera;
}

//------------------------------------------------------------------------------
/**
*/
Camera* const
CameraManager::CreateCamera(CameraCreateInfo const& info)
{
	state->cameraTable.emplace(info.hash, state->numCameras);
	assert(state->numCameras + 1 < 32);
	CameraState& camera = state->cameras[state->numCameras++];
	camera = DeriveCameraState(info.view, info.projection);
	return reinterpret_cast<Camera*>(&camera);
}

//------------------------------------------------------------------------------
/**
*/
void
CameraManager::UpdateCamera(Camera* const camera)
{
	*reinterpret_cast<Render::CameraManager::CameraState*>(camera) = DeriveCameraState(camera->view, camera->projection);
}

//------------------------------------------------------------------------------
/**
*/
Camera* const
CameraManager::GetCamera(uint32_t CAMERA_HASH)
{
	return reinterpret_cast<Camera*>(&state->cameras[state->cameraTable[CAMERA_HASH]]);
}

//------------------------------------------------------------------------------
/**
*/
void
CameraManager::Destroy()
{
	n_assert(state != nullptr);
	delete state;
	state = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
CameraManager::OnBeforeRender()
{
	index_t i;
	for (i = 0; i < state->numCameras; i++)
	{
		glm::mat4 const view = state->cameras[i].view;
		glm::mat4 const projection = state->cameras[i].projection;
		state->cameras[i] = DeriveCameraState(view, projection);
	}
}

} // namespace Game
