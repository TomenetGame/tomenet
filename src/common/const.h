/* $Id$
 *
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-95 by
 *
 *      Bjørn Stabell        (bjoerns@staff.cs.uit.no)
 *      Ken Ronny Schouten   (kenrsc@stud.cs.uit.no)
 *      Bert Gÿsbers         (bert@mc.bio.uva.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef CONST_H
#define	CONST_H

#include <limits.h>
#include <math.h>

/*
 * FLT_MAX and RAND_MAX is ANSI C standard, but some systems (BSD) use
 * MAXFLOAT and INT_MAX instead.
 */
#ifndef	FLT_MAX
#   if defined(__sgi) || defined(__FreeBSD__)
#       include <float.h>	/* FLT_MAX for SGI Personal Iris or FreeBSD */
#   else
#	if defined(__sun__)
#           include <values.h>	/* MAXFLOAT for suns */
#	endif
#	ifdef VMS
#	    include <float.h>
#	endif
#   endif
#   if !defined(FLT_MAX)
#	if defined(MAXFLOAT)
#	    define FLT_MAX	MAXFLOAT
#	else
#	    define FLT_MAX	1e30f	/* should suffice :-) */
#	endif
#   endif
#endif
#ifndef	RAND_MAX
    /*
     * Ough!  If this is possible then we shouldn't be using RAND_MAX!
     * Older systems which don't have RAND_MAX likely should have it
     * to be defined as 32767, not as INT_MAX!
     * We better get our own pseudo-random library to overcome this mess
     * and get a uniform solution for everything.
     */
#   define  RAND_MAX	INT_MAX
#endif

/* Not everyone has PI (or M_PI defined). */
#ifndef	M_PI
#   define PI		3.14159265358979323846
#else
#   define PI		M_PI
#endif

/* Not everyone has LINE_MAX either, *sigh* */
#ifdef VMS
#   undef LINE_MAX
#endif
#ifndef LINE_MAX
#   define LINE_MAX 2048
#endif

#define RES		128

#define BLOCK_SZ	35

#define TABLE_SIZE	RES

#if 0
  /* The way it was: one table, and always range checking. */
# define tsin(x)	(tbl_sin[MOD2(x, TABLE_SIZE)])
# define tcos(x)	(tbl_sin[MOD2((x)+TABLE_SIZE/4, TABLE_SIZE)])
#else
# if 0
   /* Range checking: find out where the table size is exceeded. */
#  define CHK2(x, m)	((MOD2(x, m) != x) ? (printf("MOD %s:%d:%s\n", __FILE__, __LINE__, #x), MOD2(x, m)) : (x))
# else
   /* No range checking. */
#  define CHK2(x, m)	(x)
# endif
  /* New table lookup with optional range checking and no extra calculations. */
# define tsin(x)	(tbl_sin[CHK2(x, TABLE_SIZE)])
# define tcos(x)	(tbl_cos[CHK2(x, TABLE_SIZE)])
#endif

#define NELEM(a)	((int)(sizeof(a) / sizeof((a)[0])))

#undef ABS
#define ABS(x)			( (x)<0 ? -(x) : (x) )
#ifndef MAX
#   define MIN(x, y)		( (x)>(y) ? (y) : (x) )
#   define MAX(x, y)		( (x)>(y) ? (x) : (y) )
#endif
#define sqr(x)			( (x)*(x) )
#define LENGTH(x, y)		( hypot( (double) (x), (double) (y) ) )
#define VECTOR_LENGTH(v)	( hypot( (double) (v).x, (double) (v).y ) )
#define LIMIT(val, lo, hi)	( val=(val)>(hi)?(hi):((val)<(lo)?(lo):(val)) )

/*
 * Two acros for edge wrap of x and y coordinates measured in pixels.
 * Note that the correction needed shouldn't ever be bigger than one mapsize.
 */
#define WRAP_XPIXEL(x_)	\
	(BIT(World.rules->mode, WRAP_PLAY) \
	    ? ((x_) < 0 \
		? (x_) + World.width \
		: ((x_) >= World.width \
		    ? (x_) - World.width \
		    : (x_))) \
	    : (x_))

