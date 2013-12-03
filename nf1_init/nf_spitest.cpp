/*
 *   ==== NanoFury Project ====
 *
 *   Bitfury chip-test program
 *
 *   Copyright (c) 2013 bitfury
 *   Copyright (c) 2013 Vladimir Strinski
 *
 *
 *  Originally written by Bitfury
 *  With minor syntax modifications for the NanoFury project
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
#include "stdafx.h"

#include <stdio.h>
#include <string.h>
#include "nf_spidevc.h"
#include <time.h>
#include "tvec.c"
#include <Windows.h>

#define MAX_TVEC 100

// 0 .... 31 bit
// 1000 0011 0101 0110 1001 1010 1100 0111

// 1100 0001 0110 1010 0101 1001 1110 0011
// C16A59E3

unsigned char enaconf[4] = { 0xc1, 0x6a, 0x59, 0xe3 };
unsigned char disconf[4] = { 0, 0, 0, 0 };

/* Configuration registers - control oscillators and such stuff. PROGRAMMED when magic number is matches, UNPROGRAMMED (default) otherwise */
void config_reg(int cfgreg, int ena)
{
	if (ena) spi_emit_data(0x7000+cfgreg*32, (char*)enaconf, 4);
	else     spi_emit_data(0x7000+cfgreg*32, (char*)disconf, 4);
}

/* Configuration registers 7,8,9,10,11 - enable scan chain. All should be UNPROGRAMMED. Or instead of calculating useful jobs chip will activate scanchain
   of selected columns */
/* Configuration register 6 - if UNPROGRAMMED - output to OUTCLK is taken straight from INCLK pin, if PROGRAMMED - output to OUTCLK is
   taken from internal clock bus */

/* Clock stages - first we take INCLK as input */
/* 1st stage: Configuration register 4 - if UNPROGRAMMED - then INCLK is fed to next oscillator stage if PROGRAMMED then programmable-delay slow oscillator
   output is MUXED to clock next clock stage */
/* 2nd stage: Configuration register 1 - if UNPROGRAMMED - then feedthrough clock to next stage, if PROGRAMMED then value of configuration register 0 is
   taken as clock signal (useful for single-step evaluation mode) */
/* Configuration register 2 - if UNPROGRAMMED then feedthough clock, if PROGRAMMED then fast ring oscillator is active and feeds next stage */
/* Configuration register 3 - if UNPROGRAMMED then clock is divided by 2, if PROGRAMMED then clock is fed directly to internal clock bus */
/* Config register 5 - reserved for future use */

/* Addresses and their use:

0000 0001 Xlll ebbb - PROGRAM START/STOP SIGNALS
 lll 0 - PC MAX, 1 - ELNS, 2 - WLNS, 3 - RRSTN, 4 - WRSTN, 5 - WLN, 6 - ELN, 7 - WNON
 e - 1 - STOP, 0 - START bbb - bits 0..6 (7 bits) to match against PROGRAM COUNTER.

AEW (without address translation) load:
0001 rj0w wwwb bbbb - Where wwww is round expander position (0..15), bbbbb is bit number, j - is job number and q is 0 for first round, 1 for second round
0001 rj10 0eeb bbbb - Loading AL register
0001 rj10 1aab bbbb - Loading EL register

ATR (address-translated load):
0011 xxaa ooob bbbb address        - CONVERTING ADDRESS TO LOAD EXACTLY NEXT JOB FOR NEXT SCANNING:
<H,G,F,E,D,C,B,A> - midstate3 vector
<A,B,C,D,E,F,G,H> - midstate0 vector
<W0,W1,W2>        - W vector
Done by uploading of 19 32-bit values (single uploaded job)
On read-back first 16 32-bit words read back - found nonces, last 3 32-bit words is current job - 0 or 1. Programming is done to job that currently
is not SCANNED for valid solutions. All others read-back addresses return just current scan-chain output. Server (or controller) side should track
new nonces that were found by a chip and program it quickly enough to keep number of answers within adequate limits, otherwise answers will be lost.

0101 xxxx xxaa aaaa - INTERNAL OSCILLATOR PROGRAMMING (THERMOMETER CODE)
0110 xxxx xxaa aaaa - SLOW INTERNAL OSCILLATOR PROGRAMMING (THERMOMETER CODE)
x111 xxxa aaab bbbb - CONFIGURATION REGISTER MAGIC NUMBER PROGRAMMING.

Slow versus fast oscillators. Slow oscillator should give more uniform delay and less jitter for lower frequencies, while fast oscillator
could give higher frequencies, but less stability at lower frequencies (could means that it behaves instable in SPICE, how it will in reality
though is not known, while slow is definitely works under all conditions).

*/

