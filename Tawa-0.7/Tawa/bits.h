/*
 * bits.h
 *
 * This was hacked together by Stuart Inglis (singlis@cs.waikato.ac.nz)
 *
 * If there are bugs, track them down, squash them, then write to me with
 * the fix :-)
 *
 */


#ifndef _BITS_TYPE_H
#define _BITS_TYPE_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* don't set BITS_DEFAULT to less bits than the size of BITS_TYPE. 
   eg. if BITS_TYPE is char, BITS_DEFAULT must be at least 8 */

#define CEIL(a) ((a)==(int)(a) ? (a) : (a)>0 ? 1+(int)(a) : -(1+(int)(-(a))))

#define BITS_DEFAULT 128
#define BITS_TYPE unsigned int

typedef struct {
  unsigned int _alloc;      /* actual number of (BITS_TYPE) that were allocated  */
  unsigned int len;        
  BITS_TYPE *bits;
} bits_type;

#define BITS_NUM      (sizeof(BITS_TYPE)*8)
#define BITS_DELTA(d) ((d) / BITS_NUM)
#define BITS_DELTA_F(d) ((d)*1.0 / BITS_NUM)
#define BITS_MASK(d)  (1UL << ((d) % BITS_NUM))

#define BITS_ZERO(array) \
  (void)memset((array)->bits,0,sizeof(BITS_TYPE)*((array)->_alloc))

#define BITS_ALLOC(array) \
   {array=(bits_type*)malloc(sizeof(bits_type));\
    assert(array && "bits.h:out of memory");\
   (array)->len=BITS_DEFAULT;\
    (array)->_alloc=(int)CEIL(BITS_DELTA_F((array)->len));\
    (array)->bits=(BITS_TYPE*)calloc((array)->_alloc,sizeof(BITS_TYPE));\
    assert((array)->bits && "bits.h:out of memory");}

#define BITS_COPY(dest,source) \
   {int _c;dest=(bits_type*)malloc(sizeof(bits_type));\
   assert(dest && "bits.h:out of memory");\
   (dest)->len=(source)->len;\
   _c=(int)CEIL(BITS_DELTA_F(source->len));\
   (dest)->_alloc=(source)->_alloc;\
   (dest)->bits=(BITS_TYPE*)malloc(sizeof(BITS_TYPE)*(_c));\
    assert((dest)->bits && "bits.h:out of memory");\
    memcpy((dest)->bits,(source)->bits,sizeof(BITS_TYPE)*(_c));}


#define BITS_EXTEND(array,position)\
     {if(position>=(array)->len){int _t,_i,_o;\
       _o=(array)->_alloc;\
       (array)->len=6*position/5;\
       _t=(int)CEIL(BITS_DELTA_F((array)->len));\
       if(_o!=_t){(array)->bits=(BITS_TYPE*)realloc((array)->bits,sizeof(BITS_TYPE)*(_t));\
       assert((array)->bits && "bits.h:out of memory");\
       for(_i=_o;_i<_t;_i++)(array)->bits[_i]=0;(array)->_alloc=_t;}\
       }}

#define BITS_SET(array,position)\
     {BITS_EXTEND(array,position);\
     (array)->bits[BITS_DELTA(position)] |= BITS_MASK(position);}


#define BITS_FREE(array) \
     {(void)free(array->bits);array->len=0;(void)free(array);}
   
#define BITS_CLR(array,position) ((array)->bits[BITS_DELTA(position)] &= ~BITS_MASK(position))

#define BITS_ISSET(array,position) (((position)<((array)->len))?((array)->bits[BITS_DELTA(position)] & BITS_MASK(position)):0)


#endif
