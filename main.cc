/*****************************************************************************
 DIGGER

 A DigDug / Mr Do 1980s style arcade game for X Windows.

 Copyright (C) Neil Robertson 2011-2023
 *****************************************************************************/

#define MAINFILE
#include "globals.h"

#define MAINLOOP_DELAY 20000

// Module forwards
void parseCmdLine(int argc, char **argv);
void Xinit();
void init();
void resetGameGlobals();

void mainloop();
u_int getTime();
void processXEvents();
void run();
void duringLevel();

// Local modules variables
char *disp;
bool use_db;


///////////////////////////////// START UP /////////////////////////////////

int main(int argc, char **argv)
{
	parseCmdLine(argc,argv);
#ifdef SOUND
	startSoundDaemon();
	if (do_soundtest)
	{
		// Just wait until child process exits
		int status = 0;
		wait(&status);
		exit(0);
	}
#endif
	Xinit();
	init();
	mainloop();
	return 0;
}




/*** Get the command line arguments ***/
void parseCmdLine(int argc, char **argv)
{
	const char *opt[] =
	{
		"disp",
		"size",
		"ref",
		"nodb",
#ifdef SOUND
		"nosnd",
		"nofrag",
		"sndtest",
#ifdef ALSA
		"adev",
#endif
#endif
		"ver"
	};
	enum
	{
		OPT_DISP,
		OPT_SIZE,
		OPT_REF,
		OPT_NODB,
#ifdef SOUND
		OPT_NOSND,
		OPT_NOFRAG,
		OPT_SNDTEST,
#ifdef ALSA
		OPT_ADEV,
#endif
#endif
		OPT_VER,

		OPT_END
	};
	int i,o;

	disp = NULL;
	win_width = SCR_SIZE;
	win_height = SCR_SIZE;
	win_refresh = 1;
	use_db = true;
#ifdef SOUND
	do_sound = true;
	do_fragment = true;
	do_soundtest = false;
	alsa_device = (char *)ALSA_DEVICE;
#endif

	for(i=1;i < argc;++i)
	{
		if (argv[i][0] != '-') goto USAGE;
		for(o=0;o != OPT_END;++o)
			if (!strcasecmp(opt[o],argv[i]+1)) break;

		switch(o)
		{
		case OPT_VER:
			puts("-=[ DIGGER ]=-\n");
			puts(COPYRIGHT);
			printf("Version     : %s\nBuild date  : %s\nSound system: ",
				VERSION,BUILD_DATE);
#ifdef SOUND
#ifdef ALSA
			puts("ALSA");
#else
			puts("OpenSound");
#endif
#else
			puts("<no sound>");
#endif
			exit(0);

		case OPT_NODB:
			use_db = false;
			continue;
#ifdef SOUND
		case OPT_NOSND:
			do_sound = false;
			continue;

		case OPT_NOFRAG:
			do_fragment = false;
			continue;

		case OPT_SNDTEST:
			do_soundtest = true;
			continue;
#endif
		}

		if (i++ == argc - 1) goto USAGE;

		switch(o)
		{
		case OPT_DISP:
			disp = argv[i];
			break;

		case OPT_SIZE:
			win_width = win_height = atoi(argv[i]);
			break;

		case OPT_REF:
			if ((win_refresh = atoi(argv[i])) < 1) goto USAGE;
			break;

#ifdef ALSA
		case OPT_ADEV:
			alsa_device = argv[i];
			break;
#endif

		default:
			goto USAGE;
		}
	}
	return;

	USAGE:
	printf("Usage: %s\n"
	       "       -disp <display>     : Set X display\n"
	       "       -size <pixels>      : Set window width and height\n"
	       "       -ref  <iterations>  : The number of mainloop iterations before the\n"
	       "                             window is redrawn. Default = 1\n"
#ifdef SOUND
#ifdef ALSA
	       "       -adev <ALSA device> : Set the ALSA device to use. Default = '%s'\n"
#endif
	       "       -nosnd              : Switch off sound (assuming its compiled in anyway).\n"
	       "       -nofrag             : If background sounds stutter try this option\n"
	       "                             though some short sounds might not work properly.\n"
	       "       -sndtest            : Play all the sound effects then exit.\n"
#endif
	       "       -nodb               : Don't use double buffering. For really old systems.\n"
	       "       -ver                : Print version info then exit\n",
		argv[0]
#ifdef ALSA
		,ALSA_DEVICE
#endif
		);
	exit(1);
}




