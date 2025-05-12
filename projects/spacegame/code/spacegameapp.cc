//------------------------------------------------------------------------------
// spacegameapp.cc
// (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "spacegameapp.h"
#include <cstring>
#include "imgui.h"
#include "render/renderdevice.h"
#include "render/shaderresource.h"
#include "render/textureresource.h"
#include "render/model.h"
#include "render/cameramanager.h"
#include "render/lightserver.h"
#include "input/inputserver.h"

#include <vector>
#include "core/random.h"
#include "render/debugrender.h"

#include "core/cvar.h"

#include <chrono>
#include <thread>

#include "network/server.h"
#include "network/client.h"


using namespace Display;
using namespace Render;

namespace Game
{

bool
SpaceGameApp::Open()
{
	App::Open();
	this->window = new Display::Window;
    this->window->SetSize(500, 500);

    if (this->window->Open())
	{
		// set clear color to gray
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

        RenderDevice::Init();

		// set ui rendering function
		this->window->SetUiRender([this]()
		{
			this->RenderUI();
		});
        
        return true;
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
void
SpaceGameApp::Run()
{
    int w;
    int h;
    this->window->GetSize(w, h);
    glm::mat4 projection = glm::perspective(glm::radians(90.0f), float(w) / float(h), 0.01f, 1000.f);
    Camera* cam = CameraManager::GetCamera(CAMERA_MAIN);
    cam->projection = projection;

    // load all resources
    ModelId models[6] = {
        LoadModel("assets/space/Asteroid_1.glb"),
        LoadModel("assets/space/Asteroid_2.glb"),
        LoadModel("assets/space/Asteroid_3.glb"),
        LoadModel("assets/space/Asteroid_4.glb"),
        LoadModel("assets/space/Asteroid_5.glb"),
        LoadModel("assets/space/Asteroid_6.glb")
    };
    Physics::ColliderMeshId colliderMeshes[6] = {
        Physics::LoadColliderMesh("assets/space/Asteroid_1_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_2_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_3_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_4_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_5_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_6_physics.glb")
    };
    
    // set up asteroids
    auto generateAsteroids = [&](int count, float span)
    {
        for (int i = 0; i < count; i++)
        {
            size_t resourceIndex = (size_t)(Core::FastRandom() % 6);
            glm::vec3 translation = glm::vec3(
                Core::RandomFloatNTP() * span,
                Core::RandomFloatNTP() * span,
                Core::RandomFloatNTP() * span
            );
            glm::vec3 rotationAxis = normalize(translation);
            float rotation = translation.x;
            glm::mat4 transform = glm::rotate(rotation, rotationAxis) * glm::translate(translation);

            asteroids.emplace_back(models[resourceIndex], transform); //Client visual list

            //Server asteroid
            ServerAsteroid s_asteroid;
            s_asteroid.transform = transform;
            s_asteroid.colliderID = Physics::CreateCollider(colliderMeshes[resourceIndex], transform);
            gameServer.SetAsteroid(s_asteroid); // Push into server list
        }
    };

    generateAsteroids(100, 20.0f); //Near
    generateAsteroids(50, 80.0f); //Far

    // Setup skybox
    std::vector<const char*> skybox
    {
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png"
    };
    TextureResourceId skyboxId = TextureResource::LoadCubemap("skybox", skybox, true);
    RenderDevice::SetSkybox(skyboxId);
    
    Input::Keyboard* kbd = Input::GetDefaultKeyboard();

    const int numLights = 40;
    Render::PointLightId lights[numLights];
    // Setup lights
    for (int i = 0; i < numLights; i++)
    {
        glm::vec3 translation = glm::vec3(
            Core::RandomFloatNTP() * 20.0f,
            Core::RandomFloatNTP() * 20.0f,
            Core::RandomFloatNTP() * 20.0f
        );
        glm::vec3 color = glm::vec3(
            Core::RandomFloat(),
            Core::RandomFloat(),
            Core::RandomFloat()
        );
        lights[i] = Render::LightServer::CreatePointLight(translation, color, Core::RandomFloat() * 4.0f, 1.0f + (15 + Core::RandomFloat() * 10.0f));
    }

    
    //INITALIZE THE LASER MODEL
    const ModelId laserMOD = LoadModel("assets/space/laser.glb");

    float dt = 0.01667f; //frameTime
   // constexpr double targetFrameTime = 1.0f / 240.0f; // 60FPS
    
    // game loop
    while (this->window->IsOpen())
	{
        auto timeStart = std::chrono::steady_clock::now();

      //  gameServer.Run();
        gameClient.Update();
    
		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

        glfwSwapInterval(1); //VSYNC
      
        this->window->Update();

        if (kbd->pressed[Input::Key::Code::End])
        {
            ShaderResource::ReloadShaders();
        }


        // Store all drawcalls in the render device
        for ( auto const& [model, transform] : this->asteroids)
        {
            RenderDevice::Draw(model, transform);
        }


        for(auto& ship : gameClient.spaceships)
        {
            //Make sure only apply camera, client prediction for this controlled client 
            if (ship.first == gameClient.myPlayerID)
            {
                //update this current user controlled avatar
                ship.second.ProcessInput(); //Handle input for local player
                if (ship.second.inputState.bitmap != 0) //might  need to reroute this before using
                    gameClient.SendInput(packet::InputC2S(ship.second.inputState.timeSet, ship.second.inputState.bitmap));
                ship.second.UpdateLocally(dt); //Predict movement
                ship.second.UpdateCamera(dt); // only update the local player's camera
            }

            else
            {
                //update the other connected users movements
                ship.second.UpdateLocally(dt);
            }
            RenderDevice::Draw(ship.second.model, ship.second.transform);
        }

        for(auto& [id,laser] : gameClient.lasers)
        {
            glm::vec3 forward = laser.orientation * glm::vec3(0.0f, 0.0f, 1.0f); // or whatever your forward is

            laser.updateLaserVisual(dt);
            Debug::DrawLine(laser.position, laser.position + forward * 2.0f, 2.0f, glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 1, 1));
            RenderDevice::Draw(laserMOD, laser.transform);
        }
       
        // Execute the entire rendering pipeline
        RenderDevice::Render(this->window, dt);

		// transfer new frame to window
		this->window->SwapBuffers();

        auto timeEnd = std::chrono::steady_clock::now();
        dt = std::min(0.04f, std::chrono::duration<float>(timeEnd - timeStart).count());
        //double frameDuration = std::chrono::duration<double>(timeEnd - timeStart).count();

        ////Delay for fps limit
        //if(frameDuration < targetFrameTime)
        //{
        //    double sleepTime = targetFrameTime - frameDuration;
        //    std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
        //}

        if (kbd->pressed[Input::Key::Code::Escape])
            this->Exit();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
SpaceGameApp::Exit()
{
    gameServer.ShutdownServer();
    this->window->Close();
}

//------------------------------------------------------------------------------
/**
*/
void
SpaceGameApp::RenderUI()
{
	if (this->window->IsOpen())
	{
#ifndef NDEBUG
        //ImGui::Begin("Debug");
        //Core::CVar* r_draw_light_spheres = Core::CVarGet("r_draw_light_spheres");
        //int drawLightSpheres = Core::CVarReadInt(r_draw_light_spheres);
        //if (ImGui::Checkbox("Draw Light Spheres", (bool*)&drawLightSpheres))
        //    Core::CVarWriteInt(r_draw_light_spheres, drawLightSpheres);
        //
        //Core::CVar* r_draw_light_sphere_id = Core::CVarGet("r_draw_light_sphere_id");
        //int lightSphereId = Core::CVarReadInt(r_draw_light_sphere_id);
        //if (ImGui::InputInt("LightSphereId", (int*)&lightSphereId))
        //    Core::CVarWriteInt(r_draw_light_sphere_id, lightSphereId);
        //
        //ImGui::End();

        //Debug::DispatchDebugTextDrawing();
#endif // !DEBUG

        static bool connected = false;
        static char text[10000];
        ImGui::Begin("Network Control");
        ImGui::InputText("Server IP", ipAddress, sizeof(ipAddress));

        if(!connected)
        {
            if (ImGui::Button("Host"))
            {
                gameServer.StartServer(1234);
                std::thread serverThread([]
                    {
                        while(gameServer.live)
                            gameServer.Run(); 
                    });
                serverThread.detach();

                gameClient.Create();
                if (gameClient.ConnectToServer(ipAddress, 1234))
                {
                    //On success
                    connected = true;
                }
            }

            if (ImGui::Button("Connect"))
            {
                std::cout << "GAMEAPP: CLIENT REQUESTING A CONNECTION TO THE SERVER. NOTE: CLIENT CALL CONNECT() TO ALREADY INITALIZE SERVER\n";
                gameClient.Create();
                if (gameClient.ConnectToServer(ipAddress, 1234)) 
                    connected = true;
                else
                    std::cout << "GAMEAPP: Failed to send connection request to server, check if gameclient or gameServer is initalized correctly\n";
            }
        }
        else
        {
            for (auto& ship : gameClient.spaceships) {
                ImGui::DragFloat3("Ship position", &ship.second.position[0]);
                ImGui::DragFloat4("Ship orientation", &ship.second.orientation[0]);
            }

            if (ImGui::Button("Disconnect"))
            {
                std::cout << "GAMEAPP: CLIENT SEND A DISCONNECT REQUEST TO THE SERVER TO HANDLE\n";
                gameClient.DisconnectFromServer();
                for(auto& [id,ships] :gameClient.spaceships)
                {
                    ships.RemoveSpaceship();
                }
                gameClient.spaceships.clear();
                connected = false;
                
            }

            ImGui::Text("FPS: %.1f (Frame Time: %.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
        }

        ImGui::End();

        Debug::DispatchDebugTextDrawing();
	}
}

} // namespace Game