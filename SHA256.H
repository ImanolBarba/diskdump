/***************************************************************************
 *   SHA256.H  --  This file is part of diskdump.                          *
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

#ifndef _SHA256_H
#define _SHA256_H

#include "digest.h"
#include "types.h"

#include <dos.h>
#include <stdio.h>
#include <string.h>

#define SHA256_LENGTH 32
#define SHA256_STR_LENGTH SHA256_LENGTH * 2 
#define MAX_PADDING 64 

#define RIGHTROTATE(x, c) (((x) >> (c)) | ((x) << (32 - (c))))
#define BSWAP_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |                      \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

typedef struct SHA256 {
  uint32_t h0;
  uint32_t h1;
  uint32_t h2;
  uint32_t h3;
  uint32_t h4;
  uint32_t h5;
  uint32_t h6;
  uint32_t h7;
} SHA256;

typedef struct sha256_digest_data {
  SHA256 hash_state;
  uint32_t data_len[2];
} sha256_digest_data;

void create_sha256_digest(Digest* d, sha256_digest_data* sdd);
void get_sha256_hash_string(SHA256* hash, char hash_str[SHA256_STR_LENGTH + 1]);
void digest_sha256(uint8_t far* data, ulongint data_len, digest_data hash_data);
void finish_sha256(digest_data hash_data);

#endif

