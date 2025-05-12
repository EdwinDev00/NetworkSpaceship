//------------------------------------------------------------------------------
//  renderdevice.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "renderdevice.h"
#include "model.h"
#include "textureresource.h"
#include "shaderresource.h"
#include "lightserver.h"
#include "cameramanager.h"
#include "debugrender.h"
#include "render/grid.h"
#include "core/random.h"
#include "core/cvar.h"
#include "core/random.h"
#include "particlesystem.h"

namespace Render
{

Render::ShaderProgramId directionalLightProgram;
Render::ShaderProgramId pointlightProgram;
Render::ShaderProgramId staticGeometryProgram;
Render::ShaderProgramId staticShadowProgram;
Render::ShaderProgramId skyboxProgram;
Render::ShaderProgramId lightCullingProgram;

GLuint fullscreenQuadVB;
GLuint fullscreenQuadVAO;

//------------------------------------------------------------------------------
/**
*/
RenderDevice::RenderDevice() :
    frameSizeW(1024),
    frameSizeH(1024)
{
    // empty
}

void SetupFullscreenQuad()
{
    const float verts[] = {
        3.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -3.0f, 1.0f
    };
    
    glGenBuffers(1, &fullscreenQuadVB);
    glGenVertexArrays(1, &fullscreenQuadVAO);
    glBindVertexArray(fullscreenQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, fullscreenQuadVB);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), (void*)&verts, GL_STATIC_DRAW);
    
    glEnableVertexArrayAttrib(fullscreenQuadVAO, 0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderDevice::Init()
{
    RenderDevice::Instance();
    CameraManager::Create();
    LightServer::Initialize();
    TextureResource::Create();
    
    SetupFullscreenQuad();
    
    // Shaders
    {
        auto vs = Render::ShaderResource::LoadShader(Render::ShaderResource::ShaderType::VERTEXSHADER, "shd/vs_static.glsl");
        auto fs = Render::ShaderResource::LoadShader(Render::ShaderResource::ShaderType::FRAGMENTSHADER, "shd/fs_static.glsl");
        staticGeometryProgram = Render::ShaderResource::CompileShaderProgram({ vs, fs });
    }
    {
        auto vs = Render::ShaderResource::LoadShader(Render::ShaderResource::ShaderType::VERTEXSHADER, "shd/vs_static_shadow.glsl");
        auto fs = Render::ShaderResource::LoadShader(Render::ShaderResource::ShaderType::FRAGMENTSHADER, "shd/fs_static_shadow.glsl");
        staticShadowProgram = Render::ShaderResource::CompileShaderProgram({ vs, fs });
    }
    {
        auto vs = Render::ShaderResource::LoadShader(Render::ShaderResource::ShaderType::VERTEXSHADER, "shd/vs_skybox.glsl");
        auto fs = Render::ShaderResource::LoadShader(Render::ShaderResource::ShaderType::FRAGMENTSHADER, "shd/fs_skybox.glsl");
        skyboxProgram = Render::ShaderResource::CompileShaderProgram({ vs, fs });
    }
    {
        auto vs = Render::ShaderResource::LoadShader(Render::ShaderResource::ShaderType::VERTEXSHADER, "shd/vs_fullscreen.glsl");
        auto fs = Render::ShaderResource::LoadShader(Render::ShaderResource::ShaderType::FRAGMENTSHADER, "shd/fs_directional_light.glsl");
        directionalLightProgram = Render::ShaderResource::CompileShaderProgram({ vs, fs });
    }
    {
        auto vs = Render::ShaderResource::LoadShader(Render::ShaderResource::ShaderType::VERTEXSHADER, "shd/vs_pointlight.glsl");
        auto fs = Render::ShaderResource::LoadShader(Render::ShaderResource::ShaderType::FRAGMENTSHADER, "shd/fs_pointlight.glsl");
        pointlightProgram = Render::ShaderResource::CompileShaderProgram({ vs, fs });
    }
    {
        auto cs = Render::ShaderResource::LoadShader(Render::ShaderResource::ShaderType::COMPUTESHADER, "shd/cs_lightculling.glsl");
        lightCullingProgram = Render::ShaderResource::CompileShaderProgram({ cs });
    }
    GLint dims[4] = { 0 };
    glGetIntegerv(GL_VIEWPORT, dims);
    // default viewport extents
    GLint fbWidth = dims[2];
    GLint fbHeight = dims[3];
    
    // Setup drawing buffers
    glGenFramebuffers(1, &Instance()->forwardFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, Instance()->forwardFrameBuffer );

    glGenTextures(2, Instance()->renderTargets.RT);

    glBindTexture(GL_TEXTURE_2D, Instance()->renderTargets.light);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, fbWidth, fbHeight, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glBindTexture(GL_TEXTURE_2D, Instance()->renderTargets.normal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, fbWidth, fbHeight, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glGenTextures(1, &Instance()->depthStencilBuffer);
    glBindTexture(GL_TEXTURE_2D, Instance()->depthStencilBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, fbWidth, fbHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Instance()->renderTargets.light, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, Instance()->depthStencilBuffer, 0);
    
    const GLenum drawbuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawbuffers);

    { GLenum err = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    assert(err == GL_FRAMEBUFFER_COMPLETE); }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    LightServer::UpdateWorkGroups(fbWidth, fbHeight);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    ParticleSystem::Instance()->Initialize();

    Debug::InitDebugRendering();
}

void RenderDevice::Draw(ModelId model, glm::mat4 localToWorld)
{
    Instance()->drawCommands.push_back({ model, localToWorld });
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDevice::StaticShadowPass()
{
    uint shadowMapSize = LightServer::GetShadowMapSize();
    glViewport(0, 0, shadowMapSize, shadowMapSize);
    glBindFramebuffer(GL_FRAMEBUFFER, LightServer::GetGlobalShadowFramebuffer());
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    Camera const* const mainCamera = CameraManager::GetCamera(CAMERA_MAIN);
    Camera* const shadowCamera = CameraManager::GetCamera(CAMERA_SHADOW);

    glm::vec3 shadowCamOffset = glm::normalize(LightServer::globalLightDirection) * 250.0f;
    glm::vec3 shadowCamTarget = glm::vec3(mainCamera->invView[3]);
    shadowCamTarget.y = 0.0f;
    shadowCamera->view = glm::lookAt(shadowCamTarget + shadowCamOffset,
        shadowCamTarget,
        glm::vec3(0.0f, 1.0f, 0.0f));

    CameraManager::UpdateCamera(shadowCamera);

    auto programHandle = Render::ShaderResource::GetProgramHandle(staticShadowProgram);
    glUseProgram(programHandle);
    glUniformMatrix4fv(glGetUniformLocation(programHandle, "ViewProjection"), 1, false, &shadowCamera->viewProjection[0][0]);

    GLuint baseColorFactorLocation = glGetUniformLocation(programHandle, "BaseColorFactor");
    GLuint modelLocation = glGetUniformLocation(programHandle, "Model");
    GLuint alphaCutoffLocation = glGetUniformLocation(programHandle, "AlphaCutoff");

    // Draw opaque first
    for (auto const& cmd : this->drawCommands)
    {
        Model const& model = GetModel(cmd.modelId);
        glUniformMatrix4fv(modelLocation, 1, false, &cmd.transform[0][0]);

        for (auto const& mesh : model.meshes)
        {
            for (auto& primitiveId : mesh.opaquePrimitives)
            {
                auto& primitive = mesh.primitives[primitiveId];

                glActiveTexture(GL_TEXTURE0 + Model::Material::TEXTURE_BASECOLOR);
                glBindTexture(GL_TEXTURE_2D, Render::TextureResource::GetTextureHandle(primitive.material.textures[Model::Material::TEXTURE_BASECOLOR]));
                glUniform1i(Model::Material::TEXTURE_BASECOLOR, Model::Material::TEXTURE_BASECOLOR);

                glUniform4fv(baseColorFactorLocation, 1, &primitive.material.baseColorFactor[0]);

                if (primitive.material.alphaMode == Model::Material::AlphaMode::Mask)
                    glUniform1f(alphaCutoffLocation, primitive.material.alphaCutoff);
                else
                    glUniform1f(alphaCutoffLocation, 0);

                glBindVertexArray(primitive.vao);
                glDrawElements(GL_TRIANGLES, primitive.numIndices, primitive.indexType, (void*)(intptr_t)primitive.offset);
            }
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDevice::StaticGeometryPrepass()
{
    Camera* const mainCamera = CameraManager::GetCamera(CAMERA_MAIN);
    glBindFramebuffer(GL_FRAMEBUFFER, Instance()->forwardFrameBuffer);
    glClearColor(255.0f, 0, 0, 1);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    // Models
    auto staticOpaquePrepassProgramHandle = Render::ShaderResource::GetProgramHandle(staticShadowProgram);
    glUseProgram(staticOpaquePrepassProgramHandle);
    glUniformMatrix4fv(glGetUniformLocation(staticOpaquePrepassProgramHandle, "ViewProjection"), 1, false, &mainCamera->viewProjection[0][0]);
    
    GLuint baseColorFactorLocation = glGetUniformLocation(staticOpaquePrepassProgramHandle, "BaseColorFactor");
    GLuint modelLocation = glGetUniformLocation(staticOpaquePrepassProgramHandle, "Model");
    GLuint alphaCutoffLocation = glGetUniformLocation(staticOpaquePrepassProgramHandle, "AlphaCutoff");

    // Opaque
    for (auto const& cmd : this->drawCommands)
    {
        Model const& model = GetModel(cmd.modelId);
        glUniformMatrix4fv(modelLocation, 1, false, &cmd.transform[0][0]);

        for (auto const& mesh : model.meshes)
        {
            for (auto& primitiveId : mesh.opaquePrimitives)
            {
                auto& primitive = mesh.primitives[primitiveId];

                glActiveTexture(GL_TEXTURE0 + Model::Material::TEXTURE_BASECOLOR);
                glBindTexture(GL_TEXTURE_2D, Render::TextureResource::GetTextureHandle(primitive.material.textures[Model::Material::TEXTURE_BASECOLOR]));
                glUniform1i(Model::Material::TEXTURE_BASECOLOR, Model::Material::TEXTURE_BASECOLOR);
                glUniform4fv(baseColorFactorLocation, 1, &primitive.material.baseColorFactor[0]);

                if (primitive.material.alphaMode == Model::Material::AlphaMode::Mask)
                    glUniform1f(alphaCutoffLocation, primitive.material.alphaCutoff);
                else
                    glUniform1f(alphaCutoffLocation, 0);

                glBindVertexArray(primitive.vao);
                glDrawElements(GL_TRIANGLES, primitive.numIndices, primitive.indexType, (void*)(intptr_t)primitive.offset);
            }
        }
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDevice::LightCullingPass()
{
    GLuint lightCullingProgramHandle = ShaderResource::GetProgramHandle(lightCullingProgram);
    glUseProgram(lightCullingProgramHandle);

    Camera* const mainCamera = CameraManager::GetCamera(CAMERA_MAIN);
    glUniformMatrix4fv(glGetUniformLocation(lightCullingProgramHandle, "View"), 1, false, &mainCamera->view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(lightCullingProgramHandle, "Projection"), 1, false, &mainCamera->projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(lightCullingProgramHandle, "ViewProjection"), 1, false, &mainCamera->viewProjection[0][0]);

    // Bind depth map texture to texture location 20 (which will not be used by any model texture)
    glActiveTexture(GL_TEXTURE30);
    glUniform1i(glGetUniformLocation(lightCullingProgramHandle, "DepthMap"), 30);
    glBindTexture(GL_TEXTURE_2D, depthStencilBuffer);

    glUniform1i(glGetUniformLocation(lightCullingProgramHandle, "NumPointLights"), LightServer::GetNumPointLights());
    
    // Bind shader storage buffer objects for the light and index buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, LightServer::GetBuffer(LightServer::PointLightBuffer::POSITIONS));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, LightServer::GetBuffer(LightServer::PointLightBuffer::RADII));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, LightServer::GetBuffer(LightServer::PointLightBuffer::VISIBLE_INDICES));
    
    glUniform2ui(glGetUniformLocation(lightCullingProgramHandle, "NumTiles"), LightServer::GetWorkGroupsX(), LightServer::GetWorkGroupsY());

    glDispatchCompute(LightServer::GetWorkGroupsX(), LightServer::GetWorkGroupsY(), 1);

    glActiveTexture(GL_TEXTURE30);
    glBindTexture(GL_TEXTURE_2D, 0);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDevice::StaticForwardPass()
{   
    glBindFramebuffer(GL_FRAMEBUFFER, Instance()->forwardFrameBuffer);

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    Camera* const mainCamera = CameraManager::GetCamera(CAMERA_MAIN);
    
    auto programHandle = Render::ShaderResource::GetProgramHandle(staticGeometryProgram);
    glUseProgram(programHandle);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, LightServer::GetBuffer(LightServer::PointLightBuffer::POSITIONS));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, LightServer::GetBuffer(LightServer::PointLightBuffer::COLORS));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, LightServer::GetBuffer(LightServer::PointLightBuffer::RADII));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, LightServer::GetBuffer(LightServer::PointLightBuffer::VISIBLE_INDICES));

    glUniform2ui(glGetUniformLocation(programHandle, "NumTiles"), LightServer::GetWorkGroupsX(), LightServer::GetWorkGroupsY());
    glUniformMatrix4fv(glGetUniformLocation(programHandle, "ViewProjection"), 1, false, &mainCamera->viewProjection[0][0]);
    
    glUniform4fv(glGetUniformLocation(programHandle, "CameraPosition"), 1, &mainCamera->view[3][0]);

    LightServer::Update(staticGeometryProgram);

    glActiveTexture(GL_TEXTURE16);
    glBindTexture(GL_TEXTURE_2D, LightServer::GetGlobalShadowMapHandle());
    glUniform1i(glGetUniformLocation(programHandle, "GlobalShadowMap"), 16);
    Camera* globalShadowCamera = CameraManager::GetCamera(CAMERA_SHADOW);
    glUniformMatrix4fv(glGetUniformLocation(programHandle, "GlobalShadowMatrix"), 1, false, &globalShadowCamera->viewProjection[0][0]);

    GLuint baseColorFactorLocation = glGetUniformLocation(programHandle, "BaseColorFactor");
    GLuint emissiveFactorLocation = glGetUniformLocation(programHandle, "EmissiveFactor");
    GLuint metallicFactorLocation = glGetUniformLocation(programHandle, "MetallicFactor");
    GLuint roughnessFactorLocation = glGetUniformLocation(programHandle, "RoughnessFactor");
    GLuint modelLocation = glGetUniformLocation(programHandle, "Model");
    GLuint alphaCutoffLocation = glGetUniformLocation(programHandle, "AlphaCutoff");

    // Draw opaque first
    for (auto const& cmd : this->drawCommands)
    {
        Model const& model = GetModel(cmd.modelId);
        glUniformMatrix4fv(modelLocation, 1, false, &cmd.transform[0][0]);

        for (auto const& mesh : model.meshes)
        {
            for (auto& primitiveId : mesh.opaquePrimitives)
            {
                auto& primitive = mesh.primitives[primitiveId];

                for (int i = 0; i < Model::Material::NUM_TEXTURES; i++)
                {
                    if (primitive.material.textures[i] != InvalidResourceId)
                    {
                        glActiveTexture(GL_TEXTURE0 + i);
                        glBindTexture(GL_TEXTURE_2D, Render::TextureResource::GetTextureHandle(primitive.material.textures[i]));
                        glUniform1i(i, i);
                    }
                }

                glUniform4fv(baseColorFactorLocation, 1, &primitive.material.baseColorFactor[0]);
                glUniform4fv(emissiveFactorLocation, 1, &primitive.material.emissiveFactor[0]);
                glUniform1f(metallicFactorLocation, primitive.material.metallicFactor);
                glUniform1f(roughnessFactorLocation, primitive.material.roughnessFactor);

                if (primitive.material.alphaMode == Model::Material::AlphaMode::Mask)
                    glUniform1f(alphaCutoffLocation, primitive.material.alphaCutoff);
                else
                    glUniform1f(alphaCutoffLocation, 0);

                glBindVertexArray(primitive.vao);
                glDrawElements(GL_TRIANGLES, primitive.numIndices, primitive.indexType, (void*)(intptr_t)primitive.offset);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDevice::SkyboxPass()
{
    Camera* const camera = CameraManager::GetCamera(CAMERA_MAIN);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    GLuint handle = Render::ShaderResource::GetProgramHandle(skyboxProgram);
    glUseProgram(handle);
    glBindVertexArray(fullscreenQuadVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, TextureResource::GetTextureHandle(skybox));
    glUniform1i(0, 0);
    glUniformMatrix4fv(1, 1, false, &camera->invProjection[0][0]);
    glUniformMatrix4fv(2, 1, false, &camera->invView[0][0]);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDepthFunc(GL_LESS);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDevice::ParticlePass(float dt)
{
    ParticleSystem* particles = ParticleSystem::Instance();
    GLuint simProgramHandle = ShaderResource::GetProgramHandle(particles->particleSimComputeShaderId);
    glUseProgram(simProgramHandle);
    glUniform1f(glGetUniformLocation(simProgramHandle, "TimeStep"), dt);
    
    uint32_t readIndex = (particles->writeIndex + 1) % 2;

    for (auto emitter : particles->emitters)
    {
        // Integrate particle dynamics
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, emitter->bufPositions[readIndex]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, emitter->bufColors[readIndex]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, emitter->bufVelocities[readIndex]);
        
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, emitter->bufPositions[particles->writeIndex]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, emitter->bufColors[particles->writeIndex]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, emitter->bufVelocities[particles->writeIndex]);

        glBindBuffer(GL_UNIFORM_BUFFER, particles->emitterBlockUBO);
        glBindBufferBase(GL_UNIFORM_BUFFER, 10, particles->emitterBlockUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(ParticleEmitter::EmitterBlock), &emitter->data, GL_STATIC_DRAW);

        glUniform3ui(glGetUniformLocation(simProgramHandle, "Random"), Core::FastRandom(), Core::FastRandom(), Core::FastRandom());

        const int numWorkGroups[3] = {
            emitter->data.numParticles / 1024,
            1,
            1
        };
        glDispatchCompute(numWorkGroups[0], numWorkGroups[1], numWorkGroups[2]);

        emitter->data.fireOnce = false;
    }

    Camera const* const mainCamera = CameraManager::GetCamera(CAMERA_MAIN);
    GLuint programHandle = ShaderResource::GetProgramHandle(particles->particleShaderId);
    glUseProgram(programHandle);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glm::mat4 billboardView = glm::mat4(
        { 1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 1, 0 },
        mainCamera->view[3]
    );
    glm::mat4 billboardViewProjection = mainCamera->projection * billboardView;

    glUniformMatrix4fv(glGetUniformLocation(programHandle, "ViewProjection"), 1, false, &mainCamera->viewProjection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(programHandle, "BillBoardViewProjection"), 1, false, &billboardViewProjection[0][0]);
    GLuint particleOffsetLoc = glGetUniformLocation(programHandle, "ParticleOffset");

    for (auto emitter : particles->emitters)
    { // DRAW
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, emitter->bufPositions[readIndex]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, emitter->bufColors[readIndex]);

        // Split drawcalls into smaller bits, since integer division on AMD cards is inaccurate
        int numVerts = emitter->data.numParticles * 6;
        const int numVertsPerDrawCall = 0x44580; // has to be divisible with 6
        int numDrawCalls = emitter->data.numParticles / numVertsPerDrawCall;
        int particleOffset = 0;
        while (numVerts > 0)
        {
            int drawVertCount = glm::min(numVerts, numVertsPerDrawCall);
            glUniform1i(particleOffsetLoc, particleOffset);
            glDrawArrays(GL_TRIANGLES, 0, drawVertCount);
            numVerts -= drawVertCount;
            particleOffset += drawVertCount / 6;
        }
    }

    glUseProgram(0);

    // swap doublebuffer particles index
    particles->writeIndex = readIndex;
}

void
Render::RenderDevice::FinalizePass(Display::Window* wnd)
{
    int w, h;
    wnd->GetSize(w, h);
    glViewport(0, 0, w, h);

    glBlitNamedFramebuffer(this->forwardFrameBuffer, 0, 0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDevice::Render(Display::Window* wnd, float dt)
{
    TextureResource::PollPendingTextureLoads();

    wnd->MakeCurrent();

    CameraManager::OnBeforeRender();
    LightServer::OnBeforeRender();

    // Begin depth prepass renderpass
    int w, h;
    wnd->GetSize(w, h);
    glViewport(0, 0, w, h);

    Instance()->StaticGeometryPrepass();
    // end depth prepass renderpass

    // begin light culling compute pass
    Instance()->LightCullingPass();
    // end lightculling compute pass

    // Begin sun shadowmap renderpass. this has a single subpass, with two subpass dependencies for layout transitions (shader-read -> depth-write, and back)
    Instance()->StaticShadowPass();
    // end sun shadowmap renderpass

    // begin forward shading renderpass
    glViewport(0, 0, w, h);

    Instance()->StaticForwardPass();
    
    if (Instance()->skybox != InvalidResourceId)
    {
        Instance()->SkyboxPass();
    }
    
    Instance()->ParticlePass(dt);

    // end forward shading renderpass

    // begin debug drawing renderpass
    Debug::DispatchDebugDrawing();
    LightServer::DebugDrawPointLights();
    // end debug drawing renderpass

    // begin finalization pass and present
    Instance()->FinalizePass(wnd);
    // end finalization pass and present

    Instance()->drawCommands.clear();
}

} // namespace Render
