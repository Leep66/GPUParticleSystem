#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/SimpleTriangleFont.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "ThirdParty/Noise/SmoothNoise.hpp"
#include "ThirdParty/imgui/imgui.h"
#include "Engine/ParticleSystem/ParticleSystem.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/ParticleSystem/ParticleForce.hpp"
#include "Game/GameObject.hpp"
#include "Engine/Renderer/StaticMeshDefinition.hpp"
#include "Engine/Renderer/BitmapFontPro.hpp"
#include <cmath>
#include <sstream>
#include <unordered_map>



extern App* g_theApp;
extern Renderer* g_theRenderer;
extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern DevConsole* g_theDevConsole;
extern EventSystem* g_theEventSystem;
extern BitmapFont* g_theFont;
extern BitmapFontPro* g_theFontPro;
extern Window* g_theWindow;

ParticleSystem* g_theParticleSystem = nullptr;

Game::Game(App* owner)
	: m_App(owner)
{
	Startup();

}

Game::~Game()
{
	Shutdown();
}

void Game::Startup()
{
	m_clock = new Clock();
	m_attractRingTimer = new Timer(0.5f, m_clock);

	m_player = new Player(this);
	m_entities.push_back(m_player);
	m_player->SetPosition(Vec3(-2.f, 0.f, 1.f));
	
	
	StaticMeshDefinition::InitializeStaticMeshDefinitions("Data/Definitions/StaticMeshObjDefinitions.xml");

	/*Prop* cube1 = new Prop(this, PropShape::CUBE);
	m_entities.push_back(cube1);
	cube1->SetPosition(Vec3(2.f, 2.f, 0.f));
	
	cube1->m_angularVelocity.m_pitchDegrees = 30.f;
	cube1->m_angularVelocity.m_rollDegrees = 30.f;


	Prop* cube2 = new Prop(this, PropShape::CUBE);
	m_entities.push_back(cube2);
	cube2->SetPosition(Vec3(-2.f, -2.f, 0.f));
	cube2->m_isBlink = true;


	Prop* sphere1 = new Prop(this, PropShape::SPHERE);
	m_entities.push_back(sphere1);
	sphere1->SetPosition(Vec3(10.f, -5.f, 1.f));
	sphere1->m_angularVelocity.m_yawDegrees = 45.f;
	sphere1->m_texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/TestUV.png");*/

	

	m_screenCamera.SetMode(Camera::eMode_Orthographic);

	screenWidth = (float)g_theWindow->GetClientDimensions().x;
	screenHeight = (float)g_theWindow->GetClientDimensions().y;

	m_screenCamera.SetOrthographicView(
		Vec2(0.f, 0.f),
		Vec2(screenWidth, screenHeight)
	);

	//CreateAndAddVertsForGrid(m_gridVerts, Vec3(0.f, 0.f, 0.f), Vec2(100.f, 100.f), 100, 100);

	
	/*{
		m_particleBillboardVerts.clear();
		m_particleBillboardVerts.reserve(6);

		AABB3 localBounds = AABB3(Vec3(-0.5f, -0.5f, 0.f), Vec3(0.5f, 0.5f, 0.f));
		AddVertsForAABB3D(m_particleBillboardVerts, localBounds, Rgba8::WHITE);

		m_particleTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/smoke.png");
	}*/

	m_shader = g_theRenderer->CreateShader("Data/Shaders/ParticleRender", VertexType::Vertex_PCUTBN, "MeshVertexMain");
	{
		ParticleSystemConfig particleConfig;
		particleConfig.m_game = this;
		g_theParticleSystem = new ParticleSystem(particleConfig);
		g_theParticleSystem->m_camera = m_player->GetCamera();
		
		RegisterScenes();
	}



}

void Game::Update()
{
	float deltaSeconds = m_clock->GetDeltaSeconds();

	UpdateCameras(deltaSeconds);

	float time = m_clock->GetTotalSeconds();

	PerFrameConstants debugData;
	debugData.Time = time;
	debugData.DebugInt = 0;
	debugData.DebugFloat = 0.f;

	if (g_theInput->IsKeyDown('1'))
	{
		debugData.DebugInt = 1;
	}
	else if (g_theInput->IsKeyDown('2'))
	{
		debugData.DebugInt = 2;
	}
	else if (g_theInput->IsKeyDown('3'))
	{
		debugData.DebugInt = 3;
	}
	else if (g_theInput->IsKeyDown('4'))
	{
		debugData.DebugInt = 4;
	}
	else if (g_theInput->IsKeyDown('5'))
	{
		debugData.DebugInt = 5;
	}


	g_theRenderer->SetPerFrameConstants(debugData);
	
	if (g_theInput->WasKeyJustPressed('P'))
	{
		m_clock->TogglePause();
	}

	if (g_theInput->WasKeyJustPressed('O'))
	{
		m_clock->StepSingleFrame();
	}

	if (g_theInput->IsKeyDown('T'))
	{
		m_clock->SetTimeScale(0.1f);
	}
	else if (g_theInput->IsKeyDown('Y'))
	{
		m_clock->SetTimeScale(10.f);
	}
	else
	{
		m_clock->SetTimeScale(1.f);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_TILDE))
	{
		m_isConsoleOpen = !m_isConsoleOpen;
		g_theDevConsole->ToggleOpen();
	}

	

	switch (m_currentState)
	{
	case GameState::ATTRACT:
		UpdateAttractMode(deltaSeconds);
		break;
	case GameState::PLAYING:
		UpdateGame(deltaSeconds);
		break;
	default:
		break;
	}

	if (m_currentState != m_nextState)
	{
		ExitState(m_currentState);
		m_currentState = m_nextState;
		EnterState(m_currentState);
	}
	

}

void Game::UpdateInput(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_BACK))
	{
		m_nextState = GameState::ATTRACT;
		//SoundID clickSound = g_theAudio->CreateOrGetSound("Data/Audio/click.wav");
		//g_theAudio->StartSound(clickSound, false, 1.f, 0.f, 0.5f);
		DebugRenderClear();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		m_nextState = GameState::ATTRACT;
		g_theApp->ResetGame();
	}

	if (g_theInput->WasKeyJustPressed('H'))
	{
		GetCurrentScene()->ResetPlayer(m_player);
	}

	bool imguiMode = true;

	if (g_theInput->IsKeyDown(KEYCODE_RIGHT_MOUSE))
	{
		imguiMode = false;
	}
	m_imguiCursor = imguiMode;


	if (g_theInput->WasKeyJustPressed(KEYCODE_F1))
	{
		m_debugDraw = !m_debugDraw;
	}
}

void Game::Render() const
{
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	switch (m_currentState)
	{
	case GameState::ATTRACT:
		RenderAttractMode();
		break;
	case GameState::PLAYING:
	{
		g_theRenderer->ClearScreen(Rgba8::DARK_GRAY);
		g_theRenderer->BeginCamera(m_player->GetCamera());

		std::vector<Vertex_PCU> skyVerts = m_skyVerts;
		Mat44 transform;
		Vec3 camerPos = m_player->GetCamera().GetPosition();
		transform.SetTranslation3D(Vec3(camerPos.x, camerPos.y, camerPos.z));
		transform.AppendScaleUniform3D(0.5f);
		TransformVertexArray3D(skyVerts, transform);

		g_theRenderer->BindShader(g_theRenderer->GetShader("Default"));
		g_theRenderer->BindTexture(m_skyTex);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);

		g_theRenderer->DrawVertexArray(skyVerts);
		g_theRenderer->BindShader(m_shader);
		

		m_sceneManager.Render();


		if (g_theParticleSystem) g_theParticleSystem->Render();

		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

		g_theRenderer->BindShader(g_theRenderer->GetShader("Default"));
		Lights lights = GetCurrentScene()->GetLights();

		for (int i = 0; i < lights.m_numLights; ++i)
		{
			RenderLightSource(lights.m_lightsArray[i]);
		}

		g_theRenderer->SetLightConstants(lights);

		DebugRenderWorld(m_player->GetCamera());
		DebugRenderParticleForces();

		g_theRenderer->EndCamera(m_player->GetCamera());

		g_theRenderer->BeginCamera(m_screenCamera);
		DebugRenderScreen(m_screenCamera);
		RenderUI();
		g_theRenderer->EndCamera(m_screenCamera);
	}
	break;


	default:
		break;
	}

	RenderDevConsole();
}

void Game::EnterState(GameState state)
{
	switch (state)
	{
	case GameState::ATTRACT:
		EnterAttractMode();
		break;

	case GameState::PLAYING:
		EnterPlayingMode();
		break;
	}
}

void Game::ExitState(GameState state)
{
	switch (state)
	{
	case GameState::ATTRACT:
		ExitAttractMode();
		break;
	case GameState::PLAYING:
		ExitPlayingMode();
		break;
	}
}


void Game::UpdateGame(float deltaSeconds)
{
	if (m_currentState != GameState::PLAYING)
		return;

	float time = m_clock->GetTotalSeconds();
	float FPS = 1.f / m_clock->GetDeltaSeconds();
	float frameTimeMs = m_clock->GetDeltaSeconds() * 1000.f;


	float boxWidth = 180.f;
	float boxHeight = 40.f;

	float startX = screenWidth - 3.f * boxWidth;
	float yMin = screenHeight - boxHeight;
	float yMax = screenHeight;

	// Time
	{
		std::string text = Stringf("Time: %.2f", time);
		AABB2 box(startX + 0 * boxWidth, yMin,
			startX + 1 * boxWidth, yMax);
		DebugAddScreenText(text, box, 20.f, Vec2(0.0f, 0.5f), -1.f, Rgba8::CYAN);
	}

	// FPS
	{
		std::string text = Stringf("FPS: %.0f", FPS);
		AABB2 box(startX + 1 * boxWidth, yMin,
			startX + 2 * boxWidth, yMax);
		DebugAddScreenText(text, box, 20.f, Vec2(0.0f, 0.5f), -1.f, Rgba8::CYAN);
	}

	// Frame Time
	{
		std::string text = Stringf("Frame: %.2f", frameTimeMs);
		AABB2 box(startX + 2 * boxWidth, yMin,
			startX + 3 * boxWidth, yMax);
		DebugAddScreenText(text, box, 20.f, Vec2(0.0f, 0.5f), -1.f, Rgba8::CYAN);
	}


	UpdateInput(deltaSeconds);
	UpdateParticleSystem(deltaSeconds);
	m_sceneManager.Update(deltaSeconds);

	


	for (int i = 0; i < (int)m_entities.size(); ++i)
	{
		Entity* entity = m_entities[i];
		if (entity)
		{
			entity->Update(deltaSeconds);
		}
	}

	if (GetCurrentScene()->GetName() == "Scene 4")
	{
		m_tornadoTime += deltaSeconds;

		float angle = m_tornadoTime * m_tornadoMoveSpeed * 360.f;
		Vec2 circle = Vec2::MakeFromPolarDegrees(angle, m_tornadoMoveRadius);

		m_tornadoBasePos = Vec3(circle.x, circle.y, 0.f);

		for (ParticleEmitter* em : m_tornadoEmitters)
		{
			if (!em) continue;
			em->SetPosition(m_tornadoBasePos);
		}

		for (int i = 0; i < (int)m_tornadoForceIndices.size(); i++)
		{
			int idx = m_tornadoForceIndices[i];

			ParticleForce* fPtr = g_theParticleSystem->GetForce(idx);
			if (!fPtr) continue;

			ParticleForce f = *fPtr;

			float t = m_tornadoTime;
			float seed = (float)i * 37.0f;

			float noiseX = Compute2dPerlinNoise(
				t + seed,
				0.0f,
				0.8f,
				3,
				0.5f,
				2.0f,
				true,
				1234u + i
			);

			float noiseY = Compute2dPerlinNoise(
				0.0f,
				t + seed,
				0.8f,
				3,
				0.5f,
				2.0f,
				true,
				5678u + i
			);

			float strength = 0.8f;

			f.m_position[0] = m_tornadoBasePos.x + noiseX * strength;
			f.m_position[1] = m_tornadoBasePos.y + noiseY * strength;

			g_theParticleSystem->SetForce(idx, f);
		}
	}
}

void Game::RenderLightSource(Light light) const
{
	if (!m_debugDraw) return;
	std::vector<Vertex_PCU> verts;

	Vec3 cameraForward = m_player->GetCamera().GetOrientation().GetForwardNormal();
	Vec3 depthOffset = cameraForward * -0.01f; 

	if (light.m_direction == Vec3::ZERO)
	{
		Vec3 pos = light.m_position + depthOffset;
		Rgba8 pointColor = light.m_color.ToRgba();

		AddVertsForSphere3D(verts, pos, 0.1f, pointColor);
		AddVertsForUVSphereZWireframe3D(verts, pos, light.m_innerRadius, 16, 0.02f, Rgba8::ScaleColor(pointColor, 0.6f, 0.5f));
		AddVertsForUVSphereZWireframe3D(verts, pos, light.m_outerRadius, 16, 0.02f, Rgba8::ScaleColor(pointColor, 0.3f));
	}
	else
	{
		Vec3 tipPos = light.m_position + depthOffset;
		Rgba8 coneColor = light.m_color.ToRgba();

		Vec3 mainBasePos = tipPos + light.m_direction * 0.5f;
		AddVertsForCone3D(verts, mainBasePos, tipPos, 0.2f, coneColor, AABB2::ZERO_TO_ONE, 16);

		float innerBaseRadius = light.m_innerRadius * tanf(acosf(light.m_innerDotThreshold));
		float outerBaseRadius = light.m_outerRadius * tanf(acosf(light.m_outerDotThreshold));

		Vec3 innerBasePos = tipPos + light.m_direction * light.m_innerRadius;
		AddVertsForWireCone3D(verts, innerBasePos, tipPos, innerBaseRadius, Rgba8::ScaleColor(coneColor, 0.6f, 0.5f), 16);

		Vec3 outerBasePos = tipPos + light.m_direction * light.m_outerRadius;
		AddVertsForWireCone3D(verts, outerBasePos, tipPos, outerBaseRadius, Rgba8::ScaleColor(coneColor, 0.5f), 16);
	}

	if (!verts.empty())
	{
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray((int)verts.size(), verts.data());
	}
}

void Game::RenderGame() const
{
	g_theRenderer->BindTexture(nullptr);
	

	g_theRenderer->SetModelConstants();
	g_theRenderer->SetDepthMode(DepthMode::READ_ONLY_LESS_EQUAL);
	g_theRenderer->DrawVertexArray(m_gridVerts);
}

void Game::RenderUI() const
{
	
}

