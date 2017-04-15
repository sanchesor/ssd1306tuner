#ifndef __MOVING_RECT_H
#define __MOVING_RECT_H

#include "stm32f10x.h"

typedef struct MovingRect
{
	double x1;
	double y1;
	double x2;
	double y2;
	double dx;
	double dy;
	uint8_t isFilled;
} MovingRect;

void MoveRect(MovingRect* r);

#endif // __MOVING_RECT_H
