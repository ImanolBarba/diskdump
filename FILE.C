/***************************************************************************
 *   FILE.C  --  This file is part of diskdump.                            *
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

#include "file.h"

const char ext_sequence[MAX_EXT_SEQUENCE] = {
  'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
  'Q','R','S','T','U','V','W','X','Y','Z','0','1','2','3','4','5',
  '6','7','8','9','!','#','$','%','&','\'','(',')','-','@','^','_',
  '`','{','}','~'
};

uint16_t file_tell(uint16_t handle, ulongint* file_offset) {
  uint16_t status = 0;
 
  asm {
    MOV ah, 42h
    MOV al, 1
    MOV bx, handle
    XOR cx, cx
    XOR dx, dx
    INT 21h
    JNC noerror
    LEA si, status
    MOV WORD [si], ax 
  }
  return status;

  noerror:
  asm {
    MOV si, file_offset
    MOV WORD [si], ax
    MOV WORD [si+2], dx
  }
  return status;
}

uint16_t file_close(uint16_t handle) {
  uint16_t status = 0;
  
  asm {
    MOV ah, 3Eh
    MOV bx, handle
    INT 21h
    JNC noerror
    LEA si, status
    MOV WORD [si], ax
  }
  noerror:
  return status;
}

uint16_t file_creat(const char* path) {
  uint16_t handle = 0;
  uint16_t path_seg = FP_SEG(path);
  uint16_t path_off = FP_OFF(path);

  asm {
    MOV ah, 3Ch
    XOR cx, cx
    PUSH ds
    MOV ds, path_seg
    MOV dx, path_off
    INT 21h
    POP ds
    JC error
    LEA si, handle
    MOV WORD [si], ax
  }
  return handle;

  error:
  asm {
    LEA si, handle
    MOV WORD [si], ax
  }
  printf("Error opening file: status = %d\n", handle);
  return 0;
}

uint16_t file_write(uint16_t handle, uint16_t num_bytes, uint8_t far *buf, uint16_t* num_written) {
  uint16_t status = 0;
  uint16_t buf_segment = FP_SEG(buf);
  uint16_t buf_offset = FP_OFF(buf);

  asm {
    MOV ah, 40h
    MOV bx, handle
    MOV cx, num_bytes
    PUSH ds
    MOV ds, buf_segment
    MOV dx, buf_offset
    INT 21h
    POP ds
    JC error
    MOV si, num_written
    MOV WORD [si], ax
  }
  return 0;

  error:
  asm {
    LEA si, status
    MOV WORD [si], ax
  }
  return status;
}


uint8_t determine_drive_number(const char* path) {
  // Determine drive number from path
  // This is NOT BIOS drive number, but MSDOS drive number

  if(path[1] != ':') {
    return 0;
  }
  return (path[0] - 'A' + 1);
}

void increment_extension(file_medium_data* fmd) {
  uint8_t carry = 1;
  int i;
  for(i = 0; i < 3; ++i) {
    fmd->current_extension_indexes[i] = (fmd->current_extension_indexes[i] + carry) % MAX_EXT_SEQUENCE;
    if(!(fmd->current_extension_indexes[i] == 0 && carry)) {
      carry = 0;
    }
  }
}

int check_free_space(uint8_t drive_number, ulongint space_needed) {
  uint16_t sectors_per_cluster;
  uint16_t available_clusters;
  uint16_t bytes_per_sector;

  ulongint free_space = 0;
  
  asm {
    MOV dl, drive_number 
    MOV ah, 36h
    INT 21h
    LEA si, sectors_per_cluster
    MOV WORD [si], ax
    LEA si, available_clusters
    MOV WORD [si], bx
    LEA si, bytes_per_sector
    MOV WORD [si], cx
  }

  free_space = (ulongint)available_clusters * (ulongint)sectors_per_cluster * (ulongint)bytes_per_sector;

  return free_space > space_needed;
} 

ssize_t file_medium_send(uint8_t far *buf, ulongint buf_len, medium_data md) {
  uint16_t status;
  ulongint remaining_bytes = buf_len;
  ulongint current_pos;
  uint16_t bytes_to_write;
  uint16_t bytes_written;
  file_medium_data* fmd = (file_medium_data*)md;
  char path[MAX_PATH_LENGTH + 1];
  char filename[DUMP_FILE_NAME_LENGTH + 1];
  
  while(remaining_bytes) {
    if(fmd->fd == 0) {
      if(!check_free_space(determine_drive_number(fmd->target_directory), fmd->file_size)) {
        printf("Insufficient disk space for another file\n");
        return -1;
      }
      sprintf(filename, "dump.%c%c%c", ext_sequence[fmd->current_extension_indexes[2]], ext_sequence[fmd->current_extension_indexes[1]], ext_sequence[fmd->current_extension_indexes[0]]);
      sprintf(path, "%s\\%s", fmd->target_directory, filename);
      fmd->fd = file_creat(path);
      if(!fmd->fd) {
        printf("Error opening new file\n");
        return -1;
      }
    }
    status = file_tell(fmd->fd, &current_pos);
    if(status) {
      printf("Error getting current file offset\n");
      status = file_close(fmd->fd);
      if(status) {
        printf("Error closing file. Oh well :(\n");
      }
      return -1;
    }
    bytes_to_write = min(remaining_bytes, MAX_BYTES_TO_WRITE);
    bytes_to_write = min(bytes_to_write, fmd->file_size - current_pos);
    status = file_write(fmd->fd, bytes_to_write, buf + (buf_len - remaining_bytes), &bytes_written);
    if(status) {
      printf("Error writing to file\n");
      status = file_close(fmd->fd);
      if(status) {
        printf("Error closing file. Oh well :(\n");
      }
      return -1;
    }
    if(bytes_written + current_pos == fmd->file_size) {
      status = file_close(fmd->fd);
      if(status) {
        printf("Error closing file\n");
      }
      fmd->fd = 0;
      increment_extension(fmd);
    }
    remaining_bytes -= bytes_written;
  }
  return buf_len;
}

int file_medium_ready(medium_data md) {
  md = md;
  return 1;
}

int create_file_medium(const char* target_directory, ulongint file_size, Medium* m, file_medium_data* fmd, Digest* digest) {
  uint16_t file_handle;
  char path[MAX_PATH_LENGTH + 1];
  // Test that the directory is writeable
  if(strlen(target_directory) > (MAX_PATH_LENGTH + 1 - DUMP_FILE_NAME_LENGTH)) {
    printf("Path is too long\n");
    return -1;
  }
  sprintf(path, "%s\\DUMP.AAA", target_directory);
  file_handle = file_creat(path);
  if(file_handle == 0) {
    printf("Unable to create a file in the specified path: %s. Check if the path exists and the media is not write protected\n", target_directory);
    return -1; 
  }
  file_close(file_handle);
  fmd->fd = 0;
  memset(fmd->current_extension_indexes, 0x00, sizeof(fmd->current_extension_indexes));
  fmd->target_directory = target_directory;
  fmd->file_size = file_size;
  m->send = &file_medium_send;
  m->ready = &file_medium_ready;
  m->data = (void*)fmd;
  m->digest = digest;
  return 0;
}

 

