/* TLM cache routines based on fast PPM models. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "io.h"
#include "sets.h"
#include "text.h"
#include "ppm_cache.h"
#include "ppm_model.h"

#define PPM_CACHES_SIZE 4          /* Initial max. number of caches */
#define PPM_CONTEXTS_SIZE 1024     /* Initial max. number of context records */

/* Offsets to data stored in the cache. */
/* Offsets to data in node record: */
#define PPM_CACHE_NODE_SIZE 3 /* symbol + sibling ptr + child ptr */
#define PPM_CACHE_NODE_SYMBOL 0
#define PPM_CACHE_NODE_NEXT 1
#define PPM_CACHE_NODE_CHILD 2

/* Offsets to data in leaf record: */
#define PPM_CACHE_LEAF_SIZE 3 /* symbol + sibling ptr + data */
#define PPM_CACHE_LEAF_SYMBOL 0
#define PPM_CACHE_LEAF_NEXT 1
#define PPM_CACHE_LEAF_DATA 2

/* Global variables used for storing the PPM caches */
struct PPM_cacheType *PPM_Caches = NULL; /* List of PPM caches */
unsigned int PPM_Caches_max_size = 0;      /* Current max. size of caches array */
unsigned int PPM_Caches_used = NIL;        /* List of deleted cache records */
unsigned int PPM_Caches_unused = 1;        /* Next unused cache record */

/* Global variables used for storing the PPM contexts */
unsigned int *PPM_Cache_Contexts = NULL;         /* List of ptrs to PPM contexts */
unsigned int *PPM_Cache_Contexts_deleted = NULL; /* Indicates record is on the used list */
unsigned int PPM_Cache_Contexts_max_size = 0;    /* Current max. size of contexts array */
unsigned int PPM_Cache_Contexts_used = NIL;      /* List of deleted context records */
unsigned int PPM_Cache_Contexts_unused = 1;      /* Next unused context record */

void
PPM_dump_cache_symbol (unsigned int file, unsigned int symbol,
		  void (*dump_symbol_function) (unsigned int, unsigned int))
/* Dump the symbol */
{
    assert (TXT_valid_file (file));

    if (dump_symbol_function)
        dump_symbol_function (file, symbol);
    else if (symbol == TXT_sentinel_symbol ())
	fprintf (Files [file], "<sentinel>");
    else if ((symbol <= 32) || (symbol >= 127))
	fprintf (Files [file], "<%d>", symbol);
    else
	fprintf (Files [file], "%c", symbol);
}

boolean
PPM_valid_cache_context (unsigned int context)
/* Returns non-zero if the PPM context is valid, zero otherwize. */
{
    if (context == NIL)
        return (FALSE);
    else if (context >= PPM_Cache_Contexts_unused)
        return (FALSE);
    else
        return (!PPM_Cache_Contexts_deleted [context]);
        /* This gets set to TRUE when the context gets deleted;
	   this way you can test to see if the context has been deleted or not */
}

boolean
PPM_valid_cache (unsigned int cache)
/* Returns non-zero if the PPM cache is valid, zero otherwize. */
{
    if (cache == NIL)
        return (FALSE);
    else if (cache >= PPM_Caches_unused)
        return (FALSE);
    else
        return (!PPM_Caches [cache].P_valid);
}

unsigned int
create_PPM_cache (void)
/* Creates and returns a new pointer to a PPM cache record. */
{
    unsigned int cache, old_size;

    if (PPM_Caches_used != NIL)
    { /* use the first record on the used list */
        cache = PPM_Caches_used;
	PPM_Caches_used = PPM_Caches [cache].P_next;
    }
    else
    {
	cache = PPM_Caches_unused;
        if (PPM_Caches_unused >= PPM_Caches_max_size)
	{ /* need to extend PPM_Caches array */
	    old_size = PPM_Caches_max_size * sizeof (struct PPM_cacheType);
	    if (PPM_Caches_max_size == 0)
	        PPM_Caches_max_size = PPM_CACHES_SIZE;
	    else
	        PPM_Caches_max_size = 10*(PPM_Caches_max_size+50)/9;

	    PPM_Caches = (struct PPM_cacheType *)
	        Realloc (124, PPM_Caches, PPM_Caches_max_size *
			 sizeof (struct PPM_cacheType), old_size);

	    if (PPM_Caches == NULL)
	    {
	        fprintf (stderr, "Fatal error: out of PPM caches space\n");
		exit (1);
	    }
	}
	PPM_Caches_unused++;
    }

    PPM_Caches [cache].P_max_order = -1;
    PPM_Caches [cache].P_leaf = NIL;
    PPM_Caches [cache].P_valid = FALSE;
    PPM_Caches [cache].P_next = NIL;

    return (cache);
}

