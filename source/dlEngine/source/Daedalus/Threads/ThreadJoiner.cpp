#include "dlpch.h"

#include "ThreadJoiner.h"

using namespace Daedalus;

ThreadJoiner::ThreadJoiner(std::vector<std::thread>& threads) : m_threads(threads) {}

ThreadJoiner::~ThreadJoiner()
{
	for (auto& th : m_threads)
	{
		if (th.joinable())
			th.join();
	}
}