#define WRAP_YPIXEL(y_)	\
	(BIT(World.rules->mode, WRAP_PLAY) \
	    ? ((y_) < 0 \
		? (y_) + World.height \
		: ((y_) >= World.height \
		    ? (y_) - World.height \
		    : (y_))) \
	    : (y_))

/*
 * Two acros for edge wrap of x and y coordinates measured in map blocks.
 * Note that the correction needed shouldn't ever be bigger than one mapsize.
 */
#define WRAP_XBLOCK(x_)	\
	(BIT(World.rules->mode, WRAP_PLAY) \
	    ? ((x_) < 0 \
		? (x_) + World.x \
		: ((x_) >= World.x \
		    ? (x_) - World.x \
		    : (x_))) \
	    : (x_))

#define WRAP_YBLOCK(y_)	\
	(BIT(World.rules->mode, WRAP_PLAY) \
	    ? ((y_) < 0 \
		? (y_) + World.y \
		: ((y_) >= World.y \
		    ? (y_) - World.y \
		    : (y_))) \
	    : (y_))

/*
 * Two macros for edge wrap of differences in position.
 * If the absolute value of a difference is bigger than
 * half the map size then it is wrapped.
 */
#define WRAP_DX(dx)	\
	(BIT(World.rules->mode, WRAP_PLAY) \
	    ? ((dx) < - (World.width >> 1) \
		? (dx) + World.width \
		: ((dx) > (World.width >> 1) \
		    ? (dx) - World.width \
		    : (dx))) \
	    : (dx))

#define WRAP_DY(dy)	\
	(BIT(World.rules->mode, WRAP_PLAY) \
	    ? ((dy) < - (World.height >> 1) \
		? (dy) + World.height \
		: ((dy) > (World.height >> 1) \
		    ? (dy) - World.height \
		    : (dy))) \
	    : (dy))

#ifndef MOD2
#  define MOD2(x, m)		( (x) & ((m) - 1) )
#endif	/* MOD2 */

/* Do NOT change these! */
#define MAX_CHECKS		26
#define MAX_TEAMS		10

#define PSEUDO_TEAM(i,j)\
	(Players[(i)]->pseudo_team == Players[(j)]->pseudo_team)

/*
 * Used where we wish to know if a player is simply on the same team.
 */
#define TEAM(i, j)							\
	(BIT(Players[i]->status|Players[j]->status, PAUSE)		\
	|| (BIT(World.rules->mode, TEAM_PLAY)				\
	   && (Players[i]->team == Players[j]->team)			\
	   && (Players[i]->team != TEAM_NOT_SET)))

/*
 * Used where we wish to know if a player is on the same team
 * and has immunity to shots, thrust sparks, lasers, ecms, etc.
 */
#define TEAM_IMMUNE(i, j)	(teamImmunity && TEAM(i, j))

#define CANNON_DEAD_TIME	900

#define	RECOVERY_DELAY		(FPS*3)

#define EXPIRED_MINE_ID		4096   /* assume no player has this id */
#define MAX_PSEUDO_PLAYERS      16

#define MAX_MSGS		8
#define MAX_CHARS		80
#define MSG_LEN			256

#define FONT_LEN		256

#define NUM_MODBANKS		4

#define MAX_TOTAL_SHOTS		16384

#define SPEED_LIMIT		65.0
#define MAX_PLAYER_TURNSPEED	64.0
#define MIN_PLAYER_TURNSPEED	4.0
#define MAX_PLAYER_POWER	55.0
#define MIN_PLAYER_POWER	5.0
#define MAX_PLAYER_TURNRESISTANCE	1.0
#define MIN_PLAYER_TURNRESISTANCE	0.0

#define FUEL_SCALE_BITS         8
#define FUEL_SCALE_FACT         (1<<FUEL_SCALE_BITS)
#define FUEL_MASS(f)            ((f)*0.005/FUEL_SCALE_FACT)
#define MAX_STATION_FUEL	(500<<FUEL_SCALE_BITS)
#define START_STATION_FUEL	(20<<FUEL_SCALE_BITS)
#define STATION_REGENERATION	(0.06*FUEL_SCALE_FACT)
#define MAX_PLAYER_FUEL		(2600<<FUEL_SCALE_BITS)
#define MIN_PLAYER_FUEL		(350<<FUEL_SCALE_BITS)
#define REFUEL_RATE		(5<<FUEL_SCALE_BITS)
#define ENERGY_PACK_FUEL        ((500+(rand()&511))<<FUEL_SCALE_BITS)
#define DEFAULT_PLAYER_FUEL	(1000<<FUEL_SCALE_BITS)
#define FUEL_NOTIFY             (16*FPS)

