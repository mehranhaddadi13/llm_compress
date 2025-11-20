/* Routines for storing symbols in a cumulative probability table. Based on
   Alistair Moffat's paper "An improved data structure for Cumulative
   Probability tables", Software: Practice and Experience, 1999. */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "io.h"
#include "text.h"
#include "model.h"
#include "ptable.h"

#define PTABLE_ESCAPE_SYMBOL 1    /* Where the number of singletons (1-counts) are stored; this is used
				     for calculating the "escape" probability */
#define PTABLE_SENTINEL_SYMBOL 2  /* Where the count for the break symbol is stored; this is used
				     for calculating the "escape" probability */
#define PTABLE_START_SYMBOL 3     /* The first symbol starts at position 3 after the escape and the break */ 

#define FORWARD(s) ((s) + ((s) & -(s)))
#define BACKWARD(s) ((s) & ((s) - 1))	

ptable_type *
ptable_create_table (unsigned int max)
/* Creates a new probability table of the specified type to contain max symbols initially. */
{
    unsigned int *cfreq;
    ptable_type *ptable;

    ptable = (ptable_type *) Malloc (71, sizeof (ptable_type));
    ptable->ctotal = 0;
    ptable->alloc = max + PTABLE_START_SYMBOL;
    ptable->max = max;
    cfreq = (unsigned int *) Malloc (72, (max + PTABLE_START_SYMBOL) *
				     sizeof (unsigned int));
    cfreq [PTABLE_ESCAPE_SYMBOL] = 1; /* set the number of singletons to 1 */
    ptable->cfreq = cfreq;
    ptable_increment_count (ptable, TXT_sentinel_symbol (), 1);
    return (ptable);
}

void
ptable_alloc_table (ptable_type *ptable, unsigned int s)
/* Ensures that ptable has enough allocation to store symbol s. */
{
    unsigned int alloc, old_alloc, old_size, max, i;

    assert (ptable != NULL);
    alloc = ptable->alloc;
    max = ptable->max;

    if (s > max)
      {
	ptable->max = s;

	if (s >= alloc)
	  { /* need to extend the array */
	    if (alloc == 0)
	        old_size = 0;
	    else
	        old_size = (alloc + PTABLE_START_SYMBOL) *
		  sizeof (unsigned int);
	    old_alloc = alloc;
	    alloc = 10 * (alloc + 50) / 9;
	    if (s > alloc)
	        alloc = s + PTABLE_START_SYMBOL;
	    ptable->cfreq = (unsigned int *) Realloc (73, ptable->cfreq,
	       (alloc+PTABLE_START_SYMBOL) * sizeof (unsigned int), old_size);
	    for (i=old_alloc; i<alloc+PTABLE_START_SYMBOL-1; i++)
	        ptable->cfreq [i] = 0;

	    ptable->alloc = alloc;
	  }
      }
}

ptable_type *
ptable_copy_table (ptable_type *ptable)
/* Creates a new probability table of the specified type to contain max symbols initially. */
{
    ptable_type *new_ptable;
    unsigned int i, max;

    assert (ptable != NULL);

    max = ptable->max;
    new_ptable = ptable_create_table (max);
    new_ptable->ctotal = ptable->ctotal;

    for (i = 0; i <= max; i++)
      new_ptable->cfreq [i] = ptable->cfreq [i];

    return (new_ptable);
}

unsigned int
ptable_get_lbnd (ptable_type *ptable, unsigned int s)
/* Returns the cumulative frequency of the symbols prior to s in the alphabet */
{
    unsigned int lbnd, p, q, *cfreq, max;

    assert (ptable != NULL);
    cfreq = ptable->cfreq;
    max = ptable->max;

    if (s == TXT_sentinel_symbol ())
        s = PTABLE_SENTINEL_SYMBOL;
    else
        s += PTABLE_START_SYMBOL;

    if (s > max) /* symbol does not exist yet */
        return (0); /* return lower bound for escape probability */

    p = 1;
    lbnd = 0;

    while (p < s)
      {
	lbnd += cfreq [p];
	p += p;
      }

    q = s;
    while ((q != p) && (q <= max))
      {
	lbnd -= cfreq [q];
	q = FORWARD (q);
      }
    return (lbnd);
}

