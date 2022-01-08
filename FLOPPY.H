/***************************************************************************
 *   FLOPPY.H  --  This file is part of diskdump.                          *
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

#ifndef _FLOPPY_H
#define _FLOPPY_H

#include "medium.h"
#include "disk.h"
#include "types.h"

#define MAX_RETRIES 3

typedef struct floppy_medium_data {
  legacy_descriptor ld;
} floppy_medium_data;

int create_floppy_medium(Medium* m, floppy_medium_data* fmd, Digest* digest);

#endif
