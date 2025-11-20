/* TLM cache routines based on PPMf models. */

#include "io.h"

#ifndef PPM_CACHE_H
#define PPM_CACHE_H

struct PPM_cacheType
{ /* PPM cache record */
  int P_max_order;               /* The max. order of the cache */
  unsigned int *P_Root;          /* The root node of the cache's trie */
  unsigned int *P_leaf;          /* Ptr to last leaf that was updated */
  boolean P_valid;               /* Used to indicate if cache has been deleted or not */
  unsigned int P_next;           /* Next in deleted list of records list */
};

/* Global variables used for storing the PPM caches */
extern struct PPM_cacheType *PPM_Cache;/* List of PPM caches */

void
PPM_dump_cache_symbol (unsigned int file, unsigned int symbol,
		       void (*dump_symbol_function) (unsigned int, unsigned int));
/* Dump the symbol */

boolean
PPM_valid_context (unsigned int context);
/* Returns non-zero if the PPM cache is valid, zero otherwize. */

boolean
PPM_valid_cache (unsigned int cache);
/* Returns non-zero if the PPM cache is valid, zero otherwize. */

unsigned int
PPM_create_cache (int max_order);
/* Creates and returns a new pointer to a PPM cache record. */

void
PPM_release_cache (unsigned int cache);
/* Releases the memory allocated to the cache and the cache number (which may
   be reused in later PPM_create_cache or PPM_load_cache calls).
   A run-time error will be generated if an attempt is made to release
   a cache that still has active PPM_Contexts pointing at it. */

void
PPM_dump_cache (unsigned int file, unsigned int cache,
		 void (*dump_symbol_function) (unsigned int, unsigned int));
/* Dumps out the PPM cache (for debugging purposes). */

unsigned int
PPM_create_cache_context (unsigned int cache);
/* Creates and returns an unsigned integer which provides a reference to a PPM
   context record associated with the cache's context. The current position is
   set to the null string. The current symbol is set to the first predicted
   symbol. A run-time error occurs if the PPM context being copied is for a
   dynamic cache. */

unsigned int
PPM_copy_cache_context (unsigned int cache, unsigned int context);
/* Creates a new PPM context record, copies the contents of the specified
   context into it, and returns an integer reference to it. A run-time error
   occurs if the PPM context being copied is for a dynamic cache. */

void
PPM_release_cache_context (unsigned int cache, unsigned int context);
/* Releases the memory allocated to the PPM context and the context number
   (which may be reused in later PPM_create_context or PPM_copy_context and
   TLM_copy_dynamic_context calls). */

boolean
PPM_update_cache (unsigned int cache, unsigned int context, unsigned int symbol);
/* Updates the PPM cache. Returns TRUE if the context->symbol prediction is in the cache. */

unsigned int
PPM_get_cache (unsigned int cache);
/* Returns data about the last updated context in the cache. */

void
PPM_set_cache (unsigned int cache, unsigned int data);
/* Sets the data for the last updated context in the cache. */

boolean
PPM_find_cache (unsigned int cache, unsigned int cache_context, unsigned int symbol);
/* Finds (and updates) the cache if one exists (otherwise ignores it). Returns TRUE
   if a cached result can be used, and no further processing is required. */

void
PPM_insert_cache (unsigned int cache);
/* Inserts the relevant information into the cache so that it can be
   used latter without recalculating it. */


#endif
