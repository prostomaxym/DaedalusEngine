#pragma once

#include "Daedalus/Core/Input.h"

namespace Daedalus {

class GLFWInput : public Input
{
protected:
	virtual bool IsKeyPressedImpl(int keycode) override;
	virtual bool IsMouseButtonPressedImpl(int button) override;
	virtual std::pair<float,float> GetMousePosImpl() override;

};

}