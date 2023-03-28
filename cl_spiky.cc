#include "globals.h"

#define MAX_ARM_LEN 30

static en_colours start_col[6] = 
{ 
	COL_BLACK2,
	COL_BLACK3,
	COL_BLACK4,
	COL_BLACK5,
	COL_BLACK6,
	COL_BLACK7 
};


////////////////////////////////// SETUP /////////////////////////////////////

/*** Constructor ***/
cl_spiky::cl_spiky(): cl_enemy(TYPE_SPIKY)
{
	diam = 30;
	radius = 15;
	x_mult = y_mult = 0;
}




/*** Create arms and set start position ***/
void cl_spiky::activate()
{
	cl_tunnel *tun;
	int tnum;
	int cnt;
	int len;
	int o;

	cl_enemy::activate();

	// Set up arms
	setArmsForMaterialise();

	// Find a start location in a tunnel
	do
	{
		switch(tunnels.size())
		{
		case 0:
			assert(0);

		case 1:
			tnum = 0;
			break;

		default:
			// Don't pick current player tunnel. Thats unfair.
			cnt = 0;
			do
			{
				tnum = random() % tunnels.size();
			} while(++cnt < 10 && tunnels[tnum] == player->curr_tunnel);
		}
		tun = tunnels[tnum];

		// Pick a random spot thats not in the middle of a boulder
		cnt = 0;
		do
		{
			if (tun->vert)
			{
				x = tun->x1;
				len = tun->max_y - tun->min_y - 2;
				y = tun->min_y + (random() % len) + 1;
			}
			else
			{
				len = tun->max_x - tun->min_x - 2;
				x = tun->min_x + (random() % len) + 1;
				y = tun->y1;
			}
			for(o=0;o < MAX_OBJECTS;++o)
			{
				cl_object *obj = objects[o];
				if (obj->type == TYPE_BOULDER &&
				    obj->stage != STAGE_INACTIVE &&
				    distToObject(obj) < obj->radius)
					break;
			}
		} while(++cnt < 10 && o != MAX_OBJECTS);
	// If this is try then we reached max loop count so pick new tunnel
	} while(o != MAX_OBJECTS);

	speed = level < 20 ? 3 + 0.2 * level : 7;
	lifespan = 400 + 20 * level;

	setChangeDirCnt();
	setMults();
	setStage(STAGE_MATERIALISE);
	playFGSound(SND_SPIKY_MATERIALISE);
}




/*** For object in enemies attract screen ***/
void cl_spiky::attractActivate()
{
	cl_enemy::attractActivate();

	y = 540;
	setArmsForMaterialise();
	setArmsForRun();
}




/*** Set the countdown to randomly changing direction. This is so we don't
     simply bounce ff the walls. Also hopefully could get us unstuck. ***/
void cl_spiky::setChangeDirCnt()
{
	change_dir_cnt = (random() % 130) + 20;
}




/*** Set up angles and lengths but set start colour to black ***/
void cl_spiky::setArmsForMaterialise()
{
	bzero(arm,sizeof(arm));
	for(int i=0;i < SPIKY_ARMS;++i)
	{
		arm[i].len = random() % MAX_ARM_LEN + 1;
		arm[i].len_add = (double)(random() % 10 + 1) / 10;
		arm[i].angle = random() % 360;

		do
		{
			arm[i].ang_add = (random() % 11) - 5;
		} while(!arm[i].ang_add);

		arm[i].col = start_col[random() % 6];
		arm[i].col_add = 0.1;
	}
}




/*** Set runtime arm colour and colour add ***/
void cl_spiky::setArmsForRun()
{
	for(int i=0;i < SPIKY_ARMS;++i)
	{
		arm[i].col = random() % COL_GREEN2;

		do
		{
			arm[i].col_add = (double)(random() % 30) / 10;
		} while(!arm[i].col_add);
	}
}




/*** Randomly set direction multipliers ***/
void cl_spiky::setMults()
{
	int xm;
	int ym;

	do
	{
		xm = (random() % 2) ? -1 : 1;
		ym = (random() % 2) ? -1 : 1;
	} while(xm == x_mult && ym == y_mult);

	x_mult = xm;
	y_mult = ym;
}




///////////////////////////////// RUNTIME //////////////////////////////////


