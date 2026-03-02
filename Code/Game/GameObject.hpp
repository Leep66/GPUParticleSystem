#pragma once
#include "Game/Entity.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include <string>
#include <vector>

class Texture;
struct Rgba8;
class VertexBuffer;
class IndexBuffer;


enum class ObjectType
{
	NONE = -1,
	CUBE,
	SPHERE,
	CYLINDER,
	DISC,
	TORUS,
	COUNT
};

class GameObject : public Entity
{
public:
	GameObject(Game* owner);
	virtual ~GameObject();

	void Update(float deltaSeconds) override;
	void Render() const override;

	void SetScale(const Vec3& scale);
	
	const Vec3& GetScale() const { return m_scale; }

	void SetName(const std::string& name) { m_name = name; }
	const std::string& GetName() const { return m_name; }

	void SetAcceleration(const Vec3& acceleration) { m_acceleration = acceleration; }
	const Vec3& GetAcceleration() const { return m_acceleration; }

	void ApplyForce(const Vec3& force) { m_acceleration += force; }

	void Move(const Vec3& displacement) { m_position += displacement; }
	void Rotate(const EulerAngles& rotation) { m_orientation += rotation; }
	void ScaleBy(const Vec3& scaleFactor) { m_scale = m_scale * scaleFactor; }


	Vec3 GetForwardNormal() const;
	Vec3 GetLeftNormal() const;
	Vec3 GetUpNormal() const;

	void InitializeVertsFromFile(const char* filePath);
	void InitializeVertsFromPreset(GameObject const& preset);
	void InitializeVertsFromType(ObjectType type);

	void CreateOrUpdateObjectBuffers();
	Texture* m_diffuseTexture = nullptr;
	Texture* m_normalTexture = nullptr;
	Texture* m_specGlossEmitTexture = nullptr;
	std::string m_name;

protected:
	std::vector<Vertex_PCUTBN> m_vertexes;
	std::vector<unsigned int> m_indices;

	VertexBuffer* m_vbo = nullptr;
	IndexBuffer* m_ibo = nullptr;

	Vec3 m_acceleration = Vec3::ZERO;
};