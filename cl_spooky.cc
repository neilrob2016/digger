#include "globals.h"

#define VALUE 100

XPoint cl_spooky::body[2][BODY_POINTS] =
{
	{
		{ 0,-20 },
		{ -20,15 },
		{ -17,20 },
		{ -10,15 },
		{ -5,15 },
		{ -2,20 },
		{ 5,15 },
		{ 10,15 },
		{ 13,20 },
		{ 20,15 }
	},
	{
		{ 0,-20 },
		{ -20,15 },
		{ -13,20 },
		{ -10,15 },
		{ -5,15 },
		{ 2,20 },
		{ 5,15 },
		{ 10,15 },
		{ 17,20 },
		{ 20,15 }
	}
};


XPoint cl_spooky::teeth[TEETH_POINTS] =
{
	{ -8,5 },
	{ -4,12 },
	{ 0,5 },
	{ 4,12 },
	{ 8,5 }
};


////////////////////////////////// SETUP /////////////////////////////////////


/*** Constructor ***/
cl_spooky::cl_spooky(): cl_enemy(TYPE_SPOOKY)
{
	diam = 40;
	radius = diam / 2;
}




void cl_spooky::activate()
{
	cl_enemy::activate();

	setStage(STAGE_MATERIALISE);
	x = START_X;
	y = -diam;
	dir = DIR_STOP;
	pup_x_add = 0;
	pup_y_add = 0;
	ang_inc = 0;
	bodynum = 0;
	body_col = COL_LIGHT_BLUE;
	eye_col = COL_WHITE;
	pup_col = COL_RED;

	start_speed = speed = (level < 20 ? 2 + 0.2 * level : 6);
	max_depth = level < 4 ? 2 + level : 6;

	playFGSound(SND_ENEMY_MATERIALISE);
}




/*** For attract mode enemy display ***/
void cl_spooky::attractActivate()
{
	cl_enemy::attractActivate();

	prev_y = y = 120;
	bodynum = 0;
	body_col = COL_LIGHT_BLUE;
	eye_col = COL_WHITE;
	pup_col = COL_RED;
}



//////////////////////////////// RUNTIME ///////////////////////////////////

/*** Call the appropriate behaviour ***/
void cl_spooky::run()
{
	++stage_cnt;
	prev_x = x;
	prev_y = y;

	switch(stage)
	{
	case STAGE_MATERIALISE:
		stageMaterialise();
		break;

	case STAGE_RUN:
		stageRun();
		break;

	case STAGE_FALL:
		stageFall();
		break;

	case STAGE_HIT:
		if (xsize <= 0) setStageExplode();
		break;

	case STAGE_EXPLODE:
		if (stage_cnt == 1)
		{
			if (boulder) activateBonusScore(this,200);
		}
		else if (stage_cnt == 20) setStage(STAGE_INACTIVE);
		break;

	default:
		assert(0);
	}
}




/*** Do our actual run time thang... ***/
void cl_spooky::stageRun()
{
	int ret;

	// If frozen , do nothing
	if (player->freeze_timer) return;

	// This shouldn't happen but occasionally does due to some obscure
	// bug. Cope with it.
	if (outsideTunnel((int)x,(int)y))
	{
		x = prev_x;
		y = prev_y;
		pickRandomTunnel();
		setDirection();
		move();
		return;
	}

	// If we've hit player continue moving for short time and expand
	if (hit_player)
	{
		hitPlayerMove();
		return;
	}

	// Speed up if turbo speed is on
	if (player->turbo_enemy_timer)
	{
		if (speed == start_speed) speed = speed * 2;
	}
	else speed = start_speed;

	// If we're in the same tunnel as player just head towards them
	if (!player->invisible_timer && curr_tunnel == player->curr_tunnel) 
		setDirectionToObject(player);
	else if (next_tunnel)
	{
		setDirection();
		if (dir == DIR_STOP) return;
	}
	// next_tunnel == NULL. 1 in 5 times pick a random new tunnel or if
	// player is invisible
	else if (!player->invisible_timer && random() % 5)
	{
		if ((ret = findShortestPath(
			0,
			max_depth,
			curr_tunnel,player->curr_tunnel,next_tunnel)) != -1)
		{
			setDirection();
		}
		else pickPlayerDirTunnel();
	}
	else
	{
		pickRandomTunnel();
		setDirection();
	}
	move();
}