/*** Set everything up ***/
void Xinit()
{
	XGCValues gcvals;
	XColor unused;
	XTextProperty title_prop;
	Colormap cmap;
	int screen;
	int stage;
	int white;
	int black;
	int cnt;
	u_char r,g,b;
	char colstr[5];
	char title[50];
	char *titleptr;
	char *sound_system;

	if (!(display = XOpenDisplay(disp)))
	{
		printf("ERROR: Can't connect to: %s\n",XDisplayName(disp));
		exit(1);
	}
	screen = DefaultScreen(display);
	black = BlackPixel(display,screen);
	white = WhitePixel(display,screen);
	cmap = DefaultColormap(display,screen);

	win = XCreateSimpleWindow(
		display,
		RootWindow(display,screen),
		0,0,win_width,win_height,0,white,black);

	XSetWindowBackground(display,win,black);

#ifdef SOUND
#ifdef ALSA
	sound_system = (char *)"ALSA";
#else
	sound_system = (char *)"OpenSound";
#endif
#else
	sound_system = (char *)"<no sound>";
#endif
	sprintf(title,"DIGGER v%s (%s)",VERSION,sound_system);
	titleptr = title;  // Function will crash if you pass address of array
	XStringListToTextProperty(&titleptr,1,&title_prop);
	XSetWMProperties(display,win,&title_prop,NULL,NULL,0,NULL,NULL,NULL);

	// Create colour GCs. 
	r = 0;
	g = 0xF;
	b = 0;
	stage = 1;
	
	for(cnt=0;cnt < NUM_COLOURS;++cnt) 
	{
		sprintf(colstr,"#%01X%01X%01X",r,g,b);
		if (!XAllocNamedColor(display,cmap,colstr,&xcol[cnt],&unused)) 
		{
			printf("WARNING: Can't allocate colour %s\n",colstr);
			xcol[cnt].pixel = white;
		}

		gcvals.foreground = xcol[cnt].pixel;
		gcvals.background = black;
		gc[cnt] = XCreateGC(
			display,win,GCForeground | GCBackground,&gcvals);

		switch(stage) 
		{
		case 1:
			// Green to turquoise
			if (++b == 0xF) ++stage;
			break;
	
		case 2:
			// Turquoise to blue
			if (!--g) ++stage;
			break;
	
		case 3:
			// Blue to mauve
			if (++r == 0xF) ++stage;
			break;
	
		case 4:
			// Mauve to red
			if (!--b) ++stage;
			break;
	
		case 5:
			// Red to yellow
			if (++g == 0xF) ++stage;
			break;
	
		case 6:
			// Yellow to green
			if (!--r) 
			{
				g = b = 0;
				++stage;
			} 
			break;

		case 7:
			// black to white
			++g;
			++b;
			if (r++ == 0xF)
			{
				r = g = b = 0;
				++stage;
			}
			break;

		case 8:
			// black to red
			if (r++ == 0xF)
			{
				r = g = b = 0;
				++stage;
			}
			break;

		case 9:
			// black to green
			if (g++ == 0xF)
			{
				r = g = b = 0;
				++stage;
			}
			break;

		case 10:
			// black to blue
			if (b++ == 0xF)
			{
				r = g = b = 0;
				++stage;
			}
			break;

		case 11:
			// black to mauve
			r++;
			if (b++ == 0xF)
			{
				r = g = b = 0;
				++stage;
			}
			break;

		case 12:
			// black to turquoise
			b++;
			if (g++ == 0xF)
			{
				r = g = b = 0;
				++stage;
			}
			break;

		case 13:
			// black to yellow
			r++;
			g++;
			break;
		}
	}

	if (use_db)
	{
		drw = (Drawable)XdbeAllocateBackBufferName(
			display,win,XdbeBackground);
		swapinfo.swap_window = win;
		swapinfo.swap_action = XdbeBackground;
	}
	else drw = win;

	XSelectInput(display,win,
		ExposureMask | 
		StructureNotifyMask |
		KeyPressMask | 
		KeyReleaseMask);

	XMapWindow(display,win);

	setScaling();
}




