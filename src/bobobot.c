/*
  bobobot.c
  
  BoboBot
  
  An action-packed platform-style game starring a robot monkey.
  
  Based on Capcom's Megaman series, Nintendo's Super Mario series,
  and many other games of their genre.
  
  DESIGN:   Bill Kendrick, Melissa Hardenbrook, Marianne Waage
  CODE:     Bill Kendrick
  GFX:      Marianne Waage, Bill Kendrick, Melissa Hardenbrook
  PATCHES:  Dennis Payne

  (c) New Breed Software 1998 - 2000
  Please distribute freely, with no changes and all files intact.
  
  bobobot@newbreedsoftware.com
  http://www.newbreedsoftware.com/bobobot/         <- latest info/download
  
  (Huge chunks of code from X-Bomber, et. al.)
  
  July 8, 1998 - November 30, 2000
*/

#define VERSION "Preview 3"

/* #define DEBUG */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <sys/types.h>
#include <sys/time.h>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "window.h"
#include "connect.h"
#include "hints.h"
#include "visual.h"
#include "gc.h"
#include "color.h"
#include "randnum.h"
#include "text.h"

#ifdef __amigaos__
/* The data is in PROGDIR: (DATA, from amiga/Makefile.amigaos), which takes
   no "/" after it; the frame clock is acsdl12's (amiga/xsdl). */
#define DATA_SEP ""
#include "xsdl_time.h"
#else
#define DATA_SEP "/"
#endif

#ifdef SOUND_YES
#define SOUND
#endif

#ifdef JOYSTICK_YES
#define JOYSTICK
#endif

#ifdef MUSIC_YES
#define SOUND
#define MUSIC
#endif

#ifdef SOUND
#include <SDL.h>
#include <SDL_mixer.h>
#endif

#ifdef JOYSTICK
#include <sys/ioctl.h>
#include <linux/joystick.h>
#endif


/* Screen size: */

#define TILE_WIDTH  10
#define TILE_HEIGHT  8

#define WIDTH  (TILE_WIDTH * 32)
#define HEIGHT (TILE_HEIGHT * 32)


/* 3D Values: */

#define DISTANCE    100.0
#define ASPECT       80.0
#define THREED_SIZE   6.0


/* Color limits: */

#define MAX_COLORS   256
#define COLOR_DIVIDE  16


/* Keyboard mapping: */

#define KEYBD_LEFT      XK_Left
#define KEYBD_RIGHT     XK_Right
#define KEYBD_UP        XK_Up
#define KEYBD_DOWN      XK_Down
#define KEYBD_JUMP      XK_space
#define KEYBD_FIRE      XK_z

#define KEY_LEFT   0
#define KEY_RIGHT  1
#define KEY_UP     2
#define KEY_DOWN   3
#define KEY_JUMP   4
#define KEY_FIRE   5
#define NUM_KEYS   6

int holds[NUM_KEYS], old_holds[NUM_KEYS],
  key_holds[NUM_KEYS], stick_holds[NUM_KEYS];


/* Directions: */

#define DIR_NONE  -1
#define DIR_LEFT   0
#define DIR_RIGHT  1


/* Gravity affect values: */

#define GRAV_NONE   0
#define GRAV_NORMAL 1
#define GRAV_BOUNCE 2


/* Owner abstraction */

#define OWN_PLAYER 0
#define OWN_ENEMY  1


/* Enemy Modes: */

#define MODE_NORMAL 0
#define MODE_RUSH   1
#define MODE_SPARK  2
#define MODE_CARRY  3
#define MODE_DROP   4
#define MODE_STEAM  5
#define NUM_MODES   6


/* Constraints: */

#define CHANCE_OF_GOODIE      5 /* One out of .. killed enemies gives goodie */
#define CHANCE_OF_ONEUP      15 /* One out of .. goodies is a one-up */
#define CHANCE_OF_BLINK      50
#define CHANCE_OF_BUBBLE     50
#define CHANCE_OF_DRIP        5
#define CHANCE_OF_SNOWFLAKE  10
#define CHANCE_OF_UP          4
#define CHANCE_OF_RUSH       40
#define MAX_PAGES           100 /* Max # of loadable pages per level */
#define MAX_EXPLOSIONS       10 /* Max # of explosions that can be on screen */
#define MAX_AIR_BUBBLES      50 /* Max # of air bubbles on screen */
#define MAX_DRIPS             5 /* Max # of drips on screen */
#define MAX_DUSTS            10 /* Max # of dusts on screen */
#define MAX_ICE_CHUNKS       10 /* Max # of dusts on screen */
#define MAX_SPARKS           50 /* Max # of sparks on screen */
#define MAX_NOTE_LETTERS     80 /* Max # of note letters on screen */
#define MAX_SNOWFLAKES       30 /* Max # of snowflakes on screen */
#define MAX_UPS              10 /* Max # of upgrades on screen at once */
#define MAX_BULLETS          30 /* Max # of bullets that can be on screen */
#define MAX_XSPEED            8 /* Fastest running speed */
#define MAX_ENEMIES          20 /* Max # of enemies that can be on screen */
#define MAX_PLATFORMS         5 /* Max # of platforms that can be on screen */
#define MAX_WARP_INS          8 /* Max # of warp-in effect images */
#define MAX_SPLASHES          3 /* Max # of spashes that can be on screen */
#define MAX_STARS            20 /* Max # of stars that can be on screen */
#define MIN_BLINK_TIME      100 /* # frames of stillness before blinking */
#define MIN_BLINK_DURATION    5 /* How long to blink for... min... */
#define MAX_BLINK_DURATION   10 /* ...and max */
#define STARTING_JUMPSPEED    7 /* Starting jump speed */
#define EXP_TIME_EXTENDER     2 /* How many screen refreshes/explosion frame */


/* Speed controls: */

#define FRAMERATE 22500

/*
  dinpquvxyz
  DNOSZ
  34589;,
  !${}"?               " (fix emacs color)
*/



/* Object graphics abstractions: */

enum {
  OBJ_FORE_METAL,                /* # */
  OBJ_FORE_METAL_DAMAGED,        /* + */
  OBJ_FORE_BRICK,                /* B */
  OBJ_FORE_BRICK_DAMAGED,        /* 6 */
  OBJ_FORE_ICE,                  /* I */
  OBJ_FORE_SNOW_LEFT,            /* < */
  OBJ_FORE_SNOW_CENTER,          /* = */
  OBJ_FORE_SNOW_RIGHT,           /* > */
  OBJ_FORE_SAND,                 /* S */
  OBJ_FORE_FAN0,                 /* % */
  OBJ_FORE_FAN1,                 /* % */
  OBJ_FORE_FAN2,                 /* % */
  OBJ_FORE_CONVEYOR_LEFT_0,      /* L l */
  OBJ_FORE_CONVEYOR_LEFT_1,      /* L l */
  OBJ_FORE_CONVEYOR_LEFT_2,      /* L l */
  OBJ_FORE_CONVEYOR_CENTER_0,    /* C c */
  OBJ_FORE_CONVEYOR_CENTER_1,    /* C c */
  OBJ_FORE_CONVEYOR_CENTER_2,    /* C c */
  OBJ_FORE_CONVEYOR_RIGHT_0,     /* R r */
  OBJ_FORE_CONVEYOR_RIGHT_1,     /* R r */
  OBJ_FORE_CONVEYOR_RIGHT_2,     /* R r */
  OBJ_FORE_TRACK,                /* T */
  OBJ_FORE_TRACK_BROKEN_LEFT,    /* Y */
  OBJ_FORE_TRACK_BROKEN_RIGHT,   /* U */
  OBJ_FORE_ROCK,                 /* 7 */
  OBJ_FORE_CACTUS,               /* t */
  OBJ_FORE_MOON,                 /* M */
  OBJ_FORE_PIPE_0,               /* _ */
  OBJ_FORE_PIPE_1,               /* ~ */
  OBJ_FORE_PIPE_2,               /* : */
  OBJ_EXPLOSION0,
  OBJ_EXPLOSION1,
  OBJ_EXPLOSION2,
  OBJ_EXPLOSION3,
  OBJ_ZAP,
  OBJ_AIR_BUBBLE,
  OBJ_DRIP,
  OBJ_DUST_0,
  OBJ_DUST_1,
  OBJ_SNOWFLAKE,
  OBJ_SPLASH0,
  OBJ_SPLASH1,
  OBJ_ICE_CHUNK,
  OBJ_DONE,
  OBJ_TITLE_LEFT,
  OBJ_TITLE_RIGHT,
  OBJ_MAN_LEFT_STAND,
  OBJ_MAN_LEFT_STAND_BLINK,
  OBJ_MAN_LEFT_WALK1,
  OBJ_MAN_LEFT_WALK2,
  OBJ_MAN_LEFT_JUMP,
  OBJ_MAN_LEFT_HURT,
  OBJ_MAN_RIGHT_STAND,
  OBJ_MAN_RIGHT_STAND_BLINK,
  OBJ_MAN_RIGHT_WALK1,
  OBJ_MAN_RIGHT_WALK2,
  OBJ_MAN_RIGHT_JUMP,
  OBJ_MAN_RIGHT_HURT,
  OBJ_BACK_METAL,                /* . */
  OBJ_BACK_GREY,                 /* Q */
  OBJ_BACK_SKY,                  /* - */
  OBJ_BACK_BRICK,                /* b */
  OBJ_BACK_HIERO_0,              /* e */
  OBJ_BACK_HIERO_1,              /* f */
  OBJ_BACK_SCAFOLD,              /* X */
  OBJ_BACK_WAVE0,                /* W */
  OBJ_BACK_WAVE1,                /* W */
  OBJ_BACK_WAVE2,                /* W */
  OBJ_BACK_WATER,                /* w */
  OBJ_BACK_WATER_SUCK_0,         /* V */
  OBJ_BACK_WATER_SUCK_1,         /* V */
  OBJ_BACK_WATER_SUCK_2,         /* V */
  OBJ_BACK_TAN_LEFT,             /* [ */
  OBJ_BACK_TAN_LEFT_EDGE,        /* / */
  OBJ_BACK_TAN_RIGHT,            /* ] */
  OBJ_BACK_TAN_RIGHT_EDGE,       /* \ */
  OBJ_BACK_QUICKSAND0,           /* s */
  OBJ_BACK_QUICKSAND1,           /* s */
  OBJ_BACK_QUICKSAND2,           /* s */
  OBJ_BACK_PALM_TRUNK,           /* | */
  OBJ_BACK_PALM_TOP_LEFT,        /* ( */
  OBJ_BACK_PALM_TOP_CENTER,      /* * */
  OBJ_BACK_PALM_TOP_RIGHT,       /* ) */
  OBJ_BACK_CLOUD_LEFT,           /* ` */
  OBJ_BACK_CLOUD_RIGHT,          /* ' */
  OBJ_BACK_STARS0,               /* 0 */
  OBJ_BACK_STARS1,               /* 1 */
  OBJ_BACK_STARS2_0,             /* 2 */
  OBJ_BACK_STARS2_1,             /* 2 */
  OBJ_BACK_STARS2_2,             /* 2 */
  OBJ_BACK_PINE,                 /* P */
  OBJ_BACK_GEAR_TOPLEFT_0,       /* G J */
  OBJ_BACK_GEAR_TOPLEFT_1,       /* G J */
  OBJ_BACK_GEAR_TOPLEFT_2,       /* G J */
  OBJ_BACK_GEAR_TOPRIGHT_0,      /* H K */
  OBJ_BACK_GEAR_TOPRIGHT_1,      /* H K */
  OBJ_BACK_GEAR_TOPRIGHT_2,      /* H K */
  OBJ_BACK_GEAR_BOTLEFT_0,       /* g j */
  OBJ_BACK_GEAR_BOTLEFT_1,       /* g j */
  OBJ_BACK_GEAR_BOTLEFT_2,       /* g j */
  OBJ_BACK_GEAR_BOTRIGHT_0,      /* h k */
  OBJ_BACK_GEAR_BOTRIGHT_1,      /* h k */
  OBJ_BACK_GEAR_BOTRIGHT_2,      /* h k */
  OBJ_BACK_LAMP,                 /* ^ */
  OBJ_BACK_CLOCK_DIG_0,          /* ? */
  OBJ_BACK_CLOCK_DIG_1,          /* ? */
  OBJ_BACK_CLOCK_0,              /* @ */
  OBJ_BACK_CLOCK_1,              /* @ */
  OBJ_BACK_CLOCK_2,              /* @ */
  OBJ_BACK_ROCK_0,               /* & */
  OBJ_BACK_ROCK_1,               /* A */
  OBJ_BACK_CRATER,               /* m */
  OBJ_BACK_EARTH,                /* E */
  OBJ_BACK_FLAG,                 /* F */
  OBJ_BACK_ANTENNAE,             /* a */
  OBJ_BACK_SPACE_0,
  OBJ_BACK_SPACE_1,
  OBJ_BACK_SPACE_2,
  OBJ_BACK_SPACE_3,
  OBJ_BULLET_NORMAL,
  OBJ_BULLET_ICE,
  OBJ_BULLET_BITS,
  OBJ_BULLET_WRAPS_0,
  OBJ_BULLET_WRAPS_1,
  OBJ_BULLET_SPRING,
  OBJ_BULLET_ROCK,
  OBJ_BULLET_ROCK_BIG,
  OBJ_UP_BANANA_SMALL,
  OBJ_UP_BANANA_BIG,
  OBJ_UP_ONEUP,
  OBJ_UP_COIN,
  OBJ_UP_BOLT_SMALL,
  OBJ_UP_BOLT_BIG,
  OBJ_BAD_PENGUIN_LEFT_0,
  OBJ_BAD_PENGUIN_LEFT_1,
  OBJ_BAD_PENGUIN_RIGHT_0,
  OBJ_BAD_PENGUIN_RIGHT_1,
  OBJ_BAD_ROBOFLY_0,
  OBJ_BAD_ROBOFLY_1,
  OBJ_BAD_ICICLE,
  OBJ_BAD_SPIDER_0,
  OBJ_BAD_SPIDER_1,
  OBJ_BAD_FISH_LEFT_0,
  OBJ_BAD_FISH_LEFT_1,
  OBJ_BAD_FISH_RIGHT_0,
  OBJ_BAD_FISH_RIGHT_1,
  OBJ_BAD_BOMB_WALK_0,
  OBJ_BAD_BOMB_WALK_1,
  OBJ_BAD_BOMB_SPARK_0,
  OBJ_BAD_BOMB_SPARK_1,
  OBJ_BAD_GEAR_0,                /* o */
  OBJ_BAD_GEAR_1,
  OBJ_BAD_HELI_0,
  OBJ_BAD_HELI_1,
  OBJ_BAD_MINECART,
  OBJ_BAD_MINECART_LEFT,
  OBJ_BAD_MINECART_RIGHT,
  OBJ_BAD_BIRD_LEFT_0,
  OBJ_BAD_BIRD_LEFT_1,
  OBJ_BAD_BIRD_RIGHT_0,
  OBJ_BAD_BIRD_RIGHT_1,
  OBJ_BAD_SNAKE_HEAD,
  OBJ_BAD_SNAKE_BODY,
  OBJ_BAD_ARMADILLO_LEFT,
  OBJ_BAD_ARMADILLO_RIGHT,
  OBJ_BAD_ARMADILLO_ROLL_0,
  OBJ_BAD_ARMADILLO_ROLL_1,
  OBJ_BAD_ARMADILLO_ROLL_2,
  OBJ_BAD_ROCKET_LEFT,
  OBJ_BAD_ROCKET_RIGHT,
  OBJ_BAD_TUMBLEWEED_0,
  OBJ_BAD_TUMBLEWEED_1,
  OBJ_BAD_TUMBLEWEED_2,
  OBJ_BAD_STEAM_START_0,
  OBJ_BAD_STEAM_START_1,
  OBJ_BAD_STEAM_SPRAY_0,
  OBJ_BAD_STEAM_SPRAY_1,
  OBJ_BOSS1_LEFT_0,
  OBJ_BOSS1_LEFT_1,
  OBJ_BOSS1_RIGHT_0,
  OBJ_BOSS1_RIGHT_1,
  OBJ_BOSS2_LEFT_0,
  OBJ_BOSS2_LEFT_1,
  OBJ_BOSS2_LEFT_2,
  OBJ_BOSS2_RIGHT_0,
  OBJ_BOSS2_RIGHT_1,
  OBJ_BOSS2_RIGHT_2,
  OBJ_BOSS2_CASKET,
  OBJ_BOSS3_0,
  OBJ_BOSS3_1,
  OBJ_BOSS3_2,
  OBJ_BOSS4_0,
  OBJ_BOSS4_1,
  OBJ_EVIL_LEFT_STAND,
  OBJ_EVIL_LEFT_JUMP,
  OBJ_EVIL_RIGHT_STAND,
  OBJ_EVIL_RIGHT_JUMP,
  OBJ_COVER_ICE,
  OBJ_PLATFORM,
  OBJ_HAT_DESERT_LEFT,
  OBJ_HAT_DESERT_RIGHT,
  OBJ_HAT_HARDHAT_LEFT,
  OBJ_HAT_HARDHAT_RIGHT,
  OBJ_HAT_MINER_LEFT,
  OBJ_HAT_MINER_RIGHT,
  OBJ_HAT_SCUBA_LEFT,
  OBJ_HAT_SCUBA_RIGHT,
  OBJ_HAT_PLUMBER_LEFT,
  OBJ_HAT_PLUMBER_RIGHT,
  NUM_OBJECTS
};

#define MAX_EXPLOSION_TIME 3


/* 3D Shapes to pick from: */

#define THREED_BOBO_0   0
#define THREED_BOBO_1   1
#define THREED_BOBO_2   2
#define THREED_BANANA   3
#define THREED_BOLT     4
#define THREED_EVIL     5
#define NUM_THREEDS     6

int threed_icons[NUM_THREEDS] = {
  OBJ_MAN_LEFT_STAND,
  OBJ_MAN_LEFT_JUMP,
  OBJ_MAN_LEFT_HURT,
  OBJ_UP_BANANA_BIG,
  OBJ_UP_BOLT_BIG,
  OBJ_EVIL_RIGHT_JUMP
};

#define NUM_SPIN_MODES 5


/* Object graphics filenames: */

char * object_names[NUM_OBJECTS] = {
  /* Foreground: */
  /* (Metal) */
  "fore-metal", "fore-metal-damaged",
  /* (Bricks) */
  "fore-brick", "fore-brick-damaged",
  /* (Ice/Snow) */
  "fore-ice", "fore-snow-left", "fore-snow-center", "fore-snow-right",
  /* (Sand) */
  "fore-sand",
  /* (Fan) */
  "fore-fan0", "fore-fan1", "fore-fan2",
  /* (Conveyor belts) */
  "fore-conveyor-left-0", "fore-conveyor-left-1", "fore-conveyor-left-2", 
  "fore-conveyor-center-0", "fore-conveyor-center-1", "fore-conveyor-center-2",
  "fore-conveyor-right-0", "fore-conveyor-right-1", "fore-conveyor-right-2",
  /* (Tracks) */
  "fore-track", "fore-track-broken-left", "fore-track-broken-right",
  "fore-rock",
  /* (Cactus) */
  "fore-cactus",
  /* (Moon) */
  "fore-moon",
  /* (Pipe) */
  "fore-pipe-0", "fore-pipe-1", "fore-pipe-2",
  /* Explosion: */
  "explosion0", "explosion1", "explosion2", "explosion3",
  /* Misc: */
  "zap", "air-bubble", "drip",
  "dust-0", "dust-1",
  "snowflake",
  "splash0", "splash1",
  "ice-chunk",
  "done",
  "title-left", "title-right",
  /* Player: */
  "bobo-left-stand", "bobo-left-stand-blink",
  "bobo-left-walk1", "bobo-left-walk2",
  "bobo-left-jump", "bobo-left-hurt",
  "bobo-right-stand", "bobo-right-stand-blink",
  "bobo-right-walk1", "bobo-right-walk2",
  "bobo-right-jump", "bobo-right-hurt",
  /* Backgrounds: */
  "back-metal", "back-grey", "back-sky",
  "back-brick", "back-hiero-0", "back-hiero-1",
  "back-scafold",
  /* (Water) */
  "back-wave0", "back-wave1", "back-wave2", 
  "back-water",
  "back-water-suck-0", "back-water-suck-1", "back-water-suck-2",
  /* (Pyramids) */
  "back-tan-left", "back-tan-left-edge",
  "back-tan-right", "back-tan-right-edge",
  /* (Quicksand) */
  "back-quicksand0", "back-quicksand1", "back-quicksand2",
  /* (Palm tree) */
  "back-palm-trunk",
  "back-palm-top-left", "back-palm-top-center", "back-palm-top-right",
  /* (Cloud) */
  "back-cloud-left", "back-cloud-right",
  /* (Stars) */
  "back-stars0", "back-stars1",
  "back-stars2-0", "back-stars2-1", "back-stars2-2",
  /* (Pine Trees) */
  "back-pine",
  /* (Gear) */
  "back-gear-topleft-0", "back-gear-topleft-1", "back-gear-topleft-2",
  "back-gear-topright-0", "back-gear-topright-1", "back-gear-topright-2",
  "back-gear-botleft-0", "back-gear-botleft-1", "back-gear-botleft-2",
  "back-gear-botright-0", "back-gear-botright-1", "back-gear-botright-2",
  /* (Lamp) */
  "back-lamp",
  /* (Clocks) */
  "back-clock-dig-0", "back-clock-dig-1",
  "back-clock-0", "back-clock-1", "back-clock-2",
  /* (Rock and moon) */
  "back-rock-0", "back-rock-1",
  "back-crater", "back-earth", "back-flag", "back-antennae",
  /* (Space) */
  "back-space-0", "back-space-1", "back-space-2", "back-space-3", 
  /* Weapons: */
  "bullet-normal", "bullet-ice", "bullet-bits",
  "bullet-wraps-0", "bullet-wraps-1",
  "bullet-spring", "bullet-rock", "bullet-rock-big",
  /* Upgrades: */
  "up-banana-small", "up-banana-big",
  "up-oneup", "up-coin",
  "up-bolt-small", "up-bolt-big",
  /* Enemies: */
  "bad-penguin-left-0", "bad-penguin-left-1",
  "bad-penguin-right-0", "bad-penguin-right-1",
  "bad-robofly-0", "bad-robofly-1",
  "bad-icicle",
  "bad-spider-0", "bad-spider-1",
  "bad-fish-left-0", "bad-fish-left-1",
  "bad-fish-right-0", "bad-fish-right-1",
  "bad-bomb-walk-0", "bad-bomb-walk-1",
  "bad-bomb-spark-0", "bad-bomb-spark-1",
  "bad-gear-0", "bad-gear-1",
  "bad-heli-0", "bad-heli-1",
  "bad-minecart", "bad-minecart-left", "bad-minecart-right",
  "bad-bird-left-0", "bad-bird-left-1", "bad-bird-right-0", "bad-bird-right-1",
  "bad-snake-head", "bad-snake-body",
  "bad-armadillo-left", "bad-armadillo-right",
  "bad-armadillo-roll-0", "bad-armadillo-roll-1", "bad-armadillo-roll-2",
  "bad-rocket-left", "bad-rocket-right",
  "bad-tumbleweed-0", "bad-tumbleweed-1", "bad-tumbleweed-2",
  "bad-steam-start-0", "bad-steam-start-1",
  "bad-steam-spray-0", "bad-steam-spray-1",
  /* Bosses: */
  "boss1-left-0", "boss1-left-1", "boss1-right-0", "boss1-right-1",
  "boss2-left-0", "boss2-left-1", "boss2-left-2",
  "boss2-right-0", "boss2-right-1", "boss2-right-2",
  "boss2-casket",
  "boss3-0", "boss3-1", "boss3-2",
  "boss4-0", "boss4-1",
  /* Evil: */
  "evil-left-stand", "evil-left-jump",
  "evil-right-stand", "evil-right-jump",
  /* Covers: */
  "cover-ice",
  "platform",
  /* Hats: */
  "hat-desert-left", "hat-desert-right",
  "hat-hardhat-left", "hat-hardhat-right",
  "hat-miner-left", "hat-miner-right",
  "hat-scuba-left", "hat-scuba-right",
  "hat-plumber-left", "hat-plumber-right"
};


/* Usable weapon abstractions: */

#define WPN_NORMAL  0
#define WPN_ICE     1
#define WPN_WRAPS   2
#define WPN_SPRING  3
#define WPN_ROCK    4
#define WPN_5       5
#define WPN_6       6
#define WPN_7       7
#define WPN_8       8
#define NUM_WEAPONS 9


/* Weapon names: */

char * weapon_names[NUM_WEAPONS] = {
  "Normal",
  "Ice Blaster", "Mummy Wraps", "Spring", "Rock-Gun",
  "(Unavail.)", "(Unavail.)", "(Unavail.)", "(Unavail.)"
};


/* Weapon images: */

int weapon_images[NUM_WEAPONS] = {
  OBJ_BULLET_NORMAL,
  OBJ_BULLET_ICE,
  OBJ_BULLET_WRAPS_0,
  OBJ_BULLET_SPRING,
  OBJ_BULLET_ROCK,
  -1,
  -1,
  -1,
  -1
};

	  
/* Weapon usage values: */

int weapon_usage[NUM_WEAPONS] = {
  0, 5, 2, 1, 2, 0, 0, 0, 0
};


/* Other weapons: */

#define WPN_BITS    9


/* Level name abstracts: */

char * level_names[8] = {
  "FREEZE-MAN: Antarctic Fortress",
  "THE MUMMY: Egyptian Pyramids",
  "CHRONOMAN: Swiss Clock Factory",
  "ROCKMAN: The Grand Canyon",
  "(Unavailable)",
  "(Unavailable)",
  "(Unavailable)",
  "(Unavailable)"
};


/* Up abstractions: */

#define UP_NONE       -99
#define UP_RANDOM      -1
#define UP_BANANA_SMALL 0
#define UP_BANANA_BIG   1
#define UP_BOLT_SMALL   2
#define UP_BOLT_BIG     3
#define UP_ONEUP        4
#define NUM_UPS         5

#define UP_COIN         5


/* Enemy abstractions: */

#define BAD_PENGUIN     0
#define BAD_ROBOFLY     1
#define BAD_ICICLE      2
#define BAD_SPIDER      3
#define BAD_FISH        4
#define BAD_BOMB        5
#define BAD_GEAR        6
#define BAD_HELI        7
#define BAD_MINECART    8
#define BAD_BIRD        9
#define BAD_SNAKE      10
#define BAD_ROCKER     11
#define BAD_ARMADILLO  12
#define BAD_ROCKET     13
#define BAD_TUMBLEWEED 14
#define BAD_STEAMER    15

#define FIRST_BOSS     16
#define BOSS1          16  /* -1 */
#define BOSS2          17  /* -2 */
#define BOSS3          18  /* -3 */
#define BOSS4          19  /* -4 */
#define BOSS5          20  /* -1 */
#define BOSS6          21  /* -2 */
#define BOSS7          22  /* -3 */
#define BOSS8          23  /* -4 */

#define NUM_BADS       24


/* Enemies start with this much health: */

int enemy_weapon_effects[NUM_BADS][NUM_WEAPONS] = {
  /* N  Frz  Mum  Spg  Rok   ?    ?    ?    ? */
  { 50,   0, 100,  50, 100, 100, 100, 100, 100}, /* Penguin */
  {100,   0, 100, 100, 100, 100, 100, 100, 100}, /* Robofly */
  {100,   0, 100, 100, 100, 100, 100, 100, 100}, /* Icicle */
  {100,   0,   0, 100,  50, 100, 100, 100, 100}, /* Spider */
  { 50,   0,   0, 100,   0, 100, 100, 100, 100}, /* Fish */
  { 34,   0, 100,   0, 100, 100, 100, 100, 100}, /* Bomb */
  {100,   0, 100,   0, 100, 100, 100, 100, 100}, /* Gear */
  {100,   0, 100,  50, 100, 100, 100, 100, 100}, /* Heli */
  { 50,   0, 100,   0,  34, 100, 100, 100, 100}, /* Minecart */
  {100,   0, 100, 100, 100, 100, 100, 100, 100}, /* Bird */
  { 50,   0,   0, 100, 100, 100, 100, 100, 100}, /* Snake */
  {  0,   0,   0,   0,   0,   0,   0,   0,   0}, /* Rocker */
  {  0,   0,  34,  50, 100, 100, 100, 100, 100}, /* Armadillo */
  { 34,   0,  34,  25,   0,   0,   0,   0,   0}, /* Rocket */
  {100,   0, 100, 100, 100, 100, 100, 100, 100}, /* Tumbleweed */
  {  0,   0,   0,   0,   0,   0,   0,   0,   0}, /* Steamer */
  {  3,   0,   3,   5,   2, 100, 100, 100, 100}, /* BOSS1 (Freeze) */
  {  3,   0,   0,  20,   4, 100, 100, 100, 100}, /* BOSS2 (Mummy) */
  {  5,   0,   6,   0,  30, 100, 100, 100, 100}, /* BOSS3 (Chrono) */
  {  3,   0,   8,  10,   0, 100, 100, 100, 100}, /* BOSS4 (Rock) */
  {100,   0, 100, 100, 100,   0, 100, 100, 100}, /* BOSS5 (?) */
  {100,   0, 100, 100, 100, 100,   0, 100, 100}, /* BOSS6 (?) */
  {100,   0, 100, 100, 100, 100, 100,   0, 100}, /* BOSS7 (?) */
  {100,   0, 100, 100, 100, 100, 100, 100,   0}  /* BOSS8 (?) */
};

/* You get hurt by this much by each enemy: */

#define MODE_NORMAL 0
#define MODE_RUSH   1
#define MODE_SPARK  2
#define MODE_CARRY  3
#define MODE_DROP   4
#define MODE_STEAM  5

int hurt_values[NUM_BADS][NUM_MODES] = {
  /* (Standard enemies) */
  
  {5, 5, 0, 0, 0, 0}, /* Penguin */
  {3, 3, 0, 0, 0, 0}, /* RoboFly */
  {2, 2, 0, 0, 0, 0}, /* Icicle */
  {1, 1, 0, 0, 0, 0}, /* Spider */
  {3, 5, 0, 0, 0, 0}, /* Fish */
  {1, 1, 1, 0, 0, 0}, /* Bomb */
  {2, 2, 0, 0, 0, 0}, /* Gear */
  {1, 1, 1, 1, 1, 1}, /* Heli */
  {5, 5, 5, 5, 5, 5}, /* Minecart */
  {2, 2, 2, 2, 2, 2}, /* Bird */
  {0, 0, 0, 0, 0, 0}, /* Snake */
  {0, 0, 0, 0, 0, 0}, /* Rocker */
  {3, 5, 0, 0, 0, 0}, /* Armadillo */
  {9, 0, 0, 0, 0, 0}, /* Rocket */
  {1, 1, 0, 0, 0, 0}, /* Tumbleweed */
  {0, 0, 0, 0, 0, 5}, /* Sprayer */
  /* (Bosses) */
  {5, 5, 5, 5, 5, 5}, /* Freeze-man */
  {4, 4, 4, 4, 4, 4}, /* The Mummy */
  {4, 4, 4, 4, 4, 4}, /* Chrono-Man */
  {7, 7, 7, 7, 7, 7}, /* Rock-Man */
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0}
};


