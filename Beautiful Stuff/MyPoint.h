#pragma once
#include <stdint.h>
struct MyPoint
{
	uint8_t r, g, b;
	bool operator==(const MyPoint& p);
};