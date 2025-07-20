#pragma once

#include <chrono>

namespace Daedalus 
{
	// @brief stored in nanoseconds
	class DeltaTime
	{
	public:
        DeltaTime(std::chrono::nanoseconds time = {}) : m_time(time) {}

        float GetNanoseconds() const
        {
            return m_time.count();
        }

        float GetMicroseconds() const
        {
            return m_time.count() / 1000.f;
        }

        float GetMilliseconds() const
        {
            return m_time.count() / 1000000.f;
        }

        float GetSeconds() const
        {
            return  m_time.count() / 1000000000.f;;
        }

        std::chrono::nanoseconds GetTime() const
        {
            return m_time;
        }

        //------------------------------- Operators --------------------------
    public:
        DeltaTime& operator+=(const DeltaTime& other)
        {
            m_time += other.m_time;
            return *this;
        }

        DeltaTime& operator-=(const DeltaTime& other)
        {
            m_time -= other.m_time;
            return *this;
        }

        DeltaTime operator+(const DeltaTime& other) const
        {
            DeltaTime result = *this;
            result += other;
            return result;
        }

        DeltaTime operator-(const DeltaTime& other) const
        {
            DeltaTime result = *this;
            result -= other;
            return result;
        }

        DeltaTime& operator=(const DeltaTime& other)
        {
            m_time = other.m_time;
            return *this;
        }

        bool operator==(const DeltaTime& other) const
        {
            return m_time == other.m_time;
        }

        bool operator!=(const DeltaTime& other) const
        {
            return !(*this == other);
        }

        bool operator<(const DeltaTime& other) const
        {
            return m_time < other.m_time;
        }

        bool operator>(const DeltaTime& other) const
        {
            return m_time > other.m_time;
        }

        bool operator<=(const DeltaTime& other) const
        {
            return !(*this > other);
        }

        bool operator>=(const DeltaTime& other) const
        {
            return !(*this < other);
        }

	private:
		std::chrono::nanoseconds m_time;
	};
}