// All the macros, globals and class defs

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/Xdbe.h>

#include <vector>
#include <algorithm>

#include "build_date.h"

#if defined(ALSA) && !defined(SOUND)
#error "The ALSA build option can only be used with SOUND"
#endif

using namespace std;

#ifdef MAINFILE
#define EXTERN 
#else
#define EXTERN extern
#endif

#define VERSION   "1.1.4"
#define COPYRIGHT "Copyright (C) Neil Robertson 2011-2023"

#define SCR_SIZE         650
#define SCR_MID          (SCR_SIZE / 2)
#define PLAY_AREA_TOP    50
#define PLAY_AREA_HEIGHT (SCR_SIZE - PLAY_AREA_TOP)

#define ALSA_DEVICE  "sysdefault"

#define TUNNEL_WIDTH 50
#define TUNNEL_HALF (TUNNEL_WIDTH / 2)

#define MAX_NUGGETS   40
#define MAX_BOULDERS  4
#define MAX_SPOOKYS   10
#define MAX_GRUBBLES  6
#define MAX_SPIKYS    2
#define MAX_WURMALS   2

// +2 for player and ball
#define MAX_OBJECTS (\
	MAX_NUGGETS + \
	MAX_BOULDERS + \
	MAX_SPOOKYS + \
	MAX_GRUBBLES + \
	MAX_SPIKYS + \
	MAX_WURMALS) + 2

#define MAX_STONES          50
#define MAX_TMP_POINTS      100
#define NUM_ATTRACT_ENEMIES 4
#define NUM_BONUS_SCORES    5
#define NUM_MOLEHILLS       15
#define FALL_SPEED          4
#define FAR_FAR_AWAY        9999
#define BONUS_LIFE_INC      8000

#define START_X SCR_MID
#define START_Y (PLAY_AREA_TOP + TUNNEL_HALF)

#define FULL_CIRCLE     23040
#define DEGS_PER_RADIAN 57.29578
#define SIN(A)          sin((A) / DEGS_PER_RADIAN)
#define COS(A)          cos((A) / DEGS_PER_RADIAN)

#define LOW_VOLUME   8000
#define MED_VOLUME  (LOW_VOLUME * 2)
#define HIGH_VOLUME (LOW_VOLUME * 3)

#define FILL true
#define DRAW false

enum en_colours
{
	// Colour mixes
	COL_GREEN       = 0,
	COL_TURQUOISE   = 15,
	COL_LIGHT_BLUE  = 20,
	COL_BLUE        = 30,
	COL_PURPLE      = 40,
	COL_MAUVE       = 45,
	COL_RED         = 60,
	COL_ORANGE      = 68,
	COL_YELLOW      = 75,
	COL_GREEN2      = 89,
	COL_BLACK       = 90,
	COL_DARK_GREY   = 92,
	COL_GREY        = 97,
	COL_GREYISH     = 100,
	COL_WHITE       = 105,

	// RGB brightness
	COL_BLACK2       = 106,
	COL_DARK_RED     = 111,
	COL_RED2         = 121,
	COL_BLACK3       = 122,
	COL_DARK_GREEN   = 126,
	COL_MEDIUM_GREEN = 130,
	COL_GREEN3       = 137,
	COL_BLACK4       = 138,
	COL_DARK_BLUE    = 142,
	COL_MEDIUM_BLUE  = 147,
	COL_BLUE2        = 153,

	// Mixed brightness
	COL_BLACK5      = 154,
	COL_DARK_MAUVE  = 158,
	COL_MAUVE2      = 169,
	COL_BLACK6      = 170,
	COL_STEEL_BLUE  = 174,
	COL_TURQUOISE2  = 185,
	COL_BLACK7      = 186,
	COL_KHAKI       = 190,
	COL_YELLOW2     = 201
};

#define NUM_FULL_COLOURS 106
#define NUM_COLOURS      202

enum en_game_stage
{
	GAME_STAGE_ATTRACT_PLAY,
	GAME_STAGE_ATTRACT_ENEMIES,
	GAME_STAGE_ATTRACT_KEYS,
	GAME_STAGE_LEVEL_START,
	GAME_STAGE_READY,
	GAME_STAGE_PLAY,
	GAME_STAGE_LEVEL_COMPLETE,
	GAME_STAGE_PLAYER_DIED,
	GAME_STAGE_GAME_OVER
};

#define IN_ATTRACT_MODE() (game_stage <= GAME_STAGE_ATTRACT_KEYS)

// Not all stages used by all objects
enum en_object_stage
{
	STAGE_INACTIVE,
	STAGE_MATERIALISE,
	STAGE_RUN,
	STAGE_DEMATERIALISE,
	STAGE_WOBBLE,
	STAGE_FALL,
	STAGE_BEING_EATEN,
	STAGE_HIT,
	STAGE_EXPLODE
};

enum en_dir
{
	DIR_STOP,
	DIR_LEFT,
	DIR_RIGHT,
	DIR_UP,
	DIR_DOWN
};

enum en_type
{
	TYPE_PLAYER,
	TYPE_BALL,
	TYPE_STONE,
	TYPE_NUGGET,
	TYPE_BOULDER,
	TYPE_SMALL_BOULDER, // only used internally by boulder
	TYPE_SPOOKY,
	TYPE_SPIKY,
	TYPE_GRUBBLE,
	TYPE_WURMAL
};

// Sounds in order of priority. Lowest -> highest.
enum en_sound
{
	SND_SILENCE,

