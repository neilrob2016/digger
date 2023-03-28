/// This draws the explosions for the game objects

#include "globals.h"

/*** Constructor ***/
cl_explosion::cl_explosion(cl_object *own)
{
	owner = own;
	col_add = 0;
	size_sub = 0;

	// Set values which never change
	switch(owner->type)
	{
	case TYPE_PLAYER:
		cnt = 100;
		size_sub = 0.2;
		break;

	case TYPE_BALL:
		cnt = 16;
		speed = 20;
		size = 15;
		break;

	case TYPE_SPOOKY:
		cnt = 8;
		speed = 5;
		size = 10;
		break;

	case TYPE_GRUBBLE:
		cnt = 20;
		speed = 15;
		size_sub = 0.15;
		break;

	case TYPE_WURMAL:
	case TYPE_SPIKY:
		cnt = 50;
		speed = 5;
		size_sub = 0.25;
		break;

	default:
		assert(0);
	}

	x = new double[cnt];
	y = new double[cnt];
	x_add = new double[cnt];
	y_add = new double[cnt];
}




/*** Set up all the important stuff ***/
void cl_explosion::activate()
{
	double ang = 0;
	double ang_inc = 360 / cnt;
	int i;

	start_x = owner->x;
	start_y = owner->y;
	rev = false;

	// Reset values which do change
	switch(owner->type)
	{
	case TYPE_PLAYER:
		col = COL_YELLOW;
		size = 20;
		col_add = -0.25;
		break;

	case TYPE_BALL:
		col = COL_GREEN;
		col_add = 1;
		break;

	case TYPE_SPOOKY:
		col = COL_TURQUOISE;
		col_add = 1;
		break;

	case TYPE_GRUBBLE:
	case TYPE_SPIKY:
		col = COL_PURPLE;
		col_add = 0.5;
		size = 15;
		break;

	case TYPE_WURMAL:
		col = COL_GREEN;
		size = 20;
		col_add = 0.5;
		break;

	default:
		assert(0);
	}

	for(i=0;i < cnt;++i)
	{
		x[i] = start_x;
		y[i] = start_y;

		switch(owner->type)
		{
		case TYPE_PLAYER:
		case TYPE_GRUBBLE:
		case TYPE_WURMAL:
		case TYPE_SPIKY:
			// Random explosion
			x_add[i] = SIN(ang) * (random() % 20) + 1;
			y_add[i] = COS(ang) * (random() % 20) + 1;
			break;

		default:
			// Symmetric explosion
			x_add[i] = SIN(ang) * speed;
			y_add[i] = COS(ang) * speed;
		}
		ang += ang_inc;
	}
}




/*** Set up to go backwards. For ball only at the moment. ***/
void cl_explosion::reverse()
{
	int i;

	assert(owner->type == TYPE_BALL);

	rev = true;
	col_add = -col_add;

	for(i=0;i < cnt;++i)
	{
		x_add[i] = -x_add[i];
		y_add[i] = -y_add[i];
	}
}




/*** Do everything in one function ***/
void cl_explosion::runAndDraw()
{
	for(int i=0;i < cnt;++i)
	{
		drawOrFillCircle((int)col,0,size,x[i],y[i],FILL);
		x[i] += x_add[i];
		y[i] += y_add[i];

		/* Need to keep adjusting centre because of movement of
		   owner. Otherwise materialise centre coule be a long way 
		   from them */
		if (rev)
		{
			x[i] = x[i] - start_x + owner->x;
			y[i] = y[i] - start_y + owner->y;
		}
	}

	// Do colour change
	col += col_add;
	switch(owner->type)
	{
	case TYPE_SPOOKY:
		if (col == COL_BLUE)
		{
			col = COL_BLUE2;
			col_add = -1;
		}
		break;

	case TYPE_PLAYER:
		// Go from red to black
		if (col <= COL_RED) col = COL_RED2;
		break;

	default:
		break;
	}

	// In case owner has moved
	if (rev)
	{
		start_x = owner->x;
		start_y = owner->y;
	}
	if (size_sub && size > 1) size -= size_sub;
}
