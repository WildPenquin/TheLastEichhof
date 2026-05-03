/* Allegro port Copyright (C) 2014 Gavin Smith. */
#include <allegro.h>

// Header file for sound support module.


#define SND_PACKED4		0x0001
struct sndstrc {

  short priority;		// Priority of sample.
  short samplerate;		// Sample rate in kHz.
  short flags;			// Raw or packed sample?
  long len;			// Sample len in bytes.
  unsigned char *data;		// Sampled data.

};

void create_sndstrc (struct sndstrc *s, unsigned char *data);
SAMPLE *create_SAMPLE (struct sndstrc *s);

enum pantype{
  FIXED,
  ARRAY,//custom array e.g. for boss entry
  SINUSOIDAL,
  TRACKING
};

struct playingvoicestrc{
  int playing;
  int paninst[90];//panning value array
  int pan_slow; // divide FPS with
  enum pantype pantype;
  int pancount; // counter for pan array position, -1 = unititialized
  int *locT; // tracking location, pointer
  int sizeT; // need size - some objects are very large (resulting bias to left)
};

struct playingvoicestrc playingvoices[16];

int initsound (int snd_io, int snd_irq, int snd_dma);
void shutsound (void);
int playsample_moving (SAMPLE * snd, int pan, bool loop, int time, int loc, int *locT );
int playsample_pan (SAMPLE * snd, int pan, bool loop);
int playsample_tracking(struct playingvoicestrc *current, SAMPLE *snd, bool loop, int init_pan, int *locT, int sizeT);
int playsample (SAMPLE * snd);
int playloop (SAMPLE * snd);
void playfile (int filvar, void *buffer, int bs);
void playfileloop (int filvar, void *buffer, int bs);
void haltsound (void);
int soundbusy (void);
bool speaker (int state);
bool panxplosion (int state);
bool panfoesound (int state);
// for tracking a panning sound location
int add_panningsound(struct playingvoicestrc *current, int allocated_voice, int *locT, int sizeT);
