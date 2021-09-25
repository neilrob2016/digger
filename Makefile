#
# This only appears to work properly with gmake
#

# Use for OpenSound
#SOUND=-DSOUND

# Use for ALSA
#SOUND=-DSOUND -DALSA -lasound

CC=g++
COMP=$(CC) $(SOUND) -I/usr/X11/include -Wall -pedantic -g -O2 -c $<
BIN=digg

OBJS= \
	main.o \
	draw.o \
	common.o \
	tunnels.o \
	sound.o \
	cl_tunnel.o \
	cl_explosion.o \
	cl_text.o \
	cl_object.o \
	cl_player.o \
	cl_ball.o \
	cl_rock.o \
	cl_nugget.o \
	cl_boulder.o \
	cl_small_boulder.o \
	cl_molehill.o \
	cl_enemy.o \
	cl_spooky.o \
	cl_grubble.o \
	cl_spiky.o \
	cl_wurmal.o 

GM=globals.h Makefile

$(BIN): $(OBJS)
	$(CC) $(OBJS) $(SOUND) -L/usr/X11R6/lib -L/usr/X11R6/lib64 -lX11 -lm -lXext -o $(BIN)

build_date.h:
	echo "#define BUILD_DATE \"`date +'%Y-%m-%d %T'`\"" > build_date.h

main.o: main.cc $(GM) build_date.h
	$(COMP)

draw.o: draw.cc $(GM) build_date.h
	$(COMP)

common.o: common.cc $(GM)
	$(COMP)

tunnels.o: tunnels.cc $(GM)
	$(COMP)

sound.o: sound.cc $(GM)
	$(COMP)

cl_explosion.o: cl_explosion.cc $(GM)
	$(COMP)

cl_text.o: cl_text.cc $(GM)
	$(COMP)

cl_tunnel.o: cl_tunnel.cc $(GM)
	$(COMP)

cl_object.o: cl_object.cc $(GM)
	$(COMP)

cl_player.o: cl_player.cc $(GM)
	$(COMP)

cl_ball.o: cl_ball.cc $(GM)
	$(COMP)

cl_rock.o: cl_rock.cc $(GM)
	$(COMP)

cl_nugget.o: cl_nugget.cc $(GM)
	$(COMP)

cl_boulder.o: cl_boulder.cc $(GM)
	$(COMP)

cl_small_boulder.o: cl_small_boulder.cc $(GM)
	$(COMP)

cl_molehill.o: cl_molehill.cc $(GM)
	$(COMP)

cl_enemy.o: cl_enemy.cc $(GM)
	$(COMP)

cl_spooky.o: cl_spooky.cc $(GM)
	$(COMP)

cl_grubble.o: cl_grubble.cc $(GM)
	$(COMP)

cl_spiky.o: cl_spiky.cc $(GM)
	$(COMP)

cl_wurmal.o: cl_wurmal.cc $(GM)
	$(COMP)

clean:
	rm -f $(BIN) *.o build_date.h core
