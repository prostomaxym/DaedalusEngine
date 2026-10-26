#include "dlpch.h"

#include "Input.h"

using namespace Daedalus;

Input* Input::s_instance = nullptr;

Input::Input()
{
	DL_ASSERT(!s_instance, Log::Categories::EngineCore, "Only one input system may be active");
	if (s_instance) std::abort();
	s_instance = this;
}

Input::~Input()
{
	s_instance = nullptr;
}

Input* Input::GetInstance()
{
	DL_ASSERT(s_instance, Log::Categories::EngineCore, "Input used outside Engine lifetime");
	if (!s_instance) std::abort();
	return s_instance;
}

std::pair<float, float> Input::GetStickPos(StickType type)
{
	auto pos = GetInstance()->GetStickPosImpl(type);

	if (std::abs(pos.first) < GetInstance()->m_sticks_deadzone && std::abs(pos.second) < GetInstance()->m_sticks_deadzone)
		return { 0.f, 0.f };
	else
		return pos;
}
