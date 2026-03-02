#pragma once
#include "Game/Entity.hpp"
#include "Engine/Renderer/Camera.hpp"

class Player : public Entity
{
public:
	Player(Game* owner);
	virtual ~Player();

	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

	void HandleInput(float deltaSeconds);
	void MoveByKeyboard(float deltaSeconds);
	void RotateByKeyboard(float deltaSeconds);

	void MoveByController(float deltaSeconds);
	void RotateByController(float deltaSeconds);

	virtual void SetPosition(const Vec3& position);

	Camera GetCamera() const { return m_camera; }
	Vec3 GetPosition() const { return m_position; }
	EulerAngles GetOrientation() const { return m_orientation; }
	Vec3 GetFwdNormal() const;
	Vec3 GetLeftNormal() const;
	Vec3 GetUpNormal() const;
	

private:
	
	float m_moveSpeed = 2.0f;
	float m_sprintMultiplier = 10.0f;
	float m_sensitivity = 0.125f; 
	float m_rollSpeed = 90.0f;
	Camera m_camera;
};