	// Foreground
	SND_EAT_NUGGET,
	SND_BOULDER_WOBBLE,
	SND_BOULDER_LAND,
	SND_GRUBBLE_EAT,
	SND_BALL_BOUNCE,
	SND_BALL_THROW,
	SND_BALL_RETURN,
	SND_BOULDER_EXPLODE,
	SND_SPIKY_DEMATERIALISE,
	SND_ENEMY_MATERIALISE,
	SND_SPIKY_MATERIALISE,
	SND_FALL,
	SND_SPOOKY_HIT,
	SND_GRUBBLE_HIT,
	SND_WURMAL_HIT,
	SND_BONUS_SCORE,
	SND_ENEMY_EXPLODE,
	SND_FREEZE_POWERUP,
	SND_HIGH_SCORE,
	SND_BONUS_LIFE,
	SND_PLAYER_HIT,
	SND_PLAYER_EXPLODE,
	SND_LEVEL_COMPLETE,
	SND_GAME_OVER,
	SND_START,

	// Background
	SND_INVISIBILITY_POWERUP,
	SND_SUPERBALL_POWERUP,
	SND_TURBO_ENEMY,

	NUM_SOUNDS
};



/////////////////////////////// MISC CLASSES //////////////////////////////////

/*** Tunnel class ***/
class cl_tunnel
{
public:
	int x1;
	int y1;
	int x2;
	int y2;
	int max_x;
	int min_x;
	int max_y;
	int min_y;

	bool vert;
	bool recursed;
	vector<cl_tunnel *> links;
	cl_tunnel *alt;

	cl_tunnel(int nx1, int ny1);

	void update(int nx2, int ny2);
	cl_tunnel *complete();
	cl_tunnel *checkForSimilar();
	int overlapLen(int v1, int v2, int v3, int v4);
	void setMaxMin();
	void setLinks();
	void linkTunnel(cl_tunnel *tun);
	void draw();
	void draw2();
};


class cl_object;

/*** Used in explosions ***/
class cl_explosion
{
public:
	cl_object *owner;
	double *x;
	double *y;
	double *x_add;
	double *y_add;
	double start_x;
	double start_y;
	double col;
	double col_add;
	double size;
	double size_sub;
	int cnt;
	int speed;
	bool rev;

	cl_explosion(cl_object *own);
	void activate();
	void reverse();
	void runAndDraw();
};


/*** Animated text class ***/
class cl_text
{
public:
	enum en_type
	{
		TXT_PAUSED,
		TXT_DIGGER,
		TXT_S_TO_START,
		TXT_COPYRIGHT,
		TXT_LEVEL_START,
		TXT_READY,
		TXT_GAME_OVER,
		TXT_GOT_SPIKY,
		TXT_BONUS_SCORE,
		TXT_BONUS_LIFE,
		TXT_INVISIBILITY_POWERUP,
		TXT_FREEZE_POWERUP,
		TXT_SUPERBALL_POWERUP,
		TXT_NEW_HIGH_SCORE
	} type;
	char mesg[100];
	const char *text;
	bool running;
	double x;
	double y;
	double start_x;
	double start_y;
	double y_add;
	double thick;
	double thick_add;
	double x_scale;
	double y_scale;
	double x_scale_add;
	double y_scale_add;
	double col;
	double col_add;
	double angle;
	double angle2;
	double ang_add;
	double gap;
	double dist;
	int cnt;

	cl_text(en_type t);
	void reset(cl_object *obj, int num);
	void reset(bool run = true);
	void draw();
};


/////////////////////////////// OBJECT CLASSES ////////////////////////////////

/*** Objects base class ***/
class cl_object
{
public:
	en_object_stage stage;
	cl_tunnel *curr_tunnel;
	en_type type;
	en_dir dir;
	en_dir facing_dir;
	double x;
	double y;
	double prev_x;
	double prev_y;
	double speed;
	double angle;
	double xsize;
	double ysize;
	int diam;
	int radius;
	int stage_cnt;

	cl_object(en_type t);

	virtual void activate() = 0;
	virtual void run() = 0;
	virtual void draw() = 0;
	virtual void haveCollided(cl_object *obj, double dist) { }

	void objDrawOrFillPolygon(
		int col,
		double thick, XPoint *points, int num_points, bool fill);
	void objDrawOrFillRectangle(
		int col,
		double thick, int xp, int yp, int w, int h, bool fill);
	void objDrawOrFillCircle(
		int col,
		double thick, double diam, double xp, double yp, bool fill);
	void objDrawLine(
		int col,
		double thick, double x1, double y1, double x2, double y2);
	void incAngle(double inc);
	void createExplodeBits(int cnt);
	void setStage(en_object_stage stg);
	double distToObject(cl_object *obj);
	virtual double overlapDist(cl_object *obj);
};



/*** Player class ***/
class cl_player: public cl_object
{
public:
	static const int NUM_POINTS = 4;
	static XPoint square1[NUM_POINTS];
	static XPoint square2[NUM_POINTS];
	cl_explosion *explode;

	double start_x;
	double start_y;
	double ball_x;
	double ball_y;
	double ball_ang;
	double req_ball_ang;
	double col;
	double xsize_inc;
	double ysize_inc;
	en_dir prev_dir;
	cl_tunnel *prev_tunnel;
	cl_object *boulder;
	int ball_ang_inc;
	int invisible_timer;
	int freeze_timer;
	int turbo_enemy_timer;
	bool superball;
	bool fill;
	bool hit_object;
	bool hit_edge;

	cl_player();

	void activate();
	void resetTimers();
	void run();
	void autoplay();
	void autoplayRandomMove();
	void stageRun();
	void stageFall();
	void setBallPos();
	void tunnelComplete();

	void move(KeySym key);
	void stop(KeySym key);
	void throwBall();

	void haveCollided(cl_object *obj, double dist);
	void draw();
};



/////////////////////////////////// ENEMIES 

class cl_boulder;

