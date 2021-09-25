#include "globals.h"

#define SWIDTH          (TUNNEL_HALF - 5)
#define ANGLE_INC        4
#define AUTOPLAY_REV_MOD 5

XPoint cl_player::square1[NUM_POINTS] =
{
	{ -SWIDTH,-SWIDTH },
	{ SWIDTH,-SWIDTH },
	{ SWIDTH,SWIDTH },
	{ -SWIDTH,SWIDTH }
};


XPoint cl_player::square2[NUM_POINTS] =
{
	{ -SWIDTH,-SWIDTH },
	{ SWIDTH,-SWIDTH },
	{ SWIDTH,SWIDTH },
	{ -SWIDTH,SWIDTH }
};


////////////////////////////////// START /////////////////////////////////////


/*** Constructor ***/
cl_player::cl_player(): cl_object(TYPE_PLAYER)
{
	for(int i=0;i < NUM_POINTS;++i) rotate(square2[i].x,square2[i].y,45);
	explode = new cl_explosion(this);
}




/*** Does a (re)set at the start of the level ***/
void cl_player::activate()
{
	setStage(STAGE_RUN);
	prev_x = start_x = x = START_X;
	prev_y = start_y = y = START_Y;
	xsize = ysize = 1;
	diam = square2[1].x * 2;
	radius = diam / 2;
	prev_dir = STOP;
	facing_dir = RIGHT;
	ball_ang = 90;
	req_ball_ang = 90;
	col = GREEN;
	superball = false;
	fill = FILL;
	dir = STOP;
	hit_object = false;
	hit_edge = false;

	resetTimers();

	// Create start tunnel
	curr_tunnel = createTunnel(START_X,START_Y);

	// If theres a pre-existing start tunnel then link to it so enemies
	// can follow us along it
	if (tunnels.size() > 1)
	{
		prev_tunnel = tunnels[0];
		curr_tunnel->linkTunnel(prev_tunnel);
	}
	else prev_tunnel = NULL;

	speed = level < 20 ? 1.3 + 0.15 * level : 4.3;

	switch(level)
	{
	case 1:
		ball_ang_inc = 5;
		break;

	case 2:
		ball_ang_inc = 6;
		break;

	case 3:
	case 4:
		ball_ang_inc = 7;
		break;

	case 5:
	case 6:
		ball_ang_inc = 8;
		break;

	case 7:
	case 8:
		ball_ang_inc = 9;
		break;

	default:
		ball_ang_inc = 10;
	}

	setBallPos();
	echoOff();
	playBGSound(SND_SILENCE);
}




/*** Reset the power-up timers ***/
void cl_player::resetTimers()
{
	invisible_timer = 0;
	freeze_timer = 0;
	turbo_enemy_timer = 0;
}


/////////////////////////////////// RUNTIME /////////////////////////////////

/*** Carry out appropriate run actions ***/
void cl_player::run()
{
	switch(stage)
	{
	case STAGE_RUN:
		if (game_stage == GAME_STAGE_ATTRACT_PLAY)
			autoplay();
		else
			stageRun();
		return;

	case STAGE_FALL:
		stageFall();
		return;

	case STAGE_HIT:
		xsize += xsize_inc;
		if (xsize <= 0.5 || xsize >= 1.5) xsize_inc = -xsize_inc;
		ysize += ysize_inc;
		if (ysize <= 0.5 || ysize >= 1.5) ysize_inc = -ysize_inc;
		if (++stage_cnt == 50)
		{
			explode->activate();
			setStage(STAGE_EXPLODE);	
			playFGSound(SND_PLAYER_EXPLODE);
		}
		break;

	case STAGE_EXPLODE:
		// Game stage updated in main.cc:run()
		++stage_cnt;
		break;

	default:
		assert(0);
	}
}


	

