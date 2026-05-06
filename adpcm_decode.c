/////////////////////////
/// THE LAST EICHHOFF ///
/// ADPCM decoding    ///
///                   ///
/// Information and code adapted from
/// DOSBox staging and Vogons forums
///                   ///
///   GPL-v3          ///
///                   ///
/////////////////////////
/////////////////////////

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

  /* This is something called 8 to 4 bit ADPCM */
  /* Originally, the decoding algorithm was taken from */
  /* http://wiki.multimedia.cx/index.php?title=Creative_8_bits_ADPCM */

// /*********************************************************************************
//  * UPDATED to adapt information from Dosbox Staging source -> */
//  * https://github.com/dosbox-staging/dosbox-staging/blob/main/src/hardware/audio/soundblaster.cpp */
//  *********************************************************************************/
unsigned char *adpcm4decode_dbs(unsigned char *data, int len) {

    unsigned char *new_sample = malloc (2 * (len ));
    int byte;
    unsigned char value;
    int i;
    int map_i = 0;
    int step = 0; //, shift; //  limit , sign;

    byte = 0x7f;//  data[0]; // 0x88;
    // new_sample[0] =0x7f;

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

    int lastsample= 0x7f; // data[0] ; // 0x88; // just for the first byte needed
    for (int i = 0; i < len; i++) {
      for (int j=1; j>=0; j--) {
        unsigned char value = j == 0 ? data[i] & 0x0f : ( data[i] & 0xf0 ) >> 4;
        new_sample[0] = DecodeADPCM4(value , lastsample);
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
uint8_t DecodeADPCM4(uint8_t data, int sample) {

  static int accum = 1;
  uint8_t halfAccum = (int) accum >> 1;
  uint8_t value = data & 7;
  uint8_t delta = (value * accum) + halfAccum;

  // int sample;
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
    accum = halfAccum;
    if (accum == 0) accum = 1;
  }

  else if (value >= 5) {
    accum <<= 1;
    if (accum == 0x10) accum = 8;
  }

  return sample;
}