/*** Enemy base class ***/
class cl_enemy: public cl_object
{
public:
	cl_tunnel *prev_tunnel;
	cl_tunnel *next_tunnel;
	cl_explosion *explode;
	cl_boulder *boulder;
	en_dir prev_dir;
	bool fill;
	bool hit_player;
	double start_speed;
	double dist_to_player;
	
	cl_enemy(en_type t);

	void activate();
	virtual void attractActivate();
	virtual void attractRun();
	void stageMaterialise();
	void stageFall();
	void setStageExplode();
	void setDirectionToObject(cl_object *obj);
	void reverseDirection();
	void pickRandomTunnel();
	void pickPlayerDirTunnel();
	void setDirection();
	en_dir dirToTunnel(cl_tunnel *from, cl_tunnel *to);
	void updateTunnelPtrs(cl_tunnel *oldtun, cl_tunnel *newtun);
	void hitPlayerMove();

	virtual void move() { }
};


class cl_spooky: public cl_enemy
{
public:
	static const int BODY_POINTS = 10;
	static const int TEETH_POINTS = 5;
	static XPoint body[2][BODY_POINTS];
	static XPoint teeth[TEETH_POINTS];

	int bodynum;
	int pup_x_add;
	int pup_y_add;
	int body_col;
	int eye_col;
	int pup_col;
	int max_depth;
	double ang_inc;

	cl_spooky();
	void activate();
	void attractActivate();
	void run();
	void move();
	void stageRun();
	void haveCollided(cl_object *obj, double dist);
	void draw();
};


class cl_grubble: public cl_enemy
{
public:
	static const int BODY_POINTS = 9;
	static const int TEETH_POINTS = 6;
	static XPoint body[2][BODY_POINTS];
	static XPoint top_teeth[2][TEETH_POINTS];
	static XPoint bot_teeth[2][TEETH_POINTS];
	cl_boulder *dinner;
	double req_angle;
	double dist_to_player;
	double dist_to_food;
	bool eating;

	int bodynum;
	int body_col;
	int teeth_col;
	int eye_col;
	int pup_col;
	int max_depth;

	cl_grubble();
	void activate();
	void attractActivate();
	void run();
	void move();
	void stageRun();
	bool findDinner();
	void draw();
	void haveCollided(cl_object *obj, double dist);
};


#define SPIKY_ARMS 20

class cl_spiky: public cl_enemy
{
public:
	struct st_arm
	{
		double len;
		double len_add;
		double angle;
		double ang_add;
		double col;
		double col_add;
	} arm[SPIKY_ARMS];
	int x_mult;
	int y_mult;
	int lifespan;
	int change_dir_cnt;

	cl_spiky();
	void activate();
	void attractActivate();
	void setChangeDirCnt();
	void setArmsForMaterialise();
	void setArmsForRun();
	void setMults();
	void run();
	void stageMaterialise();
	void stageRun();
	void draw();
	void haveCollided(cl_object *obj, double dist);
};


#define WURMAL_SEGMENTS 10

class cl_nugget;

class cl_wurmal: public cl_enemy
{
public:
	struct 
	{
		double x;
		double y;
	} segment[WURMAL_SEGMENTS];	
	cl_object *nugget;
	double start_y;
	double x_add;
	double y_add;
	double x_edge;
	double y_edge;
	double eye_x[2];
	double eye_y[2];
	bool find_random_nugget;
	bool eating;
	bool hit_boulder;
	int head_diam;
	int head_radius;
	int eye_col;
	int random_move_cnt;
	int eating_time;

	cl_wurmal();

	void activate();
	void attractActivate();
	void initSegments();
	void run();
	void attractRun();
	void stageRun();
	void findNugget();
	void pickRandomMove();
	void updateEyeAngle();
	void updateEdges();
	void updateSegments();
	bool outsideGround(double x, double y);
	void haveCollided(cl_object *obj, double dist);
	double overlapDist(cl_object *obj);
	void draw();
};


///////////////////////////// PLAYER BALL

class cl_ball: public cl_object
{
public:
	cl_explosion *explode;
	int x_mult;
	int y_mult;
	int explode_time;
	int superball_cnt;

	cl_ball();

	void activate();
	void run();
	void haveCollided(cl_object *obj, double dist);
	void draw();
};



///////////////////////////// ROCK CLASSES

class cl_rock: public cl_object
{
public:
	XPoint *points;
	int num_points;
	double col;
	double ang_inc;
	bool fill;

	cl_rock(en_type t);

	void activate();
	void run() { }
	void draw();
	void haveCollided(cl_object *obj, double dist) { }
};



class cl_stone: public cl_rock
{
public:
	// Put the code here, not worth seperate module
	cl_stone(): cl_rock(TYPE_STONE)
	{
		cl_rock::activate();
		x = random() % SCR_SIZE;
		y = PLAY_AREA_TOP + random() % PLAY_AREA_HEIGHT;	
		col = random() % NUM_FULL_COLOURS;
	}
};



class cl_nugget: public cl_rock
{
public:
	enum en_nugtype
	{
		WURMALLED,
		NORMAL,
		INVISIBILITY,
		SUPERBALL,
		FREEZE,
		BONUS,
		TURBO_ENEMY
	} nugtype;
	bool give_bonus;
	double start_col;
	int bonus_time;

	cl_nugget();
	void activate();
	void setBonusTime();
	void run();
	void resetBonusColSize();
	void haveCollided(cl_object *obj, double dist);
	void draw();
};



class cl_small_boulder: public cl_rock
{
public:
	cl_boulder *owner;
	double x_add;
	double y_add;

	cl_small_boulder(cl_boulder *own);
	void activate();
	void run();
	void reset();
};


#define NUM_SMALL_BOULDERS 5

