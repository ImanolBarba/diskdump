/***************************************************************************
 *   FILE.H  --  This file is part of diskdump.                            *
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

#ifndef _FILE_H
#define _FILE_H

#include "medium.h"
#include "digest.h"
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>

#define MAX_EXT_SEQUENCE 52
#define MAX_PATH_LENGTH 64
#define DUMP_FILE_NAME_LENGTH 8
#define MAX_BYTES_TO_WRITE 0xFFFF // This goes to a 16bit register
#define DEFAULT_FILE_SIZE 1474560

#define DOS_FILE_RD        0x00
#define DOS_FILE_WR        0x01
#define DOS_FILE_RDWR      0x02
#define DOS_FILE_COMPAT    0x00
#define DOS_FILE_EXCL      0x10
#define DOS_FILE_EXCL_WR   0x20
#define DOS_FILE_EXCL_RD   0x30
#define DOS_FILE_EXCL_NONE 0x40
#define DOS_FILE_INHERIT   0x00
#define DOS_FILE_PRIVATE   0x80


typedef struct file_medium_data {
  const char* target_directory;
  uint8_t current_extension_indexes[3];
  uint16_t fd;
  ulongint file_size;
} file_medium_data;

int create_file_medium(const char* target_directory, ulongint file_size, Medium* m, file_medium_data* fmd, Digest* digest);

#endif
