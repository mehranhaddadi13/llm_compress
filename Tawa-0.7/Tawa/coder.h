/* Coder routines. Includes modifications by Bill Teahan. */

/******************************************************************************
File:		coder.h

Authors: 	John Carpinelli   (johnfc@ecr.mu.oz.au)
	 	Wayne Salamonsen  (wbs@mundil.cs.mu.oz.au)

Purpose:	Data compression using a word-based model and revised 
		arithmetic coding method.

Based on: 	A. Moffat, R. Neal, I.H. Witten, "Arithmetic Coding Revisted",
		Proc. IEEE Data Compression Conference, Snowbird, Utah, 
		March 1995.

Copyright 1995 John Carpinelli and Wayne Salamonsen, All Rights Reserved.

These programs are supplied free of charge for research purposes only,
and may not sold or incorporated into any commercial product.  There is
ABSOLUTELY NO WARRANTY of any sort, nor any undertaking that they are
fit for ANY PURPOSE WHATSOEVER.  Use them at your own risk.  If you do
happen to find a bug, or have modifications to suggest, please report
the same to Alistair Moffat, alistair@cs.mu.oz.au.  The copyright
notice above and this statement of conditions must remain an integral
part of each and every copy made of these files.

******************************************************************************/
#ifndef CODER_H
#define CODER_H


#define		CODE_BITS		32
#define		BYTE_SIZE		8
#define 	MAX_BITS_OUTSTANDING	256
#define 	HALF			((unsigned) 1 << (CODE_BITS-1))
#define 	QUARTER			(1 << (CODE_BITS-2))

#define CODER_MAX_FREQUENCY (1 << 27)   /* Max. frequency for the arithmetic
					   coder */


/* provide external linkage to variables */
extern int f_bits;			/* link to f_bits in stats.c */	
extern unsigned int bytes_input;	/* make available to other modules */
extern unsigned int bytes_output;

extern unsigned int debugCoderLevel;    /* for debugging purposes */


/* function prototypes */
void arithmetic_encode (unsigned int file, unsigned int l, unsigned int h,
			unsigned int t);
unsigned int arithmetic_decode_target (unsigned int file, unsigned int t);
void arithmetic_decode (unsigned int file, unsigned int l, unsigned int h,
			unsigned int t);
void binary_arithmetic_encode (unsigned int file, int c0, int c1, int bit);
int binary_arithmetic_decode (unsigned int file, int c0, int c1);
void start_encode (unsigned int file);
void finish_encode (unsigned int file);
void start_decode (unsigned int file);
void finish_decode (unsigned int file);
void startoutputtingbits (unsigned int file);
void doneoutputtingbits (unsigned int file);
void startinputtingbits (unsigned int file);
void doneinputtingbits (unsigned int file);

void
arith_encode_start (unsigned int file);
/* Starts up the arithmetic encoding. */

void
arith_encode_finish (unsigned int file);
/* Finishes up the arithmetic encoding. */

void
arith_decode_start (unsigned int file);
/* Starts up the arithmetic decoding. */

void
arith_decode_finish (unsigned int file);
/* Finishes up the arithmetic decoding. */

void arith_encode (unsigned int file, unsigned int lbnd, unsigned int hbnd,
		   unsigned int totl);
/* Arithmetically encode the range. */

unsigned int arith_decode_target (unsigned int file, unsigned int totl);
/* Arithmetically decodes the target. */

void arith_decode (unsigned int file, unsigned int lbnd, unsigned int hbnd,
		   unsigned int totl);
/* Arithmetically decode the range. */

unsigned int
arith_read_int (unsigned int file, unsigned int range);
/* Read in the next integer from unsigned int file. */

void
arith_write_int (unsigned int file, unsigned int n, unsigned int range);
/* Write out the integer n from the file. */

#endif
