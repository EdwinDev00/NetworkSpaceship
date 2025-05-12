#pragma once
//------------------------------------------------------------------------------
/**
    Render::TextureResource

    (C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "GL/glew.h"
#include "renderdevice.h"
#include "resourceid.h"

namespace Render
{

enum class TextureSemantics
{
    DIFFUSE_SEMANTIC = 0,
    NORMAL_SEMANTIC,
    SPECULAR_SEMANTIC,
    NumTextureSemantics
};

enum class ImageType
{
    RENDERBUFFER,
    TEXTURE_2D,
	DEPTH_STENCIL
};

//struct ImageView
//{
//    unsigned x0;
//    unsigned y0;
//    unsigned z0;
//    unsigned x1;
//    unsigned y1;
//    unsigned z1;
//};

struct ImageExtents
{
    unsigned w;
    unsigned h;
};

struct ImageCreateInfo
{
    ImageType type;
    ImageExtents extents;
};

enum class MagFilter : uint16_t
{
    None,
    Nearest = 9728,
    Linear = 9729
};

enum class MinFilter : uint16_t
{
    None,
    Nearest = 9728,
    Linear = 9729,
    NearestMipMapNearest = 9984,
    LinearMipMapNearest = 9985,
    NearestMipMapLinear = 9986,
    LinearMipMapLinear = 9987
};

enum class WrappingMode : uint16_t
{
    ClampToEdge = 33071,
    MirroredRepeat = 33648,
    Repeat = 10497
};

struct TextureLoadInfo
{
    std::string name;
    void* buffer;
    uint64_t bytes;
    MagFilter magFilter;
    MinFilter minFilter;
    WrappingMode wrappingModeS;
    WrappingMode wrappingModeT;
    bool sRGB = false; // is buffer data in sRGB color space?
    TextureResourceId placeholder = InvalidResourceId; // placeholder texture while the texture is being loaded
};

class TextureResource
{
private:
    TextureResource() = default;
    static TextureResource* instance;
    static TextureResource* Instance()
    {
        return instance;
    }
    
public:
    TextureResource(const TextureResource&) = delete;
    void operator=(const TextureResource&) = delete;

    // create singleton
    static void Create();
    // destroy singleton
    static void Destroy();

    static void PollPendingTextureLoads();

    // PNG or JPG
    static TextureResourceId LoadTextureFromMemory(TextureLoadInfo const& info);

    static TextureResourceId LoadTexture(const char* path, MagFilter mag, MinFilter min, WrappingMode wrapModeS, WrappingMode wrapModeT, bool sRGB);
    static TextureResourceId LoadTextureFromMemory(std::string name, void* buffer, uint64_t bytes, ImageId imageId, MagFilter, MinFilter, WrappingMode, WrappingMode, bool sRGB);
    
    static TextureResourceId LoadCubemap(std::string const& name, std::vector<const char*> const& paths, bool sRGB);

    static ImageId AllocateImage(ImageCreateInfo info);

    static ImageExtents GetImageExtents(ImageId id);

    static GLuint GetTextureHandle(TextureResourceId tid);
    static GLuint GetImageHandle(ImageId tid);

    static ImageId GetImageId(std::string name);

    static TextureResourceId GetWhiteTexture();
    static TextureResourceId GetBlackTexture();
    static TextureResourceId GetDefaultMetallicRoughnessTexture();
    static TextureResourceId GetDefaultNormalTexture();

    static void LoadTextureLibrary(std::string folder);

private:
    std::vector<GLuint> imageHandles;
    std::vector<ImageExtents> imageExtents;
    std::vector<ImageType> imageTypes;
    std::unordered_map<std::string, ImageId> imageRegistry;

    TextureResourceId whiteTexture = InvalidResourceId;
    TextureResourceId blackTexture = InvalidResourceId;
    TextureResourceId defaultMetallicRoughnessTexture = InvalidResourceId;
    TextureResourceId defaultNormalTexture = InvalidResourceId;
};

} // namespace Render
