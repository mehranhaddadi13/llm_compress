/* Routines for contexts of cumulative probability tables. */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include "io.h"
#include "text.h"
#include "model.h"
#include "cpt_ptable.h"
#include "cpt_ctable.h"

void CPTc_init_table
(struct CPTc_table_type *table)
/* Initializes the table. */
{
    table->trie = NULL;
    table->types = 0;
}

struct CPTc_table_type *
CPTc_create_table ()
/* Creates and initializes the table. */
{
    struct CPTc_table_type *table;

    table = (struct CPTc_table_type *) Malloc (161, sizeof (struct CPTc_table_type));
    CPTc_init_table (table);
    return (table);
}

struct CPTc_trie_type
*CPTc_create_trie (struct CPTc_trie_type *node, unsigned int context,
		  unsigned int key, unsigned int freq, unsigned int pos,
		  struct CPTp_table_type *keys)
/* Creates a new node (or reuses old node). Insert (CONTEXT, KEY or KEYS) into it. */
{
    struct CPTc_trie_type *this;
    unsigned int symbol;

    if (node != NIL)
        this = node;
    else
      {
        this = (struct CPTc_trie_type *) Malloc (160, sizeof (struct CPTc_trie_type));
      }
    this->context = TXT_copy_text (context);
    assert (TXT_get_symbol (context, pos, &symbol));
    this->context_symbol = symbol;
    this->next = NULL;
    this->down = NULL;

    if (keys == NULL)
      {
	keys = CPTp_create_table ();
	CPTp_update_table (keys, key, freq);
      }
    this->table = keys;

    return (this);
}

struct CPTc_trie_type *
CPTc_copy_trie (struct CPTc_trie_type *node)
/* Creates a new node by copying from an old one. */
{
    struct CPTc_trie_type *this;

    assert (node != NIL);
    this = (struct CPTc_trie_type *) Malloc(160, sizeof (struct CPTc_trie_type));
    this->context = node->context;
    this->context_symbol = node->context_symbol;
    this->next = node->next;
    this->down = node->down;
    this->table = node->table;
    return (this);
}

struct CPTc_trie_type *
CPTc_find_list (struct CPTc_trie_type *head, unsigned int context, unsigned int pos, boolean *found)
/* Find the link that contains the symbol and return a pointer to it. Assumes
   the links are in ascending lexicographical order. If the character is not found,
   return a pointer to the previous link in the list. */
{
    struct CPTc_trie_type *this, *that;
    unsigned int symbol;
    boolean found1;

    *found = FALSE;
    if (!TXT_get_symbol (context, pos, &symbol))
        return (NULL);

    if (head == NULL)
        return (NULL);

    found1 = FALSE;
    this = head;
    that = NIL;
    while ((this != NIL) && (!found1))
      {
        if (symbol == this->context_symbol)
	    found1 = TRUE;
	else if (symbol < this->context_symbol)
	    break;
        else
	  {
	    that = this;
	    this = this->next;
	  }
      }
    *found = found1;
    if (!found1) /* link already exists */
        return (that);
    else
        return (this);
}

struct CPTc_trie_type *
CPTc_insert_list (struct CPTc_trie_type *head, struct CPTc_trie_type *here,
		 unsigned int context, unsigned int key, unsigned int freq, unsigned int pos,
		 struct CPTp_table_type *keys)
/* Insert new link after here and return it. Maintain the links in ascending
   lexicographical order. */
{
    struct CPTc_trie_type *there, *new;

    assert (head != NIL);

    if (here == NIL) { /* at the head of the list */
        /* maintain head at the same node position by copying it */
        new = CPTc_copy_trie (head);
	CPTc_create_trie (head, context, key, freq, pos, keys);
	head->next = new;
	return (head);
    }
    new = CPTc_create_trie (NIL, context, key, freq, pos, keys);
    there = here->next;
    if (there == NIL) /* at the tail of the list */
	here->next = new;
    else { /* in the middle of the list */
	here->next = new;
	new->next = there;
    }
    return (new);
}

struct CPTc_trie_type *
CPTc_find_node (struct CPTc_table_type *table, struct CPTc_trie_type *node,
	       unsigned int context, unsigned int pos)
/* Returns pointer to node if the context is found in the trie. */
{
    boolean found;
    struct CPTc_trie_type *here;

    assert (TXT_valid_text (context));
    if (node == NIL)
	return (NIL);

    here = CPTc_find_list (node, context, pos, &found);
    if (!found) /* Not in the list - insert the new context */
        return (NIL);

    /* Found in the list - is it the same context? */
    if (here->context != NIL)
      if (!TXT_compare_text (context, here->context)) /* context matches */
	return (here);

    if (here->down == NIL) /* move old context one level down if needed */
        return (NIL);
    if (pos + 1 > TXT_length_text (context)) /* end of the context */
        return (NIL);

    return (CPTc_find_node (table, here->down, context, pos+1));
}

