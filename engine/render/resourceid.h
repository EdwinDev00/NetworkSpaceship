#pragma once

namespace Render
{

using ResourceId = unsigned int;
constexpr ResourceId InvalidResourceId = UINT_MAX;

using ShaderResourceId = ResourceId;
using ShaderProgramId = ResourceId;

using ModelId = uint32_t;
using BrushGroupId = uint32_t;

using TextureResourceId = ResourceId;


}