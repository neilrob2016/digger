#include "globals.h"


/*** Constructor ***/
cl_rock::cl_rock(en_type t): cl_object(t)
{
	points = NULL;
}




/*** Create the rock shape ***/
void cl_rock::activate()
{
	double ang_inc;
	double len;
	int i;

	xsize = 1;
	ysize = 1;
	fill = true;

	setStage(STAGE_RUN);

	// Set up diameter and point count
	switch(type)
	{
	case TYPE_STONE:
		diam = 8;
		num_points = 5 + random() % 5;
		break;

	case TYPE_NUGGET:
		diam = 30;
		num_points = 5 + random() % 5;
		break;

	case TYPE_BOULDER:
		diam = 60;
		num_points = 10 + random() % 10;
		break;

	case TYPE_SMALL_BOULDER:
		diam = 20 + random() % 10;
		num_points = 5 + random() % 5;
		break;

	default:
		assert(0);
	}

	radius = diam / 2;
	ang_inc = (double)360 / num_points;

	// Create the shape
	if (points) delete points;

	points = new XPoint[num_points];
	for(i=0,angle=0;i < num_points;++i,angle+=ang_inc)
	{
		len = (double)radius - 
		      ((double)(random() % radius) - (double)(radius / 2)) / 4;
		points[i].x = (short)(SIN(angle) * len);
		points[i].y = (short)(COS(angle) * len);
	}
}




/*** Draw shape ***/
void cl_rock::draw()
{
	objDrawOrFillPolygon((int)col,0,points,num_points,fill);
}

