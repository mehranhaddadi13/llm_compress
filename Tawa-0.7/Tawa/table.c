/* Definitions of various structures for lookup tables (for storing keys and their
   associated unique ids and frequency counts). */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "io.h"
#include "text.h"
#include "table.h"

#define TABLES_ALLOC_SIZE 8      /* Initial allocation for the tables array */
#define TABLE_ALLOC_SIZE 8       /* Initial allocation for the table's keys
				     array */

struct table_stack_type
{ /* record in the stack */
  struct table_trie_type *Tnode; /* A node in the table's trie */
  struct table_stack_type *Tnext;/* The next link in the list */
};

struct table_trie_type
{ /* node in the trie */
  unsigned int Tkey;              /* The whole key if this is a terminal node*/
  unsigned int Tkey_id;           /* Unique id number associated with the key*/
  unsigned int Tkey_symbol;       /* The current symbol in the key */
  unsigned int Tkey_symbols;      /* Used for storing more symbols if needed */
  struct table_trie_type *Tnext;  /* The next link in the list */
  struct table_trie_type *Tdown;  /* The next level down in the trie */
};

struct table_type { /* stores keys and their associated unique id numbers */
  unsigned int Ttype;             /* Type of table i.e. static or dynamic */
  unsigned int Talloc;            /* Current allocation for the array of keys*/
  unsigned int Types;             /* Number of unique keys in the table */
  unsigned long long Tokens;      /* Total count for all keys in the table */
  unsigned int Tnull_count;       /* Number of null keys */
  unsigned int Tnull_id;          /* Id associated with null key */
  unsigned int *Tkeys;            /* All the text keys stored in the table */
  unsigned long long *Tcounts;    /* And their associated counts */
  struct table_trie_type *Ttrie;  /* Pointer to trie lookup table */

  /* Stack of pointers to trie nodes (used by TXT_reset_table and
     TXT_next_table): */
  struct table_stack_type *Thead; /* Head of the stack */
  struct table_stack_type *Ttail; /* Tail of the stack (used for deletion
				     only) */
};

#define Tnextused Talloc
/* Talloc is also used to store next link on used list when this record gets
   deleted */

struct table_stack_type *Used_table_stack = NULL; /* List of used table stack
						     records */

struct table_type *Tables = NULL; /* List of table records */
unsigned int Tables_max_size = 0; /* Current max. size of the Tables array */
unsigned int Tables_used = NIL;   /* List of deleted text records */
unsigned int Tables_unused = 1;   /* Next unused text record */

boolean
get_table_key (unsigned int key, unsigned int pos, unsigned int *symbol,
	       unsigned int *symbols, unsigned int *new_pos)
/* Gets the symbol(s) at position pos in the key and moves pos onto the
   next symbol position. */
{
    unsigned int symbol1, symbols1, sentinel1_symbol, syms_len;

    *new_pos = pos;

    symbols1 = NIL;
    sentinel1_symbol = TXT_sentinel1_symbol ();

    if (!TXT_get_symbol (key, pos, &symbol1))
        return (FALSE);
    if (symbol1 == sentinel1_symbol)
      {
	assert (TXT_get_symbol (key, ++pos, &syms_len));
	if (syms_len)
	  {
	    symbols1 = TXT_create_text ();
	    TXT_extract_text (key, symbols1, ++pos, syms_len);
	    pos += syms_len - 1;
	  }
      }

    *new_pos = pos + 1; /* move on to next symbol */
    *symbol  = symbol1;
    *symbols = symbols1;
    return (TRUE);
}
        
int
compare_table_keys (unsigned int symbol, unsigned int symbols,
		    unsigned int symbol1, unsigned int symbols1)
/* Compares the symbol(s). */
{
    if (symbols != NIL)
      if (symbols1 != NIL)
	return (TXT_compare_text (symbols, symbols1));
      else
	return (1); /* greater */
    else if (symbols1 != NIL)
        return (-1);
    else if (symbol < symbol1)
        return (-1);
    else if (symbol > symbol1)
        return (1);
    else
        return (0);
}

