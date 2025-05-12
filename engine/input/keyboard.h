#pragma once
//------------------------------------------------------------------------------
/**
    @file keyboard.h

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "key.h"

namespace Input
{

struct Keyboard
{
    /// contains all keys that was pressed this frame. This is reset every frame by InputServer
    bool pressed[Key::Code::NumKeyCodes];

    /// contains all keys that was released this frame. This is reset every frame by InputServer
    bool released[Key::Code::NumKeyCodes];

    /// contains all keys that are currently being held. These are reset as buttons receive the release action by InputServer
    bool held[Key::Code::NumKeyCodes];
};

} // namespace Input