/*** Move and update pupil locations ***/
void cl_spooky::move()
{

	switch(dir)
	{
	case DIR_STOP:
		break;

	case DIR_LEFT:
		x -= speed;
		pup_x_add = -4;
		pup_y_add = 0;
		break;

	case DIR_RIGHT:
		x += speed;
		pup_x_add = 4;
		pup_y_add = 0;
		break;

	case DIR_UP:
		y -= speed;
		pup_x_add = 0;
		pup_y_add = -4;
		break;

	case DIR_DOWN:
		y += speed;
		pup_x_add = 0;
		pup_y_add = 4;
		break;

	default:
		assert(0);
	}
}


//////////////////////////////// OTHER OVERLOADS ////////////////////////////


/*** Ball, player or boulder only ***/
void cl_spooky::haveCollided(cl_object *obj, double dist)
{
	switch(obj->type)
	{
	case TYPE_BOULDER:
		if (obj->stage == STAGE_FALL)
		{
			if (stage != STAGE_FALL)
			{
				boulder = (cl_boulder *)obj;
				incScore(VALUE);
				setStage(STAGE_FALL);
				playFGSound(SND_FALL);
			}
			break;
		}

		// Tunnel blocked. Reverse.
		reverseDirection();
		break;

	case TYPE_BALL:
		pup_x_add = 0;
		pup_y_add = 0;
		incScore(VALUE);
		setStage(STAGE_HIT);
		playFGSound(SND_SPOOKY_HIT);
		return;

	case TYPE_PLAYER:
		// If frozen then do nothing
		if (!player->freeze_timer)
		{
			hit_player = true;
			body_col = COL_RED;
			eye_col = COL_LIGHT_BLUE;
			pup_col = COL_WHITE;
		}
		break;

	default:
		break;
	}
}




/*** All drawing animation ***/
void cl_spooky::draw()
{
	int bcol = body_col;

	switch(stage)
	{
	case STAGE_MATERIALISE:
		// Just flash
		if ((game_stage_cnt % 4) < 2) return;
		break;

	case STAGE_RUN:
		if (!(game_stage_cnt % 10)) bodynum = !bodynum;
		bcol = player->freeze_timer ? COL_MEDIUM_BLUE : body_col;
		break;

	case STAGE_HIT:
		// Strobe eyes
		if ((game_stage_cnt % 6) < 3)
		{
			eye_col = COL_RED;
			pup_col = COL_WHITE;
		}
		else
		{
			eye_col = COL_WHITE;
			pup_col = COL_RED;
		}

		// Shiver
		if (stage_cnt < 40)
		{
			x += (random() % 10) - 5;
			y += (random() % 10) - 5;
			break;
		}

		// Spin and change size
		incAngle(ang_inc);
		ang_inc += 0.8;
		if (stage_cnt < 50)
		{
			if (xsize < 2) xsize += 0.05;
			if (xsize < 2) ysize += 0.05;
		}
		else
		{
			if (xsize > 0) xsize -= 0.05;
			if (xsize > 0) ysize -= 0.05;
		}
		break;

	case STAGE_EXPLODE:
		explode->runAndDraw();
		return;

	case STAGE_FALL:
		break;

	default:
		assert(0);
	}

	// Body
	objDrawOrFillPolygon(bcol,1,body[bodynum],BODY_POINTS,fill);

	// Eyes
	objDrawOrFillRectangle(eye_col,1,-15,-10,10,10,fill);
	objDrawOrFillRectangle(eye_col,1,5,-10,10,10,fill);

	// Pupils
	objDrawOrFillRectangle(pup_col,1,pup_x_add - 13,pup_y_add - 7,5,5,fill);
	objDrawOrFillRectangle(pup_col,1,pup_x_add + 8,pup_y_add - 7,5,5,fill);

	// Draw line of mouth and teeth
	objDrawLine(COL_RED,2,-10,5,10,5);
	objDrawOrFillPolygon(COL_GREEN,1,teeth,TEETH_POINTS,fill);

	if (stage == STAGE_HIT)
	{
		x = prev_x;
		y = prev_y;
	}
}
