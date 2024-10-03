#include "entities.h"
#include "components.h"

#include <bx/readerwriter.h>
#include <bx/file.h>
#include <bx/pixelformat.h>

struct PosNormalTangentTexcoordVertex
{
	float m_x;
	float m_y;
	float m_z;
	uint32_t m_normal;
	uint32_t m_tangent;
	int16_t m_u;
	int16_t m_v;

	static void init()
	{
		ms_layout
			.begin()
			.add(max::Attrib::Position, 3, max::AttribType::Float)
			.add(max::Attrib::Normal, 4, max::AttribType::Uint8, true, true)
			.add(max::Attrib::Tangent, 4, max::AttribType::Uint8, true, true)
			.add(max::Attrib::TexCoord0, 2, max::AttribType::Int16, true, true)
			.end();
	}

	static max::VertexLayout ms_layout;
};

max::VertexLayout PosNormalTangentTexcoordVertex::ms_layout;

inline uint32_t encodeNormalRgba8(float _x, float _y = 0.0f, float _z = 0.0f, float _w = 0.0f)
{
	const float src[] =
	{
		_x * 0.5f + 0.5f,
		_y * 0.5f + 0.5f,
		_z * 0.5f + 0.5f,
		_w * 0.5f + 0.5f,
	};
	uint32_t dst;
	bx::packRgba8(&dst, src);
	return dst;
}

static PosNormalTangentTexcoordVertex s_cubeVertices[24] =
{
	{-1.0f,  1.0f,  1.0f, encodeNormalRgba8(0.0f,  0.0f,  1.0f), 0,      0,      0 },
	{ 1.0f,  1.0f,  1.0f, encodeNormalRgba8(0.0f,  0.0f,  1.0f), 0, 0x7fff,      0 },
	{-1.0f, -1.0f,  1.0f, encodeNormalRgba8(0.0f,  0.0f,  1.0f), 0,      0, 0x7fff },
	{ 1.0f, -1.0f,  1.0f, encodeNormalRgba8(0.0f,  0.0f,  1.0f), 0, 0x7fff, 0x7fff },
	{-1.0f,  1.0f, -1.0f, encodeNormalRgba8(0.0f,  0.0f, -1.0f), 0,      0,      0 },
	{ 1.0f,  1.0f, -1.0f, encodeNormalRgba8(0.0f,  0.0f, -1.0f), 0, 0x7fff,      0 },
	{-1.0f, -1.0f, -1.0f, encodeNormalRgba8(0.0f,  0.0f, -1.0f), 0,      0, 0x7fff },
	{ 1.0f, -1.0f, -1.0f, encodeNormalRgba8(0.0f,  0.0f, -1.0f), 0, 0x7fff, 0x7fff },
	{-1.0f,  1.0f,  1.0f, encodeNormalRgba8(0.0f,  1.0f,  0.0f), 0,      0,      0 },
	{ 1.0f,  1.0f,  1.0f, encodeNormalRgba8(0.0f,  1.0f,  0.0f), 0, 0x7fff,      0 },
	{-1.0f,  1.0f, -1.0f, encodeNormalRgba8(0.0f,  1.0f,  0.0f), 0,      0, 0x7fff },
	{ 1.0f,  1.0f, -1.0f, encodeNormalRgba8(0.0f,  1.0f,  0.0f), 0, 0x7fff, 0x7fff },
	{-1.0f, -1.0f,  1.0f, encodeNormalRgba8(0.0f, -1.0f,  0.0f), 0,      0,      0 },
	{ 1.0f, -1.0f,  1.0f, encodeNormalRgba8(0.0f, -1.0f,  0.0f), 0, 0x7fff,      0 },
	{-1.0f, -1.0f, -1.0f, encodeNormalRgba8(0.0f, -1.0f,  0.0f), 0,      0, 0x7fff },
	{ 1.0f, -1.0f, -1.0f, encodeNormalRgba8(0.0f, -1.0f,  0.0f), 0, 0x7fff, 0x7fff },
	{ 1.0f, -1.0f,  1.0f, encodeNormalRgba8(1.0f,  0.0f,  0.0f), 0,      0,      0 },
	{ 1.0f,  1.0f,  1.0f, encodeNormalRgba8(1.0f,  0.0f,  0.0f), 0, 0x7fff,      0 },
	{ 1.0f, -1.0f, -1.0f, encodeNormalRgba8(1.0f,  0.0f,  0.0f), 0,      0, 0x7fff },
	{ 1.0f,  1.0f, -1.0f, encodeNormalRgba8(1.0f,  0.0f,  0.0f), 0, 0x7fff, 0x7fff },
	{-1.0f, -1.0f,  1.0f, encodeNormalRgba8(-1.0f,  0.0f,  0.0f), 0,      0,      0 },
	{-1.0f,  1.0f,  1.0f, encodeNormalRgba8(-1.0f,  0.0f,  0.0f), 0, 0x7fff,      0 },
	{-1.0f, -1.0f, -1.0f, encodeNormalRgba8(-1.0f,  0.0f,  0.0f), 0,      0, 0x7fff },
	{-1.0f,  1.0f, -1.0f, encodeNormalRgba8(-1.0f,  0.0f,  0.0f), 0, 0x7fff, 0x7fff },
};