class cl_boulder: public cl_rock
{
public:
	cl_small_boulder *small_boulder[NUM_SMALL_BOULDERS];
	en_dir cant_push_dir;
	bool on_boulder;
	double fall_start_y;
	int list_pos;
	int break_height;
	int wobble_cnt;
	int cant_push_cnt;
	int fall_y;
	int fall_check_add;

	cl_boulder();

	void activate();
	void resetFallCheck();
	void run();
	void setCurrTunnel();
	double push(en_dir push_dir);
	void setBeingEaten();
	void activateSmallBoulders();
	void runSmallBoulders();
	void haveCollided(cl_object *obj, double dist);
	void draw();
	void drawSmallBoulders();
};


///////////////////////////// MOLEHILLS

class cl_molehill
{
public:
	XPoint vertex[3];

	cl_molehill() { }
	void reset();
	void draw();
};

	
//////////////////////////// CHARACTER DEFINITIONS //////////////////////////

/*** All characters are based on a 11x11 pixel size matrix. Taken from
     Peniten-6 except for new percent character ***/
#define CHAR_SIZE 10  // Should be 11 , my mistake. Too late to fix now.
#define CHAR_HALF (CHAR_SIZE / 2)
#define CHAR_GAP 5

struct st_char_template
{
	int cnt;
	XPoint data[1];
};

#ifdef MAINFILE
struct st_space { int cnt; } char_space = { 0 };

struct st_a { int cnt; XPoint data[6]; } char_A = 
{
	6, 
	{{ 0,10 }, { 5,0 },
	{ 5,0 }, { 10,10 },
	{ 2,6 }, { 8,6 }}
};

struct st_b { int cnt; XPoint data[20]; } char_B = 
{
	20,
	{{ 0,0 }, { 8,0 },
	{ 8,0 }, { 10,1 },
	{ 10,1 }, { 10,4 },
	{ 10,4 }, { 5,5 },
	{ 5,5 }, { 10,6 },
	{ 10,6 }, { 10,9 },
	{ 10,9 }, { 8,10 },
	{ 8,10 }, { 0,10 },
	{ 0,10 }, { 0,0 },
	{ 0,5 }, { 5,5 }}
};

struct st_c { int cnt; XPoint data[14]; } char_C = 
{
	14,
	{{10,1 } , { 9,0 },
	{ 9,0 } , { 2,0 },
	{ 2,0 }, { 0,2 },
	{ 0,2 }, { 0,8 },
	{ 0,8 }, { 2,10 },
	{ 2,10 }, { 9,10 },
	{ 9,10 }, { 10,9 }}
};

struct st_d { int cnt; XPoint data[12]; } char_D = 
{
	12,
	{{ 0,0 }, { 7,0 },
	{ 7,0 }, { 10,2 },
	{ 10,2 }, { 10,8 },
	{ 10,8 }, { 7,10 },
	{ 7,10 }, { 0,10 },
	{ 0,10 }, { 0,0 }}
};

struct st_e { int cnt; XPoint data[8]; } char_E = 
{
	8,
	{{ 0,0 }, { 10,0 },
	{ 0,0 } , { 0,10 },
	{ 0,5 }, { 7,5 },
	{ 0,10 }, { 10,10 }}
};

struct st_f { int cnt; XPoint data[6]; } char_F =
{
	6,
	{{ 0,0 } , { 10,0 },
	{ 0,0 }, { 0,10 },
	{ 0,5 }, { 7,5 }}
};

struct st_g { int cnt; XPoint data[18]; } char_G =
{ 
	18,
	{{ 10,2 } , { 7,0 },
	{ 7,0 }, { 2,0 },
	{ 2,0 }, { 0,2 },
	{ 0,2 }, { 0,8 },
	{ 0,8 }, { 2,10 },
	{ 2,10 }, { 7,10 },
	{ 7,10 }, { 10,7 },
	{ 10,7 }, { 10,5 },
	{ 10,5 }, { 5,5 }}
};

struct st_h { int cnt; XPoint data[6]; } char_H =
{
	6,
	{{ 0,0 }, { 0,10 },
	{ 0,5 }, { 10,5 },
	{ 10,0 }, { 10,10 }}
};

struct st_i { int cnt; XPoint data[6]; } char_I =
{
	6,
	{{ 0,0 }, { 10,0 },
	{ 5,0 }, { 5,10 },
	{ 0,10 }, { 10,10 }}
};

struct st_j { int cnt; XPoint data[12]; } char_J =
{
	12, 
	{{ 0,0 }, { 10,0 },
	{ 7,0 }, { 7,7 },
	{ 7,7 }, { 5,10 },
	{ 5,10 }, { 2,10 },
	{ 2,10 }, { 1,8 },
	{ 1,8 }, { 1,6 }}
};

struct st_k { int cnt; XPoint data[6]; } char_K =
{
	6,
	{{ 0,0 }, { 0,10 },
	{ 0,7 }, { 10,0 },
	{ 3,5 }, { 10,10 }}
};

struct st_l { int cnt; XPoint data[4]; } char_L =
{
	4,
	{{ 0,0 }, { 0,10 },
	{ 0,10 }, { 10,10 }}
};

struct st_m { int cnt; XPoint data[8]; } char_M =
{
	8,
	{{ 0,0 } , { 0,10 },
	{ 0,0 }, { 5,5 },
	{ 5,5 }, { 10,0 },
	{ 10,0 }, { 10,10 }}
};

struct st_n { int cnt; XPoint data[6]; } char_N =
{
	6,
	{{ 0,0 }, { 0,10 },
	{ 0,0 }, { 10,10 },
	{ 10,10 }, { 10,0 }}
};

