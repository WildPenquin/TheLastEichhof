/* Allegro port Copyright (C) 2014 Gavin Smith. */
// Intro [c] Alpha-Helix 1992
// written by Dany Schoch

#include <allegro.h>

#include "xmode.h"
#include "sound.h"
#include "fileman.h"
#include "baller.h"
#include "beerconfig.h"


#define SCROLLY			311

#define BYTESPERLINE		80
#define SCREENSIZE		28000

BITMAP *bmp;
// BITMAP *introvignette;
int xoutsize, xoffset;
int introYres, introXres; // fullscreen or window

static int loadEGA (char *file) {

  unsigned char *data;
  int dataptr;
  int count, i, page;
  int x, x2, y, p;

  bmp = create_bitmap (640, 350);
  clear_bitmap (bmp);

  data = loadfile (datapool, file);
  dataptr = 0;

  count = 0;
  int inc = 0;

  int r, g, b, br, pal = 0;
  /* Set EGA 3-bit palette. */
  for (br = 0; br <= 1; br++) {
    for (r = 0; r <= 1; r++) {
      for (g = 0; g <= 1; g++) {
	for (b = 0; b <= 1; b++) {
	  RGB rgb;
	  rgb.r = r ? 32 : 0;
	  rgb.g = g ? 32 : 0;
	  rgb.b = b ? 32 : 0;

	  if (br) {
	    rgb.r += 31;
	    rgb.g += 31;
	    rgb.b += 31;
	  }

	  set_color (8 * br + 4 * r + 2 * g + b, &rgb);
	}
      }
    }
  }

  RGB rgb = { 15, 15, 15 };
  set_color (8, &rgb);		/* High-intensity black - dark grey */

  for (p = 1; p <= 8; p *= 2) {
    y = 0;
    x = 0;
    while (y <= 349) {
      if (count == 0) {
	count = data[dataptr++];
	if (count < 128) {
	  /* Output count pixels the same. */
	  inc = 0;
	} else {
	  count -= 128;
	  inc = 1;		/* Read and output count pixels. */
	}
	count += 1;
	continue;
      }

      /* Each byte has info for 8 pixels. */
      for (x2 = 7; x2 >= 0; x2--) {
	putpixel (bmp, x + (7 - x2), y, ((data[dataptr] & (1 << x2)) >> x2)	/* 0 or 1 */
		  *p + getpixel (bmp, x + (7 - x2), y));
      }


      count--;
      dataptr += inc;
      if (count == 0 && inc == 0)
	dataptr++;

      x += 8;
      if (x >= 640) {
	x = 0;
	y++;
      }
    }
  }

  stretch_blit(bmp, screen, 0, 0, bmp -> w, bmp -> h,
      xoffset, 0, xoutsize, SCREEN_H);
  unloadfile (data);

  return 0;
}


/* C is the character. S the column / 8.  PAPER is the font glyphs. */
static void putpaper (int c, int s, char *paper) {
  int i;
  char *ptr;
  char *data;

  c -= 32;
  data = &paper[(c * 5 + s) * 40];

  /* Each character is 32 pixels wide (4 x 8). */
  /* Not full EGA data - only has one byte (for brightness) instead of 4. */
  for (i = 0; i < 40; i++) {	/* Forty rows */
    int col;

    for (col = 7; col >= 0; col--) {
      int pixel = (*data & (1 << col)) >> col;	/* 0 or 1 */
      // putpixel (screen, 632 + (7 - col), 310 + i, pixel * 15);
      putpixel (bmp, 632 + (7 - col), 310 + i, pixel * 15);
    }
    data++;
  }
}

static void paperscroll () {
  /* Move left eight pixels. */
  blit(bmp, bmp, 8, 310, 0, 310, bmp->w-8, 40);
  stretch_blit(bmp, screen, 0, 310, bmp->w-8, 40, xoffset, 0 + 310*introYres/350, xoutsize-8*xoutsize/640, 40*introYres/350);
}


void intro (void) {
  char *paper;

  struct sndstrc snd;
  void *ptr;
  SAMPLE *s;

  if (eichcfg.res.fullscreen) {
    introXres = eichcfg.res.full.X;
    introYres = eichcfg.res.full.Y;
  } else {
    introXres = eichcfg.res.window.X;
    introYres = eichcfg.res.window.Y;
  }
  

  xoffset=0;
  xoutsize = introXres;
  float aspect = (float) introXres / introYres;
  // assuming 4/3 aspect originally (non-1:1 PAR):
  if ( aspect > (float) 4/3 ) {
    xoutsize = 4 *  introYres / 3;
    xoffset = ( introXres - (xoutsize) ) /2;
  };

  // Initialize sound (volume)
  haltsound ();

  // setvanillapalette(0);
  loadEGA ("BLICK.PAK");
  paper = loadfile (datapool, "PAPER.FNT");

  ptr = loadfile (datapool, "BLICK.SND");
  create_sndstrc (&snd, ptr);
  s = create_SAMPLE (&snd);
  playloop (s);

  //glowto(63, 63, 63);
  //glowin(1);

  {
    int l, c;
    char text[] =
      " . . . ALPHA HELIX PRODUCTION. (C) COPYRIGHT 1993.   "
      "PROGRAMMING, SOUND FX AND LEVEL DESIGN BY TRITONE.   "
      "GRAPHICS AND FONTS BY TWEETY.   "
      "ADDITIONAL PROGRAMMING AND MUSIC BY ZYNAX OF DARK SYSTEM.   ";

    c = 0;
    l = 0;

    clear_keybuf ();
    do {
      putpaper (text[c], l, paper);
      if (++l > 4) {
	l = 0;
	c++;
      }
      if (text[c] == 0)
	c = 0;
      vsync ();
      paperscroll ();
    } while (!keypressed ());
  }

  
  glowout ();
  stop_sample (s);
  unloadfile (ptr);

  unloadfile (paper);
  clearscreen();
  destroy_bitmap(bmp);


// All this mode switching is a little bit a 'murks'. I know.
}
