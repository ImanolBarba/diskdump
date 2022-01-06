/***************************************************************************
 *   DISK.H  --  This file is part of diskdump.                            *
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

#ifndef _DISK_H
#define _DISK_H

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <math.h>
#include "types.h"

#define HARD_DISK_FLAG      0x80

#define FLOPPY_TYPE_UNKNOWN 0x00
#define FLOPPY_TYPE_360K    0x01
#define FLOPPY_TYPE_1200K   0x02
#define FLOPPY_TYPE_720K    0x03
#define FLOPPY_TYPE_1440K   0x04

#define DRIVE_PARAMS_SIZE   30

#define MAX_SECTORS_CHS 128
// According to Wikipedia, some BIOSes can't read > 127 sectors
#define MAX_SECTORS_LBA 127

// Bytes per sector defs
#define BPS_128  0
#define BPS_256  1
#define BPS_512  2
#define BPS_1024 3

#define DAP_SIZE 16 

typedef uint8_t FLOPPY_TYPE;

typedef struct legacy_descriptor {
  uint16_t num_cylinders;
  uint8_t num_heads; // sides
  uint8_t sectors_per_track;
  FLOPPY_TYPE floppy_type;
  uint8_t drive_num;
  uint sector_size;
  ulongint current_sector;
  uint last_sector_offset;
  ulongint num_sectors;
} legacy_descriptor;

typedef struct drive_descriptor {
  uint8_t drive_num;
  uint32_t num_cylinders;
  uint32_t num_heads; //sides
  uint32_t sectors_per_track;
  uint16_t sector_size;
  uint32_t num_sectors;
  ulongint current_sector;
  uint last_sector_offset;
} drive_descriptor;

typedef struct DAP {
  uint8_t DAP_size;
  uint8_t unused;
  uint16_t num_sectors_to_read;
  uint8_t buffer_addr[4];
  uint8_t abs_sector_offset[8];
} DAP;

int check_extensions_present(uint8_t drive_num);
int get_drive_data_disk(drive_descriptor* dd);
int get_drive_data_floppy(legacy_descriptor* ld);
void print_drive_data_floppy(legacy_descriptor *ld);
void print_drive_data_disk(drive_descriptor *dd);
ssize_t read_drive_chs(legacy_descriptor* ld, uint8_t far *segment_buf, uint sectors);
ssize_t write_drive_chs(legacy_descriptor* ld, uint8_t far *segment_buf, uint sectors);
ssize_t read_drive_lba(drive_descriptor* dd, uint8_t far *segment_buf, uint sectors);

// Private
// ssize_t read_sectors_chs(legacy_descriptor* ld, uint8_t cyl, uint8_t head, uint8_t sect, uint8_t sectors_to_read, uint8_t far *buf);
// ssize_t read_sectors_lba(drive_descriptor* dd, DAP* d);

#endif
