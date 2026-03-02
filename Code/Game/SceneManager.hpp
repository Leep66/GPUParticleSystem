#pragma once
#include "Game/Scene.hpp"
#include <unordered_map>
#include <string>
#include <memory>

class SceneManager
{
public:
	SceneManager() = default;
	~SceneManager() = default;

	void RegisterScene(const std::string& name, std::unique_ptr<Scene> scene);
	void SwitchToScene(const std::string& sceneName, float fadeDuration = 1.0f);

	void Update(float deltaSeconds);
	void Render() const;

	Scene* GetCurrentScene() const { return m_currentScene; }
	Scene* GetScene(const std::string& name) const;

	void DebugRender() const;

public:
	std::unordered_map<std::string, std::unique_ptr<Scene>> m_scenes;
	Scene* m_currentScene = nullptr;
};