/*** Called from main.cc ***/
void cl_spiky::run()
{
	++stage_cnt;

	switch(stage)
	{
	case STAGE_MATERIALISE:
		stageMaterialise();
		break;

	case STAGE_RUN:
		stageRun();
		break;

	case STAGE_DEMATERIALISE:
		if (stage_cnt > 160) setStage(STAGE_INACTIVE);
		break;

	case STAGE_FALL:
		stageFall();
		break;

	case STAGE_EXPLODE:
		if (stage_cnt == 1)
		{
			if (boulder) text_got_spiky->reset(this,0);
		}
		else if (stage_cnt == 40) setStage(STAGE_INACTIVE);
		break;

	default:
		assert(0);
	}
}




/*** Slowly get bigger and fade in from black ***/
void cl_spiky::stageMaterialise()
{
	if (stage_cnt > 160)
	{
		setArmsForRun();
		setChangeDirCnt();
		setStage(STAGE_RUN);
	}
}




/*** Drift around bouncing off the tunnel walls. Uses a lot of code
     from cl_ball ***/
void cl_spiky::stageRun()
{
	int sx;
	int sy;
	int ex;
	int ey;
	int x2;
	int y2;
	int add;
	int i;

	if (player->freeze_timer) return;

	if (hit_player)
	{
		if (xsize < 2)
		{
			xsize += 0.02;
			ysize += 0.02;
		}
		return;
	}

	// See if times up
	if (stage_cnt == lifespan)
	{
		for(i=0;i < SPIKY_ARMS;++i)
		{
			arm[i].col = start_col[random() % 6];
			arm[i].col_add = -0.1;
		}
		setStage(STAGE_DEMATERIALISE);
		playFGSound(SND_SPIKY_DEMATERIALISE);
		return;
	}

	// See if its time to change our direction
	if (!--change_dir_cnt)
	{
		setChangeDirCnt();
		setMults();
	}

	// See cl_ball for code explanation
	sx = (int)x + x_mult * radius;
	sy = (int)y + y_mult * radius;
	ex = sx + x_mult * (int)speed;
	ey = sy + y_mult * (int)speed;
	
	// Check horizontal move for hit
	add = (ex > sx ? 1 : -1);
	for(x2=sx;;x2 += add)
	{
		if (outsideTunnel(x2,sy))
		{
			x_mult = -x_mult;
			x += (x2 - sx);
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
			break;
		}
		if (y2 == ey) break;
	}

	x += speed * x_mult;
	y += speed * y_mult;
}




/*** Ball, player or boulder only ***/
void cl_spiky::haveCollided(cl_object *obj, double dist)
{
	if (stage != STAGE_RUN) return;

	switch(obj->type)
	{
	case TYPE_BOULDER:
		if (obj->stage == STAGE_FALL)
		{
			if (stage != STAGE_FALL)
			{
				boulder = (cl_boulder *)obj;
				incScore(800);
				setStage(STAGE_FALL);
				playFGSound(SND_FALL);
			}
			break;
		}

		// Reverse direction
		x_mult = -x_mult;
		y_mult = -y_mult;
		break;

	case TYPE_BALL:
		// Not killed by ball, just reverse direction
		x_mult = -x_mult;
		y_mult = -y_mult;
		break;

	case TYPE_PLAYER:
		if (player->freeze_timer || hit_player) break;

		// Speed everything up to make it seem excited
		for(int i=0;i < SPIKY_ARMS;++i)
		{
			arm[i].len_add *= 3;
			arm[i].ang_add *= 3;
			arm[i].col_add *= 3;
		}
		hit_player = true;
		break;

	default:
		break;
	}
}




/*** Draw arms and update them ***/
void cl_spiky::draw()
{
	double xs;
	double ys;
	int i;

	if (stage == STAGE_EXPLODE)
	{
		explode->runAndDraw();
		return;
	}

	for(i=0;i < SPIKY_ARMS;++i)
	{
		xs = x + arm[i].len * SIN(arm[i].angle) * xsize;
		ys = y + arm[i].len * COS(arm[i].angle) * ysize;

		drawLine(
			player->freeze_timer ? COL_MEDIUM_BLUE : (int)arm[i].col,
			3,x,y,xs,ys);

		arm[i].len += arm[i].len_add;
		if (arm[i].len < 1)
		{
			arm[i].len = 1;
			arm[i].len_add = -arm[i].len_add;
		}
		else if (arm[i].len > MAX_ARM_LEN)
		{
			arm[i].len = MAX_ARM_LEN;
			arm[i].len_add = -arm[i].len_add;
		}

		::incAngle(arm[i].angle,arm[i].ang_add);

		arm[i].col += arm[i].col_add;

		if (stage == STAGE_RUN)
		{
			if (arm[i].col <= 0) arm[i].col = COL_GREEN2;
			else
			if (arm[i].col >= COL_GREEN2) arm[i].col = COL_GREEN;
		}
	}
}
