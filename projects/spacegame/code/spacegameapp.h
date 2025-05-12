#pragma once
//------------------------------------------------------------------------------
/**
	Space game application

	(C) 20222 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/app.h"
#include "render/window.h"

#include "render/model.h"
#include "physics/physics.h"

#include <thread>

namespace Game
{
class SpaceGameApp : public Core::App
{
public:
	/// constructor
	SpaceGameApp() {};
	/// destructor
	~SpaceGameApp(){};

	/// open app
	bool Open();
	/// run app
	void Run();
	/// exit app
	void Exit();
private:

	Display::Window* window;
	/// show some ui things
	void RenderUI();

	//list of objects 
	std::vector<std::pair<Render::ModelId, glm::mat4>> asteroids; //Rework client only handles the rendering part (server side handles gameplay events)
	char ipAddress[16] = "127.0.0.1";  // Default IP for localhost
};
} // namespace Game