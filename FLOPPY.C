/***************************************************************************
 *   FLOPPY.C  --  This file is part of diskdump.                          *
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

#include "floppy.h"

ssize_t floppy_medium_send(uint8_t far *buf, ulongint buf_len, medium_data md) {
  floppy_medium_data* fmd = (floppy_medium_data*)md;
  ssize_t bytes_written;
  if(buf_len % fmd->ld.sector_size != 0) {
    printf("Length of buffer must be a multiple of sector size\n");
    return -1;
  }
  bytes_written = write_drive_chs(&(fmd->ld), buf, buf_len/fmd->ld.sector_size);
    printf("Insert new floppy...\n");
    system(pause);
  
// TODO
}

int floppy_medium_ready(medium_data md) {
  int status;
  floppy_medium_data* fmd = (floppy_medium_data*)md;
  legacy_descriptor ld;
  ld->drive_num = fmd->ld.drive_num;
  do {
    status = get_drive_data_floppy(&ld);
    if(status) {
      printf("Floppy disk not ready. Insert floppy and retry\n");
      system(pause);
    }
  }while(status);
  return 1;
}

int create_floppy_medium(Medium* m, floppy_medium_data* fmd) {
  m->send = &floppy_medium_send;
  m->ready = &floppy_medium_ready;
  m->data = (void*)fmd;
  return m->ready(m->data);
}

 
