#include "Game/App.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Window/Window.hpp"
#include "Game/Game.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "ThirdParty/imgui/imgui.h"
#include "ThirdParty/imgui/backends/imgui_impl_win32.h"
#include "ThirdParty/imgui/backends/imgui_impl_dx11.h"
#include <iostream>



App*			g_theApp		= nullptr;
Renderer*		g_theRenderer	= nullptr;
InputSystem*	g_theInput		= nullptr;
AudioSystem*	g_theAudio		= nullptr;
Window*			g_theWindow		= nullptr;
BitmapFont* g_theFont = nullptr;
DevConsole* g_theDevConsole = nullptr;
EventSystem* g_theEventSystem = nullptr;

App::App()
{

}

App::~App() 
{

}


void App::Startup()
{
	XmlDocument gameDoc;
	XmlResult result = gameDoc.LoadFile("Data/GameConfig.xml");

	if (result != tinyxml2::XML_SUCCESS)
	{
		ERROR_AND_DIE("Failed to load Data/GameConfig.xml");
	}

	XmlElement* gameRoot = gameDoc.RootElement();
	if (gameRoot == nullptr)
	{
		ERROR_AND_DIE("GameConfig.xml is missing a root element");
	}

	g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*gameRoot);

	InputConfig inputConfig;
	g_theInput = new InputSystem(inputConfig);

	WindowConfig windowConfig;
	windowConfig.m_aspectRatio = g_gameConfigBlackboard.GetValue("windowAspect", 1.0f);
	windowConfig.m_inputSystem = g_theInput;
	windowConfig.m_windowTitle = g_gameConfigBlackboard.GetValue("projectName", Stringf("Unnamed Project"));
	g_theWindow = new Window(windowConfig);

	RendererConfig rendererConfig;
	rendererConfig.m_window = g_theWindow;
	g_theRenderer = new Renderer(rendererConfig);

	AudioConfig audioConfig;
	g_theAudio = new AudioSystem(audioConfig);

	DevConsoleConfig devConsoleConfig;
	devConsoleConfig.m_renderer = g_theRenderer;
	g_theDevConsole = new DevConsole(devConsoleConfig);

	EventSystemConfig eventConfig;
	g_theEventSystem = new EventSystem(eventConfig);

	DebugRenderConfig debugRenderConfig;
	debugRenderConfig.m_renderer = g_theRenderer;
	debugRenderConfig.m_fontName = "SquirrelFixedFont";

	

	
	g_theEventSystem->SubscribeEventCallbackFunction("quit", App::Event_Quit);

	g_theInput->Startup();
	g_theWindow->Startup();
	g_theRenderer->Startup();
	g_theAudio->Startup();
	g_theDevConsole->Startup();
	g_theFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	DebugRenderSystemStartup(debugRenderConfig);
	InitImGui(g_theWindow->GetHWND(), g_theRenderer->GetDevice(), g_theRenderer->GetDeviceContext());


	m_game = new Game(g_theApp);

	g_theEventSystem->SubscribeEventCallbackFunction("Controls", Game::Event_KeysAndFuncs);
	g_theEventSystem->FireEvent("Controls");
	
	g_theEventSystem->SubscribeEventCallbackFunction("DebugRenderClear", Command_DebugRenderClear);
	g_theEventSystem->SubscribeEventCallbackFunction("DebugRenderToggle", Command_DebugRenderToggle);
}




void App::Update()
{
	bool shouldUsePointerMode = !g_theWindow->HasFocus() || g_theDevConsole->IsOpen() || m_game->m_currentState == GameState::ATTRACT || m_game->m_imguiCursor;

	if (shouldUsePointerMode)
	{
		g_theInput->SetCursorMode(CursorMode::POINTER);
	}
	else
	{
		g_theInput->SetCursorMode(CursorMode::FPS);
	}

	

	m_game->Update();
}

void App::Shutdown()
{
	m_game->Shutdown();

	delete m_game;
	m_game = nullptr;

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	DebugRenderSystemShutdown();

	g_theRenderer->Shutdown();
	g_theDevConsole->Shutdown();
	g_theAudio->Shutdown();
	
	g_theWindow->ShutDown();
	g_theInput->Shutdown();

	delete g_theEventSystem;
	g_theEventSystem = nullptr;
	delete g_theDevConsole;
	g_theDevConsole = nullptr;
	delete g_theAudio;
	g_theAudio = nullptr;
	delete g_theRenderer;
	g_theRenderer = nullptr;
	delete g_theWindow;
	g_theWindow = nullptr;
	delete g_theInput;
	g_theInput = nullptr;
	
}

bool App::HandleQuitRequested()
{
	m_isQuitting = true;
	return true;
}

void App::BeginFrame()
{
	Clock::TickSystemClock();

	g_theInput->BeginFrame();
	g_theWindow->BeginFrame();
	g_theRenderer->BeginFrame();
	g_theAudio->BeginFrame();
	
	g_theDevConsole->BeginFrame();
	g_theEventSystem->BeginFrame();

	DebugRenderBeginFrame();
	//g_theNetwork->BeginFrame();

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	// ImGui::ShowDemoWindow(); 
	ShowImguiWindow();
}



void App::ResetGame()
{
	m_game->Shutdown();
	delete m_game;
	m_game = new Game(g_theApp);
}

bool App::Event_Quit(EventArgs& args)
{
	UNUSED(args);
	if (!g_theApp)
	{
		return false;
	}
	g_theApp->HandleQuitRequested();
	return true;
}

void App::Render() const
{
	m_game->Render();
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void App::EndFrame()
{
	g_theAudio->EndFrame();
	g_theRenderer->EndFrame();
	g_theWindow->EndFrame();
	g_theInput->EndFrame();
	g_theDevConsole->EndFrame();
	g_theEventSystem->EndFrame();
	
	DebugRenderEndFrame();

	
}


void App::InitImGui(void* hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
/*
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;*/


	ImGui::StyleColorsDark();
/*
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}*/

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(device, context);
}



void App::ShowImguiWindow()
{
	if (m_game)
	{
		m_game->ShowImguiWindow();
	}
}

void App::RunFrame()
{
	BeginFrame();
	Update();
	Render();
	EndFrame();
}