/*** Set up pretty much everything non X related ***/
void init()
{
	int i;
	int j;

	sprintf(version_text,"V%s, %s",VERSION,BUILD_DATE);

	srandom(time(0));

	// Set up ascii tables. Taken from peniten-6
	for(i=0;i < 256;++i) ascii_table[i] = NULL;

	ascii_table[(int)'A'] = (st_char_template *)&char_A;
	ascii_table[(int)'B'] = (st_char_template *)&char_B;
	ascii_table[(int)'C'] = (st_char_template *)&char_C;
	ascii_table[(int)'D'] = (st_char_template *)&char_D;
	ascii_table[(int)'E'] = (st_char_template *)&char_E;
	ascii_table[(int)'F'] = (st_char_template *)&char_F;
	ascii_table[(int)'G'] = (st_char_template *)&char_G;
	ascii_table[(int)'H'] = (st_char_template *)&char_H;
	ascii_table[(int)'I'] = (st_char_template *)&char_I;
	ascii_table[(int)'J'] = (st_char_template *)&char_J;
	ascii_table[(int)'K'] = (st_char_template *)&char_K;
	ascii_table[(int)'L'] = (st_char_template *)&char_L;
	ascii_table[(int)'M'] = (st_char_template *)&char_M;
	ascii_table[(int)'N'] = (st_char_template *)&char_N;
	ascii_table[(int)'O'] = (st_char_template *)&char_O;
	ascii_table[(int)'P'] = (st_char_template *)&char_P;
	ascii_table[(int)'Q'] = (st_char_template *)&char_Q;
	ascii_table[(int)'R'] = (st_char_template *)&char_R;
	ascii_table[(int)'S'] = (st_char_template *)&char_S;
	ascii_table[(int)'T'] = (st_char_template *)&char_T;
	ascii_table[(int)'U'] = (st_char_template *)&char_U;
	ascii_table[(int)'V'] = (st_char_template *)&char_V;
	ascii_table[(int)'W'] = (st_char_template *)&char_W;
	ascii_table[(int)'X'] = (st_char_template *)&char_X;
	ascii_table[(int)'Y'] = (st_char_template *)&char_Y;
	ascii_table[(int)'Z'] = (st_char_template *)&char_Z;

	ascii_table[(int)'a'] = (st_char_template *)&char_A;
	ascii_table[(int)'b'] = (st_char_template *)&char_B;
	ascii_table[(int)'c'] = (st_char_template *)&char_C;
	ascii_table[(int)'d'] = (st_char_template *)&char_D;
	ascii_table[(int)'e'] = (st_char_template *)&char_E;
	ascii_table[(int)'f'] = (st_char_template *)&char_F;
	ascii_table[(int)'g'] = (st_char_template *)&char_G;
	ascii_table[(int)'h'] = (st_char_template *)&char_H;
	ascii_table[(int)'i'] = (st_char_template *)&char_I;
	ascii_table[(int)'j'] = (st_char_template *)&char_J;
	ascii_table[(int)'k'] = (st_char_template *)&char_K;
	ascii_table[(int)'l'] = (st_char_template *)&char_L;
	ascii_table[(int)'m'] = (st_char_template *)&char_M;
	ascii_table[(int)'n'] = (st_char_template *)&char_N;
	ascii_table[(int)'o'] = (st_char_template *)&char_O;
	ascii_table[(int)'p'] = (st_char_template *)&char_P;
	ascii_table[(int)'q'] = (st_char_template *)&char_Q;
	ascii_table[(int)'r'] = (st_char_template *)&char_R;
	ascii_table[(int)'s'] = (st_char_template *)&char_S;
	ascii_table[(int)'t'] = (st_char_template *)&char_T;
	ascii_table[(int)'u'] = (st_char_template *)&char_U;
	ascii_table[(int)'v'] = (st_char_template *)&char_V;
	ascii_table[(int)'w'] = (st_char_template *)&char_W;
	ascii_table[(int)'x'] = (st_char_template *)&char_X;
	ascii_table[(int)'y'] = (st_char_template *)&char_Y;
	ascii_table[(int)'z'] = (st_char_template *)&char_Z;

	ascii_table[(int)'0'] = (st_char_template *)&char_0;
	ascii_table[(int)'1'] = (st_char_template *)&char_1;
	ascii_table[(int)'2'] = (st_char_template *)&char_2;
	ascii_table[(int)'3'] = (st_char_template *)&char_3;
	ascii_table[(int)'4'] = (st_char_template *)&char_4;
	ascii_table[(int)'5'] = (st_char_template *)&char_5;
	ascii_table[(int)'6'] = (st_char_template *)&char_6;
	ascii_table[(int)'7'] = (st_char_template *)&char_7;
	ascii_table[(int)'8'] = (st_char_template *)&char_8;
	ascii_table[(int)'9'] = (st_char_template *)&char_9;

	ascii_table[(int)' '] = (st_char_template *)&char_space;
	ascii_table[(int)'?'] = (st_char_template *)&char_qmark;
	ascii_table[(int)'!'] = (st_char_template *)&char_exmark;
	ascii_table[(int)'+'] = (st_char_template *)&char_plus;
	ascii_table[(int)'-'] = (st_char_template *)&char_minus;
	ascii_table[(int)'*'] = (st_char_template *)&char_star;
	ascii_table[(int)'='] = (st_char_template *)&char_equals;
	ascii_table[(int)'.'] = (st_char_template *)&char_dot;
	ascii_table[(int)','] = (st_char_template *)&char_comma;
	ascii_table[(int)'('] = (st_char_template *)&char_lrbracket;
	ascii_table[(int)')'] = (st_char_template *)&char_rrbracket;
	ascii_table[(int)'{'] = (st_char_template *)&char_lcbracket;
	ascii_table[(int)'}'] = (st_char_template *)&char_rcbracket;
	ascii_table[(int)'['] = (st_char_template *)&char_lsbracket;
	ascii_table[(int)']'] = (st_char_template *)&char_rsbracket;
	ascii_table[(int)'$'] = (st_char_template *)&char_dollar;
	ascii_table[(int)'#'] = (st_char_template *)&char_hash;
	ascii_table[(int)'/'] = (st_char_template *)&char_fslash;
	ascii_table[(int)'\\'] = (st_char_template *)&char_bslash;
	ascii_table[(int)'>'] = (st_char_template *)&char_greater;
	ascii_table[(int)'<'] = (st_char_template *)&char_less;
	ascii_table[(int)'_'] = (st_char_template *)&char_underscore;
	ascii_table[(int)'|'] = (st_char_template *)&char_bar;
	ascii_table[(int)'\''] = (st_char_template *)&char_squote;
	ascii_table[(int)'"'] = (st_char_template *)&char_dquote;
	ascii_table[(int)'`'] = (st_char_template *)&char_bquote;
	ascii_table[(int)':'] = (st_char_template *)&char_colon;
	ascii_table[(int)';'] = (st_char_template *)&char_semicolon;
	ascii_table[(int)'@'] = (st_char_template *)&char_at;
	ascii_table[(int)'^'] = (st_char_template *)&char_hat;
	ascii_table[(int)'~'] = (st_char_template *)&char_tilda;
	ascii_table[(int)'&'] = (st_char_template *)&char_ampersand;
	ascii_table[(int)'%'] = (st_char_template *)&char_percent;

	// Adjust data values to have origin in centre , not top left. I'm
	// too lazy to adjust all that data itself.
	for(i=32;i < 256;++i)
	{
		// A & a , B & b etc share the same struct so don't do this
		// operation twice on them
		if (ascii_table[i] && (i < 'a' || i > 'z'))
		{
			for(j=0;j < ascii_table[i]->cnt;++j)
			{
				ascii_table[i]->data[j].x -= CHAR_HALF;
				ascii_table[i]->data[j].y -= CHAR_HALF;
			}
		}
	}

	// Game object creation
	objects[0] = player = new cl_player;
	objects[1] = ball = new cl_ball;

	for(i=0,j=2;i < MAX_NUGGETS;++i,++j) objects[j] = new cl_nugget;
	for(i=0;i < MAX_BOULDERS;++i,++j)    objects[j] = new cl_boulder;
	for(i=0;i < MAX_SPOOKYS;++i,++j)     objects[j] = new cl_spooky;
	for(i=0;i < MAX_SPIKYS;++i,++j)      objects[j] = new cl_spiky;
	for(i=0;i < MAX_GRUBBLES;++i,++j)    objects[j] = new cl_grubble;
	for(i=0;i < MAX_WURMALS;++i,++j)     objects[j] = new cl_wurmal;

	// Stones belong in their own list since they do nothing and don't
	// interact with any other objects
	for(i=0;i < MAX_STONES;++i) stones[i] = new cl_stone;

	attract_enemy[0] = new cl_spooky;
	attract_enemy[1] = new cl_grubble;
	attract_enemy[2] = new cl_wurmal;
	attract_enemy[3] = new cl_spiky;

	// Text object creation
	text_digger = new cl_text(cl_text::TXT_DIGGER);
	text_s_to_start = new cl_text(cl_text::TXT_S_TO_START);
	text_copyright = new cl_text(cl_text::TXT_COPYRIGHT);
	text_level_start = new cl_text(cl_text::TXT_LEVEL_START);
	text_ready = new cl_text(cl_text::TXT_READY);
	text_paused = new cl_text(cl_text::TXT_PAUSED);
	text_game_over = new cl_text(cl_text::TXT_GAME_OVER);
	text_invisibility_powerup = new cl_text(cl_text::TXT_INVISIBILITY_POWERUP);
	text_superball_powerup = new cl_text(cl_text::TXT_SUPERBALL_POWERUP);
	text_freeze_powerup = new cl_text(cl_text::TXT_FREEZE_POWERUP);
	text_new_high_score = new cl_text(cl_text::TXT_NEW_HIGH_SCORE);
	text_bonus_life = new cl_text(cl_text::TXT_BONUS_LIFE);
	text_got_spiky = new cl_text(cl_text::TXT_GOT_SPIKY);

	for(i=0;i < NUM_BONUS_SCORES;++i)
		text_bonus_score[i] = new cl_text(cl_text::TXT_BONUS_SCORE);

	// Miscellanious
	high_score = 10000;
	done_high_score = false;
	resetGameGlobals();

	setGameStage(GAME_STAGE_ATTRACT_PLAY);
}




