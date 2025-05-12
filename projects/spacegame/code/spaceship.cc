#include "config.h"
#include "spaceship.h"
#include "input/inputserver.h"
#include "render/cameramanager.h"
#include "render/particlesystem.h"

#include "../engine/network/timer.h"
#include "network/network.h"
#include <chrono>
#include <gtx/string_cast.hpp>


using namespace Input;
using namespace glm;
using namespace Render;

namespace Game
{
#pragma region Synchronization (snapshot interpolator)
    void SnapshotInterpolator::SetTarget(const SnapShotState& state)
    {
        previous = target;
        target = state;
        interpolationTimer = 0.0f; //Reset the timer
        extrapolatedTime = 0.0f;
    }

    void SnapshotInterpolator::Update(float dt)
    {
        if(interpolationTimer < interpolationDuration)
        {
            float t = interpolationTimer / interpolationDuration;
            interpolated.position = glm::mix(previous.position, target.position,t);
            interpolated.orientation = glm::slerp(previous.orientation, target.orientation, t);
            interpolated.velocity = glm::mix(previous.velocity, target.velocity,t);

            interpolationTimer += dt;
        }
        else 
        {
            //Dead reckoning after interpolation window
            extrapolatedTime += dt;
            interpolated.position += interpolated.velocity * dt;
        }

        //OLD
        //interpolationTimer += dt;
        //float t = std::clamp(interpolationTimer / interpolationDuration, 0.0f, 1.0f);
        
        //interpolated.position = glm::mix(previous.position, target.position, t);
        //interpolated.orientation = glm::slerp(previous.orientation, target.orientation, t);
        //interpolated.velocity = glm::mix(previous.velocity, target.velocity, t);
        
        //if (t >= 1.0f) //extra interpolate (optional)
        //{
        //    float extrapolatedTime = interpolationTimer - interpolationDuration;
        //    interpolated.position += interpolated.velocity * extrapolatedTime;
        //}

    }
#pragma endregion

#pragma region Laser

    void ServerLaser::Update(float dt)
    {
        //previousPosition = position;
        glm::vec3 forward = orientation * glm::vec3(0.0f, 0.0f, 1.0f);
        position += forward * LASER_SPEED * dt;
        transform = glm::translate(position) * glm::mat4_cast(orientation);

    }

    std::optional<uint32_t> ServerLaser::CheckCollision(const std::unordered_map<uint32_t, Physics::ColliderId>& playerColliders)
    {
       /* glm::vec3 direction = normalize(position - previousPosition);
        float distance = glm::length(position - previousPosition);*/
        glm::vec3 forward = orientation * glm::vec3(0.0f, 0.0f, 1.0f); // or whatever your forward is
        const glm::vec3 rayStart = this->position - forward * 0.5f; 
        //float stepDistance = LASER_SPEED * 0.01667f;
        Physics::RaycastPayload payload = Physics::Raycast(rayStart, forward, 1.0f); //1.0f
        Debug::DrawLine(rayStart, rayStart + forward * 1.0f, 2.0f, glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1));

        if(payload.hit)
        {
            for(const auto& [id, p_collider]: playerColliders)
            {
                if (id == ownerID) continue; //Skip self check

                if (payload.collider == p_collider)
                {
                    //CHECK FOR IF WE ARE HITTING THE PLAYER PHYSICS COLLIDER
                    Debug::DrawDebugText("LASER HIT PLAYER", payload.hitPoint, vec4(1, 1, 1, 1));
                    std::cout << "SERVER: DETECT HIT COLLISION ON THE SERVER SHIP\n";   
                    return id;
                }
            }
            
            //HIT OTHER MARK THE LASER FOR DELETE
            std::cout << "SERVER: LASER HIT NON-PLAYER COLLIDER (env or other object)\n";
            Debug::DrawDebugText("LASER HIT OTHER", payload.hitPoint, vec4(1, 1, 1, 1));
            return UINT32_MAX;
        }

        return std::nullopt; //No match or hit
    }
#pragma endregion

