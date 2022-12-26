import argparse
import binascii
from dataclasses import dataclass
import hashlib
import logging
import math
import serial
import struct
import sys
import time

import alive_progress

# Constants
HEADER_MAGIC = "DISKDUMP"
ACK = b'\x55'   # 01010101
NACK = b'\xAA'  # 10101010
ABRT = b'\xCC'  # 11001100

SERIAL_TIMEOUT = 10
DEFAULT_SPEED = 1200
MAX_RETRIES = 3
DEFAULT_OUTPUT_PATH = "disk.img"

SPEEDS = {
    0: 1200,
    1: 2400,
    2: 4800,
    3: 9600,
    4: 19200,
    5: 38400,
    6: 57600,
    7: 115200,
}

def calcHashNone(path: str, expected: str) -> bool:
    logger.info("No hash provided")
    return True


def calcHashMD5(path: str, expected: str) -> bool:
    logger.info(f"Hash is MD5 = {expected}")
    return hashlib.md5(open(path, 'rb').read()).hexdigest().upper() == expected


def calcHashSHA1(path: str, expected: str) -> bool:
    logger.info(f"Hash is SHA1 = {expected}")
    return hashlib.sha1(open(path, 'rb').read()).hexdigest().upper() == expected


def calcHashSHA256(path: str, expected: str) -> bool:
    logger.info(f"Hash is SHA256 = {expected}")
    return hashlib.sha256(open(path, 'rb').read()).hexdigest().upper() == expected


HASHES = {
    0: (0, calcHashNone),
    1: (32, calcHashMD5),
    2: (40, calcHashSHA1),
    3: (64, calcHashSHA256)
}

# Globals
logger = None

@dataclass
class DiskInfo:
    numCylinders: int
    numHeads: int
    sectorsPerTrack: int
    sectorSize: int
    numSectors: int

def recvDiskData(ser: serial.Serial, speed: int, diskInfo: DiskInfo, path: str) -> bool:
    remainingBytes = diskInfo.sectorSize * diskInfo.numSectors
    logger.info(f"Receiving data for disk with length {remainingBytes} bytes")
    retries_left = MAX_RETRIES
    success = False

    with open(path, "wb") as f:
        with alive_progress.alive_bar(remainingBytes, bar='classic', spinner='triangles') as bar:
            currentPacket = 0
            while remainingBytes != 0:
                while retries_left != 0 and not success:
                    packetIndex = struct.unpack('<I', ser.read(4))[0]
                    if (packetIndex != currentPacket):
                        logger.error(f"Packet index mismatch. Expected {currentPacket} got {packetIndex}")
                        ser.write(ABRT)
                        return False

                    payloadLength = struct.unpack('<I', ser.read(4))[0]
                    payload = ser.read(payloadLength)
                    payloadCRC = struct.unpack('<I', ser.read(4))[0]

                    if (not checkCRC(payload, payloadCRC)):
                        logger.error(f"CRC mismatch for packet {currentPacket}")
                        retries_left -= 1
                        ser.write(NACK)
                    else:
                        success = True
                
                if retries_left == 0:
                    logger.error("No more retries left. Aborting transfer.")
                    ser.write(ABRT)
                    return False

                f.write(payload)
                bar(payloadLength)
                ser.write(ACK)
                remainingBytes -= payloadLength
                currentPacket += 1
                success = False

    return True


def checkCRC(data: bytes, expected: int) -> bool:
    crc = binascii.crc32(data)
    return crc == expected


def verifyImage(ser: serial.Serial, hash: int, path: str) -> bool:
    retries_left = MAX_RETRIES
    success = False

    if hash not in HASHES:
        logger.error("Invalid hash requested")
        return False

    hashLength, hashFunc = HASHES[hash]

    while retries_left != 0 and not success:
        success = False
        expectedHash = ser.read(hashLength)
        hashMsgCRC = struct.unpack('<I', ser.read(4))[0]

        if (not checkCRC(expectedHash, hashMsgCRC)):
            logger.error("Hash message contains invalid CRC")
            retries_left -= 1
            ser.write(NACK)
        else:
            success = True

    if retries_left == 0:
        logger.error("No more retries left. Aborting verification.")
        ser.write(ABRT)
        return False

    return hashFunc(path, expectedHash.decode("UTF-8"))


