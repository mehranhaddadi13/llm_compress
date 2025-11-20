/* Routines for keeping a variable length array index. */

#ifndef INDEX_H
#define INDEX_H

unsigned int
TXT_create_index ();
/* Creates and initializes an index (for storing a variable-length array of
   unsigned ints. */

void
TXT_release_index (unsigned int index);
/* Releases the memory allocated to the index and the index number (which may
   be reused in later TXT_create_index calls). */

unsigned int
TXT_max_index (unsigned int index);
/* Returns the maximum index used the array index. */

boolean
TXT_get_index (unsigned int index, unsigned int pos, unsigned int *value);
/* Returns the value associated with the position pos in the array index.
   Returns FALSE if the value does not exist i.e. pos exceeds the current
   array bounds. */

void
TXT_put_index (unsigned int index, unsigned int pos, unsigned int value);
/* Inserts the value at position pos into the array index. Extends the
   array bounds so that the value can be inserted. */

void
TXT_dump_index
(unsigned int file, unsigned int index,
 void (*dump_value_function) (unsigned int, unsigned int, unsigned int));
/* Dumps out a human readable version of the text record to the file.
   The argument dump_symbol_function is a pointer to a function for printing
   values stored in the array index. If this is NULL, then each non-zero
   symbol will be printed as an unsigned int along with its index. */

#endif