/*** Reset some stuff not done in initLevel() ***/
void resetGameGlobals()
{
	setScore(0);
	setLives(3);
	paused = false;
	done_high_score = false;
	bonus_life_score = BONUS_LIFE_INC;
}


////////////////////////////////// RUNTIME ///////////////////////////////////


/*** Get the events and draw the points ***/
void mainloop()
{
	u_int tm1;
	u_int tm2;
	int diff;
	int i;

	for(refresh_cnt=0;;refresh_cnt = (refresh_cnt + 1) % win_refresh)
	{
		tm1 = getTime();

		if (!refresh_cnt && !use_db)
			XClearWindow(display,win);
		processXEvents();

		// Switch on game stages 
		switch(game_stage)
		{
		case GAME_STAGE_ATTRACT_PLAY:
			if (game_stage_cnt < 0)
			{
				drawAsciiTable();
				goto SKIP;
			}
			else if (game_stage_cnt == 1000)
				setGameStage(GAME_STAGE_ATTRACT_ENEMIES);
			else
				run();
			break;

		case GAME_STAGE_ATTRACT_ENEMIES:
			if (game_stage_cnt == 300)
				setGameStage(GAME_STAGE_ATTRACT_KEYS);
			else
			{
				for(i=0;i < NUM_ATTRACT_ENEMIES;++i)
					attract_enemy[i]->attractRun();
				drawEnemyScreen();
			}
			goto SKIP;

		case GAME_STAGE_ATTRACT_KEYS:
			if (game_stage_cnt == 200)
				setGameStage(GAME_STAGE_ATTRACT_PLAY);
			else
				drawKeysScreen();
			goto SKIP;

		case GAME_STAGE_LEVEL_START:
			if (game_stage_cnt == 50)
				setGameStage(GAME_STAGE_READY);
			break;

		case GAME_STAGE_READY:
			if (game_stage_cnt == 50)
				setGameStage(GAME_STAGE_PLAY);
			break;

		case GAME_STAGE_PLAY:
			if (!paused) run();
			break;

		case GAME_STAGE_LEVEL_COMPLETE:
			if (game_stage_cnt == 150)
			{
				++level;
				setGroundColour();
				setGameStage(GAME_STAGE_LEVEL_START);
			}
			else ground_colour = (game_stage_cnt * 2) % COL_GREEN2;
			break;
				
		case GAME_STAGE_PLAYER_DIED:
			if (game_stage_cnt == 10)
				setGameStage(GAME_STAGE_READY);
			break;

		case GAME_STAGE_GAME_OVER:
			if (game_stage_cnt == 250)
			{
				resetGameGlobals();
				setGameStage(GAME_STAGE_ATTRACT_PLAY);
			}
			break;

		default:
			assert(0);
		}
		drawGameScreen();

		SKIP:
		if (!paused) ++game_stage_cnt;

		if (!refresh_cnt)
		{
			if (use_db) XdbeSwapBuffers(display,&swapinfo,1);
			XFlush(display);
		}

		// Timing delay code taken from final_gun.
		if ((tm2 = getTime()) > tm1)
		{
			diff = (int)(tm2 - tm1);
			if (diff < MAINLOOP_DELAY)
				usleep(MAINLOOP_DELAY - diff);
		}
		else usleep(MAINLOOP_DELAY);
	}
}




