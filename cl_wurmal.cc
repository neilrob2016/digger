#include "globals.h"

////////////////////////////////// SETUP ////////////////////////////////////

/*** Constructor ***/
cl_wurmal::cl_wurmal(): cl_enemy(TYPE_WURMAL)
{
	diam = 20;
	head_diam = diam * 2;
	radius = 10;
	head_radius = radius * 2;

	eye_x[0] = -8;
	eye_y[0] = 0;

	eye_x[1] = 8;
	eye_y[1] = 0;
}




/*** Set up the start location and segments ***/
void cl_wurmal::activate()
{
	int xmod = SCR_SIZE - diam;
	int ymod = PLAY_AREA_HEIGHT - diam;
	int i;
	int j;

	cl_enemy::activate();

	/* Find somewhere thats not in a tunnel and not close to any boulders 
	   or nuggets and not close to the player start location. Limit the 
	   number of times we do this so game doesn't hang. Better to miss
	   one wurmal than no game. */
	i = 0;
	LOOP:
	j = 0;
	do
	{
		x = (random() % xmod) + radius;
		y = PLAY_AREA_TOP + (random() % ymod) + radius;
	} while(++j < 20 && 
	        (outsideGround(x,y) ||
	         outsideGround(x-head_radius,y) ||
	         outsideGround(x,y-head_radius) ||
	         outsideGround(x+head_radius,y) ||
	         outsideGround(x,y+head_radius) ||
	         hypot(x - START_X,y - START_Y) < TUNNEL_WIDTH * 2));

	if (j == 20) return; 

	for(auto obj: objects)
	{
		switch(obj->type)
		{
		case TYPE_NUGGET:
		case TYPE_BOULDER:
			if (obj->stage == STAGE_RUN &&
			    cl_object::overlapDist(obj))
			{
				if (++i == 10) return;
				goto LOOP;
			}
			break;

		default:
			break;
		}
	}

	find_random_nugget = false;
	random_move_cnt = 0;
	x_add = 0;
	y_add = 0;
	x_edge = x;
	y_edge = y;
	eating = false;
	hit_boulder = false;
	eye_col = COL_BLACK2;

	initSegments();

	if (level < 10)
	{
		speed = 0.8 + 0.1 * level;
		eating_time = 200 - level * 10;
	}
	else
	{
		speed = 2;
		eating_time = 100;
	}

	start_y = y;
	y = -diam;

	setStage(STAGE_MATERIALISE);
}




/*** For enemies attract screen ***/
void cl_wurmal::attractActivate()
{
	cl_enemy::attractActivate();
	y = 400;
	eye_col = COL_BLACK2;
	angle = 90;
	initSegments();
}




/*** Set all segments to start position ***/
void cl_wurmal::initSegments()
{
	for(int i=0;i < WURMAL_SEGMENTS;++i)
	{
		segment[i].x = x;
		segment[i].y = y;
	}
}



///////////////////////////////// RUNTIME /////////////////////////////////

/*** Runtime switch ***/
void cl_wurmal::run()
{
	++stage_cnt;

	switch(stage)
	{
	case STAGE_MATERIALISE:
		if ((y += 15) >= start_y)
		{
			y = start_y;
			setStage(STAGE_RUN);
			findNugget();
			fill = FILL;
		}
		break;

	case STAGE_RUN:
		stageRun();
		break;

	case STAGE_HIT:
		if (xsize < 0.1) setStageExplode();
		break;

	case STAGE_EXPLODE:
		if (stage_cnt == 20)
		{
			setStage(STAGE_INACTIVE);
			++wurmals_killed;
		}
		break;

	default:
		assert(0);
	}
}




/*** For enemies attract screen ***/
void cl_wurmal::attractRun()
{
	cl_enemy::attractRun();
	++stage_cnt;
	updateSegments();
}




