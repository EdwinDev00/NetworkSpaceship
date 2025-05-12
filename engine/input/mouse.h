#pragma once
//------------------------------------------------------------------------------
/**
    @file   mouse.h

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
namespace Input
{

struct Mouse
{
    enum Button
    {
        LeftButton = 0,
        RightButton = 1,
        MiddleButton = 2,

        NumMouseButtons,
        InvalidMouseButton,
    };

    /// contains all buttons that was pressed this frame. This is reset every frame by InputServer
    bool pressed[Button::NumMouseButtons];

    /// contains all buttons that was released this frame. This is reset every frame by InputServer
    bool released[Button::NumMouseButtons];

    /// contains all buttons that are currently being held. These are reset as buttons receive the release action by InputServer
    bool held[Button::NumMouseButtons];

    // position in pixels. ([0-windowWidth], [0-windowHeight])
    glm::vec2 position;

    // delta position this frame
    glm::vec2 delta;

    // position previous frame
    glm::vec2 previousPosition;
};

} // namespace Input
