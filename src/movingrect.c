#include "movingrect.h"
#include "libssd1306.h"


void MoveRect(MovingRect* r)
{
	int max_x = 127;
	int max_y = 63;


	r->x1 += r->dx;
	r->x2 += r->dx;
	r->y1 += r->dy;
	r->y2 += r->dy;

	if(r->dx > 0 && r->x1 > max_x)
		r->x1 -= max_x;

	if(r->dx > 0 && r->x2 > max_x)
		r->x2 -= max_x;
	
	if(r->dx < 0 && r->x1 < 0)
		r->x1 += max_x;

	if(r->dx < 0 && r->x2 < 0)
		r->x2 += max_x;
	
	if(r->dy > 0 && r->y1 > max_y)
		r->y1 -= max_y;

	if(r->dy > 0 && r->y2 > max_y)
		r->y2 -= max_y;

	if(r->dy < 0 && r->y1 < 0)
		r->y1 += max_y;

	if(r->dy < 0 && r->y2 < 0)
		r->y2 += max_y;

	
	int x1,x2,y1,y2;
	int xx1,xx2,yy1,yy2;
	int is2rect = 0;
	
	if(r->x1 < r->x2)
	{
		x1 = r->x1;
		x2 = r->x2;
		xx1 = x1;
		xx2 = x2;
	}
	else
	{
		x1 = r->x1;
		x2 = max_x;
		xx1 = 0;
		xx2 = r->x2;
		is2rect = 1;
	}
	
	if(r->y1 < r->y2)
	{
		y1 = r->y1;
		y2 = r->y2;
		yy1 = y1;
		yy2 = y2;
	}
	else
	{
		y1 = r->y1;
		y2 = max_y;
		yy1 = 0;
		yy2 = r->y2;
		is2rect = 1;
	}	
	
	if(r->isFilled == 1)
	{		
		fillRect(x1, y1, x2, y2);
		if(is2rect == 1)
			fillRect(xx1, yy1, xx2, yy2);
	}
	else
	{
		drawRect(x1, y1, x2, y2);
		if(is2rect == 1)
			drawRect(xx1, yy1, xx2, yy2);
	}
}