/* Special bads (get replaced) */

#define BAD_ONEUP             100
#define BAD_BANANA            101
#define BAD_BOLT              102
#define BAD_PLATFORM_UP_DOWN  103
#define BAD_PLATFORM_UP       104
#define BAD_PLATFORM_DOWN     105
#define BAD_PLATFORM_SIDE     106
#define BAD_PLATFORM_DROP     107
#define BAD_NONE              999


/* Sound abstractions: */

enum {
  SND_EXPLODE,
  SND_HURT,
  SND_DIE,
  SND_LAND,
  SND_BUMP,
  SND_ARMADILLO,
  SND_SHOT,
  SND_BANANA,
  SND_BOLT,
  SND_1UP,
  SND_DRIP,
  SND_SPLASH,
  SND_FUSE,
  SND_ROBOFLY,
  SND_WEAPON,
  SND_FREEZE,
  SND_SPRING,
  SND_ROCK,
  SND_CUCKOO,
  SND_GLASS,
  NUM_SOUNDS
};

#define SND_BOSS1   SND_FREEZE
#define SND_BOSS2   SND_BANANA
#define SND_BOSS3   SND_CUCKOO
#define SND_BOSS4   SND_ROCK


/* Sound filenames: */

char * sound_names[NUM_SOUNDS] = {
  "explode", "hurt", "die",
  "land", "bump", "armadillo",
  "shot",
  "banana", "bolt", "1up",
  "drip", "splash",
  "fuse", "robofly", "weapon",
  "freeze", "spring", "rock",
  "cuckoo", "glass"
};


/* Music abstractions: */

#define MUSIC_TITLE 0
#define MUSIC_LEVEL1 1  /* Antarctic */
#define MUSIC_LEVEL2 2  /* Egypt */
#define MUSIC_LEVEL3 3  /* Swizterland */
#define MUSIC_LEVEL4 4  /* Grand canyon */
#define MUSIC_LEVEL5 5  /* Moon */
#define MUSIC_SELECT 6
#define MUSIC_BOSS 7
#define NUM_MODS 8

/* Music filenames: */

char * music_names[NUM_MODS] = {
  "welcome",
  "ambpower",
  "cd_orbit",
  "cd_star",
  "bombtrac",
  "zak",
  "walkntit",
  "belewian"
};


#ifdef SOUND

Mix_Chunk * samples[NUM_SOUNDS];

#ifdef MUSIC
Mix_Music * songs[NUM_MODS];
#endif

#endif

int dont_play;


/* X-Window Color Stuff: */

typedef struct _color_type {
  unsigned int red;
  unsigned int green;
  unsigned int blue;
  unsigned long pixel;
  int owner;
} color_type;


Colormap colormap;
int num_seen_colors;
color_type color_list[MAX_COLORS];
GC colorgc[MAX_COLORS];
GC threed_gc[NUM_THREEDS][MAX_COLORS];
GC whitegc, blackgc, green0gc, green1gc, green2gc, red0gc, red1gc, red2gc,
  yellow0gc, yellow1gc, yellow2gc, thickredgc, dark_gc;
GC objgcs[NUM_OBJECTS];
int black, white, dark_width, dark_height;


/* X-Window display/window stuff: */

char server[512];
Display * display;
Window window, root;
Pixmap backbuffer;
Pixmap masterbkg[3];
Visual * temp_visual;
int screen;


/* X-Window font stuff: */

XFontStruct * font;
int fh;


/* Object buffers: */

Pixmap object_pixmaps[NUM_OBJECTS], object_masks[NUM_OBJECTS];
Pixmap dark_pix;


/* Joystick globals: */

#ifdef JOYSTICK
int js_fd;
struct JS_DATA_TYPE js;
#endif


/* Typedefs: */

typedef struct player_type {
  int x, y;
  int ox, oy;
  int health, want_health, lives;
  int dir;
  int walk_count, still_count, blink_timer, ouch_timer, die_timer,
    warp_in_timer, frozen, drip_timer, was_in_water;
  int minecart_in, minecart_dir;
  int xspeed;
  int jump, jumpspeed, ym, jumptimer;
  int selected_weapon;
  int weapon[NUM_WEAPONS];
} player_type;

typedef struct platform_type {
  int alive, type, mode, timer;
  int x, y, xm, ym, xmm, ymm;
} platform_type;

typedef struct enemy_type {
  int alive, type, health, mode, dead;
  int x, y, dir;
  int wantx, wanty;
  int ox, oy;
  int xm, ym, ymm;
  int timer;
  int was_shot, frozen, drip_timer;
  int enemyslot;
} enemy_type;

typedef struct explosion_type {
  int alive, time;
  int x, y;
} explosion_type;

typedef struct air_bubble_type {
  int alive;
  int x, y, xm, xmm;
} air_bubble_type;

typedef struct star_type {
  int x, y, xm;
} star_type;

typedef struct threed_type {
  int alive;
  float x, y, z, xm, zm;
  int color_pos;
} threed_type;

typedef struct drip_type {
  int alive;
  int x, y, ym;
} drip_type;

typedef struct dust_type {
  int alive, time;
  int x, y;
} dust_type;

typedef struct ice_chunk_type {
  int alive, time;
  int x, y, xm, ym;
} ice_chunk_type;

typedef struct spark_type {
  int alive, time;
  int x, y, xm, ym;
} spark_type;

typedef struct note_letter_type {
  int alive, mode, timer, c;
  int x, y, ym;
} note_letter_type;

typedef struct snowflake_type {
  int alive;
  int x, y;
} snowflake_type;

typedef struct splash_type {
  int alive, time;
  int x, y;
} splash_type;

typedef struct up_type {
  int alive, type, time;
  float x, y, destx, desty, xm, ym;
  int enemyslot;
} up_type;

typedef struct bullet_type {
  int alive, type, owner, dead;
  int x, y;
  int xm, ym;
  int timer;
} bullet_type;


/* Page info: */

typedef struct _page_type {
  char background[TILE_HEIGHT][TILE_WIDTH];
  int left, right, up, down;
  int lowgravity, /* G */
    suction,      /* u */
    bosspage,     /* Bb */
    checkpoint,   /* C */
    snowy,        /* S */
    dark,         /* D */
    flickery,     /* F */
    mario,        /* M */
    hat;          /* 1-4 */
  int num_enemies;
  struct enemy_type starting_enemies[MAX_ENEMIES];
  char note[MAX_NOTE_LETTERS + 1];
} page_type;


/* Game globals: */

int level, use_sound, toggle, otoggle, ttoggle, frame, scrn, num_pages,
  last_checkpoint, goto_boss, gameover, distance, toggle2, ttoggle2,
  have_seen_title, last_threed_which, seen_metal_effect, ok_for_evil,
  oldbosspage, no_music, no_sound;
float cos_anglex, sin_anglex, cos_angley, sin_angley, anglex, angley;
int levels_won[8];
int threed_shape[NUM_THREEDS][32][32];
struct explosion_type explosion[MAX_EXPLOSIONS];
struct air_bubble_type air_bubble[MAX_AIR_BUBBLES];
struct drip_type drip[MAX_DRIPS];
struct dust_type dust[MAX_DUSTS];
struct ice_chunk_type ice_chunk[MAX_ICE_CHUNKS];
struct spark_type spark[MAX_SPARKS];
struct note_letter_type note_letter[MAX_NOTE_LETTERS];
struct snowflake_type snowflake[MAX_SNOWFLAKES];
struct splash_type splash[MAX_SPLASHES];
struct up_type up[MAX_UPS];
struct bullet_type bullet[MAX_BULLETS];
struct player_type player;
struct enemy_type enemy[MAX_ENEMIES];
struct platform_type platform[MAX_PLATFORMS];
page_type page[MAX_PAGES];
int old_player_x[MAX_WARP_INS], old_player_y[MAX_WARP_INS];


/* Local function prototypes: */

void initlevel(void);
int menu(void);
void eventloop(void);
void setup(int argc, char * argv[]);
void Xsetup(void);
void Xsetup_windows(void);
unsigned long MyAllocNamedColor(Display *display, Colormap colormap,
                                char* colorname, unsigned long default_color,
                                int has_color);
void loadobject(int i);
void color_clear(void);
int color_seen(unsigned int red, unsigned int green, unsigned int blue);
int color_add(unsigned int red, unsigned int green, unsigned int blue,
              unsigned long pixel, int owner);
void addexplosion(int x, int y, int play_sound);
void addplatform(int x, int y, int type);
void addup(int x, int y, int whichup, int whichenemyslot);
void addair_bubble(int x, int y);
void adddrip(int x, int y);
void adddust(int x, int y);
void addice_chunk(int x, int y);
void addspark(int x, int y);
void addsnowflake(int x, int y);
void addsplash(int x, int y);
void addbullet(int x, int y, int type, int owner, int xm, int ym);
void drawobject(int x, int y, int shape, Pixmap pix);
int solid(int x, int y, int who);
int issolid(char c);
int water(int x, int y);
int jumpthroughable(int x, int y);
int sand(int x, int y);
int conveyor(int x, int y);
int quicksand(int x, int y);
int ice(int x, int y);
int cactus(int x, int y);
void gotothispage(void);
void applyshape(char z, int x, int y);
void reset(void);
void killplayer(void);
void eraselevel(int x, int y);
void hurtplayer(int damage);
void killenemy(int whichenemy);
void addenemy(int x, int y, int type, int dir);
void levelintro(void);
int title(void);
void calc3d(short * sx, short * sy, float x, float y, float z);
void recalculatetrig(void);
void weaponscreen(void);
void playsound(int which);


/* ---- MAIN FUNCTION ---- */

int main(int argc, char * argv[])
{
  int done, i;
#ifdef JOYSTICK
  char axes, buttons;
#endif
#ifdef SOUND
  char temp[1024];
#endif
  
  
  /* Program setup: */
  
  setup(argc, argv);
  randinit();
  
  
#ifdef JOYSTICK
  fprintf(stderr, "(Using joystick...");
  
  js_fd = open("/dev/js0", 0, O_RDONLY);
  if (js_fd < 0)
    {
      perror("opening joystick");
      fprintf(stderr, ")\n");
    }
  else
    {
      ioctl(js_fd, JSIOCGAXES, &axes);
      ioctl(js_fd, JSIOCGBUTTONS, &buttons);
      
      if (axes < 2)
	{
	  fprintf(stderr, "Not enough axes! (%d, need 2!))\n", axes);
	  exit(0);
	}
      
      if (buttons < 2)
	{
	  fprintf(stderr, "Not enough buttons! (%d, need 2!))\n", buttons);
	  exit(0);
	}
      
      fcntl(js_fd, F_SETFL, O_NONBLOCK);
  
      fprintf(stderr, "Feels good!)\n");
    }
#endif


#ifdef SOUND
  fprintf(stderr, "(Using sound...");
  
#ifdef __amigaos__
  /* AHI and the screen are not given back at exit unless SDL_Quit runs,
     and BoboBot exits by exit() on any missing file. */
  atexit(SDL_Quit);
#endif
  if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
      fprintf(stderr, "Couldn't initialize SDL sound.\n%s\n",
	      SDL_GetError());
      exit(1);
    }
  
  if (Mix_OpenAudio(11025, AUDIO_S8, 2, 512) < 0)
    {
      fprintf(stderr,
	      "Warning: Couldn't set 11025 Hz 8-bit stereo audio.\n%s\n",
	      SDL_GetError());
      exit(1);
    }
  
  if (no_sound == 0)
    {
      /* (Load sound files): */
      for (i = 0; i < NUM_SOUNDS; i++)
	{
	  sprintf(temp, "%s" DATA_SEP "sounds/%s.wav", DATA, sound_names[i]);
	  samples[i] = Mix_LoadWAV(temp);
	  if (samples[i] == NULL)
	    {
	      fprintf(stderr, "Couldn't load %s: %s\n", temp, SDL_GetError());
	      exit(1);
	    }
	}
    }
  else
    fprintf(stderr, "(Sound effects disabled)...");
  
#ifdef MUSIC
  if (no_music == 0)
    {
      /* (Load mod files): */
      for (i = 0; i < NUM_MODS; i++)
	{
	  sprintf(temp, "%s" DATA_SEP "mods/%s.mod", DATA, music_names[i]);
	  songs[i] = Mix_LoadMUS(temp);
	  if (songs[i] == NULL)
	    {
	      fprintf(stderr, "Couldn't load %s: %s\n", temp, SDL_GetError());
	      exit(1);
	    }
	}
    }
  else
    fprintf(stderr, "(Music disabled)...");
#endif

  fprintf(stderr, "Sounds good!)\n");
  
#endif
  dont_play = 0;
  
  
  /* Connect to the X Servers and draw the program's window: */
  
  Xsetup_windows();
  Xsetup();
  
  have_seen_title = 0;
  
  
  /* Run the main loop: */
  
  do
    {
      /* Title screen: */
      
      do
	{
	  done = title();
	}
      while (done == 2);
      
      
      /* Stop music: */
      
#ifdef MUSIC
      if (no_music == 0)
	Mix_HaltMusic();
#endif
      
      XSync(display, 0);
      
      
      if (done == 0)
	{
	  /* Init. levels: */
	  
	  for (i = 0; i < 8; i++)
	    levels_won[i] = 0;
	  
	  for (i = 4; i < 8; i++)
	    levels_won[i] = 1;
	  
	  
	  /* Init. lives: */
	  
	  player.lives = 3;
	  
	  
	  /* Init. Weapons: */
	  
	  player.weapon[0] = 25;
	  
	  for (i = 1; i < NUM_WEAPONS; i++)
	    player.weapon[i] = 0;
	  
	  
	  seen_metal_effect = 0;
	  ok_for_evil = 0;
	  
	  do
	    {
	      gameover = menu();
	      
	      XSync(display, 0);
	      
	      if (gameover == 0)
		{
		  dont_play = 0;
		  levelintro();
		  
		  XSync(display, 0);
		  
		  initlevel();
		  eventloop();
		}
	    }
	  while (gameover == 0);
	}
    }
  while (done == 0);
  
  
  printf("\n\nThanks for playing BoboBot\n");
  printf("by Bill Kendrick, Marianne Waage & Melissa Hardenbrook!\n\n");
  
  printf("Send comments, donations, questions and bug-reports to:\n\n");
  
  printf("New Breed Software\n");
  printf("c/o Bill Kendrick\n");
  printf("919 Drake Drive #147\n");
  printf("Davis, CA 95616-0838, USA\n\n");
  
  printf("E-mail: bobobot@newbreedsoftware.com\n");
  printf("   Web: http://www.newbreedsoftware.com/bobobot/\n");
  printf("\n\n");
  
#ifdef SOUND
  for (i = 0; i < MIX_CHANNELS; i++)
    Mix_HaltChannel(i);
#endif

  return(0);
  exit(0);
}


/* Menu Screen: */

int menu(void)
{
  XEvent event;
  struct timeval now, then;
  long time_padding;
  int done, quit_mode, quit, middle, x, xx, y, num_levels_won, i, zz, c,
    toggle, otoggle, ttoggle, mod_counter, blinktimer, z, metal_effect_timer,
    ojsx, ojsy, key;
  star_type metal[4];
  
  
  /* Draw main menu background: */

  middle = WIDTH / 2;
  c = 0;
  
  /* (Clear) */
  XFillRectangle(display, masterbkg[0], blackgc, 0, 0, WIDTH, HEIGHT);
  
  /* (Title) */
  drawobject(middle - 32, 0, OBJ_TITLE_LEFT, masterbkg[0]);
  drawobject(middle, 0, OBJ_TITLE_RIGHT, masterbkg[0]);
  
  
  /* (LEVELS:) */
  /* ---Left--- */
  /* (Antarctica) */
  drawobject(16, 16, OBJ_BACK_GREY, masterbkg[0]);
  drawobject(48, 16, OBJ_BACK_GREY, masterbkg[0]);
  drawobject(48, 16, OBJ_BACK_CLOUD_LEFT, masterbkg[0]);
  drawobject(16, 48, OBJ_FORE_SNOW_LEFT, masterbkg[0]);
  drawobject(48, 48, OBJ_FORE_SNOW_RIGHT, masterbkg[0]);
  if (levels_won[0])
    eraselevel(16, 16);
  
  /* (Egypt) */
  drawobject(16, 96, OBJ_BACK_SKY, masterbkg[0]);
  drawobject(48, 96, OBJ_BACK_SKY, masterbkg[0]);
  drawobject(16, 96, OBJ_BACK_TAN_LEFT_EDGE, masterbkg[0]);
  drawobject(48, 96, OBJ_BACK_TAN_RIGHT_EDGE, masterbkg[0]);
  drawobject(16, 128, OBJ_FORE_SAND, masterbkg[0]);
  drawobject(48, 128, OBJ_FORE_SAND, masterbkg[0]);
  if (levels_won[1])
    eraselevel(16, 96);

  /* (Switzerland) */
  drawobject(16, 176, OBJ_BACK_STARS0, masterbkg[0]);
  drawobject(48, 176, OBJ_BACK_PINE, masterbkg[0]);
  drawobject(16, 208, OBJ_FORE_CONVEYOR_LEFT_0, masterbkg[0]);
  drawobject(48, 208, OBJ_FORE_CONVEYOR_RIGHT_0, masterbkg[0]);
  if (levels_won[2])
    eraselevel(16, 176);
  
  /* ---Middle--- */
  /* (Grand Canyon) */
  drawobject(middle - 32, 16, OBJ_BACK_ROCK_0, masterbkg[0]);
  drawobject(middle, 16, OBJ_BACK_ROCK_1, masterbkg[0]);
  drawobject(middle - 32, 48, OBJ_BACK_SKY, masterbkg[0]);
  drawobject(middle - 32, 48, OBJ_FORE_TRACK_BROKEN_LEFT, masterbkg[0]);
  drawobject(middle, 48, OBJ_FORE_TRACK, masterbkg[0]);
  if (levels_won[3])
    eraselevel(middle - 32, 16);

  /* (Evil Bobo's Hideout) */
  drawobject(middle - 32, 96, OBJ_BACK_SPACE_0, masterbkg[0]);
  drawobject(middle, 96, OBJ_BACK_SPACE_1, masterbkg[0]);
  drawobject(middle - 32, 128, OBJ_BACK_SPACE_2, masterbkg[0]);
  drawobject(middle, 128, OBJ_BACK_SPACE_3, masterbkg[0]);
  
  num_levels_won = 0;
  
  for (i = 0; i < 8; i++)
    num_levels_won = num_levels_won + levels_won[i];
  
  /* (???) */
  drawobject(middle - 32, 176, OBJ_BACK_SKY, masterbkg[0]);
  drawobject(middle, 176, OBJ_BACK_SKY, masterbkg[0]);
  drawobject(middle - 32, 208, OBJ_BACK_SKY, masterbkg[0]);
  drawobject(middle, 208, OBJ_BACK_SKY, masterbkg[0]);
  if (levels_won[4])
    eraselevel(middle - 32, 176);
  
  /* ---Right--- */
  /* (???) */
  drawobject(WIDTH - 80, 16, OBJ_BACK_SKY, masterbkg[0]);
  drawobject(WIDTH - 48, 16, OBJ_BACK_SKY, masterbkg[0]);
  drawobject(WIDTH - 80, 48, OBJ_BACK_SKY, masterbkg[0]);
  drawobject(WIDTH - 48, 48, OBJ_BACK_SKY, masterbkg[0]);
  if (levels_won[5])
    eraselevel(WIDTH - 80, 16);
  
  /* (???) */
  drawobject(WIDTH - 80, 96, OBJ_BACK_SKY, masterbkg[0]);
  drawobject(WIDTH - 48, 96, OBJ_BACK_SKY, masterbkg[0]);
  drawobject(WIDTH - 80, 128, OBJ_BACK_SKY, masterbkg[0]);
  drawobject(WIDTH - 48, 128, OBJ_BACK_SKY, masterbkg[0]);
  if (levels_won[6])
    eraselevel(WIDTH - 80, 96);

  /* (???) */
  drawobject(WIDTH - 80, 176, OBJ_BACK_SKY, masterbkg[0]);
  drawobject(WIDTH - 48, 176, OBJ_BACK_SKY, masterbkg[0]);
  drawobject(WIDTH - 80, 208, OBJ_BACK_SKY, masterbkg[0]);
  drawobject(WIDTH - 48, 208, OBJ_BACK_SKY, masterbkg[0]);
  if (levels_won[7])
    eraselevel(WIDTH - 80, 176);

  
  /* MAIN LOOP: */
  
  done = False;
  quit = 0;
  quit_mode = 0;
  x = 1;
  xx = 0;
  zz = 0;
  y = 1;
  z = 0;
  level = -1;
  toggle = 0;
  ttoggle = 0;
  mod_counter = 0;
  blinktimer = 0;
  goto_boss = 0;
  metal_effect_timer = 0;
  ojsx = 128;
  ojsy = 128;
  
  do
    {
      /* Toggle our toggle switches: */
      
      otoggle = toggle;
      toggle = 1 - toggle;
      
      if (toggle == 0 && otoggle == 1)
	ttoggle = 1 - ttoggle;
      
      if (toggle == 0 && ttoggle == 0)
	mod_counter++;
      
      
      gettimeofday(&then, NULL);


      key = XK_VoidSymbol;
      
#ifdef JOYSTICK
      read(js_fd, &js, JS_RETURN);
      
      if (js.x < 64 && ojsx >= 64)
	key = KEYBD_LEFT;
      else if (js.x > 192 && ojsx <= 192)
	key = KEYBD_RIGHT;
      
      if (js.y < 64 && ojsy >= 64)
	key = KEYBD_UP;
      else if (js.y > 192 && ojsy <= 192)
	key = KEYBD_DOWN;
      
      ojsx = js.x;
      ojsy = js.y;
      
      if (js.buttons)
	key = KEYBD_FIRE;
#endif
      
      
      /* Get and handle events: */
      
      while (XPending(display))
	{
	  XNextEvent(display, &event);
	  
	  if (event.type == KeyPress)
	    {
	      /* Get the key's name: */
	      
	      key = XLookupKeysym((XKeyEvent *)&event, 0);
	      
	      if (key == XK_q)
		{
		  /* Q: Quit: */
		  
		  quit_mode = 1;
		}
	    }
	}	      
      
      if (quit_mode == 0 && seen_metal_effect != 1 &&
	  seen_metal_effect != 2)
	{
	  if (key == XK_space ||
	      key == XK_KP_Enter ||
	      key == XK_Return ||
	      key == XK_b ||
	      key == KEYBD_FIRE)
	    {
	      /* Space: Start */
	      
	      if (level != -1)
		{
		  if (levels_won[level - 1] == 0)
		    done = True;
		  
		  if (key == XK_b)
		    goto_boss = 1;
		}
	    }
	  
	  
	  if (key == KEYBD_UP || key == XK_KP_8)
	    {
	      y--;
	      
	      if (y < 0)
		y = 0;
	      
	      zz = 0;
	    }
	  else if (key == KEYBD_DOWN || key == XK_F31 || key == XK_KP_5 ||
		   key == XK_KP_2)
	    {
	      y++;
	      
	      if (y > 2)
		y = 2;
	      
	      zz = 0;
	    }
	  else if (key == KEYBD_LEFT || key == XK_KP_4)
	    {
	      x--;
	      
	      if (x < 0)
		x = 0;
	      
	      zz = 0;
	    }
	  else if (key == KEYBD_RIGHT || key == XK_KP_6)
	    {
	      x++;
	      
	      if (x > 2)
		x = 2;
	      
	      zz = 0;
	    }
	}
      else
	{
	  if (key == XK_y)
	    {
	      quit = 1;
	      done = True;
	    }
	  else if (key == XK_n)
	    quit_mode = 0;
	}
      
      if (x == 0)
	level = y + 1;
      else if (x == 1)
	{
	  if (y == 0)
	    level = 4;
	  else if (y == 1)
	    level = -1;
	  else if (y == 2)
	    level = 5;
	}
      else if (x == 2)
	level = 6 + y;
      
      
      /* Redraw window: */
      
      /* (Clear to default bkgd) */
      XCopyArea(display, masterbkg[0], backbuffer, whitegc,
		0, 0, WIDTH, HEIGHT, 0, 0);
      
      
      /* (Boss) */
      
      if (level != -1)
	{
	  if (levels_won[level - 1] == 0)
	    {
	      if (level == 1)
		c = OBJ_BOSS1_RIGHT_0 + ((mod_counter / 10) % 2);
	      else if (level == 2)
		{
		  z = mod_counter % 4;
		  
		  if (z == 0)
		    c = OBJ_BOSS2_RIGHT_0;
		  else if (z == 1)
		    c = OBJ_BOSS2_RIGHT_1;
		  else if (z == 2)
		    c = OBJ_BOSS2_RIGHT_2;
		  else if (z == 3)
		    c = OBJ_BOSS2_RIGHT_1;
		}
	      else if (level == 3)
		{
		  z = mod_counter % 4;
		  
		  if (z == 0)
		    c = OBJ_BOSS3_1;
		  else if (z == 1 || z == 3)
		    c = OBJ_BOSS3_0;
		  else if (z == 2)
		    c = OBJ_BOSS3_2;
		}
	      else if (level == 4)
		{
		  c = OBJ_BOSS4_0 + ((mod_counter / 5) % 2);
		}
	      else
		c = -1;
	    }
	  else
	    c = OBJ_DONE;
	  
	  drawobject(xx + 16, y * 80 + 32, c, backbuffer);
	}
      else
	{
	  if (num_levels_won < 8)
	    {
	      drawobject(xx, y * 80 + 16, OBJ_FORE_METAL, backbuffer);
	      drawobject(xx + 32, y * 80 + 16, OBJ_FORE_METAL, backbuffer);
	      drawobject(xx, y * 80 + 48, OBJ_FORE_METAL, backbuffer);
	      drawobject(xx + 32, y * 80 + 48, OBJ_FORE_METAL, backbuffer);
	    }
	  else
	    {
	      if (seen_metal_effect == 0)
		{
		  seen_metal_effect = 1;
		  
		  metal[0].x = middle - 32;
		  metal[0].y = 96;
		  
		  metal[1].x = middle;
		  metal[1].y = 96;
		  
		  metal[2].x = middle - 32;
		  metal[2].y = 128;
		  
		  metal[3].x = middle;
		  metal[3].y = 128;
		}
	      else if (seen_metal_effect == 1)
		{
		  for (i = 0; i < 4; i++)
		    {
		      drawobject(metal[i].x + randnum(3) - 1,
				 metal[i].y + randnum(3) - 1,
				 OBJ_FORE_METAL, backbuffer);
		    }
		  
		  metal_effect_timer++;
		  
		  if (metal_effect_timer >= 50)
		    seen_metal_effect = 2;
		}
	      else if (seen_metal_effect == 2)
		{
		  for (i = 0; i < 4; i++)
		    {
		      drawobject(metal[i].x, metal[i].y, OBJ_FORE_METAL,
				 backbuffer);
		    }
		  
		  metal[0].x = metal[0].x - 4;
		  metal[0].y = metal[0].y - 4;
		  metal[1].x = metal[1].x + 4;
		  metal[1].y = metal[1].y - 4;
		  metal[2].x = metal[2].x - 4;
		  metal[2].y = metal[2].y + 4;
		  metal[3].x = metal[3].x + 4;
		  metal[3].y = metal[3].y + 4;
		  
		  if (metal[0].y < -32)
		    seen_metal_effect = 3;
		}
	      else if (seen_metal_effect == 3)
		{
		  drawobject(xx + 16, y * 80 + 32, OBJ_EVIL_LEFT_STAND,
			     backbuffer);
		  ok_for_evil = 1;
		}
	    }
	}
      
      
      /* (Cursor) */
      if (x == 0)
	xx = 16;
      else if (x == 1)
	xx = middle - 32;
      else if (x == 2)
	xx = WIDTH - 80;
      
      if (quit_mode == 0)
	zz = (zz + 1) % 32;
      
      if (seen_metal_effect != 1 && seen_metal_effect != 2)
	XDrawRectangle(display, backbuffer, whitegc,
		       xx + zz, y * 80 + 16 + zz,
		       64 - zz * 2, 64 - zz * 2);
      
      
      /* (Level's title) */
      if (level != -1)
	drawcenteredtext(display, backbuffer, whitegc, 0, WIDTH, HEIGHT - 2,
			 level_names[level - 1], font);
      else
	{
	  if (num_levels_won < 8)
	    drawcenteredtext(display, backbuffer, whitegc,
			     0, WIDTH, HEIGHT - 2,
			     "???", font);
	  else
	    drawcenteredtext(display, backbuffer, whitegc,
			     0, WIDTH, HEIGHT - 2,
			     "Evil Bobo's Hideout!", font);
	}
      
      
      /* (Quit prompt) */
      if (quit_mode == 1)
	{
	  drawcenteredtext(display, backbuffer, whitegc, 0,
			   WIDTH, HEIGHT / 2 + toggle + ttoggle,
			   " Quit, you sure? ", font);
	}
      
      
      /* Update backbuffer: */
      
      XCopyArea(display, backbuffer, window, whitegc,
		0, 0, WIDTH, HEIGHT, 0, 0);
      
      
      /* Keep framerate exact: */
      
      gettimeofday(&now, NULL);
      
      time_padding = FRAMERATE - ((now.tv_sec - then.tv_sec) * 1000000 +
				  (now.tv_usec - then.tv_usec));
      
      if (time_padding > 0)
	usleep(time_padding);


      /* Play music: */
#ifdef MUSIC
      if (no_music == 0)
	{
	  if (!Mix_PlayingMusic())
	    {
	      Mix_PlayMusic(songs[MUSIC_SELECT], 1); 
	    }
	}
#endif
    }
  while (done == False);
  

  /* Stop Music: */

#ifdef MUSIC
  if (no_music == 0)
    Mix_HaltMusic();
#endif

  return(quit);
}


/* --- MAIN EVENT LOOP --- */