// CONSTANT IS ALWAYS STARTED WHEN PC=0
// WLN = 0-1 and active 16 clock cycles
// ELN = 0   and is active 4 clock cycles
// RRSTN = 4 and active one cycle
// ELNS same as RRSTN but active 4 cycles
// WLNS is 2 cycles AFTER ELNS and is active 16 cycles
// WRSTN - active on 63 and only one cycle
// WNON is rised on WLN+4 and falled on next interval
// BUT - WNON IS POSITIVE
// BASES ARE SET TO MATCH CONSTANTS

#define FIRST_BASE 61
#define SECOND_BASE 4
char counters[16] = { 64, 64,
	SECOND_BASE, SECOND_BASE+4, SECOND_BASE+2, SECOND_BASE+2+16, SECOND_BASE, SECOND_BASE+1,
	(FIRST_BASE)%65,  (FIRST_BASE+1)%65,  (FIRST_BASE+3)%65, (FIRST_BASE+3+16)%65, (FIRST_BASE+4)%65, (FIRST_BASE+4+4)%65, (FIRST_BASE+3+3)%65, (FIRST_BASE+3+1+3)%65};

//char counters[16] = { 64, 64,
//	SECOND_BASE, SECOND_BASE+4, SECOND_BASE+2, SECOND_BASE+2+16, SECOND_BASE, SECOND_BASE+1,
//	(FIRST_BASE)%65,  (FIRST_BASE+1)%65,  (FIRST_BASE+3)%65, (FIRST_BASE+3+16)%65, (FIRST_BASE+4)%65, (FIRST_BASE+4+4)%65, (FIRST_BASE+3+3)%65, (FIRST_BASE+3+1+3)%65};
#ifdef DEBUG_SPITXRX
char *buf = "Hello, World!\x55\xaa";
char outbuf[16];
#endif

/* Oscillator setup variants (maybe more), values inside of chip ANDed to not allow by programming errors work it at higher speeds  */
/* WARNING! no chip temperature control limits, etc. It may self-fry and make fried chips with great ease :-) So if trying to overclock */
/* Do not place chip near flammable objects, provide adequate power protection and better wear eye protection ! */
/* Thermal runaway in this case could produce nice flames of chippy fries */

