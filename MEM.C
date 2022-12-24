/***************************************************************************
 *   MEM.C  --  This file is part of diskdump.                             *
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

#include "mem.h"

uint16_t get_DMA_boundary_segment(uint16_t segment) {
  // When we read sectors to memory, it needs to be within a 64K
  // boundary for DMA transfer.
  // We need to find where within is such boundary in the segment that
  // DOS gave us and express it as a 0 offset segment.
  return (uint16_t)(0x1000 * ceil((double)segment/0x1000));
}

uint16_t alloc_segment(uint16_t* segment, uint16_t* largest_block) {
  uint16_t status = 0;
  // By requesting 128 KB of memory, we're guaranted to have 1 full
  // segment within a DMA 64KB boundary. It's not elegant, but at
  // least it doesn't require UMBs, and splitting the DMA transfer in
  // 2 complicates things further since the DMA boundary won't
  // necessarily be within a $SECTOR_SIZE boundary
  _asm {
    MOV bx, 2000h // 8192 * 16 = 128 KB
    MOV ah, 48h
    INT 21h
    JC error
    MOV si, segment
    MOV WORD PTR [si], ax
    JMP end
    error:
    LEA si, status
    MOV WORD PTR [si], ax
    MOV si, largest_block
    MOV WORD PTR [si], bx
    end:
  }

  return status;
}

uint16_t free_segment(uint16_t segment) {
  uint16_t status = 0;

  _asm {
    PUSH es
    MOV es, segment
    MOV ah, 49h
    INT 21h
    POP es
    JNC noerror
    LEA bx, status
    MOV WORD PTR [bx], ax
    noerror:
  }

  return status;
}
