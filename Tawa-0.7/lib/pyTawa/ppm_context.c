/* Context routines based on PPM models. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "io.h"
#include "coderanges.h"
#include "text.h"
#include "model.h"
#include "ppm_trie.h"
#include "ppm_model.h"
#include "ppm_context.h"

#define PPM_CONTEXTS_SIZE 1024      /* Initial max. number of context records */
#define SUFFIXHEAD 0                /* the head of the suffix list is at position 0 */

struct PPM_contextType *PPM_Contexts = NULL;/* List of context records */
unsigned int PPM_Contexts_max_size = 0; /* Current max. size of the PPM_Contexts array */
unsigned int PPM_Contexts_used = NIL;   /* List of deleted context records */
unsigned int PPM_Contexts_unused = 1;   /* Next unused context record */

/* Temporary storage used for maintaining context positions */
struct PPM_positionType PPM_Context_Position;

boolean
PPM_valid_context (unsigned int context)
/* Returns non-zero if the PPM context is valid, zero otherwize. */
{
    if (context == NIL)
        return (FALSE);
    else if (context >= PPM_Contexts_unused)
        return (FALSE);
    else if ((PPM_Contexts [context].C_suffixptr > 0) && (PPM_Contexts [context].C_suffixlist == NIL))
      {
	fprintf (stderr, "Invalid context %d\n", context);
        return (FALSE);
      }
    else
        return (TRUE);
}

unsigned int
PPM_valid_coder (codingType coding_type, unsigned int coder)
/* Checks if the coder is valid and returns a valid coder number. */
{
    if ((coding_type == ENCODE_TYPE) || (coding_type == DECODE_TYPE))
        assert (coder = TLM_valid_coder (coder));
    else
        assert (coder == NO_CODER);
    return (coder);
}

unsigned int
PPM_get_max_order (unsigned int model)
/* Returns the max. order of the PPM model. */
{
    unsigned int ppm_model;

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    return (PPM_Models [ppm_model].P_max_order);
}

void
PPM_validate_symbol (unsigned int context, unsigned int symbol,
		     struct PPM_positionType *position)
/* Validates the symbol and the context for updating the context's position. */
{
    assert (PPM_valid_context (context));

    /* Check whether symbol is negative. Since we are using "negative" symbols
       to indicate the end has been reached in a trie node's symbol list, we
       have to ensure that the user doesn't insert a negative symbol themselves */
    assert ((int) symbol >= 0);

    position->P_symbol = symbol;
}

void
PPM_create_suffixlist (unsigned int model, unsigned int context)
/* Creates a suffix list record. */
{
    unsigned int *suffixlist, suffixsize, s;

    assert (context != NIL);

    suffixsize = PPM_get_max_order (model) + 2;
    suffixlist = (unsigned int *) Malloc (81, (suffixsize) * sizeof (unsigned int));
    PPM_Contexts [context].C_suffixlist = suffixlist;

    for (s=0; s < suffixsize; s++)
        suffixlist [s] = NIL;
}

unsigned int
PPM_length_suffixlist (unsigned int model, unsigned int context)
/* Returns the length of the suffix list for context. */
{
    unsigned int *suffixlist, suffixsize, s;

    assert (context != NIL);

    suffixlist = PPM_Contexts [context].C_suffixlist;
    suffixsize = PPM_get_max_order (model) + 2;

    if (!suffixlist [SUFFIXHEAD])
        return (0);

    for (s=suffixsize-1; (!suffixlist [s]) && (s > 0); s--);

    return (s+1);
}

void
PPM_behead_suffixlist (unsigned int model, unsigned int context)
/* Keep all but the tail of the suffixes list. */
{
    unsigned int *suffixlist, suffixsize, s;

    assert (context != NIL);

    suffixlist = PPM_Contexts [context].C_suffixlist;
    suffixsize = PPM_get_max_order (model) + 2;

    if (!suffixlist [SUFFIXHEAD])
        return; /* empty */

    for (s=suffixsize-1; (!suffixlist [s]) && (s > 0); s--);

    suffixlist [SUFFIXHEAD] = suffixlist [s+1];
    
    for (s=1; s < suffixsize; s++)
        suffixlist [s] = NIL;

    PPM_Contexts [context].C_suffixptr = SUFFIXHEAD;
}

void
PPM_release_suffixlist (unsigned int model, unsigned int context)
/* Release the suffixes list. */
{
    unsigned int suffixsize;

    assert (context != NIL);

    PPM_Contexts [context].C_suffixptr = SUFFIXHEAD;

    if (PPM_Contexts [context].C_suffixlist != NULL)
      {
	suffixsize = PPM_get_max_order (model) + 2;
	Free (81, PPM_Contexts [context].C_suffixlist, (suffixsize) * sizeof (unsigned int));

	PPM_Contexts [context].C_suffixlist = NIL;
      }
}

