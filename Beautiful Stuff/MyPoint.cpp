#include "MyPoint.h"
bool MyPoint::operator==(const MyPoint& p)
{
	return r == p.r && g == p.g && b == p.b;
}