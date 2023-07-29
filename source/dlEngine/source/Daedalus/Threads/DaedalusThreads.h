#pragma once

#include "Daedalus/Core/Core.h"
#include "ThreadPool.h"

namespace Daedalus
{
	class DaedalusThreads : public ThreadPool
	{
	public:
		/**
		* @brief Static method which returns singletone.
		*/
		static DaedalusThreads& Inst()
		{
			static DaedalusThreads inst;
			return inst;
		}

	private:
		/**
		* Creates pool with number of threads which is equal to hardware concurrency.
		*/
		DaedalusThreads() : ThreadPool(std::thread::hardware_concurrency() - 1)
		{}
	};
}