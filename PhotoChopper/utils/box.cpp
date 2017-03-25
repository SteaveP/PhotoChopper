#include "box.h"

namespace pc
{
namespace utils
{

Box::Box()
{
	Reset();
}

void Box::Reset()
{
	xScale = 0.2f;
	yScale = 0.2f;

	angle = 0;	//
	minx = 0;	// в уменьшенном масштабе
	maxx = 0;	// в уменьшенном масштабе
	miny = 0;	// в уменьшенном масштабе
	maxy = 0;	// в уменьшенном масштабе
}

}
}