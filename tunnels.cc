#include "globals.h"


/*** Create a tunnel and add it to the list. Only have 1 coordinate pair because
     we don't know which direction tunnel will go in yet ***/
cl_tunnel *createTunnel(int x, int y)
{
	cl_tunnel *tun = new cl_tunnel(x,y);
	tunnels.push_back(tun);
	return tun;
}




/*** Cleardown everything and set up the start tunnels ***/
void initTunnels()
{
	vector<cl_tunnel *>::iterator it;
	cl_tunnel *tun;
	int vert_y;
	int horiz_x1;
	int horiz_x2;
	int val;

	for(it=tunnels.begin();it != tunnels.end();++it) delete *it;
	tunnels.clear();
	tunnels.reserve(20);

	memset(tunnel_bitmap,1,sizeof(tunnel_bitmap));

	val = 30 * level;

	// Create vertical start tunnel
	vert_y = SCR_MID + 150 + val;
	if (vert_y > SCR_SIZE) vert_y = SCR_SIZE;

	tun = createTunnel(START_X,START_Y);
	tun->update(START_X,vert_y - TUNNEL_HALF);

	fillTunnelArea(
		START_X - TUNNEL_HALF,
		START_Y - TUNNEL_HALF,
		START_X + TUNNEL_HALF,
		vert_y);

	// Create horizontal start tunnel
	horiz_x1 = SCR_MID - 150 - val;
	if (horiz_x1 < 0) horiz_x1 = 0;
	horiz_x2 = SCR_MID + 150 + val;
	if (horiz_x2 > SCR_SIZE) horiz_x2 = SCR_SIZE;

	tun = createTunnel(horiz_x1 + TUNNEL_HALF,SCR_MID);
	tun->update(horiz_x2 - TUNNEL_HALF,SCR_MID);

	fillTunnelArea(
		horiz_x1,SCR_MID - TUNNEL_HALF,horiz_x2,SCR_MID + TUNNEL_HALF);
}




/*** Fill an area with a tunnel - used for movement ***/
void fillTunnelArea(int x1, int y1, int x2, int y2)
{
	int x;
	int y;

	assert(x1 <= x2 && y1 <= y2);

	for(x=x1;x <= x2;++x)
	{
		if (x < 0 || x >= SCR_SIZE) continue;
		for(y=y1;y <= y2;++y)
		{
			if (y >= 0 && y < SCR_SIZE) tunnel_bitmap[x][y] = 0;
		}
	}
}




/*** Return one if co-ordinate is outside a tunnel or offscreen ***/
bool outsideTunnel(int x, int y)
{
	return (offscreen(x,y) || tunnel_bitmap[x][y]);
}




/*** More efficient that just not'ing the above all the time ***/
bool insideTunnel(int x, int y)
{
	return !outsideTunnel(x,y);
}




/*** Find the shortest path - in number of links , not necessarily distance
     but frankly who cares - from given tunnel to next tunnel ***/
int findShortestPath(
	int depth,
	int max_depth, cl_tunnel *from, cl_tunnel *to, cl_tunnel *&next)
{
	vector<cl_tunnel *>::iterator it;
	vector<cl_tunnel *>::iterator best_it;
	bool first;
	int res;
	int min = -1;

	if (from == to)
	{
		next = to;
		return 0;
	}
	if (from->recursed || depth == max_depth) return -1;

	from->recursed = true;
	first = true;

	for(it=from->links.begin();it != from->links.end();++it)
	{
		res = findShortestPath(depth+1,max_depth,*it,to,next);
		if (res != -1 && (first || res < min))
		{
			best_it = it;
			min = res;
			first = false;
		}
	}
	from->recursed = false;

	if (min == -1) return -1;
	next = *best_it;
	return 1 + min;
}
