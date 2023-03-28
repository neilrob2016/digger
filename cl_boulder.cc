#include "globals.h"

#define PUSH_SPEED 0.5

static int cnum;


/*** Constructor does nothing ***/
cl_boulder::cl_boulder(): cl_rock(TYPE_BOULDER)
{
	int i;

	for(i=0;i < NUM_SMALL_BOULDERS;++i)
		small_boulder[i] = new cl_small_boulder(this);

	// Find our position in list - this will never change during the
	// life of the process.
	list_pos = 0;
	for(auto obj: objects)
	{
		if (!obj) break;
		if (obj->type == TYPE_BOULDER) ++list_pos;
	}
	assert(list_pos < MAX_BOULDERS);
}



/*** Set up params and place boulders somewhere. Each boulder has its own 
     screen quarter and for the given number of boulders the layout is as 
     follows:
         2:   3:       2:   3:        4:
         1 0  1 1  or  0 1  1 1  and  1 1
         0 1  0 1      1 0  1 0       1 1
 ***/
#define SCR_Q1 (SCR_SIZE / 4)
#define SCR_Q2 (SCR_SIZE - SCR_Q1)
#define LEEWAY (SCR_Q1 / 2)

void cl_boulder::activate()
{
	XPoint centre[2][MAX_BOULDERS] =
	{       
		//  X        Y
		{
			{ SCR_Q1, SCR_Q1 },
			{ SCR_Q2, SCR_Q2 },
			{ SCR_Q2, SCR_Q1 },
			{ SCR_Q1, SCR_Q2 }
		},
		{
			{ SCR_Q2, SCR_Q1 },
			{ SCR_Q1, SCR_Q2 },
			{ SCR_Q1, SCR_Q1 },
			{ SCR_Q2, SCR_Q2 }
		}
	};

	cl_rock::activate();

	col = COL_GREYISH;
	on_boulder = false;
	cant_push_cnt = 0;
	cant_push_dir = DIR_STOP;
	curr_tunnel = NULL;
	break_height = level < 15 ? 400 - 20 * level : 100;
	wobble_cnt = level < 10 ? 60 - 3 * level : 30;

	// If we're the first boulder set the centre array num
	if (!list_pos) cnum = random() % 2;

	/* Get our centre location and pick a random location near it.
	   Since boulders are activated first we don't have to worry about
	   ending up on top of a nugget. Nuggets have to avoid boulders. */
	x = (double)centre[cnum][list_pos].x + (random() % SCR_Q1) - LEEWAY;
	y = (double)centre[cnum][list_pos].y + (random() % SCR_Q1) - LEEWAY;

	resetFallCheck();
}




/*** Fall check add is distance past radius we check for tunnel ***/
void cl_boulder::resetFallCheck()
{
	fall_check_add = 4;
	fall_y = (int)y + radius + fall_check_add;
}




/** This big lump just sits and checks to see if the ground has been dug away 
    from underneath it. If it has it falls. ***/
void cl_boulder::run()
{
	double inc;

	++stage_cnt;

	switch(stage)
	{
	case STAGE_RUN:
		if (cant_push_cnt)
			--cant_push_cnt;
		else
			cant_push_dir = DIR_STOP;

		// Middle must be clear and either one side or the other 
		// before we fall
		if (!on_boulder && insideTunnel((int)x,fall_y))
		{
			setStage(STAGE_WOBBLE);
			playFGSound(SND_BOULDER_WOBBLE);
		}
		break;

	case STAGE_WOBBLE:
		if (stage_cnt == wobble_cnt)
		{
			fall_start_y = y;
			setStage(STAGE_FALL);
		}
		return;

	case STAGE_FALL:
		y += FALL_SPEED;
		angle += 5;

		// If we didn't have check add it could wobble then not move
		// as it found ground right underneath
		if (fall_check_add >= FALL_SPEED)
			fall_check_add -= FALL_SPEED;
		else
			fall_check_add = 0;
		fall_y = (int)y + radius + fall_check_add;

		// Stop when we hit bottom of a tunnel
		if (outsideTunnel((int)x,fall_y))
		{
			setCurrTunnel();
			if (y - fall_start_y >= break_height)
			{
				col = COL_RED;
				setStage(STAGE_HIT);
			}
			else setStage(STAGE_RUN);

			playFGSound(SND_BOULDER_LAND);
			resetFallCheck();
		}
		break;

	case STAGE_HIT:
		inc = (stage_cnt % 10) < 5 ? 0.1 : -0.1;
		xsize += inc;
		ysize += inc;
		if (stage_cnt == 40)
		{
			activateSmallBoulders();
			setStage(STAGE_EXPLODE);
			playFGSound(SND_BOULDER_EXPLODE);
		}
		break;

	case STAGE_EXPLODE:
		// Bonus score for breaking it on long drop
		if (stage_cnt == 1) activateBonusScore(this,250);
		else
		if (stage_cnt == 60)
			setStage(STAGE_INACTIVE);
		else
			runSmallBoulders();
		break;

	case STAGE_BEING_EATEN:
		// Slowly get smaller
		if (stage_cnt == eating_time) setStage(STAGE_INACTIVE);
		else
		{
			xsize = ysize = (double)(eating_time - stage_cnt) / eating_time;
			runSmallBoulders();
		}
		break;
		
	default:
		assert(0);
	}

	// Keep resetting in case boulder underneath us falls away
	on_boulder = false;
}




