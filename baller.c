/* Allegro port Copyright (C) 2014 Gavin Smith. */
 /*-----------------------------------------------------*/
/*                                                       */
/*            T H E   L A S T   E I C H H O F            */
/*                                                       */
/*           [c] copyrigth 1993 by ALPHA-HELIX           */
/*          This module written by Dany Schoch		 */
/*                                                       */
 /*-----------------------------------------------------*/

#define MAIN_MODULE

#include <allegro.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <linux/limits.h>

#include "xmode.h"
#include "fileman.h"
#include "sound.h"
#include "baller.h"
#include "beerconfig.h"


#define MEMORYREQUIRED		500000L	// Memory used to run.
#define CMDLEN			40	// Max length of command line.

static char cmd[CMDLEN];	// Command line given at startup.
extern unsigned _stklen;	// Programs stacklength.

// Saving when toggling fullscreen
BITMAP* pages_temp[2];

// Code.

// Error handling routines.

void error (char *text, int code, ...) {
  void powerdown (void);	// Prototype.
  va_list ap;			// Variable for variable argument list.

  va_start (ap, code);
  powerdown ();			// Leave graphics, close files, ...
  switch (code) {
  case ENOMEM:
    printf ("%s : NO MORE MEMORY !\n\n", text);
    break;
  case ENOENT:
    printf ("%s : File '%s' not found !\n\n", text, va_arg (ap, char *));
    break;
  case E2BIG:
    printf ("%s : Out of index table space.\n\n", text);
    break;
  case EINVAL:
    printf ("%s : Wrong level.\n\n", text);
    break;
  case -1:
  case EFAULT:
    printf ("James Bond quitting style !!\n\n");
    break;
  default:
    perror (text);
  }
  va_end (ap);
  exit (1);
}


/*------------------------------------------------------
Function: powerup & powerdown

powerup   : Initializes the system.
powerdown : Undoes the effect of powerup.
	    Please note that powerdown should not be
	    called within powerup or the sytem will
	    hang. So errors encountered in powerup
	    can't be handled by the function 'error',
	    because this would call powerdown indirectly.
------------------------------------------------------*/

void powerup (void) {
  allegro_init ();

  install_keyboard ();

  if (install_sound (DIGI_AUTODETECT, MIDI_NONE, 0)) {
    printf ("%s\n", allegro_error);
    exit (2);
  }

  // Clear NUM_LOCK, SCROLL_LOCK and CAPS_LOCK.
  set_leds (0);

  install_timer ();
  setspeed (GAMESPEED);

  set_window_title(eichcfg.misc.windowtitle);
  printf("It is %s\n", eichcfg.misc.windowtitle);
  initxmode ();			// Enter graphic mode.
  windowy1 = BARY;		// Game window y-Size.
  create_pages ();
  page = 0;

}


