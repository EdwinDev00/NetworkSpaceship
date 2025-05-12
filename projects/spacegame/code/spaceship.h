#pragma once
#include "render/model.h"
#include "physics/physics.h"
#include "render/debugrender.h"

#include <iostream>
#include <vec3.hpp>
#include <optional>

//Constant
#define LASER_SPEED 25.0f

namespace Render
{
    struct ParticleEmitter;
    struct Camera;
}

namespace Game
{

 // ==========================
 // Snapshot Interpolation (synchronization)
 // ==========================
struct SnapShotState
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat orientation = glm::identity<glm::quat>();
    glm::vec3 velocity = glm::vec3(0.0f);
    uint64_t timestamp = 0; //Server time of snapshot
};

class SnapshotInterpolator
{
public: 
    SnapshotInterpolator(float duration = 0.0833f) //12hz default (server running in 60 fps)
        : interpolationDuration(duration), interpolationTimer(0.0f) {}

    //Call this when receiving a new server update
    void SetTarget(const SnapShotState& state);
    void Update(float dt);

    const glm::vec3& GetPosition() const { return interpolated.position; }
    const glm::quat& GetOrientation() const { return interpolated.orientation; }
    const glm::vec3& GetVelocity() const { return interpolated.velocity; }
    float GetExtraInterpolatedTime() const { return extrapolatedTime; }

private:
    SnapShotState previous;
    SnapShotState target;
    SnapShotState interpolated;

    float interpolationDuration = 0.0833f;
    float interpolationTimer = 0.0f;
    float extrapolatedTime = 0.0f; //time since the interpolated ended
};

// ==========================
// Client Input State
// ==========================
struct InputState
{
    bool moveForward = false;
    bool boost = false;
    bool fire = false;
    float rotX = 0.0f, rotY = 0.0f, rotZ = 0.0f;

    uint64_t timeSet; //The time this was pressed and send
    unsigned short bitmap = 0;
    
    void ResetInputHistory()
    {
        moveForward = boost = fire = false;
        rotX = rotY = rotZ = 0.0f;
    }
};

// ==========================
// Client Spaceship
// ==========================
struct ClientSpaceship
{
    uint32_t id = 0;

    //Rendering
    Render::ModelId model;
    Render::ParticleEmitter* particleEmitterLeft;
    Render::ParticleEmitter* particleEmitterRight;

    //Camera 
    Render::Camera* cam;
    glm::vec3 camPos = glm::vec3(0, 1.0f, -2.0f);

    //State input
    InputState inputState;
    SnapshotInterpolator interpolator;

    // Predicted Movement (locally physics & movement)
    glm::vec3 position = glm::vec3(0);
    glm::quat orientation = glm::identity<glm::quat>();
    glm::vec3 linearVelocity = glm::vec3(0);
    glm::mat4 transform = glm::mat4(1);
 
    float currentSpeed = 0.0f;
    float emitterOffset = -0.5f;
    float rotationZ = 0.0f; 
    float rotXSmooth = 0.0f; 
    float rotYSmooth = 0.0f; 
    float rotZSmooth = 0.0f;

    const float normalSpeed = 10.0f;
    const float boostSpeed = normalSpeed * 2.0f;
    const float accelerationFactor = 1.0f;
    const float camOffsetY = 1.0f;
    const float cameraSmoothFactor = 10.0f;
    
    ClientSpaceship() = default;
    ClientSpaceship(uint32_t uniqueID);

    void InitSpaceship();
    void RemoveSpaceship(); //Remove the particle and id
    void ProcessInput();  // Handles input from player
    void UpdateCamera(float dt);  // Updates camera position
    void UpdateLocally(float dt); // Handles local client prediction
    void CorrectFromServer(glm::vec3 newPos, glm::quat newOrient, glm::vec3 newVel,uint64_t timestamp);  // Fixes desync
};

// ==========================
// Server Spaceship
// ==========================
struct ServerSpaceship
{
    uint32_t id; //UNique spaceship id for networking (synchronize with associated client ID)

    //Physics & movement
    glm::vec3 position = glm::vec3(0);
    glm::quat orientation = glm::identity<glm::quat>();
    glm::vec3 linearVelocity = glm::vec3(0);
    glm::mat4 transform = glm::mat4(1);

    float currentSpeed = 0.0f;
    float rotationZ = 0;
    float rotXSmooth = 0;
    float rotYSmooth = 0;
    float rotZSmooth = 0;

    float normalSpeed = 1.0f;
    float boostSpeed = normalSpeed * 2.0f;
    float accelerationFactor = 1.0f;

    uint16_t lastInputBitmap = 0;
    uint64_t lastInputTimeStamp = 0;
    float inputCooldown = 0;

    const glm::vec3 colliderEndPoints[8] = {
        glm::vec3(-1.10657, -0.480347, -0.346542),  // right wing
        glm::vec3(1.10657, -0.480347, -0.346542),  // left wing
        glm::vec3(-0.342382, 0.25109, -0.010299),   // right top
        glm::vec3(0.342382, 0.25109, -0.010299),   // left top
        glm::vec3(-0.285614, -0.10917, 0.869609), // right front
        glm::vec3(0.285614, -0.10917, 0.869609), // left front
        glm::vec3(-0.279064, -0.10917, -0.98846),   // right back
        glm::vec3(0.279064, -0.10917, -0.98846)   // left back
    };

    ServerSpaceship() = default;
    ServerSpaceship(uint32_t spaceshipID) : id(spaceshipID){}

    void Update(float dt); //updates spaceship movement
    bool CheckCollision(); //validate collisions 
};

// ==========================
// Laser
// ==========================
struct ClientLaser
{
    uint32_t uuid;
    uint64_t startTime;
    uint64_t endTime;

    glm::vec3 position;
    glm::quat orientation = glm::identity<glm::quat>();
    glm::mat4 transform = glm::identity<glm::mat4>();

    //TESTING THE SYNC LASER (CLIENT - SERVER TIME ) //WORK WITH THIS LATER WHEN TRYING ONLINE 
    uint64_t clientRecievedTime = 0;
    uint64_t serverSentTime = 0;
    float elapsedTime = 0; //THE TIME CALCULATED BETWEEN THEM ms to sec

    void updateLaserVisual(float dt)
    {
        glm::vec3 forward = orientation * glm::vec3(0.0f, 0.0f, 1.0f);
        position = position + forward * LASER_SPEED * dt;
        transform = glm::translate(position) * glm::mat4_cast(orientation);
    }
};

struct ServerLaser
{
    uint32_t uuid;
    uint32_t ownerID; 

    uint64_t startTime; //epoc ms when spawned
    uint64_t endTime; //epoc ms when it should despawn

    glm::vec3 position; //start position
    glm::vec3 previousPosition; 
    glm::quat orientation = glm::identity<glm::quat>(); //Direction as quaternion
    glm::mat4 transform = glm::identity<glm::mat4>();
    // glm::vec3 velocity; //computed from orientation

    void Update(float dt);
    std::optional<uint32_t> CheckCollision(const std::unordered_map<uint32_t, Physics::ColliderId>& playerColliders);

    bool isExpired(uint64_t now) const { return now >= endTime; }
};

}