void eventloop(void)
{
  int i, j, k, done, mod_counter, c, want_dir,
    be_affected_by_gravity, x, y, num_enemies,
    end_of_level, end_timer, xm, ym, z, flick_time, flick_on, rock_shake,
    player_fired_at, num_player_bullets, amhead, ojsb, goweaponscreen,
    gravity, hatx, haty, key;
  long time_padding;
  XEvent event;
  char tempstr[1024];
  struct timeval now, then;
  
  
  reset();
  
  
  /* MAIN LOOP: */
  
  toggle = 0;
  ttoggle = 0;
  mod_counter = 0;
  frame = 0;
  done = False;
  end_of_level = 0;
  end_timer = 0;
  player_fired_at = 999;
  num_player_bullets = 0;
  hatx = 0;
  haty = 0;
  
  c = 0;
  want_dir = -1;
  
  flick_time = 0;
  flick_on = 0;
  rock_shake = 0;
  
  strcpy(tempstr, "");
  
  
  /* Begin on page #0: */
  
  scrn = 0;
  last_checkpoint = 0;
  
  if (goto_boss == 1)
    {
      for (i = 0; i < num_pages; i++)
	{
	  if (page[i].checkpoint == 1)
	    {
	      last_checkpoint = i;
	      scrn = i;
	    }
	}
    }
  
  gotothispage();
  ojsb = 0;
  
  do
    {
      /* Toggle our toggle switches: */
      
      otoggle = toggle;
      toggle = 1 - toggle;
      
      if (toggle && otoggle == 0)
	ttoggle = 1 - ttoggle;
      
      mod_counter = (mod_counter + 1);
      
      
      gettimeofday(&then, NULL);
      
      
      /* Handle gravity:*/
      
      if (page[scrn].lowgravity == 0 || (mod_counter % 3) == 0)
	gravity = 1;
      else
	gravity = 0;
      
      
      /* Keep track of last keypress values: */
      
      for (i = 0; i < NUM_KEYS; i++)
	{
	  old_holds[i] = holds[i];
	}
      
      
      /* Get and handle events: */
      
      key = XK_VoidSymbol;
      goweaponscreen = 0;
      
#ifdef JOYSTICK
      read(js_fd, &js, JS_RETURN);
      
      if (js.x < 64)
	stick_holds[KEY_LEFT] = KeyPress;
      else
	stick_holds[KEY_LEFT] = KeyRelease;

      if (js.x > 192)
	stick_holds[KEY_RIGHT] = KeyPress;
      else
	stick_holds[KEY_RIGHT] = KeyRelease;
      
      if (js.y < 64)
	stick_holds[KEY_UP] = KeyPress;
      else
	stick_holds[KEY_UP] = KeyRelease;
      
      if (js.y > 192)
	stick_holds[KEY_DOWN] = KeyPress;
      else
	stick_holds[KEY_DOWN] = KeyRelease;
      
      if (js.buttons & 1)
	stick_holds[KEY_JUMP] = KeyPress;
      else
	stick_holds[KEY_JUMP] = KeyRelease;

      if (js.buttons & 2)
	stick_holds[KEY_FIRE] = KeyPress;
      else
	stick_holds[KEY_FIRE] = KeyRelease;
      
      if (js.buttons > 3 && ojsb <= 3)
	goweaponscreen = 1;

      ojsb = js.buttons;
#endif
      
      while (XPending(display))
	{
	  XNextEvent(display, &event);
	  
	  if (event.type == KeyPress || event.type == KeyRelease)
	    {
	      /* Get the key's name: */
	      
	      key = XLookupKeysym((XKeyEvent *)&event, 0);
	      
	      if (key == XK_q && event.type == KeyPress)
		{
		  /* Q: Abort: */
		  
		  killplayer();
		  
		  if (gameover == 0)
		    gameover = 1;
		  else
		    done = 1;
		}
	      
	      
	      if ((key == XK_Tab || key == XK_Escape) &&
		  event.type == KeyPress)
		{
		  /* Tab / Escape: Pause / Change weapon: */
		  
		  goweaponscreen = 1;
		}
	      
	      if (key == KEYBD_UP || key == XK_KP_8)
		{
		  /* Up: Nothing??? */
		  
	      key_holds[KEY_JUMP] = event.type;
		}
	      else if (key == KEYBD_DOWN || key == XK_F31 ||
		       key == XK_KP_5 || key == XK_KP_2)
		{
		  /* Down: Nothing??? */
		  
		  key_holds[KEY_DOWN] = event.type;
		}
	      else if (key == KEYBD_LEFT || key == XK_4)
		{
		  /* Left: Move left */
		  
		  key_holds[KEY_LEFT] = event.type;
		}
	      else if (key == KEYBD_RIGHT || key == XK_6)
		{
		  /* Right: Move right */
		  
		  key_holds[KEY_RIGHT] = event.type;
		}
	      else if (key == KEYBD_FIRE)
		{
		  /* Z: Fire */
		  
		  key_holds[KEY_FIRE] = event.type;
		}
	      else if (key == KEYBD_JUMP || key == XK_KP_Space)
		{
		  /* Space: Jump */
		  
		  key_holds[KEY_JUMP] = event.type;
		}
	    }
	}
      
      
      for (i = 0; i < NUM_KEYS; i++)
	{
	  if (key_holds[i] == KeyPress || stick_holds[i] == KeyPress)
	    holds[i] = KeyPress;
	  else
	    holds[i] = KeyRelease;
	}
      
      
      /* Go to weapon screen? */
      
      if (goweaponscreen == 1 && gameover == 0 && num_player_bullets == 0)
	{
	  weaponscreen();
	  
	  
	  /* Kill all keyboard and stick recordings: */
	  
	  for (i = 0; i < NUM_KEYS; i++)
	    {
	      holds[i] = KeyRelease;
	      old_holds[i] = KeyRelease;
	      key_holds[i] = KeyRelease;
	      stick_holds[i] = KeyRelease;
	    }
	}
      

      /* Record old player positions (in case of collision): */
      
      player.oy = player.y;
      player.ox = player.x;
      
      
      /* Count-down ouch-timer: */
      
      if (player.ouch_timer != 0)
	player.ouch_timer--;

      
      /* Count-down die-timer: */
      
      if (player.die_timer != 0)
	{
	  player.die_timer--;
	  
	  if (player.die_timer == 0)
	    {
	      if (gameover == 0)
		{
		  /* Stop playing boss music: */
#ifdef MUSIC
		  if (page[scrn].bosspage != 0 && no_music == 0)
		    Mix_HaltMusic();
#endif

		  scrn = last_checkpoint;
		  gotothispage();
		  reset();
		}
	      else
		done = 1;
	    }
	}
      
      
      /* Count down weapon effects: */
      
      if (player.frozen > 0)
	{
	  player.frozen--;
	  
	  
	  /* Drip when you've thawed: */
	  
	  if (player.frozen == 0)
	    player.drip_timer = 50;
	}
      
      
      /* ONLY IF PLAYER IS ALIVE AND NOT FROZEN: */
      
      if (player.die_timer == 0)
	{
	  /* Move player left/right: */
	  
	  if ((holds[KEY_RIGHT] == KeyPress &&
	       holds[KEY_LEFT] == KeyPress) ||
	      (holds[KEY_RIGHT] == KeyRelease &&
	       holds[KEY_LEFT] == KeyRelease))
	    {
	      /* Stop moving (or slow down if on ice) */
	      
	      if (ice(player.x, player.y + 32) == 0)
		player.xspeed = 0;
	      else
		{
		  if (ttoggle == 0 && toggle == 0)
		    {
		      player.xspeed--;
		      if (player.xspeed < 0)
			player.xspeed = 0;
		    }
		}
	      
	      player.walk_count = 0;
	      
	      player.still_count++;
	      
	      /* (Catch overflow:) */
	      
	      if (player.still_count >= MIN_BLINK_TIME)
		player.still_count = MIN_BLINK_TIME;
	    }
	  else if (player.ouch_timer < 10)
	    {
	      /* (Out of control if you're hurt...) */
	      
	      if (holds[KEY_RIGHT] == KeyPress)
		{
		  player.dir = DIR_RIGHT;
		  
		  if (player.xspeed == 0)
		    want_dir = DIR_RIGHT;
		}
	      else if (holds[KEY_LEFT] == KeyPress)
		{
		  player.dir = DIR_LEFT;
		  
		  if (player.xspeed == 0)
		    want_dir = DIR_LEFT;
		}
	      
	      
	      /* Handle walk/still counters: */
	      
	      player.still_count = 0;
	      
	      if (ttoggle == 0 && toggle == 0)
		{
		  player.walk_count = (player.walk_count + 1) % 4;
		}
	      
	      
	      /* Make him walk faster and faster as you hold key down: */
	      
	      if (ttoggle == 0 && toggle == 0)
		{
		  if (player.dir == want_dir)
		    player.xspeed++;
		  else
		    player.xspeed--;
		  
		  if (player.xspeed > MAX_XSPEED)
		    player.xspeed = MAX_XSPEED;
		  else if (player.xspeed < 0)
		    player.xspeed = 0;
		}
	    }
	  
	  
	  /* Be affected by conveyor belt: */
	  
	  z = conveyor(player.x, player.y + 32);
	  
	  if (z == DIR_LEFT && solid(player.x - 4, player.y, -1) == 0)
	    player.x = player.x - 4;
	  else if (z == DIR_RIGHT && solid(player.x + 4, player.y, -1) == 0)
	    player.x = player.x + 4;


	  /* Be affected by platforms: */
	  
	  for (i = 0; i < MAX_PLATFORMS; i++)
	    {
	      if (platform[i].alive == 1)
		{
		  if (player.x + 31 >= platform[i].x &&
		      player.x <= platform[i].x + 64 &&
		      player.y >= platform[i].y - 48 &&
		      player.y <= platform[i].y + 31 &&
		      player.jump == 0)
		    {
		      player.y = platform[i].y - 32;
		      player.x = player.x + platform[i].xm;
		    }
		}
	    }
	  
	  
	  /* Standard left/right movement stuff: */
	  
	  if (player.xspeed != 0)
	    {
	      if (player.frozen == 0)
		{
		  if (want_dir == DIR_RIGHT)
		    player.x = player.x + player.xspeed;
		  else
		    player.x = player.x - player.xspeed;
		}
	      
	      
	      /* Don't let him crash into stuff: */
	      
	      if (solid(player.x, player.y, -1) &&
		  jumpthroughable(player.x, player.y) == 0)
		{
		  player.x = player.ox;
		  player.xspeed = 0;
		}
	    }
	  
	  
	  /* Handle jumping: */
	  
	  if (player.frozen == 0)
	    {
	      if (holds[KEY_JUMP] == KeyPress)
		{
		  if (player.minecart_in == -1)
		    {
		      if ((player.jump == 0 &&
			   solid(player.x, player.y + 32, -1) &&
			   old_holds[KEY_JUMP] == KeyRelease) ||
			  (player.jump == 1 &&
			   ((player.jumptimer < 10 &&
			     water(player.x, player.y) == 0) ||
			    (player.jumptimer < 20 &&
			     water(player.x, player.y) == 1)) &&
			   player.jumpspeed >= STARTING_JUMPSPEED - 1))
			{
			  player.jump = 1;
			  player.jumpspeed = STARTING_JUMPSPEED;
			  player.jumptimer++;
			}
		      
		      if (player.jump == 0 && quicksand(player.x, player.y) &&
			  old_holds[KEY_JUMP] == KeyRelease)
			{
			  player.jump = 1;
			  player.jumpspeed = STARTING_JUMPSPEED;
			  player.jumptimer = 19;
			}
		    }
		  else
		    {
		      /* Make the minecart jump,
			 or jump out of it if it's already jumping: */
		      
		      if (old_holds[KEY_JUMP] == KeyRelease)
			{
			  if (solid(enemy[player.minecart_in].x,
				    enemy[player.minecart_in].y + 32,
				    player.minecart_in) &&
			      enemy[player.minecart_in].ym >= 0)
			    {
			      enemy[player.minecart_in].ym = -10;
			    }
			  else
			    {
			      player.y = player.y - 16;
			      player.jump = 1;
			      player.jumpspeed = STARTING_JUMPSPEED;
			      player.jumptimer = 19;
			    }
			}
		    }
		}
	      else
		{
		  if (player.jump == 0)
		    player.jumptimer = 0;
		}
	      
	      
	      if (player.jump == 1)
		{
		  player.y = player.y - player.jumpspeed;
		  
		  player.jumpspeed--;
		  
		  if (player.jumpspeed == 0)
		    {
		      player.jump = 0;
		    }
		  
		  if (solid(player.x, player.y, -1) &&
		      jumpthroughable(player.x, player.y) == 0)
		    {
		      playsound(SND_BUMP);
		      player.jump = 0;
		      
		      
		      /* Handle mario stuff: */
		      
		      if (page[scrn].mario == 1)
			{
			  for (x = player.x - 16; x < player.x + 48;
			       x = x + 32)
			    {
			      if (page[scrn].background[player.y / 32][x / 32]
				  == 'B')
				{
				  page[scrn].background[player.y / 32][x / 32]
				    = '-';
				  
				  applyshape('-', x / 32, player.y / 32);
				  
				  addup((x / 32) * 32,
					(player.y / 32) * 32 - 32,
					UP_COIN, -1);
				  
				  addbullet((x / 32) * 32 + 16,
					    (player.y / 32) * 32 + 16,
					    WPN_BITS,
					    OWN_PLAYER, -8, 16);
			  
				  addbullet((x / 32) * 32 + 16,
					    (player.y / 32) * 32 + 16,
					    WPN_BITS,
					    OWN_PLAYER, 8, 16);
				}
			    }
			}
		    }
		}
	    }
	  
	  
	  /* Fall or land (or get sucked in by quicksand) */
	  
	  if (player.jump == 0 && solid(player.x, player.y + 32, -1) == 0)
	    {
	      if (quicksand(player.x, player.y) == 0)
		{
		  player.y = player.y + player.ym;
		  
		  if (water(player.x, player.y) == 0 ||
		      (ttoggle == 0 && toggle == 0))
		    player.ym = player.ym + gravity;
		  
		  if (player.ym > 16)
		    player.ym = 16;
		}
	      else
		{
		  player.y = player.y + 5;
		  player.ym = 5;
		}
	    }
	  else
	    {
	      while (solid(player.x, player.y + 31, -1) && player.jump == 0)
		{
		  player.y--;
		}
	      
	      
	      /* Make landing noise: */
	      
	      if (player.ym > 0)
		playsound(SND_LAND);
	      
	      
	      /* Add dust if you landed: */
	      
	      if (player.ym > 0 && sand(player.x, player.y + 32))
		{
		  adddust(player.x - 16, player.y + 16);
		  adddust(player.x + 16, player.y + 16);
		}
	      
	      player.ym = 0;
	    }
	  
	  
	  /* Handle water suction: */
	  
	  if (water(player.x, HEIGHT - 32) && page[scrn].suction == 1)
	    player.y = player.y + 4;
	  
	  if (water(player.x + 32, HEIGHT - 32) &&
	      page[scrn].suction == 1)
	    player.x = player.x + 1;
	  else if (water(player.x - 32, HEIGHT - 32) &&
		   page[scrn].suction == 1)
	    player.x = player.x - 1;
	  
	  
	  /* Handle moving off the screen's edges: */
	  
	  if (player.x > WIDTH - 32 - 1)
	    {
	      if (page[scrn].right != -1)
		{
		  scrn = page[scrn].right;
		  player.x = 1;
		  
		  gotothispage();
		}
	      else
		player.x = WIDTH - 32 - 1;
	    }
	  else if (player.x < 0)
	    {
	      if (page[scrn].left != -1)
		{
		  scrn = page[scrn].left;
		  player.x = WIDTH - 32 - 2;
		  
		  gotothispage();
		}
	      else
		player.x = 4;
	    }
	  
	  if (player.y < 0)
	    {
	      if (page[scrn].up != -1)
		{
		  scrn = page[scrn].up;
		  player.y = HEIGHT - 32;
		  
		  gotothispage();
		}
	      else
		player.y = 0;
	    }
	  else if (player.y > HEIGHT - 32)
	    {
	      if (page[scrn].down >= 0)
		{
		  scrn = page[scrn].down;
		  player.y = 0;
		  
		  gotothispage();
		}
	      else
		{
		  if (page[scrn].down == -1)
		    {
		      /* Bottom of screen == nothing? */
		      
		      player.y = HEIGHT - 32;
		    }
		  else if (page[scrn].down == -2)
		    {
		      /* Bottom of screen == death? */
		      
		      killplayer();
		    }
		}
	    }
	  
	  
	  /* Make him blink: */
	  
	  if (player.blink_timer > 0)
	    player.blink_timer--;
	  
	  if (player.still_count == MIN_BLINK_TIME &&
	      randnum(CHANCE_OF_BLINK) == 0 && player.blink_timer == 0)
	    {
	      player.blink_timer = MIN_BLINK_DURATION +
		randnum(MAX_BLINK_DURATION - MIN_BLINK_DURATION);
	    }
	  
	  
	  /* Fire! */
	  
	  if ((holds[KEY_FIRE] == KeyPress &&
	       old_holds[KEY_FIRE] == KeyRelease) &&
	      player.frozen == 0 && player.ouch_timer < 10 &&
	      num_player_bullets < 3 &&
	      player.weapon[player.selected_weapon] > 0)
	    {
	      playsound(SND_SHOT);
	      
	      /* Reset "fired at" timer: */
	      
	      player_fired_at = 0;
	      
	      
	      /* Use up some weapon: */
	      
	      player.weapon[player.selected_weapon] =
		(player.weapon[player.selected_weapon] -
		 weapon_usage[player.selected_weapon]);
	      
	      if (player.weapon[player.selected_weapon] < 0)
		player.weapon[player.selected_weapon] = 0;
	      
	      
	      if (player.selected_weapon == WPN_NORMAL ||
		  player.selected_weapon == WPN_ICE ||
		  player.selected_weapon == WPN_WRAPS)
		{
		  /* Most weapons just go left/right: */
		  
		  if (player.dir == DIR_LEFT)
		    addbullet(player.x, player.y,
			      player.selected_weapon, OWN_PLAYER,
			      -10, 0);
		  else if (player.dir == DIR_RIGHT)
		    addbullet(player.x, player.y,
			      player.selected_weapon, OWN_PLAYER,
			      +10, 0);
		}
	      else if (player.selected_weapon == WPN_SPRING)
		{
		  /* Spring goes upwards and left or right: */
		  
		  if (player.dir == DIR_LEFT)
		    addbullet(player.x, player.y,
			      WPN_SPRING, OWN_PLAYER,
			      -10, -10);
		  else if (player.dir == DIR_RIGHT)
		    addbullet(player.x, player.y,
			      WPN_SPRING, OWN_PLAYER,
			      +10, -10);
		}
	      else if (player.selected_weapon == WPN_ROCK)
		{
		  /* Determine direction to fire: */
		  
		  xm = 0;
		  if (holds[KEY_LEFT] == KeyPress)
		    xm = -10;
		  else if (holds[KEY_RIGHT] == KeyPress)
		    xm = 10;
		  
		  ym = 0;
		  if (holds[KEY_UP] == KeyPress)
		    ym = -10;
		  else if (holds[KEY_DOWN] == KeyPress)
		    ym = 10;
		  
		  
		  /* No direction keys held? Determine based on dir. faced: */
		  
		  if (xm == 0 && ym == 0)
		    {
		      if (player.dir == DIR_LEFT)
			xm = -10;
		      else if (player.dir == DIR_RIGHT)
			xm = 10;
		    }
		  
		  addbullet(player.x, player.y,
			    player.selected_weapon, OWN_PLAYER,
			    xm, ym);
		}
	    }
	}
      
      
      /* Handle explosions: */
      
      for (i = 0; i < MAX_EXPLOSIONS; i++)
	{
	  if (explosion[i].alive == 1)
	    {
	      explosion[i].time--;
	      
	      if (explosion[i].time <= 0)
		explosion[i].alive = 0;
	    }
	}
      
      
      /* Handle up's: */
      
      for (i = 0; i < MAX_UPS; i++)
	{
	  if (up[i].alive == 1)
	    {
	      /* Move upgrade: */
	      
	      up[i].y = up[i].y + up[i].ym;
	      
	      up[i].ym = up[i].ym + gravity;
	      if (up[i].ym > 16)
		up[i].ym = 16;
	      
	      while (solid(up[i].x, up[i].y + 32, -1) && up[i].y >= 0)
		up[i].y--;
	      
	      
	      /* Conveyor belt: */
	      
	      z = conveyor(up[i].x, up[i].y + 32);
	      
	      if (z == DIR_LEFT)
		up[i].x = up[i].x - 4;
	      else if (z == DIR_RIGHT)
		up[i].x = up[i].x + 4;
	      
	      
	      /* Countdown upgrade's time: */
	      
	      if (up[i].time > 0)
		{
		  up[i].time--;
		  
		  if (up[i].time == 0 || up[i].y < 0)
		    {
		      up[i].alive = 0;
		      addexplosion(up[i].x, up[i].y, 1);
		    }
		}
	      
	      
	      /* Detect collision with player: */
	      
	      if (player.die_timer == 0 &&
		  player.x + 31 >= up[i].x &&
		  player.x <= up[i].x + 31 &&
		  player.y + 31 >= up[i].y &&
		  player.y <= up[i].y + 31)
		{
		  if (up[i].type == UP_BANANA_SMALL ||
		      up[i].type == UP_BANANA_BIG ||
		      (up[i].type == UP_COIN && player.health < 25))
		    {
		      /* Banana gives health: */

		      if (player.health < 25)
			{
			  playsound(SND_BANANA);
			  
			  if (up[i].type == UP_BANANA_SMALL)
			    player.health = player.health + 2;
			  else if (up[i].type == UP_BANANA_BIG)
			    player.health = player.health + 10;
			  else if (up[i].type == UP_COIN)
			    {
			      player.health = 25;
			      up[i].type = UP_NONE;
			    }
			  
			  if (player.health > 25)
			    player.health = 25;
			  
			  up[i].destx = 10;
			  up[i].desty = 20;
			}
		      else
			{
			  up[i].destx = up[i].x;
			  up[i].desty = HEIGHT;
			}
		    }
		  else if (up[i].type == UP_BOLT_SMALL ||
		      up[i].type == UP_BOLT_BIG ||
		      (up[i].type == UP_COIN &&
		       player.weapon[player.selected_weapon] < 25))
		    {
		      /* Bolt gives weapon: */
		      
		      if (player.weapon[player.selected_weapon] < 25)
			{
			  playsound(SND_BOLT);
			  
			  if (up[i].type == UP_BOLT_SMALL)
			    player.weapon[player.selected_weapon] =
			      player.weapon[player.selected_weapon] + 2;
			  else if (up[i].type == UP_BOLT_BIG)
			    player.weapon[player.selected_weapon] =
			      player.weapon[player.selected_weapon] + 10;
			  else if (up[i].type == UP_COIN)
			    player.weapon[player.selected_weapon] = 25;
			  
			  if (player.weapon[player.selected_weapon] > 25)
			    player.weapon[player.selected_weapon] = 25;
			  
			  up[i].destx = 30;
			  up[i].desty = 20;
			}
		      else
			{
			  up[i].destx = up[i].x;
			  up[i].desty = HEIGHT;
			}
		    }
		  else if (up[i].type == UP_COIN)
		    {
		      /* Useless coin! */
		      
		      up[i].destx = up[i].x;
		      up[i].desty = HEIGHT;
		    }
		  else if (up[i].type == UP_ONEUP)
		    {
		      /* One-up gives an extra life: */
		      
		      player.lives++;
		      
		      up[i].destx = WIDTH - 42;
		      up[i].desty = 20;
		      
		      playsound(SND_1UP);
		    }
		  
		  
		  /* Determine up's trajectory: */
		  
		  up[i].xm = (up[i].destx - up[i].x) / 10;
		  up[i].ym = (up[i].desty - up[i].y) / 10;
		  
		  
		  /* Switch to "fly" mode: */
		  
		  up[i].alive = 2;
		}
	    }
	  else if (up[i].alive == 2)
	    {
	      /* Make upgrade fly to where it belongs: */
	      
	      up[i].x = up[i].x + up[i].xm;
	      up[i].y = up[i].y + up[i].ym;
	      
	      if (up[i].x >= up[i].destx - 16 &&
		  up[i].x <= up[i].destx + 16 &&
		  up[i].y >= up[i].desty - 16 &&
		  up[i].y <= up[i].desty + 16)
		{
		  up[i].alive = 0;
		  
		  /* Remove from this page's enemy slot, if it was
		     an "enemy" up: */
		  
		  if (up[i].enemyslot != -1)
		    page[scrn].starting_enemies[up[i].enemyslot].type =
		      BAD_NONE;
		}
	    }
	}
      
      
      /* Add drips: */
      
      if (player.drip_timer > 0)
	{
	  player.drip_timer--;
	  
	  if (randnum(CHANCE_OF_DRIP) == 0)
	    adddrip(player.x - 16 + randnum(32),
		    player.y + randnum(16));
	}
      
      
      /* Add dust: */
      
      if (sand(player.x, player.y + 32) && player.xspeed > 2 &&
	  ttoggle == 0 && toggle == 0)
	adddust(player.x, player.y + 16);
      
      
      /* Add air bubbles / Handle splashing / Deal with dripping: */
      
      if (water(player.x, player.y))
	{
	  player.drip_timer = 0;
	  
	  if (randnum(CHANCE_OF_BUBBLE) == 0 && player.die_timer == 0)
	    {
	      if (player.dir == DIR_LEFT)
		addair_bubble(player.x, player.y);
	      else
		addair_bubble(player.x + 26, player.y);
	    }
	  
	  if (player.was_in_water == 0)
	    addsplash(player.x, player.y - 24);
	  
	  player.was_in_water = 1;
	}
      else
	{
	  if (player.was_in_water == 1)
	    {
	      addsplash(player.x, player.y - 24);
	      player.drip_timer = 100;
	    }
	  
	  player.was_in_water = 0;
	}
      
      
      /* Add snowflakes: */
      
      if (page[scrn].snowy == 1)
	{
	  if (randnum(CHANCE_OF_SNOWFLAKE) == 0)
	    addsnowflake(-1, -1);
	}
      
      
      /* Handle air bubbles: */
      
      for (i = 0; i < MAX_AIR_BUBBLES; i++)
	{
	  if (air_bubble[i].alive == 1)
	    {
	      air_bubble[i].x = air_bubble[i].x + air_bubble[i].xmm;
	      air_bubble[i].y = air_bubble[i].y - 2;
	      
	      air_bubble[i].xm = air_bubble[i].xm + air_bubble[i].xmm;
	      
	      if (air_bubble[i].xm < -2)
		air_bubble[i].xmm = 1;
	      else if (air_bubble[i].xm > 2)
		air_bubble[i].xmm = -1;
	      
	      if (air_bubble[i].y <= 0 ||
		  water(air_bubble[i].x, air_bubble[i].y) == 0)
		air_bubble[i].alive = 0;
	    }
	}
      
      
      /* Handle drips: */
      
      for (i = 0; i < MAX_DRIPS; i++)
	{
	  if (drip[i].alive == 1)
	    {
	      drip[i].y = drip[i].y + drip[i].ym;
	      
	      drip[i].ym = drip[i].ym + gravity;
	      
	      if (drip[i].ym > 16)
		drip[i].ym = 16;
	      
	      if (water(drip[i].x, drip[i].y) ||
		  solid(drip[i].x, drip[i].y, -1) ||
		  drip[i].y > HEIGHT)
		drip[i].alive = 0;
	    }
	}
      
      
      /* Handle dusts: */
      
      for (i = 0; i < MAX_DUSTS; i++)
	{
	  if (dust[i].alive == 1)
	    {
	      dust[i].time--;
	      
	      if (dust[i].time <= 0)
		dust[i].alive = 0;
	    }
	}
      
      
      /* Handle ice_chunks: */
      
      for (i = 0; i < MAX_ICE_CHUNKS; i++)
	{
	  if (ice_chunk[i].alive == 1)
	    {
	      ice_chunk[i].x = ice_chunk[i].x + ice_chunk[i].xm;
	      ice_chunk[i].y = ice_chunk[i].y + ice_chunk[i].ym;
	      
	      ice_chunk[i].ym = ice_chunk[i].ym + gravity;
	      
	      if (solid(ice_chunk[i].x, ice_chunk[i].y, -1))
		ice_chunk[i].ym = - ice_chunk[i].ym;
	      
	      ice_chunk[i].time--;
	      
	      if (ice_chunk[i].time == 0)
		ice_chunk[i].alive = 0;
	    }
	}
      
      
      /* Handle sparks: */
      
      for (i = 0; i < MAX_SPARKS; i++)
	{
	  if (spark[i].alive == 1)
	    {
	      spark[i].x = spark[i].x + spark[i].xm;
	      spark[i].y = spark[i].y + spark[i].ym;
	      spark[i].ym++;
	      
	      spark[i].time--;
	      
	      if (spark[i].time <= 0 || spark[i].y > HEIGHT)
		spark[i].alive = 0;
	    }
	}
      
      
      /* Handle note letters: */
      
      for (i = 0; i < MAX_NOTE_LETTERS; i++)
	{
	  if (note_letter[i].alive == 1)
	    {
	      if (note_letter[i].timer == 0)
		{
		  note_letter[i].y = note_letter[i].y + note_letter[i].ym;
		  
		  note_letter[i].ym++;
		  
		  if (note_letter[i].y >= HEIGHT / 2 &&
		      note_letter[i].mode == 0)
		    {
		      note_letter[i].y = HEIGHT / 2;
		      note_letter[i].timer = 70;
		      note_letter[i].mode = 1;
		      note_letter[i].ym = 0;
		    }
		  
		  if (note_letter[i].y > HEIGHT)
		    note_letter[i].alive = 0;
		}
	      else
		{
		  note_letter[i].timer--;
		}
	    }
	}
      
      
      /* Handle snowflakes: */
      
      for (i = 0; i < MAX_SNOWFLAKES; i++)
	{
	  if (snowflake[i].alive == 1)
	    {
	      if (solid(snowflake[i].x, snowflake[i].y + 16, -1))
		snowflake[i].alive = 0;
	      
	      snowflake[i].y = snowflake[i].y + 3;
	    }
	}
      
      
      /* Handle platforms: */
      
      for (i = 0; i < MAX_PLATFORMS; i++)
	{
	  if (platform[i].alive == 1)
	    {
	      platform[i].x = platform[i].x + platform[i].xm;
	      platform[i].y = platform[i].y + platform[i].ym;
	      
	      if (platform[i].type == BAD_PLATFORM_DOWN)
		{
		  platform[i].ym = 2;
		  
		  if (platform[i].y > HEIGHT)
		    platform[i].y = -32;
		}
	      else if (platform[i].type == BAD_PLATFORM_UP)
		{
		  platform[i].ym = -2;
		  
		  if (platform[i].y < -32)
		    platform[i].y = HEIGHT;
		}
	      else if (platform[i].type == BAD_PLATFORM_UP_DOWN)
		{
		  if (ttoggle == 0 && toggle == 0)
		    {
		      platform[i].ym = platform[i].ym + platform[i].ymm;
		      
		      if (platform[i].ym >= 4)
			platform[i].ymm = -1;
		      else if (platform[i].ym <= -4)
			platform[i].ymm = 1;
		    }
		}
	      else if (platform[i].type == BAD_PLATFORM_SIDE)
		{
		  if (ttoggle == 0 && toggle == 0)
		    {
		      platform[i].xm = platform[i].xm + platform[i].xmm;
		      
		      if (platform[i].xm >= 4)
			platform[i].xmm = -1;
		      else if (platform[i].xm <= -4)
			platform[i].xmm = 1;
		    }
		}
	      else if (platform[i].type == BAD_PLATFORM_DROP)
		{
		  if (player.x >= platform[i].x - 32 &&
		      player.x <= platform[i].x + 64 &&
		      player.y == platform[i].y - 32)
		    {
		      platform[i].timer++;
		      
		      if (platform[i].timer >= 15)
			platform[i].mode = MODE_DROP;
		    }
		  else
		    platform[i].timer = 0;
		  
		  if (platform[i].mode == MODE_DROP)
		    {
		      platform[i].ym = platform[i].ym + 1;
		      if (platform[i].ym > 16)
			platform[i].ym = 16;
		      
		      if (platform[i].y > HEIGHT)
			platform[i].alive = 0;
		    }
		}
	    }
	}
      
      
      /* Handle splashes: */
      
      for (i = 0; i < MAX_SPLASHES; i++)
	{
	  if (splash[i].alive == 1)
	    {
	      if (ttoggle == 0 && toggle == 0)
		{
		  splash[i].time++;
		  
		  if (splash[i].time == 2)
		    splash[i].alive = 0;
		}
	    }
	}
      
      
      /* Handle enemies: */
      
      num_enemies = 0;
      player.minecart_in = -1;
      
      for (i = 0; i < MAX_ENEMIES; i++)
	{
	  if (enemy[i].alive == 1)
	    {
	      /* Keep track of how many enemies onscreen: */
	      
	      num_enemies++;
	      
	      
	      if (enemy[i].frozen == 0)
		{
		  /* Determine good bullet trajectory: */
		  
		  xm = (player.x - enemy[i].x);
		  ym = (player.y - enemy[i].y);
		  
		  z = sqrt(xm * xm + ym * ym + 4);
		  
		  xm = (xm * 8) / z;
		  ym = (ym * 8) / z;
		  
		  
		  /* Remember my old position, if I need to go back: */
		  
		  enemy[i].ox = enemy[i].x;
		  enemy[i].oy = enemy[i].y;
		  
		  
		  /* Drip: */
		  
		  if (enemy[i].drip_timer > 0)
		    {
		      enemy[i].drip_timer--;
		      
		      if (randnum(CHANCE_OF_DRIP) == 0)
			adddrip(enemy[i].x - 16 + randnum(32),
				enemy[i].y + randnum(16));
		    }
		  
		  
		  /* Default: no gravity: */
		  
		  be_affected_by_gravity = GRAV_NONE;
		  
		  
		  if (enemy[i].type == BAD_PENGUIN)
		    {
		      /* Penguin */
		      
		      if (enemy[i].x < player.x - 8)
			{
			  enemy[i].x++;
			  enemy[i].dir = DIR_RIGHT;
			}
		      else if (enemy[i].x > player.x + 8)
			{
			  enemy[i].x--;
			  enemy[i].dir = DIR_LEFT;
			}
		      else
			{
			  if (enemy[i].y >= player.y - 4 && enemy[i].ym == 0 &&
			      solid(enemy[i].x, enemy[i].y + 32, i))
			    {
			      enemy[i].ym = -8;
			    }
			}
		      
		      be_affected_by_gravity = GRAV_NORMAL;
		    }
		  else if (enemy[i].type == BAD_BOMB)
		    {
		      /* Bomb */
		      
		      /* Walk towards, in normal mode, if you're far: */
		      
		      if (enemy[i].x < player.x - 48)
			{
			  enemy[i].x = enemy[i].x + 2;
			  enemy[i].mode = MODE_NORMAL;
			}
		      else if (enemy[i].x > player.x + 48)
			{
			  enemy[i].x = enemy[i].x - 2;
			  enemy[i].mode = MODE_NORMAL;
			}
		      else
			{
			  if (enemy[i].mode != MODE_SPARK)
			    {
			      /* Switch into spark mode, if not already: */
			      
			      enemy[i].mode = MODE_SPARK;
			      enemy[i].timer = 20;
			    }
			  else
			    {
			      /* Spark: */
			      
			      if (randnum(10) < 5)
				{
				  addspark(enemy[i].x + 16, enemy[i].y + 8);
				  playsound(SND_FUSE);
				}
			      
			      
			      /* Explode when timer's up! */
			      
			      enemy[i].timer--;
			      
			      if (enemy[i].timer <= 0)
				{
				  killenemy(i);
				  
				  addexplosion(enemy[i].x, enemy[i].y, 1);
				  
				  addbullet(enemy[i].x + 16, enemy[i].y + 16,
					    WPN_BITS, OWN_ENEMY,
					    0, -10);
				  addbullet(enemy[i].x + 16, enemy[i].y + 16,
					    WPN_BITS, OWN_ENEMY,
					    5, -5);
				  addbullet(enemy[i].x + 16, enemy[i].y + 16,
					    WPN_BITS, OWN_ENEMY,
					    10, 0);
				  addbullet(enemy[i].x + 16, enemy[i].y + 16,
					    WPN_BITS, OWN_ENEMY,
					    5, 5);
				  addbullet(enemy[i].x + 16, enemy[i].y + 16,
					    WPN_BITS, OWN_ENEMY,
					    0, 10);
				  addbullet(enemy[i].x + 16, enemy[i].y + 16,
					    WPN_BITS, OWN_ENEMY,
					    -5, 5);
				  addbullet(enemy[i].x + 16, enemy[i].y + 16,
					    WPN_BITS, OWN_ENEMY,
					    -10, 0);
				  addbullet(enemy[i].x + 16, enemy[i].y + 16,
					    WPN_BITS, OWN_ENEMY,
					    -5, -5);
				}
			    }
			}
		      
		      if (enemy[i].y > HEIGHT)
			enemy[i].alive = 0;
		      
		      be_affected_by_gravity = GRAV_NORMAL;
		    }
		  else if (enemy[i].type == BAD_ROBOFLY)
		    {
		      /* Robofly: */
		      
		      enemy[i].y = enemy[i].y + enemy[i].ym;
		      
		      enemy[i].ym = enemy[i].ym + enemy[i].ymm;
		      
		      if (player.x > enemy[i].x - 48 &&
			  player.x < enemy[i].x + 48)
			j = 10;
		      else
			j = 5;
		      
		      if (enemy[i].ym < -j || enemy[i].ymm == 0)
			enemy[i].ymm = 1;
		      else if (enemy[i].ym > j)
			enemy[i].ymm = -1;
		      
		      if (enemy[i].y < 0)
			{
			  enemy[i].y = 0;
			  enemy[i].ym = 0;
			  enemy[i].ymm = 1;
			}
		      else if (enemy[i].y > HEIGHT - 32)
			{
			  enemy[i].y = HEIGHT - 32;
			  enemy[i].ym = 0;
			  enemy[i].ymm = -1;
			}

		      if ((mod_counter % 5) == 0)
			playsound(SND_ROBOFLY);
		    }
		  else if (enemy[i].type == BAD_ICICLE)
		    {
		      /* Icicle: */
		      
		      enemy[i].y = enemy[i].y + enemy[i].ym;
		      
		      enemy[i].ym = enemy[i].ym + enemy[i].ymm;
		      
		      if (player.x > enemy[i].x - 48 &&
			  player.x < enemy[i].x + 48)
			enemy[i].ymm = 1;
		      
		      if (solid(enemy[i].x, enemy[i].y, i) ||
			  enemy[i].y > HEIGHT)
			{
			  /* Smash into icecubes: */
			  
			  for (j = 0; j < 2; j++)
			    addice_chunk(enemy[i].x, (enemy[i].y / 32) * 32);
			  
			  playsound(SND_GLASS);
			  killenemy(i);
			}
		      
		      
		      /* Drip: */
		      
		      enemy[i].timer--;
		      
		      if (enemy[i].timer <= 0)
			{
			  adddrip(enemy[i].x, enemy[i].y);
			  enemy[i].timer = randnum(10) + 50;
			}
		    }
		  else if (enemy[i].type == BAD_SPIDER)
		    {
		      /* Spider */
		      
		      if (enemy[i].mode == MODE_RUSH)
			{
			  /* "Rush" mode: */
			  
			  if (enemy[i].x < player.x)
			    enemy[i].x = enemy[i].x + 5;
			  else if (enemy[i].x > player.x)
			    enemy[i].x = enemy[i].x - 5;
			  
			  enemy[i].timer--;
			  
			  if (enemy[i].timer == 0)
			    enemy[i].mode = MODE_NORMAL;
			}
		      else
			{
			  /* Wait mode.  Might switch to "rush": */
			  
			  if (randnum(CHANCE_OF_RUSH) == 0)
			    {
			      enemy[i].mode = MODE_RUSH;
			      enemy[i].timer = randnum(10) + 20;
			    }
			}
		      
		      be_affected_by_gravity = GRAV_NORMAL;
		    }
		  else if (enemy[i].type == BAD_ARMADILLO)
		    {
		      /* Armadillo */
		      
		      /* Face player: */
		      
		      if (enemy[i].x < player.x)
			enemy[i].dir = DIR_RIGHT;
		      else
			enemy[i].dir = DIR_LEFT;
		      
		      if (enemy[i].mode == MODE_RUSH)
			{
			  /* "Rush" mode: */
			  
			  if (enemy[i].x < player.x)
			    enemy[i].x = enemy[i].x + 5;
			  else if (enemy[i].x > player.x)
			    enemy[i].x = enemy[i].x - 5;
			  
			  enemy[i].timer--;
			  
			  if (enemy[i].timer == 0)
			    enemy[i].mode = MODE_NORMAL;
			  
			  be_affected_by_gravity = GRAV_BOUNCE;
			}
		      else
			{
			  /* Wait mode.  Might switch to "rush": */
			  
			  if (randnum(CHANCE_OF_RUSH) == 0)
			    {
			      enemy[i].mode = MODE_RUSH;
			      enemy[i].timer = randnum(10) + 20;
			      enemy[i].ym = -4;
			    }
			  
			  be_affected_by_gravity = GRAV_NORMAL;
			}
		    }
		  else if (enemy[i].type == BAD_TUMBLEWEED)
		    {
		      /* Tumbleweed: */
		      
		      enemy[i].x = enemy[i].x - 4;
		      if (enemy[i].ym > 4)
			enemy[i].ym = 4;
		      else if (enemy[i].ym < -4)
			enemy[i].ym = -4;
		      
		      be_affected_by_gravity = GRAV_BOUNCE;
		    }
		  else if (enemy[i].type == BAD_ROCKET)
		    {
		      /* Rocket */
		      
		      if (enemy[i].mode == MODE_NORMAL)
			{
			  /* Face player and go that way: */
			  
			  if (enemy[i].x < player.x)
			    enemy[i].dir = DIR_RIGHT;
			  else
			    enemy[i].dir = DIR_LEFT;
			  
			  enemy[i].mode = MODE_RUSH;
			}
		      else
			{
			  if (enemy[i].dir == DIR_RIGHT)
			    enemy[i].x = enemy[i].x + 2;
			  else
			    enemy[i].x = enemy[i].x - 2;

			  if (enemy[i].y < player.y)
			    enemy[i].y = enemy[i].y + 2;
			  else
			    enemy[i].y = enemy[i].y - 2;
			  
			  be_affected_by_gravity = GRAV_NONE;
			  
			  if (enemy[i].x < -32)
			    enemy[i].alive = 0;
			  else if (enemy[i].x > 32 * 10)
			    enemy[i].alive = 0;
			}
		    }
		  else if (enemy[i].type == BAD_FISH)
		    {
		      /* Fish */
		      
		      if (enemy[i].xm == 0 && enemy[i].timer == 0)
			{
			  /* If not moving, and pause time is up, attack! */
			  
			  if (enemy[i].x < player.x)
			    {
			      enemy[i].xm = 8;
			      enemy[i].dir = DIR_RIGHT;
			    }
			  else
			    {
			      enemy[i].xm = -8;
			      enemy[i].dir = DIR_LEFT;
			    }
			  
			  if (enemy[i].y < player.y)
			    enemy[i].ym = 1;
			  else
			    enemy[i].ym = -1;
			  
			  enemy[i].timer = 10 + randnum(10);
			}
		      
		      
		      /* If not paused, move!: */
		      
		      if (enemy[i].timer > 0)
			{
			  enemy[i].timer--;
			  
			  enemy[i].x = enemy[i].x + enemy[i].xm;
			  enemy[i].y = enemy[i].y + enemy[i].ym;
			  
			  if (water(enemy[i].x - 24, enemy[i].y) == 0)
			    {
			      enemy[i].y = enemy[i].oy;
			      
			      if (water(enemy[i].x - 24, enemy[i].y) == 0)
				enemy[i].x = enemy[i].ox;
			    }
			}
		      
		      
		      /* If moving, slow down: */
		      
		      if (enemy[i].xm > 0)
			enemy[i].xm = enemy[i].xm - 1;
		      else if (enemy[i].xm < 0)
			enemy[i].xm = enemy[i].xm + 1;
		    }
		  else if (enemy[i].type == BAD_GEAR)
		    {
		      /* Gear */
		      
		      if (enemy[i].mode == 0)
			{
			  if (player.x >= enemy[i].x - 128 &&
			      player.x <= enemy[i].x + 128)
			    {
			      enemy[i].mode = 1;
			      
			      if (enemy[i].x < player.x)
				enemy[i].dir = DIR_RIGHT;
			      else
				enemy[i].dir = DIR_LEFT;
			    }
			}
		      else
			{
			  if (enemy[i].dir == DIR_RIGHT)
			    enemy[i].x = enemy[i].x + 2;
			  else if (enemy[i].dir == DIR_LEFT)
			    enemy[i].x = enemy[i].x - 2;
			  
			  be_affected_by_gravity = GRAV_BOUNCE;
			}
		    }
		  else if (enemy[i].type == BAD_HELI)
		    {
		      /* Heli: */
		      
		      /* Chase player vertically,
			 don't get too close though: */
		      
		      if (enemy[i].y >= player.y - 64 && enemy[i].y > 0)
			{
			  if (enemy[i].mode != MODE_CARRY)
			    enemy[i].y = enemy[i].y - 2;
			  else
			    enemy[i].y = enemy[i].y - 1;
			}
		      else if (enemy[i].y < player.y - 96)
			enemy[i].y = enemy[i].y + 3;
		      
		      
		      /* Chase player horizontally: */
		      
		      enemy[i].x = enemy[i].x + enemy[i].xm;
		      
		      if (enemy[i].x > player.x)
			{
			  enemy[i].xm = enemy[i].xm - 1;
			  
			  if (enemy[i].xm < -2)
			    enemy[i].xm = -2;
			}
		      else if (enemy[i].x < player.x)
			{
			  enemy[i].xm = enemy[i].xm + 1;
			  
			  if (enemy[i].xm > 2)
			    enemy[i].xm = 2;
			}
		      
		      
		      if (enemy[i].mode == MODE_CARRY)
			{
			  /* Carrying a bomb?  Throw it, if in range!: */
			  
			  enemy[i].timer--;
			  
			  if (enemy[i].timer <= 0)
			    enemy[i].timer = 0;
			  
			  if (((enemy[i].x >= player.x - 64 &&
				enemy[i].x <= player.x + 128) ||
			       randnum(10) == 0) &&
			      enemy[i].timer <= 0)
			    {
			      addenemy(enemy[i].x, enemy[i].y + 16, BAD_BOMB,
				       DIR_LEFT);
			      
			      enemy[i].mode = MODE_NORMAL;
			      enemy[i].timer = 50 + randnum(25);
			    }
			}
		      else
			{
			  /* Not carrying a bomb?  Regenerate one! */
			  
			  enemy[i].timer--;
			  
			  if (enemy[i].timer <= 0)
			    {
			      enemy[i].mode = MODE_CARRY;
			      enemy[i].timer = 20;
			    }
			}
		      
		      
		      /* If underwater for some silly reason,
			 add bubbles: */
		      
		      if (water(enemy[i].x, enemy[i].y) && toggle == 0)
			addair_bubble(enemy[i].x + randnum(22) - 10,
				      enemy[i].y - 4);
		    }
		  else if (enemy[i].type == BAD_MINECART)
		    {
		      /* Minecart: */
		      
		      /* Move left/right */
		      
		      if (enemy[i].dir == DIR_LEFT)
			enemy[i].x = enemy[i].x - 4;
		      else
			enemy[i].x = enemy[i].x + 4;
		      
		      
		      /* Spark: */
		      
		      if (randnum(10) < 5 &&
			  solid(enemy[i].x, enemy[i].y + 32, i))
			addspark(enemy[i].x + randnum(32),
				 enemy[i].y + 32);
		      
		      
		      /* Go away if you fall off: */
		      
		      if (enemy[i].x < -64 || enemy[i].x > WIDTH + 32 ||
			  enemy[i].y > HEIGHT)
			enemy[i].alive = 0;
			  
		      
		      /* If you bump into something, go the other way: */
		      
		      if (solid(enemy[i].x, enemy[i].y, i))
			{
			  if (enemy[i].ym < 0)
			    {
			      enemy[i].y = enemy[i].y - enemy[i].ym + 1;
			    }
			  
			  if (solid(enemy[i].x, enemy[i].y + 32, i) == 0)
			    enemy[i].y = enemy[i].y + 4;
			  
			  if (solid(enemy[i].x, enemy[i].y, i))
			    {
			      if (enemy[i].dir == DIR_LEFT)
				{
				  for (j = 0; j < 4; j++)
				    addspark(enemy[i].x, enemy[i].y + 16);
				  
				  enemy[i].dir = DIR_RIGHT;
				  enemy[i].x = enemy[i].x + 4;
				}
			      else
				{
				  for (j = 0; j < 4; j++)
				    addspark(enemy[i].x + 32, enemy[i].y + 16);
				  
				  enemy[i].dir = DIR_LEFT;
				  enemy[i].x = enemy[i].x - 4;
				}
			    }
			}
		      
		      be_affected_by_gravity = GRAV_NORMAL;
		    }
		  else if (enemy[i].type == BAD_BIRD)
		    {
		      /* Bird: */
		      
		      /* Face left/right */
		      
		      if (enemy[i].x < player.x)
			enemy[i].dir = DIR_RIGHT;
		      else if (enemy[i].x > player.x)
			enemy[i].dir = DIR_LEFT;
		      
		      
		      /* Move left/right: */
		      
		      enemy[i].x = enemy[i].x + enemy[i].xm;
		      
		      if (enemy[i].dir == DIR_RIGHT)
			{
			  enemy[i].xm = enemy[i].xm + 1;
			  if (enemy[i].xm > 4)
			    enemy[i].xm = 4;
			}
		      else if (enemy[i].dir == DIR_LEFT)
			{
			  enemy[i].xm = enemy[i].xm - 1;
			  if (enemy[i].xm < -4)
			    enemy[i].xm = -4;
			}
		      
		      
		      /* Move up/down: */
		      
		      enemy[i].y = enemy[i].y + enemy[i].ym;
		      
		      if (enemy[i].y < player.y)
			{
			  enemy[i].ym = enemy[i].ym + 1;
			  if (enemy[i].ym > 4)
			    enemy[i].ym = 4;
			}
		      else if (enemy[i].y > player.y)
			{
			  enemy[i].ym = enemy[i].ym - 1;
			  if (enemy[i].ym < -4)
			    enemy[i].ym = -4;
			}
		    }
		  else if (enemy[i].type == BAD_SNAKE)
		    {
		      /* Snake: */
		      
		      /* Am I a head? */
		      
		      amhead = 0;
		      
		      if (i == 0)
			amhead = 1;
		      
		      if (i > 0)
			{
			  if (enemy[i - 1].alive == 0)
			    amhead = 1;
			}
		      
		      
		      /* Increment timer: */
		      
		      enemy[i].timer++;
		      
		      
		      /* Wait fore a while, then move: */
		      
		      if (enemy[i].timer >= 100)
			{
			  /* Re-pick destination every once in a while: */
			  
			  if (enemy[i].timer >= 150)
			    enemy[i].timer = 100;
			  
			  
			  /* Pick a destination: */
			  
			  if (enemy[i].timer == 100)
			    {
			      enemy[i].wantx = randnum(WIDTH) - 32;
			      enemy[i].wanty = randnum(HEIGHT) - 32;
			    }
			  
			  
			  if (amhead == 1)
			    {
			      /* Heads moves towards target: */
			      
			      if (enemy[i].x < enemy[i].wantx)
				enemy[i].xm = enemy[i].xm + randnum(3);
			      else if (enemy[i].x > enemy[i].wantx)
				enemy[i].xm = enemy[i].xm - randnum(3);
			      
			      if (enemy[i].y < enemy[i].wanty + 32)
				enemy[i].ym = enemy[i].ym + randnum(3);
			      else if (enemy[i].y > enemy[i].wanty - 32 &&
				       enemy[i].y > 64)
				enemy[i].ym = enemy[i].ym - randnum(3);
				  

			      /* Keep speed sane: */
				  
			      if (enemy[i].xm < -3)
				enemy[i].xm = -3;
			      else if (enemy[i].xm > 3)
				enemy[i].xm = 3;
			      
			      if (enemy[i].ym < -5)
				enemy[i].ym = -5;
			      else if (enemy[i].ym > 5)
				enemy[i].ym = 5;
			    }
			  else
			    {
			      /* Bodies move towards next enemy
				 (more body, or head): */
				  
			      if (enemy[i].x < enemy[i - 1].x)
				enemy[i].xm++;
			      else if (enemy[i].x > enemy[i - 1].x)
				enemy[i].xm--;
				  
			      if (enemy[i].y < enemy[i - 1].y)
				enemy[i].ym++;
			      else if (enemy[i].y > enemy[i - 1].y &&
				       enemy[i].y > 64)
				enemy[i].ym--;


			      /* Keep speed sane: */
				  
			      if (enemy[i].xm < -3)
				enemy[i].xm = -3;
			      else if (enemy[i].xm > 3)
				enemy[i].xm = 3;
			      
			      if (enemy[i].ym < -5)
				enemy[i].ym = -5;
			      else if (enemy[i].ym > 5)
				enemy[i].ym = 5;
			    }
			      
			      
			  /* Move brick: */
			      
			  enemy[i].x = enemy[i].x + enemy[i].xm;
			  enemy[i].y = enemy[i].y + enemy[i].ym;
			      
			  if (enemy[i].y < 32)
			    enemy[i].y = 32;
			      
			      
			  /* Don't let it touch it's neighbor too
			     closely: */
			  
			  if (i > 0)
			    {
			      if (enemy[i - 1].alive != 0 &&
				  enemy[i].x >= enemy[i - 1].x - 24 &&
				  enemy[i].x <= enemy[i - 1].x + 24 && 
				  enemy[i].y >= enemy[i - 1].y - 24 &&
				  enemy[i].y <= enemy[i - 1].y + 24)
				{
				  enemy[i].x = enemy[i].x -
				    enemy[i].xm / 2;
				  enemy[i].y = enemy[i].y -
				    enemy[i].ym / 2;
				}
			    }
			      
			  if (enemy[i + 1].alive != 0 &&
			      enemy[i].x >= enemy[i + 1].x - 24 &&
			      enemy[i].x <= enemy[i + 1].x + 24 &&
			      enemy[i].y >= enemy[i + 1].y - 24 &&
			      enemy[i].y <= enemy[i + 1].y + 24)
			    {
			      enemy[i].x = enemy[i].x - enemy[i].xm / 2;
			      enemy[i].y = enemy[i].y - enemy[i].ym / 2;
			    }
			      
			  
			  /* Don't let it get too far, either: */
			      
			  if (i > 0)
			    {
			      if (enemy[i - 1].alive != 0)
				{
				  if (enemy[i].x > enemy[i - 1].x + 48)
				    enemy[i].x = enemy[i - 1].x + 48;
				  else if (enemy[i].x <
					   enemy[i - 1].x - 48)
				    enemy[i].x = enemy[i - 1].x - 48;
				  
				  if (enemy[i].y > enemy[i - 1].y + 48)
				    enemy[i].y = enemy[i - 1].y + 48;
				  else if (enemy[i].y <
					   enemy[i - 1].y - 16)
				    enemy[i].y = enemy[i - 1].y - 16;;
				}
			    }
			      
			  if (enemy[i + 1].alive != 0)
			    {
			      if (enemy[i].x > enemy[i + 1].x + 48)
				enemy[i].x = enemy[i + 1].x + 48;
			      else if (enemy[i].x < enemy[i + 1].x - 48)
				enemy[i].x = enemy[i + 1].x - 48;
			      
			      if (enemy[i].y > enemy[i + 1].y + 48)
				enemy[i].y = enemy[i + 1].y + 48;
			      else if (enemy[i].y < enemy[i + 1].y - 48)
				enemy[i].y = enemy[i + 1].y - 48;
			    }
			  
			  
			  /* Tails shoot randomly: */
			  
			  if (enemy[i + 1].alive == 0 && randnum(40) == 0)
			    {
			      if (randnum(10) < 5)
				{
				  addbullet(enemy[i].x, enemy[i].y, WPN_NORMAL,
					    OWN_ENEMY, -2, -2);
				  addbullet(enemy[i].x, enemy[i].y, WPN_NORMAL,
					    OWN_ENEMY, 2, -2);
				  addbullet(enemy[i].x, enemy[i].y, WPN_NORMAL,
					    OWN_ENEMY, 2, 2);
				  addbullet(enemy[i].x, enemy[i].y, WPN_NORMAL,
					    OWN_ENEMY, -2, 2);
				}
			      else
				{
				  addbullet(enemy[i].x, enemy[i].y, WPN_NORMAL,
					    OWN_ENEMY, -2, 0);
				  addbullet(enemy[i].x, enemy[i].y, WPN_NORMAL,
					    OWN_ENEMY, 2, 0);
				  addbullet(enemy[i].x, enemy[i].y, WPN_NORMAL,
					    OWN_ENEMY, 0, 2);
				  addbullet(enemy[i].x, enemy[i].y, WPN_NORMAL,
					    OWN_ENEMY, 0, -2);
				}
			    }
			}
		    }
		  else if (enemy[i].type == BAD_ROCKER)
		    {
		      /* Rock generator just spits out rocks: */
		      
                      if (randnum(20) == 0)
			addbullet(enemy[i].x, 0, WPN_ROCK,
				  OWN_ENEMY, 0, 0);
		    }
		  else if (enemy[i].type == BAD_STEAMER)
		    {
		      /* Steamer sprays: */
		      
		      if (enemy[i].mode == MODE_NORMAL)
			{
			  if ((mod_counter - (i * 10)) % 50 == 0)
			    {
			      enemy[i].mode = MODE_STEAM;
			      enemy[i].timer = 0;
			    }
			}
		      else
			{
			  enemy[i].timer++;
			  if (enemy[i].timer >= 10)
			    enemy[i].mode = MODE_NORMAL;
			}
		    }
		  else if (enemy[i].type == BOSS1)
		    {
		      /* Boss 1 (Freeze-Man) */
		      
		      /* Reset my mode if I was shot: */
		      
		      if (enemy[i].was_shot == 1)
			{
			  enemy[i].timer = 5;
			  enemy[i].mode = 0;
			}
		      
		      
		      /* Face the player: */
		      
		      if (player.x > enemy[i].x)
			enemy[i].dir = DIR_RIGHT;
		      else
			enemy[i].dir = DIR_LEFT;
		      
		      
		      enemy[i].timer--;
		      
		      if (enemy[i].timer <= 0)
			{
			  /* Shoot at player if he's alive: */
			  
			  if (player.die_timer == 0)
			    {
			      if (enemy[i].mode == 0)
				{
				  /* Fire icicle: */
				  
				  addbullet(enemy[i].x, enemy[i].y, WPN_ICE,
					    OWN_ENEMY, xm, ym);
				  
				  enemy[i].timer = 40 + randnum(20);
				  
				  enemy[i].mode = 1;
				  
				  enemy[i].y = enemy[i].y - 8;
				  
				  if (enemy[i].x > player.x)
				    enemy[i].x = enemy[i].x - 8;
				  else if (enemy[i].x < player.x)
				    enemy[i].x = enemy[i].x + 8;
				}
			      else
				{
				  /* Fire normal bullets for a while: */
				  
				  addbullet(enemy[i].x, enemy[i].y, WPN_NORMAL,
					    OWN_ENEMY, xm, ym);
				  
				  enemy[i].timer = 20;
				  
				  enemy[i].mode++;
				  
				  if (enemy[i].mode > 10)
				    enemy[i].mode = 0;
				}
			    }
			  else
			    {
			      /* Shoot like nuts if player is dead! */
			  
			      enemy[i].y = enemy[i].y - 8;
			      
			      addbullet(enemy[i].x, enemy[i].y, WPN_ICE,
					OWN_ENEMY, 0, -4);
			      addbullet(enemy[i].x, enemy[i].y, WPN_ICE,
					OWN_ENEMY, 2, -2);
			      addbullet(enemy[i].x, enemy[i].y, WPN_ICE,
					OWN_ENEMY, 4, 0);
			      addbullet(enemy[i].x, enemy[i].y, WPN_ICE,
					OWN_ENEMY, 2, 2);
			      addbullet(enemy[i].x, enemy[i].y, WPN_ICE,
					OWN_ENEMY, 0, 4);
			      addbullet(enemy[i].x, enemy[i].y, WPN_ICE,
					OWN_ENEMY, -2, 2);
			      addbullet(enemy[i].x, enemy[i].y, WPN_ICE,
					OWN_ENEMY, -4, 0);
			      addbullet(enemy[i].x, enemy[i].y, WPN_ICE,
					OWN_ENEMY, -2, -2);
			      
			      enemy[i].timer = 50;
			    }
			}
		      
		      be_affected_by_gravity = GRAV_NORMAL;
		    }
		  else if (enemy[i].type == BOSS2)
		    {
		      /* Boss 2 (The Mummy) */
		      
		      if (enemy[i].mode == MODE_NORMAL)
			{
			  /* Be a casket for a while: */
			  
			  if (player.die_timer == 0)
			    enemy[i].timer++;
			  
			  if (enemy[i].timer >= 100)
			    {
			      /* Explode into the mummy! */
			      
			      enemy[i].mode = MODE_RUSH;
			      addexplosion(enemy[i].x, enemy[i].y, 1);
			      enemy[i].dir = DIR_RIGHT;
			      
			      addbullet(enemy[i].x, enemy[i].y, WPN_WRAPS,
					OWN_ENEMY, -5, 5);
			      
			      addbullet(enemy[i].x, enemy[i].y, WPN_WRAPS,
					OWN_ENEMY, 5, 5);
			      
			      addbullet(enemy[i].x, enemy[i].y, WPN_WRAPS,
					OWN_ENEMY, 0, 10);
			    }
			}
		      else if (enemy[i].mode == MODE_RUSH)
			{
			  /* Go back to sleep if the player's dead: */
			  
			  if (player.die_timer > 0)
			    enemy[i].mode = MODE_NORMAL;
			  
			  
			  /* Move left/right: */
			  
			  if (enemy[i].dir == DIR_LEFT)
			    enemy[i].x = enemy[i].x - 5;
			  else
			    enemy[i].x = enemy[i].x + 4;
			  
			  
			  /* Turn around at the edges of the screen: */
			  
			  if (enemy[i].x <= 32)
			    enemy[i].dir = DIR_RIGHT;
			  else if (enemy[i].x >= WIDTH - 64)
			    enemy[i].dir = DIR_LEFT;
			  
			  
			  /* Shoot every now and then: */
			  
			  enemy[i].timer--;
			  
			  if (enemy[i].timer <= 0)
			    {
			      /* Shoot the way we're facing: */
			      
			      if (enemy[i].dir == DIR_LEFT)
				xm = -10;
			      else if (enemy[i].dir == DIR_RIGHT)
				xm = 10;
			      
			      
			      /* Sometimes shoot spiders: */
			      
			      if (randnum(5) == 0)
				addenemy(enemy[i].x, enemy[i].y - 16,
					 BAD_SPIDER, DIR_LEFT);
			      else
				addbullet(enemy[i].x, enemy[i].y, WPN_WRAPS,
					  OWN_ENEMY, xm, 0);
			      
			      enemy[i].timer = 20;
			    }
			  
			  be_affected_by_gravity = GRAV_NORMAL;
			}
		    }
		  else if (enemy[i].type == BOSS3)
		    {
		      /* Boss 3 (Chronoman) */
		      
		      if (enemy[i].ym != 0)
			{
			  /* Move while jumping: */
			  
			  if (player.x > enemy[i].x &&
			      solid(enemy[i].x + 1, enemy[i].y, i) == 0)
			    enemy[i].x = enemy[i].x + 1;
			  else if (player.x < enemy[i].x &&
				   solid(enemy[i].x - 1, enemy[i].y, i) == 0)
			    enemy[i].x = enemy[i].x - 1;
			  
			  
			  /* Shoot while jumping: */
			  
			  if ((enemy[i].timer == 95 || enemy[i].timer == 80) &&
			      player.die_timer == 0)
			    {
			      if (enemy[i].x > player.x)
				xm = -5;
			      else if (enemy[i].x < player.x)
				xm = 5;
			      else
				xm = 0;
			      
			      addbullet(enemy[i].x, enemy[i].y, WPN_SPRING,
					OWN_ENEMY, xm, 0);
			    }
			}
		      
		      
		      /* Jump every now and then: */
		      
		      enemy[i].timer--;
		      
		      if (enemy[i].timer <= 0 &&
			  solid(enemy[i].x, enemy[i].y + 32, i))
			{
			  enemy[i].ym = -16;
			  enemy[i].timer = 100;
			  
			  playsound(SND_CUCKOO);
			}
		      
		      be_affected_by_gravity = GRAV_BOUNCE;
		    }
		  else if (enemy[i].type == BOSS4)
		    {
		      /* Boss 4 (Rockman) */
		      
		      if (enemy[i].y < 0)
			enemy[i].y = 32;
		      
		      if (enemy[i].timer > 100)
			{
			  if (player_fired_at > 10)
			    {
			      /* Move towards player: */
			      
			      if (enemy[i].x < player.x + 64 &&
				  solid(enemy[i].x + 3, enemy[i].y, i) == 0)
				enemy[i].x = enemy[i].x + 3;
			      else if (enemy[i].x > player.x + 64 &&
				       solid(enemy[i].x - 3,
					     enemy[i].y, i) == 0)
				enemy[i].x = enemy[i].x - 3;
			    }
			  else
			    {
			      /* Move away from player if he fired! */
			      
			      if (enemy[i].x < player.x &&
				  solid(enemy[i].x - 5, enemy[i].y, i) ==0)
				enemy[i].x = enemy[i].x - 5;
			      else if (enemy[i].x > player.x &&
				  solid(enemy[i].x + 5, enemy[i].y, i) ==0)
				enemy[i].x = enemy[i].x + 5;
			    }
			}
		      else
			{
			  /* Move towards center: */
			  
			  if (enemy[i].x < WIDTH / 2 - 16 - 5)
			    enemy[i].x = enemy[i].x + 5;
			  else if (enemy[i].x > WIDTH / 2 - 16 + 5)
			    enemy[i].x = enemy[i].x - 5;
			}
		      
		      
		      enemy[i].timer--;
		      
		      
		      /* Call for some rocks: */
		      
		      if (enemy[i].timer < 10 &&
			  (enemy[i].timer % 5) == 0)
			addbullet(enemy[i].x, 0, WPN_ROCK,
				  OWN_ENEMY, 0, 0);
			  
			  
		      /* Time's up! */
		      
		      if (enemy[i].timer <= 0 && enemy[i].x > 4 * 32 &&
			  enemy[i].x < 7 * 32)
			{
			  /* Reset timer (less time the more hurt he is): */
			  
			  if (enemy[i].health < 30)
			    enemy[i].timer = 3 * enemy[i].health;
			  else
			    enemy[i].timer = 100;
			}
		      
		      
		      /* Randomly shoot normal bullets: */
		      
		      if (randnum(10) < 1 &&
			  (enemy[i].timer % 5) == 0)
			{
			  if (player.x < enemy[i].x)
			    xm = -4;
			  else
			    xm = 4;
			  
			  addbullet(enemy[i].x + xm, enemy[i].y, WPN_NORMAL,
				    OWN_ENEMY, xm, 0);
			}
		      
		      be_affected_by_gravity = GRAV_NORMAL;
		    }
		}
	      else
		{
		  /* Be frozen: */
		  
		  if (enemy[i].frozen > 0)
		    {
		      enemy[i].frozen--;
		      
		      if (enemy[i].frozen == 0)
			enemy[i].drip_timer = 50;
		    }
		  
		  be_affected_by_gravity = 1;
		}
	      
	      
	      /* Be affected by gravity: */
	      
	      if (be_affected_by_gravity != GRAV_NONE)
		{
		  if (solid(enemy[i].x, enemy[i].y + 1, i) == 0)
		    {
		      if (water(enemy[i].x, enemy[i].y) == 0)
			enemy[i].ym = enemy[i].ym + gravity;
		      else
			enemy[i].ym = gravity;
		      
		      if (enemy[i].ym > 16)
			enemy[i].ym = 16;
		    }
		  
		  enemy[i].y = enemy[i].y + enemy[i].ym;
		  
		  
		  while (solid(enemy[i].x, enemy[i].y + 31, i))
		    {
		      enemy[i].y--;
		      
		      if (be_affected_by_gravity == GRAV_NORMAL)
			enemy[i].ym = 0;
		      else if (be_affected_by_gravity == GRAV_BOUNCE)
			{
			  enemy[i].ym = - enemy[i].ym;
			  
			  if (enemy[i].type == BAD_GEAR)
			    playsound(SND_BUMP);
			  else if (enemy[i].type == BAD_ARMADILLO)
			    playsound(SND_ARMADILLO);
			}
		    }
		  
		  
		  /* Be affected by conveyor belts: */
		  
		  z = conveyor(enemy[i].x, enemy[i].y + 32);
		  
		  if (z == DIR_LEFT)
		    enemy[i].x = enemy[i].x - 1;
		  else if (z == DIR_RIGHT)
		    enemy[i].x = enemy[i].x + 1;
		  
		  
		  /* Don't knock into things: */
		  
		  if (solid(enemy[i].x, enemy[i].y, i))
		    {
		      enemy[i].x = enemy[i].ox;
		      /* enemy[i].y = enemy[i].oy; */
		    }
		}
	      
	      
	      /* Check for collision with player's bullets: */
	      
	      enemy[i].was_shot = 0;
	      
	      for (j = 0; j < MAX_BULLETS; j++)
		{
		  if (bullet[j].alive == 1 && bullet[j].owner == OWN_PLAYER &&
		      bullet[j].dead == 0)
		    {
		      if (bullet[j].x + 31 >= enemy[i].x &&
			  bullet[j].x <= enemy[i].x + 31 &&
			  bullet[j].y + 31 >= enemy[i].y &&
			  bullet[j].y <= enemy[i].y + 31)
			{
			  /* Note that I was shot: */
			  
			  enemy[i].was_shot = 1;
			  
			  
			  if (bullet[j].type == WPN_NORMAL ||
			      bullet[j].type == WPN_WRAPS ||
			      bullet[j].type == WPN_SPRING ||
			      bullet[j].type == WPN_ROCK)
			    {
			      /* Reduce enemy's health: */
			      
			      enemy[i].health = enemy[i].health - 
				enemy_weapon_effects[enemy[i].type]
                                [bullet[j].type];
			      
			      if (enemy_weapon_effects[enemy[i].type]
				  [bullet[j].type] > 0)
				{
				  /* Add explosion if it did anything: */
				  
				  addexplosion(enemy[i].x, enemy[i].y, 1);
				  
				  
				  /* Wraps push enemy back some: */
				  
				  if (bullet[j].type == WPN_WRAPS)
				    {
				      if (solid(enemy[i].x + bullet[j].xm,
						enemy[i].y, i) == 0)
					enemy[i].x = enemy[i].x + bullet[j].xm;
				    }

				  /* Remove bullet, unless enemy died
				     and you're shooting wraps: */
				  
				  if (bullet[j].type != WPN_WRAPS ||
				      enemy[i].health > 0)
				    bullet[j].alive = 0;
				}
			      else
				{
				  /* Didn't do anything? Ricochet!: */
				  
				  if (enemy[i].type != BAD_STEAMER)
				    {
				      bullet[j].dead = 1;
				      bullet[j].ym = -8;
				      if (bullet[j].xm < 0)
				        bullet[j].xm = 8;
				      else
				        bullet[j].xm = -8;
				      playsound(SND_BUMP);
                                    }
				}
			    }
			  
			  
			  if (bullet[j].type == WPN_ICE)
			    {
			      /* Ice freezes enemy: */
			      
			      if (enemy[i].frozen == 0 &&
				  enemy[i].type != BAD_SNAKE)
				{
				  enemy[i].frozen = 300;
				  playsound(SND_FREEZE);
				}
			      
			      bullet[j].alive = 0;
			    }
			  else
			    {
			      /* If frozen, non-ice unfreezes! */
			      
			      if (enemy[i].frozen > 0)
				{
				  for (k = 0; k < 4; k++)
				    addice_chunk(enemy[i].x, enemy[i].y);
				  
				  enemy[i].frozen = 0;
				}
			    }
			  
			  
			  /* No more health?  Kill enemy: */
			  
			  if (enemy[i].health <= 0)
			    {
			      enemy[i].health = 0;
			      killenemy(i);
			      
			      if (randnum(CHANCE_OF_UP) == 0)
				addup(enemy[i].x, enemy[i].y, UP_RANDOM, -1);
			      
			      if (enemy[i].type >= BOSS1)
				{
				  addexplosion(enemy[i].x + 32, enemy[i].y, 0);
				  addexplosion(enemy[i].x - 32, enemy[i].y, 0);
				  addexplosion(enemy[i].x, enemy[i].y + 32, 0);
				  addexplosion(enemy[i].x, enemy[i].y - 32, 0);
				  
				  if (enemy[i].type == BOSS1)
				    {
				      /* Freeze-Man explodes into ice: */

				      for (k = 0; k < 4; k++)
					addice_chunk(enemy[i].x, enemy[i].y);
				    }
				  else if (enemy[i].type == BOSS2)
				    {
				      /* The Mummy explodes into wraps: */
				      
				      addbullet(enemy[i].x, enemy[i].y,
						WPN_WRAPS,
						OWN_PLAYER, 8, 0);
				      addbullet(enemy[i].x, enemy[i].y,
						WPN_WRAPS,
						OWN_PLAYER, -8, 0);
				      addbullet(enemy[i].x, enemy[i].y,
						WPN_WRAPS,
						OWN_PLAYER, 0, 8);
				      addbullet(enemy[i].x, enemy[i].y,
						WPN_WRAPS,
						OWN_PLAYER, 0, -8);
				    }	
				  else if (enemy[i].type == BOSS3)
				    {
				      /* Chronoman explodes into springs: */

				      addbullet(enemy[i].x, enemy[i].y,
						WPN_SPRING,
						OWN_PLAYER, -8, -4);
				      addbullet(enemy[i].x, enemy[i].y,
						WPN_SPRING,
						OWN_PLAYER, -8, -8);
				      addbullet(enemy[i].x, enemy[i].y,
						WPN_SPRING,
						OWN_PLAYER, 8, -4);
				      addbullet(enemy[i].x, enemy[i].y,
						WPN_SPRING,
						OWN_PLAYER, 8, -8);
				    }
				  else if (enemy[i].type == BOSS4)
				    {
				      /* Rockman explodes into rocks: */

				      addbullet(enemy[i].x, enemy[i].y,
						WPN_ROCK,
						OWN_PLAYER, 8, 0);
				      addbullet(enemy[i].x, enemy[i].y,
						WPN_ROCK,
						OWN_PLAYER, -8, 0);
				      addbullet(enemy[i].x, enemy[i].y,
						WPN_ROCK,
						OWN_PLAYER, 0, 8);
				      addbullet(enemy[i].x, enemy[i].y,
						WPN_ROCK,
						OWN_PLAYER, 0, -8);
				    }
				}
			    }
			}
		    }
		}
	      
	      
	      /* Check for collision with player! */
	      
	      if (player.x + 31 >= enemy[i].x &&
		  player.x <= enemy[i].x + 31 &&
		  player.y + 31 >= enemy[i].y &&
		  player.y <= enemy[i].y + 31 &&
		  player.ouch_timer == 0 &&
		  player.die_timer == 0 &&
		  (enemy[i].type != BAD_SNAKE ||
		   enemy[i].ym > 5))
		{
		  if (enemy[i].frozen == 1 ||
		      (enemy[i].type == BAD_MINECART &&
		       player.y < enemy[i].y))
		    {
		      /* If frozen or a minecart, act like a block! */
		      
		      if (enemy[i].type == BAD_MINECART &&
			  player.minecart_in == -1)
			{
			  if (player.y <= enemy[i].y)
			    {
			      player.x = enemy[i].x;
			      player.y = enemy[i].y - 16;
			      player.minecart_in = i;
			      player.minecart_dir = enemy[i].dir;
			    }
			  else
			    {
			      hurtplayer(hurt_values[BAD_MINECART][0]);
			    }
			}
		      else
			player.y = enemy[i].y - 32;
		    }
		  else
		    {
		      /* Eat some health: */
		      
		      if (hurt_values[enemy[i].type][enemy[i].mode] != 0)
			hurtplayer(hurt_values[enemy[i].type][enemy[i].mode]);
		    }
		}
	    }
	}
      
      
      /* Get hurt by certain foreground objects: */

      if (cactus(player.x, player.y) != 0 ||
	  cactus(player.x, player.y + 32) != 0)
	hurtplayer(1);


      /* Determine if level was beaten: */
      
      if (page[scrn].bosspage == 1 && num_enemies == 0 && end_of_level == 0)
	{
	  levels_won[level - 1] = 1;
	  end_of_level = 1;
	  end_timer = 128;
	  
	  for (i = 0; i < MAX_BULLETS; i++)
	    {
	      if (bullet[i].alive && bullet[i].owner != OWN_PLAYER)
		{
		  addexplosion(bullet[i].x, bullet[i].y, 1);
		  bullet[i].alive = 0;
		}
	    }
	}
      
      if (page[scrn].bosspage == -1 && num_enemies == 0)
	{
	  page[scrn].bosspage = 0;
	  
	  for (i = 1; i < 7; i++)
	    {
	      addexplosion(32 * 9, 32 * i, 0);
	      page[scrn].background[i][9] = '.';
	      applyshape(page[scrn].background[i][9], 9, i);
	    }
	}
      
      
      /* Handle bullets: */
      
      num_player_bullets = 0;
      
      for (i = 0; i < MAX_BULLETS; i++)
	{
	  if (bullet[i].alive == 1)
	    {
	      bullet[i].x = bullet[i].x + bullet[i].xm;
	      bullet[i].y = bullet[i].y + bullet[i].ym;
	      
	      if (bullet[i].owner == OWN_PLAYER)
		num_player_bullets++;
	      
	      
	      if (bullet[i].dead == 0)
		{
		  if (bullet[i].type == WPN_SPRING ||
		      (bullet[i].type == WPN_ROCK &&
		       bullet[i].owner == OWN_ENEMY))
		    {
		      /* Springs and enemy rocks bounce: */
		      
		      if (solid(bullet[i].x, bullet[i].y, -1))
			bullet[i].xm = -bullet[i].xm;
		      
		      bullet[i].ym = bullet[i].ym + gravity;
		      
		      if (bullet[i].ym > 16)
			bullet[i].ym = 16;
		      
		      if (solid(bullet[i].x, bullet[i].y + 32, -1))
			{
			  /* Springs bounce high, rocks bounce low: */
			  
			  if (bullet[i].type == WPN_SPRING)
			    {
			      bullet[i].ym = -(bullet[i].ym * 4) / 5;
			      playsound(SND_SPRING);
			    }
			  else if (bullet[i].type == WPN_ROCK)
			    {
			      bullet[i].ym = -(bullet[i].ym * 2) / 3;
			      
			      
			      /* When enemy's rock first bounces,
				 bounce towards player: */
			  
			      if (bullet[i].xm == 0 &&
			          bullet[i].owner == OWN_ENEMY)
			        {
			          if (bullet[i].x > player.x)
				    bullet[i].xm = -4;
			          else
			            bullet[i].xm = 4;
			        }
			  
			  
			      /* When rocks bounce, shake the screen: */
			  
			      if (bullet[i].ym != 0)
			        {
			          rock_shake = abs(bullet[i].ym);
			      
			          playsound(SND_ROCK);
			        }
                            }
			}
		    }
		}
	      
	      
	      /* If an enemy bullet, check for collision with player: */
	      
	      if (bullet[i].owner == OWN_ENEMY && player.die_timer == 0)
		{
		  if (bullet[i].x + 16 >= player.x &&
		      bullet[i].x <= player.x + 16 &&
		      bullet[i].y + 16 >= player.y &&
		      bullet[i].y <= player.y + 16)
		    {
		      bullet[i].alive = 0;
		      
		      if (bullet[i].type == WPN_NORMAL ||
			  bullet[i].type == WPN_ROCK)
			{
			  /* Hurt you: */
			  
			  hurtplayer(2);
			}
		      else if (bullet[i].type == WPN_ICE)
			{
			  /* Freeze you: */
			  
			  player.frozen = 100;
			  playsound(SND_FREEZE);
			}
		      else if (bullet[i].type == WPN_WRAPS)
			{
			  /* Throw you back: */
			  
			  hurtplayer(1);
			  
			  if (bullet[i].xm != 0)
			    {
			      while (solid(player.x, player.y, -1) == 0 &&
				     player.x >= 32 && player.x <= WIDTH - 64)
				{
				  player.x = player.x + bullet[i].xm;
				}
			    }
			}
		      else if (bullet[i].type == WPN_BITS ||
			       bullet[i].type == WPN_SPRING)
			{
			  /* Hurt you: */
			  
			  hurtplayer(1);
			}
		    }
		}
	      
	      
	      /* If offscreen, remove: */
	      
	      if (bullet[i].x < 0 || bullet[i].x > WIDTH ||
		  bullet[i].y < 0 || bullet[i].y > HEIGHT)
		bullet[i].alive = 0;
	      
	      
	      /* If old, remove: */
	      
	      if (bullet[i].type == WPN_ROCK || bullet[i].type == WPN_SPRING)
		bullet[i].timer--;
	      
	      if (bullet[i].timer <= 0)
		bullet[i].alive = 0;
	      
	      
	      /* Destroy broken bits: */
	      
	      x = bullet[i].x / 32;
	      y = bullet[i].y / 32;
	      
	      c = page[scrn].background[y][x];
	      
	      if (c == '+' || c == '6')
		{
		  /* Change shape: */
		  
		  if (c == '+')
		    c = '.';
		  else if (c == '6')
		    c = 'b';
		  
		  page[scrn].background[y][x] = c;
		  
		  applyshape(c, x, y);
		  
		  
		  /* Explode / remove bullet: */
		  
		  addexplosion(x * 32, y * 32, 1);
		  
		  bullet[i].alive = 0;
		}
	    }
	}
      
      
      /* Increment fired-at counter: */
      
      player_fired_at++;
      if (player_fired_at > 999)
	player_fired_at = 999;
      
      
      /* Handle rock shaking: */
      
      if (rock_shake > 0)
	rock_shake--;
      
      
      /* Handle flicker: */
      
      if (page[scrn].flickery == 1)
	{
	  flick_time--;
	  
	  if (flick_time <= 0)
	    {
	      flick_on = 1 - flick_on;
	      
	      flick_time = randnum(50) + 50;
	    }
	}
      
      
      /* UPDATE SCREEN: */
      
      /* Erase window: */
      
      if (ttoggle == 0 && toggle == 0)
	{
	  frame = (frame + 1) % 3;
	}
      

      if (page[scrn].dark == 0 && (page[scrn].flickery == 0 ||
				   flick_on == 1))
	{
	  XCopyArea(display, masterbkg[frame], backbuffer, whitegc,
		    0, 0, WIDTH, HEIGHT,
		    0, randnum(rock_shake + 1));
	}
      else
	{
	  XFillRectangle(display, backbuffer, blackgc,
			 0, 0, WIDTH, HEIGHT);
	  
	  
	  if (player.die_timer == 0)
	    {
	      if (player.dir == DIR_RIGHT)
		i = 16;
	      else
		i = -16;
	      
	      XSetClipOrigin(display, dark_gc,
			     player.x - 32 + i, player.y - 32);
	      
	      XCopyArea(display, masterbkg[frame], backbuffer, dark_gc,
			0, 0, WIDTH, HEIGHT,
			0, randnum(rock_shake + 1));
	    }
	}
      
      
      /* Draw player: */
      
      if (player.die_timer == 0)
	{
	  if (player.ouch_timer == 0 || ttoggle == 0)
	    {
	      if (player.ouch_timer > 10 || player.ym != 0 ||
		  player.frozen > 0)
		{
		  /* Draw fall shape if frozen, hurt or falling: */
		  
		  hatx = 9;
		  haty = 0;

		  if (player.dir == DIR_LEFT)
		    c = OBJ_MAN_LEFT_HURT;
		  else if (player.dir == DIR_RIGHT)
		    c = OBJ_MAN_RIGHT_HURT;
		}
	      else if (player.jump == 1)
		{
		  /* Draw jump if jumping: */
		  
		  hatx = 0;
		  haty = 4;
		  
		  if (player.dir == DIR_LEFT)
		    c = OBJ_MAN_LEFT_JUMP;
		  else if (player.dir == DIR_RIGHT)
		    c = OBJ_MAN_RIGHT_JUMP;
		}
	      else
		{
		  /* Draw normal guy if walking or standing: */
		  
		  hatx = 1;
		  haty = 6;
		  
		  if (player.dir == DIR_LEFT)
		    {
		      if (player.walk_count == 0 || player.walk_count == 2)
			{
			  if (player.blink_timer == 0)
			    c = OBJ_MAN_LEFT_STAND;
			  else
			    c = OBJ_MAN_LEFT_STAND_BLINK;
			}
		      else if (player.walk_count == 1)
			c = OBJ_MAN_LEFT_WALK1;
		      else if (player.walk_count == 3)
			c = OBJ_MAN_LEFT_WALK2;
		    }
		  else if (player.dir == DIR_RIGHT)
		    {
		      if (player.walk_count == 0 || player.walk_count == 2)
			{
			  if (player.blink_timer == 0)
			    c = OBJ_MAN_RIGHT_STAND;
			  else
			    c = OBJ_MAN_RIGHT_STAND_BLINK;
			}
		      else if (player.walk_count == 1)
			c = OBJ_MAN_RIGHT_WALK1;
		      else if (player.walk_count == 3)
			c = OBJ_MAN_RIGHT_WALK2;
		    }
		}
	      	

	      /* Do warp-in effect: */
	      
	      if (player.warp_in_timer > 0)
		{
		  for (i = 0; i < player.warp_in_timer; i++)
		    drawobject(old_player_x[i], old_player_y[i], c,
			       backbuffer);
		  
		  for (i = 0; i < player.warp_in_timer - 1; i++)
		    {
		      old_player_x[i] = old_player_x[i + 1];
		      old_player_y[i] = old_player_y[i + 1];
		    }
		  
		  old_player_x[player.warp_in_timer - 1] = player.ox;
		  old_player_y[player.warp_in_timer - 1] = player.oy;
		  
		  if (ttoggle == 0 && toggle == 0)
		    player.warp_in_timer--;
		}

	      
	      drawobject(player.x, player.y, c, backbuffer);
	      
	      if (page[scrn].hat != -1)
		{
		  if (player.dir == DIR_LEFT)
		    {
		      drawobject(player.x + hatx, player.y + haty,
				 page[scrn].hat, backbuffer);
		    }
		  else if (player.dir == DIR_RIGHT)
		    {
		      drawobject(player.x - hatx, player.y + haty,
				 page[scrn].hat + 1, backbuffer);
		    }
		}
	    }
	  else if (player.ouch_timer > 10)
	    drawobject(player.x, player.y, OBJ_ZAP, backbuffer);
	  
	  
	  /* Draw weapon effects: */
	  
	  if (player.frozen > 0)
	    drawobject(player.x, player.y, OBJ_COVER_ICE, backbuffer);
	}
      
      
      /* Draw explosions: */
      
      for (i = 0; i < MAX_EXPLOSIONS; i++)
	{
	  if (explosion[i].alive == 1)
	    {
	      drawobject(explosion[i].x, explosion[i].y,
			 (OBJ_EXPLOSION0 +
			  (explosion[i].time / EXP_TIME_EXTENDER)),
			 backbuffer);
	    }
	}


      /* Draw up's: */
      
      for (i = 0; i < MAX_UPS; i++)
	{
	  if (up[i].alive != 0)
	    {
	      if (up[i].type == UP_BANANA_SMALL)
		drawobject(up[i].x, up[i].y, OBJ_UP_BANANA_SMALL, backbuffer);
	      else if (up[i].type == UP_BANANA_BIG)
		drawobject(up[i].x, up[i].y, OBJ_UP_BANANA_BIG, backbuffer);
	      if (up[i].type == UP_BOLT_SMALL)
		drawobject(up[i].x, up[i].y, OBJ_UP_BOLT_SMALL, backbuffer);
	      else if (up[i].type == UP_BOLT_BIG)
		drawobject(up[i].x, up[i].y, OBJ_UP_BOLT_BIG, backbuffer);
	      else if (up[i].type == UP_ONEUP)
		drawobject(up[i].x, up[i].y, OBJ_UP_ONEUP, backbuffer);
	      else if (up[i].type == UP_COIN)
		drawobject(up[i].x, up[i].y, OBJ_UP_COIN, backbuffer);
	    }
	}

      
      /* Draw air bubbles: */
      
      for (i = 0; i < MAX_AIR_BUBBLES; i++)
	{
	  if (air_bubble[i].alive == 1)
	    {
	      drawobject(air_bubble[i].x, air_bubble[i].y, OBJ_AIR_BUBBLE,
			 backbuffer);
	    }
	}
      

      /* Draw platforms: */
      
      for (i = 0; i < MAX_PLATFORMS; i++)
	{
	  if (platform[i].alive == 1)
	    {
	      drawobject(platform[i].x, platform[i].y, OBJ_PLATFORM,
			 backbuffer);
	      drawobject(platform[i].x + 32, platform[i].y, OBJ_PLATFORM,
			 backbuffer);
	    }
	}
      
      
      /* Draw drips: */
      
      for (i = 0; i < MAX_DRIPS; i++)
	{
	  if (drip[i].alive == 1)
	    {
	      drawobject(drip[i].x, drip[i].y, OBJ_DRIP,
			 backbuffer);
	    }
	}
      
      
      /* Draw dusts: */
      
      for (i = 0; i < MAX_DUSTS; i++)
	{
	  if (dust[i].alive == 1)
	    {
	      drawobject(dust[i].x, dust[i].y, OBJ_DUST_1 - (dust[i].time / 5),
			 backbuffer);
	    }
	}
      
      
      /* Draw ice_chunks: */
      
      for (i = 0; i < MAX_ICE_CHUNKS; i++)
	{
	  if (ice_chunk[i].alive == 1)
	    {
	      drawobject(ice_chunk[i].x, ice_chunk[i].y, OBJ_ICE_CHUNK,
			 backbuffer);
	    }
	}
      
      
      /* Draw sparks: */
      
      for (i = 0; i < MAX_SPARKS; i++)
	{
	  if (spark[i].alive == 1)
	    XDrawLine(display, backbuffer, yellow2gc,
		      spark[i].x, spark[i].y,
		      spark[i].x + spark[i].xm, spark[i].y + spark[i].ym);
	}
      
      
      /* Draw note letters: */
      
      for (i = 0; i < MAX_NOTE_LETTERS; i++)
	{
	  if (note_letter[i].alive == 1)
	    {
	      sprintf(tempstr, " %c ", note_letter[i].c);
	      
	      drawtext(display, backbuffer, whitegc,
		       note_letter[i].x, note_letter[i].y,
		       tempstr);
	    }
	}
      
      
      /* Draw snowflakes: */
      
      for (i = 0; i < MAX_SNOWFLAKES; i++)
	{
	  if (snowflake[i].alive == 1)
	    {
	      drawobject(snowflake[i].x, snowflake[i].y, OBJ_SNOWFLAKE,
			 backbuffer);
	    }
	}
      
      
      /* Draw splashes: */
      
      for (i = 0; i < MAX_SPLASHES; i++)
	{
	  if (splash[i].alive == 1)
	    {
	      drawobject(splash[i].x, splash[i].y,
			 OBJ_SPLASH0 + splash[i].time,
			 backbuffer);
	    }
	}
      
      
      /* Draw enemies: */
      
      for (i = 0; i < MAX_ENEMIES; i++)
	{
	  if (enemy[i].alive == 1)
	    {
	      /* Toggles used for animation should be dependant on whether
		 you're frozen or not: */
	      
	      if (enemy[i].frozen == 0)
		{
		  toggle2 = toggle;
		  ttoggle2 = ttoggle;
		}
	      else
		{
		  toggle2 = 0;
		  ttoggle2 = 0;
		}
	      
	      
	      if (enemy[i].type == BAD_PENGUIN)
		{
		  /* Penguin */
		  
		  if (enemy[i].dir == DIR_LEFT)
		    c = OBJ_BAD_PENGUIN_LEFT_0 + ttoggle2;
		  else if (enemy[i].dir == DIR_RIGHT)
		    c = OBJ_BAD_PENGUIN_RIGHT_0 + ttoggle2;
		}
	      else if (enemy[i].type == BAD_BOMB)
		{
		  /* Bomb */
		  
		  if (enemy[i].mode == MODE_NORMAL)
		    c = OBJ_BAD_BOMB_WALK_0 + ttoggle2;
		  else if (enemy[i].mode == MODE_SPARK)
		    c = OBJ_BAD_BOMB_SPARK_0 + ttoggle2;
		}
	      else if (enemy[i].type == BAD_GEAR)
		{
		  /* Gear */
		  
		  if (frame == 0)
		    c = OBJ_BAD_GEAR_0;
		  else
		    c = OBJ_BAD_GEAR_1;
		}
	      else if (enemy[i].type == BAD_FISH)
		{
		  /* Fish: */
		  
		  if (enemy[i].dir == DIR_LEFT)
		    {
		      if (enemy[i].xm != 0)
			c = OBJ_BAD_FISH_LEFT_0 + ttoggle2;
		      else
			c = OBJ_BAD_FISH_LEFT_0;
		    }
		  else if (enemy[i].dir == DIR_RIGHT)
		    {
		      if (enemy[i].xm != 0)
			c = OBJ_BAD_FISH_RIGHT_0 + ttoggle2;
		      else
			c = OBJ_BAD_FISH_RIGHT_0;
		    }
		}
	      else if (enemy[i].type == BAD_ROBOFLY)
		{
		  /* Robofly: */
		  
		  c = OBJ_BAD_ROBOFLY_0 + toggle2;
		}
	      else if (enemy[i].type == BAD_ICICLE)
		{
		  /* Icicle: */
		  
		  c = OBJ_BAD_ICICLE;
		}
	      else if (enemy[i].type == BAD_SPIDER)
		{
		  /* Spider: */
		  
		  if (enemy[i].mode == MODE_RUSH)
		    c = OBJ_BAD_SPIDER_0 + ttoggle2;
		  else
		    c = OBJ_BAD_SPIDER_0;
		}
	      else if (enemy[i].type == BAD_HELI)
		{
		  /* Heli: */
		  
		  c = OBJ_BAD_HELI_0 + toggle2;
		  
		  
		  /* If carrying a bomb, show it, too: */
		  
		  if (enemy[i].mode == MODE_CARRY)
		    drawobject(enemy[i].x, enemy[i].y + 16,
			       OBJ_BAD_BOMB_WALK_0, backbuffer);
		}
	      else if (enemy[i].type == BAD_MINECART)
		{
		  /* Mine cart: */
		  
		  if (page[scrn].dark == 0 ||
		      (enemy[i].x >= player.x - 32 &&
		       enemy[i].x <= player.x + 64 &&
		       enemy[i].y >= player.y - 32 &&
		       enemy[i].y <= player.y + 64))
		    {
		      /* Only draw when near you in a dark screen! */
			 
		      if (solid(enemy[i].x, enemy[i].y + 32, i))
			c = OBJ_BAD_MINECART;
		      else
			{
			  if (enemy[i].dir == DIR_LEFT)
			    c = OBJ_BAD_MINECART_LEFT;
			  else
			    c = OBJ_BAD_MINECART_RIGHT;
			}
		    }
		  else
		    c = -1;
		}
	      else if (enemy[i].type == BAD_BIRD)
		{
		  /* Bird: */
		  
		  if (enemy[i].y >= player.y)
		    {
		      if (enemy[i].dir == DIR_LEFT)
			c = OBJ_BAD_BIRD_LEFT_0 + ttoggle2;
		      else if (enemy[i].dir == DIR_RIGHT)
			c = OBJ_BAD_BIRD_RIGHT_0 + ttoggle2;
		    }
		  else
		    {
		      if (enemy[i].dir == DIR_LEFT)
			c = OBJ_BAD_BIRD_LEFT_0;
		      else if (enemy[i].dir == DIR_RIGHT)
			c = OBJ_BAD_BIRD_RIGHT_0;
		    }
		}
	      else if (enemy[i].type == BAD_SNAKE)
		{
		  /* Snake: */
		  
		  if (enemy[i].timer < 100)
		    c = OBJ_FORE_METAL;
		  else
		    {
		      amhead = 0;
		      
		      if (i == 0)
			amhead = 1;
		      else if (i > 0)
			{
			  if (enemy[i - 1].alive == 0)
			    amhead = 1;
			}
		      
		      if (amhead == 1)
			c = OBJ_BAD_SNAKE_HEAD;
		      else
			c = OBJ_BAD_SNAKE_BODY;
		    }
		}
	      else if (enemy[i].type == BAD_ROCKER)
		{
		  /* Rocker: */
		  
		  c = -1;
		}
	      else if (enemy[i].type == BAD_STEAMER)
		{
		  /* Steamer: */
		  
		  if (enemy[i].mode == MODE_NORMAL)
		    c = OBJ_BAD_STEAM_START_0 + ((mod_counter / 2) % 2);
		  else
		    c = OBJ_BAD_STEAM_SPRAY_0 + ((mod_counter / 2) % 2);
		}
	      else if (enemy[i].type == BAD_ARMADILLO)
		{
		  /* Armadillo: */
		  
		  if (enemy[i].mode == MODE_NORMAL)
		    {
		      if (enemy[i].dir == DIR_LEFT)
			c = OBJ_BAD_ARMADILLO_LEFT;
		      else if (enemy[i].dir == DIR_RIGHT)
			c = OBJ_BAD_ARMADILLO_RIGHT;
		    }
		  else
		    {
		      if (enemy[i].dir == DIR_LEFT)
			c = OBJ_BAD_ARMADILLO_ROLL_2 - (mod_counter) % 3;
		      else
			c = OBJ_BAD_ARMADILLO_ROLL_0 + (mod_counter) % 3;
		    }
		}
	      else if (enemy[i].type == BAD_ROCKET)
		{	
		  /* Rocket: */
		  
		  if (enemy[i].dir == DIR_LEFT)
		    c = OBJ_BAD_ROCKET_LEFT;
		  else if (enemy[i].dir == DIR_RIGHT)
		    c = OBJ_BAD_ROCKET_RIGHT;
		}
	      else if (enemy[i].type == BAD_TUMBLEWEED)
		{
		  /* Tumbleweed: */
		  
		  c = OBJ_BAD_TUMBLEWEED_0 + (mod_counter / 2) % 3;
		}
	      else if (enemy[i].type == BOSS1)
		{
		  /* Level 1 Boss - Freeze-Man */
		  
		  if (enemy[i].dir == DIR_LEFT)
		    {
		      if (enemy[i].timer < 10)
			c = OBJ_BOSS1_LEFT_1;
		      else
			c = OBJ_BOSS1_LEFT_0;
		    }
		  else if (enemy[i].dir == DIR_RIGHT)
		    {
		      if (enemy[i].timer < 10)
			c = OBJ_BOSS1_RIGHT_1;
		      else
			c = OBJ_BOSS1_RIGHT_0;
		    }
		}
	      else if (enemy[i].type == BOSS2)
		{
		  /* Level 2 Boss - The Mummy */
		  
		  z = (mod_counter / 5) % 4;
		  
		  if (enemy[i].mode == MODE_NORMAL)
		    c = OBJ_BOSS2_CASKET;
		  else if (enemy[i].mode == MODE_RUSH)
		    {
		      if (enemy[i].dir == DIR_LEFT)
			{
			  if (z == 0)
			    c = OBJ_BOSS2_LEFT_0;
			  else if (z == 1 || z == 3)
			    c = OBJ_BOSS2_LEFT_1;
			  else if (z == 2)
			    c = OBJ_BOSS2_LEFT_2;
			}
		      else
			{
			  if (z == 0)
			    c = OBJ_BOSS2_RIGHT_0;
			  else if (z == 1 || z == 3)
			    c = OBJ_BOSS2_RIGHT_1;
			  else if (z == 2)
			    c = OBJ_BOSS2_RIGHT_2;
			}
		    }
		}
	      else if (enemy[i].type == BOSS3)
		{
		  /* Level 3 Boss: Chronoman: */
		  
		  z = (mod_counter / 2) % 4;
		  
		  if (z == 0)
		    c = OBJ_BOSS3_1;
		  else if (z == 1 || z == 3)
		    c = OBJ_BOSS3_0;
		  else
		    c = OBJ_BOSS3_2;
		}
	      else if (enemy[i].type == BOSS4)
		{
		  /* Level 4 Boss: Rockman: */
		  
		  if (enemy[i].timer < 75)
		    z = (mod_counter / 4) % 2;
		  else
		    z = 0;
		  
		  c = OBJ_BOSS4_1 - z;
		}
	      
	      
	      if (c != -1)
		{
		  drawobject(enemy[i].x, enemy[i].y, c, backbuffer);
		  
		  
		  /* Draw ice block if frozen: */
		  
		  if (enemy[i].frozen != 0)
		    drawobject(enemy[i].x, enemy[i].y, OBJ_COVER_ICE,
			       backbuffer);
		}
	    }
	}
      
      
      /* Draw bullets: */
      
      for (i = 0; i < MAX_BULLETS; i++)
	{
	  if (bullet[i].alive == 1)
	    {
	      if (bullet[i].type == WPN_NORMAL)
		c = OBJ_BULLET_NORMAL;
	      else if (bullet[i].type == WPN_ICE)
		c = OBJ_BULLET_ICE;
	      else if (bullet[i].type == WPN_WRAPS)
		c = OBJ_BULLET_WRAPS_0 + ttoggle;
	      else if (bullet[i].type == WPN_SPRING)
		c = OBJ_BULLET_SPRING;
	      else if (bullet[i].type == WPN_ROCK)
		{
		  if (bullet[i].owner == OWN_PLAYER)
		    c = OBJ_BULLET_ROCK;
		  else
		    c = OBJ_BULLET_ROCK_BIG;
		}
	      else if (bullet[i].type == WPN_BITS)
		c = OBJ_BULLET_BITS;
	      
	      drawobject(bullet[i].x, bullet[i].y, c, backbuffer);
	    }
	}
      
      
      /* Handle health meter: */
	 
      if (player.want_health < player.health * 2)
	player.want_health++;
      else if (player.want_health > player.health * 2)
	player.want_health--;
      
      
      /* Draw health meter: */
      
      XFillRectangle(display, backbuffer, blackgc,
		     5, 5, 14, 54);
      
      XFillRectangle(display, backbuffer, red0gc,
		     7, 7, 10, 50);

      XFillRectangle(display, backbuffer, red1gc,
		     9, 7, 6, 50);
      
      XFillRectangle(display, backbuffer, red2gc,
		     11, 7, 2, 50);
      
      XFillRectangle(display, backbuffer, yellow0gc,
		     7, 52 - player.want_health + 5,
		     10, player.want_health);
      
      XFillRectangle(display, backbuffer, yellow1gc,
		     9, 52 - player.want_health + 5,
		     6, player.want_health);
      
      XFillRectangle(display, backbuffer, yellow2gc,
		     11, 52 - player.want_health + 5,
		     2, player.want_health);
      
      XFillRectangle(display, backbuffer, green0gc,
		     7, 52 - (player.health * 2) + 5,
		     10, player.health * 2);
      
      XFillRectangle(display, backbuffer, green1gc,
		     9, 52 - (player.health * 2) + 5,
		     6, player.health * 2);
      
      XFillRectangle(display, backbuffer, green2gc,
		     11, 52 - (player.health * 2) + 5,
		     2, player.health * 2);
      
      
      if (player.selected_weapon != WPN_NORMAL)
	{
	  /* Draw weapon meter: */
	  
	  XFillRectangle(display, backbuffer, blackgc,
			 20, 5, 14, 54);
      
	  XFillRectangle(display, backbuffer, red0gc,
			 22, 7, 10, 50);
      
	  XFillRectangle(display, backbuffer, red1gc,
			 24, 7, 6, 50);
      
	  XFillRectangle(display, backbuffer, red2gc,
			 26, 7, 2, 50);
      
	  XFillRectangle(display, backbuffer, green0gc,
			 22, (52 -
			      (player.weapon[player.selected_weapon] * 2) +
			      5),
			 10, (player.weapon[player.selected_weapon] * 2));
	  
	  XFillRectangle(display, backbuffer, green1gc,
			 24, (52 -
			      (player.weapon[player.selected_weapon] * 2) +
			      5),
			 6, (player.weapon[player.selected_weapon] * 2));
	  
	  XFillRectangle(display, backbuffer, green2gc,
			 26, (52 -
			      (player.weapon[player.selected_weapon] * 2) +
			      5),
			 2, (player.weapon[player.selected_weapon] * 2));
	  
	  
	  /* Draw the weapon over the weapon meter: */
	  
	  drawobject(10, 20, weapon_images[player.selected_weapon],
		     backbuffer);
	}
      
      
      /* Draw boss's health meter: */
      
      if (page[scrn].bosspage != 0)
	{
	  XFillRectangle(display, backbuffer, blackgc,
			 40, 5, 14, 54);
	  
	  XFillRectangle(display, backbuffer, red0gc,
			 42, 7, 10, 50);
	  
	  XFillRectangle(display, backbuffer, red1gc,
			 44, 7, 6, 50);
	  
	  XFillRectangle(display, backbuffer, red2gc,
			 46, 7, 2, 50);
	  
	  
	  /* (Assume enemy #0 is the boss!) */
	  
	  y = ((enemy[0].health / 4)) * 2;
	  
	  XFillRectangle(display, backbuffer, green0gc,
			 42, 52 - y + 5, 10, y);

	  XFillRectangle(display, backbuffer, green1gc,
			 44, 52 - y + 5, 6, y);

	  XFillRectangle(display, backbuffer, green2gc,
			 46, 52 - y + 5, 2, y);
	}
      
      /* Draw lives: */
      
      XFillRectangle(display, backbuffer, blackgc,
		     WIDTH - 42 + 8, 10, 16, 32);
      
      drawobject(WIDTH - 42, 10, OBJ_UP_ONEUP, backbuffer);
      
      sprintf(tempstr, "%d", player.lives);
      
      drawtext(display, backbuffer, whitegc, WIDTH - 42 + 10, 10 + fh,
	       tempstr);
      
      
      /* Debugging stuff: */
      
#ifdef DEBUG
      sprintf(tempstr, "xspeed:     %d", player.xspeed);
      drawtext(display, backbuffer, whitegc, 0, fh, tempstr);

      sprintf(tempstr, "walk_count: %d", player.walk_count);
      drawtext(display, backbuffer, whitegc, 0, fh * 2, tempstr);

      sprintf(tempstr, "jump:       %d", player.jump);
      drawtext(display, backbuffer, whitegc, 0, fh * 3, tempstr);

      sprintf(tempstr, "ym:         %d", player.ym);
      drawtext(display, backbuffer, whitegc, 0, fh * 4, tempstr);
#endif
      
      
      /* End-of-level sequence: */
      
      if (end_of_level == 1)
	{
	  end_timer--;
	  
	  if (mod_counter % 3 == 0)
	    addexplosion(randnum(WIDTH - 32), randnum(HEIGHT - 32), 0);
	  
	  if (end_timer <= 0)
	    done = True;
	  
	  drawcenteredtext(display, backbuffer, whitegc,
			   0, WIDTH, HEIGHT / 2, " AREA COMPLETE! ",
			   font);
	  
	  sprintf(tempstr, "You gained: %s", weapon_names[level]);
	  
	  drawcenteredtext(display, backbuffer, whitegc,
			   0, WIDTH, HEIGHT / 2 + fh, tempstr, font);
	  
	  player.weapon[level] = 25;
	}

      
      /* Game over sequence: */
      
      if (gameover == 1)
	{
	  drawcenteredtext(display, backbuffer, whitegc,
			   0, WIDTH, HEIGHT / 2, " G A M E   O V E R ! ",
			   font);
	}
      
      
      /* Update window: */
      
      if (water(player.x, player.y) == 0 || player.health == 0)
	{
	  XCopyArea(display, backbuffer, window, whitegc, 0, 0, WIDTH, HEIGHT,
		    0, 0);
	}
      else
	{
	  /* Sine-wave effect: */
	  
	  for (i = 0; i < HEIGHT; i = i + 4)
	    {
	      XCopyArea(display, backbuffer, window, whitegc,
			0, i,
			WIDTH, 4,
			sin(M_PI * (i + mod_counter) * 4 / 180.0) * 4.0 +
			sin(M_PI * (i + mod_counter) * 10 / 180.0) * 2.0,
			i + (sin(M_PI * (i + mod_counter * 2) * 2 / 180.0)
			     * 4.0));
	    }
	}
      
      
      /* Play music: */
      
#ifdef MUSIC
      if (no_music == 0)
	{
	  if (!Mix_PlayingMusic())
	    {
	      if (page[scrn].bosspage == 0)
		Mix_PlayMusic(songs[level], 1); 
	      else
		Mix_PlayMusic(songs[MUSIC_BOSS], 1);
	    }
	}
#endif
  

      /* Keep framerate exact: */
      
      gettimeofday(&now, NULL);
      
      time_padding = FRAMERATE - ((now.tv_sec - then.tv_sec) * 1000000 +
				  (now.tv_usec - then.tv_usec));
      
      if (time_padding > 0)
	usleep(time_padding);
      else
	{
	  XSync(display, 0);
	  XFlush(display);
	}
    }
  while (done == False);


  /* Stop music: */
  
#ifdef MUSIC
  if (no_music == 0)
    Mix_HaltMusic();
#endif
}