void Game::RenderDevConsole() const
{
	g_theRenderer->BeginCamera(m_screenCamera);
	if (g_theDevConsole)
	{
		g_theDevConsole->Render(AABB2(0, 0, screenWidth, screenHeight));
		
	}
	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::EnterAttractMode()
{
}

void Game::ExitAttractMode()
{
}

void Game::EnterPlayingMode()
{
	SwitchToScene("Scene 1");
}

void Game::ExitPlayingMode()
{
	Scene* currentScene = GetCurrentScene();
	if (currentScene)
	{
		currentScene->Exit();
		currentScene->SetActive(false);
	}
}


void Game::CreateOriginalWorldBasis()
{
	Vec3 basisPos;
	EulerAngles basisAngle;

	Mat44 basistransform = basisAngle.GetAsMatrix_IFwd_JLeft_KUp();
	basistransform.SetTranslation3D(basisPos);

	DebugAddWorldBasis(basistransform, -1.f, DebugRenderMode::USE_DEPTH);

	std::string xText = "x - forward";
	std::string yText = "y - left";
	std::string zText = "z - up";

	float textHeight = 0.2f;

	float alignment = 0.f;

	float duration = -1.f;

	DebugRenderMode mode = DebugRenderMode::USE_DEPTH;

	Vec3 xTextPos = basisPos + Vec3(0.2f, 0.f, 0.2f);
	Vec3 yTextPos = basisPos + Vec3(0.f, 1.8f, 0.2f);
	Vec3 zTextPos = basisPos + Vec3(0.f, -0.4f, 0.2f);

	Mat44 xTextTransform = Mat44::MakeTranslation3D(xTextPos);
	Mat44 yTextTransform = Mat44::MakeTranslation3D(yTextPos);
	Mat44 zTextTransform = Mat44::MakeTranslation3D(zTextPos);

	xTextTransform.AppendZRotation(-90.f);
	yTextTransform.AppendZRotation(180.f);
	
	zTextTransform.AppendZRotation(180.f);
	zTextTransform.AppendXRotation(90.f);

	DebugAddWorldText(xText, xTextTransform, textHeight, alignment, duration, Rgba8::RED, Rgba8::RED, mode);
	DebugAddWorldText(yText, yTextTransform, textHeight, alignment, duration, Rgba8::GREEN, Rgba8::GREEN, mode);
	DebugAddWorldText(zText, zTextTransform, textHeight, alignment, duration, Rgba8::BLUE, Rgba8::BLUE, mode);
}

void Game::CreateAndAddVertsForGrid(std::vector<Vertex_PCU>& verts, const Vec3& center, const Vec2& size, int numRows, int numCols)
{
	float gridWidth = size.x;
	float gridHeight = size.y;

	float cellWidth = gridWidth / (float)numCols;
	float cellHeight = gridHeight / (float)numRows;

	Vec3 startPos = center - Vec3(gridWidth * 0.5f, gridHeight * 0.5f, 0.0f);

	float thinLineThickness = 0.02f;
	float thickLineThickness = 0.05f;
	float originLineThickness = 0.1f;

	Rgba8 normalColor = Rgba8(127, 127, 127, 50);
	Rgba8 thickXColor = Rgba8(255, 50, 50, 50);
	Rgba8 thickYColor = Rgba8(50, 255, 50, 50);
	Rgba8 originLineXColor = Rgba8(255, 0, 0, 255);
	Rgba8 originLineYColor = Rgba8(0, 255, 0, 255);

	for (int row = 0; row <= numRows; ++row)
	{
		float y = startPos.y + row * cellHeight;
		bool isOriginLine = (y == center.y);

		for (int col = 0; col < numCols; ++col)
		{
			float x1 = startPos.x + col * cellWidth;
			float x2 = x1 + cellWidth;

			Rgba8 lineColor = isOriginLine ? originLineXColor : (row % 5 == 0) ? thickXColor : normalColor;
			float lineThickness = isOriginLine ? originLineThickness : (row % 5 == 0) ? thickLineThickness : thinLineThickness;

			AABB3 lineBounds;
			lineBounds.m_mins = Vec3(x1, y - lineThickness * 0.5f, -lineThickness * 0.5f);
			lineBounds.m_maxs = Vec3(x2, y + lineThickness * 0.5f, lineThickness * 0.5f);

			AddVertsForAABB3D(verts, lineBounds, lineColor);
		}
	}

	for (int col = 0; col <= numCols; ++col)
	{
		float x = startPos.x + col * cellWidth;
		bool isOriginLine = (x == center.x);

		for (int row = 0; row < numRows; ++row)
		{
			float y1 = startPos.y + row * cellHeight;
			float y2 = y1 + cellHeight;

			Rgba8 lineColor = isOriginLine ? originLineYColor : (col % 5 == 0) ? thickYColor : normalColor;
			float lineThickness = isOriginLine ? originLineThickness : (col % 5 == 0) ? thickLineThickness : thinLineThickness;

			AABB3 lineBounds;
			lineBounds.m_mins = Vec3(x - lineThickness * 0.5f, y1, -lineThickness * 0.5f);
			lineBounds.m_maxs = Vec3(x + lineThickness * 0.5f, y2, lineThickness * 0.5f);

			AddVertsForAABB3D(verts, lineBounds, lineColor);
		}
	}
}



void Game::ShowParticleStatsPanel()
{
	ImGui::SeparatorText("Particle System Stats");

	if (!g_theParticleSystem)
	{
		ImGui::TextDisabled("Particle system not available.");
		return;
	}

	Scene* scene = GetCurrentScene();

	if (!scene)
	{
		ImGui::TextDisabled("No active scene.");
		return;
	}

	const auto& emittersRef = scene->GetEmitters();
	std::vector<ParticleEmitter*> emitters(emittersRef.begin(), emittersRef.end());

	const int emitterCount = (int)emitters.size();
	const int totalParticles = g_theParticleSystem->GetAllParticlesCount();

	ImGui::Text("Emitters: %d", emitterCount);
	ImGui::SameLine();
	ImGui::Text("Total Particles: %d", totalParticles);

	ImGui::Separator();

	if (emitters.empty())
	{
		ImGui::TextDisabled("No active emitters.");
		return;
	}

	static bool s_onlyEnabled = false;
	ImGui::Checkbox("Only Enabled", &s_onlyEnabled);

	enum SortMode
	{
		SORT_BY_NAME = 0,
		SORT_BY_ACTIVE,
		SORT_BY_GPU,
		SORT_BY_SPAWN
	};

	static int s_sortMode = SORT_BY_GPU;
	static bool s_desc = true;

	ImGui::SameLine();
	const char* sortNames[] = { "Name", "Active", "GPU", "Spawn" };
	ImGui::SetNextItemWidth(140.f);
	ImGui::Combo("Sort", &s_sortMode, sortNames, IM_ARRAYSIZE(sortNames));

	ImGui::SameLine();
	ImGui::Checkbox("Desc", &s_desc);

	if (s_onlyEnabled)
	{
		std::vector<ParticleEmitter*> filtered;
		filtered.reserve(emitters.size());
		for (ParticleEmitter* e : emitters)
		{
			if (e && e->IsEnabled())
				filtered.push_back(e);
		}
		emitters.swap(filtered);
	}

	auto lessByName = [](ParticleEmitter* a, ParticleEmitter* b) { return a->GetName() < b->GetName(); };

	auto getActive = [](ParticleEmitter* e)->int
		{
			return (int)e->GetActiveCount();
		};

	auto getMax = [](ParticleEmitter* e)->int { return (int)e->GetMaxParticles(); };
	auto getGpu = [](ParticleEmitter* e)->float { return (float)e->GetLastGpuTimeMs(); };
	auto getSpawn = [](ParticleEmitter* e)->float { return e->GetSpawnRate(); };

	auto sortPred = [&](ParticleEmitter* a, ParticleEmitter* b) -> bool
		{
			if (a == b) return false;
			if (!a) return true;
			if (!b) return false;

			switch (s_sortMode)
			{
			case SORT_BY_NAME:
			{
				if (a->GetName() == b->GetName()) return false;
				return s_desc ? (a->GetName() > b->GetName()) : (a->GetName() < b->GetName());
			}

			case SORT_BY_ACTIVE:
			{
				int av = (int)a->GetActiveCount();
				int bv = (int)b->GetActiveCount();
				if (av == bv) return a->GetName() < b->GetName();
				return s_desc ? (av > bv) : (av < bv);
			}

			case SORT_BY_GPU:
			{
				float ag = (float)a->GetLastGpuTimeMs();
				float bg = (float)b->GetLastGpuTimeMs();

				if (::isnan(ag)) ag = -1e30f;
				if (::isnan(bg)) bg = -1e30f;

				if (ag == bg) return a->GetName() < b->GetName();
				return s_desc ? (ag > bg) : (ag < bg);
			}

			case SORT_BY_SPAWN:
			{
				float as = a->GetSpawnRate();
				float bs = b->GetSpawnRate();
				if (as == bs) return a->GetName() < b->GetName();
				return s_desc ? (as > bs) : (as < bs);
			}

			default:
				return a->GetName() < b->GetName();
			}
		};


	std::sort(emitters.begin(), emitters.end(), sortPred);

	ImGui::Spacing();

	ImGuiTableFlags flags =
		ImGuiTableFlags_Borders |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_SizingStretchProp |
		ImGuiTableFlags_ScrollY;

	const float tableHeight = 260.f;

	if (ImGui::BeginTable("EmitterStats", 5, flags, ImVec2(0.f, tableHeight)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 70.f);
		ImGui::TableSetupColumn("Active", ImGuiTableColumnFlags_WidthFixed, 90.f);
		ImGui::TableSetupColumn("Spawn", ImGuiTableColumnFlags_WidthFixed, 80.f);
		ImGui::TableSetupColumn("GPU ms", ImGuiTableColumnFlags_WidthFixed, 80.f);
		ImGui::TableSetupColumn("Age", ImGuiTableColumnFlags_WidthFixed, 70.f);
		ImGui::TableHeadersRow();

		int sumActive = 0;
		int sumMax = 0;
		double sumGpu = 0.0;
		double sumSpawn = 0.0;

		for (ParticleEmitter* e : emitters)
		{
			if (!e) continue;

			const int active = getActive(e);
			const int maxP = getMax(e);

			sumActive += active;
			sumMax += maxP;
			sumGpu += (double)getGpu(e);
			sumSpawn += (double)getSpawn(e);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(e->GetName().c_str());

			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%d / %d", active, maxP);

			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%.1f", getSpawn(e));

			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%.3f", getGpu(e));

			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%.2f", e->GetAge());
		}

		ImGui::EndTable();

		ImGui::Separator();

		ImGui::Text("Active Sum: %d / %d", sumActive, sumMax);
		ImGui::SameLine();
		ImGui::Text("Avg GPU: %.3f ms", (emitters.empty() ? 0.f : (float)(sumGpu / (double)emitters.size())));
		ImGui::SameLine();
		ImGui::Text("Total Spawn: %.1f", (float)sumSpawn);
	}
}

void Game::UpdateAttractMode(float deltaSeconds)
{

	if (g_theInput->WasKeyJustPressed(' ') || g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_A) || g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_START))
	{
		m_nextState = GameState::PLAYING;
		/*
		SoundID clickSound = g_theAudio->CreateOrGetSound("Data/Audio/click.wav");
		g_theAudio->StartSound(clickSound, false, 1.f, 0.f, 0.5f);
		*/

	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_BACK))
	{
		FireEvent("quit");
	}

	if (m_attractRingTimer->IsStopped())
	{
		m_attractRingTimer->Start();
	}

	if (m_attractRingTimer->DecrementPeriodIfElapsed())
	{
		m_zoomOut = !m_zoomOut;
	}

	if (m_zoomOut)
	{
		m_attractRingThickness += 20.f * deltaSeconds;
		m_attractRingRadius += 200.f * deltaSeconds;
	}
	else
	{
		m_attractRingThickness -= 20.f * deltaSeconds;
		m_attractRingRadius -= 200.f * deltaSeconds;
	}

}



void Game::UpdateParticleSystem(float deltaSeconds)
{
	if (g_theParticleSystem)
	{
		g_theParticleSystem->Update(deltaSeconds);
	}
}

void Game::RenderAttractMode() const
{
	g_theRenderer->ClearScreen(Rgba8(0, 127, 127, 255));
	g_theRenderer->BeginCamera(m_screenCamera);

	std::vector<Vertex_PCU> backgroundVerts;
	Texture* backTex = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Background.png");
	AddVertsForAABB2D(backgroundVerts, AABB2(Vec2(0.f, 0.f), Vec2(screenWidth, screenHeight)), Rgba8::WHITE);
	g_theRenderer->BindTexture(backTex);
	g_theRenderer->DrawVertexArray(backgroundVerts);
	g_theRenderer->BindTexture(nullptr);

	std::string title = "GPU-BASED PARTICLE SYSTEM";

	g_theFontPro->DrawTextInBox2D_Font(
		title,
		AABB2(0.f, screenHeight - 200.f, screenWidth, screenHeight - 100.f),
		120.f,
		Rgba8::WHITE,
		Vec2(0.5f, 0.5f));
	

	RenderAttractRing();


	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderAttractRing() const
{
	std::vector<Vertex_PCU> ringVerts;
	AddVertsForRing2D(ringVerts, Vec2(screenWidth / 2.f, screenHeight / 2.f), m_attractRingRadius, m_attractRingThickness, Rgba8(255, 127, 0, 255));

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexArray(ringVerts);
}

void Game::Shutdown()
{

	for (int i = 0; i < (int)m_entities.size(); ++i)
	{
		delete m_entities[i];
		m_entities[i] = nullptr;
	}
	m_entities.clear();

	m_player = nullptr;

	m_gridVerts.clear(); 
	
	delete m_clock;
	m_clock = nullptr;

	if (g_theParticleSystem)
	{
		g_theParticleSystem->Shutdown();
		delete g_theParticleSystem;
		g_theParticleSystem = nullptr;
	}

}

bool Game::Event_KeysAndFuncs(EventArgs& args)
{
	UNUSED(args);

	g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, "Controls");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "[RMB] - Hold to Aim");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "W / A - Move");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "S / D - Strafe");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Q / E - Elevate");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Shift - Sprint");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "H     - Set Camera to Origin");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "P     - Pause Game");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "O     - Step One Frame");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "T     - Slow Motion Mode");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Y     - Fast Motion Mode");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "~     - Open / Close DevConsole");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Esc   - Exit Game");

	return true;
}

