#pragma once

#include <bx/uint32_t.h>

struct Month
{
	enum Enum
	{
		January,
		February,
		March,
		April,
		May,
		June,
		July,
		August,
		September,
		October,
		November,
		December
	};
};

struct SkySettings
{
	Month::Enum m_month;
	float m_speed;
};

void skyCreate(SkySettings* _settings);

void skyDestroy();

void skyUpdate();