unsigned int
PPM_create_cache (int max_order)
/* Creates and returns a new pointer to a PPM cache record. */
{
    unsigned int cache;

    cache = create_PPM_cache ();

    assert (max_order >= 0); /* Order -1 not yet implemented */ 
    PPM_Caches [cache].P_Root = NULL;
    PPM_Caches [cache].P_max_order = max_order;

    return (cache);
}

unsigned int *
PPM_create_node (unsigned int cache, unsigned int symbol)
{
    unsigned int *ptr, bytes;

    /* Create the node */
    bytes = PPM_CACHE_NODE_SIZE * sizeof (unsigned int);
    ptr = (unsigned int *) Malloc (121, bytes);
    assert (ptr != NULL);

    /*fprintf (stderr, "Created node ptr %p\n", ptr);*/ 
    memset (ptr, 0, PPM_CACHE_NODE_SIZE * sizeof (unsigned int));
    *(ptr + PPM_CACHE_NODE_SYMBOL) = symbol;
    return (ptr);
}

unsigned int *
PPM_create_leaf (unsigned int cache, unsigned int symbol, unsigned int data)
{
    unsigned int *ptr, bytes;

    /* Create the leaf */
    bytes = PPM_CACHE_LEAF_SIZE * sizeof (unsigned int);
    ptr = (unsigned int *) Malloc (122, bytes);
    assert (ptr != NULL);

    /*fprintf (stderr, "Created leaf ptr %p\n", ptr);*/
    memset (ptr, 0, PPM_CACHE_LEAF_SIZE * sizeof (unsigned int));
    *(ptr + PPM_CACHE_LEAF_SYMBOL) = symbol;
    *(ptr + PPM_CACHE_LEAF_DATA) = data;
    return (ptr);
}

void
PPM_release_cache (unsigned int cache)
/* Releases the memory allocated to the cache and the cache number (which may
   be reused in later PPM_create_cache or PPM_load_cache calls).
   A run-time error will be generated if an attempt is made to release
   a cache that still has active PPM_Cache_Contexts pointing at it. */
{
    if (cache == NIL)
        return;

    assert (PPM_valid_cache (cache));

    /* add cache record at the head of the PPM_Caches_used list */
    PPM_Caches [cache].P_valid = FALSE;
    PPM_Caches [cache].P_next = PPM_Caches_used;
    PPM_Caches_used = cache;

    PPM_Caches [cache].P_max_order = -2; /* Used for testing if cache no.
					      is valid or not */
}

unsigned int *PPM_dump_cache_symbols = NULL;

void PPM_dump_cache_trie (unsigned int file, unsigned int cache,
			  unsigned int *ptr, unsigned int pos,
			  void (*dump_symbol_function) (unsigned int, unsigned int))
/* Recursive routine used by PPM_dump_cache. */
{
    unsigned int p, symbol, data, *snext, *child;

    while (ptr != NULL)
      { /* proceed through the symbol list */
	symbol = *(ptr + PPM_CACHE_NODE_SYMBOL);
	snext = (unsigned int *) *(ptr + PPM_CACHE_NODE_NEXT);
	if (pos < PPM_Caches [cache].P_max_order)
	  {
	    child = (unsigned int *) *(ptr + PPM_CACHE_NODE_CHILD);
	    data = NIL;
	  }
	else
	  {
	    child = NULL;
	    data = *(ptr + PPM_CACHE_LEAF_DATA);
	  }

	fprintf (Files [file], "%d [", pos);
	for (p = 0; p < pos; p++)
	  {
	    PPM_dump_cache_symbol (file, PPM_dump_cache_symbols [p], dump_symbol_function);
	    fprintf (Files [file], "," );
	  }
	PPM_dump_cache_symbol (file, symbol, dump_symbol_function);
	if (!data)
	    fprintf (Files [file], "]\n");
	else
	    fprintf (Files [file], "] data = %d\n", data);

	if (child != NULL)
	  {
	    PPM_dump_cache_symbols [pos] = symbol;
	    PPM_dump_cache_trie (file, cache, child, pos+1, dump_symbol_function);
	  }
	ptr = snext;
      }
}

void
PPM_dump_cache (unsigned int file, unsigned int cache,
		 void (*dump_symbol_function) (unsigned int, unsigned int))