void Game::ShowImguiWindow()
{
	if (m_currentState != GameState::PLAYING) return;
	if (!g_theParticleSystem) return;

	if (!ImGui::Begin("Leep's Particle System")) {
		ImGui::End();
		return;
	}

	Scene* scene = GetCurrentScene();

	

	if (m_player)
	{
		Vec3 playerPos = m_player->GetPosition();
		EulerAngles playerOrient = m_player->GetOrientation();
		Vec3 forward = m_player->GetFwdNormal();

		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Player Info:");
		ImGui::Text("Position:   X: %.2f   Y: %.2f   Z: %.2f",
			playerPos.x, playerPos.y, playerPos.z);

		ImGui::Text("Orientation (Euler): Yaw: %.1f Pitch: %.1f Roll: %.1f",
			playerOrient.m_yawDegrees, playerOrient.m_pitchDegrees, playerOrient.m_rollDegrees);

		ImGui::Text("Forward Dir: X: %.2f   Y: %.2f   Z: %.2f",
			forward.x, forward.y, forward.z);

		ImGui::Separator();
	}
	else
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Player: Not found / NULL");
		ImGui::Separator();
	}

	ImGui::Checkbox("Debug Draw", &m_debugDraw);

	{
		const int emitterCount = (int)g_theParticleSystem->GetEmitters().size();
		const int totalParticles = g_theParticleSystem->GetAllParticlesCount();
		ImGui::Text("Scene: %s | Emitters: %d | Particles: %d",
			scene ? scene->GetName().c_str() : "None",
			emitterCount,
			totalParticles);
		ImGui::Separator();
	}

	if (ImGui::BeginTable("##ParticleEditorLayout", 2,
		ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
	{
		ImGui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthFixed, 280.0f);
		ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextColumn();
		ImGui::BeginChild("##LeftPane", ImVec2(0, 0), true);

		ShowSceneSelectionPanel();
		ImGui::Separator();
		ShowEmittersListPanel();

		ImGui::EndChild();

		ImGui::TableNextColumn();
		ImGui::BeginChild("##RightPane", ImVec2(0, 0), false);

		if (ImGui::BeginTabBar("##ParticleTabs"))
		{
			if (ImGui::BeginTabItem("Stats")) {
				ShowParticleStatsPanel();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Emitter")) {
				ShowEmitterInspectorPanel();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Lighting")) {
				ShowLightingPanel();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Forces")) {
				ShowForcesPanel();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Scene"))
			{
				ShowSceneGameObjectsPanel();
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
		

		ImGui::EndChild();
		ImGui::EndTable();
	}

	ImGui::End();
}


void Game::ShowEmittersListPanel()
{
	if (!ImGui::CollapsingHeader("Emitters", ImGuiTreeNodeFlags_DefaultOpen))
		return;

	Scene* scene = GetCurrentScene();
	if (!scene)
	{
		ImGui::TextDisabled("No active scene.");
		return;
	}

	const std::vector<ParticleEmitter*>& emitters = scene->GetParticleEmitters();

	if (ImGui::Button("Add"))
	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "Emitter";
		cfg.mainStage.texPath = "Data/Images/Smoke.png";

		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);

		scene->AddParticleEmitter(em);

		m_uiEmitterIndex = (int)scene->GetParticleEmitters().size() - 1;
		m_uiPendingMaxParticles = 0;
	}

	ImGui::SameLine();

	bool hasEmitters = !emitters.empty();
	bool indexValid = (m_uiEmitterIndex >= 0 && m_uiEmitterIndex < (int)emitters.size());
	bool canRemove = (hasEmitters && indexValid);

	ImGui::BeginDisabled(!canRemove);
	if (ImGui::Button("Remove"))
	{
		ParticleEmitter* victim = emitters[m_uiEmitterIndex];

		scene->RemoveParticleEmitter(victim);
		g_theParticleSystem->DestroyEmitter(victim);

		if (m_uiSelectedEmitter == victim)
		{
			m_uiSelectedEmitter = nullptr;
		}

		int newCount = (int)scene->GetParticleEmitters().size();
		if (newCount > 0)
		{
			if (m_uiEmitterIndex >= newCount) m_uiEmitterIndex = newCount - 1;
			m_uiSelectedEmitter = scene->GetParticleEmitters()[m_uiEmitterIndex];
		}
		else
		{
			m_uiEmitterIndex = 0;
		}

		if (newCount <= 0)
		{
			m_uiEmitterIndex = 0;
		}
		else if (m_uiEmitterIndex >= newCount)
		{
			m_uiEmitterIndex = newCount - 1;
		}

		m_uiPendingMaxParticles = 0;
	}
	ImGui::EndDisabled();

	ImGui::Separator();

	if (emitters.empty())
	{
		ImGui::TextDisabled("No emitters in current scene.");
		return;
	}

	if (m_uiEmitterIndex < 0) m_uiEmitterIndex = 0;
	if (m_uiEmitterIndex >= (int)emitters.size()) m_uiEmitterIndex = (int)emitters.size() - 1;

	bool selectedStillValid = false;
	for (ParticleEmitter* e : emitters)
	{
		if (e == m_uiSelectedEmitter)
		{
			selectedStillValid = true;
			break;
		}
	}
	if (!selectedStillValid)
	{
		m_uiSelectedEmitter = nullptr;
		m_uiEmitterIndex = 0;
	}

	if (ImGui::BeginListBox("##EmitterList", ImVec2(-FLT_MIN, 220)))
	{
		for (int i = 0; i < (int)emitters.size(); ++i)
		{
			ParticleEmitter* em = emitters[i];
			bool selected = (i == m_uiEmitterIndex);

			ImGui::PushID((void*)em);
			const char* label = em ? em->GetName().c_str() : "<null emitter>";

			if (ImGui::Selectable(label, selected))
			{
				m_uiEmitterIndex = i;
				m_uiSelectedEmitter = em; 
				m_uiPendingMaxParticles = 0;
			}
			ImGui::PopID();

			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndListBox();
	}
}


void Game::ShowEmitterInspectorPanel()
{
	Scene* scene = GetCurrentScene();
	if (!scene)
	{
		ImGui::TextDisabled("No active scene.");
		return;
	}

	const std::vector<ParticleEmitter*>& emitters = scene->GetParticleEmitters();
	if (emitters.empty())
	{
		ImGui::TextDisabled("No emitter selected.");
		m_uiSelectedEmitter = nullptr;
		return;
	}

	if (!m_uiSelectedEmitter)
	{
		m_uiSelectedEmitter = emitters[0];
		m_uiEmitterIndex = 0;
	}

	bool found = false;
	for (int i = 0; i < (int)emitters.size(); ++i)
	{
		if (emitters[i] == m_uiSelectedEmitter)
		{
			found = true;
			m_uiEmitterIndex = i;
			break;
		}
	}
	if (!found)
	{
		m_uiSelectedEmitter = emitters[0];
		m_uiEmitterIndex = 0;
	}

	ParticleEmitter* em = m_uiSelectedEmitter;
	if (!em) return;

	ParticleEmitterConfig cfg = em->GetConfig();
	bool dirty = false;

	ImGui::SeparatorText("Emitter");

	ImGui::Text("Active: %u", em->GetActiveParticleCount());
	ImGui::SameLine();
	ImGui::Text("Age: %.2fs", em->GetAge());

	bool enabled = em->IsEnabled();
	if (ImGui::Checkbox("Enabled", &enabled)) em->SetEnabled(enabled);

	if (ImGui::Checkbox("Paused", &m_uiEmitterPaused)) em->SetPaused(m_uiEmitterPaused);

	ImGui::SameLine();
	if (ImGui::Button("Restart")) em->Restart();

	ImGui::SeparatorText("Config");

	{
		char nameBuf[128] = {};
		strncpy_s(nameBuf, cfg.name.c_str(), sizeof(nameBuf) - 1);
		if (ImGui::InputText("Name", nameBuf, IM_ARRAYSIZE(nameBuf)))
		{
			cfg.name = nameBuf;
			dirty = true;
		}
	}

	{
		char pathBuf[256] = {};
		strncpy_s(pathBuf, cfg.mainStage.texPath.c_str(), sizeof(pathBuf) - 1);
		if (ImGui::InputText("Texture Path", pathBuf, IM_ARRAYSIZE(pathBuf)))
		{
			cfg.mainStage.texPath = pathBuf;
			dirty = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Reload Texture"))
		{
			em->UpdateConfig(cfg);
		}
	}

	float pos[3] = { cfg.position.x, cfg.position.y, cfg.position.z };
	float ext[3] = { cfg.spawnArea.x, cfg.spawnArea.y, cfg.spawnArea.z };
	float baseV[3] = { cfg.mainStage.baseVelocity.x, cfg.mainStage.baseVelocity.y, cfg.mainStage.baseVelocity.z };
	float varV[3] = { cfg.mainStage.velocityVariance.x, cfg.mainStage.velocityVariance.y, cfg.mainStage.velocityVariance.z };

	if (ImGui::DragFloat3("Emitter Position", pos, 0.01f)) { cfg.position = Vec3(pos[0], pos[1], pos[2]); dirty = true; }
	if (ImGui::DragFloat3("Spawn Extent", ext, 0.01f, 0.f, 1000.f)) { cfg.spawnArea = Vec3(ext[0], ext[1], ext[2]); dirty = true; }
	if (ImGui::DragFloat3("Base Velocity", baseV, 0.01f)) { cfg.mainStage.baseVelocity = Vec3(baseV[0], baseV[1], baseV[2]); dirty = true; }
	if (ImGui::DragFloat3("Velocity Variance", varV, 0.01f, 0.f, 1000.f)) { cfg.mainStage.velocityVariance = Vec3(varV[0], varV[1], varV[2]); dirty = true; }

	dirty |= ImGui::DragFloat("Spawn Rate", &cfg.spawnRate, 10.f, 0.f, 1e7f);
	dirty |= ImGui::DragFloat("Particle Lifetime", &cfg.mainStage.lifetime, 0.01f, 0.f, 1000.f);
	dirty |= ImGui::DragFloat("Lifetime Variance", &cfg.mainStage.lifetimeVariance, 0.01f, 0.f, 1000.f);
	dirty |= ImGui::DragFloat("Start Size", &cfg.mainStage.startSize, 0.005f, 0.f, 1000.f);
	dirty |= ImGui::DragFloat("End Size", &cfg.mainStage.endSize, 0.005f, 0.f, 1000.f);
	dirty |= ImGui::DragFloat("Duration", &cfg.duration, 0.01f, 0.f, 1e6f);

	dirty |= ImGui::DragFloat("Start Soft", &cfg.mainStage.startSoftFactor, 0.01f, 0.f, 10.f);
	dirty |= ImGui::DragFloat("End Soft", &cfg.mainStage.endSoftFactor, 0.01f, 0.f, 10.f);

	dirty |= ImGui::DragFloat("Start Emissive", &cfg.mainStage.startEmissive, 0.01f, 0.f, 10.f);
	dirty |= ImGui::DragFloat("End Emissive", &cfg.mainStage.endEmissive, 0.01f, 0.f, 10.f);

	dirty |= ImGui::DragFloat("Base Angular Vel", &cfg.mainStage.baseAngularVelocity, 0.01f, -100.f, 100.f);
	dirty |= ImGui::DragFloat("Angular Variance", &cfg.mainStage.angularVariance, 0.01f, 0.f, 100.f);

	dirty |= ImGui::DragFloat("Spawn Probability", &cfg.mainStage.prob, 0.01f, 0.f, 1.f);

	float sc[4] = {
		cfg.mainStage.startColor.r / 255.f,
		cfg.mainStage.startColor.g / 255.f,
		cfg.mainStage.startColor.b / 255.f,
		cfg.mainStage.startColor.a / 255.f
	};
	float ec[4] = {
		cfg.mainStage.endColor.r / 255.f,
		cfg.mainStage.endColor.g / 255.f,
		cfg.mainStage.endColor.b / 255.f,
		cfg.mainStage.endColor.a / 255.f
	};

	if (ImGui::ColorEdit4("Start Color", sc))
	{
		cfg.mainStage.startColor = Rgba8(
			(uint8_t)(sc[0] * 255.f),
			(uint8_t)(sc[1] * 255.f),
			(uint8_t)(sc[2] * 255.f),
			(uint8_t)(sc[3] * 255.f)
		);
		dirty = true;
	}

	if (ImGui::ColorEdit4("End Color", ec))
	{
		cfg.mainStage.endColor = Rgba8(
			(uint8_t)(ec[0] * 255.f),
			(uint8_t)(ec[1] * 255.f),
			(uint8_t)(ec[2] * 255.f),
			(uint8_t)(ec[3] * 255.f)
		);
		dirty = true;
	}

	int billboard = (int)cfg.mainStage.billboardType;
	const char* types[] = { "Screen Facing", "World Aligned" };
	if (ImGui::Combo("Billboard Type", &billboard, types, IM_ARRAYSIZE(types)))
	{
		cfg.mainStage.billboardType = (uint32_t)billboard;
		dirty = true;
	}

	if (dirty) em->UpdateConfig(cfg);

	ImGui::SeparatorText("Capacity");

	if (m_uiPendingMaxParticles == 0) m_uiPendingMaxParticles = cfg.maxParticles;

	ImGui::InputScalar("Max Particles", ImGuiDataType_U32, &m_uiPendingMaxParticles);

	ImGui::BeginDisabled(m_uiPendingMaxParticles == cfg.maxParticles);
	if (ImGui::Button("Apply"))
	{
		cfg.maxParticles = m_uiPendingMaxParticles;
		em->Shutdown();
		em->UpdateConfig(cfg);
		em->Startup();
	}
	ImGui::EndDisabled();
}

void Game::ShowLightingPanel()
{
	if (!ImGui::CollapsingHeader("Lighting Control", ImGuiTreeNodeFlags_DefaultOpen))
		return;

	Scene* scene = GetCurrentScene();
	if (!scene) return;

	Lights& L = scene->GetLights();

	ImGui::SeparatorText("Sun Light");

	float sunColor[3] = { L.m_sunColor.x, L.m_sunColor.y, L.m_sunColor.z };
	if (ImGui::ColorEdit3("Sun Color", sunColor))
	{
		L.m_sunColor.x = sunColor[0];
		L.m_sunColor.y = sunColor[1];
		L.m_sunColor.z = sunColor[2];
	}

	float sunIntensity = L.m_sunColor.w;
	if (ImGui::DragFloat("Sun Intensity", &sunIntensity, 0.01f, 0.0f, 20.0f))
		L.m_sunColor.w = sunIntensity;

	float sunDir[3] = { L.m_sunDirection.x, L.m_sunDirection.y, L.m_sunDirection.z };
	if (ImGui::DragFloat3("Sun Direction", sunDir, 0.01f, -1.0f, 1.0f))
	{
		Vec3 d = Vec3(sunDir[0], sunDir[1], sunDir[2]);
		if (d.GetLengthSquared() > 0.0001f)
			L.m_sunDirection = d.GetNormalized();
	}


	ImGui::Spacing();
	ImGui::SeparatorText("Lights");


	m_uiLightIndex = (int)GetClamped((float)m_uiLightIndex, 0.f, (float) L.m_numLights - 1.f);

	if (ImGui::BeginTable("##LightTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable))
	{
		ImGui::TableSetupColumn("List", ImGuiTableColumnFlags_WidthFixed, 220.f);
		ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthStretch);


		ImGui::TableNextColumn();

		if (ImGui::BeginListBox("##LightList", ImVec2(-FLT_MIN, 260)))
		{
			for (int i = 0; i < L.m_numLights; i++)
			{
				bool selected = (i == m_uiLightIndex);

				char buf[64];
				snprintf(buf, sizeof(buf), "Light %d", i);

				if (ImGui::Selectable(buf, selected))
					m_uiLightIndex = i;

				if (selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndListBox();
		}


		ImGui::TableNextColumn();

		Light& light = L.m_lightsArray[m_uiLightIndex];

		float color[3] = { light.m_color.x, light.m_color.y, light.m_color.z };
		if (ImGui::ColorEdit3("Color", color))
		{
			light.m_color.x = color[0];
			light.m_color.y = color[1];
			light.m_color.z = color[2];
		}

		float intensity = light.m_color.w;
		if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 200.0f))
			light.m_color.w = intensity;

		float pos[3] = { light.m_position.x, light.m_position.y, light.m_position.z };
		if (ImGui::DragFloat3("Position", pos, 0.1f))
			light.m_position = Vec3(pos[0], pos[1], pos[2]);


		float dir[3] = { light.m_direction.x, light.m_direction.y, light.m_direction.z };
		if (ImGui::DragFloat3("Direction", dir, 0.02f, -1.0f, 1.0f))
		{
			Vec3 d = Vec3(dir[0], dir[1], dir[2]);

			if (d.GetLengthSquared() < 0.0001f)
				d = Vec3(0, 0, -1);

			light.m_direction = d.GetNormalized();
		}

		float innerAngle = ACosDegrees(light.m_innerDotThreshold);
		float outerAngle = ACosDegrees(light.m_outerDotThreshold);

		if (ImGui::DragFloat("Inner Angle", &innerAngle, 0.3f, 0.f, 89.f))
		{
			innerAngle = GetClamped(innerAngle, 0.f, outerAngle - 0.5f);
			light.m_innerDotThreshold = CosDegrees(innerAngle);
		}

		if (ImGui::DragFloat("Outer Angle", &outerAngle, 0.3f, 0.5f, 90.f))
		{
			outerAngle = Max(outerAngle, innerAngle + 0.5f);
			light.m_outerDotThreshold = CosDegrees(outerAngle);
		}

		ImGui::DragFloat("Ambience", &light.m_ambience, 0.01f, 0.f, 1.f);
		ImGui::DragFloat("Inner Radius", &light.m_innerRadius, 0.1f, 0.f);
		ImGui::DragFloat("Outer Radius", &light.m_outerRadius, 0.1f, 0.f);

		ImGui::Spacing();

		if (ImGui::Button("Delete Light"))
		{
			for (int j = m_uiLightIndex; j < L.m_numLights - 1; j++)
				L.m_lightsArray[j] = L.m_lightsArray[j + 1];

			L.m_numLights--;
			m_uiLightIndex = (int) GetClamped((float)m_uiLightIndex, 0.f, (float)L.m_numLights - 1.f);
		}

		ImGui::EndTable();
	}

	ImGui::Separator();

	if (L.m_numLights < MAX_LIGHTS)
	{
		if (ImGui::Button("Add Spot Light"))
		{
			Light& newLight = L.m_lightsArray[L.m_numLights++];

			newLight.m_color = Vec4(1.f, 0.95f, 0.8f, 20.f);
			newLight.m_position = Vec3(0, 0, 4);
			newLight.m_direction = Vec3(0, 0, -1);

			newLight.m_ambience = 0.f;
			newLight.m_innerRadius = 0.f;
			newLight.m_outerRadius = 15.f;

			newLight.m_innerDotThreshold = CosDegrees(12.f);
			newLight.m_outerDotThreshold = CosDegrees(25.f);

			m_uiLightIndex = L.m_numLights - 1;
		}

		ImGui::SameLine();

		if (ImGui::Button("Add Point Light"))
		{
			if (L.m_numLights >= 64)
			{
				return; 
			}

			Light& newLight = L.m_lightsArray[L.m_numLights];
			L.m_numLights++;

			newLight.m_color = Vec4(1.f, 1.f, 1.f, 10.f);
			newLight.m_position = Vec3(0, 0, 3);

			newLight.m_direction = Vec3(0, 0, 0);

			newLight.m_ambience = 0.1f;
			newLight.m_innerRadius = 0.5f;
			newLight.m_outerRadius = 12.f;

			newLight.m_innerDotThreshold = -1.f;
			newLight.m_outerDotThreshold = -2.f;

			m_uiLightIndex = L.m_numLights - 1;
		}
	}
	else
	{
		ImGui::TextDisabled("MAX LIGHTS REACHED");
	}
}


void Game::ShowSceneGameObjectsPanel()
{
	Scene* scene = GetCurrentScene();
	if (!scene)
	{
		ImGui::TextDisabled("No active scene.");
		return;
	}

	std::vector<GameObject*>& objs = scene->GetGameObjects();

	ImGui::SeparatorText("Objects");

	if (objs.empty())
	{
		ImGui::TextDisabled("No game objects in scene.");
		return;
	}

	ImGui::SetNextItemWidth(220.f);
	ImGui::InputText("Search", m_uiGOFilter, IM_ARRAYSIZE(m_uiGOFilter));

	int visibleCount = 0;
	for (int i = 0; i < (int)objs.size(); ++i)
	{
		GameObject* go = objs[i];
		if (!go) continue;

		const char* name = go->m_name.c_str();

		if (strcmp(name, "GameObject") == 0 || strcmp(name, "") == 0)
			continue;

		if (m_uiGOFilter[0] != '\0' && strstr(name, m_uiGOFilter) == nullptr)
			continue;

		++visibleCount;
	}

	if (visibleCount == 0)
	{
		ImGui::TextDisabled("No objects match filter (and \"GameObject\" is hidden).");
		return;
	}

	if (m_uiGOIndex < 0) m_uiGOIndex = 0;
	if (m_uiGOIndex >= visibleCount) m_uiGOIndex = visibleCount - 1;

	ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV;
	if (ImGui::BeginTable("##SceneObjLayout", 2, flags))
	{
		ImGui::TableSetupColumn("List", ImGuiTableColumnFlags_WidthFixed, 280.f);
		ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextColumn();
		ImGui::BeginChild("##GOList", ImVec2(0, 0), true);

		int visibleIdx = 0;
		int selectedRawIndex = -1;

		if (ImGui::BeginListBox("##GOListBox", ImVec2(-FLT_MIN, -FLT_MIN)))
		{
			for (int i = 0; i < (int)objs.size(); ++i)
			{
				GameObject* go = objs[i];
				if (!go) continue;

				const char* name = go->m_name.c_str();

				if (strcmp(name, "GameObject") == 0)
					continue;

				if (m_uiGOFilter[0] != '\0' && strstr(name, m_uiGOFilter) == nullptr)
					continue;

				bool selected = (visibleIdx == m_uiGOIndex);

				ImGui::PushID(i);
				if (ImGui::Selectable(name, selected))
				{
					m_uiGOIndex = visibleIdx;
				}
				ImGui::PopID();

				if (selected)
				{
					ImGui::SetItemDefaultFocus();
					selectedRawIndex = i;
				}

				++visibleIdx;
			}
			ImGui::EndListBox();
		}

		ImGui::EndChild();

		ImGui::TableNextColumn();
		ImGui::BeginChild("##GOInspector", ImVec2(0, 0), true);

		if (selectedRawIndex < 0)
		{
			int v = 0;
			for (int i = 0; i < (int)objs.size(); ++i)
			{
				GameObject* go = objs[i];
				if (!go) continue;

				const char* name = go->m_name.c_str();
				if (strcmp(name, "GameObject") == 0) continue;
				if (m_uiGOFilter[0] != '\0' && strstr(name, m_uiGOFilter) == nullptr) continue;

				if (v == m_uiGOIndex)
				{
					selectedRawIndex = i;
					break;
				}
				++v;
			}
		}

		GameObject* sel = (selectedRawIndex >= 0) ? objs[selectedRawIndex] : nullptr;
		if (!sel)
		{
			ImGui::TextDisabled("No selection.");
			ImGui::EndChild();
			ImGui::EndTable();
			return;
		}

		ImGui::SeparatorText("Selected");
		ImGui::Text("Name: %s", sel->m_name.c_str());

		// ---- Transform ----
		ImGui::SeparatorText("Transform");

		Vec3 pos = sel->GetPosition();
		Vec3 scl = sel->GetScale();
		EulerAngles ori = sel->GetOrientation();

		float p[3] = { pos.x, pos.y, pos.z };
		float s[3] = { scl.x, scl.y, scl.z };
		float r[3] = { ori.m_yawDegrees, ori.m_pitchDegrees, ori.m_rollDegrees };

		bool dirty = false;
		dirty |= ImGui::DragFloat3("Position", p, 0.05f);
		dirty |= ImGui::DragFloat3("Scale", s, 0.05f, 0.0f, 1000.0f);
		dirty |= ImGui::DragFloat3("YawPitchRoll", r, 0.2f);

		if (dirty)
		{
			sel->SetPosition(Vec3(p[0], p[1], p[2]));
			sel->SetScale(Vec3(s[0], s[1], s[2]));
			sel->SetOrientation(EulerAngles(r[0], r[1], r[2]));
		}

		// ---- Color ----
		ImGui::SeparatorText("Color");

		Rgba8 c = sel->GetColor();
		float col[4] = { c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f };

		if (ImGui::ColorEdit4("Tint", col))
		{
			Rgba8 nc(
				(uint8_t)(col[0] * 255.f),
				(uint8_t)(col[1] * 255.f),
				(uint8_t)(col[2] * 255.f),
				(uint8_t)(col[3] * 255.f)
			);
			sel->SetColor(nc);
		}

		// ---- Textures ----
		ImGui::SeparatorText("Textures");

		ImGui::InputText("Diffuse", m_uiDiffusePath, IM_ARRAYSIZE(m_uiDiffusePath));
		ImGui::SameLine();
		if (ImGui::Button("Load##D"))
		{
			sel->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile(m_uiDiffusePath);
		}

		ImGui::InputText("Normal", m_uiNormalPath, IM_ARRAYSIZE(m_uiNormalPath));
		ImGui::SameLine();
		if (ImGui::Button("Load##N"))
		{
			sel->m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile(m_uiNormalPath);
		}

		ImGui::InputText("SGE", m_uiSgePath, IM_ARRAYSIZE(m_uiSgePath));
		ImGui::SameLine();
		if (ImGui::Button("Load##S"))
		{
			sel->m_specGlossEmitTexture = g_theRenderer->CreateOrGetTextureFromFile(m_uiSgePath);
		}

		ImGui::EndChild();
		ImGui::EndTable();
	}
}

void Game::ShowForcesPanel()
{
	if (!ImGui::CollapsingHeader("Forces", ImGuiTreeNodeFlags_DefaultOpen))
		return;

	if (ImGui::Button("Add Gravity")) {
		ParticleForce f = ParticleForce::MakeParticleGravity(Vec3(0, 0, -1), 9.8f);
		f.m_enabled = 1;
		g_theParticleSystem->AddForce(f);
	}
	ImGui::SameLine();
	if (ImGui::Button("Add Direction")) {
		ParticleForce f = ParticleForce::MakeParticleDirectionForce(Vec3(1, 0, 0), 5.f, 10.f);
		f.m_enabled = 1;
		g_theParticleSystem->AddForce(f);
	}
	ImGui::SameLine();
	if (ImGui::Button("Add Point")) {
		ParticleForce f = ParticleForce::MakeParticlePointForce(Vec3(0, 0, 2), 50.f, 3.f);
		f.m_enabled = 1;
		g_theParticleSystem->AddForce(f);
	}
	ImGui::SameLine();
	if (ImGui::Button("Add FlowColumn")) {
		ParticleForce f = ParticleForce::MakeFlowColumn();
		f.m_enabled = 1;
		g_theParticleSystem->AddForce(f);
	}

	ImGui::Separator();

	const auto& forces = g_theParticleSystem->GetForces();
	if (forces.empty()) {
		ImGui::TextDisabled("No forces defined yet.");
		g_theParticleSystem->UploadForcesIfDirty();
		return;
	}

	if (m_uiForceIndex < 0 || m_uiForceIndex >= (int)forces.size())
		m_uiForceIndex = 0;

	auto typeToName = [](uint32_t t) -> const char* {
		switch (t) {
		case FORCE_GRAVITY:    return "Gravity";
		case FORCE_DIRECTION:  return "Direction";
		case FORCE_POINT:      return "Point";
		case FORCE_FLOWCOLUMN: return "Flow Column";
		default:               return "Unknown";
		}
		};

	if (ImGui::BeginTable("ForceTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
		ImGui::TableSetupColumn("Forces List", ImGuiTableColumnFlags_WidthFixed, 240.f);
		ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		ImGui::BeginChild("ForceListChild", ImVec2(0, 280), true);
		if (ImGui::BeginListBox("##Forces", ImVec2(-FLT_MIN, 0))) {
			for (int i = 0; i < (int)forces.size(); ++i) {
				const auto& f = forces[i];
				char label[128];
				snprintf(label, sizeof(label), "%d : %s", i, typeToName(f.m_forceType));
				bool selected = (i == m_uiForceIndex);
				if (ImGui::Selectable(label, selected)) {
					m_uiForceIndex = i;
				}
				if (selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndListBox();
		}

		bool canRemove = (m_uiForceIndex >= 0 && m_uiForceIndex < (int)forces.size());
		ImGui::BeginDisabled(!canRemove);
		if (ImGui::Button("Remove Selected")) {
			g_theParticleSystem->RemoveForce((uint32_t)m_uiForceIndex);
			if (m_uiForceIndex >= (int)g_theParticleSystem->GetForces().size())
				m_uiForceIndex = (int)g_theParticleSystem->GetForces().size() - 1;
			if (m_uiForceIndex < 0) m_uiForceIndex = 0;
		}
		ImGui::EndDisabled();

		ImGui::EndChild();

		ImGui::TableNextColumn();
		ImGui::BeginChild("ForceInspector", ImVec2(0, 280), true);

		ParticleForce f = forces[m_uiForceIndex];
		bool changed = false;

		ImGui::Text("Force #%d (%s)", m_uiForceIndex, typeToName(f.m_forceType));
		ImGui::Separator();

		bool enabled = (f.m_enabled != 0);
		if (ImGui::Checkbox("Enabled", &enabled)) {
			f.m_enabled = enabled ? 1u : 0u;
			changed = true;
		}

		static const char* typeNames[] = { "Gravity", "Direction", "Point", "FlowColumn" };
		static const uint32_t typeValues[] = { FORCE_GRAVITY, FORCE_DIRECTION, FORCE_POINT, FORCE_FLOWCOLUMN };
		int currentType = 0;
		for (int i = 0; i < IM_ARRAYSIZE(typeValues); ++i) {
			if (f.m_forceType == typeValues[i]) {
				currentType = i;
				break;
			}
		}
		if (ImGui::Combo("Type", &currentType, typeNames, IM_ARRAYSIZE(typeNames))) {
			uint32_t newType = typeValues[currentType];
			if (newType != f.m_forceType) {
				f.m_forceType = newType;
				changed = true;

				switch (newType) {
				case FORCE_GRAVITY:
					f.m_direction[0] = 0.0f; f.m_direction[1] = 0.0f; f.m_direction[2] = -1.0f;
					f.m_strength = 9.8f;
					f.m_range = 0.f;
					break;
				case FORCE_DIRECTION:
					f.m_direction[0] = 1.0f; f.m_direction[1] = 0.0f; f.m_direction[2] = 0.0f;
					f.m_strength = 5.f;
					f.m_range = 0.f;
					f.m_position[0] = 0.0f; f.m_position[1] = 0.0f; f.m_position[2] = 0.0f;
					break;
				case FORCE_POINT:
					f.m_position[0] = 0.0f; f.m_position[1] = 0.0f; f.m_position[2] = 2.0f;
					f.m_strength = 50.f;
					f.m_range = 3.f;
					break;
				case FORCE_FLOWCOLUMN:
					f.m_position[0] = 0.0f; f.m_position[1] = 0.0f; f.m_position[2] = 0.0f;
					f.m_direction[0] = 0.0f; f.m_direction[1] = 0.0f; f.m_direction[2] = 1.0f;
					f.m_range = 25.0f;
					f.m_strength = 65.0f;
					f.m_axialStrength = 50.0f;
					f.m_radialStrength = 35.0f;
					f.m_bottomRadius = 1.8f;
					f.m_topRadius = 9.0f;
					f.m_flags = FLOW_SWIRL_ENABLE | FLOW_RADIAL_ENABLE | FLOW_AXIAL_ENABLE;
					break;
				}
			}
		}

		ImGui::Separator();

		if (f.m_forceType == FORCE_GRAVITY || f.m_forceType == FORCE_DIRECTION) {
			ImGui::Text("Direction Force / Gravity");

			float pos[3] = { f.m_position[0], f.m_position[1], f.m_position[2] };
			if (ImGui::DragFloat3("Position (Local Center)", pos, 0.05f)) {
				f.m_position[0] = pos[0];
				f.m_position[1] = pos[1];
				f.m_position[2] = pos[2];
				changed = true;
			}

			if (ImGui::DragFloat("Range (0 = global)", &f.m_range, 0.1f, 0.f, 500.f)) changed = true;

			float dir[3] = { f.m_direction[0], f.m_direction[1], f.m_direction[2] };
			if (ImGui::DragFloat3("Direction", dir, 0.01f, -10.f, 10.f)) {
				f.m_direction[0] = dir[0];
				f.m_direction[1] = dir[1];
				f.m_direction[2] = dir[2];
				changed = true;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Normalize")) {
				Vec3 temp(f.m_direction[0], f.m_direction[1], f.m_direction[2]);
				float len = temp.GetLength();
				if (len > 1e-6f) {
					temp.Normalize();
					f.m_direction[0] = temp.x;
					f.m_direction[1] = temp.y;
					f.m_direction[2] = temp.z;
				}
				else {
					f.m_direction[0] = (f.m_forceType == FORCE_GRAVITY) ? 0.0f : 1.0f;
					f.m_direction[1] = 0.0f;
					f.m_direction[2] = (f.m_forceType == FORCE_GRAVITY) ? -1.0f : 0.0f;
				}
				changed = true;
			}

			if (ImGui::DragFloat("Strength", &f.m_strength, 0.1f, -200.f, 200.f)) changed = true;

			if (f.m_forceType == FORCE_DIRECTION && f.m_range > 0.0f) {
				ImGui::TextColored(ImVec4(1, 1, 0, 1), "Note: Using Point Force for local directional effect");
			}
		}
		else if (f.m_forceType == FORCE_POINT) {
			ImGui::Text("Point Attractor / Repeller");
			float pos[3] = { f.m_position[0], f.m_position[1], f.m_position[2] };
			if (ImGui::DragFloat3("Position", pos, 0.05f)) {
				f.m_position[0] = pos[0];
				f.m_position[1] = pos[1];
				f.m_position[2] = pos[2];
				changed = true;
			}
			if (ImGui::DragFloat("Strength", &f.m_strength, 0.5f, -500.f, 500.f)) changed = true;
			if (ImGui::DragFloat("Range", &f.m_range, 0.1f, 0.1f, 100.f)) changed = true;
		}
		else if (f.m_forceType == FORCE_FLOWCOLUMN) {
			ImGui::Text("Flow Column (Cylinder Field)");
			float pos[3] = { f.m_position[0], f.m_position[1], f.m_position[2] };
			if (ImGui::DragFloat3("Base Center", pos, 0.05f)) {
				f.m_position[0] = pos[0];
				f.m_position[1] = pos[1];
				f.m_position[2] = pos[2];
				changed = true;
			}

			float axis[3] = { f.m_direction[0], f.m_direction[1], f.m_direction[2] };
			if (ImGui::DragFloat3("Axis Direction", axis, 0.01f)) {
				f.m_direction[0] = axis[0];
				f.m_direction[1] = axis[1];
				f.m_direction[2] = axis[2];
				changed = true;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Normalize##Axis")) {
				Vec3 temp(f.m_direction[0], f.m_direction[1], f.m_direction[2]);
				float len = temp.GetLength();
				if (len > 1e-6f) {
					temp.Normalize();
					f.m_direction[0] = temp.x;
					f.m_direction[1] = temp.y;
					f.m_direction[2] = temp.z;
				}
				else {
					f.m_direction[0] = 0.0f;
					f.m_direction[1] = 0.0f;
					f.m_direction[2] = 1.0f;
				}
				changed = true;
			}

			if (ImGui::DragFloat("Height (Range)", &f.m_range, 0.1f, 0.1f, 1000.f)) changed = true;
			if (ImGui::DragFloat("Bottom Radius", &f.m_bottomRadius, 0.05f, 0.f, 100.f)) changed = true;
			if (ImGui::DragFloat("Top Radius", &f.m_topRadius, 0.05f, 0.f, 100.f)) changed = true;

			ImGui::SeparatorText("Forces");
			if (ImGui::DragFloat("Swirl Strength", &f.m_strength, 0.1f, -200.f, 200.f)) changed = true;
			if (ImGui::DragFloat("Axial Strength (along axis)", &f.m_axialStrength, 0.1f, -200.f, 200.f)) changed = true;
			if (ImGui::DragFloat("Radial Strength", &f.m_radialStrength, 0.1f, -200.f, 200.f)) changed = true;
		}

		if (changed) {
			g_theParticleSystem->SetForce((uint32_t)m_uiForceIndex, f);
		}

		ImGui::EndChild();
		ImGui::EndTable();
	}
}

void Game::ShowSceneSelectionPanel()
{
	if (!ImGui::CollapsingHeader("Scene Management", ImGuiTreeNodeFlags_DefaultOpen))
		return;

	Scene* cur = m_sceneManager.GetCurrentScene();

	static std::vector<std::pair<std::string, std::string>> s_sceneDisplay =
	{
		{"Frostwood Shelter", "Scene 1"},
		{"Rune Nexus", "Scene 2"},
		{"Pulse Sector-7", "Scene 3"},
		{"Eye of the Dunes", "Scene 4"},
		{"Bloomfall Sky", "Scene 5"},
		{"Stress Testing", "Scene 6"},
	};

	std::vector<int> validIndices;
	for (int i = 0; i < (int)s_sceneDisplay.size(); i++)
	{
		const std::string& realName = s_sceneDisplay[i].second;
		if (m_sceneManager.m_scenes.find(realName) != m_sceneManager.m_scenes.end())
		{
			validIndices.push_back(i);
		}
	}

	if (validIndices.empty())
	{
		ImGui::TextDisabled("No scenes registered.");
		return;
	}

	if (cur)
	{
		std::string curName = cur->GetName();
		for (int i = 0; i < (int)validIndices.size(); i++)
		{
			if (s_sceneDisplay[validIndices[i]].second == curName)
			{
				m_uiSceneIndex = i;
				break;
			}
		}
	}

	if (m_uiSceneIndex < 0) m_uiSceneIndex = 0;
	if (m_uiSceneIndex >= (int)validIndices.size())
		m_uiSceneIndex = (int)validIndices.size() - 1;

	ImGui::Text("Current Scene: %s",
		cur ? s_sceneDisplay[validIndices[m_uiSceneIndex]].first.c_str() : "None");

	ImGui::Separator();

	static float fade = 1.f;
	ImGui::DragFloat("Fade", &fade, 0.05f, 0.f, 10.f);

	const char* preview =
		s_sceneDisplay[validIndices[m_uiSceneIndex]].first.c_str();

	if (ImGui::BeginCombo("Scenes", preview))
	{
		for (int i = 0; i < (int)validIndices.size(); i++)
		{
			bool selected = (i == m_uiSceneIndex);

			const char* label =
				s_sceneDisplay[validIndices[i]].first.c_str();

			if (ImGui::Selectable(label, selected))
			{
				m_uiSceneIndex = i;

				const std::string& real =
					s_sceneDisplay[validIndices[i]].second;

				SwitchToScene(real, fade);
			}

			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	if (ImGui::Button("Prev"))
	{
		m_uiSceneIndex--;
		if (m_uiSceneIndex < 0)
			m_uiSceneIndex = (int)validIndices.size() - 1;

		SwitchToScene(
			s_sceneDisplay[validIndices[m_uiSceneIndex]].second,
			fade);
	}

	ImGui::SameLine();

	if (ImGui::Button("Next"))
	{
		m_uiSceneIndex++;
		if (m_uiSceneIndex >= (int)validIndices.size())
			m_uiSceneIndex = 0;

		SwitchToScene(
			s_sceneDisplay[validIndices[m_uiSceneIndex]].second,
			fade);
	}

	ImGui::SameLine();

	if (ImGui::Button("Reload"))
	{
		SwitchToScene(
			s_sceneDisplay[validIndices[m_uiSceneIndex]].second,
			fade);
	}

	ImGui::Separator();
	

	static std::vector<std::string> s_sceneDescriptions =
	{
		"This scene demonstrates layered effects using a sub-emitter system, where fire generates secondary smoke while snow particles are simulated simultaneously, showcasing multi-emitter interaction.",

		"This scene presents structured particle patterns achieved through precise emitter control, resulting in stable, repeatable motion and clearly defined spatial behavior.",

		"This is a high-density rain simulation, demonstrating efficient large-scale particle updates running in parallel on the GPU with continuous emission.",

		"In this scene, a flow field combines radial, tangential and axial forces to generate tornado-like motion, illustrating how multiple force components interact.",

		"This scene uses directional forces to produce smooth and coherent particle motion, demonstrating controlled and predictable behavior.",

		"Finally, this stress test demonstrates scalability with millions of particles, highlighting bandwidth and rendering as the primary performance bottlenecks."
	};

	ImGui::Spacing();
	ImGui::TextWrapped("%s",
		s_sceneDescriptions[validIndices[m_uiSceneIndex]].c_str());
}

void Game::DebugRender() const
{

}

void Game::DebugRenderParticleForces() const
{
	if (!g_theParticleSystem || !m_debugDraw) return;

	g_theRenderer->BeginCamera(m_player->GetCamera());

	const auto& forces = g_theParticleSystem->GetForces();

	std::vector<Vertex_PCU> verts;
	verts.reserve(forces.size() * 256);

	const Rgba8 gColor = Rgba8::YELLOW;
	const Rgba8 dColor = Rgba8::MAGENTA;
	const Rgba8 pColor = Rgba8::CYAN;
	const Rgba8 fColor = Rgba8::GREEN;
	const Rgba8 swirlColor = Rgba8::ORANGE;

	for (const ParticleForce& f : forces)
	{
		if (f.m_enabled == 0u) continue;

		switch (f.m_forceType)
		{
		case FORCE_GRAVITY:
		{
			Vec3 dir(f.m_direction[0], f.m_direction[1], f.m_direction[2]);
			float len = dir.GetLength();
			if (len > 1e-6f) dir /= len;
			else dir = Vec3(0.f, 0.f, 0.f);
			float arrowLen = fabsf(f.m_strength) * 0.6f;
			if (arrowLen < 0.15f) arrowLen = 0.15f;
			Vec3 start = Vec3(f.m_position[0], f.m_position[1], f.m_position[2]);
			Vec3 end = start + dir * arrowLen;
			AddVertsForArrow3D(verts, start, end, arrowLen * 0.02f, 0.9f, gColor);
		}
		break;

		case FORCE_DIRECTION:
		{
			Vec3 dir(f.m_direction[0], f.m_direction[1], f.m_direction[2]);
			float len = dir.GetLength();
			if (len > 1e-6f) dir /= len;
			else dir = Vec3(0.f, 0.f, 0.f);
			float arrowLen = fabsf(f.m_strength) * 0.6f;
			if (arrowLen < 0.15f) arrowLen = 0.15f;
			Vec3 start = Vec3(f.m_position[0], f.m_position[1], f.m_position[2]);
			Vec3 end = start + dir * arrowLen;
			AddVertsForArrow3D(verts, start, end, arrowLen * 0.02f, 0.9f, dColor);
			if (f.m_range > 0.f)
			{
				AddVertsForUVSphereZWireframe3D(verts, start, f.m_range, 16, 0.01f, dColor);
			}
		}
		break;

		case FORCE_POINT:
		{
			Vec3 pos(f.m_position[0], f.m_position[1], f.m_position[2]);
			AddVertsForSphere3D(verts, pos, 0.08f, pColor);
			if (f.m_range > 0.f)
			{
				AddVertsForUVSphereZWireframe3D(verts, pos, f.m_range, 16, 0.01f, pColor);
			}
		}
		break;

		case FORCE_FLOWCOLUMN:
		{
			Vec3 axis(f.m_direction[0], f.m_direction[1], f.m_direction[2]);
			float axisLen = axis.GetLength();
			if (axisLen > 1e-6f) axis /= axisLen;
			else axis = Vec3(0.f, 0.f, 1.f);

			float height = f.m_range;
			if (height < 0.f) height = 0.f;

			Vec3 base(f.m_position[0], f.m_position[1], f.m_position[2]);
			Vec3 top = base + axis * height;

			float br = f.m_bottomRadius;
			float tr = f.m_topRadius;

			AddVertsForLineSegment3D(verts, base, top, 0.03f, Rgba8(0, 255, 0, 255));

			AddVertsForCylinderWireframe3D(
				verts,
				base,
				top,
				br,
				tr,
				fColor,
				16
			);

			Vec3 ref = (fabsf(axis.z) < 0.99f) ? Vec3(0, 0, 1) : Vec3(1, 0, 0);
			Vec3 right = CrossProduct3D(axis, ref).GetNormalized();
			Vec3 forward = CrossProduct3D(right, axis).GetNormalized();

			const int stacks = 3;
			const int slices = 6;

			for (int hStep = 0; hStep <= stacks; hStep++)
			{
				float hT = (float)hStep / (float)stacks;
				Vec3 center = base + axis * (height * hT);
				float radius = br + (tr - br) * hT;

				for (int i = 0; i < slices; i++)
				{
					float angle = (float)i / (float)slices * 360.0f;
					Vec2 dir2 = Vec2::MakeFromPolarDegrees(angle);

					Vec3 radial = right * dir2.x + forward * dir2.y;
					Vec3 pos = center + radial * radius;

					Vec3 n = radial.GetNormalized();
					Vec3 tangential = CrossProduct3D(axis, n);

					float scale = 0.12f;
					float arrowRadius = 0.1f;

					if ((f.m_flags & FLOW_RADIAL_ENABLE) != 0u)
					{
						Vec3 dir = ((f.m_flags & FLOW_RADIAL_OUTWARD) != 0u) ? n : -n;
						Vec3 end = pos + dir * f.m_radialStrength * scale;
						AddVertsForArrow3D(verts, pos, end, arrowRadius, 0.7f, Rgba8(255, 0, 0, 255));
					}

					if ((f.m_flags & FLOW_SWIRL_ENABLE) != 0u)
					{
						Vec3 end = pos + tangential * f.m_strength * scale;
						AddVertsForArrow3D(verts, pos, end, arrowRadius, 0.7f, Rgba8(0, 0, 255, 255));
					}

					if ((f.m_flags & FLOW_AXIAL_ENABLE) != 0u)
					{
						Vec3 end = pos + axis * f.m_axialStrength * scale;
						AddVertsForArrow3D(verts, pos, end, arrowRadius, 0.7f, Rgba8(0, 255, 0, 255));
					}
				}
			}
		}
		break;

		default: break;
		}
	}

	if (!verts.empty())
	{
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetDepthMode(DepthMode::READ_ONLY_LESS_EQUAL);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->BindShader(g_theRenderer->GetShader("Data/Shaders/DefaultShader"));
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(verts);
	}

	g_theRenderer->EndCamera(m_player->GetCamera());
}



void Game::UpdateCameras(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	
}

void Game::SetupSceneContent()
{
	Scene* currentScene = GetCurrentScene();
	if (!currentScene) return;

	std::string sceneName = currentScene->GetName();

	if (sceneName == "Scene 1")
	{
		SetupScene1();
	}
	else if (sceneName == "Scene 2")
	{
		SetupScene2();
	}
	else if (sceneName == "Scene 3")
	{
		SetupScene3();
	}
	else if (sceneName == "Scene 4")
	{
		SetupScene4();
	}
	else if (sceneName == "Scene 5")
	{
		SetupScene5();
	}
	else if (sceneName == "Scene 6")
	{
		SetupScene6();

	}
}

void Game::SetupScene1()
{
 	Scene* scene = GetCurrentScene();
	if (!scene) return;

	m_skyTex = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene1/Sky.png");

	m_skyVerts.clear();
	AddVertsForSkySphere3D(m_skyVerts, Rgba8(200, 205, 210, 255));

	scene->ClearGameObjects();
	scene->ClearParticleEmitters();

	scene->SetupStartPosAndOrientation(Vec3(-3.f, 0.1f, 0.68f), EulerAngles(-14.4f, -12.2f, 0.f));
	scene->ResetPlayer(m_player);

	const int tilesX = 20;
	const int tilesY = 20;
	const float tileSize = 2.f;

	float offsetX = (tilesX - 1) * tileSize * 0.5f;
	float offsetY = (tilesY - 1) * tileSize * 0.5f;

	for (int y = 0; y < tilesY; ++y)
	{
		for (int x = 0; x < tilesX; ++x)
		{
			GameObject* tile = scene->CreateGameObject(this, "GameObject");

			tile->InitializeVertsFromType(ObjectType::CUBE);
			tile->SetScale(Vec3(2.f, 2.f, 1.f));
			tile->SetColor(Rgba8::WHITE);

			tile->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene1/snowyground_d.png");
			tile->m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene1/snowyground_n.png");
			tile->m_specGlossEmitTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene1/snowyground_sge.png");


			float posX = (float)x * tileSize - offsetX;
			float posY = (float)y * tileSize - offsetY;

			tile->SetPosition(Vec3(posX, posY, -0.5f));
			tile->SetColor(Rgba8::LIGHT_GRAY);
		}
	}


/*
	GameObject* obj2 = scene->CreateGameObject(this, "Sphere");
	obj2->InitializeVertsFromType(ObjectType::SPHERE);
	obj2->SetPosition(Vec3(0.f, 0.f, 5.f));
	obj2->SetColor(Rgba8::WHITE);
	obj2->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Cobblestone_Diffuse.png");
	obj2->m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Cobblestone_Normal.png");
	obj2->m_specGlossEmitTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Cobblestone_SpecGlossEmit.png");*/

	GameObject* obj3 = scene->CreateGameObject(this, "CampFire");
	obj3->InitializeVertsFromFile("Data/Meshes/Camp_Fire");
	obj3->SetPosition(Vec3(0.f, 0.f, 0.f));
	obj3->SetScale(Vec3(0.2f, 0.2f, 0.2f));
	obj3->SetColor(Rgba8::DARK_GRAY);
	
	obj3->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene1/rock.png");
	obj3->m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene1/rock_n.png");
	obj3->m_specGlossEmitTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene1/rock_sge.png");

	GameObject* cabin = scene->CreateGameObject(this, "cabin");
	cabin->InitializeVertsFromFile("Data/Meshes/cabin");
	cabin->SetPosition(Vec3(3.25f, 3.8f, 1.75f));
	cabin->SetScale(Vec3(3.0f, 3.0f, 3.0f));
	cabin->SetOrientation(EulerAngles(-40.f, 0.f, 90.f));
	cabin->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene1/cabin.png");
	cabin->m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene1/cabin_n.png");
	cabin->m_specGlossEmitTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene1/cabin_sge.png");

	GameObject* tent = scene->CreateGameObject(this, "tent");
	tent->InitializeVertsFromFile("Data/Meshes/tent");
	tent->SetPosition(Vec3(0.25f, -4.1f, 0.8f));
	tent->SetScale(Vec3(4.0f, 4.0f, 4.0f));
	tent->SetOrientation(EulerAngles(-174.f, 0.f, 90.f));
	tent->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene1/tent.png");
	tent->m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene1/tent_n.png");
	tent->m_specGlossEmitTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene1/tent_sge.png");

	const char* g_treeMeshes[3] =
	{
		"Data/Meshes/tree1",
		"Data/Meshes/tree3",
		"Data/Meshes/tree5"
	};

	const char* g_treeTextures[3] =
	{
		"Data/Images/Scene1/tree1.png",
		"Data/Images/Scene1/tree3.png",
		"Data/Images/Scene1/tree5.png"
	};

	const char* g_treeNTextures[3] =
	{
		"Data/Images/Scene1/tree1_n.png",
		"Data/Images/Scene1/tree3_n.png",
		"Data/Images/Scene1/tree5_n.png"
	};

	const char* g_treeSGETextures[3] =
	{
		"Data/Images/Scene1/tree1_sge.png",
		"Data/Images/Scene1/tree3_sge.png",
		"Data/Images/Scene1/tree5_sge.png"
	};
	
	GameObject* treePresets[3] = {};

	for (int i = 0; i < 3; ++i)
	{
		treePresets[i] = scene->CreateGameObject(this, Stringf("GameObject", i));
		treePresets[i]->InitializeVertsFromFile(g_treeMeshes[i]);
		treePresets[i]->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile(g_treeTextures[i]);
		treePresets[i]->m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile(g_treeNTextures[i]);
		treePresets[i]->m_specGlossEmitTexture = g_theRenderer->CreateOrGetTextureFromFile(g_treeSGETextures[i]);

		treePresets[i]->SetPosition(Vec3(10000.f, 10000.f, 10000.f));
		treePresets[i]->SetScale(Vec3(1.f, 1.f, 1.f));
		treePresets[i]->SetOrientation(EulerAngles(0.f, 0.f, 0.f));
	}

	float innerRadius = 10.0f;
	float outerRadius = 15.0f;
	int   treeCount = 30;
	Vec3  center(3.f, 3.f, 0.f);

	RandomNumberGenerator rng;

	for (int i = 0; i < treeCount; ++i)
	{
		float angleDeg = rng.RollRandomFloatInRange(0.f, 360.f);
		float radius = rng.RollRandomFloatInRange(innerRadius, outerRadius);

		int type = rng.RollRandomIntInRange(0, 2);

		GameObject* tree = scene->CreateGameObject(this, Stringf("tree_%d_%d", type, i));

		tree->InitializeVertsFromPreset(*treePresets[type]);

		float scale = rng.RollRandomFloatInRange(3.0f, 6.0f);
		tree->SetScale(Vec3(scale, scale, scale));

		float z = scale / 2.f;

		Vec2 offset2D = Vec2::MakeFromPolarDegrees(angleDeg, radius);
		Vec3 pos = center + Vec3(offset2D.x, offset2D.y, z);

		tree->SetPosition(pos);
		tree->SetColor(Rgba8(225, 255, 225));
		

		float yaw = rng.RollRandomFloatInRange(0.f, 360.f);
		tree->SetOrientation(EulerAngles(yaw, 0.f, 90.f));
	}


	Lights& L = scene->GetLights();
	L.m_sunColor = Vec4(1.0f, 1.0f, 1.0f, 2.0f);
	L.m_sunDirection = Vec3(0, -1, -1).GetNormalized();
	L.m_numLights = 0;

	/*if (L.m_numLights < MAX_LIGHTS) 
	{
		Light& pl = L.m_lightsArray[L.m_numLights++];
		pl.m_color = Vec4(1.f, 1.f, 1.0f, 3.0f);
		pl.m_position = Vec3(0, 0, 5);
		pl.EMPTY_PADDING = 0.f;
		pl.m_direction = Vec3(0, 0, 0);
		pl.m_ambience = 0.1f;
		pl.m_innerRadius = 0.5f;
		pl.m_outerRadius = 6.0f;
		pl.m_innerDotThreshold = -1.f;
		pl.m_outerDotThreshold = -2.f;
	}*/

	/*if (L.m_numLights < MAX_LIGHTS) 
	{
		Light& sl = L.m_lightsArray[L.m_numLights++];
		sl.m_color = Vec4(1.f, 1.f, 1.0f, 9.0f);
		sl.m_position = Vec3(0, 0, 5);
		sl.EMPTY_PADDING = 0.f;
		sl.m_direction = Vec3(0, 0, -1).GetNormalized();
		sl.m_ambience = 0.0f;
		sl.m_innerRadius = 5.0f;
		sl.m_outerRadius = 7.0f;
		sl.m_innerDotThreshold = CosDegrees(30.f);
		sl.m_outerDotThreshold = CosDegrees(35.f);
	}*/

	{
		ParticleEmitterConfig fireConfig;
		fireConfig.name = "FireWithSmoke";

		fireConfig.mainStage.texPath = "Data/Images/Scene1/fire.png";
		fireConfig.blendMode = BlendMode::ALPHA_ADDITIVE;
		fireConfig.position = Vec3(0.f, 0.f, 0.f);
		fireConfig.spawnArea = Vec3(0.3f, 0.3f, 0.f);
		fireConfig.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.5f);
		fireConfig.mainStage.velocityVariance = Vec3(0.2f, 0.2f, 0.2f);

		fireConfig.spawnRate = 10000.f;
		fireConfig.mainStage.lifetime = 0.5f;
		fireConfig.mainStage.lifetimeVariance = 0.3f;

		fireConfig.mainStage.startColor = Rgba8(200, 200, 200, 200);
		fireConfig.mainStage.endColor = Rgba8(150, 150, 150, 0);

		fireConfig.mainStage.startSize = 0.05f;
		fireConfig.mainStage.endSize = 0.2f;

		fireConfig.mainStage.baseAngularVelocity = 0.f;
		fireConfig.mainStage.angularVariance = 10.f;

		fireConfig.isLooping = true;
		fireConfig.enabled = true;
		fireConfig.maxParticles = 50000;

		fireConfig.useSubStage = true;
		fireConfig.subStage.texPath = "Data/Images/Scene1/smoke.png";

		fireConfig.subStage.lifetime = 1.0f;
		fireConfig.subStage.lifetimeVariance = 0.05f;

		fireConfig.subStage.startColor = Rgba8(255, 255, 255, 50);
		fireConfig.subStage.endColor = Rgba8(0, 0, 0, 0);

		fireConfig.subStage.startSize = 0.1f;
		fireConfig.subStage.endSize = 0.3f;

		fireConfig.subStage.baseVelocity = Vec3(0.f, 0.f, 0.1f);
		fireConfig.subStage.velocityVariance = Vec3(0.5f, 0.5f, 1.0f);
		fireConfig.subStage.prob = 0.1f;

		ParticleEmitter* fireWithSmoke = g_theParticleSystem->CreateEmitter(fireConfig);
		scene->AddParticleEmitter(fireWithSmoke);
	}

	{
		float snowHeight = 20.0f;
		float spawnThickness = 3.0f;
		float areaHalfSize = 50.0f;

		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "SnowField";
		cfg.mainStage.texPath = "Data/Images/Scene1/snow.png";
		cfg.blendMode = BlendMode::ALPHA_ADDITIVE;

		cfg.position = Vec3(0.f, 0.f, snowHeight);

		cfg.spawnArea = Vec3(areaHalfSize,
			areaHalfSize,
			spawnThickness * 0.5f);

		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, -1.0f);
		cfg.mainStage.velocityVariance = Vec3(0.8f, 0.8f, 0.5f);

		cfg.mainStage.lifetime = 30.0f;
		cfg.mainStage.lifetimeVariance = 2.0f;

		cfg.mainStage.startColor = Rgba8(255, 255, 255, 20);
		cfg.mainStage.endColor = Rgba8(255, 255, 255, 20);

		cfg.mainStage.startSize = 0.3f;
		cfg.mainStage.endSize = 0.08f;

		cfg.spawnRate = 500000.0f;
		cfg.maxParticles = 5000000;

		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;

		cfg.noiseStrength = 0.8f;
		cfg.noiseFrequency = 0.4f;

		ParticleEmitter* snowEmitter = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(snowEmitter);
	}

	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "Smoke";
		cfg.mainStage.texPath = "Data/Images/Scene1/smoke.png";
		cfg.blendMode = BlendMode::ALPHA;
		cfg.position = Vec3(1.82f, 3.22f, 3.81f);
		cfg.spawnArea = Vec3(0.1f, 0.1f, 0.f);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.5f);
		cfg.mainStage.velocityVariance = Vec3(0.1f, 0.1f, 0.f);
		cfg.mainStage.lifetime = 5.f;
		cfg.mainStage.lifetimeVariance = 1.f;
		cfg.mainStage.startColor = Rgba8(20, 20, 20, 5);
		cfg.mainStage.endColor = Rgba8(20, 20, 20, 0);
		cfg.spawnRate = 1000.f;
		cfg.maxParticles = 100000;
		cfg.noiseStrength = 0.8f;
		cfg.noiseFrequency = 0.4f;

		ParticleEmitter* smokeEmitter = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(smokeEmitter);

	}


}

void Game::SetupScene3()
{
	Scene* scene = GetCurrentScene();
	if (!scene) return;

	m_skyTex = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene3/Sky.png");

	m_skyVerts.clear();
	AddVertsForSkySphere3D(m_skyVerts, Rgba8(200, 200, 200, 255));

	scene->ClearGameObjects();
	scene->ClearParticleEmitters();

	scene->SetupStartPosAndOrientation(Vec3(26.5f, 13.f, 3.f), EulerAngles(-152.f, -10, 0.f));
	scene->ResetPlayer(m_player);

	Lights& L = scene->GetLights();
	L.m_sunColor = Vec4(1.f, 1.f, 1.f, 0.1f);
	L.m_sunDirection = Vec3(0.f, 0.6f, -0.7f).GetNormalized();
	L.m_numLights = 0;

	const char* kRainTexPath = "Data/Images/Scene3/rain.png";
	const char* kSplashTexPath = "Data/Images/Scene3/waterSplash2.png";

	float groundZ = -1.0f;
	float rainHeight = groundZ + 50.0f;
	float spawnThickness = 2.0f;
	float areaHalfSize = 100.0f;

	float tileSize = 10.0f;
	int   tilesPerAxis = (int)((areaHalfSize * 2.0f) / tileSize);

	const char* g_brickTextures[3] =
	{
		"Data/Images/Scene3/brick1.png",
		"Data/Images/Scene3/brick2.png",
		"Data/Images/Scene3/brick3.png"
	};

	const char* g_brickNTextures[3] =
	{
		"Data/Images/Scene3/brick1_n.png",
		"Data/Images/Scene3/brick2_n.png",
		"Data/Images/Scene3/brick3_n.png"
	};

	const char* g_brickSGETextures[3] =
	{
		"Data/Images/Scene3/brick1_sge.png",
		"Data/Images/Scene3/brick2_sge.png",
		"Data/Images/Scene3/brick3_sge.png"
	};
	
	RandomNumberGenerator rng;

	for (int y = 0; y < tilesPerAxis; ++y)
	{
		for (int x = 0; x < tilesPerAxis; ++x)
		{
			float gx = -areaHalfSize + (x + 0.5f) * tileSize;
			float gy = -areaHalfSize + (y + 0.5f) * tileSize;

			GameObject* ground = scene->CreateGameObject(this, "GameObject");
			ground->InitializeVertsFromType(ObjectType::CUBE);
			ground->SetPosition(Vec3(gx, gy, -0.5f));
			ground->SetColor(Rgba8::WHITE);
			ground->SetScale(Vec3(tileSize, tileSize, 1.f));

			int randomTexID = rng.RollRandomIntInRange(0, 2);
			ground->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile(g_brickTextures[randomTexID]);
			ground->m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile(g_brickNTextures[randomTexID]);
			ground->m_specGlossEmitTexture = g_theRenderer->CreateOrGetTextureFromFile(g_brickSGETextures[randomTexID]);
		}
	}

	const char* kBuildingMeshes[6] = {
		"Data/Meshes/building1",
		"Data/Meshes/building2",
		"Data/Meshes/building3",
		"Data/Meshes/building4",
		"Data/Meshes/building1",
		"Data/Meshes/building2"
	};

	const char* kBuildingTexPaths[6] = {
		"Data/Images/Scene3/building1.png",
		"Data/Images/Scene3/building2.png",
		"Data/Images/Scene3/building3.png",
		"Data/Images/Scene3/building4.png",
		"Data/Images/Scene3/building1.png",
		"Data/Images/Scene3/building2.png"
	};

	const char* kBuildingNTexPaths[6] = {
		"Data/Images/Scene3/building1_n.png",
		"Data/Images/Scene3/building2_n.png",
		"Data/Images/Scene3/building3_n.png",
		"Data/Images/Scene3/building4_n.png",
		"Data/Images/Scene3/building1_n.png",
		"Data/Images/Scene3/building2_n.png"
	};

	const char* kBuildingSGETexPaths[6] = {
		"Data/Images/Scene3/building1_sge.png",
		"Data/Images/Scene3/building2_sge.png",
		"Data/Images/Scene3/building3_sge.png",
		"Data/Images/Scene3/building4_sge.png",
		"Data/Images/Scene3/building1_sge.png",
		"Data/Images/Scene3/building2_sge.png"
	};

	Texture* buildingTex[6], * buildingNTex[6], * buildingSGETex[6];
	for (int i = 0; i < 6; ++i) {
		buildingTex[i] = g_theRenderer->CreateOrGetTextureFromFile(kBuildingTexPaths[i]);
		buildingNTex[i] = g_theRenderer->CreateOrGetTextureFromFile(kBuildingNTexPaths[i]);
		buildingSGETex[i] = g_theRenderer->CreateOrGetTextureFromFile(kBuildingSGETexPaths[i]);
	}

	struct BuildingParams {
		float zHeight;
		float scale;
		float yawDegrees;
		Vec3 positionOffset;
	};

	BuildingParams params[6] = {
	{ 30.0f, 32.0f, 0.0f, Vec3(-40.0f,  30.0f, 0) },
	{ 18.0f, 20.0f, 0.0f, Vec3(0.0f,  40.0f, 0) },
	{ 26.0f, 28.0f, 0.0f, Vec3(40.0f,  30.0f, 0) },

	{ 22.0f, 24.0f, 0.0f, Vec3(-40.0f, -30.0f, 0) },
	{ 38.0f, 40.0f, 0.0f, Vec3(0.0f, -30.0f, 0) },
	{ 18.0f, 20.0f, 180.0f, Vec3(40.0f, -40.0f, 0) }
	};

	GameObject* buildings[6] = { nullptr };

	for (int i = 0; i < 6; ++i)
	{
		char name[32];
		snprintf(name, sizeof(name), "Building%d", i + 1);

		buildings[i] = scene->CreateGameObject(this, name);
		buildings[i]->InitializeVertsFromFile(kBuildingMeshes[i]);

		Vec3 pos = Vec3(0.f, 0.f, params[i].zHeight) + params[i].positionOffset;
		buildings[i]->SetPosition(pos);

		float s = params[i].scale;
		buildings[i]->SetScale(Vec3(s, s, s));

		buildings[i]->SetOrientation(EulerAngles(params[i].yawDegrees, 0.f, 90.f));

		buildings[i]->SetColor(Rgba8::WHITE);
		buildings[i]->m_diffuseTexture = buildingTex[i];
		buildings[i]->m_normalTexture = buildingNTex[i];
		buildings[i]->m_specGlossEmitTexture = buildingSGETex[i];
	}

	{
		GameObject* car = scene->CreateGameObject(this, "Car");
		car->InitializeVertsFromFile("Data/Meshes/car");
		car->SetPosition(Vec3(-27.f, -16.9f, 2.f));
		car->SetScale(Vec3(10.f, 10.f, 10.f));
		car->SetOrientation(EulerAngles(-70.f, 0.f, 90.f));
		car->SetColor(Rgba8::WHITE);
		car->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene3/car.png");
		car->m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene3/car_n.png");
		car->m_specGlossEmitTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene3/car_sge.png");
	}

	{
		GameObject* car = scene->CreateGameObject(this, "Car");
		car->InitializeVertsFromFile("Data/Meshes/car");
		car->SetPosition(Vec3(11.f, 16.9f, 2.f));
		car->SetScale(Vec3(10.f, 10.f, 10.f));
		car->SetOrientation(EulerAngles(30.f, 0.f, 90.f));
		car->SetColor(Rgba8::WHITE);
		car->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene3/car.png");
		car->m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene3/car_n.png");
		car->m_specGlossEmitTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene3/car_sge.png");
	}

	{
		GameObject* car = scene->CreateGameObject(this, "Car");
		car->InitializeVertsFromFile("Data/Meshes/car");
		car->SetPosition(Vec3(34.f, -17.f, 2.f));
		car->SetScale(Vec3(10.f, 10.f, 10.f));
		car->SetOrientation(EulerAngles(-196.f, 0.f, 90.f));
		car->SetColor(Rgba8::WHITE);
		car->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene3/car.png");
		car->m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene3/car_n.png");
		car->m_specGlossEmitTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene3/car_sge.png");
	}

	for (int i = 0; i < 4; i++)
	{
		GameObject* left = scene->CreateGameObject(this, Stringf("StreetLight_l%d", i));
		GameObject* right = scene->CreateGameObject(this, Stringf("StreetLight_r%d", i));

		float spacing = 40.0f;
		float x = -60.0f + i * spacing;

		// ===== LEFT MODEL =====
		left->SetPosition(Vec3(x, 15.0f, 10.0f));
		left->InitializeVertsFromFile("Data/Meshes/streetlight1");
		left->SetScale(Vec3(10.0f, 10.0f, 10.0f));
		left->SetOrientation(EulerAngles(0.f, 0.f, 90.f));
		left->SetColor(Rgba8::WHITE);
		left->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene3/streetlight1.png");

		// ⭐ LEFT POINT LIGHT
		Light& leftLight = L.m_lightsArray[L.m_numLights++];

		leftLight.m_color = Vec4(0.4f, 0.7f, 1.0f, 1.f);
		leftLight.m_position = Vec3(x, 15.0f, 16.5f);
		leftLight.m_direction = Vec3(0, 0, 0);

		leftLight.m_ambience = 0.05f;
		leftLight.m_innerRadius = 5.5f;
		leftLight.m_outerRadius = 34.f;

		leftLight.m_innerDotThreshold = -1.f;
		leftLight.m_outerDotThreshold = -2.f;


		// ===== RIGHT MODEL =====
		right->SetPosition(Vec3(x, -15.0f, 10.0f));
		right->InitializeVertsFromFile("Data/Meshes/streetlight1");
		right->SetScale(Vec3(10.0f, 10.0f, 10.0f));
		right->SetOrientation(EulerAngles(180.f, 0.f, 90.f));
		right->SetColor(Rgba8::WHITE);
		right->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene3/streetlight1.png");

		// ⭐ RIGHT POINT LIGHT
		Light& rightLight = L.m_lightsArray[L.m_numLights++];

		rightLight.m_color = Vec4(1.0f, 0.3f, 0.7f, 1.f);
		rightLight.m_position = Vec3(x, -15.0f, 16.5f);
		rightLight.m_direction = Vec3(0, 0, 0);

		rightLight.m_ambience = 0.05f;
		rightLight.m_innerRadius = 5.5f;
		rightLight.m_outerRadius = 34.f;

		rightLight.m_innerDotThreshold = -1.f;
		rightLight.m_outerDotThreshold = -2.f;
	}


	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "BigRain";

		cfg.mainStage.texPath = kRainTexPath;
		cfg.blendMode = BlendMode::ALPHA;

		cfg.position = Vec3(0.f, 0.f, rainHeight);
		cfg.spawnArea = Vec3(areaHalfSize * 2,
			areaHalfSize * 2,
			spawnThickness * 0.5f);

		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, -12.0f);
		cfg.mainStage.velocityVariance = Vec3(0.7f, 0.7f, 8.0f);

		cfg.mainStage.lifetime = 99.0f;
		cfg.mainStage.lifetimeVariance = 0.f;

		cfg.mainStage.startColor = Rgba8(255, 255, 255, 100);
		cfg.mainStage.endColor = Rgba8(200, 200, 255, 0);

		cfg.mainStage.startSize = 0.2f;
		cfg.mainStage.endSize = 0.2f;
		cfg.mainStage.billboardType = 0u;

		cfg.spawnRate = 100000.0f;
		cfg.maxParticles = 1000000;

		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;

		cfg.useSubStage = true;
		cfg.subStage.texPath = kSplashTexPath;

		cfg.subStage.lifetime = 0.35f;
		cfg.subStage.lifetimeVariance = 0.10f;

		cfg.subStage.startColor = Rgba8(180, 180, 220, 200);
		cfg.subStage.endColor = Rgba8(180, 180, 220, 0);

		cfg.subStage.startSize = 1.8f;
		cfg.subStage.endSize = 2.5f;

		cfg.subStage.baseVelocity = Vec3(0.f, 0.f, 0.f);
		cfg.subStage.velocityVariance = Vec3(0.f, 0.f, 0.0f);

		cfg.subStage.prob = 0.5f;
		cfg.subStage.billboardType = 1u;

		ParticleEmitter* rainEmitter = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(rainEmitter);
	}



}

void Game::SetupScene4()
{
	Scene* scene = GetCurrentScene();
	if (!scene) return;

	m_skyTex = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene4/Sky.png");
	m_skyVerts.clear();
	AddVertsForSkySphere3D(m_skyVerts, Rgba8(255, 255, 255, 255));

	scene->ClearGameObjects();
	scene->ClearParticleEmitters();
	g_theParticleSystem->ClearForces();

	scene->SetupStartPosAndOrientation(Vec3(-40.f, 33.1f, 2.68f), EulerAngles(-48.4f, -14.2f, 0.f));
	scene->ResetPlayer(m_player);

	const int tilesX = 50;
	const int tilesY = 50;
	const float tileSize = 2.f;
	const float halfW = (tilesX * tileSize) * 0.5f;
	const float halfH = (tilesY * tileSize) * 0.5f;

	Texture* sandD = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene4/sand_d.png");
	Texture* sandN = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene4/sand_n.png");
	Texture* sandSGE = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene4/sand_sge.png");

	for (int y = 0; y < tilesY; ++y)
	{
		for (int x = 0; x < tilesX; ++x)
		{
			GameObject* tile = scene->CreateGameObject(this, "GameObject");
			tile->InitializeVertsFromType(ObjectType::CUBE);
			tile->SetScale(Vec3(tileSize, tileSize, 1.f));
			tile->SetPosition(Vec3(-halfW + (x + 0.5f) * tileSize,
				-halfH + (y + 0.5f) * tileSize,
				-0.5f));
			tile->SetColor(Rgba8(100, 100, 100, 255));
			tile->m_diffuseTexture = sandD;
			tile->m_normalTexture = sandN;
			tile->m_specGlossEmitTexture = sandSGE;
		}
	}

	const char* g_cactusMeshes[2] =
	{
		"Data/Meshes/cactus1",
		"Data/Meshes/cactus2" 
	};

	const char* g_cactusDiffuse[2] =
	{
		"Data/Images/Scene4/cactus1.png",
		"Data/Images/Scene4/cactus2.png"
	};

	const char* g_cactusNormal[2] =
	{
		"Data/Images/Scene4/cactus1_n.png",
		"Data/Images/Scene4/cactus2_n.png"
	};

	const char* g_cactusSGE[2] =
	{
		"Data/Images/Scene4/cactus1_sge.png",
		"Data/Images/Scene4/cactus2_sge.png"
	};


	GameObject* cactusPresets[2] = {};
	for (int i = 0; i < 2; ++i)
	{
		cactusPresets[i] = scene->CreateGameObject(this, Stringf("CactusPreset_%d", i));
		cactusPresets[i]->InitializeVertsFromFile(g_cactusMeshes[i]);
		cactusPresets[i]->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile(g_cactusDiffuse[i]);
		cactusPresets[i]->m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile(g_cactusNormal[i]);
		cactusPresets[i]->m_specGlossEmitTexture = g_theRenderer->CreateOrGetTextureFromFile(g_cactusSGE[i]);

		cactusPresets[i]->SetPosition(Vec3(10000.f, 10000.f, 10000.f));
		cactusPresets[i]->SetScale(Vec3(1.f, 1.f, 1.f));
		cactusPresets[i]->SetOrientation(EulerAngles(0.f, 0.f, 0.f));
	}

	RandomNumberGenerator rng; 
	int cactusCount = 28;      
	float innerRadius = 12.0f; 
	float outerRadius = 45.0f; 
	Vec3 center(0.f, 0.f, 0.f);

	for (int i = 0; i < cactusCount; ++i)
	{
		float angleDeg = rng.RollRandomFloatInRange(0.f, 360.f);
		float radius = rng.RollRandomFloatInRange(innerRadius, outerRadius);
		int type = rng.RollRandomIntInRange(0, 1); 

		GameObject* cactus = scene->CreateGameObject(this, Stringf("cactus_%d_%d", type, i));
		cactus->InitializeVertsFromPreset(*cactusPresets[type]);

		float scale = rng.RollRandomFloatInRange(2.8f, 5.5f);
		cactus->SetScale(Vec3(scale, scale, scale));

		float zOffset = scale * 0.85f; 
		Vec2 offset2D = Vec2::MakeFromPolarDegrees(angleDeg, radius);
		Vec3 pos = center + Vec3(offset2D.x, offset2D.y, zOffset);
		cactus->SetPosition(pos);

		Rgba8 baseColor(220, 220, 220, 255);

		int offsetR = rng.RollRandomIntInRange(-12, 12);
		int offsetG = rng.RollRandomIntInRange(-15, 10);
		int offsetB = rng.RollRandomIntInRange(-8, 8);

		baseColor.r = (unsigned char)GetClamped((float)baseColor.r + (float)offsetR, 40.f, 90.f);
		baseColor.g = (unsigned char)GetClamped((float)baseColor.g + (float)offsetG, 50.f, 100.f);
		baseColor.b = (unsigned char)GetClamped((float)baseColor.b + (float)offsetB, 15.f, 45.f);

		cactus->SetColor(baseColor);

		float yaw = rng.RollRandomFloatInRange(0.f, 360.f);
		cactus->SetOrientation(EulerAngles(yaw, 0.f, 90.f));
	}

	Lights& L = scene->GetLights();
	L.m_sunColor = Vec4(0.72f, 0.68f, 0.60f, 1.05f);
	L.m_sunDirection = Vec3(0.70f, -0.10f, -0.70f).GetNormalized();
	L.m_numLights = 0;

	float angle = m_tornadoTime * m_tornadoMoveSpeed * 360.f;
	Vec2 circle = Vec2::MakeFromPolarDegrees(angle, m_tornadoMoveRadius);

	Vec3 pos = Vec3(circle.x, circle.y, 0.f);

	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "TornadoCoreFine";
		cfg.mainStage.texPath = "Data/Images/Scene4/dust1.png";
		cfg.blendMode = BlendMode::ALPHA;
		cfg.position = pos;
		cfg.spawnArea = Vec3(1.4f, 1.4f, 5.5f);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.04f);
		cfg.mainStage.velocityVariance = Vec3(0.35f, 0.35f, 0.50f);
		cfg.mainStage.lifetime = 5.5f;
		cfg.mainStage.lifetimeVariance = 0.0f;
		cfg.mainStage.startColor = Rgba8(180, 160, 140, 255); 
		cfg.mainStage.endColor = Rgba8(220, 200, 160, 50);
		cfg.mainStage.startSize = 0.05f;
		cfg.mainStage.endSize = 1.2f;
		cfg.spawnRate = 100000.0f; 
		cfg.maxParticles = 1500000;
		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;
		cfg.noiseStrength = 1.0f;
		cfg.noiseFrequency = 1.0f;
		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
		m_tornadoEmitters.push_back(em);
	}

	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "TornadoCoreFine";
		cfg.mainStage.texPath = "Data/Images/Scene4/dust1.png";
		cfg.blendMode = BlendMode::ALPHA;
		cfg.position = pos;
		cfg.spawnArea = Vec3(1.4f, 1.4f, 5.5f);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.04f);
		cfg.mainStage.velocityVariance = Vec3(0.35f, 0.35f, 0.50f);
		cfg.mainStage.lifetime = 5.5f;
		cfg.mainStage.lifetimeVariance = 0.0f;
		cfg.mainStage.startColor = Rgba8(100, 60, 40, 255); 
		cfg.mainStage.endColor = Rgba8(220, 200, 180, 30);
		cfg.mainStage.startSize = 0.02f;
		cfg.mainStage.endSize = 1.2f;
		cfg.spawnRate = 100000.0f; 
		cfg.maxParticles = 1500000;
		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;
		cfg.noiseStrength = 1.0f; 
		cfg.noiseFrequency = 1.0f;
		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
		m_tornadoEmitters.push_back(em);
	}

	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "TornadoCoreFine";
		cfg.mainStage.texPath = "Data/Images/Scene4/dust1.png";
		cfg.blendMode = BlendMode::ALPHA;
		cfg.position = pos;
		cfg.spawnArea = Vec3(1.4f, 1.4f, 5.5f);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.04f);
		cfg.mainStage.velocityVariance = Vec3(0.35f, 0.35f, 0.50f); 
		cfg.mainStage.lifetime = 5.5f;
		cfg.mainStage.lifetimeVariance = 0.0f;
		cfg.mainStage.startColor = Rgba8(50, 30, 20, 255);
		cfg.mainStage.endColor = Rgba8(180, 160, 130, 30);
		cfg.mainStage.startSize = 0.02f;
		cfg.mainStage.endSize = 1.2f;
		cfg.spawnRate = 100000.0f; 
		cfg.maxParticles = 1500000;
		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;
		cfg.noiseStrength = 1.0f; 
		cfg.noiseFrequency = 1.0f;
		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
		m_tornadoEmitters.push_back(em);
	}

	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "TornadoCoreFine";
		cfg.mainStage.texPath = "Data/Images/Scene4/smoke.png";
		cfg.blendMode = BlendMode::ALPHA_ADDITIVE;
		cfg.position = pos;
		cfg.spawnArea = Vec3(10.f, 10.f, 5.5f);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.04f);
		cfg.mainStage.velocityVariance = Vec3(0.35f, 0.35f, 0.50f);
		cfg.mainStage.lifetime = 5.5f;
		cfg.mainStage.lifetimeVariance = 0.0f;
		cfg.mainStage.startColor = Rgba8(10, 10, 10, 1);
		cfg.mainStage.endColor = Rgba8(180, 160, 130, 1);
		cfg.mainStage.startSize = 0.02f;
		cfg.mainStage.endSize = 1.2f;
		cfg.spawnRate = 100000.0f;
		cfg.maxParticles = 1500000;
		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;
		cfg.noiseStrength = 1.0f;
		cfg.noiseFrequency = 1.0f;
		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
		m_tornadoEmitters.push_back(em);
	}

	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "GroundSandLift";
		cfg.mainStage.texPath = "Data/Images/Scene4/pebble1.png";
		cfg.blendMode = BlendMode::ALPHA;
		cfg.position = pos;
		cfg.spawnArea = Vec3(6.0f, 6.0f, 1.0f); 
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.6f); 
		cfg.mainStage.velocityVariance = Vec3(1.2f, 1.2f, 1.8f); 
		cfg.mainStage.lifetime = 3.2f;
		cfg.mainStage.lifetimeVariance = 0.0f;
		cfg.mainStage.startColor = Rgba8(200, 180, 140, 100); 
		cfg.mainStage.endColor = Rgba8(200, 180, 140, 100);
		cfg.mainStage.startSize = 0.18f;
		cfg.mainStage.endSize = 0.3f;
		cfg.spawnRate = 1200.0f; 
		cfg.maxParticles = 80000;
		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;
		cfg.noiseStrength = 1.1f;
		cfg.noiseFrequency = 1.3f;
		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
	}

	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "MidDustClumps";
		cfg.mainStage.texPath = "Data/Images/Scene4/pebble2.png"; 
		cfg.blendMode = BlendMode::ALPHA;
		cfg.position = pos;
		cfg.spawnArea = Vec3(3.0f, 3.0f, 10.0f);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.03f);
		cfg.mainStage.velocityVariance = Vec3(0.25f, 0.25f, 0.35f);
		cfg.mainStage.lifetime = 7.5f;
		cfg.mainStage.lifetimeVariance = 0.0f;
		cfg.mainStage.startColor = Rgba8(190, 170, 130, 30);
		cfg.mainStage.endColor = Rgba8(170, 150, 110, 30);
		cfg.mainStage.startSize = 0.2f;
		cfg.mainStage.endSize = 0.3f;
		cfg.spawnRate = 500.0f;
		cfg.maxParticles = 25000;
		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;
		cfg.noiseStrength = 0.85f;
		cfg.noiseFrequency = 0.55f;
		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
		m_tornadoEmitters.push_back(em);
	}



	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "TopMistDiffusion";
		cfg.mainStage.texPath = "Data/Images/Scene4/dust2.png"; 
		cfg.blendMode = BlendMode::ALPHA;
		cfg.position = pos;
		cfg.spawnArea = Vec3(10.0f, 10.0f, 10.0f);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 5.0f);
		cfg.mainStage.velocityVariance = Vec3(0.4f, 0.4f, 0.6f);
		cfg.mainStage.lifetime = 6.0f;
		cfg.mainStage.lifetimeVariance = 0.0f;
		cfg.mainStage.startColor = Rgba8(200, 180, 150, 10);
		cfg.mainStage.endColor = Rgba8(200, 180, 150, 0);
		cfg.mainStage.startSize = 2.0f;
		cfg.mainStage.endSize = 6.0f;
		cfg.spawnRate = 600.0f;
		cfg.maxParticles = 30000;
		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;
		cfg.noiseStrength = 0.6f;
		cfg.noiseFrequency = 0.7f;
		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
		m_tornadoEmitters.push_back(em);
	}

	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "GroundDustRise";
		cfg.mainStage.texPath = "Data/Images/Scene4/dust3.png"; 
		cfg.blendMode = BlendMode::ALPHA;
		cfg.position = Vec3(0.f, 0.f, 0.15f);
		cfg.spawnArea = Vec3(100.f, 100.f, 0.6f);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.f);
		cfg.mainStage.velocityVariance = Vec3(0.5f, 0.5f, 0.2f);
		cfg.mainStage.lifetime = 2.8f;
		cfg.mainStage.lifetimeVariance = 0.0f;
		cfg.mainStage.startColor = Rgba8(210, 190, 150, 120);
		cfg.mainStage.endColor = Rgba8(210, 190, 150, 30);
		cfg.mainStage.startSize = 0.12f;
		cfg.mainStage.endSize = 0.2f;
		cfg.spawnRate = 22000.0f;
		cfg.maxParticles = 100000;
		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;
		cfg.noiseStrength = 1.2f;
		cfg.noiseFrequency = 1.5f;
		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
	}

	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "GroundDustRise";
		cfg.mainStage.texPath = "Data/Images/Scene4/pebble1.png";  
		cfg.blendMode = BlendMode::ALPHA;
		cfg.position = Vec3(0.f, 0.f, 0.15f);  
		cfg.spawnArea = Vec3(100.f, 100.f, 0.6f);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.f);  
		cfg.mainStage.velocityVariance = Vec3(0.5f, 0.5f, 0.2f); 
		cfg.mainStage.lifetime = 2.8f;
		cfg.mainStage.lifetimeVariance = 0.0f;
		cfg.mainStage.startColor = Rgba8(255, 220, 190, 255); 
		cfg.mainStage.endColor = Rgba8(255, 220, 190, 30);
		cfg.mainStage.startSize = 0.12f;
		cfg.mainStage.endSize = 0.2f;
		cfg.spawnRate = 12000.0f;
		cfg.maxParticles = 100000;
		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;
		cfg.noiseStrength = 1.2f; 
		cfg.noiseFrequency = 1.5f;
		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
	}


	{
		ParticleForce coreVortex = ParticleForce::MakeFlowColumn(
			Vec3(0, 0, 0),
			Vec3(0, 0, 1),
			48.0f,
			1.5f,
			12.0f,
			40.0f,  
			6.0f,   
			0.0f,   
			FLOW_SWIRL_ENABLE | FLOW_AXIAL_ENABLE
		);
		uint32_t idx = g_theParticleSystem->AddForce(coreVortex);
		m_tornadoForceIndices.push_back(idx);
	}

	{
		ParticleForce inflow = ParticleForce::MakeFlowColumn(
			Vec3(0, 0, 0),
			Vec3(0, 0, 1),
			48.0f,
			4.0f,
			18.0f,
			0.0f,   
			0.0f,   
			14.0f,  
			FLOW_RADIAL_ENABLE
		);
		uint32_t idx = g_theParticleSystem->AddForce(inflow);
		m_tornadoForceIndices.push_back(idx);
	}

	{
		ParticleForce groundPull = ParticleForce::MakeFlowColumn(
			Vec3(0, 0, 0),
			Vec3(0, 0, 1),
			10.0f,
			1.5f,
			4.0f,
			20.0f,
			20.0f,
			10.0f,
			FLOW_SWIRL_ENABLE | FLOW_AXIAL_ENABLE | FLOW_RADIAL_ENABLE
		);
		uint32_t idx = g_theParticleSystem->AddForce(groundPull);
		m_tornadoForceIndices.push_back(idx);
	}

}

