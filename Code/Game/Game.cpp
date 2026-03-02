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
	m_screenCamera.SetOrthographicView(Vec2(0.f, 0.f), Vec2(1600.f, 800.f));

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

	if (g_theInput->WasKeyJustPressed(KEYCODE_1))
	{
		Vec3 lineStart = m_player->GetPosition();
		Vec3 lineEnd = lineStart + m_player->GetFwdNormal() * 20.f;
		DebugAddWorldLine(lineStart, lineEnd, 0.05f, 10.f, Rgba8::YELLOW, Rgba8::YELLOW, DebugRenderMode::X_RAY);
	}

	if (g_theInput->IsKeyDown(KEYCODE_2))
	{
		Vec3 playerPos = m_player->GetPosition();
		Vec3 spawnPos = Vec3(playerPos.x, playerPos.y, 0.f);

		Rgba8 sphereColor = Rgba8(150, 75, 0, 255);
		DebugAddWorldPoint(spawnPos, 0.5f, 60.f, sphereColor, sphereColor, DebugRenderMode::USE_DEPTH);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_3))
	{
		Vec3 playerPos = m_player->GetPosition();
		Vec3 spherePos = Vec3(playerPos.x + 2.f, playerPos.y, playerPos.z);

		

		DebugAddWorldWireSphere(playerPos, 1.f, 5.f, Rgba8::GREEN, Rgba8::RED, DebugRenderMode::USE_DEPTH);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_4))
	{
		Vec3 playerPos = m_player->GetPosition();
		EulerAngles playerOrientation = m_player->GetOrientation();

		Mat44 playerTransform = playerOrientation.GetAsMatrix_IFwd_JLeft_KUp();
		playerTransform.SetTranslation3D(playerPos); 

		DebugAddWorldBasis(playerTransform, -1.f, DebugRenderMode::USE_DEPTH);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_5))
	{
		Vec3 playerPos = m_player->GetPosition();
		EulerAngles playerOrientation = m_player->GetOrientation();

		Mat44 playerTransform = playerOrientation.GetAsMatrix_IFwd_JLeft_KUp();
		playerTransform.SetTranslation3D(playerPos);

		Mat44 cameraMatrix = m_player->GetCamera().GetCameraToWorldTransform();

		Vec3 forwardOffset = playerTransform.GetIBasis3D() * 1.5f;
		Vec3 billboardPos = playerPos + forwardOffset;



		std::string text = Stringf("Position: %.1f, %.1f, %.1f Orientation: %.1f, %.1f, %.1f",
			playerPos.x, playerPos.y, playerPos.z,
			playerOrientation.m_yawDegrees, playerOrientation.m_pitchDegrees, playerOrientation.m_rollDegrees);

		DebugAddWorldBillboardText(text, billboardPos, 0.125f, Vec2(0.5f, 0.5f), 10.f, Rgba8::WHITE, Rgba8::RED, DebugRenderMode::USE_DEPTH, BillboardType::FULL_OPPOSING);
	}


	if (g_theInput->WasKeyJustPressed(KEYCODE_6))
	{
		Vec3 playerPos = m_player->GetPosition();
		Vec3 playerUp = Vec3(0.0f, 0.0f, 1.0f);
		Vec3 playerDown = -playerUp;

		Vec3 cylinderTop = playerPos + (playerUp * 0.5f);
		Vec3 cylinderBot = playerPos + (playerDown * 0.5f);
		
		DebugAddWorldWireCylinder(cylinderBot, cylinderTop, 0.5f, 10.f, Rgba8::WHITE, Rgba8::RED, DebugRenderMode::USE_DEPTH);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_7))
	{
		std::string text;
		EulerAngles orientation = m_player->GetOrientation();
		text = Stringf("Camera Orientation: %.2f, %.2f, %.2f", orientation.m_yawDegrees, orientation.m_pitchDegrees, orientation.m_rollDegrees);

		DebugAddMessage(text, 5.f, Rgba8::WHITE, Rgba8::WHITE);
	}

	if (g_theInput->IsKeyDown(KEYCODE_RIGHT_MOUSE))
	{
		m_imguiCursor = false;
	}
	else
	{
		m_imguiCursor = true;
	}

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


		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_theRenderer->BeginOpaquePass();
		Lights lights = GetCurrentScene()->GetLights();

		for (int i = 0; i < lights.m_numLights; ++i)
		{
			RenderLightSource(lights.m_lightsArray[i]);
		}
		g_theRenderer->BindShader(m_shader);
		g_theRenderer->SetLightConstants(lights);
		m_sceneManager.Render();


		if (g_theParticleSystem) g_theParticleSystem->Render();

		g_theRenderer->EndOpaquePass();

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
	std::string timeText = Stringf("Time: %.2f FPS: %.0f ", time, FPS);

	AABB2 timeBox(1000.f, 750.f, 1600.f, 800.f);
	DebugAddScreenText(timeText, timeBox, 20.f, Vec2(1.f, 0.5f), -1.f);


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
}

