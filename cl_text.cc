// Screen text object

#include "globals.h"

#define DIGGER_STOP 150

/*** Constructor ***/
cl_text::cl_text(en_type t)
{
	type = t;
	text = NULL;
	y = -1;
	reset(false);
}




/*** For flying texts that appear where powerup/bonus etc happened ***/
void cl_text::reset(cl_object *obj, int num)
{
	reset(true);

	start_x = x = obj->x;
	start_y = y = obj->y;
	dist = 0;
	angle2 = 0;
	y_add = 0;

	switch(type)
	{
	case TXT_GOT_SPIKY:
		text = "GOT SPIKY!";
		cnt = random() % 10; // Random starting delay
		break;
	case TXT_BONUS_SCORE:
		sprintf(mesg,"BONUS %03d!",num);
		text = mesg;
		cnt = random() % 10; 
		break;
	case TXT_INVISIBILITY_POWERUP:
		text = "INVISIBILITY!";
		cnt = 0;
		break;		
	case TXT_SUPERBALL_POWERUP:
		text = "SUPERBALL!";
		break;		
	case TXT_FREEZE_POWERUP:
		text = "FREEZE!";
		break;
	default:
		assert(0);
	}
}




/*** Set up all required parameters ***/
void cl_text::reset(bool run)
{
	// Zero everything first to be safe
	x_scale = 1;
	y_scale = 1;
	x_scale_add = 0;
	y_scale_add = 0;
	col = COL_WHITE;
	col_add = 0;
	thick = 2;
	thick_add = 0;
	angle = 0;
	ang_add = 0;
	gap = 0;
	cnt = 0;
	running = run;

	switch(type)
	{
	case TXT_PAUSED:
		text = "PAUSED";
		x = SCR_MID - 180,
		y = SCR_MID - 10;
		col = COL_GREEN;
		col_add = 0.1;
		x_scale = 5;
		y_scale = 3;
		y_scale_add = 0.1;
		thick_add = 0.5;
		break;
	case TXT_DIGGER:
		col = COL_GREEN;
		col_add = 3;
		text = "DIGGER";
		start_x = x = 100;
		start_y = y = -100;
		x_scale = 6;
		y_scale = 8;
		thick = 15;
		ang_add = 10;
		break;
	case TXT_COPYRIGHT:
		text = COPYRIGHT;
		x = 47;
		y = SCR_MID + 50;
		thick = 2;
		x_scale = 1;
		y_scale = 1.5;
		col = COL_TURQUOISE;
		break;
	case TXT_S_TO_START:
		text = "PRESS 'S' TO START";
		x = 195;
		y = SCR_MID + 100;
		y_scale = 3;
		thick = 4;
		break;
	case TXT_LEVEL_START:
		thick = 10;
		x = SCR_MID - 210;
		y = SCR_MID;
		x_scale = 4;
		y_scale = 6;
		ang_add = 10;
		break;
	case TXT_READY:
		text = "READY!";
		col = COL_GREEN;
		x_scale = 6;
		y_scale = 6;
		x = SCR_MID - 210;
		y = SCR_MID;
		break;
	case TXT_GAME_OVER:
		text = "GAME OVER";
		x = SCR_SIZE;
		y = SCR_MID;
		col = COL_GREEN;
		col_add = 2;
		x_scale = 4;
		y_scale = 10;
		thick = 15;
		gap = 120;
		ang_add = -6;
		break;
	case TXT_GOT_SPIKY:
		y_scale = 3;
		thick = 3;
		break;
	case TXT_BONUS_SCORE:
		col = COL_RED;
		col_add = 1;
		y_scale = 2;
		thick = 3;
		break;
	case TXT_BONUS_LIFE:
		text = "BONUS LIFE!";
		x = SCR_SIZE;
		y = SCR_MID;	
		col = COL_GREEN;
		cnt = 0;
		break;
	case TXT_INVISIBILITY_POWERUP:
		col = COL_PURPLE;
		thick = 5;
		break;
	case TXT_SUPERBALL_POWERUP:
		col = COL_WHITE;
		thick = 5;
		break;
	case TXT_FREEZE_POWERUP:
		col = COL_BLUE;
		break;
	case TXT_NEW_HIGH_SCORE:
		text = "NEW HIGH SCORE!";
		x = SCR_SIZE;
		x_scale = 3;
		y_scale = 3;
		thick = 7;
		break;
	default:
		assert(0);
	}
}




