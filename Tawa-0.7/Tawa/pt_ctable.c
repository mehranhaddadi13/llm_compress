/* Routines for contexts of cumulative probability tables. */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include "io.h"
#include "text.h"
#include "model.h"
#include "pt_ptable.h"
#include "pt_ctable.h"

void PTc_init_table
(struct PTc_table_type *table)
/* Initializes the table. */
{
    table->trie = NULL;
    table->types = 0;
}

struct PTc_table_type *
PTc_create_table ()
/* Creates and initializes the table. */
{
    struct PTc_table_type *table;

    table = (struct PTc_table_type *) Malloc (161, sizeof (struct PTc_table_type));
    PTc_init_table (table);
    return (table);
}

struct PTc_trie_type
*PTc_create_trie (struct PTc_trie_type *node, unsigned int context,
		  unsigned int key, unsigned int freq, unsigned int pos,
		  struct PTp_table_type *keys)
/* Creates a new node (or reuses old node). Insert (CONTEXT, KEY or KEYS) into it. */
{
    struct PTc_trie_type *this;
    unsigned int symbol;

    if (node != NIL)
        this = node;
    else
      {
        this = (struct PTc_trie_type *) Malloc (160, sizeof (struct PTc_trie_type));
      }
    this->context = TXT_copy_text (context);
    assert (TXT_get_symbol (context, pos, &symbol));
    this->context_symbol = symbol;
    this->next = NULL;
    this->down = NULL;

    if (keys == NULL)
      {
	keys = PTp_create_table ();
	PTp_update_table (keys, key, freq);
      }
    this->table = keys;

    return (this);
}

struct PTc_trie_type *
PTc_copy_trie (struct PTc_trie_type *node)
/* Creates a new node by copying from an old one. */
{
    struct PTc_trie_type *this;

    assert (node != NIL);
    this = (struct PTc_trie_type *) Malloc(160, sizeof (struct PTc_trie_type));
    this->context = node->context;
    this->context_symbol = node->context_symbol;
    this->next = node->next;
    this->down = node->down;
    this->table = node->table;
    return (this);
}

