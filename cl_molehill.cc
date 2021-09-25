#include "globals.h"

#define START_X1 (START_X - TUNNEL_HALF)
#define START_X2 (START_X + TUNNEL_HALF)

void cl_molehill::reset()
{
	int x1;
	int x2;
	int width;

	width  = 50 + random() % 100;

	do
	{
		x1 = random() % (SCR_SIZE - width);
		x2 = x1 + width;
	} while(!((x1 <= START_X1 && x2 <= START_X1) || 
	          (x1 >= START_X2 && x2 >= START_X2)));

	vertex[0].x = x1;
	vertex[0].y = PLAY_AREA_TOP;

	vertex[1].x = (x1 + x2) / 2;
	vertex[1].y = PLAY_AREA_TOP - (5 + random() % 15);

	vertex[2].x = x2;
	vertex[2].y = PLAY_AREA_TOP;
}




void cl_molehill::draw()
{
	memcpy(tmp_points,vertex,sizeof(XPoint) * 3);
	drawOrFillPolygon(ground_colour,0,tmp_points,3,FILL);
}