/*** Do the drawing and variable updates ***/
void cl_text::draw()
{
	char txt[100];
	int x2;
	int i;

	if (!running) return;

	col += col_add;
	thick += thick_add;
	x_scale += x_scale_add;
	y_scale += y_scale_add;
	incAngle(angle,ang_add);

	switch(type)
	{
	case TXT_PAUSED:
		if (col >= COL_GREEN2 || col <= COL_GREEN)
			col_add = -col_add;

		if (y_scale >= 8 || y_scale <= 3)
		{
			y_scale_add = -y_scale_add;
			thick_add = -thick_add;
		}
		break;

	case TXT_DIGGER:
		if (col >= COL_GREEN2) col = COL_GREEN;

		// Slow down rotation
		if (ang_add > 5) ang_add -= 0.03;
		else if (angle > 355 || angle < 5) 
		{
			angle = 0;
			ang_add = 0;
		}

		if (game_stage_cnt >= DIGGER_STOP)
		{
			x = start_x;
			y = SCR_MID - 40;
			break;
		}
		++y_add;
		start_y += y_add;
		if (start_y >= SCR_MID - 40)
		{
			start_y -= y_add;
			y_add = (-y_add * 0.7);
			cnt = 20;
		}
		x = start_x;
		y = start_y;

		// Vibrate after bouncing
		if (cnt)
		{
			x += ((random() % 21) - 10);
			y += ((random() % 21) - 10);
			--cnt;
		}
		break;

	case TXT_COPYRIGHT:
		if (game_stage_cnt < DIGGER_STOP) return;
		break;

	case TXT_S_TO_START:
		if (game_stage_cnt < DIGGER_STOP || game_stage_cnt % 40 > 20)
			return;
		break;

	case TXT_LEVEL_START:
		sprintf(txt,"LEVEL %02d",level);
		text = txt;
		if (angle >= 350)
		{
			angle = 0;
			ang_add = 0;
		}
		break;		

	case TXT_READY:
		incAngle(angle,10 - abs(20 - (game_stage_cnt % 40)));
		thick = 15 - abs(10 - (game_stage_cnt % 20));
		col = ((int)col + 5) % COL_GREEN2;
		break;

	case TXT_GAME_OVER:
		if (gap > 0) 
		{
			gap -= 2;
			if (gap < 0) gap = 0;
		}
		if (x > 85)
		{
			x -= 10;
			if (x <= 85)
			{
				x = 85;
				ang_add = 0;
				angle = 0;
			}
		}
		if (col >= COL_GREEN2) col = COL_GREEN;
		break;

	case TXT_GOT_SPIKY:
	case TXT_BONUS_SCORE:
		if (cnt)
		{
			--cnt;
			return;
		}
		if (y < 0)
		{
			running = false;
			return;
		}
		if (angle2 < 810)
		{
			x = start_x + SIN(angle2) * dist;
			y = start_y + COS(angle2) * dist;
			angle2 += 10;
			dist += 2;
		}
		else y -= 20;
		if (type == TXT_GOT_SPIKY) col = random() % NUM_FULL_COLOURS;
		else
		{
			if (col == COL_YELLOW || col == COL_BLUE) col_add = -1;
			else
			if (col == COL_RED || col == COL_GREEN) col_add = 1;
		}
		break;

	case TXT_BONUS_LIFE:
		// This draws individual characters 
		if (x < -400)
		{
			running = false;
			return;
		}
		x2 = (int)x;
		for(i=0;i < 11;++i)
		{
			y = SCR_MID + 200 * SIN(cnt - i * 10);
			drawChar(text[i],(int)col + 2,6,0,3,3,x2,y);
			x2 += 40;
		}
		x -= 8;
		cnt = (cnt + 10) % 360;
		if (++col > COL_YELLOW) col = COL_GREEN;
		return;

	case TXT_INVISIBILITY_POWERUP:
		if (y < 0)
		{
			running = false;
			return;
		}
		x_scale += 0.1;
		y_scale += 0.1;
		x -= 10;
		y -= y_add;
		y_add += 0.3;

		if ((game_stage_cnt % 4) < 2) return;
		break;

	case TXT_SUPERBALL_POWERUP:
		if (y < 0)
		{
			running = false;
			return;
		}
		x_scale += 0.1;
		y_scale += 0.1;
		x -= 8;
		y -= y_add;
		y_add += 0.3;

		col = random() % NUM_FULL_COLOURS;
		break;

	case TXT_FREEZE_POWERUP:
		if (y < 0)
		{
			running = false;
			return;
		}
		x_scale += 0.1;
		y_scale += 0.1;
		y -= y_add;
		y_add += 0.3;

		x -= 5;
		thick = 5 + random() % 30;
		if (--col == COL_TURQUOISE) col = COL_BLUE;
		break;

	case TXT_NEW_HIGH_SCORE:
		if (x < -SCR_SIZE)
		{
			running = false;
			return;
		}
		col = random() % NUM_FULL_COLOURS;
		x -= 10;
		y = SCR_MID + SIN((game_stage_cnt * 4) % 360) * SCR_MID * 0.8;
		break;

	default:
		assert(0);
	}

	drawText(text,(int)col,(int)thick,angle,gap,x_scale,y_scale,x,y);
}
