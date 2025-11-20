/* Routines for cumulative probability tables. */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include "io.h"
#include "text.h"
#include "model.h"
#include "cpt_ptable.h"

unsigned int CPTp_escape_table
(struct CPTp_table_type *table)
/* Returns the escape count for the frequencies. */
{
    return (table->singletons + 1);
}

void CPTp_init_table
(struct CPTp_table_type *table)
/* Initializes the table. */
{
    table->trie = NULL;
    table->cfreq = 0;
    table->types = 0;
    table->singletons = 0;
}

struct CPTp_table_type *
CPTp_create_table ()
/* Creates and initializes the table. */
{
    struct CPTp_table_type *table;

    table = (struct CPTp_table_type *) Malloc (160, sizeof (struct CPTp_table_type));
    CPTp_init_table (table);
    return (table);
}

struct CPTp_trie_type
*CPTp_create_trie (struct CPTp_trie_type *node, unsigned int key, unsigned int key_pos, unsigned int freq)
/* Creates a new node (or reuses old node). Insert KEY into it. */
{
    struct CPTp_trie_type *this;
    unsigned int symbol;

    if (node != NIL)
        this = node;
    else
      {
        this = (struct CPTp_trie_type *) Malloc (160, sizeof (struct CPTp_trie_type));
      }
    this->key = TXT_copy_text (key);
    assert (TXT_get_symbol (key, key_pos, &symbol));
    this->key_symbol = symbol;
    this->next = NULL;
    this->down = NULL;
    this->freq = freq;
    this->cfreq = freq;
    this->pfreq = 0;
    return (this);
}

struct CPTp_trie_type *
CPTp_copy_trie (struct CPTp_trie_type *node)
/* Creates a new node by copying from an old one. */
{
    struct CPTp_trie_type *this;

    assert (node != NIL);
    this = (struct CPTp_trie_type *) Malloc(160, sizeof (struct CPTp_trie_type));
    this->key = node->key;
    this->key_symbol = node->key_symbol;
    this->next = node->next;
    this->down = node->down;
    this->freq = node->freq;
    this->cfreq = node->cfreq;
    this->pfreq = node->pfreq;
    return (this);
}

