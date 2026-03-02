#include "Game/SceneManager.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/StringUtils.hpp"

void SceneManager::RegisterScene(const std::string& name, std::unique_ptr<Scene> scene)
{
	if (!scene)
	{
		ERROR_RECOVERABLE(Stringf("Attempted to register null scene: %s", name.c_str()));
		return;
	}

	m_scenes[name] = std::move(scene);
}

void SceneManager::SwitchToScene(const std::string& sceneName, float /*fadeDuration*/)
{
	auto it = m_scenes.find(sceneName);
	if (it == m_scenes.end())
	{
		ERROR_RECOVERABLE(Stringf("Scene not found: %s", sceneName.c_str()));
		return;
	}

	if (m_currentScene)
	{
		m_currentScene->Exit();
		m_currentScene->SetActive(false);
	}

	m_currentScene = it->second.get();

	if (m_currentScene)
	{
		m_currentScene->Enter();
		m_currentScene->SetActive(true);
	}

}

void SceneManager::Update(float deltaSeconds)
{
	if (m_currentScene && m_currentScene->IsActive())
	{
		m_currentScene->Update(deltaSeconds);
	}
}

void SceneManager::Render() const
{
	if (m_currentScene && m_currentScene->IsActive())
	{
		m_currentScene->Render();
	}
}

Scene* SceneManager::GetScene(const std::string& name) const
{
	auto it = m_scenes.find(name);
	return (it != m_scenes.end()) ? it->second.get() : nullptr;
}

void SceneManager::DebugRender() const
{
	if (m_currentScene)
	{
		m_currentScene->DebugRender();
	}
}