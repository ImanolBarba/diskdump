/***************************************************************************
 *   DUMP.H  --  This file is part of diskdump.                            *
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

#ifndef _DUMP_H
#define _DUMP_H

#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include "disk.h"
#include "medium.h"
#include "mem.h"
#include "types.h"

#define BAR_WIDTH 70
#define MAX_LEN_UINT32_STR 10
#define MAX_RETRIES 3

void list_disks();
int dump_floppy_drive(legacy_descriptor* ld, Medium* m);
int dump_hard_drive(drive_descriptor* dd, Medium* m);

#endif