void Game::SetupScene5()
{
	Scene* scene = GetCurrentScene();
	if (!scene) return;

	m_skyTex = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene5/Sky.png");

	m_skyVerts.clear();
	AddVertsForSkySphere3D(m_skyVerts, Rgba8(255, 255, 255, 255));

	scene->ClearGameObjects();
	scene->ClearParticleEmitters();

	scene->SetupStartPosAndOrientation(Vec3(1.45f, -26.f, 5.f), EulerAngles(-270.f, -23.f, 0.f));
	scene->ResetPlayer(m_player);


	GameObject* island = scene->CreateGameObject(this, "Island");
	island->InitializeVertsFromFile("Data/Meshes/island");
	island->SetPosition(Vec3(0.f, 0.f, 0.f));
	island->SetScale(Vec3(50.f, 50.f, 50.f));
	island->SetOrientation(EulerAngles(0.f, 0.f, 90.f));

	island->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene5/island.png");
	island->m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene5/island_n.png");
	island->m_specGlossEmitTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene5/island_sge.png");

	island->SetColor(Rgba8::WHITE);


	GameObject* sakura1 = scene->CreateGameObject(this, "Sakura1");
	sakura1->InitializeVertsFromFile("Data/Meshes/sakura1");
	sakura1->SetPosition(Vec3(-13.4f, 32.2f, 32.2f));
	sakura1->SetScale(Vec3(50.f, 50.f, 50.f));
	sakura1->SetOrientation(EulerAngles(0.f, 0.f, 90.f));

	sakura1->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene5/sakura1.png");
	sakura1->m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene5/sakura1_n.png");
	sakura1->m_specGlossEmitTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene5/sakura1_sge.png");

	sakura1->SetColor(Rgba8::WHITE);


	scene->ClearParticleEmitters();
	g_theParticleSystem->ClearForces();

	Vec3 fxCenter = Vec3(0.f, 0.f, 0.f);
	Vec3 camPos = m_player ? m_player->GetPosition() : fxCenter;
	fxCenter = camPos;

	Vec3 treePos = sakura1->GetPosition();

	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "Petals_CanopyDrop";
		cfg.mainStage.texPath = "Data/Images/Scene5/petal_soft.png";
		cfg.blendMode = BlendMode::ALPHA;

		cfg.position = treePos + Vec3(0.f, 0.f, 12.0f);
		cfg.spawnArea = Vec3(52.f, 52.f, 14.f);

		cfg.mainStage.baseVelocity = Vec3(0.20f, 0.05f, -0.70f);
		cfg.mainStage.velocityVariance = Vec3(0.70f, 0.70f, 0.35f);

		cfg.mainStage.lifetime = 7.0f;
		cfg.mainStage.lifetimeVariance = 3.0f;

		cfg.mainStage.startColor = Rgba8(255, 210, 235, 170);
		cfg.mainStage.endColor = Rgba8(255, 210, 235, 25);

		cfg.mainStage.startSize = 0.90f;
		cfg.mainStage.endSize = 1.70f;

		cfg.spawnRate = 2600.0f;
		cfg.maxParticles = 140000;

		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;

		cfg.noiseStrength = 1.25f;
		cfg.noiseFrequency = 0.75f;

		scene->AddParticleEmitter(g_theParticleSystem->CreateEmitter(cfg));
	}
	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "Petals_TrunkSwirl";
		cfg.mainStage.texPath = "Data/Images/Scene5/petal_soft.png";
		cfg.blendMode = BlendMode::ALPHA;

		cfg.position = treePos + Vec3(0.f, 0.f, 5.0f);
		cfg.spawnArea = Vec3(58.f, 58.f, 18.f);

		cfg.mainStage.baseVelocity = Vec3(0.10f, 0.03f, -0.35f);
		cfg.mainStage.velocityVariance = Vec3(0.55f, 0.55f, 0.25f);

		cfg.mainStage.lifetime = 6.5f;
		cfg.mainStage.lifetimeVariance = 2.5f;

		cfg.mainStage.startColor = Rgba8(255, 205, 235, 140);
		cfg.mainStage.endColor = Rgba8(255, 205, 235, 10);

		cfg.mainStage.startSize = 0.70f;
		cfg.mainStage.endSize = 1.40f;

		cfg.spawnRate = 1600.0f;
		cfg.maxParticles = 90000;

		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;

		cfg.noiseStrength = 1.0f;
		cfg.noiseFrequency = 0.9f;

		scene->AddParticleEmitter(g_theParticleSystem->CreateEmitter(cfg));
	}
	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "Petals_ClosePassTree";
		cfg.mainStage.texPath = "Data/Images/Scene5/petal_streak.png";
		cfg.blendMode = BlendMode::ALPHA;

		cfg.position = treePos + Vec3(0.f, 0.f, 2.0f);
		cfg.spawnArea = Vec3(50.5f, 50.5f, 3.0f);

		cfg.mainStage.baseVelocity = Vec3(1.3f, 0.4f, -0.15f);
		cfg.mainStage.velocityVariance = Vec3(2.0f, 2.0f, 0.25f);

		cfg.mainStage.lifetime = 1.4f;
		cfg.mainStage.lifetimeVariance = 0.6f;

		cfg.mainStage.startColor = Rgba8(255, 220, 240, 210);
		cfg.mainStage.endColor = Rgba8(255, 220, 240, 0);

		cfg.mainStage.startSize = 1.50f;
		cfg.mainStage.endSize = 2.40f;

		cfg.spawnRate = 80.0f;
		cfg.maxParticles = 3000;

		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;

		cfg.noiseStrength = 0.8f;
		cfg.noiseFrequency = 1.2f;

		scene->AddParticleEmitter(g_theParticleSystem->CreateEmitter(cfg));
	}
	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "Petals_GroundDrift";
		cfg.mainStage.texPath = "Data/Images/Scene5/petal_soft.png";
		cfg.blendMode = BlendMode::ALPHA;

		cfg.position = treePos + Vec3(0.f, 0.f, 0.8f);
		cfg.spawnArea = Vec3(60.f, 60.f, 2.0f);

		cfg.mainStage.baseVelocity = Vec3(0.25f, 0.08f, 0.02f);
		cfg.mainStage.velocityVariance = Vec3(0.35f, 0.35f, 0.06f);

		cfg.mainStage.lifetime = 6.0f;
		cfg.mainStage.lifetimeVariance = 2.5f;

		cfg.mainStage.startColor = Rgba8(255, 210, 235, 90);
		cfg.mainStage.endColor = Rgba8(255, 210, 235, 0);

		cfg.mainStage.startSize = 0.60f;
		cfg.mainStage.endSize = 1.10f;

		cfg.spawnRate = 900.0f;
		cfg.maxParticles = 50000;

		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;

		cfg.noiseStrength = 0.6f;
		cfg.noiseFrequency = 0.7f;

		scene->AddParticleEmitter(g_theParticleSystem->CreateEmitter(cfg));
	}
	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "Pollen_Air";
		cfg.mainStage.texPath = "Data/Images/Scene5/pollen.png";
		cfg.blendMode = BlendMode::ALPHA;

		cfg.position = treePos + Vec3(0.f, 0.f, 8.0f);
		cfg.spawnArea = Vec3(60.f, 60.f, 30.f);

		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.f);
		cfg.mainStage.velocityVariance = Vec3(0.02f, 0.02f, 0.02f);

		cfg.mainStage.lifetime = 18.0f;
		cfg.mainStage.lifetimeVariance = 6.0f;

		cfg.mainStage.startColor = Rgba8(255, 245, 235, 25);
		cfg.mainStage.endColor = Rgba8(255, 245, 235, 0);

		cfg.mainStage.startSize = 0.14f;
		cfg.mainStage.endSize = 0.20f;

		cfg.spawnRate = 1200.0f;
		cfg.maxParticles = 40000;

		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;

		cfg.noiseStrength = 0.2f;
		cfg.noiseFrequency = 0.3f;

		scene->AddParticleEmitter(g_theParticleSystem->CreateEmitter(cfg));
	}

	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "Petals_UpperDrift";
		cfg.mainStage.texPath = "Data/Images/Scene5/petal_soft.png";
		cfg.blendMode = BlendMode::ALPHA;

		cfg.position = Vec3(8.f, 5.f, 35.f);
		cfg.spawnArea = Vec3(30.f, 30.f, 10.f);

		cfg.mainStage.baseVelocity = Vec3(0.6f, 0.2f, -0.15f);
		cfg.mainStage.velocityVariance = Vec3(0.4f, 0.4f, 0.2f);

		cfg.mainStage.lifetime = 10.0f;
		cfg.mainStage.lifetimeVariance = 4.0f;

		cfg.mainStage.startColor = Rgba8(255, 210, 235, 90);
		cfg.mainStage.endColor = Rgba8(255, 210, 235, 0);

		cfg.mainStage.startSize = 0.9f;
		cfg.mainStage.endSize = 1.6f;

		cfg.spawnRate = 1200.0f;
		cfg.maxParticles = 60000;

		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;

		cfg.noiseStrength = 0.6f;
		cfg.noiseFrequency = 0.5f;

		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
	}

	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "Petals_CrownEdge";
		cfg.mainStage.texPath = "Data/Images/Scene5/petal_soft.png";
		cfg.blendMode = BlendMode::ALPHA;

		cfg.position = Vec3(-25.f, 25.5f, 30.f);
		cfg.spawnArea = Vec3(20.f, 20.f, 8.f);

		cfg.mainStage.baseVelocity = Vec3(0.4f, 0.1f, -0.25f);
		cfg.mainStage.velocityVariance = Vec3(0.8f, 0.8f, 0.3f);

		cfg.mainStage.lifetime = 6.0f;
		cfg.mainStage.lifetimeVariance = 2.5f;

		cfg.mainStage.startColor = Rgba8(255, 215, 240, 110);
		cfg.mainStage.endColor = Rgba8(255, 215, 240, 0);

		cfg.mainStage.startSize = 0.7f;
		cfg.mainStage.endSize = 1.3f;

		cfg.spawnRate = 1200.0f;
		cfg.maxParticles = 70000;

		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;

		cfg.noiseStrength = 1.4f;
		cfg.noiseFrequency = 1.0f;

		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
	}

	{
		Vec3 windDir = Vec3(1.0f, 0.35f, -0.05f).GetNormalized();

		ParticleForce wind = ParticleForce::MakeParticleDirectionForce(
			windDir,
			2.2f,
			120.f,
			treePos + Vec3(0.f, 0.f, 8.f)
		);

		g_theParticleSystem->AddForce(wind);
	}

	{
		Vec3 gravityDir = Vec3(0.f, 0.f, -1.f);

		ParticleForce gravity = ParticleForce::MakeParticleGravity(gravityDir, 1.0f);

		g_theParticleSystem->AddForce(gravity);
	}
}