void
PPM_dump_suffixlist (unsigned int file, unsigned int model,
		     unsigned int context)
/* Dumps the suffix list for context. */
{
    unsigned int *suffixlist, suffixsize, s;

    assert (TXT_valid_file (file));
    assert (context != NIL);

    suffixlist = PPM_Contexts [context].C_suffixlist;
    suffixsize = PPM_get_max_order (model) + 2;

    fprintf (Files [file], "Dump of suffix list: ");

    for (s=0; suffixlist [s] && (s < suffixsize); s++)
        fprintf (Files [file], "%d ", suffixlist [s]);
    fprintf (Files [file], "\n");
}

void
PPM_init_suffixlist (unsigned int model, unsigned int context)
/* Initializes the context's suffix list */
{
    unsigned int *suffixlist, suffixsize, s;

    assert (context != NIL);

    PPM_Contexts [context].C_suffixptr = SUFFIXHEAD;

    suffixlist = PPM_Contexts [context].C_suffixlist;
    suffixsize = PPM_get_max_order (model) + 2;

    for (s=0; s < suffixsize; s++)
        suffixlist [s] = NIL;
}

void
PPM_reset_suffixlist (unsigned int model, unsigned int context)
/* Reset the context to start at the head of the suffix list. */
{
    assert (context != NIL);

    PPM_Contexts [context].C_suffixptr = SUFFIXHEAD;
}

void
PPM_start_suffix (unsigned int model, unsigned int context)
/* Append a new suffix to the suffix list that points to the TRIE_ROOT_NODE of the trie. */
{
    unsigned int *suffixlist, suffixsize, max_order, s;

    assert (context != NIL);

    max_order = PPM_get_max_order (model);
    if (max_order < 0)
        return; /* don't start any suffixes since order = -1 */

    suffixlist = PPM_Contexts [context].C_suffixlist;
    suffixsize = max_order + 2;

    if (!suffixlist [SUFFIXHEAD])
        s = 0;
    else
      {
	for (s=suffixsize-1; (!suffixlist [s]) && (s > 0); s--);
	s++;
      }

    if (s >= suffixsize)
        return; /* Don't add any new PPM_Contexts longer than max_order */

    /* create new suffix pointing to the root of the trie for the new symbol */
    suffixlist [s] = TRIE_ROOT_NODE;

    PPM_Contexts [context].C_suffixptr = SUFFIXHEAD; /* start at the head of the list */
}

void
PPM_drop_suffix (unsigned int model, unsigned int context,
		 unsigned int suffixptr)
/* Drop the suffix at the head of the context's suffix list. Do this by
   inserting a null node pointer that will be removed once we have finished
   doing the updating. */
{
    assert (context != NIL);

    /* set dropped node to NIL */
    PPM_Contexts [context].C_suffixlist [suffixptr] = NIL;
}

void
PPM_repair_suffixlist (unsigned int model, unsigned int context)
/* Repair the suffixlist by removing the dropped suffixes. */
{
    unsigned int *suffixlist, suffixsize, shift, s;

    assert (context != NIL);

    suffixlist = PPM_Contexts [context].C_suffixlist;
    suffixsize = PPM_get_max_order (model) + 2;

    for (s=0; (s < suffixsize) && (!suffixlist [s]); s++);
    if (!s || (s >= suffixsize))
        return;

    /* Shift array to the left */
    shift = s;
    for (s=s; (s < suffixsize) && (suffixlist [s]); s++)
        suffixlist [s-shift] = suffixlist [s];

    /* Shift in NILs from the right */
    for (s=s-shift; (s < suffixsize); s++)
        suffixlist [s] = NIL;
}

unsigned int
PPM_next_suffix (unsigned int model, unsigned int context)
/* Return (and move along to) the node pointed to by the next suffix in the context's suffix list. */
{
    unsigned int suffixsize, s;

    assert (context != NIL);

    suffixsize = PPM_get_max_order (model) + 2;

    s = ++PPM_Contexts [context].C_suffixptr;
    if (s < suffixsize)
        return (PPM_Contexts [context].C_suffixlist [s]);
    else
      {
	PPM_Contexts [context].C_suffixptr = NIL;
	return (NIL);
      }
}

void
PPM_copy_suffixlist (unsigned int model, unsigned int new_context,
		     unsigned int context)
/* Copy the context's suffix list to the new context's suffix list. */
{
    unsigned int *new_suffixlist, *suffixlist, suffixsize, s;

    assert (context != NIL);
    assert (new_context != NIL);

    suffixsize = PPM_get_max_order (model) + 2;

    suffixlist = PPM_Contexts [context].C_suffixlist;
    new_suffixlist = PPM_Contexts [new_context].C_suffixlist;

    PPM_Contexts [new_context].C_suffixptr = PPM_Contexts [context].C_suffixptr;

    for (s=0; s < suffixsize; s++)
        new_suffixlist [s] = suffixlist [s];
}