def changeSerialSpeed(oldSerial: serial.Serial, device: str, speed: int) -> serial.Serial:
    logger.info(f"Changing to speed: {speed} bps")
    oldSerial.close()
    newSerial = serial.Serial(device, speed, timeout=SERIAL_TIMEOUT)

    # Allow peer some time to change speed and prepare for ACK
    time.sleep(3)

    return newSerial


def recvDiskInfo(ser: serial.Serial) -> DiskInfo:
    diskInfoRaw = ser.read(18)  # 4 + 4 + 4 + 2 + 4
    diskInfo = DiskInfo(0, 0, 0, 0, 0)
    diskInfo.numCylinders, diskInfo.numHeads, diskInfo.sectorsPerTrack, diskInfo.sectorSize, diskInfo.numSectors = struct.unpack('<IIIHI', diskInfoRaw)
    return diskInfo


def printDiskInfo(diskInfo: DiskInfo) -> None:
    print("== DISK INFO ==")
    print(f"Cylinders:\t\t {diskInfo.numCylinders}")
    print(f"Heads:\t\t\t {diskInfo.numHeads}")
    print(f"Sectors per Track:\t {diskInfo.sectorsPerTrack}")
    print(f"Sector size:\t\t {diskInfo.sectorSize}")
    print(f"Total Sectors:\t\t {diskInfo.numSectors}")


def main(device: str, imgPath: str) -> int:
    logger.info(f"Dumping data to {imgPath}")
    with serial.Serial(device, DEFAULT_SPEED) as ser:
        # first packet: DISKDUMPx where x is speed, either 1200, 2400, 4800, 9600, 115200 bps
        logger.info(f"Listening to {device}")
        headerMsg = ser.read(len(HEADER_MAGIC) + 1)
        header = headerMsg[:len(HEADER_MAGIC)].decode("UTF-8")
        if (header != HEADER_MAGIC):
            logger.error(f"Invalid header: {header}")
            return 1

        speed = headerMsg[len(HEADER_MAGIC)]
        if (speed not in SPEEDS):
            logger.error(f"Invalid speed requested: {speed}")
            return 1

        speed = SPEEDS[speed]

        ser = changeSerialSpeed(ser, device, speed)
        ser.write(ACK)

        # second packet: serialised disk info (18 bytes)
        diskInfo = recvDiskInfo(ser)
        ser.write(ACK)

        print("")
        printDiskInfo(diskInfo)
        print("")
        
        ok = recvDiskData(ser, speed, diskInfo, imgPath)
        if (not ok):
            logger.error("Error receiving disk data")
            return 1

        logger.info("Data transfer finished")

        # final packet: DISKDUMPx where x is hash algo None, MD5, SHA1, SHA256
        footerMsg = ser.read(len(HEADER_MAGIC) + 1)
        footer = footerMsg[:len(HEADER_MAGIC)].decode("UTF-8")
        if (footer != HEADER_MAGIC):
            logger.error(f"Invalid footer: {footer}")
            return 1

        ser.write(ACK)
        hash = footerMsg[len(HEADER_MAGIC)]
        ok = verifyImage(ser, hash, imgPath)
        if (not ok):
            logger.error("Error verifying the saved image")
            return 1
        logger.info("Image verified")

    logger.info("Successfully received image! :)")
    return 0


if __name__ == "__main__":
    logging.basicConfig(format='%(levelname)s - %(asctime)s [%(filename)s:%(lineno)d %(funcName)s()] %(message)s', level=logging.INFO)
    logger = logging.getLogger(__name__)

    parser = argparse.ArgumentParser(description="Serial port listener for transfers from DISKDUMP")
    parser.add_argument("--output", type=str, default=DEFAULT_OUTPUT_PATH, help="Path to file where the data will be dumped to (Default: disk.img in current directory)")
    parser.add_argument("--port", type=str, required=True, help="Serial port where the communication will be established")
    args = parser.parse_args()
    exit(main(args.port, args.output))
