/* Definitions of various structures for lookup tables (for storing keys and their
   associated unique ids and frequency counts). */

#ifndef TABLE_H
#define TABLE_H

boolean
TXT_valid_table (unsigned int table);
/* Returns TRUE if the table is valid, FALSE otherwize. */

unsigned int
TXT_create_table (unsigned int type, unsigned int types);
/* Creates and initializes a text table (for storing keys, their unique identification
   number and their frequency counts). */

void
TXT_release_table (unsigned int table);
/* Releases the memory allocated to the table and the table number (which may
   be reused in later TXT_create_table calls). */

boolean
TXT_update_table1 (unsigned int table, unsigned int key, unsigned int key_increment,
		   unsigned int *key_id, unsigned int *key_count);
/* Adds the text key to the table (if necessary), and returns TRUE if the key did not previously
   exist in the table, FALSE otherwise (in which case the existing key's frequency
   count will be incremented). Also sets the argument key_count to the resulting
   count and key_id to a unique identifier value associated with the key. These
   values start from 0 and are incremented by key_increment whenever a new key is added to
   the table. */

boolean
TXT_update_table (unsigned int table, unsigned int key, unsigned int *key_id, unsigned int *key_count);
/* Adds the text key to the table (if necessary), and returns TRUE if the key did not previously
   exist in the table, FALSE otherwise (in which case the existing key's frequency
   count will be incrmented). Also sets the argument key_count to the resulting
   count and key_id to a unique identifier value associated with the key. These
   values start from 0 and are incremented by 1 whenever a new key is added to
   the table. */

void
TXT_insert_table (unsigned int table, unsigned int key, unsigned int key_id,
		  unsigned int key_increment);
/* Inserts the text key into the table and adds key_increment to its count. */

boolean
TXT_getid_table (unsigned int table, unsigned int key, unsigned int *key_id, unsigned int *key_count);
/* Returns TRUE if the text key is found in the table, FALSE otherwise. Also sets the argument
   key_id to the key's unique identifier value and key_count to its frequency count. */

unsigned int
TXT_getkey_table (unsigned int table, unsigned int key_id);
/* Returns the text number of the text key associated with the identifier value key_id in the table.
   Returns NIL is the key_id value does not exist. */

void
TXT_getinfo_table (unsigned int table, unsigned int *table_type,
		   unsigned int *types, unsigned int *tokens);
/* Returns table type (i.e. static or dynamic) and the number
   of types and tokens in it. */

void
TXT_reset_table (unsigned int table);
/* Resets the current text key so that the next call to TXT_next_table or
   TXT_nextsymbol_table will return the first key or symbol in the table. */

boolean
TXT_next_table (unsigned int table, unsigned int *key, unsigned int *key_id,
		unsigned int *key_count);
/* Returns the next key in the table. Also sets the argument key_id to the
   key's unique identifier value and key_count to its frequency count. */

unsigned int
TXT_reset_tablepos (unsigned int table);
/* Resets the current table position. */

unsigned int
TXT_next_tablepos (unsigned int table, unsigned int tablepos);
/* Returns the next table position. */

unsigned int
TXT_expand_tablepos (unsigned int table, unsigned int tablepos);
/* Returns the expanded table position (by following the current
   position in the table out one level of the trie). */

void
TXT_get_tablepos (unsigned int table, unsigned int tablepos,
		  unsigned int *key, unsigned int *key_id,
		  unsigned int *key_count, unsigned int *key_symbol,
		  unsigned int *key_symbols);
/* Returns the expanded table position (by following the current
   position in the table out one level of the trie). */

void
TXT_dump_table (unsigned int file, unsigned int table);
/* Dumps out the text keys in the table. For debugging purposes. */

void
TXT_dump_table_keys (unsigned int file, unsigned int table);
/* Dumps out the text keys in the table. For debugging purposes. */

void
TXT_dump_table_keys1 (unsigned int file, unsigned int table);
/* Dumps out the text keys in the table. For debugging purposes. */

void
TXT_dump_table_keys2 (unsigned int file, unsigned int table);
/* Dumps out the text keys in the table. For debugging purposes. */

void
TXT_write_table (unsigned int file, unsigned int table, unsigned int type);
/* Writes out the text keys in the table to a file which can
   then be latter reloaded using TXT_load_table.  */

unsigned int
TXT_load_table_keys (unsigned int file);
/* Creates a new table and loads the text keys (one per line)
   from the file into it. */

unsigned int
TXT_load_table (unsigned int file);
/* Loads the table from the file. */

void
TXT_suspend_update_table (unsigned int table);
/* Suspends the update for a dynamic table temporarily.
   The update can be resumed using TXT_resume_update_table ().

   This is useful if it needs to be determined in advance which
   of two or more dynamic tables a sequence of text should be
   added to (based on how much each requires to encode it, say). */

void
TXT_resume_update_table (unsigned int table);
/* Resumes the update for a table. See TXT_suspend_update_table (). */

#endif
