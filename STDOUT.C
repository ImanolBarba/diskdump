/***************************************************************************
 *   STDOUT.C  --  This file is part of diskdump.                          *
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

#include "stdout.h"

ssize_t stdout_medium_send(uint8_t far *buf, ulongint buf_len, medium_data md) {
  unsigned bytes_written = 0;
  unsigned status = 0;
  ulongint total_bytes_written = 0;
  unsigned bytes_to_write = 0;
  md = md;
  while(total_bytes_written != buf_len) {
    bytes_to_write = min((buf_len - total_bytes_written), MAX_BYTES_STDOUT);
    status = _dos_write(1, buf + total_bytes_written, bytes_to_write, &bytes_written);
    if(status != 0) {
      printf("Error writing to stdout. Error code: %d\n", status);
      return -1;
    }
    if(bytes_written == 0) {
      printf("Nothing was written, wtf???\n");
      return -1;
    }
    total_bytes_written += bytes_written;
  }
  return buf_len;
}

int stdout_medium_ready(medium_data md) {
  md = md;
  return MEDIUM_READY;
}

void stdout_medium_done(medium_data md, char* hash) {
  md = md;
  hash = hash;
}

void create_stdout_medium(Medium* m, Digest* digest) {
  m->send = &stdout_medium_send;
  m->ready = &stdout_medium_ready;
  m->data = NULL;
  m->done = &stdout_medium_done;
  m->digest = digest;
  m->mtu = MAX_BYTES_STDOUT;
}
