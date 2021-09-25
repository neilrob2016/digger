#include "globals.h"

#define VALUE 200

XPoint cl_grubble::body[2][BODY_POINTS] =
{
	{
		// Mouth open
		{ -20,-15 },
		{ -15,-20 },
		{ 15,-20 },
		{ 20,-15 },
		{ -5,0 },
		{ 20,15 },
		{ 15,20 },
		{ -15,20 },
		{ -20,15 }
	},
	{
		// Mouth closed
		{ -20,-15 },
		{ -15,-20 },
		{ 15,-20 },
		{ 20,-5 },
		{ -5,0 },
		{ 20,5 },
		{ 15,20 },
		{ -15,20 },
		{ -20,15 }
	}
};

XPoint cl_grubble::top_teeth[2][TEETH_POINTS] =
{
	{
		{ -1,-4 },
		{ 3,-1 },
		{ 7,-9 },

		{ 9,-9 },
		{ 13,-1 },
		{ 17,-14 }
	},
	{
		{ -1,0 },
		{ 3,3 },
		{ 7,-1 },

		{ 9,-1 },
		{ 13,3 },
		{ 17,-8 }
	}
};
	

XPoint cl_grubble::bot_teeth[2][TEETH_POINTS] =
{
	{
		{ 6,5 },
		{ 8,1 },
		{ 10,10 },

		{ 14,12 },
		{ 16,8, },
		{ 18,16 }
	},
	{
		{ 6,1 },
		{ 8,-3 },
		{ 10,6 },

		{ 14,4 },
		{ 16,-2, },
		{ 18,4 }
	}
};


////////////////////////////////// SETUP /////////////////////////////////////

cl_grubble::cl_grubble(): cl_enemy(TYPE_GRUBBLE)
{
	diam = 30;
	radius = 15;
}




void cl_grubble::activate()
{
	cl_enemy::activate();

	setStage(STAGE_MATERIALISE);
	bodynum = 0;
	x = START_X;
	y = -diam;
	dir = STOP;
	dinner = NULL;
	req_angle = 0;
	eating = false;
	dist_to_player = FAR_FAR_AWAY;
	dist_to_food = FAR_FAR_AWAY;

	body_col = PURPLE;
	teeth_col = YELLOW;
	eye_col = WHITE;
	pup_col = BLUE;

	start_speed = speed = (level < 20 ? 2.2 + 0.2 * level : 6.2);
	max_depth = level < 4 ? 3 + level : 7;

	playFGSound(SND_ENEMY_MATERIALISE);
}




/*** For attract mode enemy display ***/
void cl_grubble::attractActivate()
{
	cl_enemy::attractActivate();

	y = 260;
	body_col = PURPLE;
	teeth_col = YELLOW;
	eye_col = WHITE;
	pup_col = BLUE;
	angle = 180;
}


////////////////////////////////// RUNTIME //////////////////////////////////

/*** Terrorise the place ***/
void cl_grubble::run()
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
		if (stage_cnt < 20) return;
		if (ysize > 0.1)
		{
			ysize -= 0.1;
			xsize += 0.1;
		}
		else if (xsize > 0.1) xsize -= 0.2;
		else setStageExplode();
		break;

	case STAGE_EXPLODE:
		if (stage_cnt == 1)
		{
			if (boulder) activateBonusScore(this,300);
			if (eating) activateBonusScore(this,400);
		}
		else if (stage_cnt == 40) setStage(STAGE_INACTIVE);
		break;

	default:
		break;
	}
}




/*** Head towards player, head towards dinner or select dinner ***/
void cl_grubble::stageRun()
{
	double dist;

	// If frozen , do nothing
	if (player->freeze_timer) return;

	// Shouldn't happen but occasionally does
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

	// If we're eating just sit there for a while and then reset
	if (eating)
	{
		// Keep moving for short while so we eat on top of boulder,
		// not to one side of it but if we've overshot then stop.
		dist = distToObject(dinner);
		if (dist < dist_to_food && dist > dinner->radius)
		{
			move();
			dist_to_food = dist;
		}
		if (stage_cnt < eating_time) return;

		eating = false;
		dist_to_food = FAR_FAR_AWAY;
	}

	if (next_tunnel)
	{
		// Next tunnel set, head for it
		setDirection();
		if (dir == STOP) return; 
	}	
	else if (dinner)
	{
		// Dinner set but next_tunnel not because we've reached it
		if (dinner->stage != STAGE_RUN)
		{
			dinner = NULL;
			return;
		}
		else if (curr_tunnel == dinner->curr_tunnel)
			setDirectionToObject(dinner);
		else
		{
			// Check return code just in case its suddenly moved 
			// without us noticing
			if (findShortestPath(
				0,
				max_depth,
				curr_tunnel,
				dinner->curr_tunnel,next_tunnel) == -1)
			{
				dinner = NULL;
				pickRandomTunnel();
			}
			setDirection();
		}
	}
	// If player is invisible just move randomly
	else if (player->invisible_timer)
	{
		pickRandomTunnel();
		setDirection();
	}
	// If we're in the same tunnel as player head for him
	else if (curr_tunnel == player->curr_tunnel) 
		setDirectionToObject(player);
	// Look for some dinner else find player
	else if (!findDinner()) 
	{
		if (findShortestPath(
			0,
			max_depth,
			curr_tunnel,player->curr_tunnel,next_tunnel) == -1)
		{
			pickRandomTunnel();
		}
		setDirection();
	}
	move();
}




