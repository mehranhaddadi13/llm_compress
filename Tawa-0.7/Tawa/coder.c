/* Coder routines. Includes modifications by Bill Teahan. */

/******************************************************************************
File:		coder.c

Authors: 	John Carpinelli   (johnfc@ecr.mu.oz.au)
	 	Wayne Salamonsen  (wbs@mundil.cs.mu.oz.au)

Purpose:	Data compression using a word-based model and revised 
		arithmetic coding method.

Based on: 	A. Moffat, R. Neal, I.H. Witten, "Arithmetic Coding Revisted",
		Proc. IEEE Data Compression Conference, Snowbird, Utah, 
		March 1995.

		Low-Precision Arithmetic Coding Implementation by 
		Radford M. Neal



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
#include <stdio.h>
#include <assert.h>
#include "io.h"
#include "coder.h"

unsigned long	L;				/* lower bound */
unsigned long	R;				/* code range */
unsigned long	V;				/* current code value */
unsigned long 	r;				/* normalized range */

int 		bits_outstanding;		/* follow bit count */
int	 	buffer;				/* I/O buffer */
int		bits_to_go;			/* bits left in buffer */
unsigned int	bytes_input, bytes_output;	/* I/O counters */

boolean Arith_Encoder_Started = FALSE;
boolean Arith_Decoder_Started = FALSE;

unsigned int debugCoderLevel = 0; /* for debugging purposes */

/*
 *
 * responsible for outputing the bit passed to it and an opposite number of
 * bit equal to the value stored in bits_outstanding
 *
 */
#define BIT_PLUS_FOLLOW(file,b) 	\
do                                      \
{ 	  			        \
    OUTPUT_BIT(file,(b));               \
    while (bits_outstanding > 0)	\
    { 					\
	OUTPUT_BIT(file,!(b));      	\
	bits_outstanding -= 1;    	\
    } 	                		\
} while (0)


/*
 *
 * responsible for outputting one bit. adds the bit to a buffer 
 * and once the buffer has 8 bits, it outputs a character
 *
 */
#define OUTPUT_BIT(file,b)           	\
do { 					\
    buffer >>= 1;             		\
    if (b) 				\
	buffer |= 1 << (BYTE_SIZE-1);	\
    bits_to_go -= 1;            	\
    if (bits_to_go == 0)        	\
    { 					\
	putc(buffer, Files [file]); 	\
	bytes_output += 1;		\
        bits_to_go = BYTE_SIZE;      	\
    }	                       		\
} while (0)


/*
 *
 * reads in bits from encoded file 
 * reads in a char at a time into a buffer i.e 8 bits
 *
 */
#define ADD_NEXT_INPUT_BIT(file,v)   	\
do { 					\
    bits_to_go -= 1;			\
    if (bits_to_go < 0) 		\
    { 					\
	buffer = fgetc (Files [file]);	\
	bits_to_go = BYTE_SIZE - 1;	\
    } 					\
    v += v + (buffer & 1); 		\
    buffer >>= 1; 			\
} while (0) 

/*
 * output code bits until the range as been expanded
 * to above QUARTER
 */
#define ENCODE_RENORMALISE(file)	\
do {					\
    while (R < QUARTER)			\
    {					\
        if (L >= HALF)			\
    	{				\
    	    BIT_PLUS_FOLLOW(file,1);	\
    	    L -= HALF;			\
    	}				\
    	else if (L+R <= HALF)		\
    	{				\
    	    BIT_PLUS_FOLLOW(file,0);	\
    	}				\
    	else 				\
    	{				\
    	    bits_outstanding++;		\
    	    L -= QUARTER;		\
    	}				\
    	L += L;				\
    	R += R;				\
    }					\
} while (0)


/*
 * input code bits until range has been expanded to
 * more than QUARTER. Mimics encoder.
 */
#define DECODE_RENORMALISE(file)	\
do {					\
    while (R < QUARTER)			\
    {					\
    	if (L >= HALF)			\
    	{				\
    	    V -= HALF;			\
    	    L -= HALF;			\
    	    bits_outstanding = 0;	\
    	}				\
    	else if (L+R <= HALF)		\
    	{				\
    	    bits_outstanding = 0;	\
    	}				\
    	else				\
    	{				\
    	    V -= QUARTER;		\
    	    L -= QUARTER;		\
    	    bits_outstanding++;		\
    	}				\
    	L += L;				\
    	R += R;				\
    	ADD_NEXT_INPUT_BIT(file,V);     \
    }					\
} while (0)


/*
 *
 * encode a symbol given its low, high and total frequencies
 *
 */
