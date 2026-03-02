#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Engine/Renderer/Renderer.hpp"

class GameObject; 
class ParticleEmitter;
class Game;

class Scene
{
public:
	Scene(const std::string& name);
	virtual ~Scene();

	virtual void Update(float deltaSeconds);
	virtual void Render() const;

	virtual void Load() {}
	virtual void Enter() {}
	virtual void Exit();
	virtual void Unload() {}

	void AddGameObject(GameObject* gameObject);
	GameObject* CreateGameObject(Game* game, const std::string& name = "GameObject");

	void RemoveGameObject(GameObject* gameObject);
	void ClearGameObjects();

	void AddParticleEmitter(ParticleEmitter* emitter);
	void RemoveParticleEmitter(ParticleEmitter* emitter);
	void ClearParticleEmitters();

	void ClearForces();

	const std::string& GetName() const { return m_name; }
	bool IsActive() const { return m_isActive; }
	void SetActive(bool active) { m_isActive = active; }

	virtual void DebugRender() const;
	int GetTotalParticleCount() const;
	int GetGameObjectCount() const { return (int)m_gameObjects.size(); }
	int GetParticleEmitterCount() const { return (int)m_particleEmitters.size(); }
	const std::vector<ParticleEmitter*>& GetParticleEmitters() const;
	GameObject* FindGameObjectByName(const std::string& name);
	std::vector<GameObject*> FindGameObjectsByName(const std::string& name);

	std::vector<GameObject*>& GetGameObjects() { return m_gameObjects; }
	const std::vector<GameObject*>& GetGameObjects() const { return m_gameObjects; }


	Lights& GetLights() { return m_lights; }

	std::vector<ParticleEmitter*> GetEmitters() const { return m_particleEmitters; }

protected:
	std::string m_name;
	bool m_isActive = false;
	Lights m_lights;
	std::vector<GameObject*> m_gameObjects;
	std::vector<ParticleEmitter*> m_particleEmitters;
};