void
alloc_table_keys (struct table_type *table, unsigned int key_id)
/* Allocate enough space to include the key with key_id in the table's keys
   array. */
{
    unsigned int old_alloc, new_alloc, p;

    assert (table != NULL);
    old_alloc = table->Talloc;

    if (key_id >= old_alloc)
      { /* need to extend array */
	new_alloc = 10 * (key_id + 50)/9;

	table->Tkeys = (unsigned int *)
	  Realloc (33, table->Tkeys, new_alloc * sizeof (unsigned int),
		   old_alloc * sizeof (unsigned int));
	if (table->Tkeys == NULL)
	  {
	    fprintf (stderr, "Fatal error: out of table keys space\n");
	    exit (1);
	  }
	table->Tcounts = (unsigned long long *)
	  Realloc (33, table->Tcounts, new_alloc * sizeof (unsigned long long),
		   old_alloc * sizeof (unsigned long long));
	if (table->Tcounts == NULL)
	  {
	    fprintf (stderr, "Fatal error: out of table counts space\n");
	    exit (1);
	  }

	/* Initialize all the extra symbols to be NULL */
	for (p=old_alloc; p<new_alloc; p++)
	  {
	    table->Tkeys [p] = NIL;
	    table->Tcounts [p] = 0;
	  }

	table->Talloc = new_alloc;
      }
}

struct table_trie_type *
create_table_trie
(struct table_trie_type *node, unsigned int pos,
 unsigned int key, unsigned int key_id)
/* Creates a new node (or reuses old node). Insert KEY into it. */
{
    struct table_trie_type *newnode;
    unsigned int symbol, symbols, new_pos;

    if (node != NULL)
        newnode = node;
    else
        newnode = (struct table_trie_type *)
	    Malloc (31, sizeof( struct table_trie_type));

    assert (get_table_key (key, pos, &symbol, &symbols, &new_pos));
    newnode->Tkey_symbol = symbol;
    newnode->Tkey_symbols = symbols;
    newnode->Tkey = TXT_copy_text (key);
    newnode->Tkey_id = key_id;
    newnode->Tnext = NULL;
    newnode->Tdown = NULL;

    return (newnode);
}

struct table_trie_type *
copy_table_trie (struct table_trie_type *node)
/* Creates a new node by copying from an old one. */
{
    struct table_trie_type *newnode;

    assert( node != NULL );
    newnode = (struct table_trie_type *)
        Malloc (31, sizeof (struct table_trie_type));
    newnode->Tkey_symbol = node->Tkey_symbol;
    newnode->Tkey_symbols = TXT_copy_text (node->Tkey_symbols);
    newnode->Tkey = node->Tkey;
    newnode->Tkey_id = node->Tkey_id;
    newnode->Tnext = node->Tnext;
    newnode->Tdown = node->Tdown;
    return( newnode );
}

boolean
find_table_list (struct table_trie_type *head, unsigned int pos,
		 unsigned int key, struct table_trie_type **node,
		 unsigned int *new_pos)
/* Find the link that contains the symbol(s) at position pos in the
   key and return a pointer to it. Assumes the links are in ascending
   lexicographical order. If the symbol is not found, return a pointer
   to the previous link in the list. */
{
    struct table_trie_type *here, *there;
    unsigned int symbol, symbols;
    boolean found;
    int comp;

    if (!get_table_key (key, pos, &symbol, &symbols, new_pos))
        return (FALSE);

    found = FALSE;
    if (head == NULL)
        return (FALSE);
    here = head;
    there = NULL;
    while ((here != NULL) && (!found))
      {
        comp = compare_table_keys (symbol, symbols, here->Tkey_symbol,
				   here->Tkey_symbols);
	if (!comp)
	    found = TRUE;
	else if (comp < 0)
	    break;
        else
	  {
	    there = here;
	    here = here->Tnext;
	  }
      }
    if (!found) /* link already exists */
        *node = there;
    else
        *node = here;

    TXT_release_text (symbols);

    return (found);
}

struct table_trie_type *
insert_table_list (struct table_trie_type *head, struct table_trie_type *here,
		   unsigned int pos, unsigned int key, unsigned int key_id)
/* Insert new link after here and return it. Maintain the links in ascending
   lexicographical order. */
{
    struct table_trie_type *there, *newnode;

    assert( head != NULL );

    if (here == NULL) { /* at the head of the list */
        /* maintain head at the same node position by copying it */
        newnode = copy_table_trie (head);
	create_table_trie (head, pos, key, key_id);
	head->Tnext = newnode;
	return (head); /* the head is now the new node */
    }
    newnode = create_table_trie (NULL, pos, key, key_id);
    there = here->Tnext;
    if (there == NULL) /* at the tail of the list */
	here->Tnext = newnode;
    else { /* in the middle of the list */
	here->Tnext = newnode;
	newnode->Tnext = there;
    }
    return (newnode);
}

struct table_trie_type *
update_table_node
(struct table_type *table, struct table_trie_type *node,
 unsigned int pos, unsigned int key)
