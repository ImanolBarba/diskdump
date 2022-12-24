/***************************************************************************
 *   SERIAL.C  --  This file is part of diskdump.                          *
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

#include "serial.h"

// A lot of the code here was derived from
// https://marknelson.us/posts/1990/05/01/servicing-com-port-interrupts.html
// very interesting read!

PORT *com = NULL;
void (interrupt far* old_break_handler)() = NULL;
void (interrupt far* old_user_tick_handler)() = NULL;

uint8_t counting_enabled = 0;
uint8_t ticks = 0;

// Everything blocking that touches serial comms needs to check this
// flag to avoid lockouts
uint8_t ctrlbreak_called = 0;

void interrupt far user_tick_handler() {
  if(counting_enabled) {
    ++ticks;
  }
}

void interrupt far break_handler() {
  // Restore original interrupt handler and mark program for termination
  port_close(com);
  ctrlbreak_called = 1;
}

void flush_buffer(buffer* b) {
  if(ctrlbreak_called) {
    return;
  }
  while(b->write_pos != b->read_pos) {
    // spin
  }
}

void interrupt far serial_ISR() {
  uint8_t data;

  _enable();
  for(;;) {
    switch(inp(com->uart_base + IIR)) {
      case IIR_MODEM_STATUS:
        // Read status to clear interrupt
        inp(com->uart_base + MSR);
        break;
      case IIR_TRANSMIT:
        if(com->out.read_pos == com->out.write_pos) {
          // No more data left to send, disable tx interrupts
          outp(com->uart_base + IER, IER_RX_DATA);
          data = inp(com->uart_base + MCR);
          outp(com->uart_base + MCR, data & ~MCR_RTS);
        } else {
          data = com->out.buffer[com->out.read_pos];
          com->out.read_pos = (com->out.read_pos + 1) % BUFFER_SIZE_SERIAL;
          com->out.overrun = 0;
          outp(com->uart_base + THR, data);
        }
        break;
      case IIR_RECEIVE:
        data = (uint8_t) inp(com->uart_base + RBR);
        if((com->in.write_pos+1) % BUFFER_SIZE_SERIAL != com->in.read_pos) {
          com->in.buffer[com->in.write_pos] = data;
          com->in.write_pos = (com->in.write_pos + 1) % BUFFER_SIZE_SERIAL;
          if((com->in.write_pos+1) % BUFFER_SIZE_SERIAL != com->in.read_pos) {
            // Buffer is about to overrun, disable DTR
            data = inp(com->uart_base + MCR) & ~MCR_DTR;
            outp(com->uart_base + MCR, data);
          }
        } else {
          // Buffer overrun!
          com->in.overrun = 1;
        }
        break;
      case IIR_LINE_STATUS:
        // Read status to clear interrupt
        inp(com->uart_base + LSR);
      	break;
      default:
        // No valid interrupts left, write EOI to 8259
        outp(IRQ_CONTROLLER, EOI);
        return;
    }
  }
}

int port_open(uint address, uint interrupt_number) {
  uint8_t current_mask;

  if((com = malloc(sizeof(PORT))) == NULL) {
    com = NULL;
    return -1;
  }

  com->in.write_pos = 0;
  com->in.read_pos = 0;
  com->in.overrun = 0;
  com->out.read_pos = 0;
  com->out.write_pos = 0;
  com->out.overrun = 0;
  com->uart_base = address;
  com->irq_mask = (uint8_t) 1 << (interrupt_number % 8);
  com->interrupt_number = interrupt_number;

  com->old_vector = _dos_getvect(interrupt_number);
  _dos_setvect(interrupt_number, serial_ISR);
  old_break_handler = _dos_getvect(BREAK_VECTOR);
  _dos_setvect(BREAK_VECTOR, break_handler);
  old_user_tick_handler = _dos_getvect(USER_TICK_VECTOR);
  _dos_setvect(USER_TICK_VECTOR, user_tick_handler);

  // IRQ_CONTROLLER + 1 is the Interrupt Mask Register
  current_mask = (uint8_t) inp(IRQ_CONTROLLER + 1);
  // bit clear on mask means enable interrupts
  outp(IRQ_CONTROLLER + 1, (~com->irq_mask & current_mask));
  return 0;
}

void port_close(PORT *p) {
  uint8_t current_mask;

  if(com == NULL) {
    // Already closed
    return;
  }

  // Disable all serial interrupts
  outp(p->uart_base + IER, 0);
  current_mask = (uint8_t) inp(IRQ_CONTROLLER + 1);
  // Mask serial interrupts in 8259
  outp(IRQ_CONTROLLER + 1, p->irq_mask | current_mask);
  // Restore old ISR for serial port
  _dos_setvect(p->interrupt_number, p->old_vector);
  // Restore old user tick handler
  if(old_user_tick_handler != NULL) {
    _dos_setvect(USER_TICK_VECTOR, old_user_tick_handler);
  }
  // Restore old break handler
  if(old_break_handler != NULL) {
    _dos_setvect(BREAK_VECTOR, old_break_handler);
  }
  // Reset modem control lines
  outp(p->uart_base + MCR, 0);
  free(p);
  com = NULL;
}

void port_set(PORT *p, ulongint speed) {
  uint8_t low_divisor;
  uint8_t high_divisor;

  // Disable interrupts and read RBR in case there is data that will
  // generate an interrupt
  outp(p->uart_base + IER, 0);
  inp(p->uart_base + RBR);

  low_divisor = (uint8_t) (115200L / speed) & 0xFF;
  high_divisor = (uint8_t) (115200L / speed) >> 8;
  // Expose DLAB
  outp(p->uart_base + LCR, LCR_DLAB);
  outp(p->uart_base + DLL, low_divisor);
  outp(p->uart_base + DLM, high_divisor);
  // Restore other registers
  outp(p->uart_base + LCR, 0);

  // Set line params
  outp(p->uart_base + LCR, (LCR_NO_PARITY | LCR_1_STOP_BIT | LCR_8_DATA_BITS));

  // Enable OUT2, because apparently it's needed for interrupts
  outp(p->uart_base + MCR, MCR_OUT2);

  // Enable rx interrupts
  outp(p->uart_base + IER, IER_RX_DATA);
}

int port_send(PORT *p, uint8_t data) {
  uint8_t current_mcr;

  if((p->out.write_pos + 1) % BUFFER_SIZE_SERIAL == p->out.read_pos) {
    p->out.overrun = 1;
    return -1;
  }

  p->out.buffer[p->out.write_pos] = data;
  p->out.write_pos = (p->out.write_pos + 1) % BUFFER_SIZE_SERIAL;

  // Enable RTS
  current_mcr = inp(p->uart_base + MCR);
  if(current_mcr & MCR_RTS == 0) {
    outp(com->uart_base + MCR, current_mcr | MCR_RTS);
  }

  // Enable tx holding interrupt if disabled
  if((inp(p->uart_base + IER) & IER_THRE) == 0) {
    outp(com->uart_base + IER, IER_THRE | IER_RX_DATA);
  }

  return 0;
}

int port_recv(PORT *p, uint8_t* data) {
  uint8_t current_mcr;

  if(p->in.read_pos == p->in.write_pos) {
    return -1;
  }
  *data = p->in.buffer[p->in.read_pos];
  p->in.read_pos = (p->in.read_pos + 1) % BUFFER_SIZE_SERIAL;
  p->in.overrun = 0;

  current_mcr = inp(p->uart_base + MCR);
  if(current_mcr & MCR_DTR == 0) {
    outp(p->uart_base + MCR, current_mcr | MCR_DTR);
  }
  return 0;
}

int check_ack(serial_medium_data* smd) {
  uint8_t status = 0;
  uint8_t ack;

  if(ctrlbreak_called) {
    return 0;
  }

  counting_enabled = 1;
  do {
    status = port_recv(smd->port, &ack);

    // Why are we doing this crap: we don't get the acknowledgment
    // immediately after sending, so if we just did like a sleep(1)
    // We would be introducing a 1 sec delay after every message
    // received. It's better to just use the timer to know when to
    // stop and the delay will be as minimal as possible

  } while(status && ticks < (TICKS_PER_SEC * MAX_RETRIES_SERIAL));

  counting_enabled = 0;
  ticks = 0;

  if(status != 0) {
    printf("No acknowledgment received from serial\n");
    return 0;
  }

  if(ack == ACK) {
    return 1;
  }
  if(ack == NACK) {
    printf("Received NACK!\n");
    return 0;
  }
  printf("Received invalid reply!\n");
  return 0;
}

int write_buffer_serial(serial_medium_data* smd, uint8_t far *buf, ulongint buf_len) {
  ulongint current_pos;
  uint8_t status = 0;

  for(current_pos = 0; current_pos < buf_len; current_pos++) {
    counting_enabled = 1;
    do {
      if(ctrlbreak_called) {
        return 1;
      }
      
      status = port_send(smd->port, *(buf + current_pos));
    } while(status && ticks < (TICKS_PER_SEC * MAX_RETRIES_SERIAL));

    counting_enabled = 0;
    ticks = 0;

    if(status != 0) {
      printf("Error sending buffer to serial port\n");
      return 1;
    }
  }

  // Blockingly wait for tx to finish
  flush_buffer(&(smd->port->out));

  return 0;
}

ssize_t serial_medium_send(uint8_t far *buf, ulongint buf_len, medium_data md) {
  uint8_t status;
  uint32_t crc;

  serial_medium_data* smd = (serial_medium_data*)md;

  // Send packet index
  status = write_buffer_serial(smd, (uint8_t*)&(smd->packet_index), 4);
  if(status != 0) {
    printf("Error sending current packet index: %lu\n", smd->packet_index);
    return -1;
  }

  // Send payload length
  status = write_buffer_serial(smd, (uint8_t*)&buf_len, 4);
  if(status != 0) {
    printf("Error sending payload length %lu for packet: %lu\n", buf_len, smd->packet_index);
    return -1;
  }

  // Send payload
  status = write_buffer_serial(smd, buf, buf_len);
  if(status != 0) {
    printf("Error sending payload of length %u for packet %lu\n", buf_len, smd->packet_index);
    return -1;
  }

  // Send CRC
  crc = calc_crc(buf, buf_len);
  status = write_buffer_serial(smd, (uint8_t*)&crc, 4);
  if(status != 0) {
    printf("Error sending payload CRC 0x%04X for packet index %lu\n", crc, smd->packet_index);
    return -1;
  }

  return buf_len;
}

int serial_medium_ready(medium_data md) {
  uint8_t status = 0;

  serial_medium_data* smd = (serial_medium_data*)md;
  status = check_ack(smd);
  if(status == 0) {
    printf("Peer didn't acknowledge packet index %lu. Retransmitting...\n", smd->packet_index);
    return MEDIUM_RETRY;
  }

  smd->packet_index++;
  return MEDIUM_READY;
}

void serial_medium_done(medium_data md, char* hash) {
  size_t hash_len = 0;
  uint8_t footer[9];
  uint32_t crc;
  uint8_t status;
  serial_medium_data* smd = (serial_medium_data*)md;

  memcpy(footer, "DISKDUMP", 8);
  hash_len = strlen(hash);

  switch(hash_len) {
    case HASH_LENGTH_NONE:
      footer[8] = HASH_NONE;
      break;
    case HASH_LENGTH_MD5:
      footer[8] = HASH_MD5;
      break;
    case HASH_LENGTH_SHA1:
      footer[8] = HASH_SHA1;
      break;
    case HASH_LENGTH_SHA256:
      footer[8] = HASH_SHA256;
      break;
    default:
      printf("Detected invalid hash with length: %u\n", hash_len);
      return;
  }

  write_buffer_serial(smd, footer, 9);
  write_buffer_serial(smd, hash, hash_len);

  // Send CRC
  crc = calc_crc(hash, hash_len);
  status = write_buffer_serial(smd, (uint8_t*)&crc, 4);
  if(status != 0) {
    printf("Error sending hash CRC 0x%04X\n", crc);
    return;
  }

  port_close(com);
}

int create_serial_medium(const char* port, ulongint speed, void* descriptor, Medium* m, serial_medium_data* smd, Digest* digest) {
  int status = 0;
  uint serial_interrupt = COM1_INTERRUPT;
  uint port_number = COM1_ADDR;
  uint8_t speed_packet[9];
  uint8_t drive_num;

  legacy_descriptor* ld = NULL;
  drive_descriptor* dd = NULL;
  uint32_t num_cylinders = 0;
  uint32_t num_heads = 0;
  uint32_t sectors_per_track = 0;
  uint16_t sector_size = 0;
  uint32_t num_sectors = 0;

  if(!strcmp(port, COM2)) {
    serial_interrupt = COM2_INTERRUPT;
    port_number = COM2_ADDR;
  }

  status = port_open(port_number, serial_interrupt);
  if(status != 0) {
    printf("Unable to initialise serial port at address; %04X\n", port_number);
    return 1;
  }

  port_set(com, 1200);

  smd->speed = speed;
  smd->num_retries = 0;
  smd->packet_index = 0;
  smd->port = com;

  // Initalise comms with peer
  memcpy(speed_packet, "DISKDUMP", 8);

  switch(speed) {
    case 1200:
      speed_packet[8] = SPEED_1200;
      break;
    case 2400:
      speed_packet[8] = SPEED_2400;
      break;
    case 4800:
      speed_packet[8] = SPEED_4800;
      break;
    case 9600:
      speed_packet[8] = SPEED_9600;
      break;
    case 115200:
      speed_packet[8] = SPEED_115200;
      break;
    default:
      printf("Invalid speed requested. Defaulting to 1200\n");
      speed = 1200;
      speed_packet[8] = SPEED_1200;
      break;
  };

  write_buffer_serial(smd, speed_packet, 9);

  printf("Switching to %lu bps\n", speed);
  sleep(1);
  port_set(com, speed);
  sleep(1);

  if(check_ack(smd) == 0) {
    printf("Peer failed to acknowledge speed negotiation\n");
    port_close(com);
    return 1;
  }

  drive_num = *((uint8_t*)descriptor);
  if(drive_num & HARD_DISK_FLAG) {
    dd = (drive_descriptor*)descriptor;
    num_cylinders = dd->num_cylinders;
    num_heads = dd->num_heads;
    sectors_per_track = dd->sectors_per_track;
    sector_size = dd->sector_size;
    num_sectors = dd->num_sectors;
  } else {
    ld = (legacy_descriptor*)descriptor;
    num_cylinders = ld->num_cylinders;
    num_heads = ld->num_heads;
    sectors_per_track = ld->sectors_per_track;
    sector_size = ld->sector_size;
    num_sectors = ld->num_sectors;
  }

  // Send disk info
  write_buffer_serial(smd, (uint8_t*)&num_cylinders, 4);
  write_buffer_serial(smd, (uint8_t*)&num_heads, 4);
  write_buffer_serial(smd, (uint8_t*)&sectors_per_track, 4);
  write_buffer_serial(smd, (uint8_t*)&sector_size, 2);
  write_buffer_serial(smd, (uint8_t*)&num_sectors, 4);

  if(check_ack(smd) == 0) {
    printf("Peer failed to acknowledge disk info\n");
    port_close(com);
    return 1;
  }

  m->send = &serial_medium_send;
  m->ready = &serial_medium_ready;
  m->data = (void*)smd;
  m->done = &serial_medium_done;
  m->digest = digest;
  return 0;
}