#define TARGET_DEAD_TIME	(FPS * 60)
#define TARGET_DAMAGE		(250<<FUEL_SCALE_BITS)
#define TARGET_FUEL_REPAIR_PER_FRAME (TARGET_DAMAGE / (FPS * 10))
#define TARGET_REPAIR_PER_FRAME	(TARGET_DAMAGE / (FPS * 600))
#define TARGET_UPDATE_DELAY	(TARGET_DAMAGE / (TARGET_REPAIR_PER_FRAME \
				    * BLOCK_SZ))

#define LG2_MAX_AFTERBURNER    4
#define ALT_SPARK_MASS_FACT     4.2
#define ALT_FUEL_FACT           3
#define MAX_AFTERBURNER        ((1<<LG2_MAX_AFTERBURNER)-1)
#define AFTER_BURN_SPARKS(s,n)  (((s)*(n))>>LG2_MAX_AFTERBURNER)
#define AFTER_BURN_POWER_FACTOR(n) \
 (1.0+(n)*((ALT_SPARK_MASS_FACT-1.0)/(MAX_AFTERBURNER+1.0)))
#define AFTER_BURN_POWER(p,n)   \
 ((p)*AFTER_BURN_POWER_FACTOR(n))
#define AFTER_BURN_FUEL(f,n)    \
 (((f)*((MAX_AFTERBURNER+1)+(n)*(ALT_FUEL_FACT-1)))/(MAX_AFTERBURNER+1.0))

#ifdef	TURN_THRUST
#  define TURN_FUEL(acc)          (0.005*FUEL_SCALE_FACT*ABS(acc))
#  define TURN_SPARKS(tf)         (5+((tf)>>((FUEL_SCALE_BITS)-6)))
#endif

#define THRUST_MASS             0.7

#define MAX_TANKS               8
#define TANK_MASS               (ShipMass/10)
#define TANK_CAP(n)             (!(n)?MAX_PLAYER_FUEL:(MAX_PLAYER_FUEL/3))
#define TANK_FUEL(n)            ((TANK_CAP(n)*(5+(rand()&3)))/32)
#define TANK_REFILL_LIMIT       (MIN_PLAYER_FUEL/8)
#define TANK_THRUST_FACT        0.7
#define TANK_NOTHRUST_TIME      (HEAT_CLOSE_TIMEOUT/2+2)
#define TANK_THRUST_TIME        (TANK_NOTHRUST_TIME/2+1)

#define GRAVS_POWER		2.7

#define SHIP_SZ		        14  /* Size (pixels) of radius for legal HIT! */
#define VISIBILITY_DISTANCE	1000.0
#define WARNING_DISTANCE	(VISIBILITY_DISTANCE*0.8)

#define ECM_DISTANCE		(VISIBILITY_DISTANCE*0.4)

#define TRANSPORTER_DISTANCE	(VISIBILITY_DISTANCE*0.2)

#define SHOT_MULT(o) \
	((BIT((o)->mods.nuclear, NUCLEAR) && BIT((o)->mods.warhead, CLUSTER)) \
	 ? nukeClusterDamage : 1.0f)

#define MINE_RADIUS		8
#define MINE_RANGE              (VISIBILITY_DISTANCE*0.1)
#define MINE_SENSE_BASE_RANGE   (MINE_RANGE*1.3)
#define MINE_SENSE_RANGE_FACTOR (MINE_RANGE*0.3)
#define MINE_MASS               30.0
#define MINE_LIFETIME           (5000+(rand()&255))
#define MINE_SPEED_FACT         1.3

#define MISSILE_LIFETIME        (rand()%(64*FPS-1)+(128*FPS))
#define MISSILE_MASS            5.0
#define MISSILE_RANGE           4
#define SMART_SHOT_ACC		0.6
#define SMART_SHOT_DECFACT	3
#define SMART_SHOT_MIN_SPEED	(SMART_SHOT_ACC*8)
#define SMART_TURNSPEED         2.6
#define SMART_SHOT_MAX_SPEED	22.0
#define SMART_SHOT_LOOK_AH      4
#define TORPEDO_SPEED_TIME      (2*FPS)
#define TORPEDO_ACC 	(18.0*SMART_SHOT_MAX_SPEED/(FPS*TORPEDO_SPEED_TIME))
#define TORPEDO_RANGE           (MINE_RANGE*0.45)