/* Adds the KEY into the NODE of the trie. If NODE is NULL, then creates and
   returns it. */
{
    struct table_trie_type *here, *pnode;
    unsigned int symbol, symbols, new_pos, new_pos1;

    assert (TXT_valid_text (key));
    if (node == NULL)
      {
	node = create_table_trie (NULL, pos, key, table->Types++);
	return (node);
      }

    if (!find_table_list (node, pos, key, &here, &new_pos))
    { /* Not in the list - insert the new key */
        node = insert_table_list (node, here, pos, key, table->Types++);
	return (node);
    }
    /* Found in the list - is it the same key? */
    if (here->Tkey != NIL)
      {
	if (!TXT_compare_text (key, here->Tkey))
	    return (here); /* key matches - just return a pointer */

	/* check if not past end of the key */
	if ((here->Tdown == NULL) && (new_pos < TXT_length_text (here->Tkey)))
	  { /* move old key one level down if needed */
	    node = copy_table_trie (here);
	    assert (get_table_key (here->Tkey, new_pos, &symbol, &symbols,
				   &new_pos1));
	    node->Tkey_symbol = symbol;
	    node->Tkey_symbols = symbols;
	    node->Tnext = NULL;
	    here->Tdown = node;
	    here->Tkey = NIL;
	    here->Tkey_id = 0;
	  }
      }
    if (new_pos >= TXT_length_text (key)) /* end of the key */
      {
	here->Tkey = TXT_copy_text (key);
	here->Tkey_id = table->Types++;
	return (here);
      }

    pnode = here->Tdown;
    node = update_table_node (table, pnode, new_pos, key);
    if (!pnode)
        here->Tdown = node;
    return( node );
}

struct table_trie_type *
find_table_node (struct table_trie_type *node, unsigned int pos,
		 unsigned int key)
/* Returns the node that contains the KEY in the trie if it exists. */
{
    struct table_trie_type *here;
    unsigned int new_pos;

    assert (TXT_valid_text (key));
    if (node == NULL)
        return (NULL);

    if (!find_table_list (node, pos, key, &here, &new_pos))
        return (NULL);

    /* Found in the list - is it the same key? */
    if ((here->Tkey != NIL) && !TXT_compare_text (key, here->Tkey))
        return (here); /* key matches - just return a pointer */

    if (here->Tdown == NULL)
	return (NULL);

    return (find_table_node (here->Tdown, new_pos, key));
}

boolean
TXT_valid_table (unsigned int table)
/* Returns non-zero if the table is valid, zero otherwize. */
{
    if (table == NIL)
        return (FALSE);
    else if (table >= Tables_unused)
        return (FALSE);
    else if (Tables [table].Types > Tables [table].Tokens)
        return (FALSE);
    /* The #types gets set to > #tokens when the model gets deleted;
       this way you can test to see if the table has been deleted or not */
    else
        return (TRUE);
}

unsigned int
TXT_create_table (unsigned int type, unsigned int types)
/* Creates and initializes a text table (for storing keys, their unique identification
   number and their frequency counts). */
{
    struct table_type *table;
    unsigned int t, old_size;

    if (Tables_used != NIL)
    {	/* use the first list of tables on the used list */
	t = Tables_used;
	Tables_used = Tables [t].Tnextused;
    }
    else
    {
	t = Tables_unused;
	if (Tables_unused+1 >= Tables_max_size)
	{ /* need to extend Tables array */
	    old_size = Tables_max_size * sizeof (struct table_type);
	    if (Tables_max_size == 0)
	      Tables_max_size = TABLES_ALLOC_SIZE;
	    else
	      Tables_max_size *= 2; /* Keep on doubling the array on demand */
	    Tables = (struct table_type *)
	        Realloc (34, Tables, Tables_max_size * sizeof
			 (struct table_type), old_size);

	    if (Tables == NULL)
	    {
		fprintf (stderr, "Fatal error: out of tables space\n");
		exit (1);
	    }
	}
	Tables_unused++;
    }

    if (t != NIL)
    {
      table = Tables + t;

      table->Ttype = type;
      table->Types = types;
      table->Tokens = types;
      table->Talloc = 0;
      table->Tnull_count = 0;
      table->Tnull_id = 0;
      table->Tcounts = NULL;
      table->Tkeys = NULL;
      table->Ttrie = NULL;
      table->Thead = NULL;
      table->Ttail = NULL;
    }

    return (t);
}

void
release_table_node (struct table_trie_type *node)
/* Releases the NODE in the trie to the free list. */
{
    struct table_trie_type *old_node;

    while (node != NULL)
      {
	old_node = node;
        TXT_release_text (node->Tkey);
	release_table_node (node->Tdown);
	node = node->Tnext;
	Free (31, old_node, sizeof (struct table_trie_type));
      }
}