/*** Move baed on direction and update angle ***/
void cl_grubble::move()
{
	switch(dir)
	{
	case STOP:
		return;

	case LEFT:
		x -= speed;
		req_angle = 180;
		break;

	case RIGHT:
		x += speed;
		req_angle = 0;
		break;

	case UP:
		y -= speed;
		req_angle = 270;
		break;

	case DOWN:
		y += speed;
		req_angle = 90;
		break;

	default:
		assert(0);
	}	
		
	if (angle != req_angle) attainAngle(angle,req_angle,10);
}




/*** Find a boulder sitting in a tunnel we can munch on ***/
bool cl_grubble::findDinner()
{
	int o;

	FOR_ALL_OBJECTS(o)
	{
		if (objects[o]->type == TYPE_BOULDER && 
		    objects[o]->stage == STAGE_RUN &&
		    objects[o]->curr_tunnel &&
		    findShortestPath(
			0,
			5,
			curr_tunnel,objects[o]->curr_tunnel,next_tunnel) != -1)
		{
			dinner = (cl_boulder *)objects[o];
			setDirection();
			return true;
		}
	}
	return false;
}




/*** Need to check for being hit by falling boulder, hitting it when
     on lunch search or just being blocked. Also check for hitting player. ***/
void cl_grubble::haveCollided(cl_object *obj, double dist)
{
	switch(obj->type)
	{
	case TYPE_BOULDER:
		switch(obj->stage)
		{
		case STAGE_FALL:
			if (stage != STAGE_FALL) 
			{
				boulder = (cl_boulder *)obj;
				incScore(VALUE);
				setStage(STAGE_FALL);
				playFGSound(SND_FALL);
			}
			break;

		case STAGE_RUN:
			// Doesn't matter if its not the actual boulder we
			// picked for dinner. Eat it anyway.
			next_tunnel = NULL;
			eating = true;
			stage_cnt = 0;
			dinner = (cl_boulder *)obj;
			dinner->setBeingEaten();
			playFGSound(SND_GRUBBLE_EAT);
			break;

		case STAGE_WOBBLE:
			if (obj == dinner) dinner = NULL;
			reverseDirection();
			break;

		case STAGE_BEING_EATEN:
			// Tunnel blocked. Reverse.
			if (obj != dinner) reverseDirection();
			break;

		default:
			break;
		}
		break;

	case TYPE_BALL:
		incScore(VALUE);
		setStage(STAGE_HIT);
		playFGSound(SND_GRUBBLE_HIT);
		break;

	case TYPE_PLAYER:
		if (!player->freeze_timer)
		{
			hit_player = true;
			body_col = RED;
		}
		break;

	default:
		break;
	}
}




/*** Paint our lovely visage ***/
void cl_grubble::draw()
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
		bcol = player->freeze_timer ? MEDIUM_BLUE : body_col;
		break;

	case STAGE_FALL:
		break;

	case STAGE_HIT:
		body_col = (body_col == PURPLE ? YELLOW : PURPLE);
		teeth_col = (teeth_col == YELLOW ? PURPLE : YELLOW);
		eye_col = (eye_col == WHITE ? BLUE : WHITE);
		pup_col = (pup_col == BLUE ? WHITE : BLUE);
		break;

	case STAGE_EXPLODE:
		explode->runAndDraw();
		return;

	default:
		assert(0);
	}

	objDrawOrFillPolygon(teeth_col,0,top_teeth[bodynum],TEETH_POINTS,fill);
	objDrawOrFillPolygon(teeth_col,0,bot_teeth[bodynum],TEETH_POINTS,fill);
	objDrawOrFillPolygon(bcol,0,body[bodynum],BODY_POINTS,fill);
	if (ysize > 0.5)
	{
		objDrawOrFillCircle(eye_col,0,10,-10,0,fill);
		objDrawOrFillCircle(pup_col,0,5,-10,0,fill);
	}
}
