/*****************************************************************************
  This is code for the sound daemon that communicates with the main game parent 
  process through shared memory and parent interface functions. This generates 
  sin, square, sawtooth and whitenoise sounds and has a low pass filter, 
  distortion and echo functionality.
 *****************************************************************************/

#include "globals.h"

#ifdef SOUND
#include <errno.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#ifdef ALSA
#include <alsa/asoundlib.h>
#else
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#endif

#define PCM_FREQ      20000
#define SNDBUFF_SIZE  (PCM_FREQ / 20) /* Sample is 1/20th sec */
#define ECHOBUFF_SIZE (SNDBUFF_SIZE * 7)
#define ECHO_MULT     0.4
#define DELAY_TIME    40000
#define NUM_CHANS     3
#define PCM_FREQ      20000
#define MAX_SHORT     32767
#define MIN_SHORT     -32768

#define PRIORITY(SND) (shm->fg < SND)

// Prevent wrapping - clip instead 
#define CLIP_AND_SET_BUFFER() \
	if (res > MAX_SHORT) res = MAX_SHORT; \
	else \
	if (res < MIN_SHORT) res = MIN_SHORT; \
	sndbuff[i] = (short)res;

// PCM buffers
short sndbuff[SNDBUFF_SIZE];
short echobuff[ECHOBUFF_SIZE];

#ifdef ALSA
snd_pcm_t *handle;
#else
int sndfd;
#endif

bool echo_on;
int echo_write_pos;
double echo_mult;


// Channel globals 
double sinw_ang[NUM_CHANS];
int sq_vol[NUM_CHANS];
int sq_cnt[NUM_CHANS];

double saw_val[NUM_CHANS];

// Shared mem
int shmid;

struct st_sharmem
{
	u_char echo;
	u_char bg;
	u_char fg;
} *shm;

// Forward declarations
void checkEcho();
void playSoundType(u_char snd);

// Foreground sounds
void playEatNugget();
void playBoulderWobble();
void playBoulderLand();
void playGrubbleEat();
void playBallBounce();
void playBallThrow();
void playBallReturn();
void playBoulderExplode();
void playSpikyDematerialise();
void playEnemyMaterialise();
void playSpikyMaterialise();
void playFall();
void playEnemyExplode();
void playSpookyHit();
void playGrubbleHit();
void playWurmalHit();
void playBonusScore();
void playFreezePowerup();
void playTurboEnemy();
void playHighScore();
void playBonusLife();
void playPlayerHit();
void playPlayerExplode();
void playLevelComplete();
void playGameOver();
void playStart();

// Background sounds
void playInvisibilityPowerup();
void playSuperballPowerup();

void resetSoundChannels();
void resetSoundBuffer();
void resetEchoBuffer();
void playSound();
void addSin(int ch, double vol, double freq, int reset);
void addSquare(int ch, double vol, double freq, int reset);
void addSawtooth(int ch, double vol, double freq, int reset);
void addNoise(double vol, int gap, int reset);
void addDistortion(int clip);
void filter(short sample_size, int reset);

// Sound priorities lowest -> highest
void (*playfunc[NUM_SOUNDS])() = 
{
	NULL,

	// Foreground
	playEatNugget,
	playBoulderWobble,
	playBoulderLand,
	playGrubbleEat,
	playBallBounce,
	playBallThrow,
	playBallReturn,
	playBoulderExplode,
	playSpikyDematerialise,
	playEnemyMaterialise,
	playSpikyMaterialise,
	playFall,
	playSpookyHit,
	playGrubbleHit,
	playWurmalHit,
	playBonusScore,
	playEnemyExplode,
	playFreezePowerup,
	playHighScore,
	playBonusLife,
	playPlayerHit,
	playPlayerExplode,
	playLevelComplete,
	playGameOver,
	playStart,

	// Background
	playInvisibilityPowerup,
	playSuperballPowerup,
	playTurboEnemy
};
#endif


///////////////////////// PARENT INTERFACE FUNCTIONS //////////////////////////

void initALSA();
void initOpenSound();
void closedown();
void soundLoop();

