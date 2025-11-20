/* Definitions of various structures for maintaing contexts of cumulative probability tables. */

#ifndef PTc_TABLE_H
#define PTc_TABLE_H

struct PTc_trie_type { /* node in the trie */
  unsigned int context;         /* the whole context if this is a terminal node */
  unsigned int context_symbol;  /* the current symbol in the context */
  struct PTc_trie_type *next;   /* the next link in the list */
  struct PTc_trie_type *down;   /* the next level down in the trie */
  struct PTp_table_type *table; /* the cumulative probability table of predictions
				   associated with this context */
};

struct PTc_table_type {         /* stores context contexts and their cumulative probabilities tables of
				   associated predictions */
  struct PTc_trie_type *trie;   /* pointer to trie of contexts */
  unsigned int types;           /* Number of unique contexts */
};

void
PTc_init_table (struct PTc_table_type *table);
/* Initializes the cumulative probability table. */

struct PTc_table_type *
PTc_create_table ();
/* Creates and initializes the table. */

struct PTc_trie_type *
PTc_find_table (struct PTc_table_type *table, unsigned int context);
/* Finds the context in the cumulative probability table. */

struct PTp_table_type *
PTc_findcontext_table ();
/* Gets the context table associated with the last PTc_find_table call. */

boolean
PTc_update_table (struct PTc_table_type *ctable, unsigned int context,
		  unsigned int key, unsigned int freq, struct PTp_table_type *keys);
/* Adds the (context,key or keys) to the cumulative probability table. Returns true if the
   context is new. */

void
PTc_dump_table (unsigned int file, struct PTc_table_type *table);
/* Dumps out the contexts in the cumulative probability table data structure. */

struct PTc_table_type *
PTc_load_table (unsigned int file);
/* Loads the table from the file. */

void
PTc_write_table (unsigned int file, struct PTc_table_type *table, unsigned int type);
/* Writes out the text keys in the table to a file which can
   then be latter reloaded using TXT_load_table.  */

void
PTc_encode_arith_range (struct PTc_table_type *table, struct PTp_table_type *exclusions_table,
			unsigned int context, unsigned int key, unsigned int *lbnd,
			unsigned int *hbnd, unsigned int *totl);
/* Encode the arithmetic encoding range for the context in the cumulative probability table. */

unsigned int
PTc_decode_arith_total (struct PTc_table_type *table, struct PTp_table_type *exclusions_table,
			unsigned int context);
/* Return the total for the arithmetic range required to decode the target. */

unsigned int
PTc_decode_arith_key (struct PTc_table_type *table, struct PTp_table_type *exclusions_table,
		      unsigned int context, unsigned int target, unsigned int totl,
		      unsigned int *lbnd, unsigned int *hbnd);
/* Decode the key and arithmetic range for the target. */

#endif