/*** Look for nuggets. Update segment locations ***/
void cl_wurmal::stageRun()
{
	// If we've been trapped in a tunnel then stop. Shouldn't happen
	// but just in case.
	if (outsideGround(x,y))
	{
		eye_col = COL_BLACK2;
		eating = false;
		return;
	}

	// If frozen do nothing
	if (player->freeze_timer) return;

	// Just do some animation while eating
	if (eating)
	{
		if (stage_cnt == eating_time)
		{
			eating = false; 
			nugget = NULL;
		}
		return;
	}

	// If we have a nugget move towards it
	if (nugget)
	{
		if (nugget->stage != STAGE_RUN)
		{
			nugget = NULL;
			return;
		}

		// This monster doesn't use directions as it can go in a
		// diagonal line.
		if (fabs(x - nugget->x) > radius)
			x_add = nugget->x < x ? -1 : (nugget->x > x ? 1 : 0);
		else
			x_add = 0;

		if (fabs(y - nugget->y) > radius)
			y_add = nugget->y < y ? -1 : (nugget->y > y ? 1 : 0);
		else
			y_add = 0;

		updateEyeAngle();
		updateEdges();

		// Avoid tunnels and the edge of the screen
		if (outsideGround(x_edge,y_edge))
		{
			if (outsideGround(x_edge,y_edge)) x_add = y_add = 0;
			else
			{
				if (outsideGround(x_edge,y)) x_add = 0;
				if (outsideGround(x,y_edge)) y_add = 0;
			}
			updateEyeAngle();
			updateEdges();
		}

		// Can't head in nugget direction, try a random move.
		if (!x_add & !y_add)
		{
			nugget = NULL;
			pickRandomMove();
		}
	}
	// Keep moving along random path for given time - to stop jiggling
	// about
	else if (random_move_cnt)
	{
		--random_move_cnt;
		updateEdges();
		if (outsideGround(x_edge,y_edge))
		{
			random_move_cnt = 0;
			return;
		}
	}
	else if (hit_boulder)
	{
		// Move randomly so we don't just go back in the same direction
		// hit it again, move back again etc etc
		pickRandomMove();
		hit_boulder = false;
	}
	else
	{
		// Pick a new nugget
		x_add = 0;
		y_add = 0;
		findNugget();
		if (nugget) return;
		pickRandomMove();
	}

	// Move
	x += x_add * speed;
	y += y_add * speed;

	updateSegments();
}




/*** Find the nugget nearest to us or just a random one. Hopefully it won't be 
     across a tunnel ***/
void cl_wurmal::findNugget()
{
	cl_nugget *nug;
	cl_object *closest;
	double closest_dist;
	double dist;

	nugget = NULL;
	closest = NULL;
	closest_dist = FAR_FAR_AWAY;

	for(auto obj: objects)
	{
		if (obj->type == TYPE_NUGGET && 
		    obj->stage == STAGE_RUN)
		{
			nug = (cl_nugget *)obj;

			switch(nug->nugtype)
			{
			case cl_nugget::WURMALLED:
			case cl_nugget::TURBO_ENEMY:
			case cl_nugget::INVISIBILITY:
				// If already eaten, turbo enemy or 
				// invisibility don't eat it.
				continue;

			case cl_nugget::NORMAL:
				break;

			default:
				// Don't eat powerups at lower levels
				if (level < 6) continue;
			}

			if (find_random_nugget)
			{
				nugget = obj;
				return;
			}
			if ((dist = distToObject(obj)) < closest_dist)
			{
				closest_dist = dist;
				closest = obj;
			}
		}
	}
	nugget = closest;
}




/*** Pick a random move that doesn't send us into a tunnel ***/
void cl_wurmal::pickRandomMove()
{
	int i = 0;

	random_move_cnt = 50 + random() % 50;
	find_random_nugget = !find_random_nugget;

	do
	{
		do 
		{
			x_add = (random() % 3) - 1;
			y_add = (random() % 3) - 1;
		} while (!x_add && !y_add);

		updateEdges();
	} while(++i < 10 && outsideGround(x_edge,y_edge));

	updateEyeAngle();
}




/*** Update the angle to get the eyes locations right when drawing ***/
void cl_wurmal::updateEyeAngle()
{
	// Only need 4 different angles since eyes are symmetrical about
	// head centre
	if (!x_add) angle = 0;
	else
	if (!y_add) angle = 90;
	else
	if (x_add == y_add) angle = 135;
	else
	angle = 45;
}