void
PPM_release_position (struct PPM_positionType *position)
/* Creates a new position record if position is not NULL and returns a pointer to it. */
{
   if (position != NULL)
     {
       add_memory (MEM_FREE_TYPE, 86, sizeof (bits_type) +
		   position->P_exclusions->_alloc * sizeof(BITS_TYPE));
       BITS_FREE (position->P_exclusions);
       Free (82, position, sizeof (struct PPM_positionType));
     }
}

struct PPM_positionType *
PPM_copy_position (struct PPM_positionType *position)
/* Copies the position record and returns a pointer to the copy. */
{
    static struct PPM_positionType *new_position;

    if (position == NULL)
        return (NULL);

    new_position = (struct PPM_positionType *)
        Malloc (82, sizeof (struct PPM_positionType));
    new_position->P_node       = position->P_node;
    new_position->P_sptr       = position->P_sptr;
    new_position->P_count      = position->P_count;
    new_position->P_total      = position->P_total;
    new_position->P_subtotal   = position->P_subtotal;
    new_position->P_target     = position->P_target;
    new_position->P_coderanges = TLM_copy_coderanges (position->P_coderanges);
    new_position->P_codelength = position->P_codelength;
    new_position->P_escape_codelength = position->P_escape_codelength;

    if (position->P_exclusions == NULL)
        new_position->P_exclusions = NULL;
    else
        BITS_COPY (new_position->P_exclusions, position->P_exclusions);

    return (new_position);
}

void
PPM_encode_position (unsigned int model, codingType coding_type, unsigned int coder,
		     int order, struct PPM_positionType *position)
/* Encodes the arithmetic coding range based on the current position
   and order using the codingType and coder. */
{
    static unsigned int lbnd, hbnd, totl;

    coder = PPM_valid_coder (coding_type, coder);

    if ((coding_type == UPDATE_TYPE) || (coding_type == UPDATE1_TYPE) ||
	(coding_type == UPDATE2_TYPE))
        return; /* do nothing */

    lbnd = position->P_subtotal;
    hbnd = lbnd + position->P_count;
    totl = position->P_total;
    switch (coding_type)
      {
      case FIND_CODELENGTH_TYPE:
      case UPDATE_CODELENGTH_TYPE:
      case UPDATE1_CODELENGTH_TYPE:
	if (hbnd != totl) /* not coding an escape? */
	    position->P_codelength = position->P_escape_codelength +
	        Codelength (lbnd, hbnd, totl);
	else
	  {
	    position->P_escape_codelength += Codelength (lbnd, hbnd, totl);
	    position->P_codelength = position->P_escape_codelength;
	  }
	break;
      case FIND_MAXORDER_TYPE:
      case UPDATE_MAXORDER_TYPE:
	position->P_escape_codelength = 0.0;
	position->P_codelength = Codelength (lbnd, hbnd, totl);
	break;
      case FIND_CODERANGES_TYPE:
      case UPDATE_CODERANGES_TYPE:
	/*fprintf (stderr, "Appending to coderange %d %d %d\n", lbnd, hbnd, totl);*/
	TLM_append_coderange (position->P_coderanges, lbnd, hbnd, totl);
	break;
      case ENCODE_TYPE:
	Coders [coder].A_arithmetic_encode
	    (Coders [coder].A_encoder_output_file, lbnd, hbnd, totl);
	break;
      case DECODE_TYPE:
	Coders [coder].A_arithmetic_decode
	    (Coders [coder].A_decoder_input_file, lbnd, hbnd, totl);
	break;
      case UPDATE_TYPE:
      case UPDATE1_TYPE:
      case UPDATE2_TYPE:
      default:
	break;
      }
    if (Debug.range)
      {
        fprintf (stderr, "PPM Model %d Symbol: ", model);
	TXT_dump_symbol (Stderr_File, position->P_symbol);
        fprintf (stderr, " Order: %d Coding range: lbnd %d hbnd %d totl %d",
		 order, lbnd, hbnd, totl);
	switch (coding_type)
	  {
	  case FIND_CODELENGTH_TYPE:
	  case UPDATE_CODELENGTH_TYPE:
	  case UPDATE1_CODELENGTH_TYPE:
	  case FIND_MAXORDER_TYPE:
	  case UPDATE_MAXORDER_TYPE:
	    fprintf (stderr, " codelength %.3f total %.3f",
		     Codelength (lbnd, hbnd, totl), position->P_codelength);
	    break;
	  case ENCODE_TYPE:
	  case DECODE_TYPE:
	    fprintf (stderr, " codelength %.3f", Codelength (lbnd, hbnd, totl));
	    break;
	  default:
	    break;
	  }
	fprintf (stderr, "\n");
      }
}