/*** Get the current time down to the microsecond. Value wraps once every
     1000 seconds ***/
u_int getTime()
{
	timeval tv;
	gettimeofday(&tv,0);
	return (u_int)(tv.tv_sec % 1000) * 1000000 + tv.tv_usec;
}




/*** See what the X server has sent us ***/
void processXEvents()
{
	XWindowAttributes wa;
	XEvent event;
	KeySym ksym;
	char key;

	while(XPending(display))
	{
		XNextEvent(display,&event);

		switch(event.type)
		{
		case Expose:
			// Some window managers can shrink a window when it 
			// first appears and we get this in the expose event
			XGetWindowAttributes(display,win,&wa);
			win_width = wa.width;
			win_height = wa.height;
			setScaling();
			break;

		case ConfigureNotify:
			XGetWindowAttributes(display,win,&wa);
			win_width = event.xconfigure.width;
			win_height = event.xconfigure.height;
			setScaling();
			break;

		case UnmapNotify:
			if (game_stage == GAME_STAGE_PLAY)
			{
				// Auto pause if window unmapped
				paused = true;
				text_paused->reset();
			}
			break;

		case KeyRelease:
			if (game_stage == GAME_STAGE_PLAY)
			{
				XLookupString(&event.xkey,&key,1,&ksym,NULL);

				switch(ksym)
				{
				case XK_Left:
				case XK_Right:
				case XK_Up:
				case XK_Down:
					player->stop(ksym);
				}
			}
			break;

		case KeyPress:
			XLookupString(&event.xkey,&key,1,&ksym,NULL);
			switch(ksym)
			{
#ifdef SOUND
			case XK_v:
			case XK_V:
				do_sound = !do_sound;
				break;
#endif

			case XK_p:
			case XK_P:
				if (game_stage == GAME_STAGE_PLAY)
				{
					if (paused)
					{
						paused = false;
						break;
					}
					paused = true;
					text_paused->reset();
				}
				break;

			case XK_s:
			case XK_S:
				if (IN_ATTRACT_MODE())
				{
					level = 1;
					resetGameGlobals();
					setGameStage(GAME_STAGE_LEVEL_START);
					// Echo switched off when player
					// activated
					echoOn();
					playFGSound(SND_START);
				}
				break;

			case XK_Left:
			case XK_Right:
			case XK_Up:
			case XK_Down:
				if (game_stage == GAME_STAGE_PLAY)
					player->move(ksym);
				break;

			case XK_space:
				if (game_stage == GAME_STAGE_PLAY)
					player->throwBall();
				break;

			case XK_question:
				if (IN_ATTRACT_MODE()) game_stage_cnt = -200;
				break;

			case XK_plus:
				level = IN_ATTRACT_MODE() ? 1 : level+1;
				resetGameGlobals();
				setGameStage(GAME_STAGE_LEVEL_START);
				break;

			case XK_Escape:
				if (IN_ATTRACT_MODE()) exit(0);
				resetGameGlobals();
				setGameStage(GAME_STAGE_ATTRACT_PLAY);
				break;
			}
			break;

		default:
			break;
		}
	}
}




