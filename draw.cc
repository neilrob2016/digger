// X windows drawing functions

#include "globals.h"

///////////////////////////// HIGH LEVEL DRAWING /////////////////////////////

/*** Draw ascii table. For debugging ***/
void drawAsciiTable()
{
	int x;
	int y;
	int c;
	int c2;

	drawText("*** ASCII TABLE ***",LIGHT_BLUE,2,0,0,1.5,2,SCR_MID - 210,30);

	x = 140;
	y = 80;
	for(c=0;c < (256 - 32);++c)
	{
		c2 = c + 32;
		if (!(c % 25))
		{
			y += 60;
			x = 20;
		}
		else x += (CHAR_SIZE * 2 + CHAR_GAP);

		if (ascii_table[c2]) drawChar((char)c2,WHITE,2,0,2,4,x,y);
	}

	drawText("MADE",RED,2,0,0,1.5,2,SCR_MID - 145,SCR_SIZE - 20);
	drawText("IN",WHITE,2,0,0,1.5,2,SCR_MID - 45,SCR_SIZE - 20);
	drawText("ENGLAND",BLUE,2,0,0,1.5,2,SCR_MID + 10,SCR_SIZE - 20);
}




/*** Draw the actual game screen ***/
void drawGameScreen()
{
	vector<cl_tunnel *>::iterator it1;
	char text[20];
	int i;

	drawText("SCORE:",TURQUOISE,2,0,0,0.75,1,10,10);
	drawText(score_text,GREEN,2,0,0,1,1,85,10);

	drawText("HIGH :",TURQUOISE,2,0,0,0.75,1,10,25);
	if (!done_high_score || (game_stage_cnt % 30) < 15)
		drawText(high_score_text,PURPLE,2,0,0,1,1,85,25);

	drawText("LIVES:",TURQUOISE,2,0,0,0.75,1.5,SCR_SIZE - 85,15);
	drawText(lives_text,RED,2,0,0,1,1.5,SCR_SIZE - 10,15);

	// Draw player powerup countdowns
	if (player->invisible_timer) 
	{
		sprintf(text,"%03d",player->invisible_timer);
		drawText(text,YELLOW,2,0,0,1,1.5,SCR_MID-15,15);
	}
	else if (player->freeze_timer)
	{
		sprintf(text,"%03d",player->freeze_timer);
		drawText(text,TURQUOISE,2,0,0,1,1.5,SCR_MID-15,15);
	}

	// Draw ground, molehills and lines at top so theres always a roof on 
	// top tunnels
	drawOrFillRectangle(
		ground_colour,0,0,PLAY_AREA_TOP,SCR_SIZE,PLAY_AREA_HEIGHT,FILL);
	for(i=0;i < NUM_MOLEHILLS;++i) molehill[i].draw();
	drawLine(
		ground_colour,4,
		0,PLAY_AREA_TOP-2,START_X-TUNNEL_HALF-2,PLAY_AREA_TOP-2);
	drawLine(
		ground_colour,4,
		START_X+TUNNEL_HALF+2,PLAY_AREA_TOP-2,SCR_SIZE,PLAY_AREA_TOP-2);

	// Draw stones
	for(i=0;i < MAX_STONES;++i) stones[i]->draw();

	// Draw tunnels
	FOR_ALL_TUNNELS(it1) (*it1)->draw();

	// Draw game objects
	FOR_ALL_OBJECTS(i)
		if (objects[i]->stage != STAGE_INACTIVE) objects[i]->draw();

	switch(game_stage)
	{
	case GAME_STAGE_ATTRACT_PLAY:
		text_digger->draw();
		text_s_to_start->draw();
		text_copyright->draw();
		drawText(version_text,WHITE,1,0,0,0.8,1.2,170,SCR_SIZE - 10);
		break;

	case GAME_STAGE_LEVEL_START:
		text_level_start->draw();
		break;

	case GAME_STAGE_READY:
		text_ready->draw();
		break;

	case GAME_STAGE_PLAY:
		if (paused) text_paused->draw();
		else
		{
			text_new_high_score->draw();
			text_bonus_life->draw();
		}
		break;

	case GAME_STAGE_LEVEL_COMPLETE:
		if (game_stage_cnt % 40 < 20)
			drawText("LEVEL COMPLETE",LIGHT_BLUE,8,0,0,2.8,7,52,SCR_MID - 30);
		if (end_of_level_bonus)
			drawText(end_of_level_bonus_str,WHITE,6,0,0,2,6,175,SCR_MID + 60);
		break;

	case GAME_STAGE_GAME_OVER:
		text_game_over->draw();
		break;

	default:
		break;
	}	

	for(i=0;i < NUM_BONUS_SCORES;++i)
		if (text_bonus_score[i]->running) text_bonus_score[i]->draw();
	if (text_got_spiky->running) text_got_spiky->draw();

	text_invisibility_powerup->draw();
	text_superball_powerup->draw();
	text_freeze_powerup->draw();

#ifdef SOUND
	if (!do_sound && !IN_ATTRACT_MODE())
	{
		drawText("S",WHITE,2,0,0,2,1.5,SCR_SIZE - 190,15);
		drawText("/",RED,5,0,0,2,1.5,SCR_SIZE - 190,15);
	}
#endif
}




