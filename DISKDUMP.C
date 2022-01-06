/***************************************************************************
 *   DISKDUMP.C  --  This file is part of diskdump.                        *
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

#include <stdio.h>
#include "disk.h"
#include "dump.h"
#include "file.h"
#include "hex.h"
#include "stdout.h"
#include "md5.h"
#include "sha1.h"
#include "sha256.h"

uint8_t quiet = 0;
uint8_t progress_bar = 0;

typedef struct args {
  uint8_t list;
  uint8_t help;
  const char* path;
  ulongint file_size;
  uint8_t floppy;
  uint8_t zmodem;
  const char hostname;
  uint16_t port;
  uint8_t md5;
  uint8_t sha1;
  uint8_t sha256;
  uint8_t hex;
  uint8_t stdout_output;
  const char* drive_num;
} args;

void print_help() {
  // TODO
  printf("Help!\n");
}

int parse_args(int argc, char** argv, args* cmd) {
  // TODO 
  // hash flags are exclusive
  // medium flags are exclusive
  // set progress_bar
  // set quiet
  return 0;
}

int get_drive_num(uint8_t* drive_num, const char* num_str) {
  char* endptr;
  long num = strtol(num_str, &endptr, 10);
  if(num == 0 && endptr != (num_str + strlen(num_str))) {
    return 1;
  }
  *drive_num = (uint8_t)num;
  return 0;
}

// -- ACTIONS --
// --list     [DONE] /L
// --help            /?
// --source          /S (source)
// -- MEDIUMS --
// --file     [DONE] /D ARG /Z ARG
// --floppy          /F
// --zmodem          /Z
// --tcp             /H ARG /P ARG
// --hex      [DONE] /X
// --stdout   [DONE] /O
// -- DIGESTS --
// --md5      [DONE] /M
// --sha1     [DONE] /SHA
// --sha256   [DONE] /SHA256
// -- OTHER --
// --progress        /B
// --quiet           /Q
int main(int argc, char **argv) {
  args cmd;
  int status;
  
  // Disk data
  uint8_t drive_num;
  legacy_descriptor ld;
  drive_descriptor dd;

  // Medium data
  Medium m;
  file_medium_data fmd;
  hex_medium_data hmd;
  ulongint file_size;
  
  // Digest data
  Digest _hash;
  Digest* hash = &_hash;
  
  char hash_str_md5[MD5_STR_LENGTH+1];
  md5_digest_data mdd;

  char hash_str_sha1[SHA1_STR_LENGTH+1];
  sha1_digest_data sdd;

  char hash_str_sha256[SHA256_STR_LENGTH+1];
  sha256_digest_data sdd2;

  // Begin 
  memset(&cmd, 0x00, sizeof(args));
  status = parse_args(argc, argv, &cmd);
  if(status) {
    print_help();
    return 1;
  }
  if(cmd.list) {
    list_disks();
  } else if(cmd.drive_num) {
    status = get_drive_num(&drive_num, cmd.drive_num);
    if(status) {
      printf("Invalid drive number specified: %s\n", cmd.drive_num);
    }
    if(cmd.md5) {
      create_md5_digest(hash, &mdd);
    } else if(cmd.sha1) {
      create_sha1_digest(hash, &sdd);
    } else if(cmd.sha256) {
      create_sha256_digest(hash, &sdd2);
    } else {
      hash = NULL;
    }
    if(cmd.path) {
      if(cmd.file_size) {
        file_size = cmd.file_size;
      } else {
        file_size = DEFAULT_FILE_SIZE;
      }
      status = create_file_medium(cmd.path, file_size, &m, &fmd, hash);
      if(status) {
        printf("Unable to create file medium\n");
        return 1;
      }
    } else if(cmd.hex) {
      create_hex_medium(&m, &hmd, hash);
    } else if(cmd.stdout_output) {
      create_stdout_medium(&m, hash);
    } else {
      printf("Medium not selected\n");
      return 1;
    }
    if(drive_num & HARD_DISK_FLAG) {
      dd.drive_num = drive_num;
      get_drive_data_disk(&dd);
      printf("Dumping data from:\n\n");
      print_drive_data_disk(&dd);
      status = dump_hard_drive(&dd, &m);
    } else {
      ld.drive_num = drive_num;
      get_drive_data_floppy(&ld);
      printf("Dumping data from:\n\n");
      print_drive_data_floppy(&ld);
      status = dump_floppy_drive(&ld, &m);
    }
    if(status) {
      printf("Dump returned: %d\n", status);
      return 1;
    }
    printf("Finished dump successfully :)\n");
    if(cmd.md5) {
      get_md5_hash_string(&(mdd.hash_state), hash_str_md5);
      printf("MD5: %s\n", hash_str_md5);
    } else if(cmd.sha1) {
      get_sha1_hash_string(&(sdd.hash_state), hash_str_sha1);
      printf("SHA1: %s\n", hash_str_sha1);
    } else if(cmd.sha256) {
      get_sha256_hash_string(&(sdd2.hash_state), hash_str_sha256);
      printf("SHA256: %s\n", hash_str_sha256);
    }
  } else {
    print_help();
  }
  return 0;
}