struct PPM_positionType *
PPM_start_position (unsigned int model, unsigned int context,
		    operType oper_type, codingType coding_type,
		    unsigned int coder, struct PPM_positionType *position)
/* Starts a new context with a new position record, creating it if doesn't
   already exist and returning a pointer to it. */
{
    unsigned int ppm_model;

    ppm_model = Models [model].M_model;
    assert (PPM_valid_model (ppm_model));
    coder = PPM_valid_coder (coding_type, coder);

    if (position == NULL)
      {
        position = (struct PPM_positionType *)
	    Malloc (82, sizeof (struct PPM_positionType));
	position->P_exclusions = NULL;
	position->P_coderanges = NIL;
      }

    position->P_node = PPM_Contexts [context].C_node;

    /* set up the exclusions bitmap: */
    if ((PPM_Models [ppm_model].P_performs_full_excls) &&
	(coding_type != UPDATE_MAXORDER_TYPE))
      {
	if (position->P_exclusions == NULL)
	  {
	    BITS_ALLOC (position->P_exclusions);
	    add_memory (MEM_MALLOC_TYPE, 86, - (sizeof (bits_type) +
	       position->P_exclusions->_alloc * sizeof(BITS_TYPE)));
	  }
	BITS_ZERO (position->P_exclusions);
      }

    PPM_reset_position (model, context, oper_type, coding_type, coder,
			PPM_Contexts [context].C_node, position);

    position->P_codelength = 0.0;
    position->P_escape_codelength = 0.0;

    if ((coding_type == FIND_CODERANGES_TYPE) ||
	(coding_type == UPDATE_CODERANGES_TYPE))
      { /* Release existing coderanges list and recreate new empty one */
	TLM_release_coderanges (position->P_coderanges);
	position->P_coderanges = TLM_create_coderanges ();
      }

    if (oper_type != NEXT_SYMBOL_TYPE)
        PPM_reset_suffixlist (model, context); /* Start at the head of the suffix list */
    return (position);
}

void
PPM_reset_position (unsigned int model, unsigned int context,
		    operType oper_type, codingType coding_type,
		    unsigned int coder, int node,
		    struct PPM_positionType *position)
/* Resets various positional information according to the specified node in
   the context's trie. */
{
    static unsigned int total, tcount, sym, sentinel_sym, alphabet_size;
    static unsigned int ppm_model, sptr, head_sptr, next_sptr, escape_method;
    static int child;
    static boolean order_minus1, maxorder_type, performs_full_excls, performs_update_excls;
    static ptable_type *ptable;
    static struct PPM_trieType *trie;

    assert (PPM_valid_context (context));

    ppm_model = Models [model].M_model;
    assert (PPM_valid_model (ppm_model));
    coder = PPM_valid_coder (coding_type, coder);

    trie = PPM_Models [ppm_model].P_trie;
    ptable = PPM_Models [ppm_model].P_ptable;
    alphabet_size = PPM_Models [ppm_model].P_alphabet_size;
    performs_full_excls = PPM_Models [ppm_model].P_performs_full_excls;
    performs_update_excls = PPM_Models [ppm_model].P_performs_update_excls;
    escape_method = PPM_Models [ppm_model].P_escape_method;

    position->P_node = node;
    position->P_sptr = NIL;
    position->P_count = 0;
    position->P_subtotal = 0;

    if ((alphabet_size == 0) && (node <= TRIE_ROOT_NODE) && (ptable != NULL))
      { /* unbounded alphabet - use the cumulative probability table instead */
	position->P_total = ptable_get_total (ptable);
	/* ignore exclusions for the time being */
      }
    else
      {
	total = 0;

	order_minus1 = ((node == NIL) || (trie == NULL) ||
			(trie->T_nodes == NULL));
	maxorder_type = (coding_type == UPDATE_MAXORDER_TYPE);

	if (order_minus1)
	  head_sptr = NIL;
	else
	  { /* not at order -1 */
	    PPM_get_trie_node (trie, node, &tcount, &head_sptr);
	    /* head_sptr now points to start of list of symbols */
	  }

	if (oper_type != FIND_SYMBOL_TYPE)
	  { /* need to have total pre-calculated for next symbol function to work */
	    if (order_minus1)
	      {
		if (maxorder_type || !performs_full_excls)
		  /* add one for break symbol */
		  total = alphabet_size + 1;
		else
		  { 
		    for (sym = 0; sym < alphabet_size; sym++)
		      {
			if (!BITS_ISSET (position->P_exclusions, sym))
			  total++;
		      }
		    total++; /* Add one for the break symbol */
		  }
	      }
	    else
	      { /* calculate totals from the trie */
		if (escape_method == TLM_PPM_Method_A)
		    total = 1;
		sptr = head_sptr;
		sentinel_sym = TXT_sentinel_symbol ();
		while (sptr) /* process all the symbols */
		  {
		    PPM_get_slist (trie, sptr, &sym, &child, &next_sptr);
		    if ((sym == sentinel_sym) || maxorder_type ||
			!performs_full_excls ||
			!BITS_ISSET (position->P_exclusions, sym))
		        total += PPM_get_trie_count (trie, node, child, sptr,
						     next_sptr, sym);
		    if (!maxorder_type && (escape_method != TLM_PPM_Method_A))
		        total++; /* add 1 for escape */

		    sptr = next_sptr; /* advance to next symbol in slist */
		  }
	      }
	  }

	position->P_sptr = head_sptr;
	position->P_total = total;
      }

    if (coding_type != DECODE_TYPE)
        position->P_target = 0;
    else if (position->P_total)
      { /* find the target decoding range */
	position->P_target =
	  Coders [coder].A_arithmetic_decode_target
	    (Coders [coder].A_decoder_input_file, position->P_total);
	if (Debug.level > 1)
	    fprintf (stderr, "Debug target: target %d totl %d\n", position->P_target, position->P_total);
      }
}