void
TXT_release_table (unsigned int table)
/* Releases the memory allocated to the table and the table number (which may
   be reused in later TXT_create_table calls). */
{
    struct table_type *tablep;

    assert (TXT_valid_table (table));
    tablep = Tables + table;

    if (tablep->Tkeys != NULL)
        Free (33, tablep->Tkeys, tablep->Talloc * sizeof (unsigned int));
    if (tablep->Tcounts != NULL)
        Free (33, tablep->Tcounts, tablep->Talloc * sizeof (unsigned int));

    if (tablep->Ttrie != NULL)
        release_table_node (tablep->Ttrie);

    tablep->Types = 1; /* Used for testing later on if table no. is
			  valid or not */
    tablep->Tokens = 0; /* Used for testing later on if table no. is
			   valid or not */

    /* Append onto head of the used list */
    tablep->Tnextused = Tables_used;
    Tables_used = table;
}

void
TXT_insert_table (unsigned int table, unsigned int key, unsigned int key_id,
		  unsigned int key_increment)
/* Inserts the text key into the table and adds key_increment to its count. */
{
    struct table_type *tablep;
    struct table_trie_type *node, *pnode;
    /*unsigned int types;*/

    assert (TXT_valid_text (key));
    assert (TXT_valid_table (table));

    key = TXT_copy_text (key); /* Make a copy of the key first, and store
				  that in the table instead of the original */

    tablep = Tables + table;
    tablep->Tokens += key_increment;

    if (TXT_length_text (key) == 0)
      { /* we have a null key - to save a lot of hassle, treat it as a
	   special case */
	if (tablep->Tnull_count == 0)
	    tablep->Tnull_id = tablep->Types++;
	tablep->Tnull_count += key_increment;
	tablep->Tnull_id = key_id;
      }
    else
      {
	/* First update the trie */
	/*types = tablep->Types;*/
	pnode = tablep->Ttrie;
	node = update_table_node (tablep, tablep->Ttrie, 0, key);
	assert (node != NULL);
	if (pnode == NULL)
	    tablep->Ttrie = node;
	node->Tkey_id = key_id;
	key = node->Tkey;
      }

    /* add the key to the keys array */
    alloc_table_keys (tablep, key_id);

    assert (tablep->Tkeys [key_id] == NIL);
    tablep->Tkeys [key_id] = key;
    tablep->Tcounts [key_id] += key_increment;
}

boolean
TXT_update_table1 (unsigned int table, unsigned int key, unsigned int key_increment,
		   unsigned int *key_id, unsigned int *key_count)
/* Adds the text key to the table (if necessary), and returns TRUE if the key did not previously
   exist in the table, FALSE otherwise (in which case the existing key's frequency
   count will be incremented). Also sets the argument key_count to the resulting
   count and key_id to a unique identifier value associated with the key. These
   values start from 0 and are incremented by key_increment whenever a new key is added to
   the table. */
{
    struct table_type *tablep;
    struct table_trie_type *node, *pnode;
    unsigned int types, keyid;
    boolean added;

    assert (TXT_valid_text (key));
    assert (TXT_valid_table (table));

    tablep = Tables + table;

    assert (tablep->Ttype != TLM_Static); /* Can't add key to a static table */

    tablep->Tokens += key_increment;

    added = FALSE;
    if (TXT_length_text (key) == 0)
      { /* we have a null key - to save a lot of hassle, treat it as a special case */
	if (tablep->Tnull_count == 0)
	  {
	    added = TRUE;
	    tablep->Tnull_id = tablep->Types++;
	  }

	tablep->Tnull_count++;
	*key_count = tablep->Tnull_count;
	keyid = tablep->Tnull_id;
	*key_id = keyid;
      }
    else
      {
	/* First update the trie */
	types = tablep->Types;
	pnode = tablep->Ttrie;
	node = update_table_node (tablep, tablep->Ttrie, 0, key);
	assert (node != NULL);
	if (pnode == NULL)
	    tablep->Ttrie = node;
	keyid = node->Tkey_id;
	*key_id = keyid;
	assert (types <= tablep->Types);
	if (types != tablep->Types)
	  {
	    added = TRUE;
	    key = node->Tkey;
	  }
      }

    if (added)
      {
	assert (tablep->Types == keyid + 1);

	/* add the key to the keys array */
	alloc_table_keys (tablep, keyid);

	assert (tablep->Tkeys [keyid] == NIL);
	tablep->Tkeys [keyid] = key;
      }
    tablep->Tcounts [keyid] += key_increment;
    *key_count = tablep->Tcounts [keyid];

    return (added);
}