/*** Draw the screen showing the enemies ***/
void drawEnemyScreen()
{
	static const char *name[NUM_ATTRACT_ENEMIES] = 
	{
		"SPOOKY   - 100 POINTS",
		"GRUBBLE  - 200 POINTS",
		"WURMAL   - 400 POINTS",
		"SPIKY    - 800 POINTS",
	};
	static int colour[NUM_ATTRACT_ENEMIES] =
	{
		LIGHT_BLUE,
		PURPLE,
		TURQUOISE,
		GREEN
	};

	drawText(
		"*** THE ENEMIES ***",
		random() % NUM_FULL_COLOURS,2,0,0,1.5,2,SCR_MID - 200,30);

	for(int i=0;i < NUM_ATTRACT_ENEMIES;++i)
	{
		attract_enemy[i]->draw();

		drawText(
			name[i],colour[i],4,0,0,1,2,
			attract_enemy[i]->x + 100,attract_enemy[i]->y);
	}
}


#define KX 50
#define KY 120

/*** Display the list of keys ***/
void drawKeysScreen()
{
	drawText("*** CONTROL KEYS ***",LIGHT_BLUE,2,0,0,1.5,2,SCR_MID - 210,30);

	drawText("'S'",ORANGE,2,0,0,1,2,KX,KY);
	drawText(": START",WHITE,2,0,0,1,2,KX + 50,KY);

	drawText("'Q'",ORANGE,2,0,0,1,2,KX,KY + 40);
	drawText(": QUIT",WHITE,2,0,0,1,2,KX + 50,KY + 40);

	drawText("'P'",ORANGE,2,0,0,1,2,KX,KY + 80);
	drawText(": PAUSE",WHITE,2,0,0,1,2,KX + 50,KY + 80);

#ifdef SOUND
	drawText("'V'",ORANGE,2,0,0,1,2,KX,KY + 120);
	drawText(": SOUND ON/OFF",WHITE,2,0,0,1,2,KX + 50,KY + 120);
#endif

	drawText("ARROW KEYS",MAUVE,2,0,0,1,2,KX,KY + 200);
	drawText(": MOVE",WHITE,2,0,0,1,2,KX + 160,KY + 200);

	drawText("SPACEBAR",MAUVE,2,0,0,1,2,KX,KY + 240);
	drawText(": THROW BALL",WHITE,2,0,0,1,2,KX + 160,KY + 240);
}


///////////////////////////////// TEXT DRAWING ////////////////////////////////

/*** Draw some text ***/
void drawText(
	const char *mesg,
	int col,
	int thick,
	double ang,
	double gap,
	double x_scale, double y_scale, double x, double y)
{
	int len = strlen(mesg);

	if (!gap) gap = CHAR_GAP;

	for(int i=0;i < len;++i)
	{
		drawChar(
			mesg[i],col,thick,ang,x_scale,y_scale,
			x + i * gap  * x_scale + i * CHAR_SIZE * x_scale,y);
	}
}




/*** Draw a character ***/
void drawChar(
	char c,
	int col,
	int thick,
	double ang, double x_scale, double y_scale, double x, double y)
{
	st_char_template *tmpl;
	double x1;
	double y1;
	double x2;
	double y2;

	if (!(tmpl = ascii_table[(int)c])) return;

	// Draw character
	for(int i=0;i < tmpl->cnt;i+=2)
	{

		x1 = x + ((double)tmpl->data[i].x * x_scale * COS(ang)) -
		         ((double)tmpl->data[i].y * y_scale * SIN(ang));
		y1 = y + ((double)tmpl->data[i].y * y_scale * COS(ang)) + 
		         ((double)tmpl->data[i].x * x_scale * SIN(ang));
		x2 = x + ((double)tmpl->data[i+1].x * x_scale * COS(ang)) -
		         ((double)tmpl->data[i+1].y * y_scale * SIN(ang));
		y2 = y + ((double)tmpl->data[i+1].y * y_scale * COS(ang)) + 
		         ((double)tmpl->data[i+1].x * x_scale * SIN(ang));

		drawLine(col,thick,x1,y1,x2,y2);
	}
}



