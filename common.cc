// Common functions.

#include "globals.h"

void resetMolehills();

/*** Stages are changed in mainloop() and cl_player::run() ***/
void setGameStage(en_game_stage stg)
{
	int o;
	int i;

	game_stage = stg;
	game_stage_cnt = 0;

	// Should be no background sounds after stage reset
	playBGSound(SND_SILENCE);

	switch(stg)
	{
	case GAME_STAGE_ATTRACT_PLAY:
		resetMolehills();
		setScore(0);
		text_digger->reset();
		text_copyright->reset();
		text_s_to_start->reset();
		deactivateAllObjects();
		level = random() % 20 + 1;
		initLevel();
		player->activate();
		break;

	case GAME_STAGE_ATTRACT_ENEMIES:
		// Resets freeze counts etc so enemies in this screen have
		// correct colour
		player->activate(); 

		for(i=0;i < NUM_ATTRACT_ENEMIES;++i)
			attract_enemy[i]->attractActivate();
		break;

	case GAME_STAGE_ATTRACT_KEYS:
		break;

	case GAME_STAGE_LEVEL_START:
		resetMolehills();
		deactivateAllObjects();
		initLevel();
		text_level_start->reset();
		break;

	case GAME_STAGE_READY:
		text_ready->reset();
		break;

	case GAME_STAGE_PLAY:
		player->activate();
		break;

	case GAME_STAGE_LEVEL_COMPLETE:
		// Bonus score if no lives lost during level
		if (lives == lives_at_level_start)
		{
			end_of_level_bonus = 200 + 100 * level;
			incScore(end_of_level_bonus);
		}
		else end_of_level_bonus = 0;
		sprintf(end_of_level_bonus_str,"BONUS: %04d\n",end_of_level_bonus);
		break;

	case GAME_STAGE_PLAYER_DIED:
		setLives(lives-1);
		if (!lives)
		{
			game_stage = GAME_STAGE_GAME_OVER;
			text_game_over->reset();	
			playFGSound(SND_GAME_OVER);
		}

		// Reset all objects except boulders and nuggets which stay
		// in STAGE_RUN unless boulder is being eaten
		FOR_ALL_OBJECTS(o)
		{
			switch(objects[o]->type)
			{
			case TYPE_NUGGET:
				break;

			case TYPE_BOULDER:
				if (objects[o]->stage == STAGE_BEING_EATEN)
					objects[o]->setStage(STAGE_INACTIVE);
				break;

			default:
				objects[o]->setStage(STAGE_INACTIVE);
			}
		}
		break;

	default:
		assert(0);
	}
}




/*** Pick new position , width and height ***/
void resetMolehills()
{
	for(int i=0;i < NUM_MOLEHILLS;++i) molehill[i].reset();
}




/*** Set up the level specific stuff ***/
void initLevel()
{
	level_cnt = 0;

	materialise_y_add = level < 10 ? 1 + (double)level / 10 : 2;
	spooky_create_mod = level < 10 ? 300 - level * 20 : 100;
	grubble_create_mod = level < 15 ? 250 - level * 10 : 100;
	first_spooky_cnt = level < 10 ? 40 - level * 2 : 20;
	eating_time = level < 10 ? 100 - level * 5 : 50;
	bonus_nugget_cnt = (level < 6 ? 1 : 2);

	nugget_cnt = 0;
	invisible_powerup_cnt = 0;
	superball_powerup_cnt = 0;
	turbo_enemy_powerup_cnt = 0;
	freeze_powerup_cnt = 0;

	// Once wurmals killed on a level they stay dead
	wurmals_killed = 0;

	lives_at_level_start = lives;

	player->resetTimers();
	setGroundColour();
	initTunnels();

	// Create static objects
	switch(level)
	{
	case 1:
	case 2:
		activateObjectsTotal(TYPE_BOULDER,2);
		break;

	case 3:
		activateObjectsTotal(TYPE_BOULDER,3);
		invisible_powerup_cnt = 1;
		turbo_enemy_powerup_cnt = 1;
		break;

	case 4:
		activateObjectsTotal(TYPE_BOULDER,3);
		invisible_powerup_cnt = 1;
		superball_powerup_cnt = 1;
		turbo_enemy_powerup_cnt = 1;
		break;

	case 5:
		activateObjectsTotal(TYPE_BOULDER,3);
		invisible_powerup_cnt = 1;
		superball_powerup_cnt = 1;
		turbo_enemy_powerup_cnt = 1;
		break;

	case 6:
		activateObjectsTotal(TYPE_BOULDER,4);
		invisible_powerup_cnt = 1;
		superball_powerup_cnt = 1;
		freeze_powerup_cnt = 1;
		turbo_enemy_powerup_cnt = 1;
		break;

	case 7:
		activateObjectsTotal(TYPE_BOULDER,4);
		invisible_powerup_cnt = 2;
		superball_powerup_cnt = 1;
		freeze_powerup_cnt = 1;
		turbo_enemy_powerup_cnt = 1;
		break;

	case 8:
		activateObjectsTotal(TYPE_BOULDER,5);
		invisible_powerup_cnt = 2;
		superball_powerup_cnt = 2;
		freeze_powerup_cnt = 1;
		turbo_enemy_powerup_cnt = 2;
		break;

	default:
		activateObjectsTotal(TYPE_BOULDER,5);
		invisible_powerup_cnt = 2;
		superball_powerup_cnt = 2;
		freeze_powerup_cnt = 2;
		turbo_enemy_powerup_cnt = 2;
	}

	activateObjectsTotal(TYPE_NUGGET,10+level*3);
	sprintf(level_text,"LEVEL %02d\n",level);
}