struct st_o { int cnt; XPoint data[16]; } char_O =
{
	16,
	{{ 3,0 }, { 7,0 },
	{ 7,0 }, { 10,3 },
	{ 10,3 }, { 10,7 },
	{ 10,7 }, { 7,10 },
	{ 7,10 }, { 3,10 },
	{ 3,10 }, { 0,7 },
	{ 0,7 }, { 0,3 },
	{ 0,3 }, { 3,0 }}
};

struct st_p { int cnt; XPoint data[12]; } char_P =
{
	12,
	{{ 0,0 }, { 7,0 },
	{ 7,0 }, { 10,2 },
	{ 10,2 }, { 10,4 },
	{ 10,4 }, { 7,6 },
	{ 7,6 }, { 0,6 },
	{ 0,0 }, { 0,10 }}
};

struct st_q { int cnt; XPoint data[18]; } char_Q =
{
	18,
	{{ 3,0 }, { 7,0 },
	{ 7,0 }, { 10,3 },
	{ 10,3 }, { 10,7 },
	{ 10,7 }, { 7,10 },
	{ 7,10 }, { 3,10 },
	{ 3,10 }, { 0,7 },
	{ 0,7 }, { 0,3 },
	{ 0,3 }, { 3,0 },
	{ 0,10 }, { 4,6 }}
};

struct st_r { int cnt; XPoint data[14]; } char_R =
{
	14,
	{{ 0,0 }, { 7,0 },
	{ 7,0 }, { 10,2 },
	{ 10,2 }, { 10,4 },
	{ 10,4 }, { 7,6 },
	{ 7,6 }, { 0,6 }, 
	{ 0,0 }, { 0,10 },
	{ 3,6 }, { 9,10 }}
};

struct st_s { int cnt; XPoint data[22]; } char_S =
{
	22,
	{{ 10,1 }, { 9,0 },
	{ 9,0 }, { 1,0 },
	{ 1,0 }, { 0,1 },
	{ 0,1 }, { 0,4 }, 
	{ 0,4 }, { 1,5 },
	{ 1,5 }, { 9,5 },
	{ 9,5 }, { 10,6 },
	{ 10,6 }, { 10,9 },
	{ 10,9 }, { 9,10 }, 
	{ 9,10 }, { 1,10 },
	{ 1,10 }, { 0,9 }}
};

struct st_t { int cnt; XPoint data[4]; } char_T =
{
	4,
	{{ 0,0 }, { 10,0 },
	{ 5,0 }, { 5,10 }}
};

struct st_u { int cnt; XPoint data[10]; } char_U =
{
	10,
	{{ 0,0 } , { 0,8 },
	{ 0,8 }, { 2,10 },
	{ 2,10 }, { 8,10 },
	{ 8,10 }, { 10,8 },
	{ 10,8 }, { 10,0 }}
};

struct st_v { int cnt; XPoint data[4]; } char_V =
{
	4,
	{{ 0,0 }, { 5,10 },
	{ 5,10 }, { 10,0 }}
};

struct st_w { int cnt; XPoint data[8]; } char_W =
{
	8,
	{{ 0,0 }, { 2,10 },
	{ 2,10 }, { 5,4 },
	{ 5,4 }, { 8,10 },
	{ 8,10 }, { 10,0 }}
};

struct st_x { int cnt; XPoint data[4]; } char_X =
{
	4,
	{{ 0,0 }, { 10,10 },
	{ 10,0 }, { 0,10 }}
};

struct st_y { int cnt; XPoint data[4]; } char_Y =
{
	4,
	{{ 0,0 }, { 6,4 },
	{ 10,0 }, { 2,10 }}
};

struct st_z { int cnt; XPoint data[6]; } char_Z =
{
	6,
	{{ 0,0 }, { 10,0 },
	{ 10,0 }, { 0,10 },
	{ 0,10 }, { 10,10 }}
};

struct st_1 { int cnt; XPoint data[6]; } char_1 =
{
	6,
	{{ 2,3 }, { 5,0 },
	{ 5,0 }, { 5,10 },
	{ 0,10 }, { 10,10 }}
};

struct st_2 { int cnt; XPoint data[12]; } char_2 =
{
	12,
	{{ 0,2 }, { 2,0 },
	{ 2,0 }, { 8,0 },
	{ 8,0 }, { 10,2 },
	{ 10,2 }, { 10,4 },
	{ 10,4 }, { 0,10 },
	{ 0,10 }, { 10,10 }}
};

struct st_3 { int cnt; XPoint data[16]; } char_3 =
{
	16,
	{{ 0,1 }, { 1,0 },
	{ 1,0 }, { 9,0 },
	{ 9,0 }, { 10,1 },
	{ 10,1 }, { 10,9 },
	{ 10,9 }, { 9,10 },
	{ 9,10 }, { 1,10 },
	{ 1,10 }, { 0,9 },
	{ 3,5 }, { 10,5 }}
};

struct st_4 { int cnt; XPoint data[6]; } char_4 =
{
	6,
	{{ 10,5 }, { 0,5 },
	{ 0,5 }, { 5,0 },
	{ 5,0 }, { 5,10 }}
};

struct st_5 { int cnt; XPoint data[14]; } char_5 =
{
	14,
	{{ 10,0 }, { 0,0 },
	{ 0,0 }, { 0,5 },
	{ 0,5 }, { 9,5 },
	{ 9,5 }, { 10,6 },
	{ 10,6 }, { 10,9 },
	{ 10,9 }, { 9,10 },
	{ 9,10 }, { 0,10 }}
};

