/***************************************************************************
 *   MEDIUM.H  --  This file is part of diskdump.                          *
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

#ifndef _MEDIUM_H
#define _MEDIUM_H

#include "types.h"
#include "digest.h"

#include <stdio.h>

#define MEDIUM_NOT_READY 0
#define MEDIUM_READY 1
#define MEDIUM_RETRY 2


typedef void* medium_data;
typedef ssize_t (*medium_send)(uint8_t far*, ulongint, medium_data);
typedef int (*medium_ready)(medium_data);
typedef void (*medium_done)(medium_data, char*);

typedef struct Medium {
  medium_send send;
  medium_ready ready;
  medium_data data;
  medium_done done;
  Digest* digest;
  ulongint mtu;
} Medium;

#endif
