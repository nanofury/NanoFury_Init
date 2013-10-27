/*
 *   ==== NanoFury Project ====
 *
 *   nf_spidevc.h - SPI library for NanoFury/bitfury chip/board
 *
 *   Copyright (c) 2013 bitfury
 *   Copyright (c) 2013 Vladimir Strinski
 *
 *
 *  Originally written by Bitfury
 *  with major modifications for the NanoFury project by Vladimir Strinski
 *
 *  Link to original source can be found at:
 *  https://bitcointalk.org/index.php?topic=183368.msg2515577#msg2515577
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
*/

#ifndef NF_SPIDEVC_H
#define NF_SPIDEVC_H

#include "MCP2210-Library/mcp2210.h"


#define NANOFURY_USB_PRODUCT "NanoFury"
#define NANOFURY_USB_PRODUCT_FULL "NanoFury NF1 v0.6"

#define NANOFURY_GP_PIN_LED 0
#define NANOFURY_GP_PIN_SCK_OVR 5
#define NANOFURY_GP_PIN_PWR_EN 6

#define NANOFURY_MAX_BYTES_PER_SPI_TRANSFER 60			// due to MCP2210 limitation

extern hid_device *hndNanoFury;

/* check whether a given MCP2210 port has a NanoFury device */
bool nanofury_checkport(hid_device *handle);

/* close a given MCP2210 port with a NanoFury device */
void nanofury_device_off(hid_device *handle);

/* Initialize SPI using this function */
bool spi_init(wchar_t* szTestDeviceID, wchar_t* szFixProductString);

/* Sent RESET sequence to BUTFURY chip */
void spi_reset(hid_device *handle);

/* TX-RX single frame */
int spi_txrx(hid_device *handle, const char *wrbuf, char *rdbuf, int bufsz);

/* SPI BUFFER OPS */
void spi_clear_buf(void);
unsigned char *spi_getrxbuf(void);
unsigned char *spi_gettxbuf(void);
unsigned spi_getbufsz(void);

void spi_emit_buf_reverse(const char *str, unsigned sz); /* INTERNAL USE: EMIT REVERSED BYTE SEQUENCE DIRECTLY TO STREAM */
void spi_emit_buf(const char *str, unsigned sz); /* INTERNAL USE: EMIT BYTE SEQUENCE DIRECTLY TO STREAM */

void spi_emit_break(void); /* BREAK CONNECTIONS AFTER RESET */
void spi_emit_fsync(void); /* FEED-THROUGH TO NEXT CHIP SYNCHRONOUSLY (WITH FLIP-FLOP) */
void spi_emit_fasync(void); /* FEED-THROUGH TO NEXT CHIP ASYNCHRONOUSLY (WITHOUT FLIP-FLOP INTERMEDIATE) */

/* TRANSMIT PROGRAMMING SEQUENCE (AND ALSO READ-BACK) */
/* addr is the destination address in bits (16-bit - 0 to 0xFFFF valid ones)
   buf is buffer to be transmitted, it will go at position spi_getbufsz()+3
   len is length in _bytes_, should be 4 to 128 and be multiple of 4, as smallest
   transmission quantum is 32 bits */
void spi_emit_data(unsigned addr, const char *buf, unsigned len);


/* from BitFury's spic1 test */
void spi_clear_buf(void);


#endif