/*** Create the shared memory and spawn off the child sound daemon process.
     I could have used pthreads but fork() is probably more portable plus
     if the sound daemon crashes it won't bring down the game process. 
     This function runs whatever do_sound is set to since player may want
     to switch sound on in the game later ***/
void startSoundDaemon()
{
#ifdef SOUND
	int i;

	shmid = -1;
	echo_on = false;

#ifdef ALSA
	initALSA();
#else
	initOpenSound();
#endif
	/* Set up shared memory. 10 attempts at finding a key that works.
	   The shared memory has the following layout:

	     0        1          2 
	    --------- ---------- ----------
	   | echo on | bg sound | fg sound |
	    --------- ---------- ----------
	*/
	for(i=0;i < 10 && shmid == -1;++i)
	{
		shmid = shmget(
			(key_t)random(),
			sizeof(struct st_sharmem),IPC_CREAT | IPC_EXCL | 0666
			);
	}
	if (shmid == -1)
	{
		printf("SOUND: shmget(): %s\n",strerror(errno));
		closedown();
		return;
	}

	if ((shm = (struct st_sharmem *)shmat(shmid,NULL,0)) == (void *)-1)
	{
		printf("SOUND: shmat(): %s\n",strerror(errno));
		closedown();
		return;
	}

	// Mark for deletion when both processes have exited
	shmctl(shmid,IPC_RMID,0);

	bzero(shm,sizeof(struct st_sharmem));

	// Spawn off sound daemon as child process
	switch(fork())
	{
	case -1:
		printf("SOUND: fork(): %s\n",strerror(errno));
		closedown();
		return;

	case 0:
		// Child
		soundLoop();
		exit(0);
	}

	// Parent ends up here
#ifdef ALSA
	snd_pcm_close(handle);
#else
	close(sndfd);
#endif
#endif
}


#ifdef SOUND
#ifdef ALSA

