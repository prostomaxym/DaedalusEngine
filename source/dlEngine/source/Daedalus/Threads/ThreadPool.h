#pragma once

#include "ThreadJoiner.h"

#include <queue>
#include <thread>
#include <future>
#include <vector>
#include <atomic>
#include <condition_variable>

namespace Daedalus
{
	class DAEDALUS_API ThreadPool
	{
	public:
		ThreadPool(unsigned int n_threads = std::thread::hardware_concurrency());
		virtual ~ThreadPool();

		template <typename FuncT>
		void Submit(FuncT f)
		{
			std::lock_guard <std::mutex> lg(m_mtx);
			m_q.push(std::move(f));
			m_cv.notify_one();
		}

		template <typename FuncT>
		std::future< typename std::invoke_result_t<FuncT> > SubmitAndReturnFuture(FuncT f)
		{
			using res_t = std::invoke_result_t<FuncT>;
			std::packaged_task< res_t() > task(std::forward<FuncT>(f));
			auto res = task.get_future();

			Submit(std::move(task));

			return res;
		}

		size_t GetNumOfThreads() const { return m_workers.size(); }

	private:

		class function_wrapper
		{
			struct impl_base
			{
				virtual void call() = 0;
				virtual ~impl_base() = default;
			};

			std::unique_ptr<impl_base> impl;

			template <typename F>
			struct impl_type : impl_base
			{
				F f;
				impl_type(F&& f_) : f(std::move(f_)) {};
				void call() override { f(); }
			};

		public:
			template <typename F>
			function_wrapper(F&& f) :
				impl(std::make_unique<impl_type<F>>(std::move(f)))
			{}

			void operator ()() { impl->call(); }

			function_wrapper() = default;

			function_wrapper(function_wrapper&& other) :
				impl(std::move(other.impl))
			{}

			function_wrapper& operator=(function_wrapper&& other)
			{
				impl = std::move(other.impl);
				return *this;
			}

			function_wrapper(function_wrapper&) = delete;
			function_wrapper(const function_wrapper&) = delete;
			function_wrapper& operator=(const function_wrapper&) = delete;
		};

	private:
		void DoWork();

		std::vector<std::thread> m_workers;
		ThreadJoiner m_joiner;
		std::atomic<bool> m_done;
		std::mutex m_mtx;
		std::condition_variable m_cv;

		std::queue<function_wrapper> m_q;
	};
}