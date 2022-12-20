# DISKDUMP
An DOS utility to make forensically sound images of disks and floppies and transfer them through various mediums.

Transfer mediums:
- Stdout (can be redirected to file, piped to another program etc)
- Hexdump format printed to stdout
- Split to files
- Write to floppies
- Serial transfer
- TCP transfer (to be used with netcat or whatever) [NOT IMPLEMENTED YET]

Can also calculate a hash of the disk while copying. This does slow down the transfer significantly however, since the implementation is done in C without optimisations or fancy intruction sets being used.

Hashes:
- MD5
- SHA1
- SHA256

## Compatibility

DISKDUMP works with original Intel 8088 hardware and DOS 2.0+. However, as we're using INT 13h to do disk IO, it's left to the BIOS implementation of each machine to not be broken and behave as expected. This means that success is not guaranteed as the BIOS might have bugs and do funny things like not report the drives correctly. Work is in progress to add a consensus based mechanism to decide which drives are present using multiple sources of information.

## How to build

This project is built using Borland Turbo C (I'm currently using 3.0). The Makefile provided works with Borland `make`.

To build the regular build:
```
C:\SOURCE\DISKDUMP\> make
```

To build the debug build:
```
C:\SOURCE\DISKDUMP\> make /DDEBUG
```

To build the "fast" build:
```
C:\SOURCE\DISKDUMP\> make /DSPEED
```

Note: I have no idea if this is in any measurable way fast, it's just a compiler setting from TC. To me it did seem to do nothing except increase the file size a tiny bit so perhaps it's not noticeable for this program.

## Usage
```
DISKDUMP.EXE ACTION MEDIUM [HASH] [OTHER]

ACTIONS:
	/L List all drives reported by BIOS
	/N DRIVE_NUM Dump data from drive DRIVE_NUM
		`/N 0x80` -- Dump first disk
	/? Print help

MEDIUMS:
	/D PATH Dump to files in the specified directory.
	/Z SIZE Size in bytes of the files. Default is 1474560
		`/D D:\DUMP /Z 1000000`
	/F DRIVE_NUM Dump to floppy disks in the specified drive
		/F 0x00 -- Dump to first floppy unit (A:\)
	/ZM Dump through serial port using ZMODEM protocol
	/H HOSTNAME Dump to TCP server. Netcat should work
	/P PORT TCP port to connect to. Default port is 5700
		`/H 1.2.3.4 /P 1234` -- Dump to TCP server on 1.2.3.4:1234
	/X Dump data through stdout in hexdump format
	/O Dump data directly to stdout. Can be piped or redirected to a file
	/0 Don't dump data. Useful to only calculate hash

HASHES:
	/MD5 Calculate MD5 hash
	/SHA Calculate SHA1 hash. Somewhat slow
	/SHA2 Calculate SHA256 hash. VERY slow

OTHER FLAGS:
	/B Display progress bar
	/Q Quiet. Don't print anything to stdout. Necessary with /O
```