boolean
TXT_update_table (unsigned int table, unsigned int key, unsigned int *key_id, unsigned int *key_count)
/* Adds the text key to the table (if necessary), and returns TRUE if the key did not previously
   exist in the table, FALSE otherwise (in which case the existing key's frequency
   count will be incremented). Also sets the argument key_count to the resulting
   count and key_id to a unique identifier value associated with the key. These
   values start from 0 and are incremented by 1 whenever a new key is added to
   the table. */
{
  return (TXT_update_table1 (table, key, 1, key_id, key_count));
}

boolean
TXT_getid_table (unsigned int table, unsigned int key, unsigned int *key_id,
		 unsigned int *key_count)
/* Returns TRUE if the text key is found in the table, FALSE otherwise. Also
   sets the argument key_id to the key's unique identifier value and key_count
   to its frequency count. */
{
    struct table_type *tablep;
    struct table_trie_type *node;
    unsigned int keyid;

    assert (TXT_valid_text (key));
    assert (TXT_valid_table (table));
    tablep = Tables + table;

    if (TXT_null_text (key))
      if (tablep->Tnull_count)
	{
	  keyid = tablep->Tnull_id;
	  *key_id = keyid;
	  *key_count = tablep->Tnull_count;
	  return (TRUE);
	}
      else
	{
	  keyid = tablep->Types; /* set id to max. symbol */
	  *key_id = keyid;
	  *key_count = 0;
	  return (FALSE);
	}
    else
      { /* Not the null key */
	node = find_table_node (tablep->Ttrie, 0, key);
	if (node == NULL)
	  {
	    keyid = tablep->Types; /* set id to max. symbol */
	    *key_id = keyid;
	    *key_count = 0;
	    return (FALSE);
	  }
	else
	  {
	    keyid = node->Tkey_id;
	    *key_id = keyid;
	    *key_count = tablep->Tcounts [keyid];
	    return (TRUE);
	  }
      }
}

unsigned int
TXT_getkey_table (unsigned int table, unsigned int key_id)
/* Returns the text number of the text key associated with the identifier
   value key_id in the table. Returns NIL is the key_id value does not
   exist. */
{
    struct table_type *tablep;

    assert (TXT_valid_table (table));
    tablep = Tables + table;
    if (key_id >= tablep->Types)
        return (NIL);

    return (tablep->Tkeys [key_id]);
}

void
TXT_getinfo_table (unsigned int table, unsigned int *table_type,
		   unsigned int *types, unsigned int *tokens)
/* Returns table type (i.e. static or dynamic) and the number
   of types and tokens in it. */
{
    assert (TXT_valid_table (table));

    *table_type = Tables [table].Ttype;
    *types = Tables [table].Types;
    *tokens = Tables [table].Tokens;
}

struct table_stack_type *
alloc_table_stack (struct table_trie_type *tnode)
/* Allocates a pointer to a new stack record (or re-uses
   an old one if it exists). */
{
    struct table_stack_type *newnode;

    if (Used_table_stack == NULL)
        /* Create a new stack record */
	newnode = (struct table_stack_type *)
	    Malloc (32, sizeof (struct table_stack_type));
    else
      { /* Pop the head off the list of used stack records instead */
	newnode = Used_table_stack;
	Used_table_stack = newnode->Tnext;
      }

    newnode->Tnode = tnode;
    newnode->Tnext = NULL;
    return (newnode);
}

void
reset_table_stack (unsigned int table)
/* Resets the table stack back to empty */
{
    struct table_stack_type *head, *tail;

    head = Tables [table].Thead;
    if (head == NULL)
        return;
    tail = Tables [table].Ttail;

    /* Bung the entire table's stack onto the front of the used stack list */
    tail->Tnext = Used_table_stack;
    Used_table_stack = head;
}

void
push_table_stack (unsigned int table, struct table_trie_type *tnode)
/* Pushes the trie node tnode onto the stack. */
{
    struct table_stack_type *newnode;

    if (tnode == NULL) /* Don't add a null node to stack */
        return;

    newnode = alloc_table_stack (tnode);
    assert (newnode != NULL);

    /* Add at the head of the list: */
    newnode->Tnext = Tables [table].Thead;
    Tables [table].Thead = newnode;
    if (Tables [table].Ttail == NULL)
        Tables [table].Ttail = newnode;
}

