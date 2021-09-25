#include "globals.h"

/*** Constructor ***/
cl_enemy::cl_enemy(en_type t): cl_object(t)
{
	explode = new cl_explosion(this);
}




/*** (Re)start ***/
void cl_enemy::activate()
{
	prev_tunnel = NULL;
	curr_tunnel = NULL;
	next_tunnel = NULL;
	boulder = NULL;
	prev_dir = STOP;
	start_speed = 0;
	angle = 0;
	xsize = ysize = 1;
	fill = DRAW;
	hit_player = false;

	// Set start tunnel
	curr_tunnel = *(tunnels.begin());
}




/*** For attract mode non playing display enemies ***/
void cl_enemy::attractActivate()
{
	prev_x = x = SCR_SIZE;
	xsize = ysize = 1.5;
	fill = FILL;
	angle = 0;

	setStage(STAGE_RUN);
}




/*** Move from right side of screen ***/
void cl_enemy::attractRun()
{
	if (x > 100) x -= 5; else x = 100;
}




/*** Called by spooky and grubble ***/
void cl_enemy::stageMaterialise()
{
	if ((y += materialise_y_add) >= START_Y)
	{
		y = START_Y;
		setStage(STAGE_RUN);
		fill = FILL;
	}
}




/*** Called when in STAGE_FALL ***/
void cl_enemy::stageFall()
{
	if (ysize > 0.2) ysize *= 0.9;
	y += FALL_SPEED;
	if (boulder->stage != STAGE_FALL) setStageExplode();
}




/*** Enemy is going to blow up ***/
void cl_enemy::setStageExplode()
{
	explode->activate();
	setStage(STAGE_EXPLODE);
	playFGSound(SND_ENEMY_EXPLODE);
}




/*** Move towards the object when in same tunnel as objects ***/
void cl_enemy::setDirectionToObject(cl_object *obj)
{
	assert(curr_tunnel == obj->curr_tunnel);

	if (curr_tunnel->vert)
		dir = (y < obj->y ? DOWN : UP);
	else
		dir = (x < obj->x ? RIGHT : LEFT);
}




/*** Reverse direction. Used by spooky and grubble ***/
void cl_enemy::reverseDirection()
{
	cl_tunnel *tmp = prev_tunnel;

	x = prev_x;
	y = prev_y;
	next_tunnel = prev_tunnel;
	prev_tunnel = tmp;
	setDirection();	
}




/*** Pick a tunnel ours is linked to that seems to head in the general
     direction of the player ***/
void cl_enemy::pickPlayerDirTunnel()
{
	vector<cl_tunnel *>::iterator it;
	cl_tunnel *tun;
	double xd;
	double yd;
	bool vert;

	xd = player->x - x;
	yd = player->y - y;
	vert = (fabs(yd) > fabs(xd));

	for(it=curr_tunnel->links.begin();it != curr_tunnel->links.end();++it)
	{
		tun = *it;
		if (tun == prev_tunnel) continue;

		// Not infallible but should work ok most of the time
		if (vert && 
		    tun->vert &&
		    ((yd < 0 && tun->min_y < curr_tunnel->min_y) ||
		     (yd > 0 && tun->max_y > curr_tunnel->max_y)))
		{
			next_tunnel = tun;
			return;
		}
		if (!vert && 
		    !tun->vert &&
		    ((xd < 0 && tun->min_x < curr_tunnel->min_x) ||
		     (xd > 0 && tun->max_x > curr_tunnel->max_x)))
		{
			next_tunnel = tun;
			return;
		}
	}
	pickRandomTunnel();
}




/*** Pick one of the tunnels in the links ***/
void cl_enemy::pickRandomTunnel()
{
	int cnt = curr_tunnel->links.size();

	switch(cnt)
	{
	case 0:
		/* This can happen if only 1 tunnel created by player at start
		   then player dies, new player tunnel created but not yet
		   linked to first tunnel which then has no links */
		next_tunnel = curr_tunnel;
		return;

	case 1:
		next_tunnel = curr_tunnel->links[0];
		return;
	}

	// Find a new tunnel but not back the way we've come
	do
	{
		next_tunnel = curr_tunnel->links[random() % cnt];
	} while(next_tunnel == prev_tunnel);
}




/*** Set our direction based on where next_tunnel is ***/
void cl_enemy::setDirection()
{
	// This will usually only happen if enemy in very first tunnel and
	// is blocked so switches to previous tunnel which is NULL
	if (!next_tunnel)
	{
		dir = STOP;
		return;
	}
	// Returns STOP if we're within "speed" distance of tunnel
	else dir = dirToTunnel(curr_tunnel,next_tunnel);

	if (dir == STOP)
	{
		// Snap to centre of tunnel
		if (next_tunnel->vert)
			x = next_tunnel->x1;
		else
			y = next_tunnel->y1;
		prev_tunnel = curr_tunnel;
		curr_tunnel = next_tunnel;
		next_tunnel = NULL;
	}
}




/*** This assumes the tunnels are linked. Returns STOP if we've arrived.
     Add TUNNEL_HALF because we dont want enemys to attempt a turn if
     they're not fully aligned with tunnel since they'll move through wall ***/
en_dir cl_enemy::dirToTunnel(cl_tunnel *from, cl_tunnel *to)
{
	if (from->vert)
	{
		// Vertical tunnel next to vertical
		if (to->vert)
		{
			if (y < to->min_y + TUNNEL_HALF) return DOWN;
			if (y > to->max_y - TUNNEL_HALF) return UP;
			if (fabs(x - to->x1) < speed) return STOP;
			return (x < to->x1 ? RIGHT : LEFT);
		}

		// Vertical joined to horizontal
		if (fabs(y - to->y1) < speed) return STOP;
		return (y < to->y1 ? DOWN : UP);
	}
	if (to->vert)
	{
		// Horizontal joined to vertical
		if (fabs(x - to->x1) < speed) return STOP;
		return (x < to->x1 ? RIGHT : LEFT);
	}

	// Horizontal next to horizontal
	if (x < to->min_x + TUNNEL_HALF) return RIGHT;
	if (x > to->max_x - TUNNEL_HALF) return LEFT;
	if (fabs(y - to->y1) < speed) return STOP;
	return (y < to->y1 ? DOWN : UP);
}




/*** This is called by cl_tunnel when a tunnel is being deleted because its 
     on the route of a previous tunnel ***/
void cl_enemy::updateTunnelPtrs(cl_tunnel *oldtun, cl_tunnel *newtun)
{
	if (prev_tunnel == oldtun) prev_tunnel = newtun;

	// Reset next_tunnel to null because it may no longer be appropriate
	if (curr_tunnel == oldtun)
	{
		curr_tunnel = newtun;
		next_tunnel = NULL;
	}
	if (next_tunnel == oldtun) next_tunnel = NULL;
}




/*** This is called when we've hit the player and want to move a short 
     distance over the top of him rather than stopping dead. Used by
     Spooky and Grubble ***/
void cl_enemy::hitPlayerMove()
{
	double dist = distToObject(player);

	// If we're getting further away - eg if we're moving perpendicular 
	// to player and just brushed him - then stop
	if (dist > radius && 
	   (dist_to_player == FAR_FAR_AWAY || dist < dist_to_player))
	{
		move();
		dist_to_player = dist;
	}

	// Expand to attack size
	if (xsize < 1.5)
	{
		xsize += 0.1;
		ysize += 0.1;
	}
}
