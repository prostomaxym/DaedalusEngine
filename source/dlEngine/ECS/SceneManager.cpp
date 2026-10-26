#include "dlpch.h"
#include "SceneManager.h"
#include "Scene.h"

using namespace Daedalus;

SceneManager::SceneManager() : m_owner(std::this_thread::get_id()) {}
SceneManager::~SceneManager() { Clear(); }

void SceneManager::CheckThread() const
{
	DL_ASSERT(m_owner == std::this_thread::get_id(), Log::Categories::ECS, "Scenes must be managed on their owning thread");
	if (m_owner != std::this_thread::get_id()) std::abort();
}

void SceneManager::RequestScene(std::unique_ptr<Scene> scene, bool start_runtime)
{
	CheckThread();
	m_pending = std::move(scene);
	m_start_runtime = start_runtime;
	m_has_pending = true;
}

bool SceneManager::CommitPending()
{
	CheckThread();
	if (!m_has_pending) return true;
	m_has_pending = false;
	if (m_active) m_active->OnRuntimeStop();
	m_active = std::move(m_pending);
	if (m_active && m_start_runtime && !m_active->OnRuntimeStart())
	{
		Log::Write(Log::Levels::Error, Log::Categories::ECS, "Scene runtime could not start: {}", m_active->GetSceneName());
		return false;
	}
	return true;
}

Scene* SceneManager::GetActiveScene() const
{
	CheckThread();
	return m_active.get();
}

void SceneManager::Clear()
{
	CheckThread();
	m_pending.reset();
	m_active.reset();
	m_has_pending = false;
}

void SceneManager::OnFixedUpdate(DeltaTime dt)
{
	CheckThread();
	if (m_active) m_active->OnFixedUpdateRuntime(dt);
}

void SceneManager::OnUpdate(DeltaTime dt)
{
	CheckThread();
	if (m_active) m_active->OnUpdateRuntime(dt);
}

void SceneManager::OnRender(DeltaTime dt)
{
	CheckThread();
	if (m_active) m_active->OnRenderRuntime(dt);
}

void SceneManager::OnViewportResize(uint32_t width, uint32_t height)
{
	CheckThread();
	if (m_active) m_active->OnViewportResize(width, height);
}