/* Program set-up (check usage, load data, etc.): */

void setup(int argc, char * argv[])
{
#ifdef SOUND
  SDL_version sdlver;
#endif


  if (argc == 2)
    {
      if (strcmp(argv[1], "--version") == 0 ||
	  strcmp(argv[1], "-v") == 0)
	{
	  /* Check for "--version": */
	  
	  printf("\nBoboBot\n");
	  printf("Version: %s\n\n", VERSION);
	  
	  printf("by Bill Kendrick, Marianne Waage & Melissa Hardenbrook\n");
	  printf("bobobot@newbreedsoftware.com\n");
	  printf("http://www.newbreedsoftware.com/bobobot/\n\n");

#ifdef SOUND
	  SDL_VERSION(&sdlver);
	  printf("SDL version %d.%d.%d\n", sdlver.major,
		 sdlver.minor, sdlver.patch);
#ifdef MUSIC
	  printf("Music support installed.\n");
#endif
	  printf("\n");
#endif
#ifdef JOYSTICK
	  printf("Joystick driver version %d.%d.%d\n\n",
		 (JS_VERSION & 0xFF0000) >> 16,
		 (JS_VERSION & 0x00FF00) >> 8,
		 JS_VERSION & 0x0000FF);
#endif
	  exit(0);
	}
      else if (strcmp(argv[1], "--help") == 0 ||
	       strcmp(argv[1], "-h") == 0)
	{
	  /* Check for "-help": */
	  
	  fprintf(stderr, "\n");
	  fprintf(stderr, "Usage: %s [--nomusic | --nosound] [server]",
		  argv[0]);
	  fprintf(stderr, " | --help | --version\n");
	  fprintf(stderr, "\n");
	  fprintf(stderr, "Controls:\n");
	  fprintf(stderr, "  [Arrow] - Move\n");
	  fprintf(stderr, "  [Space] - Jump\n");
	  fprintf(stderr, "    [Z]   - Fire\n");
          fprintf(stderr, "   [TAB]  - Pause / Weapon select\n");
	  fprintf(stderr, "    [Q]   - Quit\n");
	  fprintf(stderr, "\n");
	  
	  exit(0);
	}
    }


  /* No music? */
  
  no_music = 0;
  no_sound = 0;
  if (argc > 1)
    {
      if (strcmp(argv[1], "--nomusic") == 0)
	no_music = 1;
      else if (strcmp(argv[1], "--nosound") == 0)
	no_sound = 1;
      
      if (argc > 2)
	{
	  if (strcmp(argv[2], "--nomusic") == 0)
	    no_music = 1;
	  else if (strcmp(argv[2], "--nosound") == 0)
	    no_sound = 1;
	}
    }

  
  /* Display: */
  
#ifdef __amigaos__
  /* No X server: the display is the Amiga's own screen. */
  strcpy(server, "amiga");
#else
  if (argc == 1 || argv[argc - 1][0] == '-')
    {
      if (getenv("DISPLAY") != NULL)
	{
	  strcpy(server, getenv("DISPLAY"));
	}
      else
	{
	  fprintf(stderr, "Can't determine your display!\n");
	  fprintf(stderr, "Please specify it on the command line.\n");
	  exit(1);
	}
    }
  else
    {
      strcpy(server, argv[argc - 1]);
    }
#endif
}



