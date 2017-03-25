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
	int	  minx;	// � ����������� ��������
	int	  maxx;	// � ����������� ��������
	int	  miny;	// � ����������� ��������
	int	  maxy;	// � ����������� ��������

	Box();
	void Reset();

	inline int width()  { return maxx - minx; }
	inline int height() { return maxy - miny; }
};

}
}