/* Context routines based on PPM models. */

#ifndef SSS_CONTEXT_H
#define SSS_CONTEXT_H

boolean
SSS_valid_context (unsigned int context);
/* Returns non-zero if the SSS context is valid, zero otherwize. */

struct SSS_contextType
{ /* Record of current context */
    unsigned int S_symbol;            /* Current symbol being encoded */
    /* The codelengths and coderanges are calculated directly each time */
};

#define S_next S_symbol /* S_symbol is also used to store next link on used
			   list when this record gets deleted */

extern struct SSS_contextType *SSS_Contexts; /* List of context records */
extern unsigned int SSS_Contexts_max_size;   /* Current max. size of the SSS_Contexts array */
extern unsigned int SSS_Contexts_used;       /* List of deleted context records */
extern unsigned int SSS_Contexts_unused;     /* Next unused context record */

unsigned int
SSS_create_context1 (void);
/* Return a new pointer to a context record. */

void
SSS_copy_context1 (unsigned int model, unsigned int context,
		   unsigned int new_context);
/* Copies the contents of the specified context into the new context. */

#endif
