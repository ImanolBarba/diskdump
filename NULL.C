/***************************************************************************
 *   NULL.C  --  This file is part of diskdump.                            *
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

#include "null.h"

ssize_t null_medium_send(uint8_t far *buf, ulongint buf_len, medium_data md) {
  buf = buf;
  md = md;
  return buf_len;
}

int null_medium_ready(medium_data md) {
  md = md;
  return MEDIUM_READY;
}

void null_medium_done(medium_data md, char* hash) {
  md = md;
  hash = hash;
}

void create_null_medium(Medium* m, Digest* digest) {
  m->send = &null_medium_send;
  m->ready = &null_medium_ready;
  m->data = NULL;
  m->done = &null_medium_done;
  m->digest = digest;
  m->mtu = 0;
}
