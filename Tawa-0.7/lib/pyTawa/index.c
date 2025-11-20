/* Routines for keeping a variable length array index. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include "io.h"
#include "text.h"
#include "index.h"

#define INDEXES_SIZE 20            /* Initial size of Indexes array */
#define INDEX_SIZE 100             /* Initial size of an index array */
#define INDEX_INVALID 2147483647   /* Used to indicate invalid index */

struct indexType
{ /* Input index to correct, or corrected output index */
    unsigned int Index_length;     /* Length of the index. */
    unsigned int Index_max;        /* Maximum value used in the index. */
    unsigned int *Index;           /* The array index and its values. */
};

#define Index_next Index_length    /* Used to find the "next" on the
				      used list for deleted indexes */

struct indexType *Indexes = NULL;  /* List of index records */
unsigned int Indexes_max_size = 0; /* Current max. size of the Indexes
				      array */
unsigned int Indexes_used = NIL;   /* List of deleted index records */
unsigned int Indexes_unused = 1;   /* Next unused index record */

boolean
TXT_valid_index (unsigned int index)
/* Returns non-zero if the index is valid, zero otherwize. */
{
    if (index == NIL)
        return (FALSE);
    else if (index >= Indexes_unused)
        return (FALSE);
    else if (Indexes [index].Index_length == INDEX_INVALID)
        return (FALSE);
    else
        return (TRUE);
}

unsigned int
TXT_create_index ()
/* Creates and initializes an index (for storing a variable-length array of
   unsigned ints. */
{
    struct indexType *index;
    unsigned int i, old_size;

    if (Indexes_used != NIL)
    {	/* use the first list of indexes on the used list */
	i = Indexes_used;
	Indexes_used = Indexes [i].Index_next;
    }
    else
    {
	i = Indexes_unused;
	if (Indexes_unused+1 >= Indexes_max_size)
	{ /* need to extend Indexes array */
	    old_size = Indexes_max_size * sizeof (struct indexType);
	    if (Indexes_max_size == 0)
		Indexes_max_size = INDEXES_SIZE;
	    else
		Indexes_max_size *= 2; /* Keep on doubling the Indexes array on demand */
	    Indexes = (struct indexType *) Realloc (103, Indexes,
		     Indexes_max_size * sizeof (struct indexType), old_size);

	    if (Indexes == NULL)
	    {
		fprintf (stderr, "Fatal error: out of indexes space\n");
		exit (1);
	    }
	}
	Indexes_unused++;
    }

    if (i != NIL)
    {
      index = Indexes + i;

      index->Index_length = 0;
      index->Index_max = 0;
      index->Index = NULL;
    }

    return (i);
}

void
TXT_release_index (unsigned int index)
/* Releases the memory allocated to the index and the index number (which may
   be reused in later TXT_create_index calls). */
{
    struct indexType *indexp;

    assert (TXT_valid_index (index));

    indexp = Indexes + index;
    if (indexp->Index != NULL)
        Free (104, indexp->Index, indexp->Index_length *
	      sizeof (unsigned int));

    indexp->Index_length = INDEX_INVALID;
    /* This is used for testing later on if index no. is valid or not */

    /* Append onto head of the used list */
    indexp->Index_next = Indexes_used;
}

unsigned int
TXT_max_index (unsigned int index)
/* Returns the maximum index used the array index. */
{
    assert (TXT_valid_index (index));
    assert (index != NIL);

    return (Indexes [index].Index_max);
}

boolean
TXT_get_index (unsigned int index, unsigned int pos, unsigned int *value)
/* Returns the value associated with the position pos in the array index.
   Returns FALSE if the value does not exist i.e. pos exceeds the current
   array bounds. */
{
    assert (TXT_valid_index (index));
    assert (index != NIL);

    if (pos >= Indexes [index].Index_length)
        return (FALSE);
    else if (pos > Indexes [index].Index_max)
        return (FALSE);
    else
      {
	*value = Indexes [index].Index [pos];
	return (TRUE);
      }
}

void
TXT_put_index (unsigned int index, unsigned int pos, unsigned int value)
/* Inserts the value at position pos into the array index. Extends the
   array bounds so that the value can be inserted. */
{
    unsigned int old_size, length;

    assert (TXT_valid_index (index));
    assert (index != NIL);

    if (pos >= Indexes [index].Index_length)
      { /* Need to extend array */

	length = Indexes [index].Index_length;
	if (length == 0)
	    old_size = 0;
	else
	    old_size = length * sizeof(unsigned int);

	if (length == 0)
	    length  = INDEX_SIZE;

	if (pos >= length)
	    length = (10 * (pos + 50))/9; /* Expand array to fit */

	Indexes [index].Index_length = length;

	Indexes [index].Index = (unsigned int *)
	  Realloc (12, Indexes [index].Index,
		   (length+2) * sizeof(unsigned int), old_size);
	if (Indexes [index].Index == NULL)
	{
	    fprintf (stderr, "Fatal error: out of text space at position %d\n",
		     index);
	    exit (1);
	}
      }

    Indexes [index].Index [pos] = value;
    if (pos > Indexes [index].Index_max)
        Indexes [index].Index_max = pos;
}

void
TXT_dump_index
(unsigned int file, unsigned int index,
 void (*dump_value_function) (unsigned int, unsigned int, unsigned int))
/* Dumps out a human readable version of the text record to the file.
   The argument dump_symbol_function is a pointer to a function for printing
   values stored in the array index. If this is NULL, then each non-zero
   symbol will be printed as an unsigned int along with its index. */
{
    struct indexType *indexp;
    unsigned int value, p;

    assert (TXT_valid_file (file));
    assert (TXT_valid_index (index));
    indexp = Indexes + index;

    assert (indexp->Index_length > 0);
    assert (indexp->Index != NULL);

    fprintf (Files [file], "Index length %d max %d\n",
	     indexp->Index_length, indexp->Index_max);
    for (p=0; p <= indexp->Index_max; p++)
    {
	value = indexp->Index [p];
	if (dump_value_function)
	    dump_value_function (file, p, value);
	else if (value > 0)
	    fprintf (Files [file], "%6d: %d\n", p, value);
    }
}