void create_pages (void) {
  bool vignetted = true; // letterboxing check WIP
  bool resverbose = eichcfg.misc.verbose; // more verbose printing of resolution and aspect handling
  struct resolution_s vignetres;
  static bool firstcreate = true; 
  int GameResX, GameResY, paddedXres, paddedYres;
  if (eichcfg.res.fullscreen) {
    vignetres.X = eichcfg.res.full.X;
    vignetres.Y = eichcfg.res.full.Y;
  } else {
    vignetres.X = eichcfg.res.window.X;
    vignetres.Y = eichcfg.res.window.Y;
  }

  GameResX = XMAX+1;
  GameResY = YMAX+1;

  float aspect = (float) vignetres.X/ vignetres.Y;
  if ( eichcfg.res.scale == STRETCH ) {
    if ( resverbose ) printf("Will use STRETCH scaling\n");
    paddedXres = GameResX;
    paddedYres = GameResY;
    aspect = (float) 16/11;
  } else if ( eichcfg.res.scale == INTEGER ) {
    int maxaspect_int = ( vignetres.X / GameResX < vignetres.Y / GameResY ) ?
      vignetres.X / GameResX :
      vignetres.Y / GameResY ;
    paddedXres = vignetres.X/maxaspect_int;
    paddedYres = vignetres.Y/maxaspect_int;
    if ( resverbose ) printf("Will use INTEGER scaling with int scaler %i\n", maxaspect_int);
  } else {  // ASPECT corrected (default)
    paddedXres = aspect * GameResY;
    paddedYres = GameResX / aspect; 
    if ( aspect > (float) GameResX/GameResY ) {
      if (resverbose) printf("Creating vignetted window (pillarboxed)\n");
      paddedYres = GameResY;
    } else if (aspect == (float) GameResX/GameResY) {
      if (resverbose) printf("Creating NON-vignetted window\n");
    } else {
      if (resverbose) printf("Creating vignetted window (letterboxed)\n");
      paddedXres = GameResX;
    }
  }

  if ( resverbose) printf("Paddedresvals %i, %i, aspect %f, orig aspect %f \n", paddedXres, paddedYres, aspect, (float) GameResX/GameResY);

  if ( resverbose ) printf("Paddedresvals when createing are %i, %i, aspect %f\n", paddedXres, paddedYres, aspect);
  vignet_pages[0] = create_system_bitmap (paddedXres, paddedYres);
  clear_bitmap(vignet_pages[0]);
  vignet_pages[1] = create_system_bitmap (paddedXres, paddedYres);
  clear_bitmap(vignet_pages[1]);
  full_pages[0] = create_sub_bitmap (vignet_pages[0], (paddedXres - GameResX) / 2, (paddedYres - GameResY) / 2 , GameResX, GameResY);
  full_pages[1] = create_sub_bitmap (vignet_pages[1], (paddedXres - GameResX) / 2, (paddedYres - GameResY) / 2 , GameResX, GameResY);

  pages[0] = create_sub_bitmap (full_pages[0], 0, 0, windowx1, windowy1);
  pages[1] = create_sub_bitmap (full_pages[1], 0, 0, windowx1, windowy1);
  if ( pages_temp[0] && pages_temp[1] ) { // FS was toggled, we need to restore bitmaps
    blit(pages_temp[0], full_pages[0], 0, 0, 0, 0, GameResX, GameResY);
    blit(pages_temp[1], full_pages[1], 0, 0, 0, 0, GameResX, GameResY);
    destroy_bitmap(pages_temp[0]);
    destroy_bitmap(pages_temp[1]);
  }
  firstcreate = false;
  if ( resverbose ) printf("Pages created - window %i, %i\n", windowx1, windowy1);
}

void destroy_pages (void) {
  int GameResX = XMAX+1;
  int GameResY = YMAX+1;
  if ( full_pages[0] && full_pages[1] ) { // we have game bitmap pages to save!
    pages_temp[0] = create_bitmap (GameResX, GameResY);
    pages_temp[1] = create_bitmap (GameResX, GameResY);
    blit(full_pages[0], pages_temp[0], 0, 0, 0, 0, GameResX, GameResY);
    blit(full_pages[1], pages_temp[1], 0, 0, 0, 0, GameResX, GameResY);
  }
  destroy_bitmap(pages[0]);
  destroy_bitmap(pages[1]);
  destroy_bitmap(full_pages[0]);
  destroy_bitmap(full_pages[1]);
  destroy_bitmap(vignet_pages[0]);
  destroy_bitmap(vignet_pages[1]);

}

void powerdown (void) {
  shutxmode ();

  shutsound ();
  shutfilemanager ();
}

// cmdline handling (elsewhere):
struct resolution_s wincandres={0,0};
struct resolution_s fullcandres={0,0};
static bool toggletruefullscreen = false;

