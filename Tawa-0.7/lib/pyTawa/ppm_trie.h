/* Trie routines for PPM models. */

#ifndef PPM_TRIE_H
#define PPM_TRIE_H

#define TRIE_ROOT_NODE 1          /* Root node of trie model */

/* Define width (i.e. number of bytes) for various data structures in the model (i.e. trie and slist nodes) */
#define TRIE_NODE_SWIDTH 1         /* there is one count in each static node (for static models only) */
#define TRIE_NODE_DWIDTH 2         /* there is one count plus ptr to slist in each node (for dynamic models only) */
#define TRIE_SLIST_SWIDTH 2        /* there are two fields per symbol in the static symbol list */
#define TRIE_SLIST_DWIDTH 3        /* there are three fields per symbol in the dynamic symbol list */

/* NOTE: this structure could have been implemented using counts associated with each node in the
   symbol list rather than the count associated with the trie node. i.e. This could save for leaf
   nodes. However, negative child pointers assume a count of 1, thus saving on each case where
   this occurs. TODO: can still save on leaf ptrs, by checking for this eventuality i.e. reached
   > max_depth. */

/* Define offsets for various fields in the model */
/* The following offsets are for fields in each "node" of the trie */
#define TRIE_TCOUNT_OFFSET 0       /* offset to node's count */
#define TRIE_SLIST_OFFSET 1        /* offset to list of symbols for node; for the static model,
				      the list starts from this point; for the dynamic model,
				      a ptr to the start of the list is stored (since its
				      position may change as the list grows) */

/* The following offsets are for fields in each "slist node" in the symbol list */
#define TRIE_SYMBOL_OFFSET 0       /* offset to symbol */
#define TRIE_CHILD_OFFSET 1        /* offset to pointer to one level down in the trie for symbol */
#define TRIE_NEXT_SLIST_OFFSET 2   /* offset to next node in the slist (for dynamic lists only) */


struct PPM_trieType
{ /* the data structure for the trie model */
    int *T_nodes;                 /* Nodes in the trie model */
    unsigned int T_form;          /* Either static or dynamic */
    unsigned int T_size;          /* Current maximum size of the trie model */ 

    /* The remaining fields are required only for the dynamic context trie. (Note that the input text
       is being considered as being part of the "context" trie, that's why it's included here) */ 

    unsigned int T_unused;        /* Next unused (i.e. never allocated) trie node  */

    unsigned int *T_input;        /* The input text */
    unsigned int T_input_size;    /* Current maximum size of the input array */
    unsigned int T_input_len;     /* Current length of input */
}; /* structure for storing the trie */

extern unsigned int PPM_Trie_malloc_slist; /* How much slist records have been allocated */

void
PPM_dump_symbol (unsigned int file, unsigned int symbol,
		 void (*dump_symbol_function) (unsigned int, unsigned int));
/* Dump the symbol */

void
PPM_dump_string (unsigned int file, unsigned int *str,
		 unsigned int pos, unsigned int len,
		 void (*dump_symbol_function) (unsigned int, unsigned int));
/* Dump the string STR starting at position POS. */

void
PPM_dump_input (unsigned int file, struct PPM_trieType *trie, void (*dump_symbol_function) (unsigned int, unsigned int));
/* Dump the input array for trie */

void
PPM_init_input (struct PPM_trieType *trie);
/* Initialize the input array for trie. */

unsigned int
PPM_update_input (struct PPM_trieType *trie, unsigned int symbol);
/* Insert the symbol into the input at the next input position. */

unsigned int
PPM_allocate_trie_node (struct PPM_trieType *trie, unsigned int size);
/* Allocates a node from the nodes array */

void
PPM_get_slist (struct PPM_trieType *trie, unsigned int sptr,
	       unsigned int *sym, int *child, unsigned int *next_sptr);
/* Returns the symbol, child and next symbol pointer associated with the sptr
   into the node's slist. */

void
PPM_put_slist (struct PPM_trieType *trie, unsigned int sptr, int child);
/* Inserts the child pointer associated with the sptr into the node's slist. */

unsigned int
PPM_find_slist (struct PPM_trieType *trie, unsigned int shead, unsigned int sym, 
		int *child, unsigned int *prev_sptr);
/* Returns the ptr to slist node containing sym if one exists; NIL otherwise. Also returns the slist node's child
   and a ptr to the previous slist node in the list. The trie is assumed to be dynamic. */

unsigned int
PPM_next_slist (struct PPM_trieType *trie, unsigned int sptr);
/* Returns a ptr to the next symbol in the node's slist. */

void
PPM_add_slist (struct PPM_trieType *trie, unsigned int node, unsigned int stail,
	       unsigned int symbol, int child);
/* Add a new symbol to the node's slist after the tail of the list. */

void
PPM_get_trie_node (struct PPM_trieType *trie, unsigned int node, unsigned int *tcount,
		   unsigned int *shead);
/* Returns the trie count tcount and shead (head of the symbol list)  associated with the node. */

unsigned int
PPM_get_trie_count (struct PPM_trieType *trie, int node, int child,
		unsigned int sptr, unsigned int next_sptr, unsigned int sym);
/* Gets the count for the child node at the node in the trie. */

unsigned int
PPM_create_trie_node (struct PPM_trieType *trie);
/* Allocate a new node of type node_type and return a pointer to it. */

void
PPM_increment_trie_node (struct PPM_trieType *trie, unsigned int node,
			 unsigned int escape_method);
/* Increments the total count for the node in the trie (for dynamic tries only). */

int
PPM_find_trie_node (struct PPM_trieType *trie, unsigned int node,
		    unsigned int symbol);
/* Find and return the symbol list position for symbol in the trie node. */

unsigned int
PPM_extend_trie_node (struct PPM_trieType *trie, unsigned int node);
/* Extend node to make room for one extra symbol in the symbol list. Return
   a pointer to the newnode. */

void
PPM_update_trie_node (struct PPM_trieType *trie, unsigned int node, unsigned int symbol);
/* Update the trie node. */

struct PPM_trieType *
PPM_create_trie (unsigned int type);
/* Creates a trie of type "type" (i.e. either TLM_Static or TLM_Dynamic). */ 

void
PPM_release_trie (struct PPM_trieType *trie);
/* Releases the memory used by the trie. */

struct PPM_trieType *
PPM_copy_trie (struct PPM_trieType *trie);
/* Copies the trie. */

void
PPM_dump_trie (unsigned int file, struct PPM_trieType *trie, int max_depth, void (*dump_symbol_function) (unsigned int, unsigned int));
/* Dumps the trie. */

void
PPM_stats_trie (unsigned int file, struct PPM_trieType *trie, int max_depth);
/* Produces stats for the trie */

struct PPM_trieType *
PPM_build_static_trie (struct PPM_trieType *trie, int max_depth);
/* Builds the static model from the dynamic model and returns a pointer to it. */

void
PPM_build_compressed_input (struct PPM_trieType *trie, int max_depth);
/* Builds the compressed input from the dynamic trie by deleting all input symbols that are no longer
   pointed to by it. */

#endif