/* Connect, make window, and display "please wait" */

void Xsetup_windows()
{
  char title[1024];
  int good_depth, i;
  
  
  /* Connect to display: */
  
  fprintf(stderr, "(Connecting to: %s...", server);
  
  display = ConnectToServer(server, &screen, &root);
  if (display == NULL)
    {
      perror(server);
      exit(1);
    }
  
  
  /* Load font: */
  
  font = LoadFont(display, "variable", "fixed");
  fh = FontHeight(font);
  
  
  /* Get our primitve colors: */
  
  black = BlackPixel(display, screen);
  white = WhitePixel(display, screen);
  
  
  /* Open window: */
  
  window = OpenWindow(display, root, 0, 0, WIDTH, HEIGHT,
		      CopyFromParent, black,
		      (KeyPressMask | KeyReleaseMask),
		      (Visual *)CopyFromParent);
  
  sprintf(title, "BoboBot");
  SetStandardHints(display, window, "BoboBot", title, 10, 10,
		   WIDTH, HEIGHT);
  
  
  /* Create Primitive GC's: */
  
  whitegc = CreateGC(display, window, white, black);
  
  blackgc = CreateGC(display, window, black, black);
  
  
  /* Get screen depth: */
  
  good_depth = DefaultDepthOfScreen(DefaultScreenOfDisplay(display));
  
  fprintf(stderr, "(Color depth is %dbpp)...",
	  good_depth);
  
  
  /* Create backbuffer: */
  
  backbuffer = XCreatePixmap(display, window, WIDTH, HEIGHT, good_depth);
  
  
  /* Create master background buffer: */
  
  for (i = 0; i < 3; i++)
    masterbkg[i] = XCreatePixmap(display, window, WIDTH, HEIGHT, good_depth);
  
  
  /* Put our window up!: */
  
  XMapWindow(display, window);
  XMapRaised(display, window);
  XSync(display, 0);
  
  fprintf(stderr, "Looks good!)\n");
}