/* Dumps out the PPM cache (for debugging purposes). */
{
    assert (PPM_valid_cache (cache));

    fprintf (Files [file], "Max order of cache = %d\n",
	     PPM_Caches [cache].P_max_order);

    fprintf (Files [file], "Dump of trie:\n");

    PPM_dump_cache_symbols = (unsigned int *)
      realloc (PPM_dump_cache_symbols, (PPM_Caches [cache].P_max_order+1) *
	       sizeof (unsigned int));

    PPM_dump_cache_trie (file, cache, PPM_Caches [cache].P_Root, 0, dump_symbol_function);
}

/* Context routines */
void
PPM_update_cache_context1 (unsigned int cache, unsigned int context, unsigned int symbol)
/* Updates the context for the cache using the new symbol. */
{
    register unsigned int *Context, pos;
    register int max_order;

    max_order = PPM_Caches [cache].P_max_order;

    Context = (unsigned int *) PPM_Cache_Contexts [context];
    for (pos = 0; pos < max_order; pos++)
	Context [pos] = Context [pos+1];
    Context [pos] = symbol;
}

unsigned int
PPM_create_cache_context1 (void)
/* Return a new pointer to a context record. */
{
    unsigned int context, old_size;

    if (PPM_Cache_Contexts_used != NIL)
    {	/* use the first record on the used list */
	context = PPM_Cache_Contexts_used;
	PPM_Cache_Contexts_used = PPM_Cache_Contexts [context];
    }
    else
    {
	context = PPM_Cache_Contexts_unused;
	if (PPM_Cache_Contexts_unused >= PPM_Cache_Contexts_max_size)
	{ /* need to extend PPM_Cache_Contexts array */
	    old_size = PPM_Cache_Contexts_max_size * sizeof (unsigned int);
	    if (PPM_Cache_Contexts_max_size == 0)
		PPM_Cache_Contexts_max_size = PPM_CONTEXTS_SIZE;
	    else
		PPM_Cache_Contexts_max_size = 10*(PPM_Cache_Contexts_max_size+50)/9; 

	    PPM_Cache_Contexts = (unsigned int *)
	        Realloc (123, PPM_Cache_Contexts, PPM_Cache_Contexts_max_size *
			 sizeof (unsigned int), old_size);
	    PPM_Cache_Contexts_deleted = (unsigned int *)
	        Realloc (123, PPM_Cache_Contexts_deleted, PPM_Cache_Contexts_max_size *
			 sizeof (boolean), old_size);

	    if (PPM_Cache_Contexts == NULL)
	    {
		fprintf (stderr, "Fatal error: out of PPM_Cache_Contexts space\n");
		exit (1);
	    }
	}
	PPM_Cache_Contexts_unused++;
    }
    PPM_Cache_Contexts [context] = NIL;
    PPM_Cache_Contexts_deleted [context] = FALSE;

    return (context);
}

unsigned int
PPM_create_cache_context (unsigned int cache)
/* Creates and returns an unsigned integer which provides a reference to a PPM
   context record associated with the cache's context. The current position is
   set to the null string. The current symbol is set to the first predicted
   symbol. A run-time error occurs if the PPM context being copied is for a
   dynamic cache. 
*/
{
    unsigned int bytes;
    unsigned int context, p;
    int max_order;

    assert (PPM_valid_cache (cache));

    max_order = PPM_Caches [cache].P_max_order;

    context = PPM_create_cache_context1 ();

    assert (context != NIL);

    /* Initialize the new context */
    bytes = (max_order+1) * sizeof (unsigned int);
    PPM_Cache_Contexts [context] = (unsigned int) Malloc (123, bytes);

    for (p = 0; p <= max_order; p++)
        PPM_update_cache_context1 (cache, context, TXT_sentinel_symbol ());

    return (context);
}

void
PPM_release_cache_context (unsigned int cache, unsigned int context)
/* Releases the memory allocated to the PPM context and the context number
   (which may be reused in later PPM_create_cache_context or
   PPM_copy_cache_context). */
{
    int max_order;

    assert (PPM_valid_cache (cache));
    max_order = PPM_Caches [cache].P_max_order;

    assert (PPM_valid_cache_context (context));

    Free (89, (unsigned int *) PPM_Cache_Contexts [context],
	  (max_order+1) * sizeof (unsigned int));
    PPM_Cache_Contexts_deleted [context] = TRUE;

    /* Append onto head of the used list; negative means this
       record has been placed on the used list */
    PPM_Cache_Contexts [context] = PPM_Cache_Contexts_used;
    PPM_Cache_Contexts_used = context;
}