struct st_6 { int cnt; XPoint data[18]; } char_6 =
{
	18,
	{{ 10,0 }, { 1,0 },
	{ 1,0 }, { 0,1 },
	{ 0,1 }, { 0,9 },
	{ 0,9 }, { 1,10 },
	{ 1,10 }, { 9,10 },
	{ 9,10 }, { 10,9 },
	{ 10,9 }, { 10,6 },
	{ 10,6 }, { 9,5 },
	{ 9,5 }, { 0,5 }}
};	

struct st_7 { int cnt; XPoint data[4]; } char_7 =
{ 
	4,
	{{ 0,0 }, { 10,0 },
	{ 10,0 }, { 2,10 }}
};

struct st_8 { int cnt; XPoint data[30]; } char_8 =
{
	30,
	{{ 1,0 }, { 9,0 },
	{ 9,0 }, { 10,1 },
	{ 10,1 }, { 10,4 },
	{ 10,4 }, { 9,5 },
	{ 9,5 }, { 10,6 },
	{ 10,6 }, { 10,9 },
	{ 10,9 }, { 9,10 },
	{ 9,10 }, { 1,10 },
	{ 1,10 }, { 0,9 },
	{ 0,9 }, { 0,6 },
	{ 0,6 }, { 1,5 },
	{ 1,5 }, { 0,4 },
	{ 0,4 }, { 0,1 },
	{ 0,1 }, { 1,0 }, 
	{ 1,5 }, { 9,5 }}
};

struct st_9 { int cnt; XPoint data[18]; } char_9 =
{
	18,
	{{ 1,10 }, { 9,10 },
	{ 9,10 }, { 10,9 },
	{ 10,9 }, { 10,1 },
	{ 10,1 }, { 9,0 },
	{ 9,0 }, { 1,0 },
	{ 1,0 }, { 0,1 },
	{ 0,1 }, { 0,4 },
	{ 0,4 }, { 1,5 },
	{ 1,5 }, { 10,5 }}
};

struct st_0 { int cnt; XPoint data[18]; } char_0 =
{
	18,
	{{ 3,0 }, { 7,0 },
	{ 7,0 }, { 10,3 },
	{ 10,3 }, { 10,7 },
	{ 10,7 }, { 7,10 }, 
	{ 7,10 }, { 3,10 },
	{ 3,10 }, { 0,7 },
	{ 0,7 }, { 0,3 },
	{ 0,3 }, { 3,0 },
	{ 3,10 }, { 7,0 }}
};

struct st_qmark { int cnt; XPoint data[16]; } char_qmark =
{
	16,
	{{ 0,2 }, { 2,0 },
	{ 2,0 }, { 9,0 },
	{ 9,0 }, { 10,1 },
	{ 10,1 }, { 10,4 },
	{ 10,4 }, { 9,5 },
	{ 9,5 }, { 5,5 },
	{ 5,5 }, { 5,7 },
	{ 5,9 }, { 5,10 }}
};

struct st_exmark { int cnt; XPoint data[8]; } char_exmark =
{
	8,
	{{ 4,0 }, { 6,0 },
	{ 6,0 }, { 5,7 },
	{ 5,7 }, { 4,0 },
	{ 5,9 }, { 5,10 }}
};

struct st_plus { int cnt; XPoint data[4]; } char_plus =
{
	4,
	{{ 5,0 }, { 5,10 },
	{ 0,5 }, { 10,5 }}
};

struct st_minus { int cnt; XPoint data[2]; } char_minus =
{
	2,
	{{ 0,5 }, { 10,5 }}
};

struct st_star { int cnt; XPoint data[8]; } char_star =
{
	8,
	{{ 0,5 }, { 10,5 },
	{ 5,0 }, { 5,10 },
	{ 1,1 }, { 9,9 },
	{ 1,9 }, { 9,1 }}
};

struct st_equals { int cnt; XPoint data[4]; } char_equals =
{
	4,
	{{ 0,3 }, { 10,3 },
	{ 0,7 }, { 10,7 }}
};

struct st_dot { int cnt; XPoint data[10]; } char_dot =
{
	8,
	{{ 4,8 }, { 6,8 },
	{ 6,8 }, { 6,10 },
	{ 6,10 }, { 4,10 },
	{ 4,10 }, { 4,8 }}
};

struct st_comma { int cnt; XPoint data[4]; } char_comma =
{
	4,
	{{ 7,6 }, { 6,9 },
	{ 6,9 }, { 4,10 }}
};

struct st_lrbracket { int cnt; XPoint data[10]; } char_lrbracket =
{
	10,
	{{ 6,0 }, { 5,0 },
	{ 5,0 }, { 3,2 },
	{ 3,2 }, { 3,8 },
	{ 3,8 }, { 5,10 },
	{ 5,10 }, { 6,10 }}
};

struct st_rrbracket { int cnt; XPoint data[10]; } char_rrbracket =
{
	10,
	{{ 3,0 }, { 4,0 },
	{ 4,0 }, { 6,2 },
	{ 6,2 }, { 6,8 },
	{ 6,8 }, { 4,10 },
	{ 4,10 }, { 3,10 }}
};

struct st_lcbracket { int cnt; XPoint data[12]; } char_lcbracket =
{
	12,
	{{ 6,0 }, { 4,1 },
	{ 4,1 }, { 4,4 },
	{ 4,4 }, { 2,5 },
	{ 2,5 }, { 4,6 },
	{ 4,6 }, { 4,9 },
	{ 4,9 }, { 6,10 }}
};

struct st_rcbracket { int cnt; XPoint data[12]; } char_rcbracket =
{
	12,
	{{ 4,0 }, { 6,1 },
	{ 6,1 }, { 6,4 },
	{ 6,4 }, { 8,5 },
	{ 8,5 }, { 6,6 },
	{ 6,6 }, { 6,9 },
	{ 6,9 }, { 4,10 }}
};