void
PPM_update_position (unsigned int model, unsigned int context,
		     codingType coding_type, struct PPM_positionType *position)
/* The referenced context record is updated so that the current symbol
   referred to by the context record becomes the last symbol of the current
   context position. If the model is dynamic then the internal statistics in
   the model are also updated. (Note that only one active context is permitted
   for each dynamic model, whereas there is no limit on the number of active
   contexts associated with a static model.) */
{
    unsigned int ppm_model, node, new_node, shead, sptr, prev_sptr, tcount;
    unsigned int *suffixlist, suffixpos, suffixptr, alphabet_size, escape_method;
    unsigned int symbol, next_symbol, max_symbol, max_depth;
    boolean dynamic, unbounded, static_symbol, performs_update_excls, update_excl;
    int child, max_order;
    struct PPM_trieType *trie;

    assert (PPM_valid_context (context));

    ppm_model = Models [model].M_model;
    assert (PPM_valid_model (ppm_model));

    update_excl = FALSE; /* Used for performing update exclusions for dynamic
			    models only */

    assert (TLM_valid_model (model));
    max_order = PPM_Models [ppm_model].P_max_order;

    escape_method = PPM_Models [ppm_model].P_escape_method;
    alphabet_size = PPM_Models [ppm_model].P_alphabet_size;
    performs_update_excls = PPM_Models [ppm_model].P_performs_update_excls;

    unbounded = (alphabet_size == 0);
    trie = PPM_Models [ppm_model].P_trie;
    assert ((trie != NULL) || (max_order == -1));

    dynamic = (Models [model].M_model_form == TLM_Dynamic);

    max_symbol = PPM_Models [ppm_model].P_max_symbol;
    symbol = position->P_symbol;
    static_symbol = (!unbounded && (symbol > max_symbol) &&
		     (symbol != TXT_sentinel_symbol ()));

    max_depth = max_order;
    if (dynamic && (max_order >= 0))
        max_depth++; /* Need to update counts for depth=max_depth+1 for dynamic model only */

    node = PPM_Contexts [context].C_node;
    suffixlist = PPM_Contexts [context].C_suffixlist;

    if (max_order >= 0)
      {
	if (dynamic && !static_symbol && (coding_type != UPDATE1_TYPE) &&
	    (coding_type != UPDATE2_TYPE) &&
	    (coding_type != UPDATE1_CODELENGTH_TYPE))
	    PPM_update_input (trie, symbol);

	/* Update each suffix in the suffix list */
	suffixpos = PPM_length_suffixlist (model, context);
	suffixptr = SUFFIXHEAD;
	while (suffixpos && suffixlist [suffixptr] != NIL)
	  {
	    node = suffixlist [suffixptr];
	    assert (node != NIL);

	    PPM_get_trie_node (trie, node, &tcount, &shead);
	    if (dynamic && !static_symbol && !update_excl &&
		(coding_type != UPDATE2_TYPE))
	      {
		/* Increment total count for dynamic models only: */
		PPM_increment_trie_node (trie, node, escape_method);
		if (performs_update_excls)
		    update_excl = TRUE; /* Update exclusions - stop updating counts */
	      }

	    child = NIL;
	    prev_sptr = NIL;
	    if (suffixpos <= max_depth) /* Update suffix if not too long */
	        /* See if this node predicts the symbol: */
		sptr = PPM_find_slist (trie, shead, symbol, &child,
				       &prev_sptr);
	    else
	        sptr = NIL;

	    if ((sptr != NIL) && (child > NIL))
	        /* Update to point to the symbol's child node */
	        suffixlist [suffixptr] = child;
	    else if (!dynamic || (suffixpos > max_depth))
	        /* drop the head off the suffix list */
	        PPM_drop_suffix (model, context, suffixptr);
	    else if (child == NIL)
	      {
		/* The new child node points to the input string: */
		PPM_add_slist (trie, node, prev_sptr, symbol,
			   -(trie->T_input_len));
		/* drop the head off the suffix list: */
		PPM_drop_suffix (model, context, suffixptr);
	      }
	    else /* must point to input since the child pointer is negative */
	      {
		next_symbol = trie->T_input[-(child-1)]; /* next symbol in input */

		new_node = PPM_create_trie_node (trie); /* create new node in the trie */

		PPM_put_slist (trie, sptr, new_node);
		if (suffixpos < max_depth) /* Update suffix if not too long */
		    /* Point child to next one back in input: */
		    PPM_add_slist (trie, new_node, NIL, next_symbol,
				   (child - 1));

		suffixlist [suffixptr] = new_node; /* Update to point to the new child node */
	      }

	    suffixptr++;
	    suffixpos--;
	  }
	PPM_repair_suffixlist (model, context);
	/*PPM_dump_suffixlist (stderr, model, context);*/
      }

    if (symbol == TXT_sentinel_symbol ())
      { /* break symbol: force a "null" prior context by reverting back to
           order 1 statistics */
	PPM_behead_suffixlist (model, context); /* Reset the suffixes list to contain just its tail */
      }
    else if (symbol > max_symbol)
      {
	/* When this occurs for unbounded sized alphabets it indicates that
	   a new symbol has occurred */
	if (!unbounded)
	  {
	    fprintf (stderr, "Unbounded error: symbol %d > max_symbol %d\n",
		     symbol, max_symbol);
	  }

	assert (unbounded);
	assert (symbol == max_symbol + 1);

	PPM_Models [ppm_model].P_max_symbol = symbol;
	/* this expands the alphabet by 1 */
      }

    /* Set node to be the first node in the updated suffix list if it's a static model, otherwise, set it
       to the next in the list for a dynamic model (the dynamic model needs to maintain a ptr to the longest
       node (depth=max_depth+1 in trie, and length=max_depth+2 for slist); on the other hand, the static model
       must start predicting from the max_depth node in the trie. */
    if (max_order < 0)
        node = NIL;
    else if (suffixlist [SUFFIXHEAD] == NIL)
        node = TRIE_ROOT_NODE;
    else if (!dynamic || (PPM_length_suffixlist (model, context) < max_depth))
        node = suffixlist [SUFFIXHEAD];
    else if ((suffixlist [SUFFIXHEAD] == NIL) ||
	     (suffixlist [SUFFIXHEAD+1] == NIL))
        node = TRIE_ROOT_NODE;
    else
        node = suffixlist [SUFFIXHEAD+1];

    PPM_Contexts [context].C_node = node;

    /* Add another suffix pointing to the root: */
    PPM_start_suffix (model, context);
    /*PPM_dump_suffixlist (stderr, context);*/

    /* Need to reset the context position for the TLM_next_symbol function */
    if (PPM_Contexts [context].C_position != NULL)
        PPM_Contexts [context].C_position =
	  PPM_start_position (model, context, NEXT_SYMBOL_TYPE,
			      FIND_CODELENGTH_TYPE, NO_CODER,
			      PPM_Contexts [context].C_position);

}

