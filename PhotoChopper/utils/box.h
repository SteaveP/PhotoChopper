#pragma once

namespace pc
{

namespace utils
{

struct Box
{
	float xScale;
	float yScale;

	float angle;
	int	  minx;	// в уменьшенном масштабе
	int	  maxx;	// в уменьшенном масштабе
	int	  miny;	// в уменьшенном масштабе
	int	  maxy;	// в уменьшенном масштабе

	Box();
	void Reset();

	inline int width()  { return maxx - minx; }
	inline int height() { return maxy - miny; }
};

}
}