/*** Called when we've fallen into a tunnel. Find out which one we're in 
     and set it so grubble can find us ***/
void cl_boulder::setCurrTunnel()
{
	cl_tunnel *best_tun = NULL;
	double best_dist = FAR_FAR_AWAY;
	double dist;

	for(auto tun: tunnels)
	{
		/* Can be in a number of tunnels at once. Find one we closest
		   to the centre line of. This isn't a perfect solution as we
		   could be close to centre line but right on edge but its 
		   good enough for a game */
		if (x >= tun->min_x && x <= tun->max_x &&
		    y >= tun->min_y && y <= tun->max_y)
		{
			dist = tun->vert ? fabs(tun->x1 - x) : fabs(tun->y1 - y);
			if (!best_tun || dist < best_dist)
			{
				best_dist = dist;
				best_tun = tun;
			}
		}
	}
	curr_tunnel = best_tun;
}




/*** Called when player tries to push boulder horizontally ***/
double cl_boulder::push(en_dir push_dir)
{
	int px;
	double add;

	assert(push_dir == DIR_LEFT || push_dir == DIR_RIGHT);

	if (cant_push_dir == push_dir) return 0;

	if (push_dir == DIR_LEFT)
	{
		px = (int)x - radius - 1;
		add = -PUSH_SPEED;
	}
	else
	{
		px = (int)x + radius + 1;
		add = PUSH_SPEED;
	}

	// Make sure we have a reasonably clear tunnel to push it
	if (insideTunnel(px,(int)y + radius / 2) &&
	    insideTunnel(px,(int)y) &&
	    insideTunnel(px,(int)y - radius / 2)) 
	{
		x += add;
		return PUSH_SPEED;
	}
	return 0;
}




/*** Called by grubble ***/
void cl_boulder::setBeingEaten()
{
	setStage(STAGE_BEING_EATEN);
	activateSmallBoulders();
}




/*** (Re)set the small boulders ***/
void cl_boulder::activateSmallBoulders()
{
	for(int i=0;i < NUM_SMALL_BOULDERS;++i) small_boulder[i]->activate();
}




/*** Run them for being eaten or explosion ***/
void cl_boulder::runSmallBoulders()
{
	for(int i=0;i < NUM_SMALL_BOULDERS;++i) small_boulder[i]->run();
}




/*** Only care about other boulders ***/
void cl_boulder::haveCollided(cl_object *obj, double dist)
{
	switch(obj->type)
	{
	case TYPE_BOULDER:
		// If we've landed on another boulder then stop
		if (y < obj->y)
		{
			on_boulder = true;
			if (stage != STAGE_RUN) setStage(STAGE_RUN);
		}
		// Fall through

	case TYPE_SPOOKY:
	case TYPE_GRUBBLE:
	case TYPE_WURMAL:
		// Don't want to be able to squash them. Have countdown so that
		// cant_push_dir not zeroed before push function gets to see it
		cant_push_cnt = 2;
		cant_push_dir = (x < obj->x ? DIR_RIGHT : DIR_LEFT);
		break;
		
	default:
		break;
	}
}




/*** Wobble if we're about to fall ***/
void cl_boulder::draw()
{
	double x2;
	double y2;

	switch(stage)
	{
	case STAGE_WOBBLE:
		x2 = x;
		y2 = y;
		x += (random() % 10) - 5;
		y += (random() % 10) - 5;
		cl_rock::draw();
		x = x2;
		y = y2;
		break;

	case STAGE_BEING_EATEN:
		drawSmallBoulders();
		// Fall through

	case STAGE_RUN:
	case STAGE_FALL:
		cl_rock::draw();
		break;

	case STAGE_HIT:
		cl_rock::draw();
		--col;
		break;

	case STAGE_EXPLODE:
		drawSmallBoulders();
		break;

	default:
		assert(0);
	}
}




void cl_boulder::drawSmallBoulders()
{
	for(int i=0;i < NUM_SMALL_BOULDERS;++i) small_boulder[i]->draw();
}
