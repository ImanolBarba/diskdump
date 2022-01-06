/***************************************************************************
 *   MD5.C  --  This file is part of diskdump.                             *
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

#include "md5.h"

// s specifies the per-round shift amounts
uint32_t s[64] = { 
  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
};

uint32_t K[64] = {
  0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE,
  0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
  0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE,
  0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
  0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
  0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
  0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED,
  0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
  0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C,
  0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
  0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
  0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
  0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039,
  0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
  0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1,
  0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391
};

void get_md5_hash_string(MD5* hash, char hash_str[MD5_STR_LENGTH + 1]) {
  uint8_t* hash_word;
  int i;
  hash_word = (uint8_t*)hash;
  for(i = 0; i < MD5_LENGTH; ++i) {
    sprintf(hash_str + (i*2), "%02X", *(hash_word + i));
  }
}

void create_md5_digest(Digest* d, md5_digest_data* mdd) {
  mdd->hash_state.a0 = 0x67452301;
  mdd->hash_state.b0 = 0xEFCDAB89;
  mdd->hash_state.c0 = 0x98BADCFE;
  mdd->hash_state.d0 = 0x10325476;
  mdd->data_len[0] = 0;
  mdd->data_len[1] = 0;
  d->digest = &digest_md5;
  d->data = (void*)mdd;
  d->finish = &finish_md5;
}

void do_md5(uint8_t far* data, ulongint data_len, MD5* hash_state) {
  ulongint i;

  if(data_len % 64) {
    printf("Data must have a length multiple of 64 bytes\n");
    return;
  }

  for(i = 0; i < (data_len >> 6); ++i) {
    uint32_t A = hash_state->a0;
    uint32_t B = hash_state->b0;
    uint32_t C = hash_state->c0;
    uint32_t D = hash_state->d0;
    ulongint j;
    uint32_t far* M;
    M = (uint32_t far*)(data + (i << 6));
    for(j = 0; j < 64; ++j) {
      ulongint F;
      ulongint g;
      if(j < 16) {
        F = (B & C) | ((~B) & D);
        g = j;
      } else if(j < 32) {
        F = (D & B) | ((~D) & C);
        g = (5*j + 1) % 16;
      } else if(j < 48) {
        F = B ^ C ^ D;
        g = (3*j + 5) % 16;
      } else {
        F = C ^ (B | (~D));
        g = (7*j) % 16;
      }
      F = F + A + K[j] + M[g];
      A = D;
      D = C;
      C = B;
      B = B + LEFTROTATE(F, s[j]);
    }
    hash_state->a0 += A;
    hash_state->b0 += B;
    hash_state->c0 += C;
    hash_state->d0 += D;
  }
}

void digest_md5(uint8_t far* data, ulongint data_len, digest_data hash_data) {
  md5_digest_data* mdd = (md5_digest_data*)hash_data;

  if((mdd->data_len[0] + data_len) < mdd->data_len[0]) {
    mdd->data_len[1]++; 
  }
  mdd->data_len[0] += data_len;

  do_md5(data, data_len, &(mdd->hash_state));
}

void finish_md5(digest_data hash_data) {
  md5_digest_data* mdd = (md5_digest_data*)hash_data;
  uint8_t padding[MAX_PADDING];
  uint32_t shift_carry = mdd->data_len[0] >> 29;

  mdd->data_len[0] <<= 3;
  mdd->data_len[1] <<= 3;
  mdd->data_len[1] |= shift_carry;

  padding[0] = 0x80;
  memset(padding + 1, 0x00, 55);
  memcpy(padding + 56, mdd->data_len, sizeof(ulongint)*2);
  
  do_md5(padding, MAX_PADDING, &(mdd->hash_state));
}
