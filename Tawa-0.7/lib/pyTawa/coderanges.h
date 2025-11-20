/* Routines for keeping track of a sequence of probabilities
   for a particular encoding. */

#include "model.h"

#ifndef CODERANGES_H
#define CODERANGES_H


unsigned int
TLM_create_coderanges (void);
/* Return a new pointer to a list of coderanges */

void
TLM_append_coderange (unsigned int coderanges, unsigned int lbnd,
		      unsigned int hbnd, unsigned int totl);
/* Append a new coderange record onto the tail of the coderange list */

void
TLM_overwrite_coderange (unsigned int coderanges, unsigned int lbnd,
			 unsigned int hbnd, unsigned int totl);
/* Overwrite the record at the head of the coderange list */

void
TLM_reset_coderanges (unsigned int coderanges);
/* Resets the position in the list of coderanges associated with the current symbol.
   The next call to TLM_next_coderange will return the first coderanges on the list. */

void
TLM_release_coderanges (unsigned int coderanges);
/* Release the coderange list to the used list */

boolean
TLM_next_coderange (unsigned int coderanges, unsigned int *lbnd,
		    unsigned int *hbnd, unsigned int *totl);
/* Places the current coderange in lbnd, hbnd and totl
   (see documentation for more complete description). Update the coderange record so
   that the current coderange becomes the next coderange in the list of
   coderange associated with the current symbol. If there are no more
   coderange in the list then return NIL otherwise some non-NIL value. */

unsigned int
TLM_length_coderanges (unsigned int coderanges);
/* Returns the code length of the coderanges list. */

float
TLM_codelength_coderanges (unsigned int coderanges);
/* Returns the code length of the current symbol's coderange in bits. It does this without
   altering the current symbol or the current coderange. */

void
TLM_dump_coderanges (unsigned int file, unsigned int coderanges);
/* Prints the coderange list for the current symbol in a human readable form.
   It does this without altering the current position in the coderange. list as determined
   by the functions TLM_reset_coderanges or TLM_next_coderange. */

unsigned int
TLM_copy_coderanges (unsigned int coderanges);
/* Creates a copy of the list of coderanges and returns a pointer to it. */

void
TLM_betail_coderanges (unsigned int coderanges);
/* Removes the tail of the coderange list. (This is useful for keeping the escape part of the list
   of coderanges intact). */

#endif