unsigned int
ptable_get_count (ptable_type *ptable, unsigned int s)
/* Returns the frequency count of symbol s. */
{
    unsigned int *cfreq, max, count, q, z;

    assert (ptable != NULL);
    cfreq = ptable->cfreq;
    max = ptable->max;

    if (s == TXT_sentinel_symbol ())
        s = PTABLE_SENTINEL_SYMBOL;
    else
        s += PTABLE_START_SYMBOL;

    if (s > max) /* symbol does not exist yet */
        return (cfreq [PTABLE_ESCAPE_SYMBOL]); /* return the escape count = # singletons + 1 */

    count = cfreq [s];
    q = s + 1;
    z = FORWARD (s);
    if (z > max + 1)
        z = max + 1;
    while (q < z)
      {
	count -= cfreq [q];
	q = FORWARD (q);
      }
    return (count);
}

unsigned int
ptable_get_escape_symbol (ptable_type *ptable)
/* Returns the escape symbol number (i.e. max symbol + 1). */
{
    assert (ptable != NULL);

    return (ptable->max + 1 - PTABLE_START_SYMBOL); /* subtract because escape probability is at 1, break is at 2 and
						       symbol 0 is at 3 */
}

unsigned int
ptable_get_escape_count (ptable_type *ptable)
/* Returns the escape count. */
{
    assert (ptable != NULL);

    return (ptable->cfreq [PTABLE_ESCAPE_SYMBOL]); /* return the escape count = # singletons + 1 */
}

unsigned int
ptable_get_total (ptable_type *ptable)
/* Returns the total count of all symbols. */
{
    assert (ptable != NULL && ptable->cfreq != NULL);

    return (ptable->ctotal + ptable->cfreq [PTABLE_ESCAPE_SYMBOL]);
}

void
ptable_increment_count (ptable_type *ptable, unsigned int s, unsigned int increment)
/* Add one to the frequency of symbol s. */
{
    unsigned int *cfreq, count, p;

    assert (ptable != NULL);

    /* update the number of singletons */
    if (s == TXT_sentinel_symbol ())
        s = PTABLE_SENTINEL_SYMBOL;
    else
      {
	count = ptable_get_count (ptable, s);
	s += PTABLE_START_SYMBOL;
	if (s > ptable->max) /* new symbol */
	  {
	    if (increment == 1)
	        ptable->cfreq [PTABLE_ESCAPE_SYMBOL]++;
	  }
	else if (count == 1)
            ptable->cfreq [PTABLE_ESCAPE_SYMBOL]--;
      }

    ptable_alloc_table (ptable, s);
    cfreq = ptable->cfreq;

    p = s;
    while (p > 0)
      {
	cfreq [p] += increment;
	p = BACKWARD (p);
      }

    ptable->ctotal += increment;
}

unsigned int
ptable_get_symbol (ptable_type *ptable, unsigned int target, unsigned int *lbnd, unsigned int *count)
/* Returns the symbol number s such that:

        ptable_get_lbnd (s) <= target < ptable_get_lbnd (s) + ptable_get_count (s).

   Also returns the value lbnd that would be returned by ptable_get_lbnd (s)
   and the value count. */
{
    unsigned int lbnd1, s, p, e, m, *cfreq, max;

    assert (ptable != NULL);
    cfreq = ptable->cfreq;
    max = ptable->max;

    p = 1;
    lbnd1 = 0;
    while ((p + p <= max) && (cfreq [p] <= target))
      {
	target -= cfreq [p];
	lbnd1 += cfreq [p];
	p += p;
      }

    s = p;

    m = p / 2;
    e = 0;

    while (m >= 1)
      {
	if (s + m <= max)
	  {
	    e += cfreq [s+m];
	    if ((cfreq [s] - e) <= target)
	      {
		target -= (cfreq [s] - e);
		lbnd1 += (cfreq [s] - e);
		s += m;
		e = 0;
	      }
	  }
	m /= 2;
      }

    *lbnd = lbnd1;

    if (s == PTABLE_ESCAPE_SYMBOL)
      { /* we have found the escape symbol */
	*count = ptable->cfreq [PTABLE_ESCAPE_SYMBOL];
	return (ptable->max - 2); /* max symbol plus 1 */
      }

    if (s == PTABLE_SENTINEL_SYMBOL)
        s = TXT_sentinel_symbol ();
    else
        s -= PTABLE_START_SYMBOL;

    *count = ptable_get_count (ptable, s);

    return (s);
}