void Game::SetupScene6()
{
	Scene* scene = GetCurrentScene();
	if (!scene) return;
	m_skyVerts.clear();

	scene->ClearGameObjects();
	scene->ClearParticleEmitters();

	scene->SetupStartPosAndOrientation(Vec3(0.f, 0.f, 0.f), EulerAngles(0.f, 0.f, 0.f));
	scene->ResetPlayer(m_player);

	// === Ground Plane ===
	{
		GameObject* ground = scene->CreateGameObject(this, "Ground");
		ground->InitializeVertsFromType(ObjectType::CUBE);
		ground->SetPosition(Vec3(0.f, 0.f, -0.5f));
		ground->SetScale(Vec3(10.f, 10.f, 0.1f));
		ground->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene3/brick1_sge.png");
	}

	{
		ParticleEmitterConfig fireConfig;
		fireConfig.name = "Smoke";

		fireConfig.mainStage.texPath = "Data/Images/Scene4/dust1.png";
		fireConfig.blendMode = BlendMode::ALPHA;
		fireConfig.position = Vec3(0.f, 0.f, 0.f);
		fireConfig.spawnArea = Vec3(1.f, 1.f, 1.f);
		fireConfig.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.1f);

		fireConfig.spawnRate = 10000.f;
		fireConfig.mainStage.lifetime = 1.f;
		fireConfig.mainStage.lifetimeVariance = 0.f;

		fireConfig.mainStage.startColor = Rgba8(200, 200, 200, 200);
		fireConfig.mainStage.endColor = Rgba8(200, 200, 200, 200);

		fireConfig.mainStage.startSize = 0.05f;
		fireConfig.mainStage.endSize = 0.05f;

		fireConfig.isLooping = true;
		fireConfig.enabled = true;
		fireConfig.maxParticles = 10000000;

		ParticleEmitter* fireWithSmoke = g_theParticleSystem->CreateEmitter(fireConfig);
		scene->AddParticleEmitter(fireWithSmoke);
	}
}

