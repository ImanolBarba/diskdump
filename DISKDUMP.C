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
#include "floppy.h"
#include "serial.h"
#include "md5.h"
#include "sha1.h"
#include "sha256.h"

uint8_t quiet = 0;
uint8_t progress = 0;

typedef enum digest_type {
  DIGEST_UNKNOWN = 0,
  DIGEST_MD5 = 1,
  DIGEST_SHA1 = 2,
  DIGEST_SHA256 = 3
} digest_type;

typedef enum medium_type {
  MEDIUM_UNKNOWN = 0,
  MEDIUM_FILE = 1,
  MEDIUM_HEX = 2,
  MEDIUM_STDOUT = 3,
  MEDIUM_NULL = 4,
  MEDIUM_FLOPPY = 5,
  MEDIUM_SERIAL = 6,
  MEDIUM_TCP = 7
} medium_type;

typedef enum mode {
  MODE_UNKNOWN = 0,
  MODE_LIST = 1,
  MODE_DUMP = 2
} mode;

typedef struct args {
  uint8_t list;
  const char* path;
  ulongint file_size;
  uint8_t floppy;
  const char* floppy_num;
  const char* hostname;
  uint16_t port;
  uint8_t md5;
  uint8_t sha1;
  uint8_t sha256;
  uint8_t hex;
  uint8_t stdout_output;
  uint8_t null_output;
  const char* drive_num;
  const char* serial_port;
  ulongint serial_speed;
} args;

void print_help(const char* exename) {
  signed short last_backslash;
  for(last_backslash = strlen(exename); exename[last_backslash] != '\\' && last_backslash > -1; --last_backslash) {
    // nothing to see here
  }
  if(last_backslash) {
    last_backslash++;
  }
  printf("%s ACTION MEDIUM [HASH] [OTHER]\n", exename + last_backslash);
  printf("\n");
  printf("ACTIONS:\n");
  printf("\t/L List all drives reported by BIOS\n");
  printf("\t/N DRIVE_NUM Dump data from drive DRIVE_NUM\n");
  printf("\t\t`/N 0x80` -- Dump first disk\n");
  printf("\t/? Print help\n");
  printf("\n");
  printf("MEDIUMS:\n");
  printf("\t/D PATH Dump to files in the specified directory.\n");
  printf("\t/Z SIZE Size in bytes of the files. Default is 1474560\n");
  printf("\t\t`/D D:\\DUMP /Z 1000000`\n");
  printf("\t/F DRIVE_NUM Dump to floppy disks in the specified drive\n");
  printf("\t\t/F 0x00 -- Dump to first floppy unit (A:\\)\n");
  printf("\t/S PORT /SS SPEED Dump through serial port\n");
  printf("\t\t/S COM1 /SS 115200\n");
  printf("\t/H HOSTNAME Dump to TCP server. Netcat should work\n");
  printf("\t/P PORT TCP port to connect to. Default port is 5700\n");
  printf("\t\t`/H 1.2.3.4 /P 1234` -- Dump to TCP server on 1.2.3.4:1234\n");
  printf("\t/X Dump data through stdout in hexdump format\n");
  printf("\t/O Dump data directly to stdout. Can be piped or redirected to a file\n");
  printf("\t/0 Don't dump data. Useful to only calculate hash\n");
  printf("\n");
  printf("HASHES:\n");
  printf("\t/MD5 Calculate MD5 hash\n");
  printf("\t/SHA Calculate SHA1 hash. Somewhat slow\n");
  printf("\t/SHA2 Calculate SHA256 hash. VERY slow\n");
  printf("\n");
  printf("OTHER FLAGS:\n");
  printf("\t/B Display progress bar\n");
  printf("\t/Q Quiet. Don't print anything to stdout. Necessary with /O\n");
}