/* Setup the application: */

void Xsetup(void)
{
  int i, temp_depth, xhot, yhot, ret, percent;
  XGCValues xgcvalues;
  char filename[1024];

  
  /* Set up visual: */
  

  if (SetUpVisual(display, screen, &temp_visual, &temp_depth))
    {
      if (!SetUpColormap(display, screen, window,
			 temp_visual, &colormap))
        {
          fprintf(stderr, "Could not create a colormap!\n");
	  exit(1);
        }
    }
  else
    {
      fprintf(stderr, "Could not find a PseudoColor visual!\n");
    }
  
  
  /* Grab health meter colors: */

  green0gc = CreateGC(display, window,
		      MyAllocNamedColor(display, colormap,
					"dark green", white, 1),
		      black);
  
  green1gc = CreateGC(display, window,
		      MyAllocNamedColor(display, colormap,
					"green3", white, 1),
		      black);
  
  green2gc = CreateGC(display, window,
		      MyAllocNamedColor(display, colormap,
					"green", white, 1),
		      black);
  
  
  red0gc = CreateGC(display, window,
		    MyAllocNamedColor(display, colormap,
				      "dark red", black, 1),
		    black);
  
  red1gc = CreateGC(display, window,
		    MyAllocNamedColor(display, colormap,
				      "red3", black, 1),
		    black);
  
  red2gc = CreateGC(display, window,
		    MyAllocNamedColor(display, colormap,
				      "red", black, 1),
		    black);
  
  
  yellow0gc = CreateGC(display, window,
		       MyAllocNamedColor(display, colormap,
					 "yellow4", white, 1),
		       black);
  
  yellow1gc = CreateGC(display, window,
		       MyAllocNamedColor(display, colormap,
					 "yellow3", white, 1),
		       black);
  
  yellow2gc = CreateGC(display, window,
		       MyAllocNamedColor(display, colormap,
					 "yellow", white, 1),
		       black);
  
  
  /* Thick red GC: */
  
  thickredgc = CreateGC(display, window,
			MyAllocNamedColor(display, colormap,
					  "red", black, 1),
			black);
  
  xgcvalues.line_width = 10;
  XChangeGC(display, thickredgc, GCLineWidth, &xgcvalues);
  
  
  /* Put us up!: */
  
  XMapWindow(display, window);
  XMapRaised(display, window);
  XSync(display, 0);
  
  
  /* Load object pixmaps: */
  
  for (i = 0; i < NUM_OBJECTS; i++)
    {
      /* Draw "loading..." */
      
      if (i % 10 == 0)
	drawcenteredtext(display, window, whitegc,
			 0, WIDTH, HEIGHT / 2 - 10,
			 "Loading...", font);
      

      if (i % 10 == 0)
	drawcenteredtext(display, window, whitegc,
			 0, WIDTH, fh * 2,
			 "BoboBot", font);
      
      
      /* Draw slider: */
      
      percent = ((WIDTH - 20) * i) / NUM_OBJECTS;
      
      XFillRectangle(display, window, green0gc,
		     10, HEIGHT / 2 - 5,
		     percent, 10);
      
      XFillRectangle(display, window, green1gc,
		     10, HEIGHT / 2 - 5 + 2,
		     percent, 6);
      
      XFillRectangle(display, window, green2gc,
		     10, HEIGHT / 2 - 5 + 4,
		     percent, 2);
      
      XFillRectangle(display, window, red0gc,
		     percent + 10, HEIGHT / 2 - 5,
		     WIDTH - 20 - percent, 10);
      
      XFillRectangle(display, window, red1gc,
		     percent + 10, HEIGHT / 2 - 5 + 2,
		     WIDTH - 20 - percent, 6);
      
      XFillRectangle(display, window, red2gc,
		     percent + 10, HEIGHT / 2 - 5 + 4,
		     WIDTH - 20 - percent, 2);
      
      loadobject(i);
    }
  
  
  /* Load dark XBM: */
  
  sprintf(filename, "%s" DATA_SEP "images/dark.xbm", DATA);
  
  ret = XReadBitmapFile(display, window, filename,
			&dark_width, &dark_height,
			&dark_pix, &xhot, &yhot);
  
  if (ret != BitmapSuccess)
    {
      perror(filename);
      exit(1);
    }
  
  dark_gc = CreateGC(display, window, white, white);
  
  XSetClipMask(display, dark_gc, dark_pix);
}


/* Allocate a color (or white): */

unsigned long MyAllocNamedColor(Display *display, Colormap colormap,
                                char* colorname, unsigned long default_color,
                                int has_color)
{
  if (has_color == 0)
    return(default_color);
  else
    return(AllocNamedColor(display, colormap, colorname, default_color));
}


/* Add one explosion to the screen */

void addexplosion(int x, int y, int play_sound)
{
  int i, found, min_time;
  
  min_time = 999;
  found = -1;
  
  
  /* Find a free slot, or the slot with the least time remaining: */
  
  for (i = 0; i < MAX_EXPLOSIONS; i++)
    {
      if (explosion[i].alive == 0)
	{
	  min_time = 0;
	  found = i;
	}
      
      if (explosion[i].time < min_time)
	{
	  min_time = explosion[i].time; 
	  found = i;
	}
    }
  
  
  /* Activate this explosion slot: */
  
  explosion[found].alive = 1;
  explosion[found].time = MAX_EXPLOSION_TIME * EXP_TIME_EXTENDER +
    (EXP_TIME_EXTENDER - 1);
  explosion[found].x = x;
  explosion[found].y = y;
  
  
  /* Add a bubble if underwater: */
  
  if (water(x, y))
    addair_bubble(x, y);
  
  if (play_sound == 1)
    playsound(SND_EXPLODE);
}


/* Add one air bubble to the screen */

void addair_bubble(int x, int y)
{
  int i, found;
  
  found = -1;
  
  
  /* Find a free slot, or the slot with the least time remaining: */
  
  for (i = 0; i < MAX_AIR_BUBBLES; i++)
    {
      if (air_bubble[i].alive == 0)
	found = i;
    }
  
  
  /* Activate this air bubble slot: */
  
  if (found != -1)
    {
      air_bubble[found].alive = 1;
      air_bubble[found].x = x;
      air_bubble[found].y = y;
      air_bubble[found].xm = 0;
      air_bubble[found].xmm = randnum(4) - 2;
    }
}


/* Add one drip to the screen */

void adddrip(int x, int y)
{
  int i, found;
  
  found = -1;
  
  
  /* Find a free slot, or the slot with the least time remaining: */
  
  for (i = 0; i < MAX_DRIPS; i++)
    {
      if (drip[i].alive == 0)
	found = i;
    }
  
  
  /* Activate this drip slot: */
  
  if (found != -1)
    {
      drip[found].alive = 1;
      drip[found].x = x;
      drip[found].y = y;
      drip[found].ym = 0;
    }
  
  playsound(SND_DRIP);
}


/* Add one dust to the screen */

void adddust(int x, int y)
{
  int i, found;
  
  found = -1;
  
  
  /* Find a free slot, or the slot with the least time remaining: */
  
  for (i = 0; i < MAX_DUSTS; i++)
    {
      if (dust[i].alive == 0)
	found = i;
    }
  
  
  /* Activate this dust slot: */
  
  if (found != -1)
    {
      dust[found].alive = 1;
      dust[found].x = x;
      dust[found].y = y;
      dust[found].time = 10;
    }
}


/* Add one ice_chunk to the screen */

void addice_chunk(int x, int y)
{
  int i, found;
  
  found = -1;
  
  
  /* Find a free slot, or the slot with the least time remaining: */
  
  for (i = 0; i < MAX_ICE_CHUNKS; i++)
    {
      if (ice_chunk[i].alive == 0)
	found = i;
    }
  
  
  /* Activate this ice_chunk slot: */
  
  if (found != -1)
    {
      ice_chunk[found].alive = 1;
      ice_chunk[found].x = x;
      ice_chunk[found].y = y;
      ice_chunk[found].xm = randnum(11) - 5;
      ice_chunk[found].ym = -(5 + randnum(5));
      ice_chunk[found].time = 20 + randnum(10);
    }
}


/* Add one spark to the screen */

void addspark(int x, int y)
{
  int i, found;
  
  found = -1;
  
  
  /* Find a free slot, or the slot with the least time remaining: */
  
  for (i = 0; i < MAX_SPARKS; i++)
    {
      if (spark[i].alive == 0)
	found = i;
    }
  
  
  /* Activate this spark slot: */
  
  if (found != -1)
    {
      spark[found].alive = 1;
      spark[found].x = x;
      spark[found].y = y;
      spark[found].xm = randnum(5) - 2;
      spark[found].ym = -randnum(3);
      spark[found].time = 5 + randnum(5);
    }
}


/* Add one snowflake to the screen */

