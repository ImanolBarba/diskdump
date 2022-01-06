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
  unsigned whatever;
  unsigned status;
  md = md;
  _dos_write(1, buf, buf_len, &whatever);
  return buf_len;
}

int stdout_medium_ready(medium_data md) {
  md = md;
  return 1;
}

void create_stdout_medium(Medium* m, Digest* digest) {
  m->send = &stdout_medium_send;
  m->ready = &stdout_medium_ready;
  m->data = NULL;
  m->digest = digest;
}