#pragma region Client spaceship

    void ClientSpaceship::InitSpaceship() {
        //Load spaceship model
        model = LoadModel("assets/space/spaceship.glb");

        //Initalize particle emitters for thrusters
        uint32_t numParticles = 2048;
        particleEmitterLeft = new Render::ParticleEmitter(numParticles);
        particleEmitterRight = new Render::ParticleEmitter(numParticles);

        //Configure particle emitters for thrusters
        this->particleEmitterLeft->data = {
             .origin = glm::vec4(this->position + (vec3(this->transform[2]) * emitterOffset),1),
             .dir = glm::vec4(glm::vec3(-this->transform[2]), 0),
             .startColor = glm::vec4(0.38f, 0.76f, 0.95f, 1.0f) * 2.0f,
             .endColor = glm::vec4(0,0,0,1.0f),
             .numParticles = numParticles,
             .theta = glm::radians(0.0f),
             .startSpeed = 1.2f,
             .endSpeed = 0.0f,
             .startScale = 0.025f,
             .endScale = 0.0f,
             .decayTime = 2.58f,
             .randomTimeOffsetDist = 2.58f,
             .looping = 1,
             .emitterType = 1,
             .discRadius = 0.020f
        };

        this->particleEmitterRight->data = this->particleEmitterLeft->data;

        ParticleSystem::Instance()->AddEmitter(this->particleEmitterLeft);
        ParticleSystem::Instance()->AddEmitter(this->particleEmitterRight);
    }

    void ClientSpaceship::RemoveSpaceship()
    {
        if (particleEmitterLeft)
        {

            ParticleSystem::Instance()->RemoveEmitter(this->particleEmitterLeft);
            delete particleEmitterLeft;
            particleEmitterLeft = nullptr;
        }

        if (particleEmitterRight)
        {

            ParticleSystem::Instance()->RemoveEmitter(this->particleEmitterRight);
            delete particleEmitterRight;
            particleEmitterRight = nullptr;
        }
    }

    ClientSpaceship::ClientSpaceship(uint32_t uniqueID)
    {
        id = uniqueID;
        // InitSpaceship();
    }

    void ClientSpaceship::ProcessInput()
    {
        Keyboard* kbd = Input::GetDefaultKeyboard();
        this->inputState.ResetInputHistory();

        inputState.moveForward = kbd->held[Key::W];
        inputState.boost = kbd->held[Key::Shift];
        inputState.fire = kbd->pressed[Key::Space];

        inputState.rotX = kbd->held[Key::Left] ? 1.0f :
                          kbd->held[Key::Right] ? -1.0f : 0.0f;

        inputState.rotY = kbd->held[Key::Up] ? -1.0f :
                          kbd->held[Key::Down] ? 1.0f : 0.0f;

        inputState.rotZ = kbd->held[Key::A] ? -1.0f : 
                          kbd->held[Key::D] ? 1.0f : 0.0f;

        //Input bitmap
        unsigned short bitmap = 0;
        bitmap |= (kbd->held[Key::W] ? 1 << 0 : 0);
        bitmap |= (kbd->held[Key::A] ? 1 << 1 : 0);
        bitmap |= (kbd->held[Key::D] ? 1 << 2 : 0);
        bitmap |= (kbd->held[Key::Up] ? 1 << 3 : 0);
        bitmap |= (kbd->held[Key::Down] ? 1 << 4 : 0);
        bitmap |= (kbd->held[Key::Left] ? 1 << 5 : 0);
        bitmap |= (kbd->held[Key::Right] ? 1 << 6 : 0);
        bitmap |= (kbd->pressed[Key::Space] ? 1 << 7 : 0);
        bitmap |= (kbd->held[Key::Shift] ? 1 << 8 : 0);

        //Timestamp (UNIX epoc in ms)
        inputState.timeSet = Time::Now();
        inputState.bitmap = bitmap;
    }

    void ClientSpaceship::UpdateCamera(float dt)
    {
        cam = CameraManager::GetCamera(CAMERA_MAIN);
        const vec3 desiredCamPos = this->position + vec3(this->transform * vec4(0, camOffsetY, -4.0f, 0));
        this->camPos = this->camPos = mix(this->camPos, desiredCamPos, dt * cameraSmoothFactor);
        cam->view =
            lookAt(this->camPos,
                this->camPos + vec3(this->transform[2]),
                vec3(this->transform[1]));
    }

    void ClientSpaceship::UpdateLocally(float dt)
    {
        // predict input based movement
        if (inputState.moveForward) {
            float targetSpeed = inputState.boost ? boostSpeed : normalSpeed;
            currentSpeed = glm::mix(normalSpeed, targetSpeed, dt * accelerationFactor);
        }
        else {
            currentSpeed = glm::mix(currentSpeed, 0.0f, dt * accelerationFactor);
        }

        //Compute the predicted velocity
        glm::vec3 desiredVelocity = glm::vec3(0.0f, 0.0f, this->currentSpeed) * 10.0f;
        desiredVelocity = orientation * desiredVelocity;
        linearVelocity = glm::mix(linearVelocity, desiredVelocity, dt * accelerationFactor);

        //Apply the predicted position from input
        position += linearVelocity * dt;

        //Apply rotation input
        float rotationSpeed = 1.8f * dt;
        rotXSmooth = glm::mix(rotXSmooth, inputState.rotX * rotationSpeed, dt * cameraSmoothFactor);
        rotYSmooth = glm::mix(rotYSmooth, inputState.rotY * rotationSpeed, dt * cameraSmoothFactor);
        rotZSmooth = glm::mix(rotZSmooth, inputState.rotZ * rotationSpeed, dt * cameraSmoothFactor);

        glm::quat localRotation = glm::quat(glm::vec3(-rotYSmooth, rotXSmooth, rotZSmooth));
        orientation = normalize(orientation * localRotation);

        //interpolator for correction 
        interpolator.Update(dt);
        glm::vec3 c_position = interpolator.GetPosition();
        glm::quat c_orientation = interpolator.GetOrientation();
        glm::vec3 c_velocity = interpolator.GetVelocity();

        //Blend predicted position toward server (Smoothing)
        const float blendFactor = 0.1f; // how much we trust the server over local prediction
        position = glm::mix(position, c_position, blendFactor);
        orientation = glm::slerp(orientation, c_orientation, blendFactor);
        linearVelocity = glm::mix(linearVelocity, c_velocity, blendFactor);

        //Compute final transform matrix
        transform = glm::translate(position) * glm::mat4_cast(orientation) * glm::scale(glm::vec3(1.0f));

        const float thrusterPosOffset = 0.365f;
        this->particleEmitterLeft->data.origin = glm::vec4(vec3(this->position + (vec3(this->transform[0]) * -thrusterPosOffset)) + (vec3(this->transform[2]) * emitterOffset), 1);
        this->particleEmitterLeft->data.dir = glm::vec4(glm::vec3(-this->transform[2]), 0);
        this->particleEmitterRight->data.origin = glm::vec4(vec3(this->position + (vec3(this->transform[0]) * thrusterPosOffset)) + (vec3(this->transform[2]) * emitterOffset), 1);
        this->particleEmitterRight->data.dir = glm::vec4(glm::vec3(-this->transform[2]), 0);

        float t = (currentSpeed / this->normalSpeed);
        this->particleEmitterLeft->data.startSpeed = 1.2 + (3.0f * t);
        this->particleEmitterLeft->data.endSpeed = 0.0f + (3.0f * t);
        this->particleEmitterRight->data.startSpeed = 1.2 + (3.0f * t);
        this->particleEmitterRight->data.endSpeed = 0.0f + (3.0f * t);

        //Debug drawline forward direction
       // glm::vec3 fwd = orientation * glm::vec3(0, 0, 1);
       // Debug::DrawLine(position, position + fwd * 1.5f, 2, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)); // forward direction (green)
    }

    void ClientSpaceship::CorrectFromServer(glm::vec3 newPos, glm::quat newOrient, glm::vec3 newVel, uint64_t timestamp)
    {
        interpolator.SetTarget({
                    newPos,
                    newOrient,
                    newVel,
                    timestamp
            });
    }