void
dump_table_stack (unsigned int file, unsigned int table)
/* Dumps out the contents of the table's stack. */
{
    struct table_stack_type *tnode;

    assert (TXT_valid_file (file));

    tnode = Tables [table].Thead;

    while (tnode != NULL)
      {
	fprintf (Files [file], " %p", (void *) tnode->Tnode);
	tnode = tnode->Tnext;
      }
    fprintf (Files [file], "\n");
}

struct table_trie_type *
pop_table_stack (unsigned int table)
/* Pop the trie node off the top of the stack. */
{
    struct table_stack_type *head;
    /*struct table_stack_type *tail;*/
    struct table_trie_type *tnode;

    head = Tables [table].Thead;
    if (head == NULL)
        return (NULL);
    tnode = head->Tnode;
    /*tail = Tables [table].Ttail;*/ /* Not used - comment out due to compiler warning */

    /* Pop the record off the stack by cutting off the head */
    Tables [table].Thead = head->Tnext;

    /* Add the popped off record to the head of the used list */
    head->Tnext = Used_table_stack;
    Used_table_stack = head;

    return (tnode);
}

void
TXT_reset_table (unsigned int table)
/* Resets the current text key so that the next call to TXT_next_table
   will return the first key in the table. */
{
    struct table_type *tablep;

    assert (TXT_valid_table (table));
    tablep = Tables + table;

    reset_table_stack (table);
    push_table_stack (table, tablep->Ttrie);
}

boolean
TXT_next_table (unsigned int table, unsigned int *key, unsigned int *key_id,
		unsigned int *key_count)
/* Returns the next key in the table. Also sets the argument key_id to the
   key's unique identifier value and key_count to its frequency count. */
{
    struct table_type *tablep;
    struct table_trie_type *tnode;
    unsigned int keyid;

    assert (TXT_valid_table (table));
    tablep = Tables + table;

    /* go to next node node in the trie (i.e. the "leftmost") that has a key */
    for (;;) /* repeat */
      {
	tnode = pop_table_stack (table);
	/*
	fprintf (stderr, "stack after pop:\n");
	dump_table_stack (stderr, table);
	*/

	if (tnode == NULL)
	  break; /* no more keys */

	push_table_stack (table, tnode->Tnext);
	push_table_stack (table, tnode->Tdown);
	/*
	fprintf (stderr, "stack after pushes:\n");
	dump_table_stack (stderr, table);
	*/

	if (tnode->Tkey != NIL)
	    break; /* we have found a new key, so break */
      }

    if (tnode == NULL)
      {
	*key = NIL;
	*key_id = NIL;
	*key_count = 0;
        return (FALSE);
      }
    else
      {
	*key = tnode->Tkey;
	keyid = tnode->Tkey_id;
	*key_id = keyid;
	*key_count = tablep->Tcounts [keyid];
        return (TRUE);
      }
}

unsigned int
TXT_reset_tablepos (unsigned int table)
/* Resets the current table position. */
{
    assert (TXT_valid_table (table));
    return ((uintptr_t) Tables [table].Ttrie);
}

unsigned int
TXT_next_tablepos (unsigned int table, unsigned int tablepos)
/* Returns the next table position. */
{
    struct table_trie_type *tnode;

    assert (TXT_valid_table (table));

    if (tablepos == NIL)
        return (NIL);
    else
      {
        tnode = (struct table_trie_type *) (uintptr_t) tablepos;
        return ((uintptr_t) tnode->Tnext);
      }
}

unsigned int
TXT_expand_tablepos (unsigned int table, unsigned int tablepos)
/* Returns the expanded table position (by following the current
   position in the table out one level of the trie). */
{
    struct table_trie_type *tnode;

    assert (TXT_valid_table (table));

    if (tablepos == NIL)
        return (NIL);
    else
      {
        tnode = (struct table_trie_type *) (uintptr_t) tablepos;
        return ((uintptr_t) tnode->Tdown);
      }
}

void
TXT_get_tablepos (unsigned int table, unsigned int tablepos,
		  unsigned int *key, unsigned int *key_id,
		  unsigned int *key_count, unsigned int *key_symbol,
		  unsigned int *key_symbols)
/* Returns the expanded table position (by following the current
   position in the table out one level of the trie). */
{
    struct table_trie_type *tnode;
    unsigned int keyid;

    assert (TXT_valid_table (table));

    if (tablepos == NIL)
        tnode = NULL;
    else
        tnode = (struct table_trie_type *) (uintptr_t) tablepos;

    if (tnode == NULL)
      {
	*key = NIL;
	*key_id = NIL;
	*key_count = 0;
	*key_symbol = NIL;
	*key_symbols = NIL;
      }
    else
      {
	*key = tnode->Tkey;
	keyid = tnode->Tkey_id;
	*key_id = keyid;
	*key_count = Tables [table].Tcounts [keyid];
	*key_symbol = tnode->Tkey_symbol;
	*key_symbols = tnode->Tkey_symbols;
      }
}