boolean
PPM_next_ptable_position (unsigned int model, unsigned int context,
			  operType oper_type, codingType coding_type,
			  struct PPM_positionType *position)
/* Processes a cumulative probability distribution. At the moment, always returns FALSE since you do not
   need to process anything else. */
{
    static unsigned int model_form, ppm_model, symbol, count, subtotal, sptr;
    static ptable_type *ptable;

    model_form = Models [model].M_model_form;

    ppm_model = Models [model].M_model;
    assert (PPM_valid_model (ppm_model));

    ptable = PPM_Models [ppm_model].P_ptable;

    count = 0;
    subtotal = 0;
    switch (oper_type)
      {
      case FIND_SYMBOL_TYPE:
	symbol = position->P_symbol; /* we can go directly to the target symbol */
	break;
      case FIND_TARGET_TYPE:
	symbol = ptable_get_symbol (ptable, position->P_target, &subtotal, &count);
	break;
      case NEXT_SYMBOL_TYPE:
	if (sptr <= ptable->max + 1)
	    symbol = sptr++; /* move to next symbol */
	else
	  {
	    sptr = NIL;
	    symbol = TXT_sentinel_symbol ();
	  }
	break;
      }
    if (oper_type != FIND_TARGET_TYPE)
      {
	count = ptable_get_count (ptable, symbol);
	subtotal = ptable_get_lbnd (ptable, symbol);
      }

    position->P_symbol = symbol;
    position->P_count = count;
    position->P_subtotal = subtotal;

    if ((coding_type != FIND_CODELENGTH_TYPE) &&
	(coding_type != FIND_CODERANGES_TYPE) &&
	(coding_type != FIND_MAXORDER_TYPE) &&
	(coding_type != UPDATE2_TYPE) &&
	(model_form != TLM_Static))
        ptable_increment_count (ptable, symbol, 1); /* update the counts */

    return (FALSE); /* no more processing to be done for probability table */
}