void 
arithmetic_encode (unsigned int file, unsigned int low, unsigned int high,
		   unsigned int total)
{
    unsigned long temp; 

    assert (TXT_valid_file (file));

#ifndef SHIFT_ADD
    r = R/total;
    temp = r*low;
    L += temp;
    if (high < total)
	R = r*(high-low);
    else
	R -= temp;
#else
{
    int i, nShifts;
    unsigned long numerator, denominator;
    unsigned long temp2;

    /*
     * calculate r = R/total, temp = r*low and temp2 = r*high
     * using shifts and adds 
     */
    numerator = R;
    nShifts = CODE_BITS - f_bits - 1;
    denominator = total << nShifts;
    r = 0;
    temp = 0;
    temp2 = 0;
    for (i = nShifts;; i--) 
    {
        if (numerator >= denominator) 
	{ 
	    numerator -= denominator; 
	    r++; 
	    temp += low;
	    temp2 += high;
	}
	if (i == 0) break;
        numerator <<= 1; r <<= 1; temp <<= 1; temp2 <<= 1;
    }
    L += temp;
    if (high < total)
	R = temp2 - temp;
    else
	R -= temp;
}
#endif

    ENCODE_RENORMALISE (file);

    if (bits_outstanding >= MAX_BITS_OUTSTANDING)
    {
	finish_encode (file);
	start_encode (file);
    }
}



/*
 *
 * decode the target value using the current total frequency
 * and the coder's state variables
 *
 */
unsigned 
int arithmetic_decode_target (unsigned int file, unsigned int total)
{
    unsigned long target;
    
    assert (TXT_valid_file (file));

#ifndef SHIFT_ADD
    r = R/total;
    target = (V-L)/r;
#else 
{	
    int i, nShifts;
    unsigned long numerator, denominator;

    /* divide r = R/total using shifts and adds */
    numerator = R;
    nShifts = CODE_BITS - f_bits - 1;
    denominator = total << nShifts;
    r = 0;
    for (i = nShifts;; i--) 
    {
        if (numerator >= denominator) 
	{ 
	    numerator -= denominator; 
	    r++; 
	}
	if (i == 0) break;
        numerator <<= 1; r <<= 1;
    }

    /* divide V-L by r using shifts and adds */
    if (r < (1 << (CODE_BITS - f_bits - 1)))
	nShifts = f_bits;
    else
	nShifts = f_bits - 1;
    numerator = V - L;
    denominator = r << nShifts;
    target = 0;
    for (i = nShifts;; i--) 
    {
        if (numerator >= denominator) 
	{ 
	    numerator -= denominator; 
	    target++; 
	}
	if (i == 0) break;
        numerator <<= 1; target <<= 1;
    }
}
#endif
    return (target >= total? total-1 : target);
}



/*
 *
 * decode the next input symbol
 *
 */
void 
arithmetic_decode (unsigned int file, unsigned int low, unsigned int high,
		   unsigned int total)
{     
    unsigned int temp;

    assert (TXT_valid_file (file));

#ifndef SHIFT_ADD
    /* assume r has been set by decode_target */
    temp = r*low;
    L += temp;
    if (high < total)
	R = r*(high-low);
    else
	R -= temp;
#else
{
    int i, nShifts;
    unsigned long temp2;
    
    /* calculate r*low and r*high using shifts and adds */
    r <<= f_bits;
    temp = 0;
    nShifts = CODE_BITS - f_bits - 1;
    temp2 = 0;
    for (i = nShifts;; i--) 
    {
	if (r >= HALF)
	{ 
	    temp += low;
	    temp2 += high;
	}
	if (i == 0) break;
        r <<= 1; temp <<= 1; temp2 <<= 1;
    }
    L += temp;
    if (high < total)
	R = temp2 - temp;
    else
	R -= temp;
 }
#endif

    DECODE_RENORMALISE (file);

    if (bits_outstanding >= MAX_BITS_OUTSTANDING)
    {
	finish_decode (file);
	start_decode (file);
    }
}



/*
 * 
 * encode a binary symbol using specialised binary encoding
 * algorithm
 *
 */
