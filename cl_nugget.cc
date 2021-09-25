#include "globals.h"

/*** Constructor ***/
cl_nugget::cl_nugget(): cl_rock(TYPE_NUGGET)
{
}




/*** See if we're to be a powerup and find somewhere to put us ***/
void cl_nugget::activate()
{
	int xmod;
	int ymod;
	int o;

	cl_rock::activate();

	xmod = SCR_SIZE - diam;
	ymod = PLAY_AREA_HEIGHT - diam;

	start_col = col = YELLOW + (random() % 5) - 2;

	if (invisible_powerup_cnt)
	{
		nugtype = INVISIBILITY;
		--invisible_powerup_cnt;
	}
	else if (superball_powerup_cnt)
	{
		nugtype = SUPERBALL;
		--superball_powerup_cnt;
	}
	else if (freeze_powerup_cnt)
	{
		nugtype = FREEZE;
		--freeze_powerup_cnt;
		col = BLUE;
	}
	else if (bonus_nugget_cnt)
	{
		// Not actually a powerup, just a nugget that gives a bonus
		// score, but simpler to use powerup enum
		nugtype = BONUS;
		--bonus_nugget_cnt;
		setBonusTime();
	}
	else if (turbo_enemy_powerup_cnt)
	{
		// Powerup for the spooky and grubble who move around much 
		// faster.
		nugtype = TURBO_ENEMY;
		--turbo_enemy_powerup_cnt;
		col = RED2;
	}
	else nugtype = NORMAL;

	give_bonus = false;
	++nugget_cnt;

	// Pick random spot thats not within a certain distance of player and
	// start location and not in a start tunnel. 
	LOOP:
	do
	{
		x = (random() % xmod) + radius;
		y = PLAY_AREA_TOP + (random() % ymod) + radius;
	} while(insideTunnel((int)x,(int)y) ||
	        hypot(x - START_X,y - START_Y) < TUNNEL_WIDTH * 2);

	// Check against other objects
	FOR_ALL_OBJECTS(o)
	{
		if (objects[o] != this && overlapDist(objects[o])) goto LOOP;
	}
}




/*** Set up the countdown for the nugget to give special bonus ***/
void cl_nugget::setBonusTime()
{
	bonus_time = stage_cnt + 100 + random() % 200;
}




/*** Just shrinks us when we're hit ***/
void cl_nugget::run()
{
	++stage_cnt;

	if (stage == STAGE_HIT)
	{
		xsize -= 0.1;
		ysize -= 0.1;

		if (xsize < 0.2)
		{
			setStage(STAGE_INACTIVE);
			--nugget_cnt;
		}
		return;
	}

	// Set up for next time
	if (nugtype == BONUS && stage_cnt >= bonus_time)
	{
		if (stage_cnt == bonus_time)
		{
			give_bonus = true;
			col = GREEN2;
		}
		else if (stage_cnt > bonus_time + 300)
		{
			give_bonus = false;
			resetBonusColSize();
			setBonusTime();
		}
	}
}




/*** Called to reset to start conditions ***/
void cl_nugget::resetBonusColSize()
{
	col = start_col;
	xsize = ysize = 1;
}




/*** Only player and wurmal eat us ***/
void cl_nugget::haveCollided(cl_object *obj, double dist)
{
	cl_wurmal *wurm;

	if (stage != STAGE_RUN) return;

	switch(obj->type)
	{
	case TYPE_WURMAL:
		wurm = (cl_wurmal *)obj;
		if (nugtype != WURMALLED && wurm->nugget == this)
		{
			nugtype = WURMALLED;
			col = YELLOW2;
		}
		break;

	case TYPE_PLAYER:
		xsize = ysize = 1;
		setStage(STAGE_HIT);
		if (nugtype != WURMALLED) incScore(50);
		break;

	default:
		break;
	}
}




/*** Do any powerup specific stuff then call parent draw func ***/
void cl_nugget::draw()
{
	switch(nugtype)
	{
	case WURMALLED:
		if (col > BLACK7) col -= 0.25;
		break;

	case NORMAL:
		break;

	case INVISIBILITY:
		fill = (game_stage_cnt % 20 < 10) ? DRAW : FILL;
		break;

	case SUPERBALL:
		col = random() % NUM_FULL_COLOURS;
		break;

	case FREEZE:
		if (--col == TURQUOISE) col = BLUE;
		break;

	case BONUS:
		// game_stage_cnt not incremented during a pause so sizes
		// could keep on getting bigger or smaller if game paused
		if (give_bonus && !paused)
		{
			if (!game_stage_cnt) resetBonusColSize();

			if (game_stage_cnt % 10 < 5)
			{
				xsize += 0.1;
				ysize += 0.1;
				col += 4;
				if (col > GREEN2) col = GREEN + (col - GREEN2);
			}
			else
			{
				xsize -= 0.1;
				ysize -= 0.1;
				col -= 4;
				// GREEN = 0
				if (col < GREEN) col = GREEN2 + col;
			}
		}
		break;

	case TURBO_ENEMY:
		col = (game_stage_cnt % 10 < 5 ? RED : BLACK);
		break;

	default:
		assert(0);
	}
	cl_rock::draw();
}
