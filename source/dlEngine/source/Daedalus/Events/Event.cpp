#include "dlpch.h"

#include "Event.h"

using namespace Daedalus;

std::chrono::milliseconds Event::GetEllapsedTime() const
{
	auto test = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_begin_time);
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_begin_time);
}

void Event::StartTimer()
{
	m_begin_time = std::chrono::high_resolution_clock::now();
}