void addsnowflake(int x, int y)
{
  int i, found;
  
  found = -1;
  
  
  /* Find a free slot, or the slot with the least time remaining: */
  
  for (i = 0; i < MAX_SNOWFLAKES; i++)
    {
      if (snowflake[i].alive == 0)
	found = i;
    }
  
  
  /* Activate this air bubble slot: */
  
  if (found != -1)
    {
      snowflake[found].alive = 1;
      
      if (x == -1 && y == -1)
	{
	  do
	    {
	      x = randnum(WIDTH - 32);
	    }
	  while (solid(x, 16, -1));
	  
	  snowflake[found].x = x;
	  snowflake[found].y = -32;
	}
      else
	{
	  snowflake[found].x = x;
	  snowflake[found].y = y;
	}
    }
}


/* Add one splash to the screen */

void addsplash(int x, int y)
{
  int i, found;
  
  found = -1;
  
  
  /* Find a free slot, or the slot with the least time remaining: */
  
  for (i = 0; i < MAX_SPLASHES; i++)
    {
      if (splash[i].alive == 0)
	found = i;
    }
  
  
  /* Activate this splash slot: */
  
  if (found != -1)
    {
      splash[found].alive = 1;
      splash[found].time = 0;
      splash[found].x = x;
      splash[found].y = y;
    }
  
  playsound(SND_SPLASH);
}


/* Add one up to the screen */

void addup(int x, int y, int whichup, int whichenemyslot)
{
  int i, found;
  
  found = -1;
  
  
  /* Find a free slot: */
  
  for (i = 0; i < MAX_UPS; i++)
    {
      if (up[i].alive == 0)
	found = i;
    }
  
  
  /* Activate this up slot: */
  
  if (found != -1)
    {
      up[found].alive = 1;
      
      
      /* "Enemy-ups" last forever, others go away after a few seconds: */
      
      if (whichenemyslot == -1)
	up[found].time = 150;
      else
	up[found].time = 0;
      
      if (whichup == UP_RANDOM)
	up[found].type = randnum(NUM_UPS);
      else
	up[found].type = whichup;
      up[found].x = x;
      up[found].y = y;
      up[found].ym = -3;
      up[found].enemyslot = whichenemyslot;
    }
}


/* Add one platform to the screen */

void addplatform(int x, int y, int type)
{
  int i, found;
  
  found = -1;
  
  
  /* Find a free slot: */
  
  for (i = 0; i < MAX_PLATFORMS; i++)
    {
      if (platform[i].alive == 0)
	found = i;
    }
  
  
  /* Activate this up slot: */
  
  if (found != -1)
    {
      platform[found].alive = 1;
      platform[found].type = type;
      platform[found].x = x;
      platform[found].y = y;
      platform[found].ym = 0;
      platform[found].xm = 0;
      platform[found].ymm = 1;
      platform[found].xmm = 1;
      platform[found].mode = MODE_NORMAL;
      platform[found].timer = 0;
    }
}


/* Add one bullet to the screen */

void addbullet(int x, int y, int type, int owner, int xm, int ym)
{
  int i, found;
  
  found = MAX_BULLETS - 1;
  
  
  /* Find a free slot */
  
  for (i = 0; i < MAX_BULLETS; i++)
    {
      if (bullet[i].alive == 0)
	found = i;
    }
  
  
  /* Activate this bullet slot: */
  
  bullet[found].alive = 1;
  bullet[found].dead = 0;
  bullet[found].type = type;
  bullet[found].owner = owner;
  bullet[found].x = x;
  bullet[found].y = y;
  bullet[found].xm = xm;
  bullet[found].ym = ym;
  bullet[found].timer = 100;
}


/* Load a color object: */

void loadobject(int i)
{
  int x, y, red, green, blue, color_pos, width, height, whitespace_count, c,
    threed_which, j;
  char file[512];
  FILE * fi;
  GC color0gc, color1gc;
  XColor xcol;
  XGCValues xgcv;
  unsigned long color_want;
  
  fi = NULL;
  
  sprintf(file, "%s" DATA_SEP "images/%s.ppm", DATA, object_names[i]);
  
  fi = fopen(file, "r");
  if (fi == NULL)
    {
      perror(file);
      exit(1);
    }
  
  
  /* Eat header (size had better match or something's been tampered!) */
  
  width = 32;
  height = 32;
  
  whitespace_count = 0;
  
  do
    {
      c = fgetc(fi);
      
      if (c == '#')
	{
	  /* Comment!  Eat until end of line */
	  
	  do
	    {
	      c = fgetc(fi);
	    }
	  while (c != '\n' && c != '\r' && !feof(fi));
	}
      else if (isspace(c))
	whitespace_count++;
    }
  while (whitespace_count < 4 && !feof(fi));
  
  
  color_clear();
  
  object_pixmaps[i] =
    XCreatePixmap(display, window, width, height, 
                  DefaultDepthOfScreen(DefaultScreenOfDisplay(display)));
  
  XFillRectangle(display, object_pixmaps[i], whitegc, 0, 0,
		 width, height);
  
  object_masks[i] = 
    XCreatePixmap(display, window, width, height, 1);
  
  xgcv.foreground = 0;
  color0gc = XCreateGC(display, object_masks[i], GCForeground,
		       &xgcv);
  
  xgcv.foreground = 1;
  color1gc = XCreateGC(display, object_masks[i], GCForeground,
		       &xgcv);
  
  XFillRectangle(display, object_masks[i], color0gc,
		 0, 0, width, height);
  
  if (!SetUpColormap(display, screen, object_pixmaps[i],
                     temp_visual, &colormap))
    {
      fprintf(stderr, "Error: Could not set up colormap!\n");
      exit(1);
    }
  
  
  /* See if this is one we want to use in 3D: */
  
  threed_which = -1;

  for (j = 0; j < NUM_THREEDS; j++)
    if (threed_icons[j] == i)
      threed_which = j;
  
  
  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
        {
	  red = fgetc(fi);
	  green = fgetc(fi);
	  blue = fgetc(fi);
          
	  color_pos = -1;
	  
          if (red != 255 || green != 255 || blue != 255)
            {
	      red = (red / COLOR_DIVIDE) * COLOR_DIVIDE;
	      green = (green / COLOR_DIVIDE) * COLOR_DIVIDE;
	      blue = (blue / COLOR_DIVIDE) * COLOR_DIVIDE;
	      
              color_pos = color_seen(red, green, blue);
              
              if (color_pos == -1)
                {
                  xcol.red = red * 256;
                  xcol.green = green * 256;
                  xcol.blue = blue * 256;
                  xcol.flags = DoRed | DoGreen | DoBlue;
                  
                  XAllocColor(display, colormap, &xcol);
                  
                  color_pos = color_add(red, green, blue, 
                                        xcol.pixel, i);
                  color_want = xcol.pixel;
                  
                  xgcv.foreground = color_want;
                  colorgc[color_pos] = XCreateGC(display, window,
                                                 GCForeground, &xgcv);
		  
		  if (threed_which != -1)
		    threed_gc[threed_which][color_pos] =
		      XCreateGC(display, backbuffer, GCForeground, &xgcv);
                }
              else
                {
                  color_want = color_list[color_pos].pixel;
                  
                  if (color_list[color_pos].owner != i)
                    {
                      xgcv.foreground = color_want;
                      
                      XFreeGC(display, colorgc[color_pos]);
                      colorgc[color_pos] =
                        XCreateGC(display, object_pixmaps[i],
                                  GCForeground, &xgcv);
                      
                      color_list[color_pos].owner = i;
                    }
                }
              
              XDrawPoint(display, object_pixmaps[i],
			 colorgc[color_pos], x, y);
              XDrawPoint(display, object_masks[i], color1gc, x, y);
            }
	  
	  if (threed_which != -1)
	    {
	      if (red == 0 && green == 0 && blue == 0)
		threed_shape[threed_which][y][x] = -1;
	      else
		threed_shape[threed_which][y][x] = color_pos;
	    }
	}
    }
  
  objgcs[i] = CreateGC(display, window, white, white);
  
  XSetClipMask(display, objgcs[i], object_masks[i]);
  
  fclose(fi);
}


/* Clears seen-colors list: */

void color_clear(void)
{
  num_seen_colors = 0;
}


/* Looks for a color in the seen-colors list: */

int color_seen(unsigned int red, unsigned int green, unsigned int blue)
{
  int i, found;
  
  found = -1;
  
  for (i = 0; i < num_seen_colors; i++)
    {
      if (color_list[i].red == red &&
          color_list[i].green == green &&
          color_list[i].blue == blue)
        found = i;
    }
  
  return found;
}


/* Add a color to the seen-colors list: */

int color_add(unsigned int red, unsigned int green, unsigned int blue,
               unsigned long pixel, int owner)
{
  color_list[num_seen_colors].red = red;
  color_list[num_seen_colors].green = green;
  color_list[num_seen_colors].blue = blue;
  color_list[num_seen_colors].pixel = pixel;
  color_list[num_seen_colors].owner = owner;
  
  num_seen_colors++;
  
  if (num_seen_colors > MAX_COLORS)
    {
      fprintf(stderr, "Too many colors!  Increase COLOR_DIVIDE!\n");
      exit(1);
    }
  
  return(num_seen_colors - 1);
}


/* Reset a level: */

void initlevel(void)
{
  int x, y;
  FILE * fi;
  char filename[512], temp[512];
  
  
  /* Open level file: */
  
  sprintf(filename, "%s" DATA_SEP "levels/level%d.dat", DATA, level);
  
  fi = fopen(filename, "r");
  
  if (fi == NULL)
    {
      perror(filename);
      exit(1);
    }
  
  
  /* Load pages: */
  
  num_pages = 0;
  
  do
    {
      /* Read page header: */
      
      fgets(temp, 512, fi);
      
      if (!feof(fi))
	{
	  /* Get screen layout (background): */
	  
	  for (y = 0; y < TILE_HEIGHT; y++)
	    {
	      fgets(temp, 512, fi);
	      
	      for (x = 0; x < TILE_WIDTH; x++)
		{
		  page[num_pages].background[y][x] = temp[x];
		}
	    }
	  
	  
	  /* Get page-to-page movement data: */
	  
	  fgets(temp, 512, fi);
	  
	  sscanf(temp, "%d %d %d %d",
		 &page[num_pages].left,
		 &page[num_pages].right,
		 &page[num_pages].up,
		 &page[num_pages].down);
	  
	  
	  /* Get page flags: */
	  
	  page[num_pages].lowgravity = 0;
	  page[num_pages].suction = 0;
	  page[num_pages].bosspage = 0;
	  page[num_pages].checkpoint = 0;
	  page[num_pages].snowy = 0;
	  page[num_pages].dark = 0;
	  page[num_pages].flickery = 0;
	  page[num_pages].mario = 0;
	  page[num_pages].hat = -1;
	  
	  
	  fgets(temp, 512, fi);
	  
	  if (strchr(temp, 'G') != NULL)
	    page[num_pages].lowgravity = 1;
	  
	  if (strchr(temp, 'u') != NULL)
	    page[num_pages].suction = 1;
	  
	  if (strchr(temp, 'B') != NULL)
	    page[num_pages].bosspage = 1;
	  
	  if (strchr(temp, 'b') != NULL)
	    page[num_pages].bosspage = -1;
	  
	  if (strchr(temp, 'C') != NULL)
	    page[num_pages].checkpoint = 1;
	  
	  if (strchr(temp, 'S') != NULL)
	    page[num_pages].snowy = 1;
	  
	  if (strchr(temp, 'D') != NULL)
	    page[num_pages].dark = 1;
	  
	  if (strchr(temp, 'F') != NULL)
	    page[num_pages].flickery = 1;
	  
	  if (strchr(temp, 'M') != NULL)
	    {
	      page[num_pages].mario = 1;
	      page[num_pages].hat = OBJ_HAT_PLUMBER_LEFT;
	    }
	  
	  if (strchr(temp, '1') != NULL)
	    page[num_pages].hat = OBJ_HAT_DESERT_LEFT;
	  else if (strchr(temp, '2') != NULL)
	    page[num_pages].hat = OBJ_HAT_HARDHAT_LEFT;
	  else if (strchr(temp, '3') != NULL)
	    page[num_pages].hat = OBJ_HAT_MINER_LEFT;
	  else if (strchr(temp, '4') != NULL)
	    page[num_pages].hat = OBJ_HAT_SCUBA_LEFT;
	  
	  
	  /* Read page text: */
	  
	  fgets(temp, 512, fi);
	  
	  temp[strlen(temp) - 1] = '\0';
	  strcpy(page[num_pages].note, temp);
	  
	  
	  /* Read in enemies list: */
	  
	  page[num_pages].num_enemies = 0;
	  
	  do
	    {
	      fgets(temp, 512, fi);
	      
	      if (strcmp(temp, "END\n") != 0)
		{
		  page[num_pages].
		    starting_enemies[page[num_pages].num_enemies].alive = 1;
		  
		  page[num_pages].
		    starting_enemies[page[num_pages].num_enemies].dead = 0;
		  
		  sscanf(temp, "%d %d %d",
			 &page[num_pages].
			 starting_enemies[page[num_pages].num_enemies].type,
			 &page[num_pages].
			 starting_enemies[page[num_pages].num_enemies].x,
			 &page[num_pages].
			 starting_enemies[page[num_pages].num_enemies].y);
		  
		  page[num_pages].num_enemies++;
		}
	    }
	  while (strcmp(temp, "END\n") != 0 && !feof(fi));
	  
	  
	  /* Eat blank separator line: */
	  
	  fgets(temp, 512, fi);
	  
	  num_pages++;
	}
    }
  while (!feof(fi));
  
  fclose(fi);
}


/* Draws a masked image onto a pixmap: */

void drawobject(int x, int y, int shape, Pixmap pix)
{
  if (shape != -1)
    {
      XSetClipOrigin(display, objgcs[shape], x, y);
      
      XCopyArea(display, object_pixmaps[shape], pix, objgcs[shape],
		0, 0, 32, 32, x, y);
    }
}


/* Returns whether or not a spot is solid: */

int solid(int x, int y, int who)
{
  int ok, i;
  
  
  ok = 0;
  
  
  /* Stand on solid blocks: */
  
  if (x >= -4 && x < WIDTH - 27 && y >= 0 && y < HEIGHT)
    {
      if (issolid(page[scrn].background[y / 32][(x + 4) / 32]) ||
	  issolid(page[scrn].background[y / 32][(x + 27) / 32]))
	ok = 1;
    }
  else if (x >= WIDTH - 27)
    {
      if (issolid(page[scrn].background[y / 32][x / 32]))
	ok = 1;
    }
  
  
  /* Stand on frozen enemies and minecarts, too: */
  
  for (i = 0; i < MAX_ENEMIES; i++)
    {
      if (enemy[i].alive == 1)
	{
	  if (enemy[i].frozen > 0 || enemy[i].type == BAD_SNAKE)
	    {
	      if (x + 31 >= enemy[i].x && x <= enemy[i].x + 31 &&
		  y >= enemy[i].y && y < enemy[i].y + 32 && who != i)
		ok = 1;
	    }
	  else
	    {
	      if (enemy[i].type == BAD_MINECART)
		{
		  if (x + 31 >= enemy[i].x && x <= enemy[i].x + 31 &&
		      y >= enemy[i].y + 16 && y < enemy[i].y + 24 && who != i)
		    ok = 1;
		}
	    }
	}
    }
  
  
  /* Stand on platforms, too: */
  
  for (i = 0; i < MAX_PLATFORMS; i++)
    {
      if (platform[i].alive == 1)
	{
	  if (x + 31 >= platform[i].x && x <= platform[i].x + 63 &&
	      y >= platform[i].y && y < platform[i].y + 32)
	    ok = 1;
	}
    }
  
  return(ok);
}


/* Returns whether or not a level character is solid: */

int issolid(char c)
{
  int ok;
  
  ok = 0;
  
  if (c == '#' || c == '<' || c == '=' || c == '>' || c == 'I' || c == 'B' ||
      c == 'S' || c == '+' || c == '6' || c == '%' ||
      c == 'L' || c == 'l' || c == 'C' || c == 'c' || c == 'R' || c == 'r' ||
      c == 'T' || c == 'Y' || c == 'U' || c == '7' || c == 'M')
    ok = 1;
  
  return(ok);
}


/* Returns whether or not a spot is water: */

int water(int x, int y)
{
  int ok;
  
  ok = 0;
  
  if (x >= 0 && x <= WIDTH && y >= 0 && y < HEIGHT)
    if (page[scrn].background[y / 32][(x + 4) / 32] == 'w' ||
	page[scrn].background[y / 32][(x + 27) / 32] == 'w' ||
	page[scrn].background[y / 32][(x + 4) / 32] == 'W' ||
        page[scrn].background[y / 32][(x + 27) / 32] == 'W' ||
	page[scrn].background[y / 32][(x + 4) / 32] == 'V' ||
        page[scrn].background[y / 32][(x + 27) / 32] == 'V')
      ok = 1;
  
  return(ok);
}


/* Returns whether or not a spot is jump-throughable: */

int jumpthroughable(int x, int y)
{
  int ok;
  
  ok = 0;
  
  if (x >= 0 && x <= WIDTH && y >= 0 && y < HEIGHT)
    if (page[scrn].background[y / 32][(x + 4) / 32] == 'T' ||
	page[scrn].background[y / 32][(x + 27) / 32] == 'T' ||
	page[scrn].background[y / 32][(x + 4) / 32] == 'Y' ||
	page[scrn].background[y / 32][(x + 27) / 32] == 'Y' ||
	page[scrn].background[y / 32][(x + 4) / 32] == 'U' ||
	page[scrn].background[y / 32][(x + 27) / 32] == 'U')
      ok = 1;
  
  return(ok);
}


/* Returns whether or not a spot is sand: */

int sand(int x, int y)
{
  int ok;
  
  ok = 0;
  
  if (x >= 0 && x <= WIDTH && y >= 0 && y < HEIGHT)
    if (page[scrn].background[y / 32][(x + 4) / 32] == 'S' ||
	page[scrn].background[y / 32][(x + 27) / 32] == 'S')
      ok = 1;
  
  return(ok);
}


/* Returns direction of the conveyor belt, if any: */

int conveyor(int x, int y)
{
  int dir;
  
  dir = DIR_NONE;
  
  if (x >= 0 && x <= WIDTH && y >= 0 && y < HEIGHT)
    {
      if (page[scrn].background[y / 32][(x + 4) / 32] == 'l' ||
	  page[scrn].background[y / 32][(x + 27) / 32] == 'l' ||
	  page[scrn].background[y / 32][(x + 4) / 32] == 'c' ||
	  page[scrn].background[y / 32][(x + 27) / 32] == 'c' ||
	  page[scrn].background[y / 32][(x + 4) / 32] == 'r' ||
	  page[scrn].background[y / 32][(x + 27) / 32] == 'r')
	dir = DIR_LEFT;
      else if (page[scrn].background[y / 32][(x + 4) / 32] == 'L' ||
	       page[scrn].background[y / 32][(x + 27) / 32] == 'L' ||
	       page[scrn].background[y / 32][(x + 4) / 32] == 'C' ||
	       page[scrn].background[y / 32][(x + 27) / 32] == 'C' ||
	       page[scrn].background[y / 32][(x + 4) / 32] == 'R' ||
	       page[scrn].background[y / 32][(x + 27) / 32] == 'R')
	dir = DIR_RIGHT;
    }
  
  return(dir);
}


/* Returns whether or not a spot is quicksand: */

int quicksand(int x, int y)
{
  int ok;
  
  ok = 0;
  
  if (x >= 0 && x <= WIDTH && y >= 0 && y < HEIGHT)
    if (page[scrn].background[y / 32][(x + 4) / 32] == 's' ||
	page[scrn].background[y / 32][(x + 27) / 32] == 's')
      ok = 1;
  
  return(ok);
}


/* Returns whether or not a spot is ice: */

int ice(int x, int y)
{
  int ok;
  
  ok = 0;
  
  if (x >= 0 && x <= WIDTH && y >= 0 && y < HEIGHT)
    if (page[scrn].background[y / 32][(x + 4) / 32] == 'I' ||
	page[scrn].background[y / 32][(x + 27) / 32] == 'I')
      ok = 1;
  
  return(ok);
}


/* Returns whether or not a spot is cactus: */

int cactus(int x, int y)
{
  int ok;
  
  ok = 0;
  
  if (x >= 0 && x <= WIDTH && y >= 0 && y < HEIGHT)
    if (page[scrn].background[y / 32][(x + 4) / 32] == 't' ||
	page[scrn].background[y / 32][(x + 27) / 32] == 't')
      ok = 1;
  
  return(ok);
}


/* Init. a new page of the level: *//* Init. a new page of the level: */

void gotothispage(void)
{
  int i, x, y, yy, anysolid;
  

  /* Reset explosions: */
  
  for (i = 0; i < MAX_EXPLOSIONS; i++)
    explosion[i].alive = 0;
  
  
  /* Reset ups: */
  
  for (i = 0; i < MAX_UPS; i++)
    up[i].alive = 0;
  
  
  /* Reset air bubbles: */
  
  for (i = 0; i < MAX_AIR_BUBBLES; i++)
    air_bubble[i].alive = 0;
  
  
  /* Reset drips: */
  
  for (i = 0; i < MAX_DRIPS; i++)
    drip[i].alive = 0;
  
  
  /* Reset dusts: */
  
  for (i = 0; i < MAX_DUSTS; i++)
    dust[i].alive = 0;
  
  
  /* Reset ice_chunks: */
  
  for (i = 0; i < MAX_ICE_CHUNKS; i++)
    ice_chunk[i].alive = 0;
  
  
  /* Reset sparks: */
  
  for (i = 0; i < MAX_SPARKS; i++)
    spark[i].alive = 0;
  
  
  /* Init. note letters: */
  
  for (i = 0; i < MAX_NOTE_LETTERS; i++)
    note_letter[i].alive = 0;
  
  
  /* Reset snowflakes: */
  
  for (i = 0; i < MAX_SNOWFLAKES; i++)
    snowflake[i].alive = 0;
  
  
  /* Reset splashes: */
  
  for (i = 0; i < MAX_SPLASHES; i++)
    splash[i].alive = 0;


  /* Reset platforms: */

  for (i = 0; i < MAX_PLATFORMS; i++)
    platform[i].alive = 0;
  
  
  /* Reset enemies: */
  
  for (i = 0; i < MAX_ENEMIES; i++)
    enemy[i].alive = 0;
  
  
  for (i = 0; i < page[scrn].num_enemies; i++)
    {
      enemy[i].alive = 1;
      enemy[i].mode = MODE_NORMAL;
      
      enemy[i].timer = 0;
      enemy[i].frozen = 0;
      enemy[i].drip_timer = 0;
      
      enemy[i].enemyslot = i;
      
      enemy[i].type = page[scrn].starting_enemies[i].type;
      
      
      /* Handle up's: */
      
      if (enemy[i].type == BAD_ONEUP)
	{
	  /* Turn on a one-up */
	  
	  enemy[i].alive = 0;
	  addup(page[scrn].starting_enemies[i].x * 32,
		page[scrn].starting_enemies[i].y * 32,
		UP_ONEUP, i);
	}
      else if (enemy[i].type == BAD_BANANA)
	{
	  /* Turn on a banana */
	  
	  enemy[i].alive = 0;
	  addup(page[scrn].starting_enemies[i].x * 32,
		page[scrn].starting_enemies[i].y * 32,
		UP_BANANA_BIG, i);
	}
      else if (enemy[i].type == BAD_BOLT)
	{
	  /* Turn on a bolt */
	  
	  enemy[i].alive = 0;
	  addup(page[scrn].starting_enemies[i].x * 32,
		page[scrn].starting_enemies[i].y * 32,
		UP_BOLT_BIG, i);
	}
      else if (enemy[i].type == BAD_PLATFORM_UP_DOWN ||
	       enemy[i].type == BAD_PLATFORM_UP ||
	       enemy[i].type == BAD_PLATFORM_DOWN ||
	       enemy[i].type == BAD_PLATFORM_SIDE ||
	       enemy[i].type == BAD_PLATFORM_DROP)
	{
	  /* Turn on platform: */
	  
	  enemy[i].alive = 0;
	  addplatform(page[scrn].starting_enemies[i].x * 32,
		      page[scrn].starting_enemies[i].y * 32,
		      enemy[i].type);
	}
      else if (enemy[i].type == BAD_NONE)
	{
	  /* Turn off a non-existant enemy: */
	  
	  enemy[i].alive = 0;
	}
      
      
      if (page[scrn].starting_enemies[i].dead == 1)
	enemy[i].alive = 0;
      
      
      /* Handle bosses: */
      
      if (enemy[i].type < 0)
	enemy[i].type = FIRST_BOSS + (-enemy[i].type) - 1;
      
      
      enemy[i].health = 100;
      enemy[i].x = page[scrn].starting_enemies[i].x * 32;
      enemy[i].y = page[scrn].starting_enemies[i].y * 32;
      enemy[i].dir = DIR_LEFT;
      enemy[i].ym = 0;
      enemy[i].ymm = 0;
      enemy[i].was_shot = 0;
    }
  
  
  /* Add a minecart if you were in one: (ie, keep you in it!) */
  
  if (player.minecart_in != -1)
    addenemy(player.x, player.y + 16, BAD_MINECART, player.minecart_dir);
  
  
  /* Reset bullets: */
  
  for (i = 0; i < MAX_BULLETS; i++)
    bullet[i].alive = 0;
  
  
  /* Create new master background: */
    
  for (i = 0; i < 3; i++)
    XFillRectangle(display, masterbkg[i], blackgc, 0, 0, WIDTH, HEIGHT);
  
  for (y = 0; y < TILE_HEIGHT; y++)
    for (x = 0; x < TILE_WIDTH; x++)
      {
	applyshape(page[scrn].background[y][x], x, y);
	applyshape(page[scrn].background[y][x], x, y + 1);
      }
  
  
  /* Set checkpoint: */
  
  if (page[scrn].checkpoint == 1 && scrn > last_checkpoint)
    last_checkpoint = scrn;

  
  /* Add snow everywhere (so it doesn't always start from the top) */
  
  if (page[scrn].snowy == 1)
    {
      for (i = 0; i < MAX_SNOWFLAKES / 3; i++)
	{
	  do
	    {
	      x = randnum(WIDTH - 32);
	      y = randnum(HEIGHT - 32);
	      
	      anysolid = 0;
	      
	      for (yy = 0; yy < y; yy = yy + 32)
		if (solid(x, yy, -1))
		  anysolid = 1;
	    }
	  while (anysolid == 1);
	  
	  addsnowflake(x, y);
	}
    }
  
  
  /* Turn on note letters: */
  
  if (strcmp(page[scrn].note, ".") != 0)
    {
      for (i = 0; i < strlen(page[scrn].note); i++)
	{
	  note_letter[i].alive = 1;
	  note_letter[i].x = ((WIDTH - (strlen(page[scrn].note) * 16)) / 2 +
			      i * 16);
	  note_letter[i].y = -10;
	  note_letter[i].ym = 2;
	  note_letter[i].mode = 0;
	  note_letter[i].timer = i * 3;
	  note_letter[i].c = page[scrn].note[i];
	}
      
      
      /* Never show it again: */
      
      strcpy(page[scrn].note, ".");
    }
  
  
  /* Play boss music if this is the boss page! */
  
#ifdef MUSIC
  if (no_music == 0)
    {
      if (page[scrn].bosspage != 0)
	{
	  Mix_HaltMusic();
	  Mix_PlayMusic(songs[MUSIC_BOSS], 1);
	}
      else
	{
	  if (oldbosspage == -1)
	    {
	      Mix_HaltMusic();
	      Mix_PlayMusic(songs[level], 1);
	    }
	}
    }
#endif
  
  oldbosspage = page[scrn].bosspage;
}


/* Convert from level.dat file to actual shape and draw it on backgrounds: */

