/* Allegro port Copyright (C) 2014 Gavin Smith. */
#include "sound.h"
#include <stdio.h>
#include "beerconfig.h"

/*speaker:
   state == 0  : Turn sound off.
   state == 1  : Turn sound on.
   state == -1 : Query sound. */

bool speaker (int state) {
  if (state == -1)
    return eichcfg.ss.sound;

  eichcfg.ss.sound = state ? true : false;
  if (!eichcfg.ss.sound)
    remove_sound ();
  return eichcfg.ss.sound;
}

/*pan settings for explosions and foe sounds 
 (currently not settable separately, but could be)
 * state == 0   : Turn to mono
 * state == 1   : Turn to stereo (panning according to location)
 * state == -1  : Query state */

bool panxplosion (int state) {
  if (state == -1)
    return eichcfg.ss.pan_sfx;

  eichcfg.ss.pan_sfx = state ? true : false;
  return eichcfg.ss.pan_sfx;
}

bool panfoesound (int state) {
  if (state == -1)
    return eichcfg.ss.pan_foes;

  eichcfg.ss.pan_foes = state ? true : false;
  return eichcfg.ss.pan_foes;
}


static SAMPLE *playsample_ext (struct sndstrc *s, int loop);

void shutsound (void) {
  remove_sound ();
}

/* Stop all playing sounds. */
void haltsound (void) {
  remove_sound ();
  if (eichcfg.ss.sound) {
    reserve_voices (8, 0);
    set_volume_per_voice (eichcfg.ss.scale);
    install_sound (DIGI_AUTODETECT, MIDI_NONE, 0);
  }
}

int playsample_pan (SAMPLE *s, int pan, bool loop) {
  int spl = allocate_voice (s);
  if (loop)
    voice_set_playmode (spl, PLAYMODE_LOOP);
  voice_set_pan (spl, pan);
  voice_start (spl);
  release_voice (spl);
  return spl;
}

// almost same as playsample_pan, but let's not release voice and add it panningsounds
int playsample_tracking(struct playingvoicestrc *current, SAMPLE *s, bool loop, int init_pan, int *locT, int sizeT) {
  int spl = allocate_voice(s);
  if (loop)
    voice_set_playmode (spl, PLAYMODE_LOOP);
  voice_set_pan (spl, init_pan);
  voice_start (spl);
  if ( panfoesound ) add_panningsound(current, spl, locT, sizeT);
  return spl;
}

int add_panningsound(struct playingvoicestrc *current, int playing, int *locT, int sizeT) {
  current->playing=playing;
  current->pancount=0;
  //// For an array / custom panning sound (TODO)
  // for (int p=0; p<30;p++) {
  //   current->paninst[p]=(p/10)%2;
  // }
  current->pantype=TRACKING;
  current->locT = locT;
  current->sizeT = sizeT;
  return 0;
}

int playsample (SAMPLE *s) {
  return playsample_pan (s, 127, false);
}

int playloop (SAMPLE *s) {
  return playsample_pan (s, 127, true);
}


/* Returns a pointer to SAMPLE. This value should be
   freed when no longer required. destroy_sample must
   not be called, because this attempts to free the data
   field of SAMPLE. */
SAMPLE *create_SAMPLE (struct sndstrc *s) {
  struct SAMPLE *sample = malloc (sizeof (struct SAMPLE));

  sample->bits = 8;
  sample->stereo = 0;
  sample->freq = s->samplerate * 1000;
  sample->priority = s->priority;
  sample->len = s->len;
  sample->loop_start = 0;
  sample->data = s->data;

  /* This is something called 8 to 4 bit ADPCM */
  /* Decoding algorithm taken from */
  /* http://wiki.multimedia.cx/index.php?title=Creative_8_bits_ADPCM */
  /* UPDATED to adapt information from Dosbox Staging source -> */
  /* https://github.com/dosbox-staging/dosbox-staging/blob/main/src/hardware/audio/soundblaster.cpp */
  if (s->flags & SND_PACKED4) {
    unsigned char *new_sample = malloc (2 * (sample->len) -1); // The first byte is ref, we are decoding len-1.
    int byte;
    unsigned char value;
    int i;

    int step = 0; //, shift; //  limit , sign;

    byte = s->data[0];
    new_sample[0] = byte;

    const signed char sndsb_adpcm_4bit_scalemap[64] = {
        0,  1,  2,  3,  4,  5,  6,  7,  0,  -1,  -2,  -3,  -4,  -5,  -6,  -7,
        1,  3,  5,  7,  9, 11, 13, 15, -1,  -3,  -5,  -7,  -9, -11, -13, -15,
        2,  6, 10, 14, 18, 22, 26, 30, -2,  -6, -10, -14, -18, -22, -26, -30,
        4, 12, 20, 28, 36, 44, 52, 60, -4, -12, -20, -28, -36, -44, -52, -60
    };

    const signed char sndsb_adpcm_4bit_adjustmap[64] = {
          0, 0, 0, 0, 0, 16, 16, 16,
          0, 0, 0, 0, 0, 16, 16, 16,
        240, 0, 0, 0, 0, 16, 16, 16,
        240, 0, 0, 0, 0, 16, 16, 16,
        240, 0, 0, 0, 0, 16, 16, 16,
        240, 0, 0, 0, 0, 16, 16, 16,
        240, 0, 0, 0, 0,  0,  0,  0,
        240, 0, 0, 0, 0,  0,  0,  0
    };

    for (i = 1; i < sample->len; i++) {
      for ( int j = 0; j <= 1 ; j++ ) {
        // higher or lower 4 bits
        value = j > 0 ? s->data[i] & 0x0f : ( s->data[i] & 0xf0 ) >> 4;
        int map_i = value + step;
        // clamp to max_i = 63 (len-1)
        map_i = map_i < 0 ? 0 : map_i > 63 ? 63 : map_i;
        step = ( step + sndsb_adpcm_4bit_adjustmap[map_i]) & 0xff; // why this and? drop >8 bits?
        byte = ( byte + sndsb_adpcm_4bit_scalemap[map_i] );
        // clamp to 0 ... 255
        if (byte > 0xff)
            byte = 0xff;
        if (byte < 0x00)
            byte = 0x00;
        new_sample[2*i -1 + j] = byte;
      }
    }

    sample->data = new_sample;
    sample->len = 1 + 2 * (sample->len - 1);

  }

  sample->loop_end = sample->len - 1;

  return sample;
}

/* Initialize s with the data read from the file.*/
void create_sndstrc (struct sndstrc *s, unsigned char *data) {
  s->priority = data[0] + (data[1] << 8);
  s->samplerate = data[2] + (data[3] << 8);
  s->flags = data[4] + (data[5] << 8);
  s->len = data[6] + (data[7] << 8) + (data[8] << 16)
    + (data[9] << 24);
  s->data = data + 10;
}
