/***************************************************************************
 *   SHA1.C  --  This file is part of diskdump.                            *
 *                                                                         *
 *   Copyright (C) 2021 Imanol-Mikel Barba Sabariego                       *
 *                                                                         *
 *   diskdump is free software: you can redistribute it and/or modify      *
 *   it under the terms of the GNU General Public License as published     *
 *   by the Free Software Foundation, either version 3 of the License,     *
 *   or (at your option) any later version.                                *
 *                                                                         *
 *   diskdump is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty           *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.               *
 *   See the GNU General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see http://www.gnu.org/licenses/.   *
 *                                                                         *
 ***************************************************************************/

#include "sha1.h"

void get_sha1_hash_string(SHA1* hash, char hash_str[SHA1_STR_LENGTH + 1]) {
  uint8_t* hash_word;
  int i;
  SHA1 hash_le;
  hash_word = (uint8_t*)&hash_le;
  hash_le.h0 = BSWAP_32(hash->h0);
  hash_le.h1 = BSWAP_32(hash->h1);
  hash_le.h2 = BSWAP_32(hash->h2);
  hash_le.h3 = BSWAP_32(hash->h3);
  hash_le.h4 = BSWAP_32(hash->h4);
  for(i = 0; i < SHA1_LENGTH; ++i) {
    sprintf(hash_str + (i*2), "%02X", *(hash_word + i));
  }
}

void create_sha1_digest(Digest* d, sha1_digest_data* sdd) {
  sdd->hash_state.h0 = 0x67452301;
  sdd->hash_state.h1 = 0xEFCDAB89;
  sdd->hash_state.h2 = 0x98BADCFE;
  sdd->hash_state.h3 = 0x10325476;
  sdd->hash_state.h4 = 0xC3D2E1F0;
  sdd->data_len[0] = 0;
  sdd->data_len[1] = 0;
  d->digest = &digest_sha1;
  d->data = (void*)sdd;
  d->finish = &finish_sha1;
}

void do_sha1(uint8_t far* data, ulongint data_len, SHA1* hash_state) {
  ulongint i;
  
  if(data_len % 64) {
    printf("Data must have a length multiple of 64 bytes\n");
    return;
  }
  for(i = 0; i < (data_len >> 6); ++i) {
    ulongint j;
    uint32_t w[80];
    uint32_t far *target_data = (uint32_t far*)(data + (i << 6));
    uint32_t a = hash_state->h0;
    uint32_t b = hash_state->h1;
    uint32_t c = hash_state->h2;
    uint32_t d = hash_state->h3;
    uint32_t e = hash_state->h4;
    uint32_t f;
    uint32_t k;
    uint32_t temp;

    movedata(FP_SEG(target_data), FP_OFF(target_data), FP_SEG(w), FP_OFF(w), 64);
    for(j = 0; j < 80; ++j) {
      if(j >= 16) {
        w[j] = LEFTROTATE(w[j-3] ^ w[j-8] ^ w[j-14] ^ w[j-16], 1);
      } else {
        w[j] = BSWAP_32(w[j]);
      } 
      if(j < 20) {
        f = (b & c) | ((~b) & d);
        k = 0x5A827999;
      } else if(j < 40) {
        f = b ^ c ^ d;
        k = 0x6ED9EBA1;
      } else if(j < 60) {
        f = (b & c) | (b & d) | (c & d);
        k = 0x8F1BBCDC;
      } else {
        f = b ^ c ^ d;
        k = 0xCA62C1D6;
      }
      temp = LEFTROTATE(a, 5) + f + e + k + w[j];
      e = d;
      d = c;
      c = LEFTROTATE(b,30);
      b = a;
      a = temp;
    }    
    hash_state->h0 += a;
    hash_state->h1 += b;
    hash_state->h2 += c;
    hash_state->h3 += d;
    hash_state->h4 += e;
  } 
}

void digest_sha1(uint8_t far* data, ulongint data_len, digest_data hash_data) {
  sha1_digest_data* sdd = (sha1_digest_data*)hash_data;

  if((sdd->data_len[0] + data_len) < sdd->data_len[0]) {
    sdd->data_len[1]++; 
  }
  sdd->data_len[0] += data_len;

  do_sha1(data, data_len, &(sdd->hash_state));
}

void finish_sha1(digest_data hash_data) {
  sha1_digest_data* sdd = (sha1_digest_data*)hash_data;
  uint8_t padding[MAX_PADDING];
  uint32_t shift_carry = sdd->data_len[0] >> 29;

  uint32_t data_len_lo_be = BSWAP_32(sdd->data_len[0] << 3);
  uint32_t data_len_hi_be = BSWAP_32((sdd->data_len[1] << 3) | shift_carry);

  padding[0] = 0x80;
  memset(padding + 1, 0x00, 55);
  memcpy(padding + 56, &data_len_hi_be, sizeof(ulongint));
  memcpy(padding + 60, &data_len_lo_be, sizeof(ulongint));
  
  do_sha1(padding, MAX_PADDING, &(sdd->hash_state));
}