struct CPTc_trie_type *CPTc_Context_node = NULL;

struct CPTc_trie_type *
CPTc_find_table (struct CPTc_table_type *table, unsigned int context)
/* Finds the context in the cumulative probability table. */
{
    struct CPTc_trie_type *trie;

    trie = CPTc_find_node (table, table->trie, context, 0);
    CPTc_Context_node = trie;
    return (trie);
}

struct CPTp_table_type *
CPTc_findcontext_table ()
/* Gets the context table associated with the last CPTc_find_table call. */
{
    if (CPTc_Context_node == NULL)
        return (NULL);
    else
        return (CPTc_Context_node->table);
}

struct CPTc_trie_type *
CPTc_update_node (struct CPTc_table_type *table, struct CPTc_trie_type *node,
		 unsigned int context, unsigned int key, unsigned int freq, unsigned int pos,
		 struct CPTp_table_type *keys)
/* Add the (CONTEXT,KEY) into the NODE of the trie (or all the KEYS if this is non-null).
   If NODE is NIL, then creates and returns it. */
{
    boolean found;
    unsigned int symbol;
    struct CPTc_trie_type *here, *pnode;

    assert (TXT_valid_text (context));
    if (node == NULL)
      {
	node = CPTc_create_trie (NIL, context, key, freq, pos, keys);
	return (node);
      }
    here = CPTc_find_list (node, context, pos, &found);
    if (!found) { /* Not in the list - insert the new context */
	node = CPTc_insert_list (node, here, context, key, freq, pos, keys);
	return (node);
    }
    /* Found in the list - is it the same context? */
    if (here->context != NIL)
      {
	if (!TXT_compare_text (context, here->context))
	  { /* context matches */
	    assert (here->table != NULL);
	    assert (keys == NULL);
	    CPTp_update_table (here->table, key, freq);
	    return (here);
	  }

	if ((here->down == NIL) && (pos+1 < TXT_length_text (here->context)))
	  { /* move old context one level down if needed */
	    node = CPTc_copy_trie (here);
	    assert (TXT_get_symbol (here->context, pos+1, &symbol));
	    node->context_symbol = symbol;
	    node->next = NULL;
	    here->down = node;
	    here->context = NIL;
	    here->table = NULL;
	  }
      }

    if (pos+1 >= TXT_length_text (context))
      { /* end of the context */
	here->context = TXT_copy_text (context);
	if (keys == NULL)
	  {
	    keys = CPTp_create_table ();
	    CPTp_update_table (keys, key, freq);
	  }
	here->table = keys;
	return (here);
      }

    pnode = here->down;
    node = CPTc_update_node (table, pnode, context, key, freq, pos+1, keys);
    if (!pnode)
        here->down = node;
    return (node);
}

boolean
CPTc_update_table (struct CPTc_table_type *ctable, unsigned int context, unsigned int key,
		  unsigned int freq, struct CPTp_table_type *keys)
/* Adds the context and key (or keys) to the context table. Returns true if the context is new. */
{
    struct CPTc_trie_type *node, *pnode;
    boolean new;

    pnode = ctable->trie;
    node = CPTc_update_node (ctable, ctable->trie, context, key, freq, 0, keys);
    if (pnode == NULL)
        ctable->trie = node;
    new = ((node == NULL) || (node->table == NULL) || (key != NIL) || (node->table->cfreq == freq));
    if (new)
        ctable->types++;
    return (new);
}

void
CPTc_dump_node (unsigned int file, struct CPTc_trie_type *node, unsigned int level)
/* Dumps out the contexts at the NODE in the trie. */
{
    while (node != NULL) {
        fprintf (Files [file], "        %3d [%c] ", level, node->context_symbol);
        if (node->context != NIL)
	    TXT_dump_text (file, node->context, NULL);
	fprintf (Files [file], "\n");

        if (node->table != NULL)
	    CPTp_dump_table1 (file, node->table, TRUE);

	CPTc_dump_node (file, node->down, level+1);
	node = node->next;
    }
}

void
CPTc_dump_table (unsigned int file, struct CPTc_table_type *table)
/* Dumps out the contexts in the cumulative probability table data structure. */
{
    if (table == NULL)
        fprintf (Files [file], "Table is NULL\n");
    else
        CPTc_dump_node (file, table->trie, 1);
}

