#include "commonUtils.h"

utils::img::Color::Color(int64_t color) :
	r{ uint8_t((color >> 16) & 0xFF) },
	g{ uint8_t((color >> 8) & 0xFF) },
	b{ uint8_t(color & 0xFF) }
{
	/* empty */
}

int32_t utils::img::Color::ToInt32() const
{
	return r << 16 | g << 8 | b;
}

utils::img::Point::Point(int64_t x, int64_t y, int64_t color) :
	x{ (int16_t)x },
	y{ (int16_t)y },
	color{ color }
{
	/* empty */
}

utils::img::Point::Point(int64_t x, int64_t y, Color color) :
	x{ (int16_t)x },
	y{ (int16_t)y },
	color{ color }
{
	/* empty */
}

utils::img::Update::Update(const Point& point, uint64_t timestamp) :
	point{ point },
	timestamp{ timestamp }
{
	/* empty */
}