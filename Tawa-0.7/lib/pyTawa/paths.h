/* Paths module definitions. */

#ifndef PATHS_H
#define PATHS_H

#include "confusion.h"

#define MODEL_SYMBOL 1    /* Indicates that the next symbol is a model
			     symbol */
#define GHOST_SYMBOL 2    /* Indicates that the next symbol is inserted
			     into the marked up text, but is not encoded
			     or the context updated */
#define SUSPEND_SYMBOL 3  /* Indicates that the next symbol is inserted
			     into the marked up text, the context is updated
			     but is not encoded */

void
dumpPathSymbols (unsigned int file, unsigned int text);
/* Dumps out the path symbols to the file. */

void
startPaths (unsigned int transform_model);
/* Frees the paths trie and leaves. */

void
startPath (unsigned int transform_model, unsigned int transform_type,
	   unsigned int language_model);
/* Starts a new path in the paths trie for the language model. */

void
releasePaths (unsigned int transform_model);
/* Frees the paths trie and leaves. */

unsigned int
transformPaths (unsigned int transform_model, unsigned int text,
	     void (*dump_symbol_function) (unsigned int, unsigned int),
	     void (*dump_symbols_function) (unsigned int, unsigned int),
	     void (*dump_confusion_function) (unsigned int, unsigned int));
/* Creates and returns an unsigned integer which provides a reference to a text record
   that contains the input text corrected according to the transform model. */

#endif