struct st_lsbracket { int cnt; XPoint data[6]; } char_lsbracket =
{
	6,
	{{ 7,0 }, { 3,0 },
	{ 3,0 }, { 3,10 },
	{ 3,10 }, { 7,10 }}
};

struct st_rsbracket { int cnt; XPoint data[6]; } char_rsbracket =
{
	6,
	{{ 3,0 }, { 7,0 },
	{ 7,0 }, { 7,10 },
	{ 7,10 }, { 3,10 }}
};

struct st_dollar { int cnt; XPoint data[26]; } char_dollar =
{
	26,
	{{ 10,2 }, { 9,1 },
	{ 9,1 }, { 1,1 },
	{ 1,1 }, { 0,2 },
	{ 0,2 }, { 0,4 }, 
	{ 0,4 }, { 1,5 },
	{ 1,5 }, { 9,5 },
	{ 9,5 }, { 10,6 },
	{ 10,6 }, { 10,8 },
	{ 10,8 }, { 9,9 }, 
	{ 9,9 }, { 1,9 },
	{ 1,9 }, { 0,8 },
	{ 4,0 }, { 4,10 },
	{ 6,0 }, { 6,10 }}
};

struct st_hash { int cnt; XPoint data[8]; } char_hash =
{
	8,
	{{ 0,3 }, { 10,3 },
	{ 0,7 }, { 10,7 },
	{ 3,0 }, { 3,10 },
	{ 7,0 }, { 7,10 }}
};

struct st_fslash { int cnt; XPoint data[4]; } char_fslash =
{
	2,
	{{ 10,0 }, { 0,10 }}
};

struct st_bslash { int cnt; XPoint data[4]; } char_bslash =
{
	2,
	{{ 0,0 }, { 10,10 }}
};

struct st_less { int cnt; XPoint data[4]; } char_less =
{
	4,
	{{ 10,0 }, { 0,5 },
	{ 0,5 }, { 10,10 }}
};

struct st_greater { int cnt; XPoint data[4]; } char_greater =
{
	4,
	{{ 0,0 }, { 10,5 },
	{ 10,5 }, { 0,10 }}
};

struct st_underscore { int cnt; XPoint data[2]; } char_underscore =
{
	2,
	{{ 0,10 }, { 10,10 }}
};

struct st_bar { int cnt; XPoint data[2]; } char_bar =
{
	2,
	{{ 5,0 }, { 5,10 }}
};

struct st_squote { int cnt; XPoint data[2]; } char_squote =
{
	2,
	{{ 6,0 }, { 4,3 }}
};

struct st_dquote { int cnt; XPoint data[4]; } char_dquote =
{
	4,
	{{ 3,0 }, { 3,2 },
	{ 7,0 }, { 7,2 }}
};

struct st_bquote { int cnt; XPoint data[2]; } char_bquote =
{
	2,
	{{ 4,0 }, { 6,3 }}
};

struct st_colon { int cnt; XPoint data[16]; } char_colon =
{
	16,
	{{ 4,1 }, { 6,1 },
	{ 6,1 }, { 6,3 },
	{ 6,3 }, { 4,3 },
	{ 4,3 }, { 4,1 },
	{ 4,7 }, { 6,7 },
	{ 6,7 }, { 6,9 },
	{ 6,9 }, { 4,9 },
	{ 4,9 }, { 4,7 }}
};

struct st_semicolon { int cnt; XPoint data[12]; } char_semicolon =
{
	12,
	{{ 4,1 }, { 6,1 },
	{ 6,1 }, { 6,3 },
	{ 6,3 }, { 4,3 },
	{ 4,3 }, { 4,1 },
	{ 6,6 }, { 6,8 },
	{ 6,8 }, { 4,10 }}
};	

struct st_at { int cnt; XPoint data[36]; } char_at =
{
	36,
	{{ 7,6 }, { 7,4 },
	{ 7,4 }, { 6,3 },
	{ 6,3 }, { 4,3 },
	{ 4,3 }, { 3,4 },
	{ 3,4 }, { 3,6 },
	{ 3,6 }, { 4,7 },
	{ 4,7 }, { 6,7 },
	{ 6,7 }, { 7,6 },
	{ 7,6 }, { 8,7 },
	{ 8,7 }, { 9,6 },
	{ 9,6 }, { 9,2 },
	{ 9,2 }, { 7,0 },
	{ 7,0 }, { 2,0 },
	{ 2,0 }, { 0,2 },
	{ 0,2 }, { 0,8 },
	{ 0,8 }, { 2,10 },
	{ 2,10 }, { 8,10 },
	{ 8,10 }, { 10,8 }}
};

struct st_hat { int cnt; XPoint data[4]; } char_hat =
{
	4,
	{{ 5,0 }, { 1,5 },
	{ 5,0 }, { 9,5 }}
};

struct st_tilda { int cnt; XPoint data[6]; } char_tilda =
{
	6,
	{{ 1,2 }, { 3,0 },
	{ 3,0 }, { 5,2 },
	{ 5,2 }, { 7,0 }}
};

struct st_ampersand { int cnt; XPoint data[26]; } char_ampersand =
{
	26,
	{{ 9,9 }, { 8,10 },
	{ 8,10 }, { 1,3 },
	{ 1,3 }, { 1,1 }, 
	{ 1,1 }, { 2,0 },
	{ 2,0 }, { 5,0 },
	{ 5,0 }, { 6,1 },
	{ 6,1 }, { 6,3 },
	{ 6,3 }, { 0,7 },
	{ 0,7 }, { 0,9 },
	{ 0,9 }, { 1,10 }, 
	{ 1,10 }, { 6,10 },
	{ 6,10 }, { 8,8 },
	{ 8,8 }, { 8,7 }}
};

