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
            return std::chrono::duration_cast<std::chrono::nanoseconds>(m_time).count();
        }

        float GetMicroseconds() const
        {
            return std::chrono::duration_cast<std::chrono::microseconds>(m_time).count();
        }

        float GetMilliseconds() const
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(m_time).count();
        }

        float GetSeconds() const
        {
            return std::chrono::duration_cast<std::chrono::seconds>(m_time).count();
        }

        static DeltaTime Lock60FPS()
        {
            return DeltaTime(std::chrono::nanoseconds(16666666));
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