static const uint32_t s_cubeIndices[36] =
{
	 0,  2,  1,
	 1,  2,  3,
	 4,  5,  6,
	 5,  7,  6,

	 8, 10,  9,
	 9, 10, 11,
	12, 13, 14,
	13, 15, 14,

	16, 18, 17,
	17, 18, 19,
	20, 21, 22,
	21, 23, 22,
};

void calcTangents(void* _vertices, uint32_t _numVertices, max::VertexLayout _layout, const uint32_t* _indices, uint32_t _numIndices)
{
	struct PosTexcoord
	{
		float m_x;
		float m_y;
		float m_z;
		float m_pad0;
		float m_u;
		float m_v;
		float m_pad1;
		float m_pad2;
	};

	float* tangents = new float[6 * _numVertices];
	bx::memSet(tangents, 0, 6 * _numVertices * sizeof(float));

	PosTexcoord v0;
	PosTexcoord v1;
	PosTexcoord v2;

	for (uint32_t ii = 0, num = _numIndices / 3; ii < num; ++ii)
	{
		const uint32_t* indices = &_indices[ii * 3];
		uint32_t i0 = indices[0];
		uint32_t i1 = indices[1];
		uint32_t i2 = indices[2];

		max::vertexUnpack(&v0.m_x, max::Attrib::Position, _layout, _vertices, i0);
		max::vertexUnpack(&v0.m_u, max::Attrib::TexCoord0, _layout, _vertices, i0);

		max::vertexUnpack(&v1.m_x, max::Attrib::Position, _layout, _vertices, i1);
		max::vertexUnpack(&v1.m_u, max::Attrib::TexCoord0, _layout, _vertices, i1);

		max::vertexUnpack(&v2.m_x, max::Attrib::Position, _layout, _vertices, i2);
		max::vertexUnpack(&v2.m_u, max::Attrib::TexCoord0, _layout, _vertices, i2);

		const float bax = v1.m_x - v0.m_x;
		const float bay = v1.m_y - v0.m_y;
		const float baz = v1.m_z - v0.m_z;
		const float bau = v1.m_u - v0.m_u;
		const float bav = v1.m_v - v0.m_v;

		const float cax = v2.m_x - v0.m_x;
		const float cay = v2.m_y - v0.m_y;
		const float caz = v2.m_z - v0.m_z;
		const float cau = v2.m_u - v0.m_u;
		const float cav = v2.m_v - v0.m_v;

		const float det = (bau * cav - bav * cau);
		const float invDet = 1.0f / det;

		const float tx = (bax * cav - cax * bav) * invDet;
		const float ty = (bay * cav - cay * bav) * invDet;
		const float tz = (baz * cav - caz * bav) * invDet;

		const float bx = (cax * bau - bax * cau) * invDet;
		const float by = (cay * bau - bay * cau) * invDet;
		const float bz = (caz * bau - baz * cau) * invDet;

		for (uint32_t jj = 0; jj < 3; ++jj)
		{
			float* tanu = &tangents[indices[jj] * 6];
			float* tanv = &tanu[3];
			tanu[0] += tx;
			tanu[1] += ty;
			tanu[2] += tz;

			tanv[0] += bx;
			tanv[1] += by;
			tanv[2] += bz;
		}
	}

	for (uint32_t ii = 0; ii < _numVertices; ++ii)
	{
		const bx::Vec3 tanu = bx::load<bx::Vec3>(&tangents[ii * 6]);
		const bx::Vec3 tanv = bx::load<bx::Vec3>(&tangents[ii * 6 + 3]);

		float nxyzw[4];
		max::vertexUnpack(nxyzw, max::Attrib::Normal, _layout, _vertices, ii);

		const bx::Vec3 normal = bx::load<bx::Vec3>(nxyzw);
		const float    ndt = bx::dot(normal, tanu);
		const bx::Vec3 nxt = bx::cross(normal, tanu);
		const bx::Vec3 tmp = bx::sub(tanu, bx::mul(normal, ndt));

		float tangent[4];
		bx::store(tangent, bx::normalize(tmp));
		tangent[3] = bx::dot(nxt, tanv) < 0.0f ? -1.0f : 1.0f;

		max::vertexPack(tangent, true, max::Attrib::Tangent, _layout, _vertices, ii);
	}

	delete[] tangents;
}