void Game::SetupScene2()
{
	Scene* scene = GetCurrentScene();
	if (!scene) return;
	m_skyTex = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene2/Sky.png");
	m_skyVerts.clear();
	AddVertsForSkySphere3D(m_skyVerts, Rgba8(200, 200, 200, 255));
	scene->ClearGameObjects();
	scene->ClearParticleEmitters();

	scene->SetupStartPosAndOrientation(Vec3(4.6f, 7.4f, 6.3f), EulerAngles(-135.f, 24.f, 0.f));
	scene->ResetPlayer(m_player);

	GameObject* cave = scene->CreateGameObject(this, "Cave");
	cave->InitializeVertsFromFile("Data/Meshes/cave");
	cave->SetPosition(Vec3(-0.2f, -6.5f, 4.75f));
	cave->SetScale(Vec3(20.f, 20.f, 20.f));
	cave->SetOrientation(EulerAngles(0.f, 0.f, 90.f));

	cave->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene2/cave.png");
	cave->m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene2/cave_n.png");
	cave->m_specGlossEmitTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene2/cave_sge.png");

	cave->SetColor(Rgba8::WHITE);

	GameObject* platform = scene->CreateGameObject(this, "Platform");
	platform->InitializeVertsFromFile("Data/Meshes/platform");
	platform->SetPosition(Vec3(0.f, 0.f, 0.f));
	platform->SetScale(Vec3(5.0f, 5.0f, 5.0f));
	platform->SetOrientation(EulerAngles(0.f, 0.f, 90.f));

	platform->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene2/platform.png");
	platform->m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene2/platform_n.png");
	platform->m_specGlossEmitTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene2/platform_sge.png");
	platform->SetColor(Rgba8::WHITE);


	Lights& L = scene->GetLights();
	L.m_sunColor = Vec4(0.9f, 0.95f, 1.0f, 0.1f);
	L.m_sunDirection = Vec3(0.3f, -0.6f, -0.7f).GetNormalized();
	L.m_numLights = 0;

	Light& light = L.m_lightsArray[L.m_numLights++];

	light.m_color = Vec4(
		170.f / 255.f,
		120.f / 255.f,
		253.f / 255.f,
		1.5f);

	light.m_position = Vec3(0.f, 0.f, 5.4f);

	light.m_direction = Vec3(0.f, 0.f, 0.f);

	light.m_ambience = 0.1f;
	light.m_innerRadius = 3.0f;
	light.m_outerRadius = 25.0f;

	light.m_innerDotThreshold = -1.0f;
	light.m_outerDotThreshold = -2.0f;


	Vec3 circleBaseCenter = Vec3(0.f, 0.f, 0.51f);
	RandomNumberGenerator rng;


	struct MagicCircleParticleLayer
	{
		const char* texPath;
		float       radius;
		float       height;
		Rgba8       color;
		float       spinSpeedDegPerSec;
		float       spawnRate;
		float       startSize;
		float       endSize;
		unsigned int maxParticles;
	};

	MagicCircleParticleLayer circleLayers[] = {
		{ "Data/Images/Scene2/m1.png", 2.50f, 0.00f, Rgba8(255,255,255,250),  1.0f,  1.0f, 5.8f, 0.9f, 1 },
		{ "Data/Images/Scene2/m2.png", 3.075f,1.00f, Rgba8(200,170,255,200), -1.2f,  1.0f, 7.7f, 0.85f, 1 },
		{ "Data/Images/Scene2/m3.png", 1.875f,2.00f, Rgba8(170,200,255,220),  1.5f,  1.0f, 4.6f, 0.8f, 1 },
		{ "Data/Images/Scene2/m4.png", 2.175f,3.00f, Rgba8(200,120,255,180), -0.4f,  1.0f, 6.65f, 0.9f, 1 },
	};

	int layerCount = (int)(sizeof(circleLayers) / sizeof(circleLayers[0]));

	for (int i = 0; i < layerCount; ++i)
	{
		const auto& layer = circleLayers[i];

		ParticleEmitterConfig cfg{};
		cfg.m_owner = g_theParticleSystem;
		cfg.name = Stringf("MagicCircleLayer_Particles_%d", i);
		cfg.blendMode = BlendMode::ALPHA_ADDITIVE;
		cfg.mainStage.texPath = layer.texPath;

		cfg.position = circleBaseCenter + Vec3(0, 0, layer.height);
		cfg.spawnArea = Vec3(0.f, 0.f, 0.f);

		cfg.mainStage.billboardType = 1u;
		cfg.mainStage.startSize = layer.startSize;
		cfg.mainStage.endSize = layer.endSize;
		cfg.mainStage.startColor = layer.color;
		cfg.mainStage.endColor = Rgba8(layer.color.r, layer.color.g, layer.color.b, layer.color.a - 20);

		cfg.mainStage.lifetime = 999999999.f;  
		cfg.mainStage.lifetimeVariance = 0.0f;

		cfg.mainStage.baseAngularVelocity = layer.spinSpeedDegPerSec;
		cfg.mainStage.angularVariance = 0.f;

		cfg.spawnRate = layer.spawnRate;
		cfg.maxParticles = layer.maxParticles;

		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;
		cfg.useSubStage = false;

		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
	}

	Vec3 orbCenter = circleBaseCenter + Vec3(0.f, 0.f, 0.75f);
	const char* ringTex = "Data/Images/Scene2/mRing.png";
	Rgba8 orbColor = Rgba8(200, 170, 255, 255);
	float baseScale = 0.6f;
	int orbRingCount = 6;
	EulerAngles ringAngles[] =
	{
		EulerAngles(0.f, 0.f, 0.f),
		EulerAngles(90.f, 0.f, 0.f),
		EulerAngles(0.f, 0.f, 90.f),
		EulerAngles(45.f, 0.f, 45.f),
		EulerAngles(-45.f, 0.f, 45.f),
		EulerAngles(0.f, 45.f, 45.f),
	};
	for (int r = 0; r < orbRingCount; ++r)
	{
		GameObject* ring = scene->CreateGameObject(this, Stringf("MagicOrbRing_%d", r));
		ring->InitializeVertsFromType(ObjectType::TORUS);
		ring->SetPosition(orbCenter);
		ring->SetScale(Vec3(baseScale, baseScale, baseScale));
		ring->SetOrientation(ringAngles[r]);
		ring->SetColor(orbColor);
		ring->SetAngularVelocity(EulerAngles(0.f, 18.f + r * 3.f, 0.f));
		ring->m_diffuseTexture =
			g_theRenderer->CreateOrGetTextureFromFile(ringTex);
		ring->m_normalTexture = nullptr;
		ring->m_specGlossEmitTexture = nullptr;
	}
	{
		int emitterCount = 32;
		float ringRadius = 1.8f;
		Vec3 ringCenter = Vec3(0.f, 0.f, 0.52f);
		const char* sparkTexPaths[3] = {
			"Data/Images/Scene2/mspark1.png",
			"Data/Images/Scene2/mspark2.png",
			"Data/Images/Scene2/mspark3.png"
		};
		for (int e = 0; e < emitterCount; ++e)
		{
			ParticleEmitterConfig cfg;
			cfg.m_owner = g_theParticleSystem;
			cfg.name = Stringf("MagicRingEnergy_%d", e);
			int texIdx = rng.RollRandomIntInRange(0, 2);
			cfg.mainStage.texPath = sparkTexPaths[texIdx];
			cfg.blendMode = BlendMode::ADDITIVE;
			float angleDeg = (360.f / (float)emitterCount) * (float)e;
			Vec2 off = Vec2::MakeFromPolarDegrees(angleDeg, ringRadius);
			cfg.position = ringCenter + Vec3(off.x, off.y, 0.f);
			cfg.spawnArea = Vec3(0.5f, 0.5f, 0.2f);
			cfg.mainStage.lifetime = 5.0f;
			cfg.mainStage.lifetimeVariance = 0.25f;
			unsigned char r = (unsigned char)rng.RollRandomIntInRange(170, 230);
			unsigned char g = (unsigned char)rng.RollRandomIntInRange(80, 160);
			unsigned char b = (unsigned char)rng.RollRandomIntInRange(200, 255);
			unsigned char a = (unsigned char)rng.RollRandomIntInRange(200, 255);
			cfg.mainStage.startColor = Rgba8(r, g, b, a);
			cfg.mainStage.endColor = Rgba8((unsigned char)(r / 2), (unsigned char)(g / 2), (unsigned char)(b / 2), 0);
			cfg.mainStage.startSize = 0.06f;
			cfg.mainStage.endSize = 0.02f;
			cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.2f);
			cfg.mainStage.velocityVariance = Vec3(0.05f, 0.05f, 0.05f);
			cfg.mainStage.startEmissive = 1.f;
			cfg.mainStage.endEmissive = 1.f;
			cfg.mainStage.billboardType = 0u;
			cfg.spawnRate = 200.0f;
			cfg.maxParticles = 30000;
			cfg.isLooping = true;
			cfg.enabled = true;
			cfg.duration = -1.0f;
			cfg.useSubStage = false;
			ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
			scene->AddParticleEmitter(em);
		}
	}
	{
		float orbRadius = 0.35f;
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "Bursts";
		cfg.blendMode = BlendMode::ADDITIVE;
		cfg.mainStage.texPath = "Data/Images/Scene2/mspark1.png";
		cfg.position = orbCenter;
		cfg.spawnArea = Vec3(orbRadius, orbRadius, orbRadius);
		cfg.mainStage.lifetime = 0.12f;
		cfg.mainStage.lifetimeVariance = 0.08f;
		cfg.mainStage.startSize = 0.3f;
		cfg.mainStage.endSize = 0.00f;
		cfg.mainStage.startEmissive = 1.f;
		cfg.mainStage.endEmissive = 1.f;
		unsigned char r = (unsigned char)rng.RollRandomIntInRange(200, 255);
		unsigned char g = (unsigned char)rng.RollRandomIntInRange(140, 210);
		unsigned char b = (unsigned char)rng.RollRandomIntInRange(220, 255);
		unsigned char a = (unsigned char)rng.RollRandomIntInRange(180, 255);
		cfg.mainStage.startColor = Rgba8(r, g, b, a);
		cfg.mainStage.endColor = Rgba8((unsigned char)(r / 2), (unsigned char)(g / 2), (unsigned char)(b / 2), 0);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.05f);
		cfg.mainStage.velocityVariance = Vec3(0.25f, 0.25f, 0.25f);
		cfg.mainStage.billboardType = 0u;
		cfg.spawnRate = 500.0f;
		cfg.maxParticles = 1000;
		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;
		cfg.useSubStage = false;
		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
	}
	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "SingleRisingRing";
		cfg.blendMode = BlendMode::ALPHA;
		cfg.mainStage.texPath = "Data/Images/Scene2/m5.png";
		Vec3 startPos = circleBaseCenter + Vec3(0.f, 0.f, 0.15f);
		cfg.position = startPos;
		cfg.spawnArea = Vec3(0.02f, 0.02f, 0.02f);
		float riseSpeed = 0.25f;
		float lifeTime = 10.0f;
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, riseSpeed);
		cfg.mainStage.velocityVariance = Vec3(0.0f, 0.0f, 0.0f);
		cfg.mainStage.baseAngularVelocity = 1.f;
		cfg.mainStage.startSize = 3.0f;
		cfg.mainStage.endSize = 10.0f;
		cfg.mainStage.startColor = Rgba8(200, 170, 255, 100);
		cfg.mainStage.endColor = Rgba8(200, 170, 255, 50);
		cfg.mainStage.lifetime = lifeTime;
		cfg.mainStage.lifetimeVariance = 0.0f;
		cfg.mainStage.startEmissive = 1.f;
		cfg.mainStage.endEmissive = 1.f;
		cfg.mainStage.billboardType = 1u;
		cfg.maxParticles = 1;
		cfg.spawnRate = 1.0f / lifeTime;
		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;
		cfg.useSubStage = false;
		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
	}
	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "MagicSmokeMist";
		cfg.blendMode = BlendMode::ALPHA;
		cfg.mainStage.texPath =
			"Data/Images/Scene2/msmoke.png";
		cfg.position = circleBaseCenter - Vec3(0.f, 0.f, 0.1f);
		cfg.spawnArea = Vec3(6.8f, 6.8f, 0.6f);
		cfg.mainStage.lifetime = 4.0f;
		cfg.mainStage.lifetimeVariance = 1.5f;
		cfg.mainStage.startSize = 0.55f;
		cfg.mainStage.endSize = 1.35f;
		cfg.mainStage.startColor = Rgba8(180, 150, 255, 20);
		cfg.mainStage.endColor = Rgba8(180, 150, 255, 0);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.05f);
		cfg.mainStage.velocityVariance = Vec3(0.04f, 0.04f, 0.04f);
		cfg.mainStage.billboardType = 0u;
		cfg.spawnRate = 60.0f;
		cfg.maxParticles = 480;
		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;
		cfg.useSubStage = false;
		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
	}
}

void Game::RegisterScenes()
{
	m_sceneManager.RegisterScene("Scene 1", std::make_unique<Scene>("Scene 1"));
	m_sceneManager.RegisterScene("Scene 2", std::make_unique<Scene>("Scene 2"));
	m_sceneManager.RegisterScene("Scene 3", std::make_unique<Scene>("Scene 3"));
	m_sceneManager.RegisterScene("Scene 4", std::make_unique<Scene>("Scene 4"));
	m_sceneManager.RegisterScene("Scene 5", std::make_unique<Scene>("Scene 5"));
	m_sceneManager.RegisterScene("Scene 6", std::make_unique<Scene>("Scene 6"));
}

void Game::SwitchToScene(const std::string& sceneName, float fadeDuration /*= 1.0f*/)
{
	m_sceneManager.SwitchToScene(sceneName, fadeDuration);
	SetupSceneContent();
	m_uiSelectedEmitter = nullptr;
	m_uiEmitterIndex = 0;
	m_uiPendingMaxParticles = 0;
	m_uiEmitterPaused = false;
}
