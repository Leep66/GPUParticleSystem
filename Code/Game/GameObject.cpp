#include "Game/GameObject.hpp"
#include "Game/Game.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/StaticMesh.hpp"
#include "Engine/Core/StaticMeshUtils.hpp"
#include <d3d11.h>

extern Renderer* g_theRenderer;

GameObject::GameObject(Game* owner)
	: Entity(owner)
	, m_name("GameObject")
{
}

GameObject::~GameObject()
{
	if (m_vbo)
	{
		delete m_vbo;
		m_vbo = nullptr;
	}

	if (m_ibo)
	{
		delete m_ibo;
		m_ibo = nullptr;
	}
}

void GameObject::Update(float deltaSeconds)
{
	if (!m_isActive) return;

	m_velocity += m_acceleration * deltaSeconds;
	m_position += m_velocity * deltaSeconds;

	m_orientation += m_angularVelocity * deltaSeconds;

	m_orientation.m_pitchDegrees = fmodf(m_orientation.m_pitchDegrees, 360.0f);
	m_orientation.m_yawDegrees = fmodf(m_orientation.m_yawDegrees, 360.0f);
	m_orientation.m_rollDegrees = fmodf(m_orientation.m_rollDegrees, 360.0f);

	m_acceleration = Vec3::ZERO;
}

void GameObject::Render() const
{
	if (!m_isVisible || !g_theRenderer || !m_vbo || !m_ibo) return;

	g_theRenderer->BindTexture(m_diffuseTexture, 2);
	g_theRenderer->BindTexture(m_normalTexture, 3);
	g_theRenderer->BindTexture(m_specGlossEmitTexture, 4);

	g_theRenderer->SetModelConstants(GetModelToWorldTransform(), m_color);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	if (m_color.a < 255)
	{
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);

	}
	else
	{
		g_theRenderer->SetBlendMode(BlendMode::Blend_OPAQUE);
	}
	g_theRenderer->SetStatesIfChanged();

	g_theRenderer->CopyCPUToGPU(m_vbo, m_ibo, m_vertexes.data(), m_indices.data(), (int)m_vertexes.size(), (int)m_indices.size());

	g_theRenderer->DrawIndexedVertexBuffer(m_vbo, m_ibo, (int)m_indices.size());

	g_theRenderer->SetModelConstants(Mat44(), Rgba8::WHITE);


}

void GameObject::SetScale(const Vec3& scale)
{
	m_scale = scale;
}


Vec3 GameObject::GetForwardNormal() const
{
	return m_orientation.GetForwardNormal();
}

Vec3 GameObject::GetLeftNormal() const
{
	return m_orientation.GetLeftNormal();
}

Vec3 GameObject::GetUpNormal() const
{
	return m_orientation.GetUpNormal();
}

void GameObject::InitializeVertsFromFile(const char* filePath)
{
	StaticMesh mesh(m_name, filePath);

	m_vertexes = mesh.m_vertices;
	m_indices = mesh.m_indices;

	CreateOrUpdateObjectBuffers();
}

void GameObject::InitializeVertsFromPreset(GameObject const& preset)
{
	m_vertexes = preset.m_vertexes;
	m_indices = preset.m_indices;
	m_diffuseTexture = preset.m_diffuseTexture;
	m_normalTexture = preset.m_normalTexture;
	m_specGlossEmitTexture = preset.m_specGlossEmitTexture;

	CreateOrUpdateObjectBuffers();
}

void GameObject::InitializeVertsFromType(ObjectType type)
{
	switch (type)
	{
	case ObjectType::NONE:
		break;
	case ObjectType::CUBE:
		AddVertsForAABB3D(m_vertexes, m_indices, AABB3(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f)));
		break;
	case ObjectType::SPHERE:
		AddVertsForSphere3D(m_vertexes, m_indices, Vec3(0.f, 0.f, 0.f), 0.5f);
		break;
	case ObjectType::CYLINDER:
		AddVertsForCylinderZ3D(m_vertexes, m_indices, Vec2(0.f, 0.f), FloatRange(0.f, 1.f), 0.5f, 16, Rgba8::WHITE);
		break;
	case ObjectType::DISC:
		AddVertsForDiscZ3D(m_vertexes, m_indices, Vec2(0.f, 0.f), /*z*/ 0.f, /*radius*/ 0.5f,
			/*numSlices*/ 64, Rgba8::WHITE, AABB2(Vec2(0.f, 0.f), Vec2(1.f, 1.f)),
			/*isTopFace*/ true);
		AddVertsForDiscZ3D(m_vertexes, m_indices, Vec2(0.f, 0.f), /*z*/ 0.f, /*radius*/ 0.5f,
			/*numSlices*/ 64, Rgba8::WHITE, AABB2(Vec2(0.f, 0.f), Vec2(1.f, 1.f)),
			/*isTopFace*/ false);
		break;
	case ObjectType::TORUS:
	{
		float majorR = 0.6f;
		float tubeDia = 0.10f;
		int   majorSeg = 96;
		int   tubeSeg = 24;

		AddVertsForTorus3D(
			m_vertexes,
			m_indices,
			Vec3(0.f, 0.f, 0.f),
			Vec3(0.f, 0.f, 1.f),
			majorR,
			tubeDia,
			Rgba8(255, 255, 255, 255),
			majorSeg,
			tubeSeg
		);

		
	}
	break;
	case ObjectType::COUNT:
		break;
	default:
		break;
	}

	CreateOrUpdateObjectBuffers();
}

void GameObject::CreateOrUpdateObjectBuffers()
{
	if (!g_theRenderer) return;
	if ((int)m_vertexes.size() == 0 || (int)m_indices.size() == 0) return;

	const size_t vbStride = sizeof(Vertex_PCUTBN);
	const size_t vertexCnt = m_vertexes.size();
	const size_t vbBytes = vertexCnt * vbStride;

	const size_t ibStride = sizeof(unsigned int);
	const size_t indexCnt = m_indices.size();
	const size_t ibBytes = indexCnt * ibStride;

	SAFE_DELETE(m_vbo);
	SAFE_DELETE(m_ibo);

	m_vbo = g_theRenderer->CreateVertexBuffer((unsigned int)vbBytes, (unsigned int)vbStride);
	m_ibo = g_theRenderer->CreateIndexBuffer((unsigned int)ibBytes, (unsigned int)ibStride);
}

