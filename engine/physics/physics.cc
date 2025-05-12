//------------------------------------------------------------------------------
//  @file physics.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "physics.h"
#include "core/idpool.h"
#include "render/gltf.h"
#include "render/debugrender.h"
#include "core/random.h"
#include "core/cvar.h"
#include <iostream>
namespace Physics
{

struct ColliderMesh
{
    struct Triangle
    {
        glm::vec3 vertices[3];
        glm::vec3 normal;
    };
    std::vector<Triangle> tris;
    float bSphereRadius;
};

struct Colliders
{
    std::vector<bool> active;
    std::vector<uint16_t> masks;
    std::vector<void*> userData;
    std::vector<glm::vec4> positionsAndScales;
    std::vector<glm::mat4> invTransforms;
    std::vector<ColliderMeshId> meshes;
};

static Colliders colliders;
static std::vector<ColliderMesh> meshes;
static Util::IdPool<ColliderMeshId> colliderMeshPool;
static Util::IdPool<ColliderId> colliderPool;

//------------------------------------------------------------------------------
/**
    templated with index type because gltf supports everything from 8 to 32 bits, signed or unsigned.
*/
template<typename INDEX_T> void
LoadFromIndexBuffer(fx::gltf::Document const& doc, ColliderMesh* mesh)
{
    fx::gltf::Primitive const& primitive = doc.meshes[0].primitives[0];

    fx::gltf::Accessor const& ibAccessor = doc.accessors[primitive.indices];
    fx::gltf::BufferView const& ibView = doc.bufferViews[ibAccessor.bufferView];
    fx::gltf::Buffer const& ib = doc.buffers[ibView.buffer];

    fx::gltf::Accessor const& vbAccessor = doc.accessors[primitive.attributes.find("POSITION")->second];
    fx::gltf::BufferView const& vbView = doc.bufferViews[vbAccessor.bufferView];
    fx::gltf::Buffer const& vb = doc.buffers[vbView.buffer];

    size_t numIndices = ibAccessor.count;
    INDEX_T const* indexBuffer = (INDEX_T const*)&ib.data[ibAccessor.byteOffset + ibView.byteOffset];

    float const* vertexBuffer = (float const*)&vb.data[vbAccessor.byteOffset + vbView.byteOffset];

#if _DEBUG
    assert(vbAccessor.type == fx::gltf::Accessor::Type::Vec3 || vbAccessor.type == fx::gltf::Accessor::Type::Vec4);
#endif
    size_t vSize = (vbAccessor.type == fx::gltf::Accessor::Type::Vec3) ? 3 : 4; // HACK: Assumes 3d or 4d vertex positions
    for (int i = 0; i < numIndices; i += 3)
    {
        ColliderMesh::Triangle tri;
        tri.vertices[0] = glm::vec3(
            vertexBuffer[vSize * indexBuffer[i]],
            vertexBuffer[vSize * indexBuffer[i] + 1],
            vertexBuffer[vSize * indexBuffer[i] + 2]
        );
        tri.vertices[1] = glm::vec3(
            vertexBuffer[vSize * indexBuffer[i + 1]],
            vertexBuffer[vSize * indexBuffer[i + 1] + 1],
            vertexBuffer[vSize * indexBuffer[i + 1] + 2]
        );
        tri.vertices[2] = glm::vec3(
            vertexBuffer[vSize * indexBuffer[i + 2]],
            vertexBuffer[vSize * indexBuffer[i + 2] + 1],
            vertexBuffer[vSize * indexBuffer[i + 2] + 2]
        );

        // compute plane's normal
        glm::vec3 AB = tri.vertices[1] - tri.vertices[0];
        glm::vec3 AC = tri.vertices[2] - tri.vertices[0];
        tri.normal = glm::cross(AC, AB);

        mesh->tris.push_back(std::move(tri));
    }

    // bounding sphere radius is max of x, y or z from aabb
    mesh->bSphereRadius = vbAccessor.max[0];
    mesh->bSphereRadius = std::max(mesh->bSphereRadius, vbAccessor.max[1]);
    mesh->bSphereRadius = std::max(mesh->bSphereRadius, vbAccessor.max[2]);
    mesh->bSphereRadius = std::max(mesh->bSphereRadius, fabs(vbAccessor.min[0]));
    mesh->bSphereRadius = std::max(mesh->bSphereRadius, fabs(vbAccessor.min[1]));
    mesh->bSphereRadius = std::max(mesh->bSphereRadius, fabs(vbAccessor.min[2]));
}


//------------------------------------------------------------------------------
/**
*/
ColliderMeshId
LoadColliderMesh(std::string path)
{
    ColliderMeshId id;
    ColliderMesh* mesh;
    if (colliderMeshPool.Allocate(id))
    {
        ColliderMesh newMesh;
        meshes.push_back(std::move(newMesh));
    }
    mesh = &meshes[id.index];

    // Load mesh from file
    fx::gltf::Document doc;
    try
    {
        if (path.substr(path.find_last_of(".") + 1) == "glb")
            doc = fx::gltf::LoadFromBinary(path);
        else
            doc = fx::gltf::LoadFromText(path);
    }
    catch (const std::exception& err)
    {
        printf(err.what());
        assert(false);
        colliderMeshPool.Deallocate(id);
        return ColliderMeshId();
    }

    // HACK: currently only supports one primtive per collider mesh. Needs to be the only one in the GLTF as well...
    fx::gltf::Primitive const& primitive = doc.meshes[0].primitives[0];
    fx::gltf::Accessor const& ibAccessor = doc.accessors[primitive.indices];
    fx::gltf::Accessor::ComponentType componentType = ibAccessor.componentType;

    switch (componentType)
    {
    case fx::gltf::Accessor::ComponentType::Byte:
        LoadFromIndexBuffer<int8_t>(doc, mesh);
        break;
    case fx::gltf::Accessor::ComponentType::UnsignedByte:
        LoadFromIndexBuffer<uint8_t>(doc, mesh);
        break;
    case fx::gltf::Accessor::ComponentType::Short:
        LoadFromIndexBuffer<int16_t>(doc, mesh);
        break;
    case fx::gltf::Accessor::ComponentType::UnsignedShort:
        LoadFromIndexBuffer<uint16_t>(doc, mesh);
        break;
    case fx::gltf::Accessor::ComponentType::UnsignedInt:
        LoadFromIndexBuffer<uint32_t>(doc, mesh);
        break;
    default:
        assert(false); // not supported
        break;
    }

    return id;
}

//------------------------------------------------------------------------------
/**
*/
ColliderId
CreateCollider(ColliderMeshId meshId, glm::mat4 const& transform, uint16_t mask, void* userData)
{
#if _DEBUG
    {
        // Only allows uniform scaling along all axes
        float x = glm::length(glm::vec3(transform[0]));
        float y = glm::length(glm::vec3(transform[1]));
        float z = glm::length(glm::vec3(transform[2]));
        assert(fabs(x - y) < 0.00001f && fabs(x - z) < 0.00001f);
    }
#endif
    glm::vec4 PS = glm::vec4(transform[3]);
    PS.w = glm::length(transform[0]);

    ColliderId id;
    if (colliderPool.Allocate(id))
    {
        colliders.positionsAndScales.push_back(PS);
        colliders.invTransforms.push_back(glm::inverse(transform));
        colliders.meshes.push_back(meshId);
        colliders.active.push_back(true);
        colliders.userData.push_back(userData);
        colliders.masks.push_back(mask);
    }
    else
    {
        colliders.positionsAndScales[id.index] = PS;
        colliders.invTransforms[id.index] = glm::inverse(transform);
        colliders.meshes[id.index] = meshId;
        colliders.active[id.index] = true;
        colliders.userData[id.index] = userData;
        colliders.masks[id.index] = mask;
    }
    return id;
}

//------------------------------------------------------------------------------
/**
*/
void
SetTransform(ColliderId collider, glm::mat4 const& transform)
{
    assert(colliderPool.IsValid(collider));
#if _DEBUG
    {
        // Only allows uniform scaling along all axes
        float x = glm::length(glm::vec3(transform[0]));
        float y = glm::length(glm::vec3(transform[1]));
        float z = glm::length(glm::vec3(transform[2]));
        assert(fabs(x - y) < 0.00001f && fabs(x - z) < 0.00001f);
    }
#endif
    glm::vec4 PS = glm::vec4(transform[3]);
    PS.w = glm::length(transform[0]);
    colliders.positionsAndScales[collider.index] = PS;
    colliders.invTransforms[collider.index] = glm::inverse(transform);
}

//------------------------------------------------------------------------------
/**
    Cast ray from start point in direction. Make sure the direction is a unit vector.
*/
RaycastPayload
Raycast(glm::vec3 start, glm::vec3 dir, float maxDistance, uint16_t mask)
{
    RaycastPayload ret;
    ret.hitDistance = maxDistance;
    // TODO: spatial acceleration instead of just checking everything...
    int numColliders = (int)colliders.active.size();
    for (int colliderIndex = 0; colliderIndex < numColliders; colliderIndex++)
    {
        if (colliders.active[colliderIndex] && (mask == 0 || (colliders.masks[colliderIndex] & mask) != 0))
        {
            ColliderMesh const* const mesh = &meshes[colliders.meshes[colliderIndex].index];
            glm::vec3 bSphereCenter = colliders.positionsAndScales[colliderIndex];
            float radius = mesh->bSphereRadius * colliders.positionsAndScales[colliderIndex][3];

            // Coarse check against bounding sphere
            {
                glm::vec3 cDir = bSphereCenter - start;

                float r2 = radius * radius;
                float c2 = glm::dot(cDir, cDir);

                if (c2 < r2)
                    goto CHECK_MESH; // ray starts within sphere

                float d = glm::dot(cDir, dir);
                if (d < 0.0f)
                    continue; // ray is pointing away from sphere

                float discr = d * d - (c2 - r2);

                // A negative discriminant corresponds to ray missing sphere 
                if (discr < 0.0f)
                    continue;

                // NOTE: this should be equivalent to this: (sqrtf(c2) - radius > ret.hitDistance)), but faster
                if ((c2 > (ret.hitDistance * ret.hitDistance) + (2 * radius * ret.hitDistance) + r2))
                    continue; // ray is too short
            }

        CHECK_MESH:
            // transform ray into modelspace
            glm::mat4 const& invT = colliders.invTransforms[colliderIndex];
            glm::vec3 invRayStart = invT * glm::vec4(start, 1.0f);
            glm::vec3 invRayDir = invT * glm::vec4(dir, 0);

            // fine check against mesh
            int numTris = (int)mesh->tris.size();
            for (int i = 0; i < numTris; ++i)
            {
                glm::vec3 const& N = mesh->tris[i].normal;

                float NdotRayDirection = glm::dot(N, invRayDir);
                if (NdotRayDirection < 0)
                    continue; // backfacing surface

                glm::vec3 const& A = mesh->tris[i].vertices[0];
                glm::vec3 const& B = mesh->tris[i].vertices[1];
                glm::vec3 const& C = mesh->tris[i].vertices[2];

                float d = -glm::dot(N, A);
                float t = -(glm::dot(N, invRayStart) + d) / NdotRayDirection;

                if (t < 0)
                    continue;  //the triangle is behind the ray

                glm::vec3 P = invRayStart + invRayDir * t;

                // check triangle bounds
                glm::vec3 K;  //vector perpendicular to one of three subdivided triangles's plane 
                glm::vec3 edge0 = B - A;
                glm::vec3 vp0 = P - A;
                K = glm::cross(vp0, edge0);
                if (glm::dot(N, K) < 0)
                    continue;

                glm::vec3 edge1 = C - B;
                glm::vec3 vp1 = P - B;
                K = glm::cross(vp1, edge1);
                if (glm::dot(N, K) < 0)
                    continue;

                glm::vec3 edge2 = A - C;
                glm::vec3 vp2 = P - C;
                K = glm::cross(vp2, edge2);
                if (glm::dot(N, K) < 0)
                    continue;

                // intersection with at least one triangle
                if (ret.hitDistance >= t)
                {
                    ret.hit = true;
                    ret.hitDistance = t;
                    ret.collider = ColliderId::Create(colliderIndex, colliderPool.generations[colliderIndex]);
                }
            }
        }
    }

    if (ret.hit)
    {
        //calculate hitpoint
        ret.hitPoint = start + dir * ret.hitDistance;
    }

    return ret;
}

} // namespace Physics