void createCube(std::unordered_map<std::string, EntityHandle>& _entities, const char* _name, const TransformComponent& _transform, max::MeshHandle _cube, const bx::Vec3 _color)
{
	RenderComponent rc;
	rc.m_mesh = _cube;

	MaterialComponent mc;
	bx::strCopy(mc.m_diffuse.m_filepath, 1024, "");
	mc.m_diffuse.m_texture = MAX_INVALID_HANDLE;

	mc.m_diffuseFactor[0] = _color.x;
	mc.m_diffuseFactor[1] = _color.y;
	mc.m_diffuseFactor[2] = _color.z;

	bx::strCopy(mc.m_normal.m_filepath, 1024, "");
	mc.m_normal.m_texture = MAX_INVALID_HANDLE;

	mc.m_normalFactor[0] = 1.0f;
	mc.m_normalFactor[1] = 1.0f;
	mc.m_normalFactor[2] = 1.0f;

	bx::strCopy(mc.m_metallic.m_filepath, 1024, "");
	mc.m_metallic.m_texture = MAX_INVALID_HANDLE;

	mc.m_metallicFactor = 1.0f;

	bx::strCopy(mc.m_roughness.m_filepath, 1024, "");
	mc.m_roughness.m_texture = MAX_INVALID_HANDLE;

	mc.m_roughnessFactor = 1.0f;

	_entities[_name].m_handle = max::createEntity();
	max::addComponent<TransformComponent>(_entities[_name].m_handle,
		max::createComponent<TransformComponent>(_transform)
	);
	max::addComponent<RenderComponent>(_entities[_name].m_handle,
		max::createComponent<RenderComponent>(rc)
	);
	max::addComponent<MaterialComponent>(_entities[_name].m_handle,
		max::createComponent<MaterialComponent>(mc)
	);
}