/*** Use ALSA ***/
void initALSA()
{
	snd_pcm_hw_params_t *params;
	u_int freq = PCM_FREQ;
	int err;

	if ((err = snd_pcm_open(&handle,alsa_device,SND_PCM_STREAM_PLAYBACK,0)) < 0)
	{
		printf("SOUND: snd_pcm_open(): %s\n",snd_strerror(err));
		handle = NULL;
		return;
	}

	// Allocate hardware params structure 
	if ((err = snd_pcm_hw_params_malloc(&params)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_malloc(): %s\n",snd_strerror(err));
		closedown();
		return;
	}

	// Init structure 
	if ((err = snd_pcm_hw_params_any(handle,params)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_any(): %s\n",snd_strerror(err));
		closedown();
		return;
	}
	
	// Set interleaved regardless of mono or stereo 
	if ((err = snd_pcm_hw_params_set_access(handle,params,SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_set_access(): %s\n",snd_strerror(err));
		closedown();
		return;
	}

	// Set number of channels. 1 in this case because we want mono 
	if ((err = snd_pcm_hw_params_set_channels(handle,params,1)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_set_channels(): %s\n",snd_strerror(err));
		closedown();
		return;
	}

	// Set word format 
	if ((err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S16_LE)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_set_format(): %s\n",snd_strerror(err));
		closedown();
		return;
	}

	// Set PCM frequency 
	if ((err = snd_pcm_hw_params_set_rate_near(handle,params,&freq,0)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_set_rate_near(): %s\n",snd_strerror(err));
		closedown();
		return;
	}
	if (freq != PCM_FREQ)
	{
		printf("ERROR: Device PCM freq %dHz is not requested freq %dHz\n",
			freq,PCM_FREQ);
		closedown();
		return;
	}

	// Do actual set of parameters on device 
	if ((err = snd_pcm_hw_params(handle,params)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params(): %s\n",snd_strerror(err));
		closedown();
		return;
	}
	snd_pcm_hw_params_free(params);	

	// Not sure what this is for 
	if ((err = snd_pcm_prepare(handle)) < 0)
	{
		printf("SOUND: snd_pcm_prepare(): %s\n",snd_strerror(err));
		closedown();
		return;
	}
}


#else

void initOpenSound()
{
	u_int tmp;
	int frag;

	// Open sound device
	if ((sndfd=open("/dev/dsp",O_WRONLY)) == -1 &&
	    (sndfd=open("/dev/audio",O_WRONLY)) == -1)
	{
		printf("SOUND: Can't open /dev/dsp or /dev/audio: %s\n",strerror(errno));
		if (errno == EBUSY)
			puts("SOUND: Consider using 'aoss' wrapper script");
		return;
	}

	// Reset it
	if (ioctl(sndfd,SNDCTL_DSP_RESET) == -1)
	{
		printf("SOUND: Can't reset sound device: %s\n",
			strerror(errno));
		closedown();
		return;
	}

	// Set endian type
	tmp = AFMT_S16_NE;
	if (ioctl(sndfd,SNDCTL_DSP_SETFMT,&tmp) == -1)
	{
		printf("SOUND: ioctl(SNDCTL_DSP_SETFMT) failed: %s\n",
			strerror(errno));
		closedown();
		return;
	}

	tmp = PCM_FREQ;
	if (ioctl(sndfd,SNDCTL_DSP_SPEED,&tmp) == -1)
	{
		printf("SOUND: ioctl(SNDCTL_DSP_SPEED) failed: %s\n",
			strerror(errno));
		closedown();
		return;
	}
	if (tmp != PCM_FREQ)
	{
		printf("SOUND: Device PCM freq %dHz is not requested freq %dHz\n",
			tmp,PCM_FREQ);
		closedown();
		return;
	}

	/* Set so short sounds get played immediately, not buffered. This 
	   value seemd to work without messing the sounds up when played
	   direct however it seems to cause issues with background sounds
	   when sound done via aoss. *shrug*
	   http://manuals.opensound.com/developer/SNDCTL_DSP_SETFRAGMENT.html
	*/
	if (do_fragment)
	{
		frag = (2 << 16) | 10;
		if (ioctl(sndfd,SNDCTL_DSP_SETFRAGMENT,&frag))
		{
			printf("SOUND: ioctl(SNDCTL_DSP_SETFRAGMENT) failed: %s\n",
				strerror(errno));
			closedown();
			return;
		}
	}
}
#endif /* ALSA */
#endif /* SOUND */


/*** Obvious really ***/
void closedown()
{
#ifdef SOUND
#ifdef ALSA
	snd_pcm_close(handle);
	handle = NULL;
#else
	close(sndfd);
	sndfd = -1;
#endif
#endif
}




/*** Set up a short foreground sound - eg explosion. Level complete set 
     is a hack because SND_FILL will have been sey just before this but
     SM_FG might not yet be zero ***/
void playFGSound(en_sound snd)
{
#ifdef SOUND
#ifdef ALSA
	if (do_sound && handle && !IN_ATTRACT_MODE() && snd > shm->fg)
#else
	if (do_sound && sndfd != -1 && !IN_ATTRACT_MODE() && snd > shm->fg)
#endif
		shm->fg = snd;
#endif
}




/*** Set up a constant background sound. No priorities with BG sounds. ***/
void playBGSound(en_sound snd)
{
#ifdef SOUND
#ifdef ALSA
	if (!handle) return;
#else
	if (sndfd == -1) return;
#endif
	if (snd == SND_SILENCE) shm->bg = SND_SILENCE;
	else
	if (do_sound && !IN_ATTRACT_MODE()) shm->bg = snd;
#endif
}




void echoOn()
{
#ifdef SOUND
#ifdef ALSA
	if (handle) shm->echo = 1;
#else
	if (sndfd != -1) shm->echo = 1;
#endif
#endif
}




void echoOff()
{
#ifdef SOUND
#ifdef ALSA
	if (handle) shm->echo = 0;
#else
	if (sndfd != -1) shm->echo = 0;
#endif
#endif
}



///////////////////////////////// MAIN LOOP //////////////////////////////////

#ifdef SOUND

/*** Child process sits here, watches the shared memory segment and plays the 
     sounds ***/
void soundLoop()
{
	u_char snd;
	int check_cnt;

	resetSoundChannels();
	resetSoundBuffer();
	resetEchoBuffer();
	filter(0,1);

	if (do_soundtest)
	{
		// Play all the sounds then exit
		for(snd=1;snd < NUM_SOUNDS;++snd)
		{
			printf("%d\n",snd);
			resetSoundBuffer();
			playfunc[snd]();
		}
		sleep(1);
		exit(0);
	}

	for(check_cnt=0;;check_cnt = (check_cnt + 1) % 20)
	{
		/* Samples take 1/20th of a second to play so pause to let
		   the device buffer empty and so we don't kill the CPU.
		   Using the DSP_SYNC ioctl() leads to nasty stuttering. */
		usleep(DELAY_TIME);

		checkEcho();

		/* Foreground sounds are played once then stop. Background
		   are continuous until reset by parent process. Loop on 
		   foreground because if new one interrupts current we don't
		   want usleep delay */
		while(shm->fg != SND_SILENCE)
		{
			snd = shm->fg;
			shm->fg = 0;
			resetSoundBuffer();
			playSoundType(snd);

			checkEcho();
		}

		if (shm->bg != SND_SILENCE) playSoundType(shm->bg);
		else
		if (echo_on)
		{
			// Let any echos play out. Reset sound buffer so the 
			// echos fade away instead of building on each other.
			resetSoundBuffer();
			playSound();
		}

		// See if parent has died by checking if we've been reparented.
		// No need to check it on every loop however hence check_cnt
		if (!check_cnt && getppid() == 1)
		{
			puts("SOUND: Parent process dead - exiting");
			closedown();
			exit(0);
		}
	}
}




/*** Check for echo request and set flag on or off appropriately ***/
void checkEcho()
{
	if (shm->echo && !echo_on)
	{
		echo_on = true;
		resetEchoBuffer();
	}
	else if (!shm->echo) echo_on = false;
}




/*** Play a sound type. Foreground sounds play then stop and reset the
     foreground byte. Background sounds are called over and over until
     reset by parent process ***/
void playSoundType(u_char snd)
{
	assert(snd != SND_SILENCE && snd < NUM_SOUNDS);
	playfunc[snd]();
}


///////////////////////////// FOREGROUND SOUNDS ///////////////////////////////

void playEatNugget()
{
	addSin(0,MED_VOLUME,200,1);
	playSound();
}




void playBoulderWobble()
{
	int freq = 40;
	for(int i=0;i < 20 && PRIORITY(SND_BOULDER_WOBBLE);++i)
	{
		addSquare(0,LOW_VOLUME,freq,1);
		addNoise(LOW_VOLUME,30,0);
		filter(10,0);
		playSound();
		freq = (freq == 40 ? 20 : 40);
	}
}




void playBoulderLand()
{
	for(int i=0;i < 3 && PRIORITY(SND_BOULDER_LAND);++i)
	{
		addNoise(HIGH_VOLUME,15 + i * 10,1);
		addSquare(0,LOW_VOLUME,40,0);
		playSound();
	}
}




void playGrubbleEat()
{
	for(int i=0;i < 10 && PRIORITY(SND_GRUBBLE_EAT);++i)
	{
		addNoise(LOW_VOLUME,20,1);
		addSawtooth(0,LOW_VOLUME,60,0);
		playSound();
		/* Alsa is slower then Opensound */
#ifdef ALSA
		usleep(125000);
#else
		usleep(250000);
#endif
	}
}




void playBallBounce()
{
	if (!PRIORITY(SND_BALL_BOUNCE)) return;
	addSquare(0,LOW_VOLUME,70,1);
	filter(10,0);
	playSound();

	if (!PRIORITY(SND_BALL_BOUNCE)) return;
	addSquare(0,LOW_VOLUME,60,1);
	filter(10,0);
	playSound();
}




void playBallThrow()
{
	int freq = 100;
	int i;

	for(i=0;i < 5 && PRIORITY(SND_BALL_THROW);++i)
	{
		addSquare(0,LOW_VOLUME,freq,0);
		filter(10,0);
		playSound();
		freq *= 2;
	}
}




void playBallReturn()
{
	int vol = 0;
	int i;
	for(i=0;i < 15 && PRIORITY(SND_BALL_RETURN);++i)
	{
		addNoise(vol,40-i*2,1);
		playSound();
		vol += 1000;
	}
}




void playBoulderExplode()
{
	int vol = MED_VOLUME;

	for(int i=0;i < 20 && PRIORITY(SND_BOULDER_EXPLODE);++i)
	{
		addNoise(vol,50+i* 10,1);
		vol -= 1000;
		playSound();
	}
}




/*** Spiky dematerialising ***/
void playSpikyDematerialise()
{
	int vol = HIGH_VOLUME;
	int centre = 80;
	int freq;
	int i;

	for(i=0;centre > 10 && PRIORITY(SND_SPIKY_DEMATERIALISE);centre-=2,++i)
	{
		freq = centre + (i % 3) * 10;
		addSin(0,vol,freq,1);
		addSin(1,vol,freq+10,0);
		addSin(2,vol,freq+20,0);
		playSound();
		if (vol > 100) vol -= 500;
	}
}




/*** Spooky and Grubble ***/
void playEnemyMaterialise()
{
	int vol = 0;
	int add = 200;
	int i;

	for(i=0;i < 15 && PRIORITY(SND_ENEMY_MATERIALISE);++i)
	{
		addSquare(0,vol,80,1);
		addSquare(1,vol,81,0);
		addSawtooth(2,vol,82,0);
		playSound();

		vol += add;
		if (vol >= 1800)
		{
			vol -= add;
			add = -add;
		}
	}
}




/*** Spiky ***/
void playSpikyMaterialise()
{
	int vol = 1000;
	int freq;
	int i;

	for(i=0;i < 65 && PRIORITY(SND_SPIKY_MATERIALISE);++i)
	{
		freq = 60 + (i % 3) * 3;
		addSquare(0,vol,freq,1);
		addSquare(1,vol,freq+1,0);
		addSquare(2,vol,freq*2,0);
		filter(i * 2,0);
		playSound();
		if (vol < MED_VOLUME) vol += 500;
	}
}




void playFall()
{
	int start = 500;
	int freq = start;
	int i;

	for(i=0;i < 30 && PRIORITY(SND_FALL);++i)
	{
		addSin(0,LOW_VOLUME,freq,1);
		addSin(1,LOW_VOLUME,freq+50,0);
		addSin(2,LOW_VOLUME,freq+200,0);
		playSound();

		freq -= 75;
		if (freq < 0 || freq == start - 150)
		{
			start -= 50;
			freq = start;	
		}
	}
}




void playSpookyHit()
{
	int centre = 800;
	int ran = 200;
	int freq;
	int i;

	for(i=0;i < 30 && PRIORITY(SND_SPOOKY_HIT);++i)
	{
		freq = centre + (random() % ran) - (ran / 2);
		addSin(0,LOW_VOLUME,freq,1);
		addSin(1,LOW_VOLUME,freq+20,0);
		addSin(2,LOW_VOLUME,freq+40,0);
		if (i > 10)
		{
			centre -= 30; 
			ran = 100;
		}
		playSound();
	}
}




void playGrubbleHit()
{
	int freq = 300;
	int i;

	for(i=0;i < 15 && PRIORITY(SND_GRUBBLE_HIT);++i)
	{
		addSawtooth(0,LOW_VOLUME,freq,1);
		addSawtooth(1,LOW_VOLUME,freq+10,0);
		addSawtooth(2,LOW_VOLUME,freq+20,0);
		filter(20,0);
		playSound();
		freq -= 20;
	}
}




void playWurmalHit()
{
	int freq = 150;
	int i;

	for(i=0;i < 10 && PRIORITY(SND_WURMAL_HIT);++i)
	{
		addSin(0,HIGH_VOLUME,freq,1);
		addSin(1,HIGH_VOLUME,freq+3,0);
		addSin(2,HIGH_VOLUME,freq+6,0);
		addNoise(LOW_VOLUME,15 + i * 2,0);
		playSound();
		freq -= 15;
	}
}




void playBonusScore()
{
	int start = 300;
	int freq = start;

	for(int i=0;i < 15 && PRIORITY(SND_BONUS_SCORE);++i)
	{
		addSin(0,MED_VOLUME,freq,1);
		addSin(1,MED_VOLUME,freq+1,0);
		addSin(2,MED_VOLUME,freq+2,0);
		playSound();
		if (freq == start + 200)
		{
			start += 50;
			freq = start;
		}
		else freq += 100;
	}
}




void playEnemyExplode()
{
	int vol = HIGH_VOLUME;

	for(int i=0;i < 20 && PRIORITY(SND_ENEMY_EXPLODE);++i)
	{
		addNoise(vol,20+i*3,1);
		vol -= 1000;
		playSound();
	}
}




/*** Could have made it a background sound but chose to make it foreground
     with an echo instead. echoOn() in cl_player.cc ***/
void playFreezePowerup()
{
	int start = 300;
	int freq = 200;
	int i;

	for(i=0;i < 50 && PRIORITY(SND_FREEZE_POWERUP);++i)
	{
		addSin(0,MED_VOLUME,freq,i);
		addSin(1,MED_VOLUME,freq+10,0);
		addSin(2,MED_VOLUME,freq+20,0);
		playSound();
		freq += 50;
		if (freq == start + 600)
		{
			freq = start;
			start += 100;
		}
	}
}




void playHighScore()
{
	int freq;
	int i;

	for(i=0,freq=100;i < 30 && PRIORITY(SND_HIGH_SCORE);++i)
	{
		addSin(0,MED_VOLUME,freq,1);
		addSin(1,MED_VOLUME,freq+2,0);
		playSound();
		freq += 200;
		if (freq == 1100) freq = 100;
	}
}




void playBonusLife()
{
	int freq = 100;
	int vol = MED_VOLUME;
	int i;

	for(i=0;i < 20 && vol > 0 && PRIORITY(SND_BONUS_LIFE);++i)
	{
		addSquare(0,vol,freq,1);
		addSawtooth(1,vol,freq+2,0);
		addSawtooth(2,vol,freq+4,0);
		filter(i,!i);
		playSound();

		addSquare(0,vol,freq+50,1);
		addSquare(1,vol,freq+52,0);
		addSquare(2,vol,freq+54,0);
		filter(i,0);
		playSound();

		if (i == 4) freq = 150;
		else
		if (i == 8) freq = 200;

		vol -= 400;
	}
}




void playPlayerHit()
{
	double freq;
	int centre = 500;
	int ang = 0;
	int i;

	for(i=0;i < 20 && PRIORITY(SND_PLAYER_HIT);++i)
	{
		freq = SIN(ang) * 100 + centre;
		addSin(0,MED_VOLUME,(int)freq,1);
		addSin(1,MED_VOLUME,(int)freq+200,0);
		addDistortion(MED_VOLUME);
		playSound();

		ang = (ang + 120) % 360;
		centre -= 30;
	}
}




void playPlayerExplode()
{
	int vol = HIGH_VOLUME;
	int i;

	for(i=0;i < 60 && PRIORITY(SND_PLAYER_EXPLODE);++i,vol-=100)
	{
		addNoise(vol,i * 2,1);
		playSound();
	}
}




void playLevelComplete()
{
	int freq;
	int start;
	int i;

	start = freq = 100;
	for(i=1;i < 40 && PRIORITY(SND_LEVEL_COMPLETE);++i)
	{
		addSawtooth(0,MED_VOLUME,freq,1);
		addSawtooth(1,MED_VOLUME,freq+1,0);
		addSquare(2,MED_VOLUME,freq+2,0);
		filter(5 + abs(i % 10 - 5),0);
		playSound();

		if (i && !(i % 4))
		{
			start += 50;
			freq = start;
		}
		else freq += 100;
	}
}




void playGameOver()
{
	int vol = MED_VOLUME;
	int freq = 200;
	int add = 5;
	int i;

	for(i=0;i < 120 && PRIORITY(SND_GAME_OVER);++i)
	{
		addSquare(0,vol,freq,!(i % 10));
		addSquare(1,vol,freq+2,0);
		addSquare(2,vol,freq+4,0);
		filter(i / 5,0);
		playSound();

		freq += add;
		if (freq == 205) add = -add;
		else
		if (freq < 30)
		{
			freq = 30;
			vol -= 300;
			if (vol < 0) vol = 0;
		}
	}
}




/*** Player has pressed 'S' key ***/
void playStart()
{
	int freq = 100;
	int i;

	for(i=0;i < 45 && PRIORITY(SND_START);++i)
	{
		addSquare(0,LOW_VOLUME,freq,1);
		addSquare(1,LOW_VOLUME,freq+2,0);
		addSquare(2,LOW_VOLUME,freq+4,0);
		filter(i,0);
		playSound();
		if (i > 10 && i < 20) ++freq;
	}
}


///////////////////////////// BACKGROUND SOUNDS ///////////////////////////////


void playInvisibilityPowerup()
{
	static int add = 1;
	static int i = 0;

	addSin(0,LOW_VOLUME,300,1);
	addSin(1,LOW_VOLUME,200-i,0);
	addSin(2,LOW_VOLUME,200+i,0);
	playSound();
	i += add;
	if (i == 20 || !i) add = -add;
}




void playSuperballPowerup()
{
	static int f = 0;
	static int add = 1;

	addSquare(0,LOW_VOLUME,30,0);
	addSquare(0,LOW_VOLUME,31,0);
	filter(2 + f,0);
	playSound();

	f += add;
	if (f > 10 || f < 0) add = -add;
}




void playTurboEnemy()
{
	static double ang = 0;
	double freq_add = SIN(ang) * 100;
	static int filt = 0;
	static int filt_add = 1;

	addSawtooth(0,MED_VOLUME,500 + freq_add,1);
	addSawtooth(1,MED_VOLUME,500 - freq_add,0);
	addSawtooth(2,MED_VOLUME,600,0);
	filter(filt,0);

	playSound();

	incAngle(ang,60);
	filt += filt_add;
	if (!filt || filt == 10) filt_add = -filt_add;
}



/////////////////////////// LOW LEVEL FUNCTIONS ///////////////////////////////

void resetSoundChannels()
{
	bzero(sinw_ang,sizeof(sinw_ang));
	bzero(sq_vol,sizeof(sq_vol));
	bzero(sq_cnt,sizeof(sq_cnt));
	bzero(saw_val,sizeof(saw_val));
}




void resetSoundBuffer()
{
	bzero(sndbuff,sizeof(sndbuff));
}




void resetEchoBuffer()
{
	bzero(echobuff,sizeof(echobuff));
	echo_write_pos = 0;
}




/*** Play sound as-is or with an echo ***/
void playSound()
{
	int res;
	int i;
	int j;
	int len;
#ifdef ALSA
	int frames;
#else
	int bytes;
#endif

	if (echo_on)
	{
		// Update echo buffer
		for(i=0,j=echo_write_pos;i < SNDBUFF_SIZE;++i)
		{
			echobuff[j] = (short)((double)echobuff[j] * ECHO_MULT);
			res = (int)echobuff[j] + sndbuff[i];
			CLIP_AND_SET_BUFFER();
			echobuff[j] = sndbuff[i];
			j = (j + 1) % ECHOBUFF_SIZE;
		}
		echo_write_pos = j;
	}

#ifdef ALSA
	/* Write sound data to device. ALSA uses frames (frame = size of format
	   * number of channels so mono 16 bit = 2 bytes, stereo = 4 bytes) */
	for(len = sizeof(sndbuff)/sizeof(short),frames=1;
	    frames > 0 && len > 0;len -= frames)
	{
		/* Occasionally get underrun and need to recover because stream
		   won't work again until you do */
		if ((frames = snd_pcm_writei(handle,sndbuff,len)) < 0)
			snd_pcm_recover(handle,frames,1);
	}
#else
	/* Opensound uses write() so length is done in bytes */
	for(len = sizeof(sndbuff),bytes=0;bytes != -1 && len > 0;len -= bytes) 
		bytes = write(sndfd,sndbuff,sizeof(sndbuff));
#endif
}




/*** This has channels because starting from the previous angle prevents
     a clicking sound. If we didn't have this then playing 2 sin notes at
     the same time would be a mess. ***/
void addSin(int ch, double vol, double freq, int reset)
{
	double ang_inc;
	int res;
	int i;
	
	assert(ch < NUM_CHANS);

	if (freq < 1) freq = 1;
	if (reset) resetSoundBuffer();

	ang_inc = 360 / (PCM_FREQ / freq);

	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		res = sndbuff[i] + (int)(vol * SIN(sinw_ang[ch]));
		CLIP_AND_SET_BUFFER();

		sinw_ang[ch] += ang_inc;
		if (sinw_ang[ch] >= 360) sinw_ang[ch] -= 360;
	}
}




/*** Square wave. A proper square wave function would need to do some 
     interpolation to produce every frequency due to PCM sample size limits.
     This one doesn't. ***/
void addSquare(int ch, double vol, double freq, int reset)
{
	int period;
	int res;
	int i;

	assert(ch < NUM_CHANS);

	if (freq < 1) freq = 1;
	if (reset) resetSoundBuffer();

	period = (int)((PCM_FREQ / freq) / 2);
	if (period < 1) period = 1;
	if (fabs(sq_vol[ch]) != fabs(vol)) sq_vol[ch] = (int)vol;

	for(i=0;i < SNDBUFF_SIZE;++i,++sq_cnt[ch])
	{
		if (!(sq_cnt[ch] % period)) sq_vol[ch] = -sq_vol[ch];
		res = sndbuff[i] + sq_vol[ch];	
		CLIP_AND_SET_BUFFER();
	}
}




/*** Sawtooth waveform - sharp drop followed by gradual climb, then a sharp
     drop again etc etc ***/
void addSawtooth(int ch, double vol, double freq, int reset)
{
	double inc;
	double period;
	int res;
	int i;

	assert(ch < NUM_CHANS);

	if (freq < 1) freq = 1;
	if (reset) resetSoundBuffer();

	period = (double)(PCM_FREQ / freq) - 1;
	if (period < 1) period = 1;
	inc = (vol / period) * 2;

	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		res = sndbuff[i] + (short)saw_val[ch];
		CLIP_AND_SET_BUFFER();

		if (saw_val[ch] >= vol)
			saw_val[ch] = -vol;
		else
			saw_val[ch] += inc;
	}
}




/*** Create noise. 'gap' is the gap between new random values. Between these
     the code interpolates. The larger the gap the more the noise becomes pink 
     noise rather than white since the max frequency drops ***/
void addNoise(double vol, int gap, int reset)
{
	// Static otherwise we get a clicking sound on each call
	static double prev_res = 0;
	double res;
	double inc;
	double target;
	short svol;
	int i;
	int j;

	assert(gap <= SNDBUFF_SIZE);

	if (gap < 1) gap = 1;
	if (reset) resetSoundBuffer();

	svol = (short)vol;
	inc = 0;
	target = 0;
	for(i=0,j=0;i < SNDBUFF_SIZE;++i,j=(j+1) % gap)
	{
		res = prev_res + inc;
		prev_res = res;

		if (!j)
		{
			target = (random() % (svol * 2 + 1)) - svol;
			inc = (target - res) / gap;
		}
		res = sndbuff[i] + res;

		CLIP_AND_SET_BUFFER();
	}
}




/*** Distort by clipping ***/
void addDistortion(int clip)
{
	for(int i=0;i < SNDBUFF_SIZE;++i)
	{
		if (sndbuff[i] > clip) sndbuff[i] = clip;
		else
		if (sndbuff[i] < -clip) sndbuff[i] = -clip;
	}
}




/*** This low pass filter works by setting each point to the average of the 
     previous sample_size points ***/
void filter(short sample_size, int reset)
{
	static double avg[SNDBUFF_SIZE];
	static int a = 0;
	double tot;
	int i;
	int j;

	assert(sample_size < SNDBUFF_SIZE);
	if (sample_size < 1) sample_size = 1;

	if (reset)
	{
		bzero(avg,sizeof(avg));
		a = 0;
	}

	for(i=0;i < SNDBUFF_SIZE;++i,a = (a + 1) % sample_size)
	{
		avg[a] = sndbuff[i];
		for(j=0,tot=0;j < sample_size;++j) tot += avg[j];
		sndbuff[i] = (short)(tot / sample_size);
	}
}

#endif
