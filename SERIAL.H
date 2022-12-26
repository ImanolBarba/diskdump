/***************************************************************************
 *   SERIAL.H  --  This file is part of diskdump.                          *
 *                                                                         *
 *   Copyright (C) 2022 Imanol-Mikel Barba Sabariego                       *
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

#ifndef _SERIAL_H
#define _SERIAL_H

#include "crc.h"
#include "digest.h"
#include "disk.h"
#include "medium.h"

#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>

#define ACK 0x55
#define NACK 0xAA

#define DEFAULT_SPEED 0
#define SPEED_1200    0
#define SPEED_2400    1
#define SPEED_4800    2
#define SPEED_9600    3
#define SPEED_19200   4
#define SPEED_38400   5
#define SPEED_57600   6
#define SPEED_115200  7

#define HASH_NONE   0
#define HASH_MD5    1
#define HASH_SHA1   2
#define HASH_SHA256 3

#define HASH_LENGTH_NONE   0
#define HASH_LENGTH_MD5    32
#define HASH_LENGTH_SHA1   40
#define HASH_LENGTH_SHA256 64

#define MAX_RETRIES_SERIAL 3
#define BUFFER_SIZE_SERIAL 256
#define TICKS_PER_SEC      18  // It's an approximation

#define RBR              0    // Receive Buffer Register
#define THR              0    // Transmit Holding Register
#define IER              1    // Interrupt Enable Register
#define IER_RX_DATA      1    // Interrupt for RX
#define IER_THRE         2    // Interrupt for TX
#define IER_MDM_STATUS   8    // Interrupt for modem status change
#define IIR              2    // Interrupt ID Register
#define IIR_MODEM_STATUS 0    // Modem status interrupt ID
#define IIR_TRANSMIT     2    // Transmit interrupt ID
#define IIR_RECEIVE      4    // Receive interrupt ID
#define IIR_LINE_STATUS  6    // Line status interrupt ID
#define LCR              3    // Line Control Register
#define LCR_DLAB         0x80 // Divisor Access bit
#define LCR_NO_PARITY    0
#define LCR_1_STOP_BIT   0
#define LCR_8_DATA_BITS  3
#define MCR              4    // Modem Control Register
#define MCR_DTR          1    // Set DTR line
#define MCR_RTS          2    // Set RTS line
#define MCR_OUT1         4    // Enable OUT1
#define MCR_OUT2         8    // Enable OUT2
#define LSR              5    // Line Status Register
#define MSR              6    // Modem Status Register
#define DLL              0    // Divisor Latch LSB
#define DLM              1    // Divisor Latch MSB

#define IRQ_CONTROLLER_A 0x20
#define IRQ_MASK_REG_A   0x21
#define EOI              0x20
#define BREAK_VECTOR     0x23
#define USER_TICK_VECTOR 0x1C

#define COM1           "COM1"
#define COM1_ADDR_BDA  MK_FP(0x0040, 0x0000)
#define COM1_INTERRUPT 12
#define COM2           "COM2"
#define COM2_ADDR_BDA  MK_FP(0x0040, 0x0002)
#define COM2_INTERRUPT 11

typedef struct buffer {
  uint8_t buffer[BUFFER_SIZE_SERIAL];
  uint write_pos;
  uint read_pos;
  uint8_t overrun;
} buffer;

typedef struct PORT {
  void (interrupt far* old_vector)();
  uint uart_base;
  uint irq_mask;
  uint interrupt_number;
  buffer in;
  buffer out;
} PORT;

typedef struct serial_medium_data {
  PORT* port;
  ulongint speed;
  uint num_retries;
  ulongint packet_index;
} serial_medium_data;

void port_close(PORT *p);
int create_serial_medium(const char* port, ulongint speed, void* descriptor, Medium* m, serial_medium_data* fmd, Digest* digest);

#endif