unsigned char osc4[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00 };
unsigned char osc5[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };
unsigned char osc6[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x00 };
unsigned char osc2[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00 }; // Thermometer code from left to right - more ones ==> faster clock!
unsigned char osc3[8] = { 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // Thermometer code from left to right - more ones ==> faster clock!
unsigned char osc[8]  = { 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // Thermometer code from left to right - more ones ==> faster clock!

unsigned char osc48[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00 };
unsigned char osc49[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00 };
unsigned char osc50[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0x00 };
unsigned char osc51[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x00 };
unsigned char osc52[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x00 };
unsigned char osc53[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 0x00 };
unsigned char osc54[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F, 0x00 };
unsigned char osc55[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x00 };
unsigned char osc56[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };

/* Test vectors to calculate (using address-translated loads) */
unsigned atrvec[] = {
0xb0e72d8e, 0x1dc5b862, 0xe9e7c4a6, 0x3050f1f5, 0x8a1a6b7e, 0x7ec384e8, 0x42c1c3fc, 0x8ed158a1, /* MIDSTATE */
0,0,0,0,0,0,0,0,
0x8a0bb7b7, 0x33af304f, 0x0b290c1a, 0xf0c4e61f, /* WDATA: hashMerleRoot[7], nTime, nBits, nNonce */

0x9c4dfdc0, 0xf055c9e1, 0xe60f079d, 0xeeada6da, 0xd459883d, 0xd8049a9d, 0xd49f9a96, 0x15972fed, /* MIDSTATE */
0,0,0,0,0,0,0,0,
0x048b2528, 0x7acb2d4f, 0x0b290c1a, 0xbe00084a, /* WDATA: hashMerleRoot[7], nTime, nBits, nNonce */

0x0317b3ea, 0x1d227d06, 0x3cca281e, 0xa6d0b9da, 0x1a359fe2, 0xa7287e27, 0x8b79c296, 0xc4d88274, /* MIDSTATE */
0,0,0,0,0,0,0,0,
0x328bcd4f, 0x75462d4f, 0x0b290c1a, 0x002c6dbc, /* WDATA: hashMerleRoot[7], nTime, nBits, nNonce */

0xac4e38b6, 0xba0e3b3b, 0x649ad6f8, 0xf72e4c02, 0x93be06fb, 0x366d1126, 0xf4aae554, 0x4ff19c5b, /* MIDSTATE */
0,0,0,0,0,0,0,0,
0x72698140, 0x3bd62b4f, 0x3fd40c1a, 0x801e43e9, /* WDATA: hashMerleRoot[7], nTime, nBits, nNonce */

0x9dbf91c9, 0x12e5066c, 0xf4184b87, 0x8060bc4d, 0x18f9c115, 0xf589d551, 0x0f7f18ae, 0x885aca59, /* MIDSTATE */
0,0,0,0,0,0,0,0,
0x6f3806c3, 0x41f82a4f, 0x3fd40c1a, 0x00334b39, /* WDATA: hashMerleRoot[7], nTime, nBits, nNonce */

};

#define rotrFixed(x,y) (((x) >> (y)) | ((x) << (32-(y))))
#define s0(x) (rotrFixed(x,7)^rotrFixed(x,18)^(x>>3))
#define s1(x) (rotrFixed(x,17)^rotrFixed(x,19)^(x>>10))
#define Ch(x,y,z) (z^(x&(y^z)))
#define Maj(x,y,z) (y^((x^y)&(y^z)))
#define S0(x) (rotrFixed(x,2)^rotrFixed(x,13)^rotrFixed(x,22))
#define S1(x) (rotrFixed(x,6)^rotrFixed(x,11)^rotrFixed(x,25))

/* SHA256 CONSTANTS */
static const unsigned SHA_K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static const unsigned sha_initial_state[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
static unsigned ini_vec[8] = { 0x5be0cd19,0x1f83d9ab,0x9b05688c,0x510e527f,0xa54ff53a,0x3c6ef372,0xbb67ae85,0x6a09e667 };
static unsigned midstate3[8], midstate0[8]; /* HGFEDCBA */

void ms3_compute(unsigned *p)
{
	unsigned a,b,c,d,e,f,g,h, ne, na,  i;

	a = p[0]; b = p[1]; c = p[2]; d = p[3]; e = p[4]; f = p[5]; g = p[6]; h = p[7];
	
	for (i = 0; i < 3; i++) {
		ne = p[i+16] + SHA_K[i] + h + Ch(e,f,g) + S1(e) + d;
		na = p[i+16] + SHA_K[i] + h + Ch(e,f,g) + S1(e) + S0(a) + Maj(a,b,c);
		d = c; c = b; b = a; a = na;
		h = g; g = f; f = e; e = ne;
	}

	p[15] = a; p[14] = b; p[13] = c; p[12] = d; p[11] = e; p[10] = f; p[9] = g; p[8] = h;
}

/* Nonce decoding */
unsigned decnonce(unsigned in)
{
	unsigned out;

	/* First part load */
	out = (in & 0xFF) << 24; in >>= 8;

	/* Byte reversal */
	in = (((in & 0xaaaaaaaa) >> 1) | ((in & 0x55555555) << 1));
	in = (((in & 0xcccccccc) >> 2) | ((in & 0x33333333) << 2));
	in = (((in & 0xf0f0f0f0) >> 4) | ((in & 0x0f0f0f0f) << 4));

	out |= (in >> 2)&0x3FFFFF;

	/* Extraction */
	if (in & 1) out |= (1 << 23);
	if (in & 2) out |= (1 << 22);

	out -= 0x800004;
	return out;
}


#define blk0(i) (W[i] = data[i])
#define blk2(i) (W[i&15]+=s1(W[(i-2)&15])+W[(i-7)&15]+s0(W[(i-15)&15]))

#define a(i) T[(0-i)&7]
#define b(i) T[(1-i)&7]
#define c(i) T[(2-i)&7]
#define d(i) T[(3-i)&7]
#define e(i) T[(4-i)&7]
#define f(i) T[(5-i)&7]
#define g(i) T[(6-i)&7]
#define h(i) T[(7-i)&7]

#define R(i) h(i)+=S1(e(i))+Ch(e(i),f(i),g(i))+SHA_K[i+j]+(j?blk2(i):blk0(i));\
        d(i)+=h(i);h(i)+=S0(a(i))+Maj(a(i),b(i),c(i))

static void SHA256_Full(unsigned *state, unsigned *data, const unsigned *st)
{
        unsigned W[16];
        unsigned T[8];
        unsigned j;

        T[0] = state[0] = st[0]; T[1] = state[1] = st[1]; T[2] = state[2] = st[2]; T[3] = state[3] = st[3];
        T[4] = state[4] = st[4]; T[5] = state[5] = st[5]; T[6] = state[6] = st[6]; T[7] = state[7] = st[7];
        j = 0;
        for (j = 0; j < 64; j+= 16) { R(0); R(1);  R(2); R(3); R(4); R(5); R(6); R(7); R(8); R(9); R(10); R(11); R(12); R(13); R(14); R(15); }
        state[0] += T[0]; state[1] += T[1]; state[2] += T[2]; state[3] += T[3];
        state[4] += T[4]; state[5] += T[5]; state[6] += T[6]; state[7] += T[7];
}

unsigned test_nonce(unsigned tnon, unsigned *ms, unsigned *w)
{
	unsigned dd[16], st[8], ho[8];
	int i;

	memset(dd, 0, sizeof(dd));
	dd[0] = w[0]; dd[1] = w[1]; dd[2] = w[2]; dd[3] = tnon; dd[4] = 0x80000000; dd[15] = 0x280;
//	for (i = 0; i < 8; i++) st[i] = ms[7-i];
	SHA256_Full(ho, dd, ms);
	memset(dd, 0, sizeof(dd));
	memcpy(dd, ho, 4*8);
	dd[8] = 0x80000000; dd[15] = 0x100;
	SHA256_Full(ho, dd, sha_initial_state);

	if (ho[7] == 0) return 1;

	return 0;
}

unsigned fix_nonce(unsigned innonce, unsigned *ms, unsigned *w)
{
	unsigned tnon;
	unsigned non = decnonce(innonce);

	tnon = non;           if (test_nonce(tnon,ms,w)) return tnon;
	tnon = non-0x400000;  if (test_nonce(tnon,ms,w)) return tnon;
	tnon = non-0x800000;  if (test_nonce(tnon,ms,w)) return tnon;
	tnon = non+0x2800000; if (test_nonce(tnon,ms,w)) return tnon;
	tnon = non+0x2C00000; if (test_nonce(tnon,ms,w)) return tnon;
	tnon = non+0x400000;  if (test_nonce(tnon,ms,w)) return tnon;

	return 0;
}

int spitest_main(unsigned char* osc)
{
	unsigned w[16];
	int i;

  time_t rawtime;
  struct tm * timeinfo;



	/* Each SHA256 is computed by executing 64 permutation round, however in bitcoin specific hashing only 61 permutation rounds
           are sufficient. So we need to precompute effect of first 3 rounds on midstate and feed to chip. Unfortunately implementing
           this in full-custom manner in chip is very cumbersome and timeconsuming, so it is done in software */
	ms3_compute(&atrvec[0]);
	ms3_compute(&atrvec[20]);
	ms3_compute(&atrvec[40]);
	//spi_init();		// has already been done before calling this sub

#ifdef DEBUG_SPITXRX
	/* This is commented out, just to test IN-OUT transmissions with logic analyzer and to see that there's problems with unwanted logic transitions */
	for (;;) spi_txrx(buf,outbuf,16);
#endif

	spi_clear_buf();

	/* After SPI reset we get all chips by default configured in forwarding mode. The reason for that is to forward RESET sequence
           to next chip. RESET chip is nothing else, but non-standard SPI sequence of holding SCK high while toggling MOSI output. So
           first thing that we do - is "breaking" forward chain and force first chip in a sequence to parse incoming input */
	spi_emit_break(); /* First we want to break chain! Otherwise we'll get all of traffic bounced to output */

	spi_emit_data(0x6000, (char*)osc, 8); /* Program internal on-die slow oscillator frequency */	// osc6 was originally

	/* Shut off scan chain COMPLETELY, all 5 should be disabled, but it is unfortunately that any of them will be PROGRAMMED anyway by default  */
	config_reg(7,0); config_reg(8,0); config_reg(9,0); config_reg(10,0); config_reg(11,0);
        
	/* Propagate output to OUTCLK from internal clock to see clock generator performance, it can be measured on OUTCLK pin, and
           different clock programming methods (i.e. slow internal oscillator, fast internal oscillator, external oscillator, with clock
           divider turned on or off) can be verified and compared indirectly by using same output buffer. Signal to OUTCLK is taken from rather
           representative internal node, that is used to feed rounds (close to worst-case). */
	config_reg(6,1);

	/* Enable slow internal osciallators, as we want to get stable clock. Fast oscillator should be used only for faster clocks (like 400 Mhz and more
           or divided), and likely would work bad on slower clocks. Slow internal osciallator likely should be more stable and work at clocks below that */
	config_reg(4,1); /* Enable slow oscillator */
        
	/* Configuring other clock distribution path (i.e. disabling debug step clock, disabling fast oscillator, disabling flip-flop dividing clock) */
	config_reg(1,0); config_reg(2,0); config_reg(3,0);

	spi_emit_data(0x0100, (char*)counters, 16); /* Program counters correctly for rounds processing, here baby should start consuming power */

	/* Prepare internal buffers */
	/* PREPARE BUFFERS (INITIAL PROGRAMMING) */
	memset(&w, 0, sizeof(w)); w[3] = 0xffffffff; w[4] = 0x80000000; w[15] = 0x00000280;

	/* During loads of jobs into chip we load only parts of data that changed, while don't load padding data. Padding data is loaded
           only once during initialization. Padding data is fixed in SHA256 for fixed-size inputs. For more information looks for SHA256 padding section. */
	spi_emit_data(0x1000, (char*)w, 16*4);
	spi_emit_data(0x1400, (char*)w,  8*4);
	memset(w, 0, sizeof(w)); w[0] = 0x80000000; w[7] = 0x100;
	spi_emit_data(0x1900, (char*)&w[0],8*4); /* Prepare MS and W buffers! */

	/* During tests we shall see some nonces in spi_getrxbuf() found from previous searches! */
	/* GREAT TODO - implementing actually miner... But this should be done only when nonces manually validated and dumped from buffer
           and FIXED */

	spi_emit_data(0x3000, (char*)&atrvec[0], 19*4);

	/* SPI TXRX makes this thread sleeping and actually executing hardware bi-directional I/O over SPI - simultaneously sending bits from txbuf
           and receiving bits into RXBUF */
	spi_txrx(hndNanoFury, (char*)spi_gettxbuf(), (char*) spi_getrxbuf(), spi_getbufsz());

	 int oldKey = GetAsyncKeyState(VK_ESCAPE);
	 long curTime = GetTickCount();
	 long prevTime = curTime;
	/* Communication routine with the chip! */
	for (;;) {
		static unsigned oldbuf[17], newbuf[17];
		static unsigned prevms[8], prevw[4];
		static unsigned curvec = 0;
		//static struct timespec ts;
		static unsigned solutions = 0;
		static unsigned tasks = 0;

		//ts.tv_sec = 0;
		//ts.tv_nsec = 100000000; /* Sleep for 100 ms */
		//nanosleep(&ts, 0);
		Sleep(20);

		curTime = GetTickCount();
		time ( &rawtime );
		timeinfo = localtime ( &rawtime );
		printf("testing [%i:%i:%i]...\r", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

		spi_clear_buf(); spi_emit_break();
		spi_emit_data(0x3000, (char*)&atrvec[0], 19*4);
		spi_txrx(hndNanoFury, (char*)spi_gettxbuf(), (char*)spi_getrxbuf(), spi_getbufsz());

		memcpy(newbuf, spi_getrxbuf()+4, 17*4);

		if (newbuf[16] != oldbuf[16]) { /* Job switched! Upload current vector and start processing new one! */
			/* Printing last values */
	//		printf("JOB %u MS0 %08x NONCE %08x RESULTS", newbuf[16]&1, atrvec[0], atrvec[19]);
			for (i = 0; i < 16; i++) {
				if (oldbuf[i] != newbuf[i]) {
					unsigned non = fix_nonce(newbuf[i], prevms, prevw);
					if (non) {

						printf("FOUND VALID SOLUTION %08x MIDSTATE0 %08x NONCE (TV) %08x BUF %08x @ %04u mS [%i:%i:%i]\n", non, prevms[0], prevw[3], newbuf[i], curTime-prevTime, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
						prevTime = curTime;
						++solutions;
					}
				}
			}

			memcpy(oldbuf, newbuf, sizeof(oldbuf));
			
			/* Programming next value */
			for (i = 0; i < 8; i++) { atrvec[8+i] = 0; prevms[i] = atrvec[i]; atrvec[i] = hash_tvec[curvec*20+i]; }
			 prevw[0] = atrvec[16];atrvec[16] = hash_tvec[curvec*20+8]; 
			 prevw[1] = atrvec[17];atrvec[17] = hash_tvec[curvec*20+9]; 
			 prevw[2] = atrvec[18];atrvec[18] = hash_tvec[curvec*20+10];
			 prevw[3] = atrvec[19];atrvec[19] = hash_tvec[curvec*20+11];

			ms3_compute(&atrvec[0]);
			curvec ++; ++tasks;
		}

		if (curvec >= MAX_TVEC) {
			printf("total tasks: %u total solutions: %u\n", tasks, solutions);
			return 0;
		}

		// user hit ESC to cancel ...
		if (oldKey != GetAsyncKeyState(VK_ESCAPE))
			break;
	}
		
	/* Here we shall get in rxbuf all nonces that were found during previous test vector submissions in rxbuf */
	/* These sleeps and atrvec should be replaced with actual job fetching and uploading to chip. while spi_getrxbuf() should be used
           to parse answers. Also this communication by combining also with spi_emit_fasync() / spi_emit_fsync() calls should be used
           to access more chips during single spi_txrx transmission, so long chains of chips can be accessed at once */

	/* So testing will go as following - first make sure that SPI comm works and that oscillations are set up correctly */
	/* Then dump rxbuf and see that it finds correctly answers using test vectors */
	/* Then quickly add mainloop and miner code and get actual hashing */

	return 0;
}

