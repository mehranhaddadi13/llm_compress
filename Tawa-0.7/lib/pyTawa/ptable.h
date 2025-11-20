/* Routines for storing symbols in a cumulative probability table. Based on
   Alistair Moffat's paper "An improved data structure for Cumulative
   Probability tables", Software: Practice and Experience, 1999. */

#ifndef PTABLE_H
#define PTABLE_H

typedef struct
{ /* Defines a data structure for storing a cumulative probability table. */
    unsigned int *cfreq;    /* Array of cumulative frequency counts, count [1] is used to store #singletons */
    unsigned int ctotal;    /* Total of all counts for the array excluding singletons */
    unsigned int alloc;     /* Current allocation for the array */
    unsigned int max;       /* Current maximum symbol in the array */
} ptable_type;

ptable_type *
ptable_create_table (unsigned int max);
/* Creates a new probability table of the specified type to contain max symbols initially. */

ptable_type *
ptable_copy_table (ptable_type *ptable);
/* Copies the probability table. */

unsigned int
ptable_get_lbnd (ptable_type *ptable, unsigned int s);
/* Returns the cumulative frequency of the symbols prior to s in the alphabet */

unsigned int
ptable_get_count (ptable_type *ptable, unsigned int s);
/* Returns the frequency count of symbol s. */

unsigned int
ptable_get_escape_symbol (ptable_type *ptable);
/* Returns the escape symbol number (i.e. max symbol + 1). */

unsigned int
ptable_get_escape_count (ptable_type *ptable);
/* Returns the escape count. */

unsigned int
ptable_get_total (ptable_type *ptable);
/* Returns the total count of all symbols. */

void
ptable_increment_count (ptable_type *ptable, unsigned int s, unsigned int increment);
/* Add one to the frequency of symbol s. */

unsigned int
ptable_get_symbol (ptable_type *ptable, unsigned int target, unsigned int *lbnd, unsigned int *count);
/* Returns the symbol number s such that:

        ptable_get_lbnd (s) <= target < ptable_get_lower_bounds (s) + ptable_get_count (s).

   Also returns the value lbnd that would be returned by ptable_get_lower_bound (s)
   and the value count. */

void
ptable_dump_table (unsigned int file, ptable_type *ptable);
/* Dumps out the table of cumulative frequencies. */

void
ptable_dump_symbols (unsigned int file, ptable_type *ptable);
/* Dumps out all the symbols in ptable. */

void
ptable_write_table (unsigned int file, ptable_type *ptable);
/* Writes out the table to the file which then can be re-loaded
   at a later date. */

ptable_type *
ptable_load_table (unsigned int file);
/* Reads in the table from the file. */

#endif