/*** During games attract mode ***/
void cl_player::autoplay()
{
	cl_object *obj;
	double xd;
	double yd;
	int o;

	switch(game_stage_cnt)
	{
	case 1:
		move(XK_Down);
		break;

	case 100:
		move(XK_Up);
		break;

	case 101:
		stop(XK_Up);
		break;

	case 130:
		throwBall();
		break;

	case 160:
		move(random() % 2 ? XK_Left : XK_Right);
		break;

	case 250:
		move(random() % 2 ? XK_Up : XK_Down);
		break;

	default:
		if (hit_edge)
		{
			hit_edge = false;
			autoplayRandomMove();
			break;
		}

		if (hit_object)
		{
			hit_object = false;
			autoplayRandomMove();
			break;
		}

		// See if an enemy is near and if so move away
		FOR_ALL_OBJECTS(o)
		{
			obj = objects[o];
			if (obj->stage != STAGE_RUN && 
			    obj->stage != STAGE_MATERIALISE) continue;

			switch(obj->type)
			{
			case TYPE_SPOOKY:
			case TYPE_GRUBBLE:
			case TYPE_WURMAL:
			case TYPE_SPIKY:
				if (distToObject(obj) < 100)
				{
					throwBall();
					xd = x - obj->x;
					yd = y - obj->y;
					if (fabs(xd) > fabs(yd))
						move(xd < 0 ? XK_Left : XK_Right);
					else
						move(yd < 0 ? XK_Up : XK_Down);
					stageRun();
					return;
				}

			default:
				break;
			}
		}

		if (game_stage_cnt > 300 && !(random() % 100))
			autoplayRandomMove();
	}
		
	stageRun();
}




/*** Pick a random move that is usually 90 degrees to our current direction ***/
void cl_player::autoplayRandomMove()
{
	switch(prev_dir)
	{
	case STOP:
		switch(random() % 4)
		{
		case 0:
			move(XK_Up);
			break;

		case 1:
			move(XK_Down);
			break;

		case 2:
			move(XK_Left);
			break;

		case 3:
			move(XK_Right);
		}
		break;

	case UP:
		// Theres a 1 in 5 chance of reversing direction
		if (!(random() % AUTOPLAY_REV_MOD))
			move(XK_Down);
		else
			move(random() % 2 ? XK_Left : XK_Right);
		break;

	case DOWN:
		if (!(random() % AUTOPLAY_REV_MOD))
			move(XK_Up);
		else
			move(random() % 2 ? XK_Left : XK_Right);
		break;

	case LEFT:
		if (!(random() % AUTOPLAY_REV_MOD))
			move(XK_Right);
		else
			move(random() % 2 ? XK_Up : XK_Down);
		break;

	case RIGHT:
		if (!(random() % AUTOPLAY_REV_MOD))
			move(XK_Left);
		else
			move(random() % 2 ? XK_Up : XK_Down);
		break;
	}
}




/*** For STAGE_RUN ***/
void cl_player::stageRun()
{
	prev_x = x;
	prev_y = y;

	// Check and set powerup counts
	if (invisible_timer)
	{
		if (!--invisible_timer)
		{
			fill = FILL;
			playBGSound(SND_SILENCE);
		}
	}
	if (turbo_enemy_timer)
	{
		if (!--turbo_enemy_timer)
		{
			setGroundColour();
			playBGSound(SND_SILENCE);
		}
		else ground_colour = BLACK2 + (turbo_enemy_timer * 2 % 15);
	}
	if (freeze_timer)
	{
		if (!--freeze_timer)
		{
			setGroundColour();
			echoOff();
		}
		else ground_colour = MEDIUM_BLUE;
	}

	if (ball_ang != req_ball_ang)
		attainAngle(ball_ang,req_ball_ang,ball_ang_inc);

	setBallPos();

	// Deal with movement. If we're stopped do nothing.
	switch(dir)
	{
	case STOP : return;
	case LEFT : x -= speed;  break;
	case RIGHT: x += speed;  break;
	case UP   : y -= speed;  break;
	case DOWN : y += speed;  break;

	default   : assert(0);
	}

	if (x > SCR_SIZE - TUNNEL_HALF)
	{
		x = SCR_SIZE - TUNNEL_HALF;
		dir = STOP;
		hit_edge = true;
	}
	else if (x < TUNNEL_HALF) 
	{
		x = TUNNEL_HALF;
		dir = STOP;
		hit_edge = true;
	}

	if (y < PLAY_AREA_TOP + TUNNEL_HALF)
	{
		y = PLAY_AREA_TOP + TUNNEL_HALF;
		dir = STOP;
		hit_edge = true;
	}
	else if (y > SCR_SIZE - TUNNEL_HALF)
	{
		y = SCR_SIZE - TUNNEL_HALF;
		dir = STOP;
		hit_edge = true;
	}
	curr_tunnel->update((int)x,(int)y);

	switch(dir)
	{
	case STOP:
		break;

	case LEFT : 
		fillTunnelArea(
			(int)x - TUNNEL_HALF,
			(int)y - TUNNEL_HALF,
			(int)prev_x - TUNNEL_HALF,
			(int)y + TUNNEL_HALF);
		incAngle(-ANGLE_INC);
		break;

	case RIGHT:
		fillTunnelArea(
			(int)prev_x + TUNNEL_HALF,
			(int)y - TUNNEL_HALF,
			(int)x + TUNNEL_HALF,
			(int)y + TUNNEL_HALF);
		incAngle(ANGLE_INC);
		break;

	case UP:
		fillTunnelArea(
			(int)x - TUNNEL_HALF,
			(int)y - TUNNEL_HALF,
			(int)x + TUNNEL_HALF,
			(int)prev_y - TUNNEL_HALF);
		incAngle(-ANGLE_INC);
		break;

	case DOWN:
		fillTunnelArea(
			(int)x - TUNNEL_HALF,
			(int)prev_y + TUNNEL_HALF,
			(int)x + TUNNEL_HALF,
			(int)y + TUNNEL_HALF);
		incAngle(ANGLE_INC);
		break;
	}
}




