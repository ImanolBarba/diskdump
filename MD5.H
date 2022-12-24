/***************************************************************************
 *   MD5.H  --  This file is part of diskdump.                             *
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

#ifndef _MD5_H
#define _MD5_H

#include "digest.h"
#include "types.h"

#include <dos.h>
#include <stdio.h>
#include <string.h>

#define MD5_LENGTH 16
#define MD5_STR_LENGTH MD5_LENGTH * 2 
#define MAX_PADDING 64 

#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))

typedef struct MD5 {
  uint32_t a0;
  uint32_t b0;
  uint32_t c0;
  uint32_t d0;
} MD5;

typedef struct md5_digest_data {
  MD5 hash_state;
  uint32_t data_len[2];
} md5_digest_data;

void create_md5_digest(Digest* d, md5_digest_data* mdd);
void get_md5_hash_string(MD5* hash, char hash_str[MD5_STR_LENGTH + 1]);
void digest_md5(uint8_t far* data, ulongint data_len, digest_data hash_data);
void finish_md5(digest_data hash_data);

#endif