int parse_args(int argc, char** argv, args* cmd) {
  int status;
  int i;
  long num;
  mode md = MODE_UNKNOWN;
  digest_type d = DIGEST_UNKNOWN;
  medium_type m = MEDIUM_UNKNOWN;
  cmd->file_size = DEFAULT_FILE_SIZE;
  for(i = 1; i < argc; ++i) {
    if(!strcmp(argv[i], "/L")) {
      if(md != MODE_UNKNOWN) {
        printf("More than one mode specified\n");
        return 1;
      }
      md = MODE_LIST;
      cmd->list = 1;
    } else if(!strcmp(argv[i], "/N")) {
      if(md != MODE_UNKNOWN) {
	      printf("More than one mode specified\n");
        return 1;
      }
      md = MODE_DUMP;
      cmd->drive_num = argv[++i];
    } else if(!strcmp(argv[i], "/?")) {
      // Fuck the rest of the parsing
      print_help(argv[0]);
      exit(0);
    } else if(!strcmp(argv[i], "/D")) {
      if(m != MEDIUM_UNKNOWN) {
        printf("More than one medium specified\n");
        return 1;
      }
      m = MEDIUM_FILE;
      cmd->path = argv[++i];
    } else if(!strcmp(argv[i], "/Z")) {
      status = parse_num(&num, argv[++i]);
      if(status) {
        printf("Invalid file size specified: %s\n", argv[i]);
        return 1;
      }
      cmd->file_size = (ulongint)num;
    } else if(!strcmp(argv[i], "/F")) {
      if(m != MEDIUM_UNKNOWN) {
        printf("More than one medium specified\n");
        return 1;
      }
      m = MEDIUM_FLOPPY;
      cmd->floppy = 1;
      cmd->floppy_num = argv[++i];
    } else if(!strcmp(argv[i], "/S")) {
      if(m != MEDIUM_UNKNOWN) {
        printf("More than one medium specified\n");
        return 1;
      }
      m = MEDIUM_SERIAL;
      cmd->serial_port = argv[++i];
      if(strcmp(cmd->serial_port, COM1) != 0 && strcmp(cmd->serial_port, COM2) != 0) {
        printf("Invalid serial port specified: %s\n", cmd->serial_port);
        return 1;    
      }
    } else if(!strcmp(argv[i], "/SS")) {
      status = parse_num(&num, argv[++i]);
      if(status) {
        printf("Invalid serial speed specified: %s\n", argv[i]);
        return 1;
      }
      switch(num) {
        case 1200:
        case 2400:
        case 4800:
        case 9600:
        case 115200:
          cmd->serial_speed = num;
          break;
        default:
          printf("Unsupported speed requested: %ul\n", num);
          return 1;
      }
    } else if(!strcmp(argv[i], "/H")) {
      if(m != MEDIUM_UNKNOWN) {
        printf("More than one medium specified\n");
        return 1;
      }
      m = MEDIUM_TCP;
      cmd->hostname = argv[++i];
    } else if(!strcmp(argv[i], "/P")) {
      status = parse_num(&num, argv[++i]);
      if(status) {
        printf("Invalid port specified: %s\n", argv[i]);
        return 1;
      }
      cmd->port = (uint16_t)num;
    } else if(!strcmp(argv[i], "/X")) {
      if(m != MEDIUM_UNKNOWN) {
        printf("More than one medium specified\n");
        return 1;
      }
      m = MEDIUM_HEX;
      cmd->hex = 1;
    } else if(!strcmp(argv[i], "/O")) {
      if(m != MEDIUM_UNKNOWN) {
        printf("More than one medium specified\n");
        return 1;
      }
      m = MEDIUM_STDOUT;
      cmd->stdout_output = 1;
    } else if(!strcmp(argv[i], "/0")) {
      if(m != MEDIUM_UNKNOWN) {
        printf("More than one medium specified\n");
        return 1;
      }
      m = MEDIUM_NULL;
      cmd->null_output = 1;
    } else if(!strcmp(argv[i], "/MD5")) {
      if(quiet) {
        continue;
      }
      if(d != DIGEST_UNKNOWN) {
        printf("More than one digest specified\n");
        return 1;
      }
      d = DIGEST_MD5;
      cmd->md5 = 1;
    } else if(!strcmp(argv[i], "/SHA")) {
      if(quiet) {
        continue;
      }
      if(d != DIGEST_UNKNOWN) {
        printf("More than one digest specified\n");
        return 1;
      }
      d = DIGEST_SHA1;
      cmd->sha1 = 1;
    } else if(!strcmp(argv[i], "/SHA2")) {
      if(quiet) {
        continue;
      }
      if(d != DIGEST_UNKNOWN) {
        printf("More than one digest specified\n");
        return 1;
      }
      d = DIGEST_SHA256;
      cmd->sha256 = 1;
    } else if(!strcmp(argv[i], "/B")) {
      if(quiet) {
        continue;
      }
      if(m == MEDIUM_HEX || m == MEDIUM_STDOUT) {
        continue;
      }
      progress = 1;
    } else if(!strcmp(argv[i], "/Q")) {
      cmd->md5 = 0;
      cmd->sha1 = 0;
      cmd->sha256 = 0;
      progress = 0;
      d = DIGEST_UNKNOWN;
      quiet = 1;
    } else {
      printf("Unrecognised token: %s\n", argv[i]);
      return 1;
    }
  }
  return 0;
}

int parse_num(long* num, const char* num_str) {
  char* endptr;
  long num_ret = strtoul(num_str, &endptr, 0);
  if(num == 0 && endptr != (num_str + strlen(num_str))) {
    return 1;
  }
  *num = num_ret;
  return 0;
}

