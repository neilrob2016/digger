#include "globals.h"

#define SLOW_SPEED 3

cl_ball::cl_ball(): cl_object(TYPE_BALL)
{
	diam = 20;
	radius = 10;
	explode = new cl_explosion(this);
}


//////////////////////////// OVERLOADED VIRTUALS /////////////////////////////

void cl_ball::activate()
{
	setStage(STAGE_RUN);
	x = player->ball_x;
	y = player->ball_y;

	if (player->superball)
	{
		// Means it'll kill 2 monsters before it goes back to normal
		// which means it can kill 3 in total.
		switch(level)
		{
		case 1:
		case 2:
		case 3:
			assert(0);

		case 4:
		case 5:
			superball_cnt = 2;
			break;

		case 6:
		case 7:
			superball_cnt = 3;
			break;

		default:
			superball_cnt = 4;
			break;
		}
		speed = 6;
	}
	else
	{
		superball_cnt = false;
		speed = SLOW_SPEED;
	}

	switch(player->facing_dir)
	{
	case DIR_LEFT:
		x_mult = -1;
		y_mult = 1;
		break;
	case DIR_RIGHT:
		x_mult = 1;
		y_mult = 1;
		break;
	case DIR_UP:
		x_mult = 1;
		y_mult = -1;
		break;
	case DIR_DOWN:
		x_mult = -1;
		y_mult = 1;
		break;
	default:
		assert(0);
	}

	explode_time = level < 8 ? 200 - level * 20 : 40;
}




void cl_ball::run()
{
	int sx;
	int sy;
	int ex;
	int ey;
	int x2;
	int y2;
	int add;

	++stage_cnt;

	switch(stage)
	{
	case STAGE_RUN:
		if (stage_cnt == 200) setStage(STAGE_INACTIVE);
		break;

	case STAGE_EXPLODE:
		if (stage_cnt == explode_time || player->superball)
		{
			explode_time = stage_cnt;
			setStage(STAGE_MATERIALISE);
			explode->reverse();
		}
		return;

	case STAGE_MATERIALISE:
		x = player->x;
		y = player->y;
		if (stage_cnt == explode_time - 35)
			playFGSound(SND_BALL_RETURN);
		else
		if (stage_cnt == explode_time)
			setStage(STAGE_INACTIVE);
		return;

	default:
		assert(0);
	}

	// sx,sy = start point of movement in direction of travel
	// ex,ey = end point of next movement
	sx = (int)x + x_mult * radius;
	sy = (int)y + y_mult * radius;
	ex = sx + x_mult * (int)speed;
	ey = sy + y_mult * (int)speed;

	/* See if we're going to hit the wall. Check horizontal then vertical
	   seperately so we can reverse x or y. If we checked diagonal it
	   would almost always be positive and we wouldn't know whether to 
	   reverse X or Y direction */
	add = (ex > sx ? 1 : -1);
	for(x2=sx;;x2 += add)
	{
		if (outsideTunnel(x2,sy)) 
		{
			x_mult = -x_mult;
			x += (x2 - sx);
			playFGSound(SND_BALL_BOUNCE);
			break;
		}
		if (x2 == ex) break;
	}

	// Check vertical
	add = (ey > sy ? 1 : -1);
	for(y2=sy;;y2 += add)
	{
		if (outsideTunnel(sx,y2)) 
		{
			y_mult = -y_mult;
			y += (y2 - sy);
			playFGSound(SND_BALL_BOUNCE);
			break;
		}
		if (y2 == ey) break;
	}

	x += speed * x_mult;
	y += speed * y_mult;
}




void cl_ball::haveCollided(cl_object *obj, double dist)
{
	switch(obj->type)
	{
	case TYPE_PLAYER:
		if (stage_cnt > 10) setStage(STAGE_INACTIVE);
		break;

	case TYPE_BOULDER:
	case TYPE_WURMAL:
		// Just reverse in X and Y. Not worth the trouble of working
		// out what angle we hit boulder or wurmal at
		x_mult = -x_mult;
		y_mult = -y_mult;
		playFGSound(SND_BALL_BOUNCE);
		break;

	case TYPE_SPOOKY:
	case TYPE_GRUBBLE:
		if (superball_cnt)
		{
			--superball_cnt; 
			if (!superball_cnt) speed = SLOW_SPEED;
		}
		else
		{
			setStage(STAGE_EXPLODE);
			explode->activate();
		}
		break;

	case TYPE_SPIKY:
		// Unaffected by the ball and kills superballs
		setStage(STAGE_EXPLODE);
		explode->activate();
		break;

	default:
		break;
	}
}




void cl_ball::draw()
{
	int col;

	switch(stage)
	{
	case STAGE_RUN:
		if (superball_cnt)
		{
			col = random() % NUM_FULL_COLOURS;
			drawOrFillCircle(col,0,diam,x,y,FILL);

			// Draw rings. Just eye candy.
			drawOrFillCircle(col,4,diam + (stage_cnt % 10) * 3,x,y,DRAW);
		}
		else drawOrFillCircle(COL_RED,0,diam,x,y,FILL);
		break;

	case STAGE_EXPLODE:
		explode->runAndDraw();
		break;

	case STAGE_MATERIALISE:
		explode->runAndDraw();
		break;

	default:
		assert(0);
	}
}