void
dump_table_node (unsigned int file, unsigned int table, struct table_trie_type *node,
		 unsigned int level)
/* Dumps out the keys at the NODE in the trie. */
{
    assert (TXT_valid_file (file));

    while (node != NULL)
      {
	/*
	  fprintf (stderr, "level = %d symbol = %d key = %d id = %d\n",
	           level, node->Tkey_symbol, node->Tkey, node->Tkey_id);
	*/
        if (node->Tkey != NIL)
	  {
	    fprintf (Files [file], "id %5d key = {", node->Tkey_id);
	    TXT_dump_text (file, node->Tkey, TXT_dump_symbol);
	    fprintf (Files [file], "}\n");
	  }
	dump_table_node (file, table, node->Tdown, level+1);
	node = node->Tnext;
      }
}

void
TXT_dump_table (unsigned int file, unsigned int table)
/* Dumps out the text keys in the table. For debugging purposes. */
{
    struct table_type *tablep;
    unsigned int types, k;

    assert (TXT_valid_file (file));
    assert (TXT_valid_table (table));
    tablep = Tables + table;

    types = tablep->Types;
    if (tablep->Ttype == TLM_Dynamic)
        fprintf (Files [file], "Table is dynamic.\n");
    else
        fprintf (Files [file], "Table is static.\n");

    fprintf (Files [file], "Number of types  = %d\n", types);
    fprintf (Files [file], "Number of tokens = %lld\n", tablep->Tokens);

    fprintf (Files [file], "\nDump of table:\n");
    if (tablep->Tnull_count)
      fprintf (Files [file], "null id %5d null count = %d key = {} \n", tablep->Tnull_id,
	       tablep->Tnull_count);
    dump_table_node (file, table, tablep->Ttrie, 0);

    fprintf (Files [file], "\nDump of index:\n");
    for (k = 0; k < types; k++)
      {
	fprintf (Files [file], "id %5d count %5lld key = {", k, tablep->Tcounts [k]);
	if (!tablep->Tnull_count || (k != tablep->Tnull_id))
	    TXT_dump_text (file, tablep->Tkeys [k], TXT_dump_symbol);
	fprintf (Files [file], "}\n");
      }
}

void
TXT_dump_table_keys (unsigned int file, unsigned int table)
/* Dumps out the text keys in the table. For debugging purposes. */
{
    unsigned int key, key_id, key_count;

    TXT_reset_table (table);
    while (TXT_next_table (table, &key, &key_id, &key_count))
      {
	fprintf (Files [file], "key ");
	TXT_dump_text (file, key, TXT_dump_symbol);
	fprintf (Files [file], " = %d, count = %d\n", key_id, key_count);
      }
}

void
TXT_dump_table_keys1 (unsigned int file, unsigned int table)
/* Dumps out the text keys in the table. For debugging purposes. */
{
    unsigned int key, key_id, key_count;

    TXT_reset_table (table);
    while (TXT_next_table (table, &key, &key_id, &key_count))
      {
	fprintf (Files [file], "count %12d key ", key_count);
	TXT_dump_text (file, key, TXT_dump_symbol);
	fprintf (Files [file], "\n");
      }
}

void
TXT_dump_table_keys2 (unsigned int file, unsigned int table)
/* Dumps out the text keys in the table. For debugging purposes. */
{
    unsigned int key, key_id, key_count;

    TXT_reset_table (table);
    while (TXT_next_table (table, &key, &key_id, &key_count))
      {
	TXT_dump_text (file, key, TXT_dump_symbol);
	fprintf (Files [file], "\n");
      }
}

unsigned int
TXT_load_table_keys (unsigned int file)
/* Creates a new table and loads the text keys (one per line)
   from the file into it. */
{
    unsigned int table, key, key_id, key_count;

    assert (TXT_valid_file (file));

    key = TXT_create_text ();
    table = TXT_create_table (TLM_Dynamic, 0);

    while (TXT_readline_text (file, key) > 0)
    {
	TXT_update_table (table, key, &key_id, &key_count);
    }
    TXT_release_text (key);
    Tables [table].Ttype = TLM_Static;

    return (table);
}

