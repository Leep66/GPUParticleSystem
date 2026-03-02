#include "Game/Scene.hpp"
#include "Game/GameObject.hpp" 
#include "Engine/ParticleSystem/ParticleEmitter.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/ParticleSystem/ParticleSystem.hpp"

extern Renderer* g_theRenderer;
extern ParticleSystem* g_theParticleSystem;

Scene::Scene(const std::string& name)
	: m_name(name)
{
}

Scene::~Scene()
{
	for (GameObject* o : m_gameObjects)
	{
		if (!o) continue;
		delete o;
		o = nullptr;
	}

	
}

void Scene::Update(float deltaSeconds)
{
	for (auto& gameObject : m_gameObjects)
	{
		if (gameObject && gameObject->IsActive())
		{
			gameObject->Update(deltaSeconds);
		}
	}

	for (auto& emitter : m_particleEmitters)
	{
		if (emitter && emitter->IsAlive())
		{
			emitter->Update(deltaSeconds);
		}
	}
}

void Scene::Render() const
{
	for (auto& gameObject : m_gameObjects)
	{
		if (gameObject && gameObject->IsVisible())
		{
			gameObject->Render();
		}
	}

	for (auto& emitter : m_particleEmitters)
	{
		if (emitter && emitter->IsAlive())
		{
			emitter->Render();
		}
	}
}

void Scene::Exit()
{
	ClearGameObjects();

	ClearParticleEmitters();

	ClearForces();


}

void Scene::AddGameObject(GameObject* gameObject)
{
	if (gameObject)
	{
		m_gameObjects.push_back(gameObject);
	}
}

GameObject* Scene::CreateGameObject(Game* game, const std::string& name)
{
	GameObject* rawPtr = new GameObject(game);
	rawPtr->SetName(name);
	m_gameObjects.push_back(rawPtr);

	return rawPtr;
}


void Scene::RemoveGameObject(GameObject* /*gameObject*/)
{

}

void Scene::ClearGameObjects()
{
	m_gameObjects.clear();
}

void Scene::AddParticleEmitter(ParticleEmitter* emitter)
{
	if (emitter)
	{
		m_particleEmitters.push_back(emitter);
	}


}

void Scene::RemoveParticleEmitter(ParticleEmitter* emitter)
{
	if (!emitter) return;

	g_theParticleSystem->DestroyEmitter(emitter);

	m_particleEmitters.erase(
		std::remove_if(m_particleEmitters.begin(), m_particleEmitters.end(),
			[emitter](ParticleEmitter* e) { return e == emitter; }),
		m_particleEmitters.end()
	);

}

void Scene::ClearParticleEmitters()
{
	for (ParticleEmitter* em : m_particleEmitters)
	{
		RemoveParticleEmitter(em);
	}

	m_particleEmitters.clear();

}

void Scene::ClearForces()
{
	g_theParticleSystem->ClearForces();
}

void Scene::DebugRender() const
{
	if (!g_theRenderer) return;

	std::vector<Vertex_PCU> debugVerts;

	std::string sceneInfo = Stringf("Scene: %s | GameObjects: %d | Emitters: %d | Particles: %d",
		m_name.c_str(),
		GetGameObjectCount(),
		GetParticleEmitterCount(),
		GetTotalParticleCount());

	DebugAddScreenText(sceneInfo, AABB2(0.f, 780.f, 800.f, 800.f), 15.f, Vec2(0.f, 0.5f), -1.f);

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexArray(debugVerts);
}

int Scene::GetTotalParticleCount() const
{
	int totalParticles = 0;

	for (const auto& emitter : m_particleEmitters)
	{
		if (emitter && emitter->IsAlive())
		{
			totalParticles += emitter->GetActiveParticleCount();
		}
	}

	return totalParticles;
}

const std::vector<ParticleEmitter*>& Scene::GetParticleEmitters() const
{
	return m_particleEmitters;
}

GameObject* Scene::FindGameObjectByName(const std::string& name)
{
	for (auto& gameObject : m_gameObjects)
	{
		if (gameObject && gameObject->GetName() == name)
		{
			return gameObject;
		}
	}
	return nullptr;
}

std::vector<GameObject*> Scene::FindGameObjectsByName(const std::string& name)
{
	std::vector<GameObject*> result;
	for (auto& gameObject : m_gameObjects)
	{
		if (gameObject && gameObject->GetName() == name)
		{
			result.push_back(gameObject);
		}
	}
	return result;
}