void
binary_arithmetic_encode (unsigned int file,int c0, int c1, int bit)
{
    int LPS, cLPS, rLPS;

    assert (TXT_valid_file (file));

    if (c0 < c1) 
    {
	LPS = 0;
	cLPS = c0;
    } else {
	LPS = 1;
	cLPS = c1;
    }
#ifndef SHIFT_ADD
    r = R / (c0+c1);
    rLPS = r * cLPS;
#else
{	
    int i, nShifts;
    unsigned long int numerator, denominator;

    numerator = R;
    nShifts = CODE_BITS - f_bits - 1;
    denominator = (c0 + c1) << nShifts;
    r = 0;
    rLPS = 0;
    for (i = nShifts;; i--) 
    {
	if (numerator >= denominator) 
	{ 
	    numerator -= denominator; 
	    r++;
	    rLPS += cLPS;
	}
	if (i == 0) break;
	numerator <<= 1; r <<= 1; rLPS <<= 1;
    }
}
#endif
    if (bit == LPS) 
    {
	L += R - rLPS;
	R = rLPS;
    } else {
	R -= rLPS;
    }

    /* renormalise, as for arith_encode */
    ENCODE_RENORMALISE (file);

    if (bits_outstanding > MAX_BITS_OUTSTANDING)
    {
	finish_encode (file);
	start_encode (file);
    }
}



/*
 *
 * decode a binary symbol given the frequencies of 1 and 0 for
 * the context
 *
 */
int
binary_arithmetic_decode (unsigned int file, int c0, int c1)
{
    int LPS, cLPS, rLPS, bit;

    assert (TXT_valid_file (file));

    if (c0 < c1) 
    {
	LPS = 0;
	cLPS = c0;
    } else {
	LPS = 1;
	cLPS = c1;
    }
#ifndef SHIFT_ADD
    r = R / (c0+c1);
    rLPS = r * cLPS;
#else 
{
    int i, nShifts;
    unsigned long int numerator, denominator;

    numerator = R;
    nShifts = CODE_BITS - f_bits - 1;
    denominator = (c0 + c1) << nShifts;
    r = 0;
    rLPS = 0;
    for (i = nShifts;; i--) 
    {
	if (numerator >= denominator) 
	{ 
	    numerator -= denominator; 
	    r++;
	    rLPS += cLPS;
	}
	if (i == 0) break;
	numerator <<= 1; r <<= 1; rLPS <<= 1;
    }
}
#endif
    if ((V-L) >= (R-rLPS)) 
    {
	bit = LPS;
	L += R - rLPS;
	R = rLPS;
    } else {
	bit = (1-LPS);
	R -= rLPS;
    }

    /* renormalise, as for arith_decode */
    DECODE_RENORMALISE (file);

    if (bits_outstanding > MAX_BITS_OUTSTANDING)
    {
	finish_decode (file);
	start_decode (file);
    }
    return(bit);
}




/*
 *
 * start the encoder
 *
 */
void 
start_encode (unsigned int file)
{
    assert (TXT_valid_file (file));

    L = 0;
    R = HALF-1;
    bits_outstanding = 0;
}



/*
 *
 * finish encoding by outputting follow bits and three further
 * bits to make the last symbol unambiguous
 * could tighten this to two extra bits in some cases,
 * but does anybody care?
 *
 */
void 
finish_encode (unsigned int file)
{
    int bits, i;
    const int nbits = 3;

    assert (TXT_valid_file (file));

    bits = (L+(R>>1)) >> (CODE_BITS-nbits);
    for (i = 1; i <= nbits; i++)     	/* output the nbits integer bits */
        BIT_PLUS_FOLLOW (file,((bits >> (nbits-i)) & 1));
}



/*
 *
 * start the decoder
 *
 */
void 
start_decode(unsigned int file)
{
    int 	i;
    static int	fill_V = 1;

    assert (TXT_valid_file (file));

    if (fill_V)
    {
	V = 0;
	for (i = 0; i<CODE_BITS; i++)
	    ADD_NEXT_INPUT_BIT (file,V);
	fill_V = 0;
    }
    L = 0;
    R = HALF - 1;
    bits_outstanding = 0;

    r = 0;
}


/*
 *
 * finish decoding by consuming the disambiguating bits generated
 * by finish_encode
 *
 */
void 
finish_decode (unsigned int file)
{
    int i;
    const int nbits = 3;

    assert (TXT_valid_file (file));

    for (i = 1; i <= nbits; i++)
	ADD_NEXT_INPUT_BIT (file,V);	
    bits_outstanding = 0;
}


/*
 *
 * initialize the bit output function
 *
 */
void 
startoutputtingbits (unsigned int file)
{
    assert (TXT_valid_file (file));

    buffer = 0;
    bits_to_go = BYTE_SIZE;
}


/*
 *
 * start the bit input function
 *
 */
void 
startinputtingbits (unsigned int file)
{
    assert (TXT_valid_file (file));

    bits_to_go = 0;
}



/*
 *
 * complete outputting bits
 *
 */