///////////////////////////// LOW LEVEL DRAWING ///////////////////////////////

/*** Set the thickness of the graphics context for the given colour ***/
void setThickness(int col, double thick)
{
	thick *= avg_scaling;
	if (thick < 1) thick = 1;
	XSetLineAttributes(
		display,gc[col],(int)rint(thick),LineSolid,CapRound,JoinRound);
}




/*** Draw a line taking into acount the scaling factors. Can't check onscreen
     because start and end points may be offscreen by line crosses screen.
     Same applies to rectangles and polygons ***/
void drawLine(int col, double thick, double x1, double y1, double x2, double y2)
{
	if (refresh_cnt) return;

	x1 *= x_scaling;
	y1 *= y_scaling;
	x2 *= x_scaling;
	y2 *= y_scaling;
	if (col < 0 || col >= NUM_COLOURS) col = 0;

	setThickness(col,thick);
	XDrawLine(display,drw,gc[col],(int)x1,(int)y1,(int)x2,(int)y2);
}




/*** Draw or fill a circle. Check whether its onscreen to see if we'll bother
     to draw it. Saves a bit of X traffic not having to draw all the stars ***/
void drawOrFillCircle(
	int col, double thick, double diam, double x, double y, bool fill)
{
	double x_diam;
	double y_diam;
	double radius;
	double xp;
	double yp;

	if (refresh_cnt) return;

	if (col < 0 || col >= NUM_COLOURS) col = 0;

	x_diam = diam * x_scaling;
	y_diam = diam * y_scaling;

	// X seems not to draw arcs with a diam < 2
	if (x_diam < 2) x_diam = 2;
	if (y_diam < 2) y_diam = 2;

	radius = diam / 2;
	xp = (x - radius) * x_scaling;
	yp = (y - radius) * y_scaling;

	// Don't draw circle if its outside the window. Cuts down the X11
	// traffic a bit.
	if (xp >= -diam && xp <= (double)win_width && 
	    yp >= -diam && yp <= (double)win_height)
	{
		if (fill == FILL)
		{
			XFillArc(
				display,drw,gc[col],
				(int)xp,(int)yp,(int)x_diam,(int)y_diam,
				0,FULL_CIRCLE);
		}
		else
		{
			setThickness(col,thick);
			XDrawArc(
				display,drw,gc[col],
				(int)xp,(int)yp,(int)x_diam,(int)y_diam,
				0,FULL_CIRCLE);
		}
	}
}




/*** Fill a polygon. The scale argument is used for efficiency in
     cl_polygon::draw() and only makes a difference if window is resized ***/
void drawOrFillPolygon(
	int col, double thick, XPoint *points, int num_points, bool fill)
{
	int i;

	if (refresh_cnt) return;

	if (col < 0 || col >= NUM_COLOURS) col = 0;

	for(i=0;i < num_points;++i)
	{
		points[i].x = (int)((double)points[i].x * x_scaling);
		points[i].y = (int)((double)points[i].y * y_scaling);
	}
	if (fill == FILL)
	{
		XFillPolygon(
			display,drw,gc[col],
			points,num_points,Nonconvex,CoordModeOrigin);
	}
	else
	{
		setThickness(col,thick);
		XDrawLines(
			display,drw,gc[col],points,num_points,CoordModeOrigin);

		// Draw from last point back to start. Not done automatically
		// by X.
		XDrawLine(
			display,drw,gc[col],
			points[num_points-1].x,points[num_points-1].y,
			points[0].x,points[0].y);
	}
}




/*** Fill a rectangle - more efficient that filling a polygon ***/
void drawOrFillRectangle(
	int col,
	double thick, double x, double y, double w, double h, bool fill)
{
	if (refresh_cnt) return;

	x *= x_scaling;
	y *= y_scaling;
	w *= x_scaling;
	h *= y_scaling;

	if (w < 1) w = 1;
	if (h < 1) h = 1;

	if (col < 0 || col >= NUM_COLOURS) col = 0;

	if (fill == FILL)
		XFillRectangle(display,drw,gc[col],(int)x,(int)y,(int)w,(int)h);
	else
	{
		setThickness(col,thick);
		XDrawRectangle(display,drw,gc[col],(int)x,(int)y,(int)w,(int)h);
	}
}
