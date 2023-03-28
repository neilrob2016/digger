#include "globals.h"

/*** Constructor ***/
cl_tunnel::cl_tunnel(int nx1, int ny1)
{
	x1 = x2 = nx1;
	y1 = y2 = ny1;
	min_x = x1 - TUNNEL_HALF;
	max_x = x2 + TUNNEL_HALF;
	min_y = y1 - TUNNEL_HALF;
	max_y = y1 + TUNNEL_HALF;
	vert = true;
	recursed = false;
}




/*** Called while tunnel is being created ***/
void cl_tunnel::update(int nx2, int ny2)
{
	x2 = nx2;
	y2 = ny2;
	vert = (x1 == x2);
	setMaxMin();
}




/*** Tunnel is now complete. If we're on the same route as a previous tunnel
     then remove from list ***/
cl_tunnel *cl_tunnel::complete()
{
	cl_tunnel *tun;
	cl_enemy *mon;

	// If no simmilar tunnels just link us to other tunnels
	if (!(tun = checkForSimilar())) 
	{
		setMaxMin(); // Probably not needed cos in update() but...
		setLinks();
		return NULL;
	}

	// If we're here we're on the route of another tunnel and will be
	// deleted. We'll be last on the list so can just pop
	tunnels.pop_back();

	if (player->prev_tunnel) 
	{
		assert(*(player->prev_tunnel->links.rbegin()) == this);
		player->prev_tunnel->links.pop_back();
	}
	tun->setMaxMin();
	tun->setLinks();

	// Tell appropriate objects to update their tunnel pointers
	for(auto obj: objects)
	{
		if (obj->stage == STAGE_INACTIVE) continue;

		switch(obj->type)
		{
		case TYPE_PLAYER:
		case TYPE_BALL:
		case TYPE_NUGGET:
		case TYPE_BOULDER:
		case TYPE_SPIKY:
			break;

		case TYPE_SPOOKY:
		case TYPE_GRUBBLE:
		case TYPE_WURMAL:
			mon = (cl_enemy *)obj;
			mon->updateTunnelPtrs(this,tun);
			break;

		default:
			assert(0);
		}
	}
	return tun;
}




/*** Find a tunnel we're simply another part of  - we then extend that ***/
cl_tunnel *cl_tunnel::checkForSimilar()
{
	for(auto tun: tunnels)
	{
		if (tun == this || vert != tun->vert) continue;

		// If we're a continuation of a tunnel just update that
		// tunnel and delete this one
		if (vert && 
		    x1 == tun->x1 && 
		    overlapLen(min_y,max_y,tun->min_y,tun->max_y))
		{
			if (tun->y1 < tun->y2)
			{
				if (y1 < tun->y1) tun->y1 = y1;
				else
				if (y1 > tun->y2) tun->y2 = y1;

				if (y2 < tun->y1) tun->y1 = y2;
				else
				if (y2 > tun->y2) tun->y2 = y2;
			}
			else
			{
				if (y1 < tun->y2) tun->y2 = y1;
				else
				if (y1 > tun->y1) tun->y1 = y1;

				if (y2 < tun->y2) tun->y2 = y2;
				else
				if (y2 > tun->y1) tun->y1 = y2;
			}
			return tun;
		}

		if (!vert &&
		    y1 == tun->y1 &&
		    overlapLen(min_x,max_x,tun->min_x,tun->max_x))
		{
			if (tun->x1 < tun->x2)
			{
				if (x1 < tun->x1) tun->x1 = x1;
				else
				if (x1 > tun->x2) tun->x2 = x1;

				if (x2 < tun->x1) tun->x1 = x2;
				else
				if (x2 > tun->x2) tun->x2 = x2;
			}
			else
			{
				if (x1 < tun->x2) tun->x2 = x1;
				else
				if (x1 > tun->x1) tun->x1 = x1;

				if (x2 < tun->x2) tun->x2 = x2;
				else
				if (x2 > tun->x1) tun->x1 = x2;
			}
			return tun;
		}
	}
	return NULL;
}




/*** Max and min are the corners of the tunnel ***/
void cl_tunnel::setMaxMin()
{
	if (vert)
	{
		if (y1 < y2)
		{
			if (y1 - TUNNEL_HALF < min_y) min_y = y1 - TUNNEL_HALF;
			if (y2 + TUNNEL_HALF > max_y) max_y = y2 + TUNNEL_HALF;
		}
		else 
		{
			if (y2 - TUNNEL_HALF < min_y) min_y = y2 - TUNNEL_HALF;
			if (y1 + TUNNEL_HALF > max_y) max_y = y1 + TUNNEL_HALF;
		}
	}
	else
	{
		if (x1 < x2)
		{
			if (x1 - TUNNEL_HALF < min_x) min_x = x1 - TUNNEL_HALF;
			if (x2 + TUNNEL_HALF > max_x) max_x = x2 + TUNNEL_HALF;
		}
		else 
		{
			if (x2 - TUNNEL_HALF < min_x) min_x = x2 - TUNNEL_HALF;
			if (x1 + TUNNEL_HALF > max_x) max_x = x1 + TUNNEL_HALF;
		}
	}
}




/*** Link us to any other tunnels that we overlap ***/
void cl_tunnel::setLinks()
{
	vector<cl_tunnel *>::iterator it;
	int xlen;
	int ylen;

	for(auto tun: tunnels)
	{
		if (tun != this && 
		    find(links.begin(),links.end(),tun) == links.end())
		{
			/* Both overlaps must be > 0 and at least one must
			   be >= TUNNEL_WIDTH otherwise enemies would move
			   through the wall */
			xlen = overlapLen(min_x,max_x,tun->min_x,tun->max_x);
			ylen = overlapLen(min_y,max_y,tun->min_y,tun->max_y);
			if (xlen && ylen && 
			    (xlen >= TUNNEL_WIDTH || ylen >= TUNNEL_WIDTH))
				linkTunnel(tun);
		}
	}
}




/*** Link us to the other tunnel ***/
void cl_tunnel::linkTunnel(cl_tunnel *tun)
{
	links.push_back(tun);
	tun->links.push_back(this);
}




/*** Returns length of overlap. Always called with min - max in order so 
     don't have to worry about v1 < v2 or v3 < v4 ***/
int cl_tunnel::overlapLen(int v1, int v2, int v3, int v4)
{
	assert(v1 <= v2 && v3 <= v4);

	if (v1 >= v4 || v2 <= v3) return 0;

	if (v1 >= v3)
	{
		/* 3-------4
		     1---2   */
		if (v2 <= v4) return v2 - v1;

		/* 3-------4
		     1--------2 */
		return v4 - v1;
	}

	/*  3-------4
	 1------2     */
	if (v2 <= v4) return v2 - v3;

	/*  3-------4
	 1------------2 */
	return v4 - v3;
}




void cl_tunnel::draw()
{
	drawOrFillRectangle(
		COL_BLACK,
		0,min_x,min_y,max_x - min_x + 1,max_y - min_y + 1,FILL);
}


void cl_tunnel::draw2()
{
	drawLine(COL_RED,2,x1,y1,x2,y2);
}
