#include <Engine/Math/Vec2.hpp>
#include <Engine/Core/Rgba8.hpp>
#include <Engine/Renderer/Camera.hpp>
#include <Engine/Core/Vertex_PCU.hpp>
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Game/Game.hpp"

class App 
{
public:
	App();
	~App();
	void Startup();
	void Shutdown();
	void RunFrame();

	bool HandleQuitRequested();
	bool IsQuitting() const { return m_isQuitting; }
	
	void ResetGame();

	static bool Event_Quit(EventArgs& args);

private:
	void BeginFrame();
	void Update();
	void Render() const;
	void EndFrame();
	void InitImGui(void* hwnd, ID3D11Device* device, ID3D11DeviceContext* context);

	void ShowImguiWindow();

private:
	Game* m_game = nullptr;
	bool m_isQuitting = false;

};