void
ptable_dump_table (unsigned int file, ptable_type *ptable)
/* Dumps out the table of cumulative frequencies. */
{
    unsigned int *cfreq, max, p;

    assert (TXT_valid_file (file));

    fprintf (Files [file], "Dump of cumulative probability table: \n");
    if (ptable == NULL)
      {
        fprintf (Files [file], "Table is NULL\n");
	return;
      }

    cfreq = ptable->cfreq;
    max = ptable->max;

    fprintf (Files [file], "escape symbol: count %5d\n", cfreq [PTABLE_ESCAPE_SYMBOL]);
    fprintf (Files [file], "symbol: break count: %5d cumulative count %5d\n",
		 ptable_get_count (ptable, TXT_sentinel_symbol ()), cfreq [PTABLE_SENTINEL_SYMBOL]);
    for (p=PTABLE_START_SYMBOL; p < max + PTABLE_START_SYMBOL; p++)
      if (cfreq [p])
        fprintf (Files [file], "symbol: %5d count: %5d cumulative count %5d\n",
		 p-PTABLE_START_SYMBOL, ptable_get_count (ptable, p-2), cfreq [p]);
}

void
ptable_dump_symbols (unsigned int file, ptable_type *ptable)
/* Dumps out all the symbols in ptable. */
{
    unsigned int s, lbnd, total;

    assert (TXT_valid_file (file));

    fprintf (Files [file], "Dump of cumulative probability table: \n");
    if (ptable == NULL)
      {
        fprintf (Files [file], "Table is NULL\n");
	return;
      }

    total = ptable_get_total (ptable);

    s = ptable_get_escape_symbol (ptable) + 1;
    lbnd = ptable_get_lbnd (ptable, s);
    fprintf (Files [file], "escape   lbnd %d hbnd %d totl %d\n", lbnd,
	    lbnd + ptable_get_count (ptable, s), total);
    lbnd = ptable_get_lbnd (ptable, TXT_sentinel_symbol ());
    fprintf (Files [file], "sentinel lbnd %d hbnd %d totl %d\n", lbnd,
	    lbnd + ptable_get_count (ptable, TXT_sentinel_symbol ()), total);
    for (s = 0; s < ptable_get_escape_symbol (ptable); s++)
      {
	lbnd = ptable_get_lbnd (ptable, s);
	fprintf (Files [file], "symbol %d lbnd %d hbnd %d totl %d\n", s, lbnd,
		lbnd + ptable_get_count (ptable, s), total);
      }
}

void
ptable_write_table (unsigned int file, ptable_type *ptable)
/* Writes out the table to the file which then can be re-loaded
   at a later date. */
{
    unsigned int max, p;

    assert (ptable != NULL);
    assert (TXT_valid_file (file));

    /* write out the total of all counts */
    fwrite_int (file, ptable->ctotal, INT_SIZE);

    /* write out the current maximum symbol */
    max = ptable->max;
    fwrite_int (file, max, INT_SIZE);

    /* Now write out cumulative frequency counts */
    p = 0;
    for (p = 0; p < max + PTABLE_START_SYMBOL; p++)
      {
	fwrite_int (file, ptable->cfreq [p], INT_SIZE);
      }
}

ptable_type *
ptable_load_table (unsigned int file)
/* Reads in the table from the file. */
{
    ptable_type *ptable;
    unsigned int ctotal, max, p;

    assert (TXT_valid_file (file));

    /* read in the total of all counts */
    ctotal = fread_int (file, INT_SIZE);

    /* read in the current maximum symbol */
    max = fread_int (file, INT_SIZE);

    ptable = ptable_create_table (max);
    ptable->ctotal = ctotal;

    /* Now read in cumulative frequency counts */
    p = 0;
    for (p = 0; p < max + PTABLE_START_SYMBOL; p++)
      {
	ptable->cfreq [p] = fread_int (file, INT_SIZE);
      }
    return (ptable);
}
