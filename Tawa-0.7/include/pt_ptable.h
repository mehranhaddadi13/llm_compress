/* Definitions of various structures for cumulative probability tables. */

#ifndef PTp_TABLE_H
#define PTp_TABLE_H

struct PTp_trie_type { /* node in the trie */
  unsigned int key;            /* the whole key if this is a terminal node */
  unsigned int key_symbol;     /* the current symbol in the key */
  struct PTp_trie_type *next;  /* the next link in the list */
  struct PTp_trie_type *down;  /* the next level down in the trie */
  unsigned int freq;           /* the frequency count for the key */
  unsigned int cfreq;          /* the cumulative frequency of nodes at lower levels */
};

struct PTp_table_type {        /* stores keys and their cumulative probabilities */
  struct PTp_trie_type *trie;  /* pointer to trie of predictions */
  unsigned int cfreq;          /* the cumulative frequency of the trie */
  unsigned int types;          /* number of keys */
  unsigned int singletons;     /* number of keys with freq == 1 */
};

void
PTp_init_table (struct PTp_table_type *table);
/* Initializes the cumulative probability table. */

struct PTp_table_type *
PTp_create_table ();
/* Creates and initializes the table. */

struct PTp_trie_type *
PTp_find_table (struct PTp_table_type *table, unsigned int key);
/* Finds the key in the cumulative probability table. */

unsigned int
PTp_update_table (struct PTp_table_type *table, unsigned int key, unsigned int freq);
/* Adds the key to the cumulative probability table. Returns true if the key is new. */

void
PTp_dump_table1 (unsigned int file, struct PTp_table_type *table, boolean indented);
/* Dumps out the keys in the cumulative probability table data structure. */

void
PTp_dump_table (unsigned int file, struct PTp_table_type *table);
/* Dumps out the keys in the cumulative probability table data structure. */

struct PTp_table_type *
PTp_load_table (unsigned int file);
/* Loads the table from the file. */

void
PTp_write_table (unsigned int file, struct PTp_table_type *table, unsigned int type);
/* Writes out the text keys in the table to a file which can
   then be latter reloaded using TXT_load_table.  */

void
PTp_encode_arith_range (struct PTp_table_type *table, struct PTp_table_type *exclusions_table,
			unsigned int key, unsigned int *lbnd, unsigned int *hbnd, unsigned int *totl);
/* Encode the arithmetic encoding range for the key in the cumulative probability table. */

boolean
PTp_check_arith_ranges (struct PTp_table_type *table, struct PTp_table_type *exclusions_table,
			boolean debug);
/* Check to see the arithmetic encoding ranges for the table (and its exclusions) are
   consistent. */

unsigned int
PTp_decode_arith_total (struct PTp_table_type *table, struct PTp_table_type *exclusions_table);
/* Return the total for the arithmetic range required to decode the target. */

unsigned int
PTp_decode_arith_key (struct PTp_table_type *table, struct PTp_table_type *exclusions_table,
		      unsigned int target, unsigned int totl, unsigned int *lbnd, unsigned int *hbnd);
/* Decode the key and arithmetic range for the target. */

#endif
