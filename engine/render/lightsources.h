#pragma once
//------------------------------------------------------------------------------
/**
    @file lightsources

    holds all types of lightsources

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
namespace Render
{

struct PointLightId
{
    uint32_t index : 22; // 4M concurrent lights
    uint32_t generation : 10; // 1024 generations per index

    static PointLightId Create(uint32_t id)
    {
        PointLightId ret;
        ret.index = id & 0x003FFFFF;
        ret.generation = (id & 0xFFC0000) >> 22;
        return ret;
    }
    explicit constexpr operator uint32_t() const
    {
        return ((generation << 22) & 0xFFC0000) + (index & 0x003FFFFF);
    }
    static constexpr PointLightId Invalid()
    {
        return { 0xFFFFFFFF, 0xFFFFFFFF };
    }
    constexpr uint32_t HashCode() const
    {
        return index;
    }
    const bool operator==(const PointLightId& rhs) const { return uint32_t(*this) == uint32_t(rhs); }
    const bool operator!=(const PointLightId& rhs) const { return uint32_t(*this) != uint32_t(rhs); }
    const bool operator<(const PointLightId& rhs) const { return index < rhs.index; }
    const bool operator>(const PointLightId& rhs) const { return index > rhs.index; }
};

} // namespace Render
