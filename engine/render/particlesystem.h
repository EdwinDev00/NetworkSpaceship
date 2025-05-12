#pragma once
#include <vector>
#include "resourceid.h"
#include <gl/glew.h>

namespace Render
{

struct ParticleEmitter
{
    ParticleEmitter(uint32_t numParticles);

    ~ParticleEmitter();

    struct EmitterBlock
    {
        glm::vec4 origin = glm::vec4(0, 0, 0, 1); // where does particles spawn from?
        glm::vec4 dir = glm::vec4(1); // general direction of particle emitter cone
        glm::vec4 startColor = glm::vec4(1);
        glm::vec4 endColor = glm::vec4(1);
        uint32_t numParticles = 1024; // don't change in runtime!
        float theta = glm::radians(45.0f); // radians of emitter cone
        float startSpeed = 5.0f; // initial speed for each particle
        float endSpeed = 0.1f; // what's the speed when the particle dies?
        float startScale = 0.25f; // initial scale of each particle
        float endScale = 0.0f; // final scale of each particle
        float decayTime = 5.0f; // how long does each particle live?
        float randomTimeOffsetDist = 0.0f; // new particles will start with lifetime between 0 and this value.
        uint32_t looping = 0; // should the particle respawn after it dies?
        uint32_t fireOnce = 1; // set to true if you want to fire this particle system once. This will reset all particles to their initial states.
        uint32_t emitterType = 0; // 0 is spherical, 1 is from a circular disc with "dir" as normal and theta as spread.
        float discRadius; // only used if the emitterType is 1.
    } data;

    GLuint bufPositions[2]; // position.xyz and scale
    GLuint bufVelocities[2]; // velocity.xyz and lifetime
    GLuint bufColors[2]; // rgba - TODO: alpha should use stippling
};

class ParticleSystem
{
public:
    ParticleSystem() = default;
    static ParticleSystem* Instance()
    {
        static ParticleSystem instance;
        return &instance;
    }
public:
    ParticleSystem(const ParticleSystem&) = delete;
    void operator=(const ParticleSystem&) = delete;

    void Initialize();

    void AddEmitter(ParticleEmitter* emitter)
    {
        this->emitters.push_back(emitter);
    }

    void RemoveEmitter(ParticleEmitter* emitter)
    {
        this->emitters.erase(std::find(this->emitters.begin(), this->emitters.end(), emitter));
    }
        
private:
    std::vector<ParticleEmitter*> emitters;
    friend class RenderDevice;
    GLuint writeIndex = 0;
    Render::ShaderProgramId particleShaderId;
    Render::ShaderProgramId particleSimComputeShaderId;
    GLuint emitterBlockUBO;
};

}