// -- ACTIONS --
// --list     [DONE] /L
// --help     [DONE] /?
// --drive    [DONE] /N (drive_num)
// -- MEDIUMS --
// --file     [DONE] /D ARG /Z ARG
// --floppy   [DONE] /F ARG
// --serial          /S ARG /SS ARG
// --tcp             /H ARG /P ARG
// --hex      [DONE] /X
// --stdout   [DONE] /O
// --null     [DONE] /0
// -- DIGESTS --
// --md5      [DONE] /MD5
// --sha1     [DONE] /SHA
// --sha256   [DONE] /SHA2
// -- OTHER --
// --progress [DONE] /B
// --quiet    [DONE] /Q

int main(int argc, char **argv) {
  args cmd;
  int status;

  // Disk data
  long drive_num;
  legacy_descriptor ld;
  drive_descriptor dd;

  // Medium data
  Medium m;
  file_medium_data fmd;
  hex_medium_data hmd;
  floppy_medium_data fmd2;
  long floppy_num; // for floppy medium
  serial_medium_data smd;

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
    print_help(argv[0]);
    return 1;
  }
  if(cmd.list) {
    // We're listing drives
    list_disks();
  } else if(cmd.drive_num) {
    // We're dumping a disk
    status = parse_num(&drive_num, cmd.drive_num);
    if(status) {
      printf("Invalid drive number specified: %s\n", cmd.drive_num);
    }
  
    // Populate drive data first (needed for some mediums)  
    if(drive_num & HARD_DISK_FLAG) {
      dd.drive_num = (uint8_t)drive_num;
      status = get_drive_data_disk(&dd);
      if(status) {
        printf("Invalid drive number specified: %02X\n", (uint8_t)drive_num);
        return 1;
      }
    } else {
      ld.drive_num = (uint8_t)drive_num;
      status = get_drive_data_floppy(&ld);
      if(status) {
        printf("Invalid drive number specified: %02X\n", (uint8_t)drive_num);
        return 1;
      }
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
      status = create_file_medium(cmd.path, cmd.file_size, &m, &fmd, hash);
      if(status) {
        printf("Unable to create file medium\n");
        return 1;
      }
    } else if(cmd.hex) {
      create_hex_medium(&m, &hmd, hash);
    } else if(cmd.stdout_output) {
      create_stdout_medium(&m, hash);
    } else if(cmd.null_output) {
      create_null_medium(&m, hash);
    } else if(cmd.floppy) {
      status = parse_num(&floppy_num, cmd.floppy_num);
      if(status) {
        printf("Invalid destination floppy number specified: %s\n", cmd.floppy_num);
      }
      fmd2.ld.drive_num = (uint8_t)floppy_num;
      status = get_drive_data_floppy(&(fmd2.ld));
      if(status) {
        printf("Unable to get floppy drive data. Invalid destination floppy number specified: %02X\n", (uint8_t)floppy_num);
        return 1;
      }
      create_floppy_medium(&m, &fmd2, hash);
    } else if(cmd.serial_port) {
      if(drive_num & HARD_DISK_FLAG) {
        status = create_serial_medium(cmd.serial_port, cmd.serial_speed, (void*)&dd, &m, &smd, hash);
      } else {
        status = create_serial_medium(cmd.serial_port, cmd.serial_speed, (void*)&ld, &m, &smd, hash);
      }
      if(status != 0) {
        printf("Unable to initialise serial communication with peer\n");
        if(smd.port != NULL) {
          port_close(smd.port);
        }
        return 1;
      }
    } else {
      printf("Medium not selected\n");
      return 1;
    }
    if(drive_num & HARD_DISK_FLAG) {
      if(!quiet) {
        printf("Dumping data from:\n\n");
        print_drive_data_disk(&dd);
      }
      status = dump_hard_drive(&dd, &m);
    } else {
      if(!quiet) {
        printf("Dumping data from:\n\n");
        print_drive_data_floppy(&ld);
      }
      status = dump_floppy_drive(&ld, &m);
    }
    if(status) {
      printf("\n\nDump returned: %d\n", status);
      
      // Cleanup
      if(smd.port != NULL) {
        port_close(smd.port);
      }
      
      return 1;
    }
    if(!quiet) {
      printf("\n\nFinished dump successfully :)\n");
    }
    if(cmd.md5) {
      get_md5_hash_string(&(mdd.hash_state), hash_str_md5);
      m.done(m.data, hash_str_md5);
      printf("MD5: %s\n", hash_str_md5);
    } else if(cmd.sha1) {
      get_sha1_hash_string(&(sdd.hash_state), hash_str_sha1);
      m.done(m.data, hash_str_sha1);
      printf("SHA1: %s\n", hash_str_sha1);
    } else if(cmd.sha256) {
      get_sha256_hash_string(&(sdd2.hash_state), hash_str_sha256);
      m.done(m.data, hash_str_sha256);
      printf("SHA256: %s\n", hash_str_sha256);
    } else {
      m.done(m.data, NULL);
    }
  } else {
    // I don't know WTF we're doing so, let's print help
    print_help(argv[0]);
  }
  return 0;
}