struct PTc_trie_type *
PTc_find_list (struct PTc_trie_type *head, unsigned int context, unsigned int pos, boolean *found)
/* Find the link that contains the symbol and return a pointer to it. Assumes
   the links are in ascending lexicographical order. If the character is not found,
   return a pointer to the previous link in the list. */
{
    struct PTc_trie_type *this, *that;
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

struct PTc_trie_type *
PTc_insert_list (struct PTc_trie_type *head, struct PTc_trie_type *here,
		 unsigned int context, unsigned int key, unsigned int freq, unsigned int pos,
		 struct PTp_table_type *keys)
/* Insert new link after here and return it. Maintain the links in ascending
   lexicographical order. */
{
    struct PTc_trie_type *there, *new;

    assert (head != NIL);

    if (here == NIL) { /* at the head of the list */
        /* maintain head at the same node position by copying it */
        new = PTc_copy_trie (head);
	PTc_create_trie (head, context, key, freq, pos, keys);
	head->next = new;
	return (head);
    }
    new = PTc_create_trie (NIL, context, key, freq, pos, keys);
    there = here->next;
    if (there == NIL) /* at the tail of the list */
	here->next = new;
    else { /* in the middle of the list */
	here->next = new;
	new->next = there;
    }
    return (new);
}

struct PTc_trie_type *
PTc_find_node (struct PTc_table_type *table, struct PTc_trie_type *node,
	       unsigned int context, unsigned int pos)
/* Returns pointer to node if the context is found in the trie. */
{
    boolean found;
    struct PTc_trie_type *here;

    assert (TXT_valid_text (context));
    if (node == NIL)
	return (NIL);

    here = PTc_find_list (node, context, pos, &found);
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

    return (PTc_find_node (table, here->down, context, pos+1));
}

struct PTc_trie_type *PTc_Context_node = NULL;

struct PTc_trie_type *
PTc_find_table (struct PTc_table_type *table, unsigned int context)
/* Finds the context in the cumulative probability table. */
{
    struct PTc_trie_type *trie;

    trie = PTc_find_node (table, table->trie, context, 0);
    PTc_Context_node = trie;
    return (trie);
}

struct PTp_table_type *
PTc_findcontext_table ()
/* Gets the context table associated with the last PTc_find_table call. */
{
    if (PTc_Context_node == NULL)
        return (NULL);
    else
        return (PTc_Context_node->table);
}

struct PTc_trie_type *
PTc_update_node (struct PTc_table_type *table, struct PTc_trie_type *node,
		 unsigned int context, unsigned int key, unsigned int freq, unsigned int pos,
		 struct PTp_table_type *keys)
/* Add the (CONTEXT,KEY) into the NODE of the trie (or all the KEYS if this is non-null).
   If NODE is NIL, then creates and returns it. */
{
    boolean found;
    unsigned int symbol;
    struct PTc_trie_type *here, *pnode;

    assert (TXT_valid_text (context));
    if (node == NULL)
      {
	node = PTc_create_trie (NIL, context, key, freq, pos, keys);
	return (node);
      }
    here = PTc_find_list (node, context, pos, &found);
    if (!found) { /* Not in the list - insert the new context */
	node = PTc_insert_list (node, here, context, key, freq, pos, keys);
	return (node);
    }
    /* Found in the list - is it the same context? */
    if (here->context != NIL)
      {
	if (!TXT_compare_text (context, here->context))
	  { /* context matches */
	    assert (here->table != NULL);
	    assert (keys == NULL);
	    PTp_update_table (here->table, key, freq);
	    return (here);
	  }

	if ((here->down == NIL) && (pos+1 < TXT_length_text (here->context)))
	  { /* move old context one level down if needed */
	    node = PTc_copy_trie (here);
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
	    keys = PTp_create_table ();
	    PTp_update_table (keys, key, freq);
	  }
	here->table = keys;
	return (here);
      }

    pnode = here->down;
    node = PTc_update_node (table, pnode, context, key, freq, pos+1, keys);
    if (!pnode)
        here->down = node;
    return (node);
}

boolean
PTc_update_table (struct PTc_table_type *ctable, unsigned int context, unsigned int key,
		  unsigned int freq, struct PTp_table_type *keys)
/* Adds the context and key (or keys) to the context table. Returns true if the context is new. */
{
    struct PTc_trie_type *node, *pnode;
    boolean new;

    pnode = ctable->trie;
    node = PTc_update_node (ctable, ctable->trie, context, key, freq, 0, keys);
    if (pnode == NULL)
        ctable->trie = node;
    new = ((node == NULL) || (node->table == NULL) || (node->table->cfreq == freq));
    if (new)
      {
        ctable->types++;
	/*
	fprintf (stderr, "Adding entry %d to table - context: ",
		 ctable->types);
	TXT_dump_text (Stderr_File, context, NULL);
	fprintf (stderr, " key: ");
	TXT_dump_text (Stderr_File, key, NULL);
	fprintf (stderr, "\n");
	*/
      } 
    return (new);
}

void
PTc_dump_node (unsigned int file, struct PTc_trie_type *node, unsigned int level)
/* Dumps out the contexts at the NODE in the trie. */
{
    while (node != NULL) {
        fprintf (Files [file], "        %3d [%c] ", level, node->context_symbol);
        if (node->context != NIL)
	    TXT_dump_text (file, node->context, NULL);
	fprintf (Files [file], "\n");

        if (node->table != NULL)
	    PTp_dump_table1 (file, node->table, TRUE);

	PTc_dump_node (file, node->down, level+1);
	node = node->next;
    }
}

void
PTc_dump_table (unsigned int file, struct PTc_table_type *table)
/* Dumps out the contexts in the cumulative probability table data structure. */
{
    if (table == NULL)
        fprintf (Files [file], "Table is NULL\n");
    else
        PTc_dump_node (file, table->trie, 1);
}

struct PTc_table_type *
PTc_load_table (unsigned int file)
/* Loads the table from the file. */
{
  /*unsigned int type;*/
    unsigned int types;
    unsigned int context, k;
    struct PTc_table_type *ctable;
    struct PTp_table_type *keys;

    assert (TXT_valid_file (file));

    context = TXT_create_text ();

    /* Read in the table type */
    /*type =*/ fread_int (file, INT_SIZE);

    /* Read in the number of contexts */
    types = fread_int (file, INT_SIZE);

    /*
    fprintf (stderr, "Reading #types: %d\n", types);
    */

    ctable = PTc_create_table ();

    /* Now read in each context */
    for (k = 0; k < types; k++)
      {
	/* read in the context and its associated probability table */
	TXT_load_filetext (file, context);

	/*
	fprintf (stderr, "Reading context: {");
	TXT_dump_text (Stderr_File, context, NULL);
	fprintf (stderr, "}\n");
	*/

	keys = PTp_load_table (file);
	PTc_update_table (ctable, context, NIL, 0, keys);
      }

    TXT_release_text (context);

    return (ctable);
}

unsigned int PTc_Write_Table_Count = 0;

void
PTc_write_node (unsigned int file, struct PTc_trie_type *node, unsigned int level,
		unsigned int type)
/* Writes out the contexts at the NODE in the trie to the FILE. */
{
    while (node != NULL)
      {
        if (node->context != NIL)
	  {
	    PTc_Write_Table_Count++;

	    TXT_write_filetext (file, node->context);

	    /*
	    fprintf (stderr, "Writing context: {");
	    TXT_dump_text (Stderr_File, node->context, NULL);
	    fprintf (stderr, "}\n");
	    */

	    PTp_write_table (file, node->table, type);
	  }
	PTc_write_node (file, node->down, level+1, type);
	node = node->next;
      }
}

void
PTc_write_table (unsigned int file, struct PTc_table_type *table, unsigned int type)
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
	PTc_Write_Table_Count = 0;

        fwrite_int (file, table->types, INT_SIZE);
	/*
	fprintf (stderr, "Writing #types: %d\n", table->types);
	*/

	/* Write out the table's trie */
	PTc_write_node (file, table->trie, 1, type);

	/* Check that correct number of contexts were written out */
	if (table->types != PTc_Write_Table_Count)
	  fprintf (stderr, "Fatal error writing out PTc table: #types (%d) != contexts written (%d)\n", table->types, PTc_Write_Table_Count);

	assert (table->types == PTc_Write_Table_Count);
      }
}

