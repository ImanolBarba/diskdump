/***************************************************************************
 *   HEX.H  --  This file is part of diskdump.                             *
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

#ifndef _HEX_H
#define _HEX_H

#include "medium.h"
#include "digest.h"

// Maxium displayable size is 1TB, which is fine

typedef struct hex_medium_data {
  uint8_t current_offset_hi;
  ulongint current_offset_lo;
} hex_medium_data;

void create_hex_medium(Medium* m, hex_medium_data* hmd, Digest* digest);

#endif