void Game::RenderLightSource(Light light) const
{
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
		g_theDevConsole->Render(AABB2(0, 0, 1600, 800));
		
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
	SwitchToScene("Scene 2");
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

	const auto& emittersRef = g_theParticleSystem->GetEmitters();
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

	if (ImGui::BeginTable("EmitterStats", 7, flags, ImVec2(0.f, tableHeight)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Enabled", ImGuiTableColumnFlags_WidthFixed, 70.f);
		ImGui::TableSetupColumn("Active", ImGuiTableColumnFlags_WidthFixed, 90.f);
		ImGui::TableSetupColumn("Spawn", ImGuiTableColumnFlags_WidthFixed, 80.f);
		ImGui::TableSetupColumn("GPU ms", ImGuiTableColumnFlags_WidthFixed, 80.f);
		ImGui::TableSetupColumn("Age", ImGuiTableColumnFlags_WidthFixed, 70.f);
		ImGui::TableSetupColumn("Notes");
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
			ImGui::TextUnformatted(e->IsEnabled() ? "Yes" : "No");

			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%d / %d", active, maxP);

			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%.1f", getSpawn(e));

			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%.3f", getGpu(e));

			ImGui::TableSetColumnIndex(5);
			ImGui::Text("%.2f", e->GetAge());

			ImGui::TableSetColumnIndex(6);
			float usage = (maxP > 0) ? (float)active / (float)maxP : 0.f;
			if (usage > 0.95f) ImGui::TextUnformatted("Near cap");
			else if (usage > 0.70f) ImGui::TextUnformatted("High");
			else ImGui::TextUnformatted("-");
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
	UNUSED(deltaSeconds);

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

	RenderAttractRing();


	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderAttractRing() const
{
	DebugDrawRing(Vec2(800.f, 400.f), m_attractRingRadius, m_attractRingThickness, Rgba8(255, 127, 0, 255));
	g_theRenderer->BindTexture(nullptr);
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
		delete g_theParticleSystem;
		g_theParticleSystem = nullptr;
	}

}

bool Game::Event_KeysAndFuncs(EventArgs& args)
{
	UNUSED(args);

	g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, "Controls");

	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Mouse  - Aim");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "W / A  - Move");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "S / D  - Strafe");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Q / E  - Roll");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Z / C  - Elevate");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Shift  - Sprint");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "H      - Set Camera to Origin");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "P      - Pause Game");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "O      - Step One Frame");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "T      - Slow Motion Mode");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "~      - Open / Close DevConsole");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Escape - Exit Game");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "1      - Spawn a Forward Line");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "2      - Spawn a Sphere Below At Z = 0");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "3      - Spawn a Wireframe Sphere");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "4      - Spawn a Player World Basis");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "5      - Spawn a Full Opposing Billboard Text of Player Pos and Ori");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "6      - Spawn a Wireframe Cylinder");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "7      - Add a Screen Message with Current Camera Orientation");

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
		if (ImGui::InputText("Name", nameBuf, IM_ARRAYSIZE(nameBuf))) {
			cfg.name = nameBuf; dirty = true;
		}
	}

	{
		char pathBuf[256] = {};
		strncpy_s(pathBuf, cfg.mainStage.texPath.c_str(), sizeof(pathBuf) - 1);
		if (ImGui::InputText("Texture Path", pathBuf, IM_ARRAYSIZE(pathBuf))) {
			cfg.mainStage.texPath = pathBuf; dirty = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Reload Texture")) {
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

	int sc[4] = { cfg.mainStage.startColor.r, cfg.mainStage.startColor.g, cfg.mainStage.startColor.b, cfg.mainStage.startColor.a };
	int ec[4] = { cfg.mainStage.endColor.r, cfg.mainStage.endColor.g, cfg.mainStage.endColor.b, cfg.mainStage.endColor.a };
	if (ImGui::SliderInt4("Start Color", sc, 0, 255)) { cfg.mainStage.startColor = Rgba8((uint8_t)sc[0], (uint8_t)sc[1], (uint8_t)sc[2], (uint8_t)sc[3]); dirty = true; }
	if (ImGui::SliderInt4("End Color", ec, 0, 255)) { cfg.mainStage.endColor = Rgba8((uint8_t)ec[0], (uint8_t)ec[1], (uint8_t)ec[2], (uint8_t)ec[3]); dirty = true; }

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

	{
		float sunColor[3] = { L.m_sunColor.x, L.m_sunColor.y, L.m_sunColor.z };
		if (ImGui::ColorEdit3("Sun Color", sunColor))
		{
			L.m_sunColor.x = sunColor[0];
			L.m_sunColor.y = sunColor[1];
			L.m_sunColor.z = sunColor[2];
		}

		float sunIntensity = L.m_sunColor.w;
		if (ImGui::DragFloat("Sun Intensity", &sunIntensity, 0.01f, 0.0f, 10.0f))
		{
			L.m_sunColor.w = sunIntensity;
		}

		float sunDir[3] = { L.m_sunDirection.x, L.m_sunDirection.y, L.m_sunDirection.z };
		if (ImGui::DragFloat3("Sun Direction", sunDir, 0.01f, -1.0f, 1.0f))
		{
			L.m_sunDirection = Vec3(sunDir[0], sunDir[1], sunDir[2]).GetNormalized();
		}
	}

	ImGui::Spacing();
	ImGui::SeparatorText("Point & Spot Lights");

	if (L.m_numLights <= 0)
	{
		ImGui::TextDisabled("No lights in the scene.");
		ImGui::Separator();
	}
	else
	{
		if (m_uiLightIndex < 0) m_uiLightIndex = 0;
		if (m_uiLightIndex >= L.m_numLights) m_uiLightIndex = L.m_numLights - 1;

		auto isSpot = [&](const Light& li)->bool
			{
				return (li.m_direction.GetLengthSquared() > 0.1f);
			};

		auto labelFor = [&](int i)->std::string
			{
				const Light& li = L.m_lightsArray[i];
				char buf[128];
				snprintf(buf, sizeof(buf), "Light %d (%s)", i, isSpot(li) ? "Spot" : "Point");
				return std::string(buf);
			};

		if (ImGui::BeginTable("##LightLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
		{
			ImGui::TableSetupColumn("List", ImGuiTableColumnFlags_WidthFixed, 220.0f);
			ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextColumn();
			ImGui::BeginChild("##LightList", ImVec2(0, 260), true);

			if (ImGui::BeginListBox("##LightListBox", ImVec2(-FLT_MIN, -FLT_MIN)))
			{
				for (int i = 0; i < L.m_numLights; ++i)
				{
					bool selected = (i == m_uiLightIndex);
					std::string item = labelFor(i);
					if (ImGui::Selectable(item.c_str(), selected))
						m_uiLightIndex = i;
					if (selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndListBox();
			}

			ImGui::EndChild();

			ImGui::TableNextColumn();
			ImGui::BeginChild("##LightInspector", ImVec2(0, 260), true);

			ImGui::Text("Editing: %d / %d", m_uiLightIndex, L.m_numLights - 1);
			ImGui::Separator();

			Light& light = L.m_lightsArray[m_uiLightIndex];

			float lightColor[3] = { light.m_color.x, light.m_color.y, light.m_color.z };
			if (ImGui::ColorEdit3("Color", lightColor))
			{
				light.m_color.x = lightColor[0];
				light.m_color.y = lightColor[1];
				light.m_color.z = lightColor[2];
			}

			float lightIntensity = light.m_color.w;
			if (ImGui::DragFloat("Intensity", &lightIntensity, 0.1f, 0.0f, 100.0f))
			{
				light.m_color.w = lightIntensity;
			}

			float lightPos[3] = { light.m_position.x, light.m_position.y, light.m_position.z };
			if (ImGui::DragFloat3("Position", lightPos, 0.1f))
			{
				light.m_position = Vec3(lightPos[0], lightPos[1], lightPos[2]);
			}

			bool spotlight = isSpot(light);
			if (ImGui::Checkbox("Spotlight", &spotlight))
			{
				if (spotlight)
				{
					if (light.m_direction.GetLengthSquared() <= 0.1f)
						light.m_direction = Vec3(0, 0, -1);

					if (light.m_outerDotThreshold <= -1.0f) light.m_outerDotThreshold = CosDegrees(30.0f);
					if (light.m_innerDotThreshold <= -1.0f) light.m_innerDotThreshold = CosDegrees(15.0f);
				}
				else
				{
					light.m_direction = Vec3(0, 0, 0);
					light.m_innerDotThreshold = -1.0f;
					light.m_outerDotThreshold = -2.0f;
				}
			}

			if (spotlight)
			{
				float lightDir[3] = { light.m_direction.x, light.m_direction.y, light.m_direction.z };
				if (ImGui::DragFloat3("Direction", lightDir, 0.01f, -1.0f, 1.0f))
				{
					light.m_direction = Vec3(lightDir[0], lightDir[1], lightDir[2]).GetNormalized();
				}

				float innerAngle = ACosDegrees(light.m_innerDotThreshold);
				float outerAngle = ACosDegrees(light.m_outerDotThreshold);
				float oldInner = innerAngle;
				float oldOuter = outerAngle;

				if (ImGui::DragFloat("Inner Angle", &innerAngle, 0.5f, 0.0f, 90.0f))
				{
					float delta = innerAngle - oldInner;
					if (delta > 0 && innerAngle > outerAngle - 2.0f)
					{
						outerAngle = Min(innerAngle + 2.0f, 90.0f);
						light.m_outerDotThreshold = CosDegrees(outerAngle);
					}
					light.m_innerDotThreshold = CosDegrees(innerAngle);
				}

				if (ImGui::DragFloat("Outer Angle", &outerAngle, 0.5f, 0.0f, 90.0f))
				{
					float delta = outerAngle - oldOuter;
					if (delta < 0 && outerAngle < innerAngle + 2.0f)
					{
						innerAngle = Max(outerAngle - 2.0f, 0.0f);
						light.m_innerDotThreshold = CosDegrees(innerAngle);
					}
					light.m_outerDotThreshold = CosDegrees(outerAngle);
				}
			}

			ImGui::DragFloat("Ambience", &light.m_ambience, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Inner Radius", &light.m_innerRadius, 0.1f, 0.0f, 10.0f);
			ImGui::DragFloat("Outer Radius", &light.m_outerRadius, 0.1f, 0.0f, 50.0f);

			ImGui::Spacing();
			ImGui::Separator();

			if (ImGui::Button("Remove Selected Light"))
			{
				for (int j = m_uiLightIndex; j < L.m_numLights - 1; ++j)
					L.m_lightsArray[j] = L.m_lightsArray[j + 1];

				L.m_numLights--;
				if (L.m_numLights <= 0) m_uiLightIndex = 0;
				else if (m_uiLightIndex >= L.m_numLights) m_uiLightIndex = L.m_numLights - 1;
			}

			ImGui::EndChild();

			ImGui::EndTable();
		}
	}

	ImGui::Separator();
	if (L.m_numLights < MAX_LIGHTS)
	{
		if (ImGui::Button("Add Point Light"))
		{
			Light& newLight = L.m_lightsArray[L.m_numLights++];
			newLight.m_color = Vec4(1.0f, 1.0f, 1.0f, 5.0f);
			newLight.m_position = Vec3(0, 0, 3);
			newLight.m_direction = Vec3(0, 0, 0);
			newLight.m_ambience = 0.1f;
			newLight.m_innerRadius = 0.5f;
			newLight.m_outerRadius = 10.0f;
			newLight.m_innerDotThreshold = -1.0f;
			newLight.m_outerDotThreshold = -2.0f;

			m_uiLightIndex = L.m_numLights - 1;
		}

		ImGui::SameLine();

		if (ImGui::Button("Add Spot Light"))
		{
			Light& newLight = L.m_lightsArray[L.m_numLights++];
			newLight.m_color = Vec4(1.0f, 0.8f, 0.6f, 10.0f);
			newLight.m_position = Vec3(0, 0, 5);
			newLight.m_direction = Vec3(0, 0, -1);
			newLight.m_ambience = 0.0f;
			newLight.m_innerRadius = 0.0f;
			newLight.m_outerRadius = 15.0f;
			newLight.m_innerDotThreshold = CosDegrees(15.0f);
			newLight.m_outerDotThreshold = CosDegrees(30.0f);

			m_uiLightIndex = L.m_numLights - 1;
		}
	}
	else
	{
		ImGui::Text("Maximum number of lights reached (%d)", MAX_LIGHTS);
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

	ImGui::Checkbox("Debug Draw Forces", &m_debugDraw);

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
					f.m_radialFalloffPow = 1.8f;
					f.m_heightFalloffPow = 5.0f;
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

			ImGui::SeparatorText("Falloff");
			if (ImGui::DragFloat("Radial Falloff Pow", &f.m_radialFalloffPow, 0.1f, 0.f, 20.f, "%.1f")) changed = true;
			if (ImGui::DragFloat("Height Falloff Pow", &f.m_heightFalloffPow, 0.1f, 0.f, 20.f, "%.1f")) changed = true;
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
	ImGui::Text("Current Scene: %s", cur ? cur->GetName().c_str() : "None");

	std::vector<std::string> sceneNames;
	sceneNames.reserve(m_sceneManager.m_scenes.size());
	for (const auto& pair : m_sceneManager.m_scenes)
	{
		sceneNames.push_back(pair.first);
	}

	if (sceneNames.empty())
	{
		ImGui::Separator();
		ImGui::TextDisabled("No scenes registered.");
		return;
	}

	if (cur)
	{
		const std::string& curName = cur->GetName();
		for (int i = 0; i < (int)sceneNames.size(); ++i)
		{
			if (sceneNames[i] == curName)
			{
				m_uiSceneIndex = i;
				break;
			}
		}
	}

	if (m_uiSceneIndex < 0) m_uiSceneIndex = 0;
	if (m_uiSceneIndex >= (int)sceneNames.size()) m_uiSceneIndex = (int)sceneNames.size() - 1;

	ImGui::Separator();

	static float s_fadeDuration = 1.0f;
	ImGui::SetNextItemWidth(140.f);
	ImGui::DragFloat("Fade (s)", &s_fadeDuration, 0.05f, 0.0f, 10.0f, "%.2f");

	ImGui::TextUnformatted("Select Scene:");
	const char* preview = sceneNames[m_uiSceneIndex].c_str();

	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::BeginCombo("##SceneCombo", preview))
	{
		for (int i = 0; i < (int)sceneNames.size(); ++i)
		{
			bool selected = (i == m_uiSceneIndex);
			if (ImGui::Selectable(sceneNames[i].c_str(), selected))
			{
				m_uiSceneIndex = i;
				SwitchToScene(sceneNames[i], s_fadeDuration);
			}
			if (selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	ImGui::Spacing();
	if (ImGui::Button("Prev"))
	{
		int nextIdx = m_uiSceneIndex - 1;
		if (nextIdx < 0) nextIdx = (int)sceneNames.size() - 1;
		m_uiSceneIndex = nextIdx;
		SwitchToScene(sceneNames[m_uiSceneIndex], s_fadeDuration);
	}

	ImGui::SameLine();
	if (ImGui::Button("Next"))
	{
		int nextIdx = m_uiSceneIndex + 1;
		if (nextIdx >= (int)sceneNames.size()) nextIdx = 0;
		m_uiSceneIndex = nextIdx;
		SwitchToScene(sceneNames[m_uiSceneIndex], s_fadeDuration);
	}

	ImGui::SameLine();
	ImGui::BeginDisabled(cur == nullptr);
	if (ImGui::Button("Reload"))
	{
		SwitchToScene(sceneNames[m_uiSceneIndex], s_fadeDuration);
	}
	ImGui::EndDisabled();
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
			if (br > 0.f)
			{
				AddVertsForUVSphereZWireframe3D(verts, base, br, 16, 0.01f, fColor);
			}

			float tr = f.m_topRadius;
			if (tr > 0.f)
			{
				AddVertsForUVSphereZWireframe3D(verts, top, tr, 16, 0.01f, fColor);
			}

			if (br > 0.f || tr > 0.f)
			{
				float midT = 0.5f;
				Vec3 mid = base + axis * (height * midT);
				float midRadius = br + (tr - br) * midT;
				if (midRadius > 0.f)
				{
					AddVertsForUVSphereZWireframe3D(verts, mid, midRadius, 16, 0.01f, fColor);
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
}

void Game::SetupScene1()
{
 	Scene* scene = GetCurrentScene();
	if (!scene) return;

	m_skyTex = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene1/Sky.png");

	m_skyVerts.clear();
	AddVertsForSkySphere3D(m_skyVerts, Rgba8(255, 255, 255, 255));

	scene->ClearGameObjects();
	scene->ClearParticleEmitters();

	m_player->SetPosition(Vec3(-3.f, 0.1f, 0.68f));
	m_player->SetOrientation(EulerAngles(-14.4f, -12.2f, 0.f));

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

	float innerRadius = 8.0f;
	float outerRadius = 20.0f;
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
		fireConfig.subStage.prob = 0.01f;

		ParticleEmitter* fireWithSmoke = g_theParticleSystem->CreateEmitter(fireConfig);
		scene->AddParticleEmitter(fireWithSmoke);
	}

	{
		float snowHeight = 15.0f;
		float spawnThickness = 3.0f;
		float areaHalfSize = 50.0f;

		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "SnowField";
		cfg.mainStage.texPath = "Data/Images/Scene1/snow.png";
		cfg.blendMode = BlendMode::ALPHA;

		cfg.position = Vec3(0.f, 0.f, snowHeight);

		cfg.spawnArea = Vec3(areaHalfSize,
			areaHalfSize,
			spawnThickness * 0.5f);

		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, -2.0f);
		cfg.mainStage.velocityVariance = Vec3(0.8f, 0.8f, 2.0f);

		cfg.mainStage.lifetime = 8.0f;
		cfg.mainStage.lifetimeVariance = 2.0f;

		cfg.mainStage.startColor = Rgba8(255, 255, 255, 255);
		cfg.mainStage.endColor = Rgba8(255, 255, 255, 255);

		cfg.mainStage.startSize = 0.05f;
		cfg.mainStage.endSize = 0.08f;

		cfg.spawnRate = 3000.0f;
		cfg.maxParticles = 200000;

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
		cfg.mainStage.startColor = Rgba8(127, 127, 127, 100);
		cfg.mainStage.endColor = Rgba8(127, 127, 127, 0);
		cfg.spawnRate = 500.f;
		cfg.maxParticles = 10000;
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

	m_player->SetPosition(Vec3(-30.5f, -51.f, 3.f));
	m_player->SetOrientation(EulerAngles(97.f, -18.f, 0.f));

	Lights& L = scene->GetLights();
	L.m_sunColor = Vec4(1.f, 1.f, 1.f, 0.2f);
	L.m_sunDirection = Vec3(0.f, -1.f, -1.f).GetNormalized();
	L.m_numLights = 0;

	const char* kRainTexPath = "Data/Images/rainwater.png";
	const char* kSplashTexPath = "Data/Images/waterSplash2.png";

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

	const char* kBuildingMeshes[5] = {
		"Data/Meshes/building1",
		"Data/Meshes/building2",
		"Data/Meshes/building3",
		"Data/Meshes/building4",
		"Data/Meshes/building5"
	};

	const char* kBuildingTexPaths[5] = {
		"Data/Images/Scene3/building1.png",
		"Data/Images/Scene3/building2.png",
		"Data/Images/Scene3/building3.png",
		"Data/Images/Scene3/building4.png",
		"Data/Images/Scene3/building5.png"
	};

	const char* kBuildingNTexPaths[5] = {
		"Data/Images/Scene3/building1_n.png",
		"Data/Images/Scene3/building2_n.png",
		"Data/Images/Scene3/building3_n.png",
		"Data/Images/Scene3/building4_n.png",
		"Data/Images/Scene3/building5_n.png"
	};

	const char* kBuildingSGETexPaths[5] = {
		"Data/Images/Scene3/building1_SGE.png",
		"Data/Images/Scene3/building2_SGE.png",
		"Data/Images/Scene3/building3_SGE.png",
		"Data/Images/Scene3/building4_SGE.png",
		"Data/Images/Scene3/building5_SGE.png"
	};

	Texture* buildingTex[5], * buildingNTex[5], * buildingSGETex[5];
	for (int i = 0; i < 5; ++i) {
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

	BuildingParams params[5] = {
		{ 30.0f,  32.0f,   0.0f, Vec3(0.0f,   20.0f, 0) },
		{ 18.0f,  20.0f,  90.0f, Vec3(-50.0f,  -40.0f, 0) },
		{ 26.0f,  28.0f, -45.0f, Vec3(-45.0f,   35.0f, 0) },
		{ 22.0f,  24.0f,  45.0f, Vec3(45.0f,   35.0f, 0) },
		{ 38.0f,  40.0f,   0.0f, Vec3(0.0f,  -80.0f, 0) } 
	};

	GameObject* buildings[5] = { nullptr };

	for (int i = 0; i < 5; ++i)
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

		cfg.mainStage.startColor = Rgba8(255, 255, 255, 220);
		cfg.mainStage.endColor = Rgba8(200, 200, 255, 0);

		cfg.mainStage.startSize = 0.05f;
		cfg.mainStage.endSize = 0.05f;
		cfg.mainStage.billboardType = 0u;

		cfg.spawnRate = 200000.0f;
		cfg.maxParticles = 1000000;

		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;

		cfg.useSubStage = true;
		cfg.subStage.texPath = kSplashTexPath;

		cfg.subStage.lifetime = 0.35f;
		cfg.subStage.lifetimeVariance = 0.10f;

		cfg.subStage.startColor = Rgba8(180, 180, 220, 255);
		cfg.subStage.endColor = Rgba8(180, 180, 220, 0);

		cfg.subStage.startSize = 0.8f;
		cfg.subStage.endSize = 1.5f;

		cfg.subStage.baseVelocity = Vec3(0.f, 0.f, 0.f);
		cfg.subStage.velocityVariance = Vec3(0.f, 0.f, 0.0f);

		cfg.subStage.prob = 1.0f;
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

	m_player->SetPosition(Vec3(-40.f, 33.1f, 2.68f));
	m_player->SetOrientation(EulerAngles(-48.4f, -14.2f, 0.f));

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
			tile->SetColor(Rgba8(230, 210, 170, 255));
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


	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "TornadoCoreFine";
		cfg.mainStage.texPath = "Data/Images/Scene4/dust1.png";
		cfg.blendMode = BlendMode::ALPHA_ADDITIVE;
		cfg.position = Vec3(0.f, 0.f, 1.8f);
		cfg.spawnArea = Vec3(1.4f, 1.4f, 5.5f);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.04f);
		cfg.mainStage.velocityVariance = Vec3(0.35f, 0.35f, 0.50f);
		cfg.mainStage.lifetime = 5.5f;
		cfg.mainStage.lifetimeVariance = 3.0f;
		cfg.mainStage.startColor = Rgba8(120, 80, 60, 5); 
		cfg.mainStage.endColor = Rgba8(220, 200, 160, 5);
		cfg.mainStage.startSize = 0.15f;
		cfg.mainStage.endSize = 1.2f;
		cfg.spawnRate = 35000.0f; 
		cfg.maxParticles = 1500000;
		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;
		cfg.noiseStrength = 1.0f;
		cfg.noiseFrequency = 1.0f;
		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
	}

	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "TornadoCoreFine";
		cfg.mainStage.texPath = "Data/Images/Scene4/dust1.png";
		cfg.blendMode = BlendMode::ALPHA;
		cfg.position = Vec3(0.f, 0.f, 1.8f);
		cfg.spawnArea = Vec3(1.4f, 1.4f, 5.5f);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.04f);
		cfg.mainStage.velocityVariance = Vec3(0.35f, 0.35f, 0.50f);
		cfg.mainStage.lifetime = 5.5f;
		cfg.mainStage.lifetimeVariance = 3.0f;
		cfg.mainStage.startColor = Rgba8(100, 60, 20, 255); 
		cfg.mainStage.endColor = Rgba8(220, 200, 180, 30);
		cfg.mainStage.startSize = 0.15f;
		cfg.mainStage.endSize = 1.2f;
		cfg.spawnRate = 25000.0f; 
		cfg.maxParticles = 150000;
		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;
		cfg.noiseStrength = 1.0f; 
		cfg.noiseFrequency = 1.0f;
		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
	}

	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "TornadoCoreFine";
		cfg.mainStage.texPath = "Data/Images/Scene4/dust1.png";
		cfg.blendMode = BlendMode::ALPHA;
		cfg.position = Vec3(0.f, 0.f, 1.8f);
		cfg.spawnArea = Vec3(1.4f, 1.4f, 5.5f);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.04f);
		cfg.mainStage.velocityVariance = Vec3(0.35f, 0.35f, 0.50f); 
		cfg.mainStage.lifetime = 5.5f;
		cfg.mainStage.lifetimeVariance = 3.0f;
		cfg.mainStage.startColor = Rgba8(60, 40, 20, 255);
		cfg.mainStage.endColor = Rgba8(180, 160, 130, 30);
		cfg.mainStage.startSize = 0.15f;
		cfg.mainStage.endSize = 1.2f;
		cfg.spawnRate = 15000.0f; 
		cfg.maxParticles = 150000;
		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;
		cfg.noiseStrength = 1.0f; 
		cfg.noiseFrequency = 1.0f;
		ParticleEmitter* em = g_theParticleSystem->CreateEmitter(cfg);
		scene->AddParticleEmitter(em);
	}

	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "GroundSandLift";
		cfg.mainStage.texPath = "Data/Images/Scene4/pebble1.png";
		cfg.blendMode = BlendMode::ALPHA;
		cfg.position = Vec3(0.f, 0.f, 0.2f);
		cfg.spawnArea = Vec3(6.0f, 6.0f, 1.0f); 
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.6f); 
		cfg.mainStage.velocityVariance = Vec3(1.2f, 1.2f, 1.8f); 
		cfg.mainStage.lifetime = 3.2f;
		cfg.mainStage.lifetimeVariance = 1.5f;
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
		cfg.position = Vec3(0.f, 0.f, 5.0f);
		cfg.spawnArea = Vec3(3.0f, 3.0f, 10.0f);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.03f);
		cfg.mainStage.velocityVariance = Vec3(0.25f, 0.25f, 0.35f);
		cfg.mainStage.lifetime = 7.5f;
		cfg.mainStage.lifetimeVariance = 4.0f;
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
	}



	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "TopMistDiffusion";
		cfg.mainStage.texPath = "Data/Images/Scene4/dust2.png"; 
		cfg.blendMode = BlendMode::ALPHA;
		cfg.position = Vec3(0.f, 0.f, 35.0f);
		cfg.spawnArea = Vec3(10.0f, 10.0f, 10.0f);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 5.0f);
		cfg.mainStage.velocityVariance = Vec3(0.4f, 0.4f, 0.6f);
		cfg.mainStage.lifetime = 6.0f;
		cfg.mainStage.lifetimeVariance = 2.5f;
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
	}

	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = g_theParticleSystem;
		cfg.name = "GroundDustRise";
		cfg.mainStage.texPath = "Data/Images/Scene4/dust2.png"; 
		cfg.blendMode = BlendMode::ALPHA;
		cfg.position = Vec3(0.f, 0.f, 0.15f);
		cfg.spawnArea = Vec3(100.f, 100.f, 0.6f);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.f);
		cfg.mainStage.velocityVariance = Vec3(0.5f, 0.5f, 0.2f);
		cfg.mainStage.lifetime = 2.8f;
		cfg.mainStage.lifetimeVariance = 1.0f;
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
		cfg.mainStage.lifetimeVariance = 1.0f;
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
		ParticleForce tornado = ParticleForce::MakeFlowColumn(
			Vec3(0.f, 0.f, 0.f),                    
			Vec3(0.f, 0.f, 1.f),
			48.0f,
			1.4f,
			16.0f,
			24.0f,
			60.0f,
			48.0f,
			1.4f,
			5.0f,
			FLOW_SWIRL_ENABLE | FLOW_RADIAL_ENABLE | FLOW_AXIAL_ENABLE
		);
		g_theParticleSystem->AddForce(tornado);
	}

	{
		ParticleForce topDiffusion = ParticleForce::MakeFlowColumn(
			Vec3(4.5f, 0.3f, 22.f),                    
			Vec3(0.f, 0.f, 1.f),
			18.0f,
			7.0f,
			15.0f,
			12.0f,
			5.0f,
			-18.0f,
			2.2f,
			2.5f,
			FLOW_RADIAL_OUTWARD | FLOW_AXIAL_ENABLE
		);
		g_theParticleSystem->AddForce(topDiffusion);
	}

	{
		ParticleForce midSwirl = ParticleForce::MakeFlowColumn(
			Vec3(-2.7f, 1.2f, 12.f),                    
			Vec3(0.1f, 0.05f, 1.f).GetNormalized(),
			20.0f,
			4.0f,
			8.0f,
			35.0f,
			15.0f,
			20.0f,
			2.0f,
			3.0f,
			FLOW_SWIRL_ENABLE | FLOW_RADIAL_ENABLE | FLOW_AXIAL_ENABLE
		);
		g_theParticleSystem->AddForce(midSwirl);
	}

	{
		ParticleForce noisePerturb = ParticleForce::MakeFlowColumn(
			Vec3(-0.9f, -0.4f, 10.f),                   
			Vec3(0.f, 0.f, 1.f),
			30.0f,
			5.0f,
			10.0f,
			25.0f,
			10.0f,
			15.0f,
			3.0f,
			4.0f,
			FLOW_SWIRL_ENABLE | FLOW_RADIAL_ENABLE | FLOW_AXIAL_ENABLE
		);
		g_theParticleSystem->AddForce(noisePerturb);
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

	m_player->SetPosition(Vec3(-26.5f, -19.f, 5.f));
	m_player->SetOrientation(EulerAngles(-270.f, 4.4f, 0.f));
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
	m_player->SetPosition(Vec3(0.1f, 2.1f, 2.37f));
	m_player->SetOrientation(EulerAngles(-97.4f, 33.2f, 0.f));

	GameObject* cave = scene->CreateGameObject(this, "Cave");
	cave->InitializeVertsFromFile("Data/Meshes/cave");
	cave->SetPosition(Vec3(0.f, 0.f, 0.f));
	cave->SetScale(Vec3(100.0f, 100.0f, 100.0f));
	cave->SetOrientation(EulerAngles(0.f, 0.f, 90.f));

	cave->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene2/cave.png");

	cave->SetColor(Rgba8::WHITE);

	GameObject* platform = scene->CreateGameObject(this, "Platform");
	platform->InitializeVertsFromFile("Data/Meshes/platform");
	platform->SetPosition(Vec3(0.f, 0.f, 0.f));
	platform->SetScale(Vec3(5.0f, 5.0f, 5.0f));
	platform->SetOrientation(EulerAngles(0.f, 0.f, 90.f));

	platform->m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Scene2/platform.png");

	platform->SetColor(Rgba8::WHITE);


	Lights& L = scene->GetLights();
	L.m_sunColor = Vec4(0.9f, 0.95f, 1.0f, 0.5f);
	L.m_sunDirection = Vec3(0.3f, -0.6f, -0.7f).GetNormalized();
	L.m_numLights = 0;
	Vec3 circleBaseCenter = Vec3(0.f, 0.f, 0.51f);
	RandomNumberGenerator rng;
	float layerHeightStep = 0.4f;

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
		{ "Data/Images/Scene2/m1.png", 2.50f, 0.00f, Rgba8(255,255,255,150),  1.0f,  1.0f, 5.8f, 0.9f, 1 },
		{ "Data/Images/Scene2/m2.png", 3.075f,0.40f, Rgba8(200,170,255,100), -1.2f,  1.0f, 7.7f, 0.85f, 1 },
		{ "Data/Images/Scene2/m3.png", 1.875f,0.80f, Rgba8(170,200,255,120),  1.5f,  1.0f, 4.6f, 0.8f, 1 },
		{ "Data/Images/Scene2/m4.png", 2.175f,1.20f, Rgba8(200,120,255, 80), -0.4f,  1.0f, 6.65f, 0.9f, 1 },
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
		cfg.mainStage.texPath = "Data/Images/Scene2/m3.png";
		Vec3 startPos = circleBaseCenter + Vec3(0.f, 0.f, 0.15f);
		cfg.position = startPos;
		cfg.spawnArea = Vec3(0.02f, 0.02f, 0.02f);
		float riseSpeed = 0.25f;
		float lifeTime = 5.0f;
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, riseSpeed);
		cfg.mainStage.velocityVariance = Vec3(0.0f, 0.0f, 0.0f);
		cfg.mainStage.startSize = 3.0f;
		cfg.mainStage.endSize = 5.0f;
		cfg.mainStage.startColor = Rgba8(200, 170, 255, 200);
		cfg.mainStage.endColor = Rgba8(200, 170, 255, 0);
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
		cfg.spawnArea = Vec3(3.8f, 3.8f, 0.3f);
		cfg.mainStage.lifetime = 4.0f;
		cfg.mainStage.lifetimeVariance = 1.5f;
		cfg.mainStage.startSize = 0.55f;
		cfg.mainStage.endSize = 1.35f;
		cfg.mainStage.startColor = Rgba8(180, 150, 255, 80);
		cfg.mainStage.endColor = Rgba8(180, 150, 255, 0);
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, 0.05f);
		cfg.mainStage.velocityVariance = Vec3(0.04f, 0.04f, 0.04f);
		cfg.mainStage.billboardType = 0u;
		cfg.spawnRate = 6.0f;
		cfg.maxParticles = 48;
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