/*------------------------------------------------------
Function: cmdline

Description: Do first steps on the command line.
------------------------------------------------------*/
void cmdline (int argc, char *argv[]) {
  int i;
  char *e1;

  void printhelpstring() {
    printf ("Syntax:           BALLER [options]\n");
    printf ("  /ns             Play without sound.\n");
    printf ("  -x 960 -y 660   set window resolution,\n");
    printf ("  -X 3440 -Y 1440 set another resolution,\n");
    printf ("  -r              Reset / auto-detect window resolution.\n");
    printf ("  -R              Reset / auto-detect another resolution.\n");
    printf ("  -s a|i|s        Use aspect (default), integer or stretch scaling.\n");
    printf ("  -v              Toggle verbosity to STDOUT.\n");
    printf ("  -l              Toggle speedrunning lap times to STDOUT.\n");
    printf ("\n");
    printf
      ("To force SoundBlaster on, use the BLASTER environment variable.\n");
    printf ("  e.g. set BLASTER = A220 I7 D1\n");
    printf ("\n\n");
  }

// Concatenate all command strings together in 'cmd'.
  cmd[0] = '\0';		// Clear string.
  for (i = 1; i < argc; i++) {
    if (strlen (cmd) + strlen (argv[i]) < CMDLEN)
      strcat (cmd, argv[i]);
  }
  strupr (cmd);

  int c;
  while ((c = getopt(argc, argv, ":x:y:X:Y:rRfs:vl")) != -1) {
    switch(c) {
      case 'x':
        eichcfg.res.window.X = atoi(optarg);
        break;
      case 'y':
        eichcfg.res.window.Y = atoi(optarg);
        break;
      case 'X':
        eichcfg.res.full.X = atoi(optarg);
        break;
      case 'Y':
        eichcfg.res.full.Y = atoi(optarg);
        break;
      case 'f':
        eichcfg.res.truefullscreen = !eichcfg.res.truefullscreen;
        break;
      case 'r':
        eichcfg.res.window.X = eichcfg.res.window.Y = 0;
        break;
      case 'R':
        eichcfg.res.full.X = eichcfg.res.full.Y = 0;
        break;
      case 's':
        if ( strcmp(optarg, "a" ) == 0 ) eichcfg.res.scale = ASPECT;
        else if ( strcmp(optarg, "s" ) == 0 ) eichcfg.res.scale = STRETCH;
        else if ( strcmp(optarg, "i" ) == 0 ) eichcfg.res.scale = INTEGER;
        else printf("Unknown scaling parameter, ignored (%s)\n", optarg);
        break;
      case 'v':
        eichcfg.misc.verbose =!eichcfg.misc.verbose;
        printf("Verbose output toggled: %s\n", eichcfg.misc.verbose ? "ON " :"OFF");
        break;
      case 'l':
        eichcfg.misc.speedrun=!eichcfg.misc.speedrun;
        printf("Speedrun output toggled: %s\n", eichcfg.misc.speedrun ? "ON " :"OFF");
        break;
      case '?':
                    fprintf(stderr,
                "Unrecognized option: '-%c'\n", optopt);
    }
  }

// Help?
  if (strstr (cmd, "/?") || strstr (cmd, "-?") || strstr(cmd, "-h") ) {	// Help?
    printhelpstring();
    exit (0);			// Exit nicely.
  }

  // Get cheat level.

  if ((e1 = strstr (cmd, "007.")) == NULL) {
    cheatlevel = 0;		// No cheating this time.
  } else {
    cheatlevel = e1[4] - '0';
    printf ("Cheat level:\n");
    if (cheatlevel & CHEATLIFES)
      printf ("    - Unlimited lives\n");
    if (cheatlevel & CHEATMONEY)
      printf ("    - Unlimited money\n");
    if (cheatlevel & CHEATCRASH)
      printf ("    - Eichhof can't be destroyed\n");
    printf ("\n");
  }

  if ((e1 = strstr (cmd, "42069.")) == NULL) {
    cheatstage = 0; // start at the default level
  } else {
    cheatstage = e1[6] - '0'; //
    if (!(cheatstage ^ 4)) cheatstage = 4;
    else if (!(cheatstage ^ 3)) cheatstage = 3;
    else if (!(cheatstage ^ 2)) cheatstage = 2;
    else if (!(cheatstage ^ 1)) cheatstage = 1;
    else cheatstage = 0;
    printf ("Cheat - will start at level: %i\n", cheatstage);
  }
  printf ("Starting version %s\n", VERSION);

}

int main (int argc, char *argv[]) {
#if 0
  printf ("\n\n\nTHE LAST EICHHOF [c] copyright 1993 by ALPHA-HELIX.\n");
  printf ("Release 1.1\n\n");
  printf ("This game is FREEWARE. Please copy like crazy.\n");

  /* Don't know if this is still valid. */
  printf ("If you like it, just send a postcard to ALPHA-HELIX\n");
  printf ("                                        Rehhalde 18\n");
  printf ("                                        6332 Hagendorn\n");
  printf ("                                        Switzerland\n");
  printf ("\nInternet contact address: tritone@ezinfo.vmsmail.ethz.ch\n\n\n");
#endif

// No point in continuing if can not find files!
  findbeerfiles();

// Load options of last time.
  loadconfig ();

// Process command line.
  cmdline (argc, argv);

  saveconfig();
// Do initialization.
  powerup ();

// Check command line for 'nosound' option.
  if (strstr (cmd, "/NS") || strstr (cmd, "-NS"))
    speaker (0);


// Open date bases.
  initfilemanager (40, 512, 8192, error);
  datapool = opendatabase (findbeerfile (BEER_DATAFILE));

  intro ();			// Show Blick intro.

  menu ();

  closedatabase (datapool);

// Going down and return to OS.
  powerdown ();
  printf ("QUIT\n");
  return 0;
}