/*** For STAGE_FALL ***/
void cl_player::stageFall()
{
	setBallPos();

	if (ysize > 0.2) ysize *= 0.9;
	y += FALL_SPEED;

	if (boulder->stage != STAGE_FALL)
	{
		setStage(STAGE_EXPLODE);
		playFGSound(SND_PLAYER_EXPLODE);
		explode->activate();
	}
}




/*** Set the start position of the ball ***/
void cl_player::setBallPos()
{
	ball_x = 0;
	ball_y = -16;
	rotate(ball_x,ball_y,ball_ang);
	ball_x += x;
	ball_y += y;
}




/*** Digging of current tunnel is finished either because we've changed
     direction or because we've been killed ***/
void cl_player::tunnelComplete()
{
	cl_tunnel *tun;

	// Returns pointer to pre-existing tunnel if we don't need current one
	if ((tun = curr_tunnel->complete()))
	{
		delete curr_tunnel;
		prev_tunnel = tun;
	}
	else prev_tunnel = curr_tunnel;
}

//////////////////////////// KEYBOARD FUNCTIONS ////////////////////////////

/*** Keyboard key pressed so this is called ***/
void cl_player::move(KeySym key)
{
	if (stage != STAGE_RUN) return;

	switch(key)
	{
	case XK_Left:
		dir = LEFT;
		req_ball_ang = 270;
		break;

	case XK_Right:
		dir = RIGHT;
		req_ball_ang = 90;
		break;

	case XK_Up:
		dir = UP;
		req_ball_ang = 0;
		break;

	case XK_Down:
		dir = DOWN;
		req_ball_ang = 180;
		break;

	default:
		assert(0);
	}
	facing_dir = dir;

	// New direction
	if (dir != prev_dir)
	{
		// Changed direction
		if (prev_dir != STOP)
		{
			tunnelComplete();

			start_x = x;
			start_y = y;
			curr_tunnel = createTunnel((int)x,(int)y);
			curr_tunnel->linkTunnel(prev_tunnel);
		}
		prev_dir = dir;
	}
}




/*** Key lifted so stop moving ***/
void cl_player::stop(KeySym key)
{
	if (dir == STOP) return;

	switch(key)
	{
	case XK_Left:
		if (dir == LEFT) break;
		return;

	case XK_Right:
		if (dir == RIGHT) break;
		return;

	case XK_Up:
		if (dir == UP) break;
		return;

	case XK_Down:
		if (dir == DOWN) break;
		return;

	default:
		assert(0);
	}
	dir = STOP;
}




/*** Throw the ball if its not already in play ***/
void cl_player::throwBall()
{
	if (stage == STAGE_RUN && ball->stage == STAGE_INACTIVE)
	{
		playFGSound(SND_BALL_THROW);
		ball->activate();
		superball = false;
		playBGSound(SND_SILENCE);
	}
}



