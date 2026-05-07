/////////////////////////
/// THE LAST EICHHOFF ///
/// ADPCM decoding    ///
///                   ///
/// Information and code adapted from
/// DOSBox staging and Vogons forums
///                   ///
/// License: GPL-v3   ///
///                   ///
/// Author: Ville Aakko aka Wild Penguin
///                   ///
/// Adapted from DOSBox staging and
/// the Vogons forums - both were useful in
/// achieving (I hope) correct ADPCM decoding
///                   ///
/// Both of the algorithms here (should!)
/// produce bit-for-bit identical
/// results.          ///
///                   ///
/// Oh, someone might ask why does this
/// text poke out of this frame like this?
///                   ///
/// Well, I just think it looks cool,
/// kind of like some pseudo-3D effect,
/// text poking out of the frame.
///                   ///
/////////////////////////
////////////////////////


#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "adpcm_decode.h"

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

#define ADPCM_INIT 0x7f // value to init to. Could be anything
                        // I've noticed 127 produces least clicks
                        // ADPCM samples tend to have end/start clicks anyways

  /* This is something called 8 to 4 bit ADPCM */
  /* Originally, the decoding algorithm was taken from */
  /* http://wiki.multimedia.cx/index.php?title=Creative_8_bits_ADPCM */

// /*********************************************************************************
//  * UPDATED to adapt information from Dosbox Staging source -> */
//  * https://github.com/dosbox-staging/dosbox-staging/blob/main/src/hardware/audio/soundblaster.cpp */
//  *********************************************************************************/
unsigned char *adpcm4decode_dbs(unsigned char *data, int len) {

  unsigned char *new_sample = malloc (2 * (len ));
  int byte = ADPCM_INIT;
  unsigned char value;
  int map_i;
  int step = 0;

  for (int i = 0; i < len; i++) {
    for ( int j = 1; j >= 0 ; j-- ) { // low<>high bits
      // higher or lower 4 bits
      value = j == 0 ? data[i] & 0x0f : ( data[i] & 0xf0 ) >> 4;
      map_i = value + step;
      // clamp to max_i = 63 (len-1)
      map_i = map_i < 0 ? 0 : map_i > 63 ? 63 : map_i;
      step = ( step + sndsb_adpcm_4bit_adjustmap[map_i]) & 0xff; // why this AND? drop >8 bits?
      byte = ( byte + sndsb_adpcm_4bit_scalemap[map_i] );
      // clamp to 0 ... 255
      byte = byte < 0 ? 0 : byte > 255 ? 255 : byte;
      new_sample[0] = byte;
      new_sample++;
    }
  }
  new_sample-=2 * ( len - 0 ) ;
  return new_sample;
}

unsigned char *adpcm4decode_ban(unsigned char *data, int len) {

  unsigned char *new_sample = malloc (2*len);
  int accum = 1;
  int lastsample = ADPCM_INIT;

  for (int i = 0; i < len; i++) {
    for (int j=1; j>=0; j--) {
      unsigned char value = j == 0 ? data[i] & 0x0f : ( data[i] & 0xf0 ) >> 4;
      new_sample[0] = DecodeADPCM4(value , lastsample, &accum);
      lastsample=new_sample[0];
      new_sample++;
    }
  }
  new_sample-=2*len;
  return new_sample;
}

// /*********************************************************************************
//  * Decoder adapted from:
//  * https://www.vogons.org/viewtopic.php?t=93707
//  * by bananaboy
//  *********************************************************************************/
uint8_t DecodeADPCM4(uint8_t data, int sample, int *accum) {

  uint8_t halfAccum = (int) *accum >> 1;
  uint8_t value = data & 7;
  uint8_t delta = (value * (*accum)) + halfAccum;

  if (data & 8) {
    // data is negative
    sample = sample - delta;
  } else {
    // data is positive
    sample = sample + delta;
  }

  // Clamp to 0...255
  sample = sample < 0 ? 0 : sample > 255 ? 255 : sample;

  if (value == 0) {
    *accum = halfAccum;
    if (*accum == 0) *accum = 1;
  }

  else if (value >= 5) {
    *accum <<= 1;
    if (*accum == 0x10) *accum = 8;
  }

  return sample;
}
