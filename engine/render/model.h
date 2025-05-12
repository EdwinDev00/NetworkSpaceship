#pragma once
//------------------------------------------------------------------------------
/**
    Render::Model

    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "GL/glew.h"
#include <string>
#include <vector>
#include "renderdevice.h"
#include "resourceid.h"
#include "textureresource.h"

namespace Render
{

struct Model
{
    struct VertexAttribute
    {
        GLuint slot = 0;
        GLint components = 0;
        GLenum type = GL_NONE;
        GLsizei stride = 0;
        GLsizei offset = 0;
        GLboolean normalized = GL_FALSE;
    };

    struct Material
    {
        enum
        {
            TEXTURE_BASECOLOR,
            TEXTURE_NORMAL,
            TEXTURE_METALLICROUGHNESS,
            TEXTURE_EMISSIVE,
            TEXTURE_OCCLUSION,
            NUM_TEXTURES
        };

        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        glm::vec4 emissiveFactor = glm::vec4(1.0f);
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        TextureResourceId textures[NUM_TEXTURES] = {
            InvalidResourceId,
            InvalidResourceId,
            InvalidResourceId,
            InvalidResourceId,
            InvalidResourceId
        };

        enum class AlphaMode : uint8_t
        {
            Opaque,
            Mask,
            Blend
        };

        float alphaCutoff{ 0.5f };
        AlphaMode alphaMode{ AlphaMode::Opaque };
        bool doubleSided{ false };
    };

    struct Mesh
    {
        struct Primitive
        {
            GLuint vao;
            GLuint numIndices;
            GLuint offset = 0;
            GLenum indexType;
            Material material;
        };

        std::vector<Primitive> primitives;
        std::vector<uint16_t> opaquePrimitives; // contains ids of all primitives to be rendered opaque or mask mode
        std::vector<uint16_t> blendPrimitives; // contains ids of all primitives to be rendered with blend mode
    };

    std::vector<Mesh> meshes;
    //std::vector<TextureResourceId> textures;
    std::vector<GLuint> buffers;
    uint refcount;
};

ModelId LoadModel(std::string name);

void UnloadModel(ModelId);

bool const IsModelValid(ModelId);

Model const& GetModel(ModelId id);

} // namespace Render