#pragma endregion

#pragma region Server spaceship

    void ServerSpaceship::Update(float dt)
    {
        inputCooldown += dt;
        const float inputTimeout = 0.2f; // 200ms without input = stop

        if (inputCooldown > inputTimeout)
        {
            lastInputBitmap = 0;
            lastInputTimeStamp = 0;
        }

        //Movement input
        bool forward = lastInputBitmap & (1 << 0);
        bool boost = lastInputBitmap & (1 << 8);

        // Rotation input
        float rotX = (lastInputBitmap & (1 << 6)) ? -1.0f : (lastInputBitmap & (1 << 5)) ? 1.0f : 0.0f;
        float rotY = (lastInputBitmap & (1 << 4)) ? -1.0f : (lastInputBitmap & (1 << 3)) ? 1.0f : 0.0f;
        float rotZ = (lastInputBitmap & (1 << 1)) ? -1.0f : (lastInputBitmap & (1 << 2)) ? 1.0f : 0.0f;

        if (forward)
            currentSpeed = boost ? boostSpeed : normalSpeed;
        else
            currentSpeed = 0;

        //Apply acceleration to velocity
        glm::vec3 desiredVelocity = glm::vec3(0, 0, currentSpeed * 10.0f);
        desiredVelocity = orientation * desiredVelocity; // Apply orientation to movement
        linearVelocity = glm::mix(linearVelocity, desiredVelocity, dt * accelerationFactor);

        //Update position
        position += linearVelocity * dt;

        //update rotation quat
        float rotationSpeed = 1.8f * dt;
        const float smoothFactor = 10;
        rotXSmooth = glm::mix(rotXSmooth, rotX * rotationSpeed, dt * smoothFactor);
        rotYSmooth = glm::mix(rotYSmooth, rotY * rotationSpeed, dt * smoothFactor);
        rotZSmooth = glm::mix(rotZSmooth, rotZ * rotationSpeed, dt * smoothFactor);

        glm::quat localRotation = glm::quat(glm::vec3(-rotYSmooth, rotXSmooth, rotZSmooth));
        orientation = normalize(orientation * localRotation);

        //Update transformation matrix
        transform = glm::translate(position) * glm::mat4_cast(orientation) * glm::scale(glm::vec3(1.0f));
    }

    bool ServerSpaceship::CheckCollision()
    {
        glm::mat4 rotation = glm::mat4(orientation);  //use transforms orientation for rotation
        glm::vec3 position = glm::vec3(transform[3]); //current position data inside the transform

        bool hit = false;
        for (int i = 0; i < 8; i++)
        {
            glm::vec3 direction = rotation * vec4(normalize(colliderEndPoints[i]), 0.0f);
            const float len = glm::length(colliderEndPoints[i]);
            const Physics::RaycastPayload payload = Physics::Raycast(position, direction, len);
           // Debug::DrawLine(position, position + direction * len, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);

            if (payload.hit)
            {
                Debug::DrawDebugText("HIT", payload.hitPoint, vec4(1, 1, 1, 1));
                std::cout << "SERVER: DETECT HIT COLLISION ON THE SERVER SHIP\n";
                hit = true;
            }
        }
        return hit;
    }
}

#pragma endregion