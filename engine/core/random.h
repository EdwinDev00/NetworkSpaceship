#pragma once
//------------------------------------------------------------------------------
/**
    @file random.h

    Contains random-number helper functions

    @copyright
    (C) 2018-2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

namespace Core
{

/// Produces an xorshift128 pseudo random number.
uint FastRandom();

/// Produces an xorshift128 psuedo based floating point random number in range 0..1
/// Note that this is not a truely random random number generator
float RandomFloat();

/// Produces an xorshift128 psuedo based floating point random number in range -1..1
/// Note that this is not a truely random random number generator
float RandomFloatNTP();

} // namespace Core