struct CPTc_table_type *
CPTc_load_table (unsigned int file)
/* Loads the table from the file. */
{
    unsigned int type, types;
    unsigned int context, k;
    struct CPTc_table_type *ctable;
    struct CPTp_table_type *keys;

    assert (TXT_valid_file (file));

    context = TXT_create_text ();

    /* Read in the table type */
    type = fread_int (file, INT_SIZE);

    /* Read in the number of contexts */
    types = fread_int (file, INT_SIZE);

    ctable = CPTc_create_table ();

    /* Now read in each context */
    for (k = 0; k < types; k++)
      {
	/* read in the context and its associated probability table */
	TXT_load_filetext (file, context);
	keys = CPTp_load_table (file);
	CPTc_update_table (ctable, context, NIL, 0, keys);
      }

    TXT_release_text (context);

    return (ctable);
}

void
CPTc_write_node (unsigned int file, struct CPTc_trie_type *node, unsigned int level,
		unsigned int type)
/* Writes out the contexts at the NODE in the trie to the FILE. */
{
    while (node != NULL)
      {
        if (node->context != NIL)
	  {
	    TXT_write_filetext (file, node->context);
	    CPTp_write_table (file, node->table, type);
	  }
	CPTc_write_node (file, node->down, level+1, type);
	node = node->next;
      }
}

void
CPTc_write_table (unsigned int file, struct CPTc_table_type *table, unsigned int type)
/* Writes out the text keys in the table to a file which can
   then be latter reloaded using TXT_load_table.  */
{
    assert (TXT_valid_file (file));

    /* Write out the table type  */
    fwrite_int (file, type, INT_SIZE);

    /* Write out the number of contexts */
    if (table == NULL)
        fwrite_int (file, 0, INT_SIZE);
    else
      {
        fwrite_int (file, table->types, INT_SIZE);
	/* Write out the table's trie */
	CPTc_write_node (file, table->trie, 1, type);
      }
}

void
CPTc_encode_arith_range (struct CPTc_table_type *table, struct CPTp_table_type *exclusions_table,
			unsigned int context, unsigned int key, unsigned int *lbnd,
			unsigned int *hbnd, unsigned int *totl)
/* Encode the arithmetic encoding range for the context in the trie. */
{
    struct CPTc_trie_type *node;

    node = CPTc_find_table (table, context);
    if (node != NULL)
        CPTp_encode_arith_range (node->table, exclusions_table, key, lbnd, hbnd, totl);
    else
      {
	*lbnd = 0;
	*hbnd = 0;
	*totl = 0;
      }
}

unsigned int
CPTc_decode_arith_total (struct CPTc_table_type *table, struct CPTp_table_type *exclusions_table,
			unsigned int context)
/* Return the total for the arithmetic range required to decode the target. */
{
    struct CPTc_trie_type *node;

    node = CPTc_find_table (table, context);
    if (node == NULL)
        return (0);
    else
        return (CPTp_decode_arith_total (node->table, exclusions_table));
}

unsigned int
CPTc_decode_arith_key (struct CPTc_table_type *table, struct CPTp_table_type *exclusions_table,
		      unsigned int context, unsigned int target, unsigned int totl,
		      unsigned int *lbnd, unsigned int *hbnd)
/* Decode the key and arithmetic range for the target. */
{
    struct CPTc_trie_type *node;

    node = CPTc_find_table (table, context);
    if (node == NULL)
        return (NIL);
    else
        return (CPTp_decode_arith_key (node->table, exclusions_table, target, totl, lbnd, hbnd));
}

/* Scaffolding for table module.

int main()
{
    boolean new;
    char word [80];
    char *context;
    unsigned int lbnd, hbnd, totl, target, totl1, freq;
    struct CPTc_table_type Table;

    CPTc_init_table (&Table);
    for (;;) {
        printf ("word? ");
        scanf ("%s", word);
	printf ("freq? ");
	scanf ("%d", &freq);

	new = CPTc_update_table (&Table, word);
	printf ("new = %d\n", new);

	CPTc_dump_table (Stdout_File, &Table);
	CPTc_encode_arith_range (&Table, word, &lbnd, &hbnd, &totl);
	printf ("lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
	totl1 = CPTc_decode_arith_total (&Table);
	printf ("totl = %d\n", totl1);
	for (target=0; target<totl1; target++) {
	    context = CPTc_decode_arith_context (&Table, target, totl1, &lbnd, &hbnd);
	    printf ("lbnd = %d hbnd = %d totl = %d ", lbnd, hbnd, totl1);
	    if (context == NIL)
	        printf ("context = NIL\n");
	    else
	        printf ("context = %s\n", context);
	}
    }
}
*/
