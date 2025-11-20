/* Trie routines for PPM models. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "io.h"
#include "text.h"
#include "model.h"
#include "ppm_trie.h"

#define DETERM_FACTOR 3	           /* Scaling factor for deterministic contexts */

#define TRIE_INPUT_SIZE 1024	   /* Initial max. size of input text associated with the context trie */
#define TRIE_NODES_SIZE 1024       /* Initial max. size of the trie nodes array */
#define TRIE_SLIST_SIZE 128        /* Initial max. size of the slist nodes array */

#define TRIE_TCOUNT_INIT 1         /* initial tcount (total count) when new trie node is created */
#define TRIE_TCOUNT_INCR_A 1	   /* tcount increment for node, escape method A */
#define TRIE_TCOUNT_INCR_C 1	   /* tcount increment for node, escape method C */
#define TRIE_TCOUNT_INCR_D 2	   /* tcount increment for node, escape method D */

unsigned int PPM_Trie_malloc_slist = 0;/* How much slist records have been allocated */

void
PPM_dump_symbol (unsigned int file, unsigned int symbol,
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

void
PPM_dump_string (unsigned int file, unsigned int *str,
		 unsigned int pos, unsigned int len,
		 void (*dump_symbol_function) (unsigned int, unsigned int))
/* Dump the string STR starting at position POS. */
{
    unsigned int symbol, p;

    assert (TXT_valid_file (file));

    for (p = pos; p < pos + len; p++)
    {
	symbol = str[p];
	if (dump_symbol_function)
	    dump_symbol_function (file, symbol);
	else if (symbol == TXT_sentinel_symbol ())
	    fprintf (Files [file], "<sentinel>");
	else if ((symbol <= 31) || (symbol >= 127))
	    fprintf (Files [file], "<%d>", symbol);
	else
	    fprintf (Files [file], "%c", symbol);
    }
}

void
PPM_dump_input (unsigned int file, struct PPM_trieType *trie,
		void (*dump_symbol_function) (unsigned int, unsigned int))
/* Dump the input array for trie */
{
    unsigned int p;

    assert (TXT_valid_file (file));

    fprintf (Files [file], "Dump of input: (length = %d storage size = %d)\n%6d: ", trie->T_input_len, trie->T_input_size, 1);
    for (p=1; p<=trie->T_input_len; p++)
      {
	if ((p % 50) == 0)
	  fprintf (Files [file], "\n%6d: ", p);
	PPM_dump_symbol (file, trie->T_input [p], dump_symbol_function);
      }
    if ((p % 50) != 0)
      fprintf (Files [file], "\n");
}

void
PPM_init_input (struct PPM_trieType *trie)
/* Initialize the input array for trie. */
{
    trie->T_input = (unsigned int *) Malloc (1, TRIE_INPUT_SIZE * sizeof (unsigned int));
    trie->T_input_size = TRIE_INPUT_SIZE;
}

unsigned int
PPM_update_input (struct PPM_trieType *trie, unsigned int symbol)
/* Insert the symbol into the input at the next input position. */
{
    unsigned int pos, old_memsize;

    pos = ++(trie->T_input_len);
    if (pos >= trie->T_input_size)
    { /* extend array */
        old_memsize = trie->T_input_size * sizeof (unsigned int);
	trie->T_input_size += TRIE_INPUT_SIZE;
	trie->T_input = (unsigned int *) Realloc (1, trie->T_input,
			 trie->T_input_size * sizeof (unsigned int), old_memsize);
	if (trie->T_input == NULL)
	{
	    fprintf (stderr, "Fatal error: out of input space\n");
	    exit (1);
	}
    }
    trie->T_input [pos] = symbol;
    return (pos);
}

unsigned int
PPM_allocate_trie_node (struct PPM_trieType *trie, unsigned int size)
/* Allocates a node from the nodes array */
{
    unsigned int node, old_memsize;

    assert (trie != NULL);

    if (trie->T_unused + size + 1 >= trie->T_size)
      { /* need to extend nodes array */
	old_memsize = trie->T_size * sizeof (unsigned int);
	if (trie->T_size == 0)
	    trie->T_size = TRIE_NODES_SIZE;
	else
	    trie->T_size = 10*(trie->T_size+50)/9;
	while (trie->T_size < trie->T_unused + size + 1)
	    trie->T_size = 10*(trie->T_size+50)/9;

	trie->T_nodes = (int *) Realloc (2, trie->T_nodes, trie->T_size * sizeof (unsigned int),
					 old_memsize);

	if (trie->T_nodes == NULL)
	  {
	    fprintf (stderr, "Fatal error: out of nodes space\n");
	    exit (1);
	  }
      }

    node = trie->T_unused;
    trie->T_unused += size;
    return (node);
}

unsigned int
PPM_allocate_slist (struct PPM_trieType *trie, unsigned int symbol, int child)
/* Allocates an slist node from the slist nodes array */
{
    unsigned int snew;

    assert (trie != NULL);
    snew = PPM_allocate_trie_node (trie, TRIE_SLIST_DWIDTH);
    trie->T_nodes [snew+TRIE_SYMBOL_OFFSET] = symbol;
    trie->T_nodes [snew+TRIE_CHILD_OFFSET] = child;
    trie->T_nodes [snew+TRIE_NEXT_SLIST_OFFSET] = NIL;

    PPM_Trie_malloc_slist += TRIE_SLIST_DWIDTH * sizeof (int);

    return (snew);
}

void
PPM_get_slist (struct PPM_trieType *trie, unsigned int sptr,
	       unsigned int *sym, int *child, unsigned int *next_sptr)
/* Returns the symbol, child and next symbol pointer associated with the sptr
   into the node's slist. */
{
    int sym1;

    *sym = 0;
    *child = NIL;
    if ((trie == NULL) || (sptr == NIL))
	return;
    else if (trie->T_nodes == NULL)
        return;
    else
      {
	*child = trie->T_nodes [sptr+TRIE_CHILD_OFFSET];
	sym1 = trie->T_nodes [sptr+TRIE_SYMBOL_OFFSET];
	if (sym1 == SPECIAL_SYMBOL)
	    *sym = 0;
	else if (sym1 >= 0)
	    *sym = sym1;
	else /* A negative symbol means we have reached the last symbol in
		the list and there aren't any more */
	    *sym = -sym1;
	    
	if (trie->T_form == TLM_Dynamic)
	    *next_sptr = trie->T_nodes [sptr+TRIE_NEXT_SLIST_OFFSET];
	else if ((sym1 < 0) || (sym1 == SPECIAL_SYMBOL))
	    *next_sptr = NIL; /* at the end of the symbol list for static model */
	else
	    *next_sptr = sptr + TRIE_SLIST_SWIDTH; /* more symbols to process */
      }
}

void
PPM_put_slist (struct PPM_trieType *trie, unsigned int sptr, int child)
/* Inserts the child pointer associated with the sptr into the node's slist. */
{
    if ((trie == NULL) || (sptr == NIL))
	return;
    else if (trie->T_nodes == NULL)
        return;
    else
      {
	trie->T_nodes [sptr+TRIE_CHILD_OFFSET] = child;
      }
}

unsigned int
PPM_find_slist (struct PPM_trieType *trie, unsigned int shead, unsigned int sym,
		int *child, unsigned int *prev_sptr)
/* Returns the ptr to slist node containing sym if one exists; NIL otherwise. Also returns the slist node's child
   and a ptr to the previous slist node in the list. The trie is assumed to be dynamic. */
{
    unsigned int sptr, sym1, next_sptr, prev_sptr1;
    int child1;

    *child = NIL;
    *prev_sptr = NIL;
    if ((trie == NULL) || (shead == NIL))
	return (NIL);
    else if (trie->T_nodes == NULL)
        return (NIL);
    else
      {
	child1 = NIL;
	prev_sptr1 = NIL;

	sptr = shead;
	while (sptr != NIL) /* process all the symbols */
	  { /* move along to next symbol in the list */
	    PPM_get_slist (trie, sptr, &sym1, &child1, &next_sptr);
	    if (sym == sym1)
	        break; /* found the symbol */
	    prev_sptr1 = sptr;
	    sptr = next_sptr; /* advance to next symbol in slist */
	  }

	if (sptr == NIL) /* symbol not found */
	    *child = NIL;
	else
	    *child = child1;

	*prev_sptr = prev_sptr1;
	return (sptr);
      }
}

void
PPM_add_slist (struct PPM_trieType *trie, unsigned int node, unsigned int stail,
	       unsigned int symbol, int child)
/* Add a new symbol to the node's slist after the tail of the list. */
{
    unsigned int snew, shead;

    assert (trie != NULL);
    assert (trie->T_form == TLM_Dynamic);
    assert (trie->T_nodes != NULL);

    snew = PPM_allocate_slist (trie, symbol, child);
    assert (snew != NIL);

    shead = trie->T_nodes [node+TRIE_SLIST_OFFSET]; /* the start of list of symbols is a ptr for a dynamic model */

    if (stail != NIL)
      { /* add after the tail */
	assert (shead != NIL); /* we can't have the head of the slist being NIL if sptr is not NIL */
	assert (trie->T_nodes [stail+TRIE_NEXT_SLIST_OFFSET] == NIL);

	trie->T_nodes [stail+TRIE_NEXT_SLIST_OFFSET] = snew;
      }
    else
      {
	assert (shead == NIL); /* if the tail is NIL, then the head must be also */

	trie->T_nodes [node+TRIE_SLIST_OFFSET] = snew; /* create a new head of the list */
      }
}

unsigned int
PPM_count_slist (struct PPM_trieType *trie, unsigned int node, unsigned int shead)
/* Returns the number of symbols in the symbol list starting from shead. */
{
    unsigned int scount, sym, sptr, next_sptr;
    int child;

    if ((trie == NULL) || (shead == NIL))
	return (0);
    else if (trie->T_nodes == NULL)
        return (0);
    else
      {
	scount = 0;
	sptr = shead;
	while (sptr) /* process all the symbols */
	  {
	    scount++;
	    PPM_get_slist (trie, sptr, &sym, &child, &next_sptr);
	    sptr = next_sptr; /* advance to next symbol in slist */
	  }

	return (scount);
      }
}

void
PPM_get_trie_node (struct PPM_trieType *trie, unsigned int node, unsigned int *tcount,
		   unsigned int *shead)
/* Returns the trie count tcount and shead (head of the symbol list) associated with the
   node. */
{
    *shead = NIL;
    if ((trie == NULL) || (trie->T_nodes == NULL) || (node == NIL))
	*tcount = 0;
    else
      {
	*tcount = trie->T_nodes [node+TRIE_TCOUNT_OFFSET];

	if (trie->T_form == TLM_Dynamic)
	    *shead = trie->T_nodes [node+TRIE_SLIST_OFFSET]; /* the start of list of symbols is a ptr for a dynamic model */
	else if (*tcount != 0)
	    *shead = node+TRIE_SLIST_OFFSET; /* return the start of the list of symbols for a static model */
      }
}

unsigned int
PPM_get_trie_count (struct PPM_trieType *trie, int node, int child,
		    unsigned int sptr, unsigned int next_sptr,
		    unsigned int sym)
/* Gets the count for the child node at the node in the trie. */
{
    unsigned int tcount, shead, deterministic;

    if ((trie == NULL) || (node == NIL)) /* encoding using order -1 probabilities */
	return (1); /* set tcount for all symbols in order -1 to 1 */

    if (child <= NIL) /* points to input stream */
	tcount = 1;
    else
    {
        PPM_get_trie_node (trie, child, &tcount, &shead);

	/* check for deterministic lists */
	deterministic = ((next_sptr == NIL) && (sptr == shead));
	if (deterministic && (tcount > 1))
	    tcount *= DETERM_FACTOR; /* scale count up by deterministic factor */
    }
    return (tcount);
}

void
PPM_increment_trie_node (struct PPM_trieType *trie, unsigned int node,
			 unsigned int escape_method)
/* Increments the total count for the node in the trie (for dynamic tries only). */
{
    unsigned int increment;

    assert ((trie != NULL) && (trie->T_nodes != NULL) && (node != NIL));

    switch (escape_method)
      {
      case TLM_PPM_Method_A:
	increment = TRIE_TCOUNT_INCR_A;
	break;
      case TLM_PPM_Method_C:
	increment = TRIE_TCOUNT_INCR_C;
	break;
      case TLM_PPM_Method_D:
	increment = TRIE_TCOUNT_INCR_D;
	break;
      default:
	increment = TRIE_TCOUNT_INCR_D;
	break;
      }
    trie->T_nodes [node+TRIE_TCOUNT_OFFSET] += increment;
}

int
PPM_find_trie_node (struct PPM_trieType *trie, unsigned int node,
		    unsigned int symbol)
/* Find and return the symbol list position for symbol in the trie node. */
{
    unsigned int sym, tcount, sptr, next_sptr;
    int child;

    assert (trie != NULL);
    assert (node != NIL);

    /* Get start of list of symbols: */
    PPM_get_trie_node (trie, node, &tcount, &sptr);

    while (sptr) /* process all the symbols */
      {
	PPM_get_slist (trie, sptr, &sym, &child, &next_sptr);
	if ((sym == symbol) && (child > NIL))
	    return (sptr);

	sptr = next_sptr; /* advance to next symbol in slist */
      }

    return (NIL);
}

unsigned int
PPM_create_trie_node (struct PPM_trieType *trie)
/* Allocate a new node of type node_type and return a pointer to it. */
{
    unsigned int node;

    assert (trie != NULL);

    assert (trie->T_form == TLM_Dynamic); /* Trie must be type dynamic if we wish to create a new node */
    node = PPM_allocate_trie_node (trie, TRIE_NODE_DWIDTH);
    trie->T_nodes [node+TRIE_TCOUNT_OFFSET] = TRIE_TCOUNT_INIT;
    trie->T_nodes [node+TRIE_SLIST_OFFSET] = NIL;
    return (node);
}

void
PPM_update_trie_node (struct PPM_trieType *trie, unsigned int node,
		      unsigned int symbol)
/* Update the trie node. */
{
    unsigned int tcount, sptr;

    assert (trie != NULL);
    assert (node != NIL);

    assert (trie->T_form == TLM_Dynamic); /* Must be type dynamic if we are to update it! */

    if (trie->T_nodes != NULL)
      {
	PPM_get_trie_node (trie, node, &tcount, &sptr);
	trie->T_nodes [node+TRIE_TCOUNT_OFFSET] = tcount + 1;
      }
}

struct PPM_trieType *
PPM_create_trie (unsigned int form)
/* Creates a trie of type "type" (i.e. either TLM_Static or TLM_Dynamic). */ 
{
    struct PPM_trieType *trie;

    trie = (struct PPM_trieType *) Malloc (4, sizeof(struct PPM_trieType));
    trie->T_form = form;
    trie->T_nodes = NULL;
    trie->T_size = 0;
    trie->T_unused = 1;

    trie->T_input = NULL;
    trie->T_input_size = 0;
    trie->T_input_len = 0;

    if (form == TLM_Dynamic)
      {
	/* create empty node at the root */
	trie->T_unused = TRIE_ROOT_NODE;
	PPM_allocate_trie_node (trie, TRIE_NODE_DWIDTH);
	trie->T_nodes [TRIE_ROOT_NODE+TRIE_TCOUNT_OFFSET] = 0;
	trie->T_nodes [TRIE_ROOT_NODE+TRIE_SLIST_OFFSET] = NIL;
      }

    return (trie);
}

void
PPM_release_trie (struct PPM_trieType *trie)
/* Releases the memory used by the trie. */
{
    if (trie != NULL)
      {
	if (trie->T_nodes != NULL)
	    Free (2, trie->T_nodes,
		  trie->T_size * sizeof (unsigned int));
	if (trie->T_input != NULL)
	    Free (1, trie->T_input,
		  trie->T_input_size * sizeof (unsigned int));
	Free (4, trie, sizeof(struct PPM_trieType));
      }
}

struct PPM_trieType *
PPM_copy_trie (struct PPM_trieType *trie)
/* Copies the trie. */
{
    struct PPM_trieType *new_trie;
    unsigned int i;

    assert (trie != NULL);
    new_trie = (struct PPM_trieType *) Malloc (4, sizeof(struct PPM_trieType));
    new_trie->T_form = trie->T_form;
    new_trie->T_nodes = (int *) Malloc (2, trie->T_size * sizeof (unsigned int));
    new_trie->T_size = trie->T_size;
    new_trie->T_unused = trie->T_unused;
    for (i = 0; i < trie->T_size; i++) /* Copy nodes array */
        new_trie->T_nodes [i] = trie->T_nodes [i];

    new_trie->T_input = (unsigned int *) Malloc (1, trie->T_input_size * sizeof (unsigned int));
    new_trie->T_input_size = trie->T_input_size;
    new_trie->T_input_len = trie->T_input_len;
    for (i = 0; i < trie->T_input_size; i++) /* Copy input array */
        new_trie->T_input [i] = trie->T_input [i];

    return (new_trie);
}

unsigned int *dumpTrieStr = NULL;
int dumpTrieStrLen = 0;

void
PPM_dump_trie1 (unsigned int file, struct PPM_trieType *trie, int node, unsigned int tp, int d, int max_depth,
	    void (*dump_symbol_function) (unsigned int, unsigned int))
/* Dump the trie; d is 0 when at the top level. */
{
    unsigned int sym, sptr, next_sptr, tcount;
    int child;

    assert (TXT_valid_file (file));
    fprintf (Files [file], "%5d %8d  %8d  ", d, node, tp);

    if (node == NIL)
        fprintf (Files [file], "*** ERROR - Node is NIL ***\n");
    assert (node != NIL);

    if (node < 0)  /* pointer to input */
	fprintf (Files [file], "         <");
    else
    {
      /* Get start of list of symbols: */
	PPM_get_trie_node (trie, node, &tcount, &sptr);
	fprintf (Files [file], " %5d   <", tcount);
    }
    PPM_dump_string (file, dumpTrieStr, 0, d, dump_symbol_function);
    fprintf (Files [file], ">\n");

    if ((node > 0) && (d <= max_depth))
      while (sptr)
	{
	  PPM_get_slist (trie, sptr, &sym, &child, &next_sptr);
	  if (!dumpTrieStrLen || (d >= dumpTrieStrLen))
	    {
	      if (!dumpTrieStrLen)
	        dumpTrieStrLen = 64;
	      while (d >= dumpTrieStrLen)
	        dumpTrieStrLen *= 2;
	      dumpTrieStr = (unsigned int *) realloc (dumpTrieStr, dumpTrieStrLen * sizeof (unsigned int));
	    }
	  dumpTrieStr [d] = sym;

	  PPM_dump_trie1 (file, trie, child, sptr, d+1, max_depth,
			  dump_symbol_function);
	  sptr = next_sptr; /* advance to next symbol in slist */
	}
}

void
PPM_dump_trie (unsigned int file, struct PPM_trieType *trie, int max_depth,
	       void (*dump_symbol_function) (unsigned int, unsigned int))
/* Dumps the trie. */
{
    assert (TXT_valid_file (file));

    fprintf (Files [file], "Dump of Trie : \n");
    fprintf (Files [file], "---------------\n");
    fprintf (Files [file], "depth     node     child   count  node\n");
    if (trie != NULL)
        PPM_dump_trie1 (file, trie, TRIE_ROOT_NODE, 0, 0, max_depth,
			dump_symbol_function);
    fprintf (Files [file], "---------------\n");
    fprintf (Files [file], "\n");
}

unsigned int PPM_Stats_nodes;
unsigned int PPM_Stats_scounts;

void
PPM_stats_trie1 (unsigned int file, struct PPM_trieType *trie, int node, int d,
		 int max_depth)
/* Produce stats for the trie; d is 0 when at the top level. */
{
    unsigned int sptr, next_sptr, sym, scount, tcount;
    int child;

    assert (TXT_valid_file (file));
    assert (node != NIL);

    PPM_Stats_nodes++;
    if ((node > 0) && (d <= max_depth))
    {
        /* Get start of list of symbols: */
	PPM_get_trie_node (trie, node, &tcount, &sptr);

	scount = 0;
	while (sptr)
	{
	    scount++;
	    PPM_get_slist (trie, sptr, &sym, &child, &next_sptr);

	    PPM_stats_trie1 (file, trie, child, d+1, max_depth);
	    sptr = next_sptr; /* advance to next symbol in slist */
	}
	PPM_Stats_scounts += scount;
    }
}

void
PPM_stats_trie (unsigned int file, struct PPM_trieType *trie, int max_depth)
/* Produces stats for the trie */
{
    PPM_Stats_nodes = 0;
    PPM_Stats_scounts = 0;

    assert (TXT_valid_file (file));

    if (trie != NULL)
        PPM_stats_trie1 (file, trie, TRIE_ROOT_NODE, 0, max_depth);

    fprintf (Files [file], "Number of nodes   = %d\n", PPM_Stats_nodes );
    fprintf (Files [file], "Number of scounts = %d\n", PPM_Stats_scounts );
}

void
PPM_build_static_trie1 (struct PPM_trieType *trie, struct PPM_trieType *strie, int node, int pnode, int d, int max_depth)
/* Builds the static strie from the dynamic trie in model. */
{
    unsigned int p, alloc, sptr, scount, tcount, next_sptr, sym;
    int child;

    assert (node != NIL);

    if (node < 0) /* pointer to input */
        strie->T_nodes [pnode] = node;
    else /* pointer to node in the trie */
    {
      /* Get start of list of symbols: */
      PPM_get_trie_node (trie, node, &tcount, &sptr);

      scount = PPM_count_slist (trie, node, sptr);

      if (d > max_depth)
	  alloc = TRIE_NODE_SWIDTH; /* allocate for single count only */
      else
	  alloc = TRIE_NODE_SWIDTH + scount * TRIE_SLIST_SWIDTH; /* allocate for all symbols as well */

      p = PPM_allocate_trie_node (strie, alloc);
      strie ->T_nodes [pnode] = p;

      /* store the counts */
      strie->T_nodes [p+TRIE_TCOUNT_OFFSET] = tcount;

      /* Now store nodes for each symbol if needed */
      if (d <= max_depth)
      {
	p += TRIE_NODE_SWIDTH; /* skip to start of symbol list */
	while (sptr)
        {
	  PPM_get_slist (trie, sptr, &sym, &child, &next_sptr);

	  if (next_sptr != NIL)
	      strie->T_nodes [p] = sym; /* store the symbol */
	  else if (sym == 0)
	      strie->T_nodes [p] = SPECIAL_SYMBOL; /* special case - indicate last
						      symbol in list is zero */
	  else
	      strie->T_nodes [p] = -sym; /* negative means there are no more symbols in the list */

	  PPM_build_static_trie1 (trie, strie, child, p+TRIE_CHILD_OFFSET, d+1, max_depth);
	  sptr = next_sptr; /* advance to next symbol in slist */
	  p += TRIE_SLIST_SWIDTH;
	}
      }
    }
}

struct PPM_trieType *
PPM_build_static_trie (struct PPM_trieType *trie, int max_depth)
/* Builds the static model from the dynamic model and returns a pointer to it. */
{
    struct PPM_trieType *strie;

    if ((trie == NULL) || (max_depth < 0))
        strie = NULL;
    else
      {
	strie = PPM_create_trie (TLM_Static);
        PPM_build_static_trie1 (trie, strie, TRIE_ROOT_NODE, NIL, 0,
				max_depth);
      }
    return (strie);
}

void
PPM_build_compressed_input1
  (struct PPM_trieType *trie, unsigned int *trie_ptrs,
   unsigned int *old_input_ptrs, unsigned int *new_input_ptrs,
   int node, int pnode, int d, int max_depth)
/* Builds the compressed input from the dynamic trie by deleting all input symbols that are no longer
   pointed to by it. */
{
    unsigned int sptr, next_sptr, sym, tcount;
    int child, p;

    assert (node != NIL);

    if (node < 0) /* pointer to input */
    {
        /* Store the back pointer pnode so that the trie can be updated to point to the
	   compressed input. You cannot do this using a simple array because the next time round
           the context part of the input may no longer exist - hence you need to allow for collisions.
           This also means that you can't have the input pointers pointing at the start of the context
           (as in the Computer Journal paper) rather than at the point at which it gets updated, since
           the compression process performed here could lose the context's starting locations. */
	p = ++trie_ptrs [0];
        trie_ptrs [p] = pnode; /* store the back pointer */
	old_input_ptrs [p] = -node; /* store its old input position */
        for (p=-node; p<=-node-d+max_depth+1; p++)
	    new_input_ptrs [p] = 1; /* indicates this input symbol is useful (it is being pointed at or may be in the future) */
    }
    else /* pointer to node in the trie */
    {
        /* Get start of list of symbols: */
	PPM_get_trie_node (trie, node, &tcount, &sptr);

        /* Now store nodes for each symbol if needed */
	if (d <= max_depth)
	{
	    while (sptr)
	      {
		PPM_get_slist (trie, sptr, &sym, &child, &next_sptr);

		PPM_build_compressed_input1 (trie, trie_ptrs, old_input_ptrs,
		    new_input_ptrs, child, sptr, d+1, max_depth);
		sptr = next_sptr; /* advance to next symbol in slist */
	      }
	}
    }
}

void
PPM_build_compressed_input (struct PPM_trieType *trie, int max_depth)
/* Builds the compressed input from the dynamic trie by deleting all input symbols that are no longer
   pointed to by it. */
{
    unsigned int pos, size, p;
    unsigned int new_input_pos, *trie_ptrs, *old_input_ptrs, *new_input_ptrs;

    if (trie == NULL)
        return;

    size = sizeof (unsigned int) * (trie->T_input_len + 2);
    fprintf (stderr, "size = %d trie len = %d\n", size, trie->T_input_len);
    trie_ptrs = (unsigned int *) malloc (size); /* used to store back ptrs from input; cell 0 contains its current max_size */
    old_input_ptrs = (unsigned int *) malloc (size); /* used to store the old poitions into the input */
    new_input_ptrs = (unsigned int *) malloc (size); /* used to store the new positions into the re-compressed input array */
    memset (old_input_ptrs, 0, size);
    trie_ptrs [0] = 0; /* set current max_size to 0 */

    PPM_build_compressed_input1 (trie, trie_ptrs, old_input_ptrs,
	new_input_ptrs, TRIE_ROOT_NODE, NIL, 0, max_depth);

    /* First replace input with its compressed version and insert into new_input_ptrs the new positions
       in compressed array at the same time */
    pos = 0;
    for (p=1; p<=trie->T_input_len; p++)
      if (new_input_ptrs [p] != 0)
	{
	  pos++;
	  trie->T_input [pos] = trie->T_input [p];
	  new_input_ptrs [p] = pos;
	}

    /* Now update the pointers in the trie accordingly */
    for (p=1; p<=trie_ptrs [0]; p++)
      {
	assert (old_input_ptrs [p] < size);

	new_input_pos = new_input_ptrs [old_input_ptrs [p]];
	PPM_put_slist (trie, trie_ptrs [p], -new_input_pos); /* replace old input ptr with negative of new one */
      }
    fprintf (stderr, "Compressing the input string: original size = %d compressed size %d (%.3f%%)\n",
	     trie->T_input_len, pos, (100.0 * pos/trie->T_input_len));
    trie->T_input_len = pos;
    trie->T_input_size = pos;

    free (trie_ptrs);
    /*free (new_input_ptrs);*/ /* Kludge - it should be freed, but this causes
				  a segment. fault in some obscure cases */
    free (old_input_ptrs);
}

/* Scaffolding for the trie module:

int
main ()
{
    unsigned int s, i, index, previndex, alloc, prevalloc;
    struct PPM_trieType *trie;

    *//* The following checks the indexes and allocations for consistency *//*

    previndex = 0;
    prevalloc = 0;

    trie = PPM_create_trie (TLM_Dynamic);
    s = PPM_create_trie_node (trie);
    for (i=1; i<10000; i++)
      {
	s = extend_trie_node (trie, 1);
	printf ("i = %3d s = %d total size = %d\n", i, s, trie->T_size);
	assert (s <  trie->T_size);
      }
    printf ("Total size = %d\n", trie->T_size);
    release_trie (trie);
}
*/