boolean
PPM_next_position (unsigned int model, unsigned int context,
		   operType oper_type, codingType coding_type,
		   struct PPM_positionType *position)
/* Returns TRUE if more context symbol positions need to be processed to
   locate the requested symbol or target. */
{
    static unsigned int ppm_model, count, sptr, next_sptr;
    static unsigned int alphabet_size, symbol, target_symbol;
    static struct PPM_trieType *trie;
    static int node, child;

    static boolean found;         /* found the symbol or target range we were looking for */
    static boolean maxorder_type; /* only for maxorder type operations */
    static boolean perform_excl;  /* exclusions need to be performed */
    static boolean symbol_excl;   /* symbol has been excluded; don't add it to the counts */
    static boolean order_minus1;  /* we have escaped down or at at order -1 */

    ppm_model = Models [model].M_model;
    assert (PPM_valid_model (ppm_model));

    alphabet_size = PPM_Models [ppm_model].P_alphabet_size;
    node = position->P_node;

    if ((alphabet_size == 0) && (node <= TRIE_ROOT_NODE) &&
	(PPM_Models [ppm_model].P_ptable != NULL))
      /* return if we have found a probability table: */
       return (PPM_next_ptable_position (model, context, oper_type,
					 coding_type, position));

    /* Not a probability table - have to use the node's symbol list instead */
    target_symbol = position->P_symbol;

    trie = PPM_Models [ppm_model].P_trie;

    child = NIL;
    order_minus1 = FALSE;

    sptr = position->P_sptr;

    /* Advance to next symbol if necessary */
    order_minus1 = (node == NIL) || (trie == NULL) || (trie->T_nodes == NULL);
    if (order_minus1)
      { /* order -1 */
	child = NIL;
	if (sptr < alphabet_size)
	    symbol = sptr++;
	else
	  {
	    symbol = TXT_sentinel_symbol ();
	    sptr = NIL;
	  };
      }
    else
      { /* not order -1 */
	PPM_get_slist (trie, sptr, &symbol, &child, &next_sptr);
	sptr = next_sptr; /* advance to next symbol in slist */
      }

    maxorder_type = (coding_type == UPDATE_MAXORDER_TYPE);
    perform_excl = (symbol != TXT_sentinel_symbol ()) &&
        PPM_Models [ppm_model].P_performs_full_excls;
    symbol_excl = perform_excl && !maxorder_type &&
      BITS_ISSET (position->P_exclusions, symbol);

    /* Get the count */
    if (symbol_excl)
        count = 0;
    else
        count = PPM_get_trie_count (trie, node, child, position->P_sptr,
				    next_sptr, symbol);

    /* Have we found what we were looking for? */
    switch (oper_type)
      {
      case NEXT_SYMBOL_TYPE:
	found = (!symbol_excl);
	break;
      case FIND_SYMBOL_TYPE:
	found = (symbol == target_symbol);
	break;
      case FIND_TARGET_TYPE:
	found = (!position->P_count && (position->P_target < position->P_subtotal + count));
	break;
      default:
	found = FALSE; /* we shouldn't ever get here */
	break;
      }

    /* Update the context counts and totals */
    if (found)
      {
	position->P_symbol = symbol;
        position->P_count = count;
      }
    if (oper_type == FIND_SYMBOL_TYPE)
        position->P_total += count +
	  /* plus one for escape count for each symbol found */
	  (!order_minus1 && !maxorder_type);

    if (!symbol_excl && !position->P_count)
        /* haven't found the symbol or target yet */
        position->P_subtotal += count;

    /* add symbol to list of exclusions: */
    if (perform_excl && !maxorder_type && !symbol_excl && !order_minus1)
        BITS_SET (position->P_exclusions, symbol);

    position->P_sptr = sptr;

    return (!(((!symbol_excl) && (oper_type == NEXT_SYMBOL_TYPE)) ||
	      ((position->P_count != 0) && (sptr == NIL)))); /* not any more symbols to be processed */
}

boolean
PPM_find_position (unsigned int model, unsigned int context,
		   operType oper_type, codingType coding_type,
		   unsigned int coder, struct PPM_positionType *position)
