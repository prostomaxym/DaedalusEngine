#pragma once

#include "Macros.h"
#include "Utils/DeltaTime.h"
#include <memory>
#include <thread>

namespace Daedalus
{
	class Scene;

	class DAEDALUS_API SceneManager
	{
	public:
		SceneManager();
		~SceneManager();
		SceneManager(const SceneManager&) = delete;
		SceneManager& operator=(const SceneManager&) = delete;
		SceneManager(SceneManager&&) = delete;
		SceneManager& operator=(SceneManager&&) = delete;

		// Requests are committed at a frame boundary, never during a script callback.
		void RequestScene(std::unique_ptr<Scene> scene, bool start_runtime = true);
		bool CommitPending();
		Scene* GetActiveScene() const;
		bool HasPending() const { return m_has_pending; }
		void Clear();
		void OnFixedUpdate(DeltaTime dt);
		void OnUpdate(DeltaTime dt);
		void OnRender(DeltaTime dt);
		void OnViewportResize(uint32_t width, uint32_t height);

	private:
		void CheckThread() const;
		std::unique_ptr<Scene> m_active;
		std::unique_ptr<Scene> m_pending;
		const std::thread::id m_owner;
		bool m_has_pending{ false };
		bool m_start_runtime{ true };
	};
}