void applyshape(char z, int x, int y)
{
  int i;
  int c;
  int cs[3];
  
  c = -1;
  cs[0] = -1;
  cs[1] = -1;
  cs[2] = -1;
  
  if (z == '.')
    c = OBJ_BACK_METAL;
  else if (z == '#')
    c = OBJ_FORE_METAL;
  else if (z == '+')
    c = OBJ_FORE_METAL_DAMAGED;
  else if (z == 'P')
    c = OBJ_BACK_PINE;
  else if (z == '0')
    c = OBJ_BACK_STARS0;
  else if (z == '1')
    c = OBJ_BACK_STARS1;
  else if (z == '2')
    {
      for (i = 0; i < 3; i++)
	cs[i] = OBJ_BACK_STARS2_0 + i;
    }
  else if (z == '6')
    c = OBJ_FORE_BRICK_DAMAGED;
  else if (z == 'B')
    c = OBJ_FORE_BRICK;
  else if (z == 'b')
    c = OBJ_BACK_BRICK;
  else if (z == 't')
    {
      for (i = 0; i < 3; i++)
	cs[i] = OBJ_FORE_CACTUS;
    }
  else if (z == 'M')
    c = OBJ_FORE_MOON;
  else if (z == '_')
    c = OBJ_FORE_PIPE_0;
  else if (z == '~')
    c = OBJ_FORE_PIPE_1;
  else if (z == ':')
    c = OBJ_FORE_PIPE_2;
  else if (z == 'm')
    c = OBJ_BACK_CRATER;
  else if (z == 'E')
    c = OBJ_BACK_EARTH;
  else if (z == 'F')
    c = OBJ_BACK_FLAG;
  else if (z == 'a')
    c = OBJ_BACK_ANTENNAE;
  else if (z == 'e')
    c = OBJ_BACK_HIERO_0;
  else if (z == 'f')
    c = OBJ_BACK_HIERO_1;
  else if (z == 'Q')
    c = OBJ_BACK_GREY;
  else if (z == '-')
    c = OBJ_BACK_SKY;
  else if (z == 'I')
    c = OBJ_FORE_ICE;
  else if (z == '<')
    c = OBJ_FORE_SNOW_LEFT;
  else if (z == '=')
    c = OBJ_FORE_SNOW_CENTER;
  else if (z == '>')
    c = OBJ_FORE_SNOW_RIGHT;
  else if (z == '[')
    c = OBJ_BACK_TAN_LEFT;
  else if (z == '/')
    c = OBJ_BACK_TAN_LEFT_EDGE;
  else if (z == ']')
    c = OBJ_BACK_TAN_RIGHT;
  else if (z == '\\')
    c = OBJ_BACK_TAN_RIGHT_EDGE;
  else if (z == 'S')
    c = OBJ_FORE_SAND;
  else if (z == 's')
    {
      for (i = 0; i < 3; i++)
	cs[i] = OBJ_BACK_QUICKSAND0 + i;
    }
  else if (z == '%')
    {
      for (i = 0; i < 3; i++)
	cs[i] = OBJ_FORE_FAN0 + i;
    }
  else if (z == 'X')
    c = OBJ_BACK_SCAFOLD;
  else if (z == 'w')
    c = OBJ_BACK_WATER;
  else if (z == 'W')
    {
      for (i = 0; i < 3; i++)
	cs[i] = OBJ_BACK_WAVE0 + i;
    }
  else if (z == '|')
    c = OBJ_BACK_PALM_TRUNK;
  else if (z == '(')
    c = OBJ_BACK_PALM_TOP_LEFT;
  else if (z == '*')
    c = OBJ_BACK_PALM_TOP_CENTER;
  else if (z == ')')
    c = OBJ_BACK_PALM_TOP_RIGHT;
  else if (z == '`')
    c = OBJ_BACK_CLOUD_LEFT;
  else if (z == '\'')
    c = OBJ_BACK_CLOUD_RIGHT;
  else if (z == 'L')
    {
      for (i = 0; i < 3; i++)
        cs[i] = OBJ_FORE_CONVEYOR_LEFT_0 + i;
    }
  else if (z == 'l')
    {
      for (i = 0; i < 3; i++)
        cs[i] = OBJ_FORE_CONVEYOR_LEFT_2 - i;
    }
  else if (z == 'C')
    {
      for (i = 0; i < 3; i++)
        cs[i] = OBJ_FORE_CONVEYOR_CENTER_0 + i;
    }
  else if (z == 'c')
    {
      for (i = 0; i < 3; i++)
        cs[i] = OBJ_FORE_CONVEYOR_CENTER_2 - i;
    }
  else if (z == 'R')
    {
      for (i = 0; i < 3; i++)
        cs[i] = OBJ_FORE_CONVEYOR_RIGHT_0 + i;
    }
  else if (z == 'r')
    {
      for (i = 0; i < 3; i++)
        cs[i] = OBJ_FORE_CONVEYOR_RIGHT_2 - i;
    }
  else if (z == '^')
    c = OBJ_BACK_LAMP;
  else if (z == 'o')
    {
      cs[0] = OBJ_BAD_GEAR_0;
      cs[1] = OBJ_BAD_GEAR_1;
      cs[2] = OBJ_BAD_GEAR_1;
    }
  else if (z == '?')
    {
      cs[0] = OBJ_BACK_CLOCK_DIG_0;
      cs[1] = OBJ_BACK_CLOCK_DIG_0;
      cs[2] = OBJ_BACK_CLOCK_DIG_1;
    }
  else if (z == '@')
    {
      for (i = 0; i < 3; i++)
	cs[i] = OBJ_BACK_CLOCK_0 + i;
    }
  else if (z == 'G')
    {
      for (i = 0; i < 3; i++)
	cs[i] = OBJ_BACK_GEAR_TOPLEFT_0 + i;
    }
  else if (z == 'g')
    {
      for (i = 0; i < 3; i++)
	cs[i] = OBJ_BACK_GEAR_BOTLEFT_0 + i;
    }
  else if (z == 'H')
    {
      for (i = 0; i < 3; i++)
	cs[i] = OBJ_BACK_GEAR_TOPRIGHT_0 + i;
    }
  else if (z == 'h')
    {
      for (i = 0; i < 3; i++)
	cs[i] = OBJ_BACK_GEAR_BOTRIGHT_0 + i;
    }
  else if (z == 'J')
    {
      for (i = 0; i < 3; i++)
	cs[i] = OBJ_BACK_GEAR_TOPLEFT_2 - i;
    }
  else if (z == 'j')
    {
      for (i = 0; i < 3; i++)
	cs[i] = OBJ_BACK_GEAR_BOTLEFT_2 - i;
    }
  else if (z == 'K')
    {
      for (i = 0; i < 3; i++)
	cs[i] = OBJ_BACK_GEAR_TOPRIGHT_2 - i;
    }
  else if (z == 'k')
    {
      for (i = 0; i < 3; i++)
	cs[i] = OBJ_BACK_GEAR_BOTRIGHT_2 - i;
    }
  else if (z == 'V')
    {
      for (i = 0; i < 3; i++)
	cs[i] = OBJ_BACK_WATER_SUCK_0 + i;
    }
  else if (z == '&')
    c = OBJ_BACK_ROCK_0;
  else if (z == 'A')
    c = OBJ_BACK_ROCK_1;
  else if (z == '7')
    c = OBJ_FORE_ROCK;
  else if (z == 'T')
    c = OBJ_FORE_TRACK;
  else if (z == 'Y')
    c = OBJ_FORE_TRACK_BROKEN_LEFT;
  else if (z == 'U')
    c = OBJ_FORE_TRACK_BROKEN_RIGHT;
  
  
  /* Draw shape(s) onto background pixmaps: */
  
  if (c != -1)
    {
      for (i = 0; i < 3; i++)
	drawobject(x * 32, y * 32, c, masterbkg[i]);
    }
  else if (cs[0] != -1)
    {
      for (i = 0; i < 3; i++)
	drawobject(x * 32, y * 32, cs[i], masterbkg[i]);
    }
  else
    {
      for (i = 0; i < 3; i++)
	XFillRectangle(display, masterbkg[i], blackgc,
		       x * 32, y * 32, 32, 32);
    }
}


/* Reset level: */

void reset(void)
{
  int i, found;
  
  
  /* INIT PLAYER STUFF: */

  /* Position: */  
  
  player.x = 32;
  player.y = 64;
  found = 0;
  
  for (i = 0; i < HEIGHT && found == 0; i = i + 32)
    {
      if (solid(player.x, i, -1) == 0)
	{
	  player.y = i;
	  found = 1;
	}
    }

  player.ox = player.x;
  player.oy = player.y;
  
  
  /* Init warp-ins: */
  
  for (i = 0; i < MAX_WARP_INS; i++)
    {
      old_player_x[i] = player.x;
      old_player_y[i] = (player.y * i) / MAX_WARP_INS;
    }
  
  
  /* Direction: */
  
  player.dir = DIR_RIGHT;
  player.xspeed = 0;
  player.ym = 0;
  
  
  /* Timers: */
  
  player.walk_count = 0;
  player.still_count = 0;
  player.blink_timer = 0;
  
  
  /* Jump info: */
   
  player.jump = 0;
  player.jumpspeed = 0;
  player.jumptimer = 0;
  
  
  /* Health: */
  
  player.health = 25;
  player.want_health = 50;
  
  
  /* Enemy weapon effects: */
  
  player.frozen = 0;
  
  
  /* Timers: */
  
  player.ouch_timer = 0;
  player.die_timer = 0;
  player.warp_in_timer = MAX_WARP_INS;
  player.drip_timer = 0;
  player.was_in_water = 0;
  player.minecart_in = -1;
  
  
  /* Weapon: */
  
  player.selected_weapon = WPN_NORMAL;
  
  
  /* Reset keyboard: */
  
  for (i = 0; i < NUM_KEYS; i++)
    {
      holds[i] = KeyRelease;
      old_holds[i] = KeyRelease;
      key_holds[i] = KeyRelease;
      stick_holds[i] = KeyRelease;
    }
  
  dont_play = 0;
}


/* Make player die: */

void killplayer(void)
{
  int i, j;
  
  
  if (player.die_timer == 0)
    {
      addexplosion(player.x, player.y, 1);
      addexplosion(player.x + 32, player.y, 0);
      addexplosion(player.x - 32, player.y, 0);
      addexplosion(player.x, player.y + 32, 0);
      addexplosion(player.x, player.y - 32, 0);
      
      player.die_timer = 100;
      
      player.lives--;
      
      playsound(SND_DIE);
      dont_play = 1;
      
      if (player.lives <= 0)
	gameover = 1;
      
      
      /* Bring enemies back to life: */
      
      for (i = 0; i < num_pages; i++)
	{
	  for (j = 0; j < page[i].num_enemies; j++)
	    page[i].starting_enemies[j].dead = 0;
	}
    }
  
  player.health = 0;
}


/* Grey out a level icon on the main menu: */

void eraselevel(int x, int y)
{
  int i;

  for (i = 0; i < 64; i = i + 2)
    {
      XDrawLine(display, masterbkg[0], blackgc,
		x + i, y + 63,
		x, y + 63 - i);
      
      XDrawLine(display, masterbkg[0], blackgc,
		x + i + 1, y,
		x + 64, y + 63 - i);
    }
  
  
  XDrawArc(display, masterbkg[0], thickredgc,
	   x + 5, y + 5, 54, 54,
	   0, 360 * 64);
  
  XDrawLine(display, masterbkg[0], thickredgc,
	    x + 10, y + 10, x + 54, y + 54);
}


/* Hurt the player: */

void hurtplayer(int damage)
{
  if (player.die_timer == 0 && player.ouch_timer == 0)
    {
      player.health = player.health - damage;
      
      if (player.health <= 0)
	{
	  player.health = 0;
	  killplayer();
	}
      
      
      /* Become invincible (and partially invisible) for
	 a moment: */
      
      player.ouch_timer = 20;
      
      
      /* Get shoved backwards: */
      
      if (player.frozen == 0)
	{
	  if (player.dir == DIR_LEFT)
	    {
	      player.x = player.x + 24;
	      if (solid(player.x, player.y, -1))
		player.x = player.x - 24;
	    }
	  else
	    {
	      player.x = player.x - 24;
	      if (solid(player.x, player.y, -1))
		player.x = player.x + 24;
	    }
	}
    }
  
  playsound(SND_HURT);
}


/* Turns an enemy off, and removes it from the page's enemy list: */

void killenemy(int whichenemy)
{
  enemy[whichenemy].alive = 0;
  
  if (enemy[whichenemy].enemyslot != -1)
    page[scrn].starting_enemies[enemy[whichenemy].enemyslot].dead = 1;
}


/* Add an enemy: */

void addenemy(int x, int y, int type, int dir)
{
  int found, i;
  
  
  /* Find a free slot: */
  
  found = -1;
  
  for (i = 0; i < MAX_ENEMIES; i++)
    if (enemy[i].alive == 0)
      found = i;
  
  
  /* Init. this new enemy: */
  
  if (found != -1)
    {
      enemy[found].alive = 1;
      enemy[found].mode = MODE_NORMAL;
      enemy[found].enemyslot = -1;

      enemy[found].timer = 0;
      enemy[found].frozen = 0;
      enemy[found].drip_timer = 0;
      
      enemy[found].type = type;
      
      enemy[found].health = 100;
      enemy[found].x = x;
      enemy[found].y = y;
      enemy[found].dir = dir;
      enemy[found].ym = 0;
      enemy[found].ymm = 0;
      enemy[found].was_shot = 0;
    }
}


void levelintro(void)
{
  struct timeval now, then;
  long time_padding;
  int drop_y, drop_ym, drop_mode, dropping, drop_timer, i, c;
  star_type star[MAX_STARS];
  
  
  c = 0;
  
  /* Init stars: */
  
  for (i = 0; i < MAX_STARS; i++)
    {
      star[i].x = randnum(WIDTH);
      star[i].y = randnum(HEIGHT);
      star[i].xm = randnum(10) + 2;
    }

  
  /* Init. enemy icon: */
  
  drop_y = 0;
  drop_ym = 0;
  dropping = 1;
  drop_mode = 0;
  drop_timer = 0;
  
  
  do
    {
      gettimeofday(&then, NULL);
      
      
      /* Draw screen: */
      
      /* (Clear) */
      XFillRectangle(display, backbuffer, blackgc, 0, 0, WIDTH, HEIGHT);
      
      
      /* (Stars) */
      for (i = 0; i < MAX_STARS; i++)
	{
	  XDrawPoint(display, backbuffer, whitegc, star[i].x, star[i].y);
	  
	  star[i].x = star[i].x + star[i].xm;
	  
	  if (star[i].x > WIDTH)
	    {
	      star[i].x = -randnum(20);
	      star[i].xm = randnum(10) + 2;
	      star[i].y = randnum(HEIGHT);
	    }
	}
      
      
      drop_y = drop_y + drop_ym;
      
      
      if (drop_y >= HEIGHT / 2 - 16 && drop_mode == 0)
	{
	  drop_y = HEIGHT / 2 - 16;
	  drop_mode = 1;
	  drop_timer = 50;
	  dropping = 0;
	  drop_ym = 0;
	  
#ifdef SOUND
	  Mix_HaltChannel(-1);
	  if (level == 1)
	    playsound(SND_BOSS1);
	  else if (level == 2)
	    playsound(SND_BOSS2);
	  else if (level == 3)
	    playsound(SND_BOSS3);
	  else if (level == 4)
	    playsound(SND_BOSS4);
#endif
	}
      
      if (dropping == 1)
	drop_ym = drop_ym + 1;
      
      
      if (drop_timer > 0)
	{
	  drop_timer--;
	  
	  if (drop_timer <= 0)
	    {
	      dropping = 1;
	      drop_ym = -8;
	    }
	}
      
      
      /* (Enemy) */
      if (level == 1)
	c = OBJ_BOSS1_LEFT_0;
      else if (level == 2)
	c = OBJ_BOSS2_LEFT_0;
      else if (level == 3)
	c = OBJ_BOSS3_0;
      else if (level == 4)
	c = OBJ_BOSS4_0;
      
      drawobject(WIDTH / 2 - 16, drop_y, c, backbuffer);
      
      
      if (drop_mode == 1 && drop_timer > 0)
	{
	  /* (Level title) */
	  if (level != -1)
	    drawcenteredtext(display, backbuffer, whitegc, 0,
			     WIDTH, HEIGHT / 2 + 16 + fh,
			     level_names[level - 1], font);
	  else
	    drawcenteredtext(display, backbuffer, whitegc, 0,
			     WIDTH, HEIGHT / 2 + 16 + fh,
			     "???", font);
	}
      
      
      XCopyArea(display, backbuffer, window, whitegc, 0, 0, WIDTH, HEIGHT,
		0, 0);
      
      
      /* Keep framerate exact: */
      
      XFlush(display);
      
      gettimeofday(&now, NULL);
      
      time_padding = FRAMERATE - ((now.tv_sec - then.tv_sec) * 1000000 +
				  (now.tv_usec - then.tv_usec));
      
      if (time_padding > 0)
	usleep(time_padding);
    }
  while (drop_y < HEIGHT);
}


int title(void)
{
  XEvent event;
  char tip[80];
  struct timeval now, then;
  long time_padding;
  int quit, done, i, x, y, num_threeds, mode, timer, threed_which, ym,
    want_done, exploding, explode_timer, spin_mode, key;
  float zz;
  XPoint point[4];
  struct star_type star[MAX_STARS];
  struct threed_type threed[32][32];
  
  
  /* Init stars: */
  
  for (i = 0; i < MAX_STARS; i++)
    {
      star[i].x = randnum(WIDTH);
      star[i].y = randnum(HEIGHT);
      star[i].xm = randnum(10) + 2;
    }
  
  
  /* Pick a 3D Shape: */
  
  strcpy(tip, "");
  
  if (have_seen_title == 0)
    {
      threed_which = THREED_BOBO_0;
      have_seen_title = 1;
    }
  else
    {
      do
	{
	  threed_which = randnum(NUM_THREEDS);
	}
      while (threed_which == last_threed_which ||
	     (threed_which == THREED_EVIL && ok_for_evil == 0));
      
      if (threed_which == THREED_BANANA)
	strcpy(tip, "Tip: Bananas provide health!");
      else if (threed_which == THREED_BOLT)
	strcpy(tip, "Tip: Bolts rejuvinate your weapons!");
    }
  
  last_threed_which = threed_which;
  
  
  /* Init. 3D pixels: */
  
  num_threeds = 0;
  
  for (y = 0; y < 32; y++)
    {
      for (x = 0; x < 32; x++)
	{
	  threed[y][x].alive = 1;
	  threed[y][x].x = (x - 16) * THREED_SIZE;
	  threed[y][x].y = (y - 16) * THREED_SIZE;
	  threed[y][x].z = (float) 0.0;
	  threed[y][x].color_pos = threed_shape[threed_which][y][x];
	  threed[y][x].xm = randnum(10) - 5;
	  threed[y][x].zm = randnum(10) - 5;
	}
    }
  
  
  /* Title screen main loop: */
  
  done = 0;
  want_done = 0;
  quit = 0;
  distance = 2000;
  anglex = 0;
  angley = 0;
  mode = MODE_RUSH;
  timer = 0;
  explode_timer = 0;
  ym = -5;
  exploding = 0;
  
  spin_mode = randnum(NUM_SPIN_MODES);
  
  
  do
    {
      gettimeofday(&then, NULL);
      
      
      /* Get and handle events: */
      
      key = XK_VoidSymbol;
      
      while (XPending(display))
	{
	  XNextEvent(display, &event);
	  
	  if (event.type == KeyPress)
	    {
	      /* Get the key's name: */
	      
	      key = XLookupKeysym((XKeyEvent *)&event, 0);
	    }
	}

#ifdef JOYSTICK
      read(js_fd, &js, JS_RETURN);
      if (js.buttons)
	key = XK_space;
#endif
      
      
      if (key == XK_q)
	{
	  /* Q: Quit: */
	  
	  quit = 1;
	  want_done = 1;
	  done = 1;
	}
      else if (key == XK_space || key == XK_Return || key == XK_KP_Space ||
	       key == XK_KP_Enter)
	{
	  /* Space / Enter - Begin: */
	  
	  exploding = 1;
	  timer = 0;
	  want_done = 1;
	}
      
      
      /* Move 3D Object: */
      
      if (mode == MODE_RUSH)
	{
	  if (spin_mode == 0 || spin_mode == 1)
	    anglex = - distance / 2;
	  
	  if (spin_mode == 1 || spin_mode == 2)
	    angley = - distance / 3;
	  
	  
	  if (distance > 0)
	    distance = distance - 20;
	  else
	    {
	      distance = 0;
	      
	      timer++;
	      
	      if (timer > 80)
		{
		  mode = MODE_NORMAL;
		  timer = 0;
		  spin_mode = randnum(NUM_SPIN_MODES);
		}
	    }
	}
      else if (mode == MODE_NORMAL)
	{
	  if (spin_mode == 0 || spin_mode == 1)
	    {
	      anglex = anglex + 5;
	      
	      if (anglex > 360)
		anglex = anglex - 360;
	    }
	  
	  if (spin_mode == 1 || spin_mode == 2)
	    {
	      angley = angley + 4;
	      
	      if (angley > 360)
		angley = angley - 360;
	    }
	}
      

      if (exploding == 1)
	{
	  for (y = 0; y < 32; y++)
	    {
	      for (x = 0; x < 32; x++)
		{
		  threed[y][x].y = threed[y][x].y + ym;
		  threed[y][x].x = threed[y][x].x + threed[y][x].xm;
		  threed[y][x].z = threed[y][x].z + threed[y][x].zm;
		}
	    }
	  
	  ym = ym + 1;
	  
	  
	  /* Keep exploding for a little bit: */
	  
	  explode_timer++;
	  
	  if (explode_timer >= 40)
	    {
	      if (want_done == 1)
		quit = 0;
	      else
		quit = 2;
	      
	      done = 1;
	    }
	}
      else
	{
	  if (mode == MODE_NORMAL)
	    {
	      timer++;
	      
	      if (timer >= 500)
		{
		  exploding = 1;
		  explode_timer = 0;
		}
	    }
	}
      
      
      recalculatetrig();
      
      
      /* Draw screen: */
      
      /* (Clear) */
      XFillRectangle(display, backbuffer, blackgc, 0, 0, WIDTH, HEIGHT);
      
      /* (Stars) */
      for (i = 0; i < MAX_STARS; i++)
	{
	  XDrawPoint(display, backbuffer, whitegc, star[i].x, star[i].y);
	  
	  star[i].x = star[i].x + star[i].xm;
	  
	  if (star[i].x > WIDTH)
	    {
	      star[i].x = -randnum(20);
	      star[i].xm = randnum(10) + 2;
	      star[i].y = randnum(HEIGHT);
	    }
	}
      
      
      /* (3D Shape): */
      for (y = 0; y < 32; y++)
	{
	  for (x = 0; x < 32; x++)
	    {
	      if (spin_mode == 4 && mode == MODE_NORMAL)
		zz = sin(M_PI * (y * 32 + x + timer) * 10 / 180.0) * 3.0;
	      else
		zz = 0;
	      
	      if (threed[y][x].color_pos != -1)
		{
		  calc3d(&(point[0].x), &(point[0].y),
			 threed[y][x].x,
			 threed[y][x].y,
			 threed[y][x].z + zz);
		  calc3d(&(point[1].x), &(point[1].y),
			 threed[y][x].x + THREED_SIZE,
			 threed[y][x].y,
			 threed[y][x].z + zz);
		  calc3d(&(point[2].x), &(point[2].y),
			 threed[y][x].x + THREED_SIZE,
			 threed[y][x].y + THREED_SIZE,
			 threed[y][x].z + zz);
		  calc3d(&(point[3].x), &(point[3].y),
			 threed[y][x].x,
			 threed[y][x].y + THREED_SIZE,
			 threed[y][x].z + zz);
		  
		  XFillPolygon(display, backbuffer,
			       threed_gc[threed_which][threed[y][x].color_pos],
			       point, 4, Complex,
			       CoordModeOrigin);
		}
	    }
	}
      
      
      /* Draw text: */
      
      if (distance == 0 && exploding == 0)
	{
	  drawobject(WIDTH / 2 - 32, HEIGHT / 2, OBJ_TITLE_LEFT,
		     backbuffer);
	  drawobject(WIDTH / 2, HEIGHT / 2, OBJ_TITLE_RIGHT,
		     backbuffer);
	  
	  drawcenteredtext(display, backbuffer, whitegc,
			   0, WIDTH, HEIGHT - fh * 5,
			   "By Bill Kendrick, Marianne Waage",
			   font);
	  
	  drawcenteredtext(display, backbuffer, whitegc,
			   0, WIDTH, HEIGHT - fh * 4,
			   "and Melissa Hardenbrook",
			   font);
	  
	  drawcenteredtext(display, backbuffer, whitegc,
			   0, WIDTH, HEIGHT - fh * 3,
			   "(c) 2000 New Breed Software", font);
	  
#ifndef JOYSTICK
	  drawcenteredtext(display, backbuffer, red2gc,
			   0, WIDTH, HEIGHT - fh * 1,
			   "Press SPACE to play!", font);
#else
	  drawcenteredtext(display, backbuffer, red2gc,
			   0, WIDTH, HEIGHT - fh * 1,
			   "Press SPACE or FIRE to play!", font);
#endif
	  
	  if (strlen(tip) > 0)
	    drawcenteredtext(display, backbuffer, green2gc,
			     0, WIDTH, fh * 2, tip, font);
	}
      
      
      /* Update window: */
      
      XCopyArea(display, backbuffer, window, whitegc, 0, 0, WIDTH, HEIGHT,
		0, 0);
      
      
      
      /* Play music: */
      
#ifdef MUSIC
      if (no_music == 0)
	{
	  if (!Mix_PlayingMusic())
	    {
	      Mix_PlayMusic(songs[MUSIC_TITLE], 1);
	    }
	}
#endif
  

      /* Keep framerate exact: */
      
      XFlush(display);
      
      gettimeofday(&now, NULL);
      
      time_padding = FRAMERATE - ((now.tv_sec - then.tv_sec) * 1000000 +
				  (now.tv_usec - then.tv_usec));
      
      if (time_padding > 0)
	usleep(time_padding);
    }
  while (done == 0);

  
  return(quit);
}


void calc3d(short * sx, short * sy, float x, float y, float z)
{
  float xx, yy, zz;
  
  
  /* Rotate: */
  
  xx = x * cos_anglex - z * sin_anglex;
  zz = x * sin_anglex + z * cos_anglex;
  
  yy = y * cos_angley - zz * sin_angley;
  zz = y * sin_angley + zz * cos_angley;
  
  
  zz = zz + distance;
  
  
  if (zz < -DISTANCE + 25)
    zz = -DISTANCE + 25;
  
  
  /* Convert (x,y,z) into (x,y) with a 3D look: */
  
  *sx = xx / ((zz + DISTANCE) / ASPECT);
  *sy = yy / ((zz + DISTANCE) / ASPECT);
  
  
  /* Transpose (0,0) origin to the center of the window: */
  
  *sx = *sx + WIDTH / 2;
  *sy = *sy + HEIGHT / 3;
}


void recalculatetrig(void)
{
  cos_anglex = cos(M_PI * anglex / 180.0);
  sin_anglex = sin(M_PI * anglex / 180.0);

  cos_angley = cos(M_PI * angley / 180.0);
  sin_angley = sin(M_PI * angley / 180.0);
}


void weaponscreen(void)
{
  XEvent event;
  struct timeval now, then;
  long time_padding;
  int done, i, top, left, width, height, ojsx, ojsy, ojsb, key;
  
  
  top = 64 - 16;
  left = 24 - 16;
  width = 55 + 32 + 32 + 32;
  height = (24 * 8) + 32 + fh + 5;
  
  
  done = 0;
  
  
  /* Draw background (or black if it's dark (don't give away any secrets!)) */
  
  if (page[scrn].dark == 0)
    {
      XCopyArea(display, masterbkg[0], backbuffer, whitegc,
		0, 0, WIDTH, HEIGHT, 0, 0);
      
      XFillRectangle(display, backbuffer, blackgc,
		     top, left, width, height);
    }
  else
    XFillRectangle(display, backbuffer, blackgc, 0, 0, WIDTH, HEIGHT);
  
  
  for (i = 0; i < 4; i++)
    {
      XDrawRectangle(display, backbuffer, whitegc,
		     top + i, left + i, width - i * 2, height - i * 2);
    }
  
  
  for (i = 0; i < NUM_WEAPONS; i++)
    {
      if (i == 0 || levels_won[i - 1] == 1)
	{
	  /* Draw this weapon's meter: */
	  
	  XFillRectangle(display, backbuffer, blackgc,
			 96, i * 24 + 24,
			 55, 15);
	  
	  XFillRectangle(display, backbuffer, red0gc,
			 98, i * 24 + 24 + 2,
			 50, 10);
	  
	  XFillRectangle(display, backbuffer, red1gc,
			 98, i * 24 + 24 + 2 + 2,
			 50, 6);
	  
	  XFillRectangle(display, backbuffer, red2gc,
			 98, i * 24 + 24 + 2 + 4,
			 50, 2);
	  
	  XFillRectangle(display, backbuffer, green0gc,
			 98, i * 24 + 24 + 2,
			 player.weapon[i] * 2, 10);
	  
	  XFillRectangle(display, backbuffer, green1gc,
			 98, i * 24 + 24 + 2 + 2,
			 player.weapon[i] * 2, 6);
	  
	  XFillRectangle(display, backbuffer, green2gc,
			 98, i * 24 + 24 + 2 + 4,
			 player.weapon[i] * 2, 2);
	  
	  
	  /* Draw the weapon next to its meter: */
	  
	  drawobject(98 + 50, i * 24 + 24 + 2 - 8,
		     weapon_images[i],
		     backbuffer);
	  
	  
	  /* Draw the weapon's name next to it: */
	  
	  drawtext(display, backbuffer, whitegc, 98, i * 24 + 24 + 10 + fh,
		   weapon_names[i]);
	}
    }
  


  /* Turn down music volume: */

  ojsx = 128;
  ojsy = 128;
  ojsb = 1;
  
  do
    {
      gettimeofday(&then, NULL);
      
      
      /* Get and handle events: */
      
      key = XK_VoidSymbol;
      
      while (XPending(display))
	{
	  XNextEvent(display, &event);
	  
	  if (event.type == KeyPress)
	    {
	      /* Get the key's name: */
	      
	      key = XLookupKeysym((XKeyEvent *)&event, 0);
	    }
	}	      
      
      
#ifdef JOYSTICK
      read(js_fd, &js, JS_RETURN);
      
      if (js.x < 64 && ojsx >= 64)
	key = KEYBD_LEFT;
      else if (js.x > 192 && ojsx <= 192)
	key = KEYBD_RIGHT;
      
      if (js.y < 64 && ojsy >= 64)
	key = KEYBD_UP;
      else if (js.y > 192 && ojsy <= 192)
	key = KEYBD_DOWN;
      
      if (js.buttons && ojsb == 0)
	key = KEYBD_FIRE;

      ojsx = js.x;
      ojsy = js.y;
      ojsb = js.buttons;
#endif


      if (key == XK_q)
	{
	  /* Q: Abort: */
	  
	  killplayer();
	  gameover = 1;
	  done = 1;
	}
      
      if (key == XK_Tab || key == XK_Escape || key == KEYBD_FIRE ||
	  key == XK_space || key == XK_Return || key == XK_KP_Enter ||
	  key == XK_KP_Space)
	{
	  /* Tab / Escape / Space / Return: Return to game: */
	  
	  done = 1;
	}
      
      
      if (key == KEYBD_UP || key == XK_KP_8)
	{
	  /* Up: */
	  
	  do
	    {
	      player.selected_weapon--;
	      
	      if (player.selected_weapon < 0)
		player.selected_weapon = NUM_WEAPONS - 1;
	    }
	  while (player.selected_weapon != 0 &&
		 levels_won[player.selected_weapon - 1] == 0);
	}
      else if (key == KEYBD_DOWN || key == XK_F31 || key == XK_KP_5 ||
	       key == XK_KP_2)
	{
	  /* Down: */
	  
	  do
	    {
	      player.selected_weapon++;
	      
	      if (player.selected_weapon >= NUM_WEAPONS)
		player.selected_weapon = 0;
	    }
	  while (player.selected_weapon != 0 &&
		 levels_won[player.selected_weapon - 1] == 0);
	}
      
      
      /* Update window: */
      
      XCopyArea(display, backbuffer, window, whitegc, 0, 0, WIDTH, HEIGHT,
		0, 0);


      /* Draw Bobo: */
      
      drawobject(64, player.selected_weapon * 24 + 24 - 8, OBJ_MAN_RIGHT_WALK1,
		 window);
      
      
      /* Play music: */
      
#ifdef MUSIC
      if (no_music == 0)
	{
	  if (!Mix_PlayingMusic())
	    {
	      Mix_PlayMusic(songs[level], 1); 
	    }
	}
#endif


      /* Keep framerate exact: */
      
      XFlush(display);
      
      gettimeofday(&now, NULL);
      
      time_padding = FRAMERATE - ((now.tv_sec - then.tv_sec) * 1000000 +
				  (now.tv_usec - then.tv_usec));
      
      if (time_padding > 0)
	usleep(time_padding);
    }
  while (done == 0);


#ifdef JOYSTICK
  do
    {
      read(js_fd, &js, JS_RETURN);
    }
  while (js.buttons != 0);
#endif
  
  playsound(SND_WEAPON);
}


/* Plays a sound: */

void playsound(int which)
{
#ifdef SOUND
  int i, found;
  
  if (dont_play == 0 && no_sound == 0)
    {
      /* Pick a quiet channel: */
      
      found = -1;
      
      for (i = 0; i < MIX_CHANNELS; i++)
	{
	  if (!Mix_Playing(i))
	    found = i;
	}
      
      
      /* Are they all playing?  Pre-empt a random one: */
      
      if (found == -1)
	found = randnum(MIX_CHANNELS);

      
      /* Play the sound: */
      
      Mix_PlayChannel(found, samples[which], 0);
    }
#endif
}