boolean
PPM_update_cache_trie
(unsigned int cache, unsigned int context)
/* Updates the cache for the new context. */
{
    register unsigned int *ptr, *parent, *child, *snext, *sprev;
    register unsigned int sym, symbol, pos;
    register unsigned int *Context;
    register int max_order;
    register boolean found;

    found = TRUE;
    parent = NULL;
    ptr = PPM_Caches [cache].P_Root;
    max_order = PPM_Caches [cache].P_max_order;
    Context = (unsigned int *) PPM_Cache_Contexts [context];
    for (pos = 0; pos <= max_order; pos++)
      { /* create or access non leaf nodes */
	symbol = Context [pos];
	child = NULL;
	sprev = NULL;
	while (ptr != NULL)
	  { /* find out where the symbol is */
	    sym = *(ptr + PPM_CACHE_NODE_SYMBOL);
	    snext = (unsigned int *) *(ptr + PPM_CACHE_NODE_NEXT);
	    if (pos >= max_order)
	        child = NULL;
	    else
	        child = (unsigned int *) *(ptr + PPM_CACHE_NODE_CHILD);
	    if (sym == symbol)
	        break;
	    sprev = ptr;
	    ptr = snext;
	  }

	if (ptr == NULL)
	  { /* haven't found the symbol - create it */
	    if (pos != max_order)
	        ptr = PPM_create_node (cache, symbol);
	    else
	      {
		found = FALSE;
	        ptr = PPM_create_leaf (cache, symbol, NIL);
	      }

	    if (sprev != NULL)
		*(sprev + PPM_CACHE_NODE_NEXT) = (unsigned int) ptr;
	    else if (parent == NULL)
	        PPM_Caches [cache].P_Root = ptr;
	    else
	        *(parent + PPM_CACHE_NODE_CHILD) = (unsigned int) ptr;
	    child = NULL;
	  }

	parent = ptr;
	ptr = child; /* move down the trie */
      }

    PPM_Caches [cache].P_leaf = parent;
    return (found);
}

boolean
PPM_update_cache (unsigned int cache, unsigned int context, unsigned int symbol)
/* Updates the PPM cache. Returns TRUE if the context->symbol prediction is in the cache. */
{
    unsigned int *Context;
 
    assert (PPM_valid_cache (cache));

    assert (PPM_valid_cache_context (context));
    Context = (unsigned int *) PPM_Cache_Contexts [context];

    /* first update the context */
    PPM_update_cache_context1 (cache, context, symbol);

    /* next update the trie */
    return (PPM_update_cache_trie (cache, context));
}

unsigned int
PPM_get_cache (unsigned int cache)
/* Returns data about the last updated context in the cache. */
{
    register unsigned int *ptr;

    assert (PPM_valid_cache (cache));

    ptr = PPM_Caches [cache].P_leaf;
    assert (ptr != NIL);

    return (*(ptr + PPM_CACHE_LEAF_DATA));
}

void
PPM_set_cache (unsigned int cache, unsigned int data)
/* Sets the data for the last updated context in the cache. */
{
    register unsigned int *ptr;

    assert (PPM_valid_cache (cache));

    ptr = PPM_Caches [cache].P_leaf;
    assert (ptr != NIL);

    *(ptr + PPM_CACHE_LEAF_DATA) = data;
}

boolean
PPM_find_cache (unsigned int cache, unsigned int cache_context, unsigned int symbol)
/* Finds (and updates) the cache if one exists (otherwise ignores it). Returns TRUE
   if a cached result can be used, and no further processing is required. */
{
    boolean found;

    /*
    ppm_model = Models [model].M_model;
    if (!PPM_Models [ppm_model].P_performs_caching)
        return (FALSE);

    cache = PPM_Models [ppm_model].P_cache;
    */

    if (cache == NIL)
      return (FALSE);

    found = PPM_update_cache (cache, cache_context, symbol);
    if (found)
        TLM_Codelength = (float) PPM_get_cache (cache);
    return (found);
}

void
PPM_insert_cache (unsigned int cache)
/* Inserts the relevant information into the cache so that it can be
   used latter without recalculating it. */
{
    /*
    unsigned int ppm_model;

    ppm_model = Models [model].M_model;
    if (!PPM_Models [ppm_model].P_performs_caching)
        return;

    cache = PPM_Models [ppm_model].P_cache;
    */

    PPM_set_cache (cache, (unsigned int) TLM_Codelength);
}
