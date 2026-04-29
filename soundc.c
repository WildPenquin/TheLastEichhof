/* Allegro port Copyright (C) 2014 Gavin Smith. */
#include "sound.h"
#include <stdio.h>

bool soundon = true;
bool sound_panxplosion = true;
bool sound_panfoesound = true;


/*speaker:
   state == 0  : Turn sound off.
   state == 1  : Turn sound on.
   state == -1 : Query sound. */

bool speaker (int state) {
  if (state == -1)
    return soundon;

  soundon = state ? true : false;
  if (!soundon)
    remove_sound ();
  return soundon;
}

/*pan settings for explosions and foe sounds
 * state == 0   : Turn to mono
 * state == 1   : Turn to stereo (panning according to location)
 * state == -1  : Query state */

bool panxplosion (int state) {
  if (state == -1)
    return sound_panxplosion;

  sound_panxplosion = state ? true : false;
  return sound_panxplosion;
}

bool panfoesound (int state) {
  if (state == -1)
    return sound_panfoesound;

  sound_panfoesound = state ? true : false;
  return sound_panfoesound;
}


static SAMPLE *playsample_ext (struct sndstrc *s, int loop);

void shutsound (void) {
  remove_sound ();
}

/* Stop all playing sounds. */
void haltsound (void) {
  remove_sound ();
  if (soundon) {
    reserve_voices (8, 0);
    set_volume_per_voice (3);
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
    unsigned char *new_sample = malloc (1 + 2 * (sample->len - 1));
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
        value = j > 0 ? s->data[i] & 0x0f : ( s->data[i] ) >> 4;
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
        new_sample[2*i + j] = byte;
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