struct CPTp_trie_type *
CPTp_find_list (struct CPTp_trie_type *head, unsigned int key, unsigned int key_pos, boolean *found)
/* Find the link that contains the symbol and return a pointer to it. Assumes
   the links are in ascending lexicographical order. If the character is not found,
   return a pointer to the previous link in the list. */
{
    struct CPTp_trie_type *this, *that;
    unsigned int symbol;
    boolean found1;

    *found = FALSE;
    if (!TXT_get_symbol (key, key_pos, &symbol))
        return (NULL);

    if (head == NULL)
        return (NULL);

    found1 = FALSE;
    this = head;
    that = NIL;
    while ((this != NIL) && (!found1))
      {
        if (symbol == this->key_symbol)
	    found1 = TRUE;
	else if (symbol < this->key_symbol)
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

struct CPTp_trie_type *
CPTp_insert_list (struct CPTp_trie_type *head, struct CPTp_trie_type *here,
		 unsigned int key_pos, unsigned int key, unsigned int freq)
/* Insert new link after here and return it. Maintain the links in ascending
   lexicographical order. */
{
    struct CPTp_trie_type *there, *new;

    assert (head != NIL);

    if (here == NIL) { /* at the head of the list */
        /* maintain head at the same node position by copying it */
        new = CPTp_copy_trie (head);
	CPTp_create_trie (head, key, key_pos, freq);
	head->next = new;
	return (head);
    }
    new = CPTp_create_trie (NIL, key, key_pos, freq);
    there = here->next;
    if (there == NIL) /* at the tail of the list */
	here->next = new;
    else { /* in the middle of the list */
	here->next = new;
	new->next = there;
    }
    return (new);
}

struct CPTp_trie_type *
CPTp_find_node (struct CPTp_trie_type *node, unsigned int key, unsigned int key_pos)
/* Returns pointer to node if the key is found in the trie. */
{
    boolean found;
    struct CPTp_trie_type *here;

    assert (TXT_valid_text (key));
    if (node == NIL)
	return (NIL);

    here = CPTp_find_list (node, key, key_pos, &found);
    if (!found) /* Not in the list */
        return (NIL);

    /* Found in the list - is it the same key? */
    if (here->key != NIL)
      if (!TXT_compare_text (key, here->key)) /* key matches */
	return (here);

    if (here->down == NIL) 
        return (NIL);
    if (key_pos + 1 > TXT_length_text (key)) /* end of the key */
        return (NIL);

    return (CPTp_find_node (here->down, key, key_pos+1));
}

struct CPTp_trie_type *
CPTp_find_table (struct CPTp_table_type *table, unsigned int key)
/* Finds the key in the cumulative probability table. */
{
    if (table == NULL)
        return (NULL);
    else
        return (CPTp_find_node (table->trie, key, 0));
}

struct CPTp_trie_type *
CPTp_update_node (struct CPTp_table_type *table, struct CPTp_trie_type *node,
		 unsigned int key, unsigned int key_pos, unsigned int freq)
/* Add the KEY into the NODE of the trie. If NODE is NIL,
   then creates and returns it. Adds the frequency. */
{
    boolean found;
    unsigned int symbol;
    struct CPTp_trie_type *here, *pnode;

    assert (TXT_valid_text (key));
    if (node == NULL)
      {
	if (freq == 1)
	    table->singletons++;
	node = CPTp_create_trie (NIL, key, key_pos, freq);
	return (node);
      }
    here = CPTp_find_list (node, key, key_pos, &found);
    if (!found) { /* Not in the list - insert the new key */
	if (freq == 1)
	    table->singletons++;
	node = CPTp_insert_list (node, here, key_pos, key, freq);
	return (node);
    }
    /* Found in the list - is it the same key? */
    if (here->key != NIL)
      {
	if (!TXT_compare_text (key, here->key))
	  { /* key matches - add freq */
	    if (here->freq == 1)
	        table->singletons--;
	    here->freq += freq;
	    here->cfreq += freq;
	    return (here);
	  }

	if ((here->down == NIL) && (key_pos+1 < TXT_length_text (here->key)))
	  { /* move old key one level down if needed */
	    node = CPTp_copy_trie (here);
	    assert (TXT_get_symbol (here->key, key_pos+1, &symbol));
	    node->key_symbol = symbol;
	    node->next = NULL;
	    here->down = node;
	    here->key = NIL;
	    here->freq = 0;
	  }
      }

    here->cfreq += freq;
    if (key_pos+1 >= TXT_length_text (key))
      { /* end of the key */
	if (freq == 1)
	    table->singletons++;
	here->key = TXT_copy_text (key);
	here->freq = freq;
	return (here);
      }

    pnode = here->down;
    node = CPTp_update_node (table, pnode, key, key_pos+1, freq);
    if (!pnode)
        here->down = node;
    return (node);
}

boolean
CPTp_update_table (struct CPTp_table_type *table, unsigned int key, unsigned int freq)
/* Adds the key to the cumulative probability table. Returns true if the key is new. */
{
    struct CPTp_trie_type *node, *pnode;
    boolean new;

    pnode = table->trie;
    node = CPTp_update_node (table, table->trie, key, 0, freq);
    if (pnode == NIL)
        table->trie = node;
    table->cfreq += freq;

    new = ((node == NULL) || (node->freq == freq));
    if (new)
        table->types++;
    return (new);
}

void
CPTp_dump_node (unsigned int file, struct CPTp_trie_type *node, unsigned int level, boolean indented)
/* Dumps out the keys at the NODE in the trie. */
{
    while (node != NULL) {
        if (indented)
	    fprintf (Files [file], "        > ");
        fprintf (Files [file], "        %3d [%c] (%5d)", level, node->key_symbol, node->cfreq);
        if (node->key != NIL)
	  {
	    fprintf (Files [file], "% 5d ", node->freq);
	    TXT_dump_text (file, node->key, NULL);
	  }
	fprintf (Files [file], "\n");
	CPTp_dump_node (file, node->down, level+1, indented);
	node = node->next;
    }
}

void
CPTp_dump_table1 (unsigned int file, struct CPTp_table_type *table, boolean indented)
/* Dumps out the keys in the cumulative probability table data structure. */
{
    if (indented)
        fprintf (Files [file], "        > ");
    fprintf (Files [file], "        Total = %d singletons = %d\n",
	     table->cfreq, table->singletons);
    CPTp_dump_node (file, table->trie, 1, indented);
}

void
CPTp_dump_table (unsigned int file, struct CPTp_table_type *table)
/* Dumps out the keys in the cumulative probability table data structure. */
{
    if (table == NULL)
        fprintf (Files [file], "Table is NULL\n");
    else
        CPTp_dump_table1 (file, table, FALSE);
}

struct CPTp_table_type *
CPTp_load_table (unsigned int file)
/* Loads the table from the file. */
{
    unsigned int type, types;
    unsigned int key, key_count, k;
    struct CPTp_table_type *table;

    assert (TXT_valid_file (file));

    key = TXT_create_text ();

    /* Read in the table type */
    type = fread_int (file, INT_SIZE);

    /* Read in the number of keys */
    types = fread_int (file, INT_SIZE);

    table = CPTp_create_table ();

    /* Now read in each key */
    for (k = 0; k < types; k++)
      {
	/* read in the key and its count */
	TXT_load_filetext (file, key);
	key_count = fread_int (file, INT_SIZE);

	/*
	fprintf (stderr, "Key = ");
	TXT_dump_text (Stderr_File, key, NULL);
	fprintf (stderr, " count = %d\n", key_count);
	*/

	/* Now insert all the data into the table */
	CPTp_update_table (table, key, key_count);
      }

    TXT_release_text (key);

    return (table);
}

void
CPTp_write_node (unsigned int file, struct CPTp_trie_type *node, unsigned int level)
/* Writes out the keys at the NODE in the trie to the FILE. */
{
    while (node != NULL) {
        if (node->key != NIL)
	  {
	    TXT_write_filetext (file, node->key);
	    fwrite_int (file, node->freq, INT_SIZE);
	  }
	CPTp_write_node (file, node->down, level+1);
	node = node->next;
    }
}

void
CPTp_write_table (unsigned int file, struct CPTp_table_type *table, unsigned int type)
/* Writes out the text keys in the table to a file which can
   then be latter reloaded using TXT_load_table.  */
{
    assert (TXT_valid_file (file));

    /* Write out the table type  */
    fwrite_int (file, type, INT_SIZE);

    /* Write out the number of keys  */
    if (table == NULL)
        fwrite_int (file, 0, INT_SIZE);
    else
      {
	fwrite_int (file, table->types, INT_SIZE);
	/* Write out the table's trie */
	CPTp_write_node (file, table->trie, 1);
      }
}

boolean
CPTp_get_exclusions (struct CPTp_trie_type *node, unsigned int key_pos,
		     unsigned int *excls_key, struct CPTp_trie_type **excls_node,
		     struct CPTp_trie_type *excls_end_node, unsigned int *excls_cfreq,
		     struct CPTp_trie_type **excls_down,
		     struct CPTp_trie_type **excls_end_down)
/* Returns TRUE if an exclusions key is found with the nodes; returns it if so, plus its frequency
   and cumulative frequency. */
{
    struct CPTp_trie_type *xnode, *xdown, *xend_down;
    unsigned int xsymbol, xkey, xcfreq;
    boolean found;

    xnode = *excls_node;
    if ((xnode != NULL) && (xnode->key_symbol < node->key_symbol))
        xnode = xnode->next;

    found = FALSE;
    if (xnode != NULL)
      {
	xkey = xnode->key;
	xcfreq = xnode->cfreq;
	if (xnode->key_symbol == node->key_symbol)
	    found = TRUE;
	else /* exclusions assume that "higher order" predictions will always occur at the "lower order"
		so check for extraneous error nodes */
	  assert (xnode->key_symbol > node->key_symbol);
      }
    else if (excls_end_node != NULL)
      {
	xkey = excls_end_node->key;
	xcfreq = excls_end_node->cfreq;
	if ((xkey != NIL) && (TXT_get_symbol (xkey, key_pos, &xsymbol)) && (xsymbol == node->key_symbol))
	    found = TRUE;
      }
    else
      {
	xkey = NIL;
	xcfreq = 0;
      }

    *excls_node = xnode;
    *excls_key = xkey;
    *excls_cfreq = xcfreq;

    if (!found)
      {
        xdown = NULL;
	xend_down = NULL;
      }
    else
      {
	xend_down = excls_end_node;
	if (xnode == NULL)
	    xdown = NULL;
	else
	    xdown = xnode->down;
	if ((xkey != NIL) && (excls_end_node == NULL) && (xdown == NULL))
	    xend_down = xnode; /* save the end node on the excls trie for lower
				  nodes on the main trie */
      }

    *excls_down = xdown;
    *excls_end_down = xend_down;

    return (found);
}

void
CPTp_encode_node (struct CPTp_trie_type *node, struct CPTp_trie_type *excls_node,
		 struct CPTp_trie_type *excls_end_node, unsigned int key,
		 unsigned int key_pos, unsigned int *lbnd,
		 unsigned int *hbnd, unsigned int *totl)
/* Gets the frequency and cumulative frequency associated with the key. */
{
    struct CPTp_trie_type *excls_down, *excls_end_down;
    unsigned int symbol, excls_key, excls_cfreq, key_length;
    boolean found_excls;

    assert (TXT_get_symbol (key, key_pos, &symbol));
    key_length = TXT_length_text (key);
    while (node != NULL)
      {
	/* find the corresponding exclusion node if there is one and set found_excls to TRUE
	   if we still have a valid exclusion key prefix */
	found_excls = CPTp_get_exclusions
	    (node, key_pos, &excls_key, &excls_node, excls_end_node, &excls_cfreq,
	     &excls_down, &excls_end_down);

	if (symbol == node->key_symbol)
	  {
	    if (node->key != NIL)
	      {
		if (TXT_compare_text (key, node->key))
		  {
		    if (!found_excls || (excls_key == NIL) || TXT_compare_text (node->key, excls_key))
		        *lbnd += node->freq;
		  }
		else
		  { /* key matches */
		    *hbnd = *lbnd + node->freq;
		    return;
		  }
	      }

	    if ((key_pos+1 < key_length) && (node->down != NULL))
	      {
	        CPTp_encode_node (node->down, excls_down, excls_end_down, key, key_pos+1,
				 lbnd, hbnd, totl);
		return;
	      }
	  }
	else
	  {
	    *lbnd += node->cfreq;
	    if (found_excls)
	      {
		assert (excls_cfreq <= node->cfreq);
		*lbnd -= excls_cfreq;
	      }
	  }

	node = node->next;
      }
}

void
CPTp_encode_arith_range (struct CPTp_table_type *table, struct CPTp_table_type *exclusions_table,
			unsigned int key, unsigned int *lbnd, unsigned int *hbnd, unsigned int *totl)
/* Encode the arithmetic encoding range for the key in the trie. */
{
    struct CPTp_trie_type *excls_trie;
    unsigned int escape;

    escape = CPTp_escape_table (table);
    *totl = escape + table->cfreq;

    /* check that the key has not already been excluded - this is required for debug checking only */
    assert ((key == NIL) || (exclusions_table == NULL) ||
	    !CPTp_find_table (exclusions_table, key));

    if (key != NIL)
      {
	*lbnd = escape;
	*hbnd = 0;
	if (exclusions_table == NULL)
	    excls_trie = NULL;
	else
	    excls_trie = exclusions_table->trie;
	CPTp_encode_node (table->trie, excls_trie, NULL, key, 0, lbnd, hbnd, totl);
      }

    if (exclusions_table != NULL)
      {
	assert (*totl > exclusions_table->cfreq);
	*totl -= exclusions_table->cfreq; /* subtract exclusions if there are any */
      }
    if ((key == NIL) || !*hbnd)
      { /* encode escape */
	*lbnd = 0;
	*hbnd = escape;
      }

    assert (*totl > 0);
    assert (*hbnd <= *totl);
    assert (*lbnd < *hbnd);
    assert (*lbnd != *hbnd);
}

unsigned int
CPTp_decode_arith_total (struct CPTp_table_type *table, struct CPTp_table_type *exclusions_table)
/* Return the total for the arithmetic range required to decode the target. */
{
    unsigned int escape, totl, excls_totl;

    escape = CPTp_escape_table (table);
    totl = table->cfreq + escape;

    if (exclusions_table != NULL)
      {
	excls_totl = exclusions_table->cfreq;
	assert (excls_totl < totl);
	totl -= excls_totl;
    }
    return (totl);
}

unsigned int
CPTp_decode_node (struct CPTp_trie_type *node, struct CPTp_trie_type *excls_node,
		 struct CPTp_trie_type *excls_end_node, unsigned int key_pos,
		 unsigned int target, unsigned int *lbnd, unsigned int *hbnd)
/* Decodes the key associated with the arithmetic target for the node. */
{
    struct CPTp_trie_type *excls_down, *excls_end_down;
    unsigned int cfreq, excls_key, excls_cfreq;
    boolean found_excls, not_excluded;

    while (node != NIL)
      {
	/* find the corresponding exclusion node if there is one and set found_excls to TRUE
	   if we still have a valid exclusion key prefix */
	found_excls = CPTp_get_exclusions
	    (node, key_pos, &excls_key, &excls_node, excls_end_node, &excls_cfreq,
	     &excls_down, &excls_end_down);

	cfreq = node->cfreq;
	if (found_excls)
	  {
	    assert (excls_cfreq <= cfreq);
	    cfreq -= excls_cfreq;
	  }
        if (*lbnd + cfreq <= target)
	    *lbnd += cfreq;
	else
	  {
	    not_excluded = !found_excls || (node->key == NIL) ||
	        (excls_key == NIL) || TXT_compare_text (node->key, excls_key);
	    if (not_excluded && (*lbnd + node->freq > target))
	      {
	        *hbnd = *lbnd + node->freq;
		return (node->key);
	      }
	    else if (node->down != NULL)
	      {
	        if (not_excluded && node->freq) /* add freq for key at this node if there is one */
		    *lbnd += node->freq;
	        return (CPTp_decode_node (node->down, excls_down, excls_end_down, key_pos+1, target,
					 lbnd, hbnd));
	      }
	  }
	node = node->next;
      }
    return (NIL);
}

unsigned int
CPTp_decode_arith_key (struct CPTp_table_type *table, struct CPTp_table_type *exclusions_table,
		      unsigned int target, unsigned int totl, unsigned int *lbnd, unsigned int *hbnd)
/* Decodes the key and arithmetic range for the target. */
{
    struct CPTp_trie_type *excls_trie;
    unsigned int key, escape;

    escape = CPTp_escape_table (table);
    assert ((target >= 0) && (target <= totl));

    if ((target >= 0) && (target < escape))
      { /* decode escape */
        *lbnd = 0;
	*hbnd = escape;
	return (NIL); /* means an escape has been encoded */
      }

    *lbnd = escape;

    if (exclusions_table == NULL)
        excls_trie = NULL;
    else
	excls_trie = exclusions_table->trie;

    key = CPTp_decode_node (table->trie, excls_trie, NULL, 0, target, lbnd, hbnd);

    assert (totl > 0);
    assert (*hbnd <= totl);
    assert (*lbnd < *hbnd);
    assert (*lbnd != *hbnd);

    assert (TXT_valid_text (key));
    return (key);
}

unsigned int Check_lbnd, Check_hbnd, Check_totl;

boolean
CPTp_check_arith_ranges_node (struct CPTp_table_type *table, struct CPTp_table_type *exclusions_table,
			     struct CPTp_trie_type *node, boolean debug)
/* Checks the arith coding ranges for the node. */
{
    unsigned int lbnd, hbnd, totl;
    unsigned int lbnd1, hbnd1, totl1, key1, target1;
    boolean errors, error;

    errors = FALSE;
    while (node != NULL)
      {
        if ((node->key != NIL) && (CPTp_find_table (exclusions_table, node->key) == NULL))
	  { /* Check only keys that have not been excluded */
	    CPTp_encode_arith_range (table, exclusions_table, node->key, &lbnd, &hbnd, &totl);
	    error = (totl != Check_totl) || (lbnd < Check_hbnd);
	    errors = errors || error;

	    if (debug || error)
	      {
		fprintf (stderr, "Encoding key ");
		TXT_dump_text (Stderr_File, node->key, NULL);
		fprintf (stderr, ": lbnd %d hbnd %d totl %d", lbnd, hbnd, totl);
	      }
	    if (error)
	        fprintf (stderr, " *** error ***");
	    if (debug || error)
	        fprintf (stderr, "\n");
	    Check_lbnd = lbnd;
	    Check_hbnd = hbnd;

	    totl1 = CPTp_decode_arith_total (table, exclusions_table);
	    for (target1 = lbnd; target1 < hbnd; target1++)
	      {
		key1 = CPTp_decode_arith_key (table, exclusions_table, target1, totl1, &lbnd1, &hbnd1);
		error = (lbnd != lbnd1) || (hbnd != hbnd1) || (totl != totl1) ||
		    TXT_compare_text (key1, node->key);
		errors = errors || error;

		if (debug || error)
		  {
		    fprintf (stderr, "Decoding key ");
		    TXT_dump_text (Stderr_File, node->key, NULL);
		    fprintf (stderr, ": key ");
		    TXT_dump_text (Stderr_File, key1, NULL);
		    fprintf (stderr, " target %d lbnd %d hbnd %d totl %d", target1, lbnd1, hbnd1, totl1);
		  }
		if (error)
		    fprintf (stderr, " *** error ***");
		if (debug || error)
		    fprintf (stderr, "\n");
	      }
	  }
	if (!CPTp_check_arith_ranges_node (table, exclusions_table, node->down, debug))
	    errors = TRUE;
	node = node->next;
      }
    return (!errors);
}

boolean
CPTp_check_arith_ranges (struct CPTp_table_type *table, struct CPTp_table_type *exclusions_table,
			boolean debug)
/* Check to see the arithmetic encoding ranges for the table (and its exclusions) are
   consistent. */
{
    unsigned int check_decode_totl;
    boolean ok;

    /* Check escape key first */
    CPTp_encode_arith_range (table, exclusions_table, NIL, &Check_lbnd, &Check_hbnd, &Check_totl);
    if (debug)
        fprintf (stderr, "Checking escape key: lbnd %d hbnd %d totl %d\n",
		 Check_lbnd, Check_hbnd, Check_totl);

    check_decode_totl = CPTp_decode_arith_total (table, exclusions_table);
    ok = (check_decode_totl == Check_totl);
    if (debug || !ok)
        fprintf (stderr, "Checking decode total: encode totl %d decode totl %d",
		 check_decode_totl, Check_totl);
    if (!ok)
        fprintf (stderr, " *** error ***");
    if (debug || !ok)
        fprintf (stderr, "\n");

    /* Then check all the non-excluded keys in the table one at a time, making sure
       the ranges do not overlap and the total is the same in all cases */
    return (CPTp_check_arith_ranges_node (table, exclusions_table, table->trie, debug) && ok);
}

/* Scaffolding for table module.

int main()
{
    boolean new;
    char word [80];
    char *key;
    unsigned int lbnd, hbnd, totl, target, totl1, freq;
    struct CPTp_table_type Table;

    CPTp_init_table (&Table);
    for (;;) {
        printf ("word? ");
        scanf ("%s", word);
	printf ("freq? ");
	scanf ("%d", &freq);

	new = CPTp_update_table (&Table, word, freq);
	printf ("new = %d\n", new);

	CPTp_dump_table (Stdout_File, &Table);
	CPTp_encode_arith_range (&Table, word, &lbnd, &hbnd, &totl);
	printf ("lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
	totl1 = CPTp_decode_arith_total (&Table);
	printf ("totl = %d\n", totl1);
	for (target=0; target<totl1; target++) {
	    key = CPTp_decode_arith_key (&Table, target, totl1, &lbnd, &hbnd);
	    printf ("lbnd = %d hbnd = %d totl = %d ", lbnd, hbnd, totl1);
	    if (key == NIL)
	        printf ("key = NIL\n");
	    else
	        printf ("key = %s\n", key);
	}
    }
}
*/
