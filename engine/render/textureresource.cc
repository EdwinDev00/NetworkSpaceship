//------------------------------------------------------------------------------
//  textureresource.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "textureresource.h"
#include <cstring>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_NO_SIMD
#include "stb_image.h"
#include "stb_image_write.h"

#include <iostream>
#include <chrono>

#include <future>
#include <thread>

namespace Render
{

TextureResource* TextureResource::instance = nullptr;

struct LoadTask
{
    std::thread::id loadingThread;
    std::future<void> poll;
    std::function<void()> complete;
};

struct ImageLoadResult
{
    int w, h, channels;
    std::shared_ptr<unsigned char[]> data; // ??
};

static std::vector<LoadTask> loadingTasks;

//------------------------------------------------------------------------------
/**
*/
size_t
PendingTextureLoads()
{
    return loadingTasks.size();
}

//------------------------------------------------------------------------------
/**
*/
void
TextureResource::PollPendingTextureLoads()
{
    // Only allow for a certain number of completed tasks per poll,
    // otherwise we might stall because too many textures are finished at the same time
#if _DEBUG
    int count = 1; 
#else
    int count = 2;
#endif
    for (size_t i = 0; i < loadingTasks.size() && count > 0; i++)
    {
        LoadTask& item = loadingTasks[i];
        if (item.loadingThread != std::this_thread::get_id())
            continue;

        if (item.poll.valid())
        {
            if (item.poll.wait_for(std::chrono::seconds{ 0 }) == std::future_status::timeout)
                continue;
            item.poll.get();
        }

        // load complete, erase task
        item.complete();
        loadingTasks.erase(loadingTasks.begin() + i);
        i--;
        count--;
    }
}


//------------------------------------------------------------------------------
/**
*/
void
TextureResource::Create()
{
    assert(TextureResource::instance == nullptr);
    TextureResource::instance = new TextureResource();
    
    // setup default textures
    ImageCreateInfo info;
    info.extents = { 1,1 };
    info.type = Render::ImageType::TEXTURE_2D;
    ImageId imageIds[4] =
    {
        AllocateImage(info),
        AllocateImage(info),
        AllocateImage(info),
        AllocateImage(info)
    };

    byte colors[4][4] = {
        {255,255,255,255}, // white texture
        {0,0,0,0}, // black texture
        {0, 255, 0, 0}, // metallic roughness texture (g = roughness, b = metalness)
        {128,128,255,0} // flat normal texture
    };

    for (int i = 0; i < 4; i++)
    {
        GLuint const& imageHandle = instance->imageHandles[imageIds[i]];
        ImageExtents const& imageExtents = instance->imageExtents[imageIds[i]];

        glBindTexture(GL_TEXTURE_2D, imageHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        void* buffer = (void*)colors[i];
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageExtents.w, imageExtents.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    instance->whiteTexture = imageIds[0];
    instance->blackTexture = imageIds[1];
    instance->defaultMetallicRoughnessTexture = imageIds[2];
    instance->defaultNormalTexture = imageIds[3];
}

//------------------------------------------------------------------------------
/**
*/
void
TextureResource::Destroy()
{
    assert(TextureResource::instance != nullptr);
    // TODO: Resource cleanup
    delete TextureResource::instance;
}

//------------------------------------------------------------------------------
/**
*/
TextureResourceId
TextureResource::LoadTextureFromMemory(TextureLoadInfo const& info)
{
    // Make sure imageid isn't already loaded, otherwise generate new
    ImageId iid = GetImageId(info.name);
    if (iid != InvalidImageId)
        return iid;
    else
    {
        // Make sure the image returned is valid by copying the data from the placeholder image. This will be overriden later when the real dat has been loaded.
        ImageCreateInfo createInfo{};
        createInfo.type = ImageType::TEXTURE_2D;
        createInfo.extents = GetImageExtents(info.placeholder);
        iid = AllocateImage(createInfo);
        Instance()->imageRegistry.emplace(info.name, iid);

        GLuint handle = GetImageHandle(iid);
        
        glBindTexture(GL_TEXTURE_2D, handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLenum)info.minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLenum)info.magFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLenum)info.wrappingModeS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLenum)info.wrappingModeT);
        