void 
doneoutputtingbits (unsigned int file)
{
    assert (TXT_valid_file (file));

    putc (buffer >> bits_to_go, Files [file]);
    bytes_output += 1;
}


/*
 *
 * complete inputting bits
 *
 */
void 
doneinputtingbits (unsigned int file)
{
    assert (TXT_valid_file (file));

    bits_to_go = 0;
}

void
arith_encode_start (unsigned int file)
/* Starts up the arithmetic encoding. */
{
    assert (TXT_valid_file (file));

    Arith_Encoder_Started = TRUE;
    bytes_output = 0;
    startoutputtingbits (file);
    start_encode (file);
}

void
arith_encode_finish (unsigned int file)
/* Finishes up the arithmetic encoding. */
{
    assert (TXT_valid_file (file));

    finish_encode (file);
    doneoutputtingbits (file);
}
void
arith_decode_start (unsigned int file)
/* Starts up the arithmetic decoding. */
{
    assert (TXT_valid_file (file));

    Arith_Decoder_Started = TRUE;
    bytes_input = 0;
    startinputtingbits (file);
    start_decode (file);
}

void
arith_decode_finish (unsigned int file)
/* Finishes up the arithmetic decoding. */
{
    assert (TXT_valid_file (file));

    finish_decode (file);
    doneinputtingbits (file);
}

void
arith_encode (unsigned int file, unsigned int lbnd, unsigned int hbnd,
	      unsigned int totl)
/* Arithmetically encode the range. */
{
    assert (Arith_Encoder_Started);
    assert (TXT_valid_file (file));

    if (Debug.coder)
      fprintf (stderr, "Coder encode: lbnd %d hbnd %d totl %d\n",
	       lbnd, hbnd, totl);

    if (lbnd || hbnd || totl)
      { /* Ignore lbnd == hbnd == totl == 0 case */
	if ((lbnd >= hbnd) || (hbnd > totl))
	    fprintf( stderr, "Fatal error - invalid range : lbnd %d hbnd %d totl %d\n",
		     lbnd, hbnd, totl );
	assert ((lbnd < hbnd) && (hbnd <= totl));
      }

    if ((lbnd == 0) && (hbnd == totl))
    {
	/* probility = 1 - no need to encode it */
    }
    else
	arithmetic_encode (file, lbnd, hbnd, totl);
}

unsigned int
arith_decode_target (unsigned int file, unsigned int totl)
/* Arithmetically decodes the target. */
{
    int target;

    assert (Arith_Decoder_Started);
    assert (TXT_valid_file (file));

    if (totl == 0)
      return (0);

    target = arithmetic_decode_target (file, totl);

    if (Debug.coder_target)
      fprintf (stderr, "Coder decode target: target %d totl %d\n",
	       target, totl);

    return (target);
}

void
arith_decode (unsigned int file, unsigned int lbnd, unsigned int hbnd,
	      unsigned int totl)
/* Arithmetically decode the range. */
{
    assert (Arith_Decoder_Started);
    assert (TXT_valid_file (file));

    if (Debug.coder)
      fprintf (stderr, "Coder decode: lbnd %d hbnd %d totl %d\n",
	       lbnd, hbnd, totl);

    if (lbnd || hbnd || totl)
      { /* Ignore lbnd == hbnd == totl == 0 case */
	if ((lbnd >= hbnd) || (hbnd > totl))
	    fprintf( stderr, "Fatal error - invalid range : lbnd %d hbnd %d totl %d\n",
		     lbnd, hbnd, totl );
	assert ((lbnd < hbnd) && (hbnd <= totl));
      }

    if ((lbnd == 0) && (hbnd == totl))
      {
        /* probability = 1 - no need to decode it */
      }
    else
      {
        arithmetic_decode (file, lbnd, hbnd, totl);
      }
}

unsigned int arith_read_int (unsigned int file, unsigned int range)
/* Read in the next integer from the file. */
{
    unsigned int n;

    assert (Arith_Decoder_Started);
    assert (TXT_valid_file (file));

    n = arithmetic_decode_target (file, range);
    arithmetic_decode (file, n, n+1, range);

    if (debugCoderLevel > 4)
        fprintf (stderr, "Reading int %d range %d\n", n, range);

    return (n);
}

void arith_write_int (unsigned int file, unsigned int n, unsigned int range)
/* Write out the integer n from the file. */
{
    assert (Arith_Encoder_Started);
    assert (TXT_valid_file (file));

    if (debugCoderLevel > 4)
        fprintf (stderr, "Writing int %d range %d\n", n, range);

    arithmetic_encode (file, n, n+1, range);
}