#define NUKE_SPEED_TIME		(2*FPS)
#define NUKE_ACC 		(5*TORPEDO_ACC)
#define NUKE_RANGE		(MINE_RANGE*1.5)
#define NUKE_MASS_MULT		1
#define NUKE_MINE_EXPL_MULT	3
#define NUKE_SMART_EXPL_MULT	4

#define HEAT_RANGE              (VISIBILITY_DISTANCE/2)
#define HEAT_SPEED_FACT         1.7
#define HEAT_CLOSE_TIMEOUT      (2*FPS)
#define HEAT_CLOSE_RANGE        HEAT_RANGE
#define HEAT_CLOSE_ERROR        0
#define HEAT_MID_TIMEOUT        (4*FPS)
#define HEAT_MID_RANGE          (2*HEAT_RANGE)
#define HEAT_MID_ERROR          8
#define HEAT_WIDE_TIMEOUT       (8*FPS)
#define HEAT_WIDE_ERROR         16

#define CLUSTER_MASS_SHOTS(mass) ((mass) * 0.9 / ShotsMass)
#define CLUSTER_MASS_DRAIN(mass) (CLUSTER_MASS_SHOTS(mass)*ED_SHOT)

#define BALL_RADIUS		10

#define MISSILE_LEN		15
#define SMART_SHOT_LEN		12
#define HEAT_SHOT_LEN		15
#define TORPEDO_LEN		18

#define MAX_LASERS		5
#define PULSE_SPEED		90
#define PULSE_SAMPLE_DISTANCE	5
#define PULSE_LENGTH		(PULSE_SPEED - PULSE_SAMPLE_DISTANCE)
#define PULSE_MAX_LIFE		6
#define PULSE_LIFE(lasers)	(PULSE_MAX_LIFE - (MAX_LASERS+1 - (lasers)) / 4)

#define MAX_TRACTORS		4

#define TRACTOR_MAX_RANGE(pl)  (200 + (pl)->item[ITEM_TRACTOR_BEAM] * 50)
#define TRACTOR_MAX_FORCE(pl)  (-40 + (pl)->item[ITEM_TRACTOR_BEAM] * -20)
#define TRACTOR_PERCENT(pl, maxdist) \
	(1.0-(0.5*(pl)->lock.distance/(maxdist)))
#define TRACTOR_COST(percent) (-1.5 * FUEL_SCALE_FACT * (percent))
#define TRACTOR_FORCE(pl, percent, maxforce) \
	((percent) * (maxforce) * ((pl)->tractor_pressor ? -1 : 1))

#define WARN_TIME               2

#define BALL_STRING_LENGTH      120

#define TEAM_NOT_SET		0xffff
#define TEAM_NOT_SET_STR	"4095"

#define DEBRIS_MASS             4.5
#define DEBRIS_SPEED(intensity) ((rand()%(1+(intensity>>2)))|20)
#define DEBRIS_LIFE(intensity)  ((rand()%(1+(intensity>>1)))|8)
#define DEBRIS_TYPES		(8 * 4 * 4)

#define PL_DEBRIS_MASS          3.5
#define PL_DEBRIS_SPEED(mass)   DEBRIS_SPEED(((int)mass)<<1)
#define PL_DEBRIS_LIFE(mass)    (4+(rand()%(int)(1+mass*1.5)))

#define ENERGY_RANGE_FACTOR	(2.5/FUEL_SCALE_FACT)

#define WORM_BRAKE_FACTOR	1
#define WORMCOUNT		64

#ifdef __GNUC__
#define	INLINE	inline
#else
#define INLINE
#endif /* __GNUC__ */

#if defined(__sun__) && !defined(__svr4__)
#  define srand(s)	srandom(s)
#  define rand()	random()
#endif /* __sun__ */

#if defined(ultrix) || defined(AIX)
/* STDRUP_OBJ should be uncomented in Makefile also */
extern char* strdup(const char*);
#endif

#endif
