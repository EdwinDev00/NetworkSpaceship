#pragma once
//------------------------------------------------------------------------------
/**
    @file inputserver.h

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "mouse.h"
#include "keyboard.h"
#include "gamepad.h"
#include <vector>

namespace Input
{

//
class InputHandler
{
    InputHandler() = delete;
    ~InputHandler() = delete;
public:
    static void Create();
    // call before handling input
    static void BeginFrame();

    static void HandleKeyEvent(int32 key, int32 scancode, int32 action, int32 mods);
    static void HandleMousePressEvent(int32 button, int32 pressed, int32);
    static void HandleMouseMoveEvent(double x, double y);
};

Keyboard* GetDefaultKeyboard();
Mouse* GetDefaultMouse();
Gamepad* GetGamepad(int id);

} // namespace Input
