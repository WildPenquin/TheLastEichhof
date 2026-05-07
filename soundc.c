/* Allegro port Copyright (C) 2014 Gavin Smith. */
#include "sound.h"
#include <stdio.h>
#include "beerconfig.h"
#include "adpcm_decode.h"

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

  // adpcm decoding moved to adpcm_decode.c
  if (s->flags & SND_PACKED4) {
    sample->data = adpcm4decode_ban(s->data, sample->len);
    sample->len = 2 * sample->len;
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