/*** Have a guess ***/
void deactivateAllObjects()
{
	int o;
	FOR_ALL_OBJECTS(o) objects[o]->setStage(STAGE_INACTIVE);
}




/*** Make sure the given number of objects are activated, if not then 
     activate them ***/
void activateObjectsTotal(en_type type, int num)
{
	int o;
	int cnt = 0;

	if (num < 1) return;

	FOR_ALL_OBJECTS(o)
	{
		cnt += (objects[o]->stage != STAGE_INACTIVE && 
		        objects[o]->type == type);
	}
	if (cnt < num) activateObjects(type,num - cnt);
}




/*** Activate the given number of objects ***/
void activateObjects(en_type type, int num)
{
	int o;
	int cnt = 0;
	
	if (num < 1) return;

	FOR_ALL_OBJECTS(o)
	{
		if (objects[o]->type == type && 
		    objects[o]->stage == STAGE_INACTIVE)
		{
			objects[o]->activate();
			if (++cnt == num) break;
		}
	}
}




/*** For when something has happened ***/
void activateBonusScore(cl_object *obj, int bonus)
{
	for(int i=0;i < NUM_BONUS_SCORES;++i)
	{
		if (!text_bonus_score[i]->running)
		{
			text_bonus_score[i]->reset(obj,bonus);
			break;
		}
	}
	incScore(bonus);
	playFGSound(SND_BONUS_SCORE);
}




/*** Set up the window scaling factors ***/
void setScaling()
{
	x_scaling = (double)win_width / SCR_SIZE;
	y_scaling = (double)win_height / SCR_SIZE;
	avg_scaling = (x_scaling + y_scaling) / 2;
}




/*** Set the score to the specific value ***/
void setScore(int val)
{
	score = val;
	if (score > high_score && !IN_ATTRACT_MODE()) 
	{
		high_score = score;
		if (!done_high_score)
		{
			text_new_high_score->reset();
			done_high_score = true;
			playFGSound(SND_HIGH_SCORE);
		}
	}
	if (score >= bonus_life_score && !IN_ATTRACT_MODE())
	{
		bonus_life_score += BONUS_LIFE_INC;
		text_bonus_life->reset();
		setLives(lives + 1);
		++lives_at_level_start;
		playFGSound(SND_BONUS_LIFE);
	}
	sprintf(score_text,"%06u",score);
	sprintf(high_score_text,"%06u",high_score);
}




void incScore(int val)
{
	setScore(score + val);
}




void setLives(int val)
{
	lives = val;
	sprintf(lives_text,"%u",lives);
}




/*** Set the ground colour based on the level ***/
void setGroundColour()
{
	switch(level % 10)
	{
	case 1:
	case 2:
		ground_colour = KHAKI;
		break;

	case 3:
	case 4:
		ground_colour = DARK_GREEN;
		break;

	case 5:
	case 6:
		ground_colour = STEEL_BLUE;
		break;

	case 7:
	case 8:
		ground_colour = DARK_MAUVE;
		break;

	case 9:
	case 0:
		ground_colour = DARK_RED;
	}
}




/*** 2D rotation about a point ***/
void rotate(double &x, double &y, double ang)
{
	double tmp_x = x;

	x = x * COS(ang) - y * SIN(ang);
	y = y * COS(ang) + tmp_x * SIN(ang);
}




/*** As above with shorts ***/
void rotate(short &x, short &y, double ang)
{
	double dx = (double)x;
	double dy = (double)y;
	double tmp_x = x;

	dx = dx * COS(ang) - dy * SIN(ang);
	dy = dy * COS(ang) + tmp_x * SIN(ang);
	x = (short)dx;
	y = (short)dy;
}




/*** Increment the angle in the direction of required angle by increment ***/
void attainAngle(double &ang, double req_ang, int inc)
{
	double diff = angleDiff(ang,req_ang);

	if (fabs(diff) <= inc)
		ang = req_ang;
	else
		incAngle(ang,diff < 0 ? -inc : inc);
}




/*** Increment and normalise ***/
void incAngle(double &ang, double inc)
{
	ang += inc;
	while(ang >= 360) ang -= 360;
	while(ang < 0) ang += 360;
}




/*** Get the difference between the 2 angles ***/
double angleDiff(double ang1, double ang2)
{
	ang2 -= ang1;
	if (ang2 >= 360) ang2 -= 360;
	else
	if (ang2 < 0) ang2 += 360;

	return ang2 <= 180 ? ang2 : -(360 - ang2);
}




/*** Return true if location is out of the game area ***/
bool offscreen(int x, int y)
{
	return (x < 0 || x >= SCR_SIZE || y < PLAY_AREA_TOP || y >= SCR_SIZE);
}
