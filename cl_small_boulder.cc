#include "globals.h"

cl_small_boulder::cl_small_boulder(cl_boulder *own):
	cl_rock(TYPE_SMALL_BOULDER)
{
	owner = own;
}




void cl_small_boulder::activate()
{
	cl_rock::activate();
	col = COL_GREYISH;
	reset();
}




void cl_small_boulder::run()
{
	if (++stage_cnt < 0) return;

	y_add += 0.4;
	x += x_add;
	y += y_add;

	// Reset when we've gone off screen if boulder being eaten. 
	if (y > SCR_SIZE && owner->stage == STAGE_BEING_EATEN) 
	{
		reset();
		stage_cnt = 0;
	}
}




void cl_small_boulder::reset()
{
	x = owner->x;
	y = owner->y;
	x_add = (random() % 5) - 2;
	y_add = -5 - random() % 5;

	// Add small delay to start so boulders don't all start flying at same
	// if we're being eaten
	if (owner->stage == STAGE_BEING_EATEN) stage_cnt = -(random() % 50);
}