/*** Run everything and check for collisions ***/
void run()
{
	double dist;
	int o;
	int p;

	// If player has died flick ground colour and reset to appropriate 
	// game stage
	if (player->stage == STAGE_EXPLODE)
	{
		if (player->stage_cnt < 10)
			ground_colour = random() % NUM_FULL_COLOURS;
		else if (player->stage_cnt == 100)
		{
			setGameStage(IN_ATTRACT_MODE() ? 
			             GAME_STAGE_ATTRACT_ENEMIES : GAME_STAGE_PLAYER_DIED);
			return;
		}
		else setGroundColour(); 
	}

	// Run objects
	for(auto obj: objects) if (obj->stage != STAGE_INACTIVE) obj->run();

	// Check for collions in a seperate loop so all objects have already
	// run.
	for(o=0;o < MAX_OBJECTS-1;++o)
	{
		cl_object *obj1 = objects[o];
		switch(obj1->stage)
		{
		case STAGE_RUN:
		case STAGE_WOBBLE:
		case STAGE_FALL:
		case STAGE_BEING_EATEN:
			break;

		default:
			continue;
		}

		for(p=o+1;p < MAX_OBJECTS;++p)
		{
			cl_object *obj2 = objects[p];
			switch(obj2->stage)
			{
			case STAGE_RUN:
			case STAGE_WOBBLE:
			case STAGE_FALL:
			case STAGE_BEING_EATEN:
				// Wurmal is special case - must always use its
				// overloaded version of function
				if (obj2->type == TYPE_WURMAL)
					dist = obj2->overlapDist(obj1);
				else
					dist = obj1->overlapDist(obj2);

				if (dist)
				{
					obj1->haveCollided(obj2,dist);
					obj2->haveCollided(obj1,dist);
				}
				break;

			default:
				break;
			}
		}
	}

	duringLevel();
}