void
PTc_encode_arith_range (struct PTc_table_type *table, struct PTp_table_type *exclusions_table,
			unsigned int context, unsigned int key, unsigned int *lbnd,
			unsigned int *hbnd, unsigned int *totl)
/* Encode the arithmetic encoding range for the context in the trie. */
{
    struct PTc_trie_type *node;

    node = PTc_find_table (table, context);
    if (node != NULL)
        PTp_encode_arith_range (node->table, exclusions_table, key, lbnd, hbnd, totl);
    else
      {
	*lbnd = 0;
	*hbnd = 0;
	*totl = 0;
      }
}

unsigned int
PTc_decode_arith_total (struct PTc_table_type *table, struct PTp_table_type *exclusions_table,
			unsigned int context)
/* Return the total for the arithmetic range required to decode the target. */
{
    struct PTc_trie_type *node;

    node = PTc_find_table (table, context);
    if (node == NULL)
        return (0);
    else
        return (PTp_decode_arith_total (node->table, exclusions_table));
}

unsigned int
PTc_decode_arith_key (struct PTc_table_type *table, struct PTp_table_type *exclusions_table,
		      unsigned int context, unsigned int target, unsigned int totl,
		      unsigned int *lbnd, unsigned int *hbnd)
/* Decode the key and arithmetic range for the target. */
{
    struct PTc_trie_type *node;

    node = PTc_find_table (table, context);
    if (node == NULL)
        return (NIL);
    else
        return (PTp_decode_arith_key (node->table, exclusions_table, target, totl, lbnd, hbnd));
}

/* Scaffolding for table module.

int main()
{
    boolean new;
    char word [80];
    char *context;
    unsigned int lbnd, hbnd, totl, target, totl1, freq;
    struct PTc_table_type Table;

    PTc_init_table (&Table);
    for (;;) {
        printf ("word? ");
        scanf ("%s", word);
	printf ("freq? ");
	scanf ("%d", &freq);

	new = PTc_update_table (&Table, word);
	printf ("new = %d\n", new);

	PTc_dump_table (Stdout_File, &Table);
	PTc_encode_arith_range (&Table, word, &lbnd, &hbnd, &totl);
	printf ("lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
	totl1 = PTc_decode_arith_total (&Table);
	printf ("totl = %d\n", totl1);
	for (target=0; target<totl1; target++) {
	    context = PTc_decode_arith_context (&Table, target, totl1, &lbnd, &hbnd);
	    printf ("lbnd = %d hbnd = %d totl = %d ", lbnd, hbnd, totl1);
	    if (context == NIL)
	        printf ("context = NIL\n");
	    else
	        printf ("context = %s\n", context);
	}
    }
}
*/