        // setup a temporary texture
        byte pixel[] = { 1, 1, 1 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, createInfo.extents.w, createInfo.extents.h, 0, GL_RGB, GL_UNSIGNED_BYTE, &pixel);

    }

    // Queue image loading task. Make sure the result will override existing data in image upon completion. Also, generate mips if necessary.
    
    // copy buffer to use in task
    std::vector<unsigned char> bufferCopy;
    bufferCopy.assign((unsigned char*)info.buffer, (unsigned char*)info.buffer + info.bytes);
    
    // setup a packaged task. This will return the resulting data from loading using stbi
    std::packaged_task<ImageLoadResult()> task { 
        [buffer = std::move(bufferCopy)] () {
            int w, h, channels;
            void* decompressed = stbi_load_from_memory((uchar*)buffer.data(), (int)buffer.size(), &w, &h, &channels, 0);
            assert(decompressed);

            return ImageLoadResult
            {
                w, h, channels, { (unsigned char*)decompressed, stbi_image_free }
            };
    }};

    // create a future for retrieving the result of the task.
    std::shared_future<ImageLoadResult> result = task.get_future().share();

    loadingTasks.emplace_back(LoadTask {
        .loadingThread = std::this_thread::get_id(),
        .poll = std::async(std::move(task)),
        .complete = [
            result,
            iid,
            handle = GetImageHandle(iid),
            min = info.minFilter,
            mag = info.magFilter,
            wrapModeS = info.wrappingModeS,
            wrapModeT = info.wrappingModeT,
            sRGB = info.sRGB]()
        {
            auto& img = result.get();
            
            glBindTexture(GL_TEXTURE_2D, handle);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLenum)min);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLenum)mag);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLenum)wrapModeS);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLenum)wrapModeT);

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            if (img.channels == 3)
                glTexImage2D(GL_TEXTURE_2D, 0, sRGB ? GL_SRGB : GL_RGB, img.w, img.h, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data.get());
            else if (img.channels == 4)
                glTexImage2D(GL_TEXTURE_2D, 0, sRGB ? GL_SRGB_ALPHA : GL_RGBA, img.w, img.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data.get());
            
            glGenerateMipmap(GL_TEXTURE_2D);

            Instance()->imageExtents[iid] = { (unsigned)img.w, (unsigned)img.h };
        }
    });

    // queue task
    //loadingTasks.emplace_back(std::move(loadTask));

    return iid;
}

//------------------------------------------------------------------------------
/**
*/
TextureResourceId
TextureResource::LoadTexture(const char * path, MagFilter mag, MinFilter min, WrappingMode wrapModeS, WrappingMode wrapModeT, bool sRGB = false)
{
    ImageId iid = GetImageId(path);
    if (iid == InvalidImageId) {
        ImageCreateInfo info{};
        info.type = ImageType::TEXTURE_2D;
        iid = AllocateImage(info);
        Instance()->imageRegistry.emplace(path, iid);
    }
    else
    {
        return iid;
    }

    int w, h, n; //Width, Height, components per pixel (ex. RGB = 3, RGBA = 4)
    auto start = std::chrono::high_resolution_clock::now();
    unsigned char *image = stbi_load(path, &w, &h, &n, STBI_default);
    auto stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = stop - start;
    std::cout << "Loaded " << w << "x" << h << " " << n << "BPP texture from file in : " << duration.count() << " (ms)" << std::endl;

    if (image == nullptr)
    {
        printf("Could not find texture file!\n");
        assert(false);
    }

    GLuint handle;
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLenum)min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLenum)mag);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLenum)wrapModeS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLenum)wrapModeT);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // If there's no alpha channel, use RGB colors. else: use RGBA.
    if (n == 3)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, sRGB ? GL_SRGB : GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    }
    else if (n == 4)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, sRGB ? GL_SRGB_ALPHA : GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    }

    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(image);

    Instance()->imageHandles[iid] = handle;
    Instance()->imageExtents[iid] = { (unsigned)w, (unsigned)h };
    Instance()->imageTypes[iid] = ImageType::TEXTURE_2D;
    return iid;
}

