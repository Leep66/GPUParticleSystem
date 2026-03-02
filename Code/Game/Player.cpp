#include "Game/Player.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Window/Window.hpp"
#include "Game/Game.hpp"

extern DevConsole* g_theDevConsole;
extern InputSystem* g_theInput;
extern Window* g_theWindow;

Player::Player(Game* owner)
	:Entity(owner)
{
	Mat44 mat;
	mat.SetIJK3D(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
	m_camera.SetCameraToRenderTransform(mat);

	float fov = 60.f;
	float aspect = g_theWindow->GetAspect();
	float zNear = 0.1f;
	float zFar = 1000.f;
	m_camera.SetMode(Camera::eMode_Perspective);
	m_camera.SetPerspectiveView(aspect, fov, zNear, zFar);

	m_orientation = EulerAngles(0.f, 0.f, 0.f);
	m_camera.SetOrientation(m_orientation);

	m_position = Vec3::ZERO;
	m_camera.SetPosition(m_position);

}

Player::~Player()
{
}

void Player::Update(float deltaSeconds)
{
	deltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();
	if (g_theDevConsole->IsOpen()) return;

	HandleInput(deltaSeconds);

}


void Player::Render() const
{
}

void Player::HandleInput(float deltaSeconds)
{

	if (g_theInput->WasKeyJustPressed('H'))
	{
		m_position = Vec3();
		m_orientation = EulerAngles();
	}

	MoveByKeyboard(deltaSeconds);
	RotateByKeyboard(deltaSeconds);

	MoveByController(deltaSeconds);
	RotateByController(deltaSeconds);
}

void Player::MoveByKeyboard(float deltaSeconds)
{
	Vec3 cameraFwd, cameraLeft, cameraUp;

	m_orientation.GetAsVectors_IFwd_JLeft_KUp(cameraFwd, cameraLeft, cameraUp);

	Vec3 moveDirection;

	cameraUp = Vec3(0, 0, 1);

	if (g_theInput->IsKeyDown('W')) moveDirection += cameraFwd;
	if (g_theInput->IsKeyDown('S')) moveDirection -= cameraFwd;

	if (g_theInput->IsKeyDown('A')) moveDirection += cameraLeft;
	if (g_theInput->IsKeyDown('D')) moveDirection -= cameraLeft;

	if (g_theInput->IsKeyDown('Q')) moveDirection -= cameraUp;
	if (g_theInput->IsKeyDown('E')) moveDirection += cameraUp;

	if (moveDirection != Vec3())
	{
		moveDirection = moveDirection.GetNormalized();
	}

	float speed = m_moveSpeed;
	if (g_theInput->IsKeyDown(KEYCODE_SHIFT))
	{
		speed *= m_sprintMultiplier;
	}

	m_position += moveDirection * speed * deltaSeconds;
	m_camera.SetPosition(m_position);
}

void Player::RotateByKeyboard(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	Vec2 mouseDelta = g_theInput->GetCursorClientDelta();

	m_orientation.m_yawDegrees -= mouseDelta.x * m_sensitivity;

	m_orientation.m_pitchDegrees += mouseDelta.y * m_sensitivity;

	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.0f, 85.0f);

	/*float rollDelta = 0.0f;
	if (g_theInput->IsKeyDown('Q')) rollDelta -= m_rollSpeed * deltaSeconds;
	if (g_theInput->IsKeyDown('E')) rollDelta += m_rollSpeed * deltaSeconds;

	m_orientation.m_rollDegrees += rollDelta;

	m_orientation.m_rollDegrees = GetClamped(m_orientation.m_rollDegrees, -45.0f, 45.0f);*/

	m_camera.SetOrientation(m_orientation);
}

void Player::MoveByController(float deltaSeconds)
{
	XboxController controller = g_theInput->GetController(0);
	Vec3 cameraFwd, cameraLeft, cameraUp;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(cameraFwd, cameraLeft, cameraUp);

	Vec3 moveDirection;

	cameraUp = Vec3(0, 0, 1);

	Vec2 leftStick = controller.GetLeftStick().GetPosition();
	moveDirection += cameraFwd * leftStick.y;
	moveDirection -= cameraLeft * leftStick.x;

	if (controller.IsButtonDown(XBOX_BUTTON_LB))
	{
		moveDirection -= cameraUp;
	}

	if (controller.IsButtonDown(XBOX_BUTTON_RB))
	{
		moveDirection += cameraUp;
	}

	if (moveDirection != Vec3())
	{
		moveDirection = moveDirection.GetNormalized();
	}

	float speed = m_moveSpeed;
	if (controller.IsButtonDown(XBOX_BUTTON_A))
	{
		speed *= m_sprintMultiplier;
	}

	m_position += moveDirection * speed * deltaSeconds;
	m_camera.SetPosition(m_position);
}

void Player::RotateByController(float deltaSeconds)
{
	XboxController controller = g_theInput->GetController(0);
	Vec2 rightStick = controller.GetRightStick().GetPosition();
	m_orientation.m_yawDegrees -= rightStick.x * m_sensitivity;
	m_orientation.m_pitchDegrees -= rightStick.y * m_sensitivity;

	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.0f, 85.0f);

	float leftTrigger = controller.GetLeftTrigger();
	float rightTrigger = controller.GetRightTrigger();
	float rollDelta = (rightTrigger - leftTrigger) * m_rollSpeed * deltaSeconds;
	m_orientation.m_rollDegrees += rollDelta;

	m_orientation.m_rollDegrees = GetClamped(m_orientation.m_rollDegrees, -45.0f, 45.0f);

	m_camera.SetOrientation(m_orientation);
}


void Player::SetPosition(const Vec3& position)
{
	m_position = position;
	m_camera.SetPosition(m_position);
}

Vec3 Player::GetFwdNormal() const
{
	Vec3 fwd, left, up;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(fwd, left, up);

	return fwd;
}

Vec3 Player::GetLeftNormal() const
{
	Vec3 fwd, left, up;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(fwd, left, up);

	return left;
}

Vec3 Player::GetUpNormal() const
{
	Vec3 fwd, left, up;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(fwd, left, up);

	return up;
}