/*** Edges of the head used for checking if in tunnel ***/
void cl_wurmal::updateEdges()
{
	x_edge = x + x_add * speed + x_add * head_radius;
	y_edge = y + y_add * speed + y_add * head_radius;
}




/*** Segment 0 is head , the rest follow ***/
void cl_wurmal::updateSegments()
{
	segment[0].x = x;
	segment[0].y = y;

	if (!(stage_cnt % 5))
	{
		for(int i=WURMAL_SEGMENTS-1;i > 0;--i)
		{
			segment[i].x = segment[i-1].x;
			segment[i].y = segment[i-1].y;
		}
	}
}




/*** Returns true if we're not in the dirt ***/
bool cl_wurmal::outsideGround(double x, double y)
{
	return offscreen((int)x,(int)y) || !tunnel_bitmap[(int)x][(int)y];
}


///////////////////////////// OTHER OVERLOADS ///////////////////////////////

/*** Only care about hitting nugget and boulder ***/
void cl_wurmal::haveCollided(cl_object *obj, double dist)
{
	switch(obj->type)
	{
	case TYPE_BOULDER:
		// Move away from it for a short while
		x_add = (obj->x < x ? 1 : -1);
		y_add = (obj->y < y ? 1 : -1);

		updateEyeAngle();
		updateEdges();

		nugget = NULL;
		hit_boulder = true;
		random_move_cnt = 5;
		break;

	case TYPE_NUGGET:
		if (!eating && obj == nugget)
		{
			nugget = NULL;
			eating = true;
			stage_cnt = 0;
		}
		break;

	case TYPE_PLAYER:
		// If player has hit us when invisible then die
		if (player->invisible_timer || player->freeze_timer)
		{
			incScore(400);
			setStage(STAGE_HIT);
			playFGSound(SND_WURMAL_HIT);
		}
		break;

	default:
		break;
	}
}




/*** Scree is a non regular shape therefor we have a special case ***/
double cl_wurmal::overlapDist(cl_object *obj)
{
	double dist;
	int rad;
	int i;

	// Two screes colliding is irrelevant to game. Don't care.
	if (obj->type == TYPE_WURMAL) return 0;

	for(i=0;i < WURMAL_SEGMENTS;++i)
	{
		rad = i ? radius : head_radius;
		dist = rad + obj->radius - 
		       hypot(segment[i].x - obj->x,segment[i].y - obj->y);

		if (dist > 0) return dist;
	}
	return 0;
}




/*** Draw all the segments - draw last first ***/
void cl_wurmal::draw()
{
	double hd = 0; // Stops OSF/1 compiler whinge

	switch(stage)
	{
	case STAGE_MATERIALISE:
		hd = head_diam;
		break;

	case STAGE_RUN:
		hd = eating ? 
		     head_diam + (head_diam - abs((game_stage_cnt * 2 % head_diam * 2) - head_diam)) / 2 : 
		     head_diam;
		break;

	case STAGE_HIT:
		xsize -= 0.03;
		ysize -= 0.03;
		eye_col = (game_stage_cnt % 2) ? COL_BLACK2 : COL_RED2;
		hd = head_diam * xsize;
		break;

	case STAGE_EXPLODE:
		explode->runAndDraw();
		return;

	default:
		assert(0);
	}

	// Draw tail segments
	if (stage != STAGE_MATERIALISE)
	{
		for(int i=WURMAL_SEGMENTS-1;i >= 1;--i)
		{
			// Can't use objDrawOrFillCircle() because angle value 
			// will cause problems
			drawOrFillCircle(
				COL_GREEN + i * 3,
				0,(double)diam * xsize,
				segment[i].x,segment[i].y,fill);
		}
	}

	// Draw head
	objDrawOrFillCircle(
		player->freeze_timer ? COL_MEDIUM_BLUE : COL_PURPLE,
		4,hd,0,0,fill);
	objDrawOrFillCircle(eye_col,4,10,eye_x[0]*xsize,eye_y[0]*ysize,fill);
	objDrawOrFillCircle(eye_col,4,10,eye_x[1]*xsize,eye_y[1]*ysize,fill);

	if (eye_col != COL_BLACK && ++eye_col == COL_BLACK3)
		eye_col = COL_BLACK2;
}
