/***************************************************************************
 *   DUMP.C  --  This file is part of diskdump.                            *
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

#include "dump.h"

void list_disks() {
  drive_descriptor dd;
  legacy_descriptor ld;
  unsigned int drive_num;
  int status;
  int i;
  printf("== FLOPPY DATA ==\n");
  for(i = 0; i <= 0x7F; ++i) {
    ld.drive_num = i;
    status = get_drive_data_floppy(&ld);
    if(status) {
      break;
    }
    print_drive_data_floppy(&ld);
  }
  printf("== DRIVE DATA ==\n");
  for(i = 0x80; i <= 0xFF; ++i) {
    if(check_extensions_present(i)) {
      dd.drive_num = i;
      status = get_drive_data_disk(&dd);
      if(status) {
        break;
      }
      print_drive_data_disk(&dd);
    } else {
      ld.drive_num = i;
      status = get_drive_data_floppy(&ld);
      if(status) {
        break;
      }
      print_drive_data_floppy(&ld);
    }
  }
}

int dump_floppy_drive(legacy_descriptor* ld, Medium* m) {
  uint16_t segment, largest_block;
  ssize_t bytes_read, bytes_sent;
  uint8_t far *buf;
  uint16_t status = alloc_segment(&segment, &largest_block);
  if(status) {
    printf("Failed to allocate segment. Error: %04X.\nLargest block: %04X\n", status, largest_block);
    return -1;
  }
  buf = MK_FP(get_DMA_boundary_segment(segment), 0x0000);
  while((bytes_read = read_drive_chs(ld, buf, MAX_SECTORS_CHS)) != 0) {

    if(bytes_read < 0) {
      printf("Failed to read floppy\n");
      free_segment(segment);
      return -1;
    }
    if(m->digest) {
      m->digest->digest(buf, (ulongint)bytes_read, m->digest->data);
    }

    bytes_sent = m->send(buf, bytes_read, m->data);
    if(bytes_read != bytes_sent) {
      printf("Came short when transferring to medium :(\n");
      free_segment(segment);
      return -1;
    }
    if(!m->ready(m->data)) {
      printf("Medium not ready\n");
      free_segment(segment);
      return -1;
    }
  }
  if(m->digest) {
    m->digest->finish(m->digest->data);
  }
  free_segment(segment);
  return 0;
}

int dump_hard_drive(drive_descriptor* dd, Medium* m) {
  uint16_t segment, largest_block;
  ssize_t bytes_read, bytes_sent;
  uint8_t far *buf;
  uint16_t status;
  legacy_descriptor ld;

  if(!check_extensions_present(dd->drive_num)) {
    printf("LBA extensions not present\n");
    printf("Dumping using CHS addressing\n");
    ld.drive_num = dd->drive_num;
    status = get_drive_data_floppy(&ld);
    print_drive_data_floppy(&ld);
    if(status) {
      printf("Unable to obtain hard drive data using CHS addressing\n");
      return -1;
    }
    return dump_floppy_drive(&ld, m);
  }

  status = alloc_segment(&segment, &largest_block);
  if(status) {
    printf("Failed to allocate segment. Error: %04X.\nLargest block: %04X\n", status, largest_block);
    return -1;
  }
  buf = MK_FP(get_DMA_boundary_segment(segment), 0x0000);
  // We will only request the maximum sectors of LBA that can be read 
  // at once tops, if we go for maximum 128 we end up doing 2 transfers,
  // one for 127 sectors and 1 for 1 sector, which is inefficient.
  while((bytes_read = read_drive_lba(dd, buf, MAX_SECTORS_LBA)) != 0) {
    if(bytes_read < 0) {
      printf("Failed to read disk\n");
      free_segment(segment);
      return -1;
    }
    if(m->digest) {
      m->digest->digest(buf, (ulongint)bytes_read, m->digest->data);
    }
    bytes_sent = m->send(buf, bytes_read, m->data);
    if(bytes_read != bytes_sent) {
      printf("Came short when transferring to medium :(\n");
      free_segment(segment);
      return -1;
    }
    if(!m->ready(m->data)) {
      printf("Medium not ready\n");
      free_segment(segment);
      return -1;
    }
  }
  if(m->digest) {
    m->digest->finish(m->digest->data);
  }
  free_segment(segment);
  return 0;
}
