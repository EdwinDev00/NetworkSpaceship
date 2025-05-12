#pragma once
//------------------------------------------------------------------------------
/**
    ShaderResource

    (C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resourceid.h"
#include <vector>
#include <string>
#include <gl/glew.h>

namespace Render
{

class ShaderResource
{
private:
    ShaderResource();
    static ShaderResource* Instance()
    {
        static ShaderResource instance;
        return &instance;
    }
public:
    ShaderResource(const ShaderResource&) = delete;
    void operator=(const ShaderResource&) = delete;

    enum ShaderType
    {
        VERTEXSHADER,
        FRAGMENTSHADER,
        GEOMETRYSHADER,
        COMPUTESHADER
    };

    static ShaderResourceId LoadShader(ShaderType type, const char* path);
    static ShaderProgramId CompileShaderProgram(std::vector<ShaderResourceId> const& shaders);

    static GLuint GetProgramHandle(ShaderProgramId);

    static void ReloadShaders();

private:
    //
    // ShaderResourceId
    std::vector<std::string> shaderSources;
    std::vector<GLuint> shaders;
    std::vector<ShaderType> shaderTypes;

    // ShaderProgramId
    std::vector<std::vector<ShaderResourceId>> programShaders;
    std::vector<GLuint> programs;
};


} // namespace Render
