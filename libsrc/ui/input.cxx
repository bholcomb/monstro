/*-----------------------------------------------------------------------------
Copyright 2017
Bob Holcomb
-----------------------------------------------------------------------------*/

#include "ui/input.h"

using namespace UI;

MouseState::MouseState()
{

}

bool MouseState::isButtonClicked(int button, bool repeat /*= false*/)
{
   return false;
}

void MouseState::newFrame()
{

}

void KeyboardState::newFrame()
{

}

bool KeyboardState::keyPressed(Key key)
{
   return false;
}

bool KeyboardState::keyReleased(Key key)
{
   return false;
}

bool KeyboardState::keyJustPressed(Key key)
{
   return false;
}
