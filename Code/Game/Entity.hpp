#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Rgba8.hpp"

class Game;

class Entity
{
public:
	Entity(Game* owner);
	virtual ~Entity();

	virtual void Update(float deltaSeconds) = 0;
	virtual void Render() const = 0;

	virtual Mat44 GetModelToWorldTransform() const;

	bool IsActive() { return m_isActive; }
	bool IsVisible() { return m_isVisible; }

	virtual void SetPosition(const Vec3& position) { m_position = position; }
	const Vec3& GetPosition() const { return m_position; }

	void SetVelocity(const Vec3& velocity) { m_velocity = velocity; }
	const Vec3& GetVelocity() const { return m_velocity; }

	void SetOrientation(const EulerAngles& orientation) { m_orientation = orientation; }
	const EulerAngles& GetOrientation() const { return m_orientation; }

	void SetAngularVelocity(const EulerAngles& angularVelocity) { m_angularVelocity = angularVelocity; }
	const EulerAngles& GetAngularVelocity() const { return m_angularVelocity; }

	void SetColor(const Rgba8& color) { m_color = color; }
	const Rgba8& GetColor() const { return m_color; }

	void SetActive(bool active) { m_isActive = active; }
	bool IsActive() const { return m_isActive; }

	void SetVisible(bool visible) { m_isVisible = visible; }
	bool IsVisible() const { return m_isVisible; }

public:
	Game* m_game = nullptr;
	Vec3 m_position = Vec3(0.f, 0.f, 0.f);
	Vec3 m_velocity;
	Vec3 m_scale = Vec3(1.f, 1.f, 1.f);
	EulerAngles m_orientation = EulerAngles(0.f, 0.f, 0.f);
	EulerAngles m_angularVelocity;
	Rgba8 m_color = Rgba8::WHITE;

	bool m_isActive = true;
	bool m_isVisible = true;
};