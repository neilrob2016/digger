#include "globals.h"

cl_object::cl_object(en_type t)
{
	setStage(STAGE_INACTIVE);
	type = t;
	x = 0;
	y = 0;
	speed = 0;
	dir = STOP;
	angle = 0;
	prev_x = 0;
	prev_y = 0;
	diam = 0;
	radius = 0;
	curr_tunnel = NULL;
	xsize = 1;
	ysize = 1;
}




/*** Rotate points by object angle then draw or fill the polygon ***/
void cl_object::objDrawOrFillPolygon(
	int col, double thick, XPoint *points, int num_points, bool fill)
{
	assert(num_points <= MAX_TMP_POINTS);

	memcpy(tmp_points,points,sizeof(XPoint) * num_points);
	for(int i=0;i < num_points;++i)
	{
		if (angle) rotate(tmp_points[i].x,tmp_points[i].y,angle);
		tmp_points[i].x = (short)((double)tmp_points[i].x * xsize + x);
		tmp_points[i].y = (short)((double)tmp_points[i].y * ysize + y);
	}
	drawOrFillPolygon(col,thick,tmp_points,num_points,fill);
}




/*** Rotate and draw a rectangle ***/
void cl_object::objDrawOrFillRectangle(
	int col, double thick, int xp, int yp, int w, int h, bool fill)
{
	XPoint points[4];

	points[0].x = points[3].x = xp;
	points[1].x = points[2].x = xp + w;

	points[0].y = points[1].y = yp;
	points[2].y = points[3].y = yp + h;

	objDrawOrFillPolygon(col,thick,points,4,fill);
}




/*** Rotate a circle about the object centre based on the angle ***/
void cl_object::objDrawOrFillCircle(
	int col, double thick, double diam, double xp, double yp, bool fill)
{
	diam *= ((xsize + ysize) / 2);

	rotate(xp,yp,angle);
	xp += x;
	yp += y;
	drawOrFillCircle(col,thick,diam,xp,yp,fill);
}




/*** Rotate and draw a line ***/
void cl_object::objDrawLine(
	int col, double thick, double x1, double y1, double x2, double y2)
{
	rotate(x1,y1,angle);
	rotate(x2,y2,angle);

	x1 = x + x1 * xsize;
	y1 = y + y1 * ysize;
	x2 = x + x2 * xsize;
	y2 = y + y2 * ysize;

	drawLine(col,thick,x1,y1,x2,y2);
}




void cl_object::incAngle(double inc)
{
	::incAngle(angle,inc);
}




/*** Set the objects stage and play appropriate sound if required ***/
void cl_object::setStage(en_object_stage stg)
{
	stage = stg;
	stage_cnt = 0;
}




/*** Distance to centre3 of object ***/
double cl_object::distToObject(cl_object *obj)
{
	return hypot(x - obj->x,y - obj->y);
}




/*** If 2 objects are overlapping/touching returning how much by ***/
double cl_object::overlapDist(cl_object *obj)
{
	double dist = radius + obj->radius - distToObject(obj);
	return (dist <= 0 ? 0 : dist);
}