/*** Timed events during a level. I use game_stage_cnt because its reset to 
     zero each time player dies. ***/
void duringLevel()
{
	if (!game_stage_cnt) return;

	// Check for player completing level.
	if (game_stage == GAME_STAGE_PLAY && !nugget_cnt)
	{
		setGameStage(GAME_STAGE_LEVEL_COMPLETE);
		playFGSound(SND_LEVEL_COMPLETE);
		return;
	}

	// Keep creating spookys at regular intervals
	if (game_stage_cnt == first_spooky_cnt || 
	    !(game_stage_cnt % spooky_create_mod)) 
		activateObjects(TYPE_SPOOKY,1);

	switch(level)
	{
	case 1:
		if (game_stage_cnt == 500 || game_stage_cnt == 1000) 
			activateObjects(TYPE_GRUBBLE,1);
		break;

	case 2:
		if (game_stage_cnt == 400 || 
		    game_stage_cnt == 800 ||
		    game_stage_cnt == 1200) activateObjects(TYPE_GRUBBLE,1);
		break;

	case 3:
		if (game_stage_cnt == 1)
			activateObjects(TYPE_WURMAL,1 - wurmals_killed);
		else
		if (!(game_stage_cnt % 300)) activateObjects(TYPE_GRUBBLE,1);
		break;

	case 4:
		if (game_stage_cnt == 1)
			activateObjects(TYPE_WURMAL,1 - wurmals_killed);
		else
		if (!(game_stage_cnt % 300)) activateObjects(TYPE_GRUBBLE,1);

		if (game_stage_cnt > 500 && !(random() % 400))
			activateObjectsTotal(TYPE_SPIKY,1);
		break;

	case 5:
	case 6:
		if (game_stage_cnt == 1)
			activateObjects(TYPE_WURMAL,1 - wurmals_killed);
		else
		if (!(game_stage_cnt % 250)) activateObjects(TYPE_GRUBBLE,1);

		if (game_stage_cnt > 500 && !(random() % 300))
			activateObjectsTotal(TYPE_SPIKY,1);
		break;

	case 7:
	case 8:
		if (game_stage_cnt == 1)
			activateObjects(TYPE_WURMAL,2 - wurmals_killed);
		else
		if (!(game_stage_cnt % 200)) activateObjects(TYPE_GRUBBLE,1);

		if (game_stage_cnt > 500 && !(random() % 300))
			activateObjects(TYPE_SPIKY,1);
		break;

	default:
		if (game_stage_cnt == 1)
			activateObjects(TYPE_WURMAL,2 - wurmals_killed);
		else
		if (!(game_stage_cnt % grubble_create_mod))
			activateObjects(TYPE_GRUBBLE,1);

		if (game_stage_cnt > 400 && !(random() % 200))
			activateObjects(TYPE_SPIKY,1);
		break;
	}
}
