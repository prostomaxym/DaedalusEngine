#pragma once

#include <vector>
#include <thread>

namespace Daedalus
{
	/**
	* @brief Utility class which uses RAII idiom to join threads
	*/
	class ThreadJoiner final
	{
	public:
		/**
		* @brief Constructor
		* @param threads vector of threads which should be joined at destruction
		*/
		ThreadJoiner(std::vector<std::thread>& threads);

		/**
		* @brief Joins threads at destruction. It means that all thread must finish first.
		*/
		~ThreadJoiner();

	private:
		std::vector<std::thread>& m_threads;
	};
}