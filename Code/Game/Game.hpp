#pragma once
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/Clock.hpp"
#include "Game/Player.hpp"
#include "Engine/Core/Timer.hpp"
#include "Game/Prop.hpp"
#include "Engine/ParticleSystem/ParticleEmitter.hpp"
#include "Game/SceneManager.hpp"
#include "Game/Scene.hpp"
#include <cstdint>

enum class GameState
{
	NONE,
	ATTRACT,
	PLAYING,

	COUNT
};

class Game 
{
public:

	Game(App* owner);	
	~Game();
	void Startup();

	void Update();
	void Render() const;

	void EnterState(GameState state);
	void ExitState(GameState state);

	void Shutdown();
	
	static bool Event_KeysAndFuncs(EventArgs& args);

	void ShowImguiWindow();

	void RegisterScenes();
	void SwitchToScene(const std::string& sceneName, float fadeDuration = 1.0f);
	Scene* GetCurrentScene() const { return m_sceneManager.GetCurrentScene(); }
	SceneManager& GetSceneManager() { return m_sceneManager; }

public:
	App* m_App = nullptr;
	RandomNumberGenerator* m_rng = nullptr;
	bool m_isDebugActive = false;
	Rgba8 m_startColor = Rgba8(0, 255, 0, 255);
	Camera m_screenCamera;
	bool m_isConsoleOpen = false;
	Clock* m_clock = nullptr;

	Player* m_player;
	std::vector<Entity*> m_entities;

	GameState m_currentState = GameState::NONE;
	GameState m_nextState = GameState::ATTRACT;

	Texture* m_skyTex = nullptr;
	std::vector<Vertex_PCU> m_skyVerts;

	bool m_imguiCursor = true;

	uint32_t m_handleGravity ;
	uint32_t m_handleWind;
	uint32_t m_handlePoint;
	SceneManager m_sceneManager;
	Shader* m_shader = nullptr;
	Shader* m_particleRenderShader = nullptr;
private:
	void UpdateInput(float deltaSeconds);
	void DebugRender() const;
	void DebugRenderParticleForces() const;
	void UpdateCameras(float deltaSeconds);
	void UpdateAttractMode(float deltaSeconds);
	void UpdateParticleSystem(float deltaSeconds);
	void RenderAttractMode() const;
	void RenderAttractRing() const;

	void UpdateGame(float deltaSeconds);
	void RenderGame() const;
	void RenderLightSource(Light light) const;
	void RenderUI() const;
	void RenderDevConsole() const;

	void EnterAttractMode();
	void ExitAttractMode();
	void EnterPlayingMode();
	void ExitPlayingMode();

	void CreateOriginalWorldBasis();
	void CreateAndAddVertsForGrid(std::vector<Vertex_PCU>& verts, const Vec3& center, const Vec2& size, int numRows, int numCols);

	void ShowParticleStatsPanel();
	void ShowEmittersListPanel();
	void ShowEmitterInspectorPanel();
	void ShowForcesPanel();
	void ShowSceneSelectionPanel();
	void ShowLightingPanel();

	void ShowSceneGameObjectsPanel();

	void SetupSceneContent();
	void SetupScene1();
	void SetupScene2();
	void SetupScene3();
	void SetupScene4();
	void SetupScene5();

private:
	float m_attractRingRadius = 150.f;
	float m_attractRingThickness = 20.f;
	Timer* m_attractRingTimer = nullptr;
	bool m_zoomOut = true;
	size_t m_globalGravityId = 0;
	size_t m_pointGravityId = 0;
	size_t m_dirGravityId = 0;
	bool m_debugDraw = false;

	std::vector<Vertex_PCU> m_gridVerts;


private:
	int      m_uiSceneIndex = 0;
	int      m_uiEmitterIndex = 0;
	int      m_uiLightIndex = 0;
	int      m_uiForceIndex = 0;

	bool     m_uiEmitterPaused = false;
	uint32_t m_uiPendingMaxParticles = 0;

	int  m_uiGOIndex = 0;
	char m_uiGOFilter[64] = {};
	char m_uiDiffusePath[256] = {};
	char m_uiNormalPath[256] = {};
	char m_uiSgePath[256] = {};

	ParticleEmitter* m_uiSelectedEmitter = nullptr;


};