//------------------------------------------------------------------------------
/**
*/
TextureResourceId
TextureResource::LoadTextureFromMemory(std::string name, void* buffer, uint64_t bytes, ImageId imageId, MagFilter mag, MinFilter min, WrappingMode wrapModeS, WrappingMode wrapModeT, bool sRGB = false)
{
	assert(Instance()->imageRegistry.count(name) == 0);
    int channels;
    GLuint const& imageHandle = instance->imageHandles[imageId];
    ImageExtents& imageExtents = instance->imageExtents[imageId]; 

    auto start = std::chrono::high_resolution_clock::now();
    void* decompressed = stbi_load_from_memory((uchar*)buffer, (int)bytes, (int*)&imageExtents.w, (int*)&imageExtents.h, (int*)&channels, 0);
    auto stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = stop - start;
    std::cout << "Loaded " << imageExtents.w << "x" << imageExtents.h << " " << channels << "BPP texture from memory in : " << duration.count() << " (ms)" << std::endl;
    glBindTexture(GL_TEXTURE_2D, imageHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLenum)min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLenum)mag);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLenum)wrapModeS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLenum)wrapModeT);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // If there's no alpha channel, use RGB colors. else: use RGBA.
    if (channels == 3)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, sRGB ? GL_SRGB : GL_RGB, imageExtents.w, imageExtents.h, 0, GL_RGB, GL_UNSIGNED_BYTE, decompressed);
    }
    else if (channels == 4)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, sRGB ? GL_SRGB_ALPHA : GL_RGBA, imageExtents.w, imageExtents.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, decompressed);
    }

    stbi_image_free(decompressed);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    Instance()->imageRegistry.emplace(name, imageId);
    return imageId;
}

//------------------------------------------------------------------------------
/**
*/
TextureResourceId
TextureResource::LoadCubemap(std::string const& name, std::vector<const char*> const& paths, bool sRGB = false)
{
    GLuint handle;
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_CUBE_MAP, handle);

    int w, h, nrChannels;
    for (unsigned int i = 0; i < paths.size(); i++)
    {
        unsigned char* data = stbi_load(paths[i], &w, &h, &nrChannels, 0);
        assert(data);
        
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, sRGB ? GL_SRGB : GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data
        );
        stbi_image_free(data);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    ImageId iid = (ImageId)Instance()->imageHandles.size();
    Instance()->imageHandles.push_back(handle);
    Instance()->imageExtents.push_back({ (unsigned)w, (unsigned)h });
    Instance()->imageTypes.push_back(ImageType::TEXTURE_2D);
    Instance()->imageRegistry.emplace(name, iid);
    return iid;
}

//------------------------------------------------------------------------------
/**
*/
ImageId
TextureResource::AllocateImage(ImageCreateInfo info)
{
    GLuint handle;
    glGenTextures(1, &handle);
    
    ImageId iid = (ImageId)Instance()->imageHandles.size();
    Instance()->imageHandles.push_back(handle);
    Instance()->imageExtents.push_back(info.extents);
    Instance()->imageTypes.push_back(info.type);
    return iid;
}

//------------------------------------------------------------------------------
/**
*/
ImageExtents
TextureResource::GetImageExtents(ImageId id)
{
    return Instance()->imageExtents[id];
}

//------------------------------------------------------------------------------
/**
*/
GLuint
TextureResource::GetTextureHandle(TextureResourceId tid)
{
    return Instance()->imageHandles[tid];
}

//------------------------------------------------------------------------------
/**
*/
GLuint
TextureResource::GetImageHandle(ImageId tid)
{
    return Instance()->imageHandles[tid];
}

//------------------------------------------------------------------------------
/**
*/
ImageId
TextureResource::GetImageId(std::string name)
{
    auto index = Instance()->imageRegistry.find(name);
    if (index != Instance()->imageRegistry.end())
    {
        return (*index).second;
    }
    return InvalidImageId;
}

//------------------------------------------------------------------------------
/**
*/
TextureResourceId
TextureResource::GetWhiteTexture()
{
    return Instance()->whiteTexture;
}

//------------------------------------------------------------------------------
/**
*/
TextureResourceId
TextureResource::GetBlackTexture()
{
    return Instance()->blackTexture;
}

//------------------------------------------------------------------------------
/**
*/
TextureResourceId
TextureResource::GetDefaultMetallicRoughnessTexture()
{
    return Instance()->defaultMetallicRoughnessTexture;
}

//------------------------------------------------------------------------------
/**
*/
TextureResourceId
TextureResource::GetDefaultNormalTexture()
{
    return Instance()->defaultNormalTexture;
}

//------------------------------------------------------------------------------
/**
*/
void
TextureResource::LoadTextureLibrary(std::string folder)
{
    auto* instance = Instance();

    
}

} // namespace Render

