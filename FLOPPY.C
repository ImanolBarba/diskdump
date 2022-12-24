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

int floppy_medium_ready(medium_data md) {
  int status;
  int retries = 0;
  floppy_medium_data* fmd = (floppy_medium_data*)md;
  legacy_descriptor ld;
  ld.drive_num = fmd->ld.drive_num;
  do {
    status = reset_floppy(&ld);
    if(status) {
      printf("Error resetting disk. Try a different floppy\n");
      system("pause");
    }
    status = get_drive_data_floppy(&ld);
    if(status) {
      printf("Floppy disk not ready. Insert floppy and retry\n");
      system("pause");
    }
  }while(status && (++retries != MAX_RETRIES));
  if(status) {
    printf("Reached maximum retries. Giving up.\n");
    return MEDIUM_NOT_READY;
  }
  return MEDIUM_READY;
}

ssize_t floppy_medium_send(uint8_t far *buf, ulongint buf_len, medium_data md) {
  floppy_medium_data* fmd = (floppy_medium_data*)md;
  ssize_t bytes_written;
  int status;
  ssize_t total_written = 0;
  if(buf_len % fmd->ld.sector_size != 0) {
    printf("Length of buffer must be a multiple of sector size\n");
    return -1;
  }
  while(buf_len > total_written) {
    //printf("cs:%lu ns: %lu len: %lu writ: %ld\n", fmd->ld.current_sector, fmd->ld.num_sectors, buf_len, total_written);
    if(fmd->ld.current_sector == fmd->ld.num_sectors) {
      printf("Insert new floppy...\n");
      system("pause");
      status = floppy_medium_ready(md);
      if(!status) {
        printf("Floppy disk not available\n");
        return -1;
      }
      status = get_drive_data_floppy(&(fmd->ld));
      if(status) {
        printf("Unable to get drive data: Floppy disk not available\n");
        return -1;
      }
    }
    bytes_written = write_drive_chs(&(fmd->ld), buf, (buf_len-total_written)/fmd->ld.sector_size);
    if(bytes_written < 0) {
      printf("Error writing to floppy\n");
      return -1;
    }
    total_written += bytes_written;
  }  
  return total_written;  
}

void floppy_medium_done(medium_data md, char* hash) {
  md = md;
  hash = hash;
}

int create_floppy_medium(Medium* m, floppy_medium_data* fmd, Digest* digest) {
  m->send = &floppy_medium_send;
  m->ready = &floppy_medium_ready;
  m->data = (void*)fmd;
  m->done = &floppy_medium_done;
  m->digest = digest;
  return m->ready(m->data);
}

 
