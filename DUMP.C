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

extern uint8_t progress;
size_t num_equals = 0;

// Printing progress will inevitably slow things down a bit if the
// delay introduced by the digest function and IO is too small
void print_progress(ulongint current, ulongint total) {
  char num_str[MAX_LEN_UINT32_STR + 1];
  float percent = (float)current/total;
  size_t num_equals_new = (size_t)(percent * BAR_WIDTH);
  int i;
  if(!current) {
    // Okay so all of this bullshit is so we can make the redrawing
    // logic simpler and save some precious run time. It's a one off
    // the first time the progress bar is drawn.
    sprintf(num_str, "%lu", total);
    printf("0");
    for(i = 0; i < strlen(num_str)-1; ++i) {
      printf(" ");
    }
    printf(" out of %lu sectors\n", total);
    // Draw bar if first time
    printf("%.02f\%   [                                                                      ]", 100*percent);
    gotoxy(1, wherey()-1);
  } else {
    gotoxy(1, wherey()-1);
    printf("%lu", current);
    gotoxy(1, wherey()+1);
    printf("%.02f\%", 100*percent);
    if(num_equals_new != num_equals) {
      // Only write to the screen if we need to update
      gotoxy(10 + num_equals, wherey());
      for(i = num_equals; i < num_equals_new; ++i) {
        printf("=");
      }
      if(i != BAR_WIDTH) {
        printf(">");
      }
      num_equals = num_equals_new;
    }
  }
}

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
  if(progress) {
    print_progress(ld->current_sector, ld->num_sectors);
  }
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
    if(progress) {
      print_progress(ld->current_sector, ld->num_sectors);
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
  if(progress) {
    print_progress(dd->current_sector, dd->num_sectors);
  }
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
    if(progress) {
      print_progress(dd->current_sector, dd->num_sectors);
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
