/***************************************************************************
 *   HEX.C  --  This file is part of diskdump.                             *
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

#include "hex.h"

void print_offset(uint8_t hi, ulongint lo) {
  printf("%02X%04X%04X  ", hi, *((uint16_t*)((uint8_t*)(&lo)+2)), *((uint16_t*)(&lo)));
}

int is_printable(unsigned char c) {
  if(c >= 0x20 && c < 127) {
    return 1;
  }
  return 0;
}

ssize_t hex_medium_send(uint8_t far *buf, ulongint buf_len, medium_data md) {
  ulongint i;
  unsigned char ascii_str[17];
  hex_medium_data* hmd = (hex_medium_data*)md;
  print_offset(hmd->current_offset_hi, hmd->current_offset_lo);
  for(i = 0; i < buf_len; ++i) {
    printf("%02X ",buf[i]);
    if(is_printable(buf[i])) {
      ascii_str[i%16] = buf[i];
    } else {
      ascii_str[i%16] = '.';
    }
    if(i % 16 == 15) {
      if(hmd->current_offset_lo + 16 < hmd->current_offset_lo) {
        hmd->current_offset_hi++;
      }
      hmd->current_offset_lo += 16;
      ascii_str[16] = '\0';
      printf(" |%s|\n", ascii_str);
      print_offset(hmd->current_offset_hi, hmd->current_offset_lo);
    }
  }
  return buf_len;
}

int hex_medium_ready(medium_data md) {
  md = md;
  return 1;
}

void create_hex_medium(Medium* m, hex_medium_data* hmd, Digest* digest) {
  hmd->current_offset_hi = 0;
  hmd->current_offset_lo = 0;
  m->send = &hex_medium_send;
  m->ready = &hex_medium_ready;
  m->data = (void*)hmd;
  m->digest = digest;
}
