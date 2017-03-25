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
	minx = 0;	// � ����������� ��������
	maxx = 0;	// � ����������� ��������
	miny = 0;	// � ����������� ��������
	maxy = 0;	// � ����������� ��������
}

}
}