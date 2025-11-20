/* Context routines based on PPM models. */

#ifndef PPM_CONTEXT_H
#define PPM_CONTEXT_H

#include "bits.h"
#include "ptable.h"
#include "model.h"

boolean
PPM_valid_context (unsigned int context);
/* Returns non-zero if the PPM context is valid, zero otherwize. */

struct PPM_positionType
{ /* Temporary data associated with the current position in the context. */
    int P_node;                      /* Current node in the trie for this
					position */

    unsigned int P_symbol;           /* Current symbol being encoded */
    unsigned int P_count;            /* This symbol's count */
    unsigned int P_sptr;             /* Ptr to next location in symbol list */

    unsigned int P_total;            /* Total of all frequencies for symbol
					list */
    unsigned int P_subtotal;         /* Sub-total of previous symbols in
					 symbol list */

    unsigned int P_target;           /* Target for decoding */
    unsigned int P_coderanges;       /* List of arithmetic coding ranges
					required for encoding the symbol */

    float P_codelength;              /* Total codelength for encoding the
					symbol */
    float P_escape_codelength;       /* Total codelength of escapes for
					encoding the symbol */

    bits_type *P_exclusions;         /* For computing exclusions for each
					symbol */
};

struct PPM_contextType
{ /* Record of current context */
    int C_node;                      /* The parent node in the trie */

    /* Data needed to maintain track of current context position. */
    struct PPM_positionType *C_position;

    unsigned int C_suffixptr;        /* Current position in the list of suffixes */
    unsigned int *C_suffixlist;      /* The list of suffixes */

};

#define C_next C_node                      /* C_node is also used to store next link on used list when this record gets deleted */

extern struct PPM_contextType *PPM_Contexts; /* List of context records */
extern unsigned int PPM_Contexts_max_size;   /* Current max. size of the PPM_Contexts array */
extern unsigned int PPM_Contexts_used;       /* List of deleted context records */
extern unsigned int PPM_Contexts_unused;     /* Next unused context record */

/* Temporary storage used for maintaining context positions */
extern struct PPM_positionType PPM_Context_Position;

unsigned int
PPM_get_max_order (unsigned int model);
/* Returns the max. order of the PPM model. */

void
PPM_validate_symbol (unsigned int context, unsigned int symbol,
		     struct PPM_positionType *position);
/* Validates the symbol and the context for updating the context's position. */

void
PPM_create_suffixlist (unsigned int model, unsigned int context);
/* Creates a suffix list record. */

unsigned int
PPM_length_suffixlist (unsigned int model, unsigned int context);
/* Returns the length of the suffix list for context. */

void
PPM_behead_suffixlist (unsigned int model, unsigned int context);
/* Release all but the tail of the suffixes list to the used list. */

void
PPM_release_suffixlist (unsigned int model, unsigned int context);
/* Release the suffixes list to the used list */

void
PPM_dump_suffixlist (unsigned int file, unsigned int model, unsigned int context);
/* Dumps the suffix list for context. */

void
PPM_init_suffixlist (unsigned int model, unsigned int context);
/* Initializes the context's suffix list */

void
PPM_reset_suffixlist (unsigned int model, unsigned int context);
/* Reset the context to start at the head of the suffix list. */

void
PPM_start_suffix (unsigned int model, unsigned int context);
/* Append a new suffix to the suffix list that points to the TRIE_ROOT_NODE of the trie. */

void
PPM_drop_suffix (unsigned int model, unsigned int context, unsigned int suffixptr);
/* Drop the suffixptr at the head of the context's suffix list. */

unsigned int
PPM_next_suffix (unsigned int model, unsigned int context);
/* Return (and move along to) the node pointed to by the next suffix in the context's suffix list. */

void
PPM_copy_suffixlist (unsigned int model, unsigned int new_context,
		 unsigned int context);
/* Copy the context's suffix list to the new context's suffix list. */

void
PPM_release_position (struct PPM_positionType *position);
/* Creates a new position record if position is not NULL and returns a pointer to it. */

struct PPM_positionType *
PPM_copy_position (struct PPM_positionType *position);
/* Copies the position record and returns a pointer to the copy. */

void
PPM_encode_position (unsigned int model, codingType coding_type, unsigned int coder,
		     int order, struct PPM_positionType *position);
/* Encodes the arithmetic coding range based on the current position
   and order using the codingType and coder. */

struct PPM_positionType *
PPM_start_position (unsigned int model, unsigned int context,
		    operType oper_type, codingType coding_type,
		    unsigned int coder, struct PPM_positionType *position);
/* Starts off a new context with a new position record, creating it if
   doesn't already exist and returning a pointer to it. */

void
PPM_reset_position (unsigned int model, unsigned int context,
		    operType oper_type, codingType coding_type,
		    unsigned int coder, int node,
		    struct PPM_positionType *position);
/* Resets various positional information according to the specified node in
   the context's trie. */

boolean
PPM_find_position (unsigned int model, unsigned int context,
		   operType oper_type, codingType coder_type,
		   unsigned int coder, struct PPM_positionType *position);
/* Moves along to the requested symbol or target in the context, updating all
   the ncessary information and returning all the data necessary to find the
   arithmetic coding range. Returns TRUE if the symbol or target has been
   found. */

unsigned int
PPM_create_context1 (void);
/* Return a new pointer to a context record. */

void
PPM_copy_context1 (unsigned int model, unsigned int context,
		  unsigned int new_context);
/* Copies the contents of the specified context into the new context. */

#endif