unsigned int
TXT_load_table (unsigned int file)
/* Loads the table from the file. */
{
    unsigned int table, type, types, tokens, null_count, null_id;
    unsigned int key, key_id, key_count, k;

    assert (TXT_valid_file (file));

    key = TXT_create_text ();

    /* Read in the table type */
    type = fread_int (file, INT_SIZE);

    /* Read in the number of types */
    types = fread_int (file, INT_SIZE);

    table = TXT_create_table (type, types);

    /* Read in the number of tokens */
    tokens = fread_int (file, INT_SIZE);

    /* Read in the null count */
    null_count = fread_int (file, INT_SIZE);

    /* Read in the null id  */
    null_id = fread_int (file, INT_SIZE);
    if (null_count) /* add the null key if necessary */
	TXT_insert_table (table, key, null_id, null_count);

    /* Now read in each key */
    for (k = 0; k < types; k++)
      if (!null_count || (k != null_id))
      {
	/* read in the key id and count */
	key_id = fread_int (file, INT_SIZE);
	key_count = fread_int (file, INT_SIZE);

	/* read in the key itself */
	TXT_load_symbols (file, key);

	/*
	fprintf (stderr, "key id = %d count = %d\n", key_id, key_count);
	fprintf (stderr, "Symbols = ");
	TXT_dump_text (Stderr_File, key, NULL);
	fprintf (stderr, "\n");
	*/

	/* Now insert all the data into the table */
	TXT_insert_table (table, key, key_id, key_count);
      }

    Tables [table].Types = types;
    Tables [table].Tokens = tokens;
    Tables [table].Tnull_id = null_id;
    Tables [table].Tnull_count = null_count;

    TXT_release_text (key);

    return (table);
}

void
TXT_write_table (unsigned int file, unsigned int table, unsigned int type)
/* Writes out the text keys in the table to a file which can
   then be latter reloaded using TXT_load_table.  */
{
    struct table_type *tablep;
    unsigned int types, null_count, null_id, k;

    assert (TXT_valid_file (file));
    assert (TXT_valid_table (table));

    tablep = Tables + table;

    /* Write out the table type  */
    fwrite_int (file, type, INT_SIZE);

    /* Write out the number of types  */
    types = tablep->Types;
    fwrite_int (file, types, INT_SIZE);

    /* Write out the number of tokens */
    fwrite_int (file, tablep->Tokens, INT_SIZE);

    /* Write out the null count */
    null_count = tablep->Tnull_count;
    fwrite_int (file, null_count, INT_SIZE);

    /* Write out the id for the null key */
    null_id = tablep->Tnull_id;
    fwrite_int (file, null_id, INT_SIZE);

    /* Write out the table's keys and counts */

    /* Now write out each key */
    for (k = 0; k < types; k++)
      if (!null_count || (k != null_id))
      {
	/* write out the key id and count */
	fwrite_int (file, k, INT_SIZE);
	fwrite_int (file, tablep->Tcounts [k], INT_SIZE);

	/* read in the key itself */
	TXT_write_symbols (file, tablep->Tkeys [k]);

	/*
	fprintf (stderr, "key id = %d count = %d\n", k, tablep->Tcounts [k]);
	fprintf (stderr, "Symbols = ");
	TXT_dump_text (Stderr_File, tablep->Tkeys [k], NULL);
	fprintf (stderr, "\n");
	*/
      }
}

void
TXT_suspend_update_table (unsigned int table)
/* Suspends the update for a dynamic table temporarily.
   The update can be resumed using TXT_resume_update_table ().

   This is useful if it needs to be determined in advance which
   of two or more dynamic tables a sequence of text should be
   added to (based on how much each requires to encode it, say). */
{
    assert (TXT_valid_table (table));
    assert (Tables [table].Ttype == TLM_Dynamic);

    Tables [table].Ttype = TLM_Static;
}

void
TXT_resume_update_table (unsigned int table)
/* Resumes the update for a table. See TXT_suspend_update_table (). */
{
    assert (TXT_valid_table (table));
    assert (Tables [table].Ttype == TLM_Static);

    Tables [table].Ttype = TLM_Dynamic;
}

/* Scaffolding for table module.

int
main()
{
    unsigned int table, key, key_id, key_count;

    key = TXT_create_text ();
    table = TXT_create_table (TLM_Dynamic, 0);

    while (TXT_readline_text (stdin, key) > 0)
    {
	TXT_update_table (table, key, &key_id, &key_count);
	printf ("Id number for key ");
	TXT_dump_text (stdout, key, TXT_dump_symbol);
	printf ( " = %d, count = %d\n", key_id, key_count);
	printf ("\nDump of table:\n\n");
	TXT_dump_table (stdout, table);
    }
}
*/
