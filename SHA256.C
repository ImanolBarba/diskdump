/***************************************************************************
 *   SHA256.C  --  This file is part of diskdump.                          *
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

#include "sha256.h"

uint32_t k[64] = {
  0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 
  0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
  0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 
  0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
  0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 
  0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
  0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 
  0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
  0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 
  0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
  0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 
  0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
  0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 
  0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
  0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 
  0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};
   
void get_sha256_hash_string(SHA256* hash, char hash_str[SHA256_STR_LENGTH + 1]) {
  uint8_t* hash_word;
  int i;
  SHA256 hash_le;
  hash_word = (uint8_t*)&hash_le;
  hash_le.h0 = BSWAP_32(hash->h0);
  hash_le.h1 = BSWAP_32(hash->h1);
  hash_le.h2 = BSWAP_32(hash->h2);
  hash_le.h3 = BSWAP_32(hash->h3);
  hash_le.h4 = BSWAP_32(hash->h4);
  hash_le.h5 = BSWAP_32(hash->h5);
  hash_le.h6 = BSWAP_32(hash->h6);
  hash_le.h7 = BSWAP_32(hash->h7);
  for(i = 0; i < SHA256_LENGTH; ++i) {
    sprintf(hash_str + (i*2), "%02X", *(hash_word + i));
  }
}

void create_sha256_digest(Digest* d, sha256_digest_data* sdd) {
  sdd->hash_state.h0 = 0x6A09E667;
  sdd->hash_state.h1 = 0xBB67AE85;
  sdd->hash_state.h2 = 0x3C6EF372;
  sdd->hash_state.h3 = 0xA54FF53A;
  sdd->hash_state.h4 = 0x510E527F;
  sdd->hash_state.h5 = 0x9B05688C;
  sdd->hash_state.h6 = 0x1F83D9AB;
  sdd->hash_state.h7 = 0x5BE0CD19;
  sdd->data_len[0] = 0;
  sdd->data_len[1] = 0;
  d->digest = &digest_sha256;
  d->data = (void*)sdd;
  d->finish = &finish_sha256;
}

void do_sha256(uint8_t far* data, ulongint data_len, SHA256* hash_state) {
  ulongint i;
  
  if(data_len % 64) {
    printf("Data must have a length multiple of 64 bytes\n");
    return;
  }
  for(i = 0; i < (data_len >> 6); ++i) {
    ulongint j;
    uint32_t w[64];
    uint32_t far *target_data = (uint32_t far*)(data + (i << 6));
    uint32_t a = hash_state->h0;
    uint32_t b = hash_state->h1;
    uint32_t c = hash_state->h2;
    uint32_t d = hash_state->h3;
    uint32_t e = hash_state->h4;
    uint32_t f = hash_state->h5;
    uint32_t g = hash_state->h6;
    uint32_t h = hash_state->h7;
    uint32_t S0;
    uint32_t S1;
    uint32_t temp1;
    uint32_t temp2;
    uint32_t ch;
    uint32_t maj;  

    movedata(FP_SEG(target_data), FP_OFF(target_data), FP_SEG(w), FP_OFF(w), 64);
    for(j = 0; j < 64; ++j) {
      if(j >= 16) {
        w[j] = w[j-16] + (RIGHTROTATE(w[j-15], 7) ^ RIGHTROTATE(w[j-15], 18) ^ (w[j-15] >> 3)) + w[j-7] + (RIGHTROTATE(w[j-2], 17) ^ RIGHTROTATE(w[j-2], 19) ^ (w[j-2] >> 10)); 
      } else {
        w[j] = BSWAP_32(w[j]);
      }
      S1 = RIGHTROTATE(e, 6) ^ RIGHTROTATE(e, 11) ^ RIGHTROTATE(e, 25);
      ch = (e & f) ^ ((~e) & g);
      temp1 = h + S1 + ch + k[j] + w[j];
      S0 = RIGHTROTATE(a, 2) ^ RIGHTROTATE(a, 13) ^ RIGHTROTATE(a, 22);
      maj = (a & b) ^ (a & c) ^ (b & c);
      temp2 = S0 + maj;

      h = g;
      g = f;
      f = e;
      e = d + temp1;
      d = c;
      c = b;
      b = a;
      a = temp1 + temp2;
    }    
    hash_state->h0 += a;
    hash_state->h1 += b;
    hash_state->h2 += c;
    hash_state->h3 += d;
    hash_state->h4 += e;
    hash_state->h5 += f;
    hash_state->h6 += g;
    hash_state->h7 += h;
  } 
}

void digest_sha256(uint8_t far* data, ulongint data_len, digest_data hash_data) {
  sha256_digest_data* sdd = (sha256_digest_data*)hash_data;

  if((sdd->data_len[0] + data_len) < sdd->data_len[0]) {
    sdd->data_len[1]++; 
  }
  sdd->data_len[0] += data_len;

  do_sha256(data, data_len, &(sdd->hash_state));
}

void finish_sha256(digest_data hash_data) {
  sha256_digest_data* sdd = (sha256_digest_data*)hash_data;
  uint8_t padding[MAX_PADDING];
  uint32_t shift_carry = sdd->data_len[0] >> 29;

  uint32_t data_len_lo_be = BSWAP_32(sdd->data_len[0] << 3);
  uint32_t data_len_hi_be = BSWAP_32((sdd->data_len[1] << 3) | shift_carry);

  padding[0] = 0x80;
  memset(padding + 1, 0x00, 55);
  memcpy(padding + 56, &data_len_hi_be, sizeof(ulongint));
  memcpy(padding + 60, &data_len_lo_be, sizeof(ulongint));
  
  do_sha256(padding, MAX_PADDING, &(sdd->hash_state));
}