/* Moves along to the requested symbol or target in the context, updating all
   the ncessary information and returning all the data necessary to find the
   arithmetic coding range. Returns TRUE if the symbol or target has been
   found. */
{
    static int node, max_order, order;
    static unsigned int ppm_model;
    static struct PPM_trieType *trie;

    ppm_model = Models [model].M_model;
    assert (PPM_valid_model (ppm_model));
    coder = PPM_valid_coder (coding_type, coder);

    node = position->P_node;
    max_order = PPM_Models [ppm_model].P_max_order;
    trie = PPM_Models [ppm_model].P_trie;
    order = PPM_length_suffixlist (model, context) - 1;
    if (order > max_order)
        order = max_order;

    assert ((trie != NULL) || (max_order == -1));

    if (oper_type == NEXT_SYMBOL_TYPE)
      {
	position->P_count = 0;
        position->P_codelength = 0.0;
        position->P_escape_codelength = 0.0;

	/* Release existing coderanges list and recreate new empty one */
	/* *** */
	TLM_betail_coderanges (position->P_coderanges);
      }

    /* Advance to requested symbol */
    for (;;) /* Keep searching for a target or a symbol that
		hasn't been excluded */
    {
	if ((max_order >= 0) && !position->P_sptr)
	{ /* no more symbols in this level of the trie -
             "escape" down to a lower order level */
	    if (coding_type == UPDATE_MAXORDER_TYPE)
	        return (FALSE);

	    if (node == NIL)
	      {
		if ((coding_type != FIND_CODELENGTH_TYPE) &&
		    (coding_type != FIND_CODERANGES_TYPE) &&
		    (coding_type != FIND_MAXORDER_TYPE))
		  PPM_update_position (model, context, coding_type, position);
	        return (FALSE); /* no more symbols to process */
	      }

	    if (position->P_subtotal > 0)
	    { /* Only need to encode escape when symbols have been found */
		position->P_count = position->P_total - position->P_subtotal; 
		PPM_encode_position (model, coding_type, coder, order, position); /* encode an escape */
	    }

	    if (order > -1)
	        order--;
	    node = PPM_next_suffix (model, context);
	    
	    PPM_reset_position (model, context, oper_type, coding_type, coder,
				node, position);
	}

	if (!PPM_next_position (model, context, oper_type, coding_type,
				position))
	    break;
    }

    PPM_encode_position (model, coding_type, coder, order, position);
    if (Debug.level > 4)
        PPM_dump_trie (Stderr_File, trie, order, NULL);
    if (oper_type == NEXT_SYMBOL_TYPE)
        position->P_subtotal += position->P_count;
    if ((coding_type != FIND_CODELENGTH_TYPE) &&
	(coding_type != FIND_CODERANGES_TYPE) &&
	(coding_type != FIND_MAXORDER_TYPE))
        PPM_update_position (model, context, coding_type, position);
    return (TRUE);
}

unsigned int
PPM_create_context1 (void)
/* Return a new pointer to a context record. */
{
    unsigned int context, old_size;

    if (PPM_Contexts_used != NIL)
    {	/* use the first record on the used list */
	context = PPM_Contexts_used;
	PPM_Contexts_used = PPM_Contexts [context].C_next;
    }
    else
    {
	context = PPM_Contexts_unused;
	if (PPM_Contexts_unused >= PPM_Contexts_max_size)
	{ /* need to extend PPM_Contexts array */
	    old_size = PPM_Contexts_max_size * sizeof (struct PPM_contextType);
	    if (PPM_Contexts_max_size == 0)
		PPM_Contexts_max_size = PPM_CONTEXTS_SIZE;
	    else
		PPM_Contexts_max_size = 10*(PPM_Contexts_max_size+50)/9; 

	    PPM_Contexts = (struct PPM_contextType *)
	        Realloc (85, PPM_Contexts, PPM_Contexts_max_size *
			 sizeof (struct PPM_contextType), old_size);

	    if (PPM_Contexts == NULL)
	    {
		fprintf (stderr, "Fatal error: out of PPM_Contexts space\n");
		exit (1);
	    }
	}
	PPM_Contexts_unused++;
    }

    PPM_Contexts [context].C_node = NIL;

    PPM_Contexts [context].C_suffixptr = NIL;

    /*fprintf (stderr, "Creating context %d\n", context);*/

    return (context);
}

void
PPM_copy_context1 (unsigned int model, unsigned int context,
		   unsigned int new_context)
/* Copies the contents of the specified context into the new context. */
{
    PPM_Contexts [new_context].C_node  = PPM_Contexts [context].C_node;

    PPM_copy_suffixlist (model, new_context, context);

    PPM_Contexts [new_context].C_position =
        PPM_copy_position (PPM_Contexts [context].C_position);
}