struct st_percent { int cnt; XPoint data[18]; } char_percent =
{
	18,
	{{ 0,10 }, { 10,0 },

	{ 0,0 }, { 4,0 },
	{ 4,0 }, { 4,4 },
	{ 4,4 }, { 0,4 },
	{ 0,4 }, { 0,0 },

	{ 6,6 }, { 10,6 },
	{ 10,6 }, { 10,10 },
	{ 10,10 }, { 6,10 },
	{ 6,10 }, { 6,6 }}
};
#endif

EXTERN st_char_template *ascii_table[256];

////////////////////////////////// GLOBALS ///////////////////////////////////

EXTERN Display *display;
EXTERN Window win;
EXTERN Drawable drw;
EXTERN GC gc[NUM_COLOURS];
EXTERN XColor xcol[NUM_COLOURS];
EXTERN XdbeSwapInfo swapinfo;
EXTERN XPoint tmp_points[MAX_TMP_POINTS];

EXTERN en_game_stage game_stage;

EXTERN char *alsa_device;
EXTERN int ground_colour;
EXTERN int game_stage_cnt;
EXTERN int level;
EXTERN int lives;
EXTERN int lives_at_level_start;
EXTERN int score;
EXTERN int high_score;
EXTERN int level_cnt;
EXTERN int nugget_cnt;
EXTERN int win_width;
EXTERN int win_height;
EXTERN int win_refresh;
EXTERN int refresh_cnt;
EXTERN int eating_time;
EXTERN int invisible_powerup_cnt;
EXTERN int superball_powerup_cnt;
EXTERN int freeze_powerup_cnt;
EXTERN int bonus_nugget_cnt;
EXTERN int turbo_enemy_powerup_cnt;
EXTERN int spooky_create_mod;
EXTERN int grubble_create_mod;
EXTERN int first_spooky_cnt;
EXTERN int bonus_life_score;
EXTERN int wurmals_killed;
EXTERN int end_of_level_bonus;

EXTERN double x_scaling;
EXTERN double y_scaling;
EXTERN double avg_scaling;
EXTERN double materialise_y_add;

EXTERN bool paused;
EXTERN bool done_high_score;

EXTERN char tunnel_bitmap[SCR_SIZE][SCR_SIZE];
EXTERN cl_object *objects[MAX_OBJECTS];
EXTERN cl_object *stones[MAX_STONES];
EXTERN cl_enemy *attract_enemy[NUM_ATTRACT_ENEMIES];
EXTERN cl_player *player;
EXTERN cl_ball *ball;
EXTERN cl_molehill molehill[NUM_MOLEHILLS];
EXTERN vector<cl_tunnel *> tunnels;

EXTERN cl_text *text_digger;
EXTERN cl_text *text_copyright;
EXTERN cl_text *text_s_to_start;
EXTERN cl_text *text_level_start;
EXTERN cl_text *text_ready;
EXTERN cl_text *text_paused;
EXTERN cl_text *text_game_over;
EXTERN cl_text *text_got_spiky;
EXTERN cl_text *text_bonus_score[NUM_BONUS_SCORES];
EXTERN cl_text *text_invisibility_powerup;
EXTERN cl_text *text_superball_powerup;
EXTERN cl_text *text_freeze_powerup;
EXTERN cl_text *text_new_high_score;
EXTERN cl_text *text_bonus_life;

EXTERN char score_text[10];
EXTERN char high_score_text[10];
EXTERN char lives_text[10];
EXTERN char level_text[10];
EXTERN char version_text[50];
EXTERN char end_of_level_bonus_str[20];

#ifdef SOUND
EXTERN bool do_sound;
EXTERN bool do_fragment;
EXTERN bool do_soundtest;
#endif

//////////////////////////// FORWARD DECLARATIONS ////////////////////////////

// common.cc
void setGameStage(en_game_stage stg);
void initLevel();
void deactivateAllObjects();
void activateObjectsTotal(en_type type, int num);
void activateObjects(en_type type, int num);
void activateBonusScore(cl_object *obj, int bonus);
void setScaling();
void setScore(int val);
void incScore(int val);
void setLives(int val);
void setGroundColour();
void rotate(double &x,double &y, double ang);
void rotate(short &x,short &y, double ang);
void attainAngle(double &ang, double req_ang, int inc);
void incAngle(double &ang, double inc);
double angleDiff(double ang, double pang);
bool offscreen(int x, int y);

// tunnels.cc
cl_tunnel *createTunnel(int x, int y);
void initTunnels();
void fillTunnelArea(int x1, int y1, int x2, int y2);
bool outsideTunnel(int x, int y);
bool insideTunnel(int x, int y);
int findShortestPath(
	int depth,
	int max_depth, cl_tunnel *from, cl_tunnel *to, cl_tunnel *&next);

// draw.cc
void drawAsciiTable();
void drawGameScreen();
void drawEnemyScreen();
void drawKeysScreen();
void drawLine(int col, double thick, double x1, double y1, double x2, double y2);
void drawOrFillCircle(
	int col, double thick, double diam, double x, double y, bool fill);
void drawOrFillPolygon(
	int col, double thick, XPoint *points, int num_points, bool fill);
void drawOrFillRectangle(
	int col,
	double thick, double x, double y, double w, double h, bool fill);
void drawText(
        const char *mesg,
        int col,
        int thick,
        double ang,
        double gap,
        double x_scale, double y_scale, double x, double y);
void drawChar(
	char c,
	int col,
	int thick,
	double ang, double x_scale, double y_scale, double x, double y);

// sound.cc
void startSoundDaemon();
void playFGSound(en_sound snd);
void playBGSound(en_sound snd);
void echoOn();
void echoOff();

