#include "sky.h"

#include <max/max.h>

#include "components.h"

struct SkySystem
{
	void create(SkySettings* _settings)
	{
		m_settings = _settings;

		m_latitude = 50.0f;
		m_eclipticObliquity = bx::toRad(23.4f);
		m_delta = 0.0f;
	}

	void destroy()
	{}

	void update()
	{
		// Calculate time.
		int64_t now = bx::getHPCounter();
		static int64_t last = now;
		const int64_t frameTime = now - last;
		last = now;
		const double freq = double(bx::getHPFrequency());
		const float deltaTime = float(frameTime / freq);
		m_time += m_settings->m_speed * deltaTime;
		m_time = bx::mod(m_time, 24.0f);

		// Calculate sun orbit.
		const float day = 30.0f * float(m_settings->m_month) + 15.0f;
		float lambda = 280.46f + 0.9856474f * day;
		lambda = bx::toRad(lambda);
		m_delta = bx::asin(bx::sin(m_eclipticObliquity) * bx::sin(lambda));

		// Compute sun position based on Earth's orbital elements.
		// https://nssdc.gsfc.nasa.gov/planetary/factsheet/earthfact.html
		const float hour = m_time / 12.0f;
		const float latitude = bx::toRad(m_latitude);
		const float hh = hour * bx::kPi / 12.0f;
		const float azimuth = bx::atan2(
			bx::sin(hh), 
			bx::cos(hh) * bx::sin(latitude) - bx::tan(m_delta) * bx::cos(latitude)
		);
		const float altitude = bx::asin(
			bx::sin(latitude) * bx::sin(m_delta) + bx::cos(latitude) * bx::cos(m_delta) * bx::cos(hh)
		);
		const bx::Quaternion rot0 = bx::fromAxisAngle({0.0f, 1.0f, 0.0f}, -azimuth); 
		const bx::Vec3 dir = bx::mul({1.0f, 0.0f, 0.0f}, rot0); // North
		const bx::Vec3 uxd = bx::cross({ 0.0f, 1.0f, 0.0f }, dir);
		const bx::Quaternion rot1 = bx::fromAxisAngle(uxd, altitude);
		const bx::Vec3 lightDir = bx::mul(dir, rot1);
		m_lightDir[0] = lightDir.x;
		m_lightDir[1] = lightDir.y;
		m_lightDir[2] = lightDir.z;

		// Update directional light in scene.
		max::System<SkyComponent> sky;
		sky.each(1, [](max::EntityHandle _entity, void* _userData)
		{
			SkySystem* system = (SkySystem*)_userData;
			SkyComponent* sc = max::getComponent<SkyComponent>(_entity);
			sc->m_direction = { system->m_lightDir[0], system->m_lightDir[1], system->m_lightDir[2] };
			sc->m_color = { 1.0f, 1.0f, 1.0f }; // @todo

		}, this);

		BX_ASSERT(sky.m_num <= 1, "Only supports one directional light per scene.");
	}

	SkySettings* m_settings;

	float m_lightDir[3];
	float m_lightCol[3];
	int64_t m_timeOffset;
	float m_time;
	float m_turbidity;
	float m_latitude;
	float m_eclipticObliquity;
	float m_delta;
};

static SkySystem* s_ctx = NULL;

void skyCreate(SkySettings* _settings)
{
	s_ctx = BX_NEW(max::getAllocator(), SkySystem);
	s_ctx->create(_settings);
}

void skyDestroy()
{
	s_ctx->destroy();
	bx::deleteObject<SkySystem>(max::getAllocator(), s_ctx);
	s_ctx = NULL;
}

void skyUpdate()
{
	s_ctx->update();
}

