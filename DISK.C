/***************************************************************************
 *   DISK.C  --  This file is part of diskdump.                            *
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

#include "disk.h"

uint8_t drive_params[DRIVE_PARAMS_SIZE];

int check_extensions_present(uint8_t drive_num) {
  asm {
    MOV dl, drive_num
    MOV bx, 55AAh
    MOV ah, 41h
    INT 13h
    JNC ext_present
  }

  return 0;

  ext_present:
  return 1;
}

int get_drive_data_disk(drive_descriptor* dd) {
  uint8_t status = 0;
  uint8_t drive_num = dd->drive_num;
  uint16_t dp_segment = FP_SEG(drive_params);
  uint16_t dp_offset = FP_OFF(drive_params);
  asm {
    MOV ah, 48h
    MOV dl, drive_num
    PUSH ds
    MOV ds, dp_segment
    MOV si, dp_offset
    MOV WORD [ds:si], 1Eh
    INT 13h
    POP ds
    LEA si, status
    MOV BYTE [si], ah
  };
  if(!status) {
    dd->num_cylinders = *((uint32_t*)(drive_params+4));
    dd->num_heads = *((uint32_t*)(drive_params+8));
    dd->sectors_per_track = *((uint32_t*)(drive_params+12));
    dd->num_sectors = *((uint32_t*)(drive_params+16));
    // We're not reading the high order number of sectors because
    // assuming 512 byte sectors it would only hold a value if the
    // disk size were to be 2TB+, which we simply won't find
    dd->sector_size = *((uint16_t*)(drive_params+24));
    dd->current_sector = 0;
    dd->last_sector_offset = 0;
  }
  return (int)status;
}

int get_drive_data_floppy(legacy_descriptor* ld) {
  uint8_t status = 0;
  uint8_t bytes_per_sector_id = 0;
  uint8_t drive_num = ld->drive_num;

  asm {
    MOV dl, drive_num
    MOV ah, 08h
    INT 13h
    LEA si, status
    MOV BYTE [si], ah
    TEST ah, ah
    JNZ end
    MOV ah, cl
    AND cl, 0x3F
    SHR ah, 1 // Annoying AF, but I can't use CL or 286 instructions
    SHR ah, 1
    SHR ah, 1
    SHR ah, 1
    SHR ah, 1
    SHR ah, 1
    MOV al, ch
    ADD al, 1
    MOV si, ld
    MOV [si], ax   // num_cyl
    ADD dh, 1
    MOV [si+2], dh // num_heads
    MOV [si+3], cl // sect_per_track
    MOV [si+4], bl // drive_type
    MOV al, [es:di+3] 
    LEA si, bytes_per_sector_id
    MOV BYTE [si], al
  };

  end:
  if(!status) {
    switch(bytes_per_sector_id) {
      case BPS_128:
	ld->sector_size = 128;
      break;
      case BPS_256:
	ld->sector_size = 256;
      break;
      case BPS_512:
	ld->sector_size = 512;
      break;
      case BPS_1024:
	ld->sector_size = 1024;
      break;
      default:
	printf("Unknown sector size %d for drive %#.02X\n",bytes_per_sector_id, drive_num);
      break;
    }
    ld->num_sectors = ld->num_heads * ld->num_cylinders * ld->sectors_per_track;
    ld->current_sector = 0;
    ld->last_sector_offset = 0;
  }
  return (int)status;
}

void print_drive_data_floppy(legacy_descriptor *ld) {
  printf("Drive number:\t\t%02X\n", ld->drive_num);
  printf("Cylinders:\t\t%d\n", ld->num_cylinders);
  printf("Heads:\t\t\t%d\n", ld->num_heads);
  printf("Sectors per track:\t%d\n", ld->sectors_per_track);
  printf("Sector size:\t\t%d\n", ld->sector_size);
  printf("Total sectors:\t\t%ld\n", ld->num_sectors);
  printf("Current sector:\t\t%ld\n", ld->current_sector);
  printf("Last sector offset:\t%d\n", ld->last_sector_offset);
  printf("Floppy Type:\t\t");
  switch(ld->floppy_type) {
    case FLOPPY_TYPE_360K:
      printf("360K, 5.25\", 40 Track Double Density\n");
    break;
    case FLOPPY_TYPE_1200K:
      printf("1.2M, 5.25\", 80 Track High Density\n");
    break;
    case FLOPPY_TYPE_720K:
      printf("720K, 3.5\", 80 Track Double Density\n");
    break;
    case FLOPPY_TYPE_1440K:
      printf("1.44M, 3.5\", 80 Track High Density\n");
    break;
    case FLOPPY_TYPE_UNKNOWN:
    default:
      printf("UNKNOWN\n");
    break;
  }
  printf("\n");
}

void print_drive_data_disk(drive_descriptor *dd) {
  printf("Drive number:\t\t%02X\n", dd->drive_num);
  printf("Cylinders:\t\t%ld\n", dd->num_cylinders);
  printf("Heads:\t\t\t%ld\n", dd->num_heads);
  printf("Sectors per track:\t%ld\n", dd->sectors_per_track);
  printf("Sector size:\t\t%d\n", dd->sector_size);
  printf("Total sectors:\t\t%ld\n",dd->num_sectors);
  printf("Current sector:\t\t%ld\n", dd->current_sector);
  printf("Last sector offset:\t%d\n", dd->last_sector_offset);
  printf("\n");
}

void lba_2_chs(legacy_descriptor* ld, ulongint lba, uint8_t* c, uint8_t* h, uint8_t* s) {
  uint temp = lba % (ld->num_heads * ld->sectors_per_track);
  *c = lba / (ld->num_heads * ld->sectors_per_track);
  *h = temp / ld->sectors_per_track;
  *s = temp % ld->sectors_per_track + 1;
}

ssize_t read_sectors_chs(legacy_descriptor* ld, uint8_t cyl, uint8_t head, uint8_t sect, uint8_t sectors_to_read, uint8_t far *buf) {
  uint8_t status;
  uint8_t sectors_read;
  uint16_t buf_segment = FP_SEG(buf);
  uint16_t buf_offset = FP_OFF(buf);
  if(sectors_to_read > ld->sectors_per_track) {
    printf("Requesting cross-track read. Request read of no more than %d sectors\n", ld->sectors_per_track);
    return -1;
  }
  //printf("c:%d h:%d s:%d\n",cyl, head, sect);
  //printf("Reading %d (%ld/%ld) sectors into %04X:%04X\n", sectors_to_read, ld->current_sector, ld->num_sectors, FP_SEG(buf), FP_OFF(buf));
  asm {
    MOV si, ld
    MOV dl, BYTE [si+5] // drive_num
    MOV al, sectors_to_read
    MOV ch, cyl
    MOV cl, sect
    MOV dh, head
    PUSH es
    MOV es, buf_segment
    MOV bx, buf_offset
    MOV ah, 02h
    INT 13h
    POP es
    LEA si, status
    MOV BYTE [si], ah
    LEA si, sectors_read
    MOV BYTE [si], al
  }
  if(status) {
    printf("Read sector CHS status: %d\n", status);
    return -1;
  }
  return sectors_read;
}

ssize_t read_sectors_lba(drive_descriptor* desc, DAP* dap) {
  uint8_t status = 0;
  uint16_t dap_segment = FP_SEG(dap);
  uint16_t dap_offset = FP_OFF(dap);
  //printf("Reading %d (%ld/%ld) sectors into %04X:%04X\n", dap->num_sectors_to_read, desc->current_sector, desc->num_sectors, *((uint16_t*)(dap->buffer_addr+2)), *((uint16_t*)(dap->buffer_addr)));
  asm {
    MOV si, desc
    MOV dl, BYTE [si]
    MOV ah, 42h
    PUSH ds
    MOV ds, dap_segment
    MOV si, dap_offset
    INT 13h
    POP ds
    JNC noerror
    LEA si, status
    MOV BYTE [si], ah
  }
  printf("read_sectors_lba returned status: %d\n", status);
  return -1;
  noerror:
  return dap->num_sectors_to_read;
}

ssize_t read_drive_lba(drive_descriptor* dd, uint8_t far *segment_buf, uint sectors) {
  int status;
  DAP dap;
  ulongint remaining_sectors = dd->num_sectors - dd->current_sector;
  uint8_t sectors_requested = min(remaining_sectors, sectors);
  size_t sectors_read = 0;

  if(!remaining_sectors) {
    return 0;
  }

  while(sectors_read < sectors_requested) {
    ssize_t num_read;
    uint16_t buf_segment = FP_SEG(segment_buf);
    uint16_t buf_offset = sectors_read * dd->sector_size;
    dap.DAP_size = DAP_SIZE;
    dap.unused = 0;
    dap.num_sectors_to_read = min(MAX_SECTORS_LBA, sectors_requested - sectors_read);
    memcpy(dap.buffer_addr, &buf_offset, sizeof(uint16_t));
    memcpy(dap.buffer_addr+2, &buf_segment, sizeof(uint16_t));
    memcpy(dap.abs_sector_offset, &(dd->current_sector), sizeof(uint32_t));
    memset(dap.abs_sector_offset+4, 0x00, sizeof(uint32_t));
    num_read = read_sectors_lba(dd, &dap);
    if(num_read < 0) {
      return -1;
    }
    dd->current_sector += num_read;
    sectors_read += num_read;
  }
  segment_buf = MK_FP(FP_SEG(segment_buf),0x0000);
  return (ulongint)sectors_read * dd->sector_size;
}

ssize_t read_drive_chs(legacy_descriptor* ld, uint8_t far *segment_buf, uint sectors) {
  int status;
  uint8_t cyl, head, sect;
  ulongint remaining_sectors = ld->num_sectors - ld->current_sector;
  uint8_t sectors_requested = min(remaining_sectors, sectors);
  size_t sectors_read = 0;

  if(!remaining_sectors) {
    return 0;
  }

  while(sectors_read < sectors_requested) {
    ssize_t num_read;
    uint8_t sectors_to_read = min(ld->sectors_per_track, sectors_requested - sectors_read);
    sectors_to_read = min(sectors_to_read, ld->sectors_per_track - (ld->current_sector % ld->sectors_per_track));
    lba_2_chs(ld, ld->current_sector, &cyl, &head, &sect);
    segment_buf = MK_FP(FP_SEG(segment_buf), sectors_read * ld->sector_size);
    num_read = read_sectors_chs(ld, cyl, head, sect, sectors_to_read, segment_buf);
    if(num_read < 0) {
      return -1;
    }
    ld->current_sector += num_read;
    sectors_read += num_read;
  }
  segment_buf = MK_FP(FP_SEG(segment_buf),0x0000);
  return (ulongint)sectors_read * ld->sector_size;
}

ssize_t write_drive_chs(legacy_descriptor* ld, uint8_t far *segment_buf, uint sectors) {
  int status;
  uint8_t cyl, head, sect;
  ulongint remaining_sectors = ld->num_sectors - ld->current_sector;
  uint8_t sectors_requested = min(remaining_sectors, sectors);
  size_t sectors_read = 0;

  if(!remaining_sectors) {
    return 0;
  }

  while(sectors_read < sectors_requested) {
    ssize_t num_read;
    uint8_t sectors_to_read = min(ld->sectors_per_track, sectors_requested - sectors_read);
    sectors_to_read = min(sectors_to_read, ld->sectors_per_track - (ld->current_sector % ld->sectors_per_track));
    lba_2_chs(ld, ld->current_sector, &cyl, &head, &sect);
    segment_buf = MK_FP(FP_SEG(segment_buf), sectors_read * ld->sector_size);
    num_read = read_sectors_chs(ld, cyl, head, sect, sectors_to_read, segment_buf);
    if(num_read < 0) {
      return -1;
    }
    ld->current_sector += num_read;
    sectors_read += num_read;
  }
  segment_buf = MK_FP(FP_SEG(segment_buf),0x0000);
  return (ulongint)sectors_read * ld->sector_size;
}