void Entities::load()
{
	// Resources
	// 
	max::MeshHandle cube = max::loadMesh("meshes/cube.bin", true); // THIS DOESNT HAVE TANGENTS
	//PosNormalTangentTexcoordVertex::init();

	//calcTangents(s_cubeVertices
	//	, BX_COUNTOF(s_cubeVertices)
	//	, PosNormalTangentTexcoordVertex::ms_layout
	//	, s_cubeIndices
	//	, BX_COUNTOF(s_cubeIndices)
	//);
	//max::MeshHandle cube = max::createMesh(max::makeRef(s_cubeVertices, sizeof(s_cubeVertices))
	//	, max::makeRef(s_cubeIndices, sizeof(s_cubeIndices))
	//	, PosNormalTangentTexcoordVertex::ms_layout);

	// Player (Maya)
	m_entities["MayaPlayer"].m_handle = max::createEntity();
	max::addComponent<CameraComponent>(m_entities["MayaPlayer"].m_handle,
		max::createComponent<CameraComponent>(CameraComponent(0, 45.0f))
	);

	// Player
	CameraComponent cc = CameraComponent(1, 45.0f);
	cc.m_position = { 15.0f, 5.f, 0.0 };
	m_entities["Player"].m_handle = max::createEntity();
	max::addComponent<CameraComponent>(m_entities["Player"].m_handle,
		max::createComponent<CameraComponent>(cc)
	);
	
	createCube(m_entities, 
		"Floor", 
		{ {0.0f, -0.25, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {5.0f, 0.5f, 5.0f} },
		cube,
		{1.0f, 1.0f, 1.0f});

	createCube(m_entities,
		"Ciel",
		{ {0.0f, 10.25, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {5.0f, 0.5f, 5.0f} },
		cube,
		{ 1.0f, 1.0f, 1.0f });

	createCube(m_entities,
		"Back",
		{ {-5.5f, 5.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {0.5f, 4.75f, 5.0f} },
		cube,
		{ 1.0f, 1.0f, 1.0f });

	createCube(m_entities,
		"Right",
		{ {0.0f, 5.0f, -5.5f}, {0.0f, 0.0f, 0.0f, 1.0f}, {5.0f, 4.75f, 0.5f} },
		cube,
		{ 0.0f, 1.0f, 0.0f });

	createCube(m_entities,
		"Left",
		{ {0.0f, 5.0f, 5.5f}, {0.0f, 0.0f, 0.0f, 1.0f}, {5.0f, 4.75f, 0.5f} },
		cube,
		{ 1.0f, 0.0f, 0.0f });

	createCube(m_entities,
		"Left Cube",
		{ {0.5f, 2.25f, 2.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {1.5f, 3.0f, 1.5f} },
		cube,
		{ 1.0f, 1.0f, 1.0f });

	createCube(m_entities,
		"Right Cube",
		{ {0.5f, 0.75f, -2.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {1.5f, 1.5f, 1.5f} },
		cube,
		{ 1.0f, 1.0f, 1.0f });
}

void Entities::unload()
{
	if (m_entities.size() <= 0)
	{
		return;
	}

	for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
	{
		if (isValid(it->second.m_handle))
		{
			if (RenderComponent* rc = max::getComponent<RenderComponent>(it->second.m_handle))
			{
				max::destroy(rc->m_mesh);
				rc->m_mesh = MAX_INVALID_HANDLE;
			}
			if (MaterialComponent* rc = max::getComponent<MaterialComponent>(it->second.m_handle))
			{
				if (isValid(rc->m_diffuse.m_texture))
				{
					max::destroy(rc->m_diffuse.m_texture);
					rc->m_diffuse.m_texture = MAX_INVALID_HANDLE;
				}

				if (isValid(rc->m_normal.m_texture))
				{
					max::destroy(rc->m_normal.m_texture);
					rc->m_normal.m_texture = MAX_INVALID_HANDLE;
				}

				if (isValid(rc->m_roughness.m_texture))
				{
					max::destroy(rc->m_roughness.m_texture);
					rc->m_roughness.m_texture = MAX_INVALID_HANDLE;
				}

				if (isValid(rc->m_metallic.m_texture))
				{
					max::destroy(rc->m_metallic.m_texture);
					rc->m_metallic.m_texture = MAX_INVALID_HANDLE;
				}
			}
			max::destroy(it->second.m_handle);
			it->second.m_handle = MAX_INVALID_HANDLE;
		}
	}

	m_entities.clear();
}

void Entities::update()
{
}