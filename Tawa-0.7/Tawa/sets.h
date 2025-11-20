/* Routines for sets. */

#ifndef SET_H
#define SET_H

#define POWER_OF_2(n,b) ((unsigned int) n << b)

#define ONE_BIT TRUE
#define ZERO_BIT FALSE

/* Macro definitions for bitsets (sequences of bits of known length
   defined as unsigned char *). */
#define BITSET_TYPE unsigned char
#define BITS_PER_BYTE   8
#define BITSET_SIZE     (sizeof(BITSET_TYPE)*8)
#define BITSET_DELTA(d) ((d) / BITSET_SIZE)
#define BITSET_MASK(d)  (1UL << ((d) % BITSET_SIZE))

#define ZERO_BITSET(bitset,bitlength) \
    (void)memset(bitset,0,(bitlength+BITS_PER_BYTE-1)/BITS_PER_BYTE)

#define SET_BITSET(bitset,position) \
    (bitset[BITSET_DELTA(position)] |= BITSET_MASK(position))

#define CLR_BITSET(bitset,position) \
    (bitset[BITSET_DELTA(position)] &= ~BITSET_MASK(position))

#define ISSET_BITSET(bitset,position) \
    ((bitset[BITSET_DELTA(position)] & BITSET_MASK(position)))

/* Macro definitions for bitsets declared as unsigned ints. */
#define UINT_SIZE (sizeof (unsigned int) *8)
#define UINT_DELTA(d) ((d) / UINT_SIZE)
#define UINT_MASK(d) (1UL << ((d) % UINT_SIZE))
#define UINT_SET(n,p) (n |= UINT_MASK(p))
#define UINT_CLR(n,p) (n &= ~UINT_MASK(p))
#define UINT_ISSET(n,p) (n & UINT_MASK(p))


#endif