////////////////////////////// OTHER OVERLOADS /////////////////////////////

/*** Do something when we collide with another object ***/
void cl_player::haveCollided(cl_object *obj, double dist)
{
	cl_nugget *nug;

	if (stage != STAGE_RUN) return;

	switch(obj->type)
	{
	case TYPE_NUGGET:
		nug = (cl_nugget *)obj;

		switch(nug->nugtype)
		{
		case cl_nugget::WURMALLED:
		case cl_nugget::NORMAL:
			break;

		case cl_nugget::INVISIBILITY:
			invisible_timer = level < 10 ? 350 - level * 10 : 250;
			fill = DRAW;
			playBGSound(SND_INVISIBILITY_POWERUP);
			text_invisibility_powerup->reset(this,0);
			break;

		case cl_nugget::SUPERBALL:
			superball = true;
			playBGSound(SND_SUPERBALL_POWERUP);
			text_superball_powerup->reset(this,0);
			break;

		case cl_nugget::FREEZE:
			freeze_timer = level < 10 ? 250 - level * 10 : 150;
			echoOn();
			playFGSound(SND_FREEZE_POWERUP);
			text_freeze_powerup->reset(this,0);
			break;

		case cl_nugget::BONUS:
			if (nug->give_bonus) activateBonusScore(this,300);
			break;

		case cl_nugget::TURBO_ENEMY:
			turbo_enemy_timer = 100 + 10 * level;
			playBGSound(SND_TURBO_ENEMY);
			break;

		default:
			assert(0);
		}
		playFGSound(SND_EAT_NUGGET);
		break;

	case TYPE_BOULDER:
		if (obj->stage == STAGE_FALL)
		{
			// Give a bit of leaway so player isn't killed by
			// a small graze
			if (dist > 10)
			{
				boulder = obj;
				tunnelComplete();
				setStage(STAGE_FALL);
				playFGSound(SND_FALL);
			}
			return;
		}

		// Allow player a bit of leaway so not blocked by boulder 
		// just by touching it
		if (dist < 10) return;

		// If we're going left or right we might be able to push
		// the boulder
		if (dir == LEFT || dir == RIGHT)
		{
			x = prev_x;
			x += ((cl_boulder *)obj)->push(dir);
		}
		else
		{
			y = prev_y;
			dir = STOP;
		}
		hit_object = true;
		break;
		
	case TYPE_WURMAL:
		// Immune to wurmals when invisible
		if (invisible_timer) break;
		// Fall through

	case TYPE_SPOOKY:
	case TYPE_SPIKY:
	case TYPE_GRUBBLE:
		if (obj->stage != STAGE_RUN || freeze_timer) break;
		xsize_inc = -0.2;
		ysize_inc = 0.2;
		tunnelComplete();
		setStage(STAGE_HIT);
		playFGSound(SND_PLAYER_HIT);
		break;

	default:
		break;
	}
}




/*** Draw rotating body and ball holder + ball if its there ***/
void cl_player::draw()
{
	double ball_diam;
	int ball_col;

	if (stage == STAGE_EXPLODE)
	{
		explode->runAndDraw();
		return;
	}

	// Draw body
	objDrawOrFillPolygon((int)col,0,square1,NUM_POINTS,fill);
	objDrawOrFillPolygon(
		(int)(col + BLUE) % GREEN2,0,square2,NUM_POINTS,fill);

	if (stage == STAGE_RUN)
	{
		col += 0.2;
		if (col >= GREEN2) col = GREEN;
	}

	// Draw ball holder. Based on angle = 0
	drawLine(YELLOW,5*xsize,x,y,ball_x,ball_y);

	// Draw ball or its little holding cup if its not there
	if (ball->stage == STAGE_INACTIVE)
	{
		if (superball)
		{
			ball_diam = 10 + (game_stage_cnt % 20);
			ball_col = random() % NUM_FULL_COLOURS;
		}
		else
		{
			ball_diam = ball->diam;
			ball_col = BALL_COLOUR;
		}
		ball_diam *= xsize;

		drawOrFillCircle(ball_col,0,ball_diam,ball_x,ball_y,fill);
	}
	else drawOrFillCircle(YELLOW,0,ball->diam / 2 * xsize,ball_x,ball_y,fill);
}
