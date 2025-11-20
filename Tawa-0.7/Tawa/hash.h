/* Hash module definitions used by both the encoder and the decoder. */

#include "model.h"

extern unsigned int Hash_Malloc;

struct hashpType
{   unsigned int H_transform_model; /* The transform model identifier number */
    unsigned int H_language_model;  /* The language model identifier number */
    unsigned int H_input_position;  /* The input position */
    int H_context_position;         /* The hashed context position in the trie
				       model */
    float H_total_codelength;       /* The min. tot. codelength assocociated
				       with this context path */
    float H_symbol_codelength;      /* The codelength associated with encoding
				       the symbol. */
    struct leafType *H_leaf;        /* The leaf associated with this context */
    struct hashpType *H_next;       /* The next entry in this list */
};

struct hashmType
{   unsigned int H_transform_model; /* The transform model identifier number */
    unsigned int H_transform_type;  /* The transform type for this lang. model */
    unsigned int H_language_model;  /* The language model identifier number */
    unsigned int H_input_position;  /* The input position */
    unsigned int H_context;         /* The current context */
    unsigned int H_sentinel_context;/* The sentinel context */
    float H_total_codelength;       /* The min. tot. codelength assocociated
				       with this context path */
    float H_symbol_codelength;      /* The codelength associated with encoding
				       the symbol. */
    float H_sentinel_codelength;    /* The codelength associated with encoding
				       the sentinel symbol. */
    struct hashmType *H_next;       /* The next entry in this list */
};

void
dumpHashp (unsigned int file, unsigned int transform_model);

unsigned int
countHashp (unsigned int transform_model);

void
initHashp (unsigned int transform_model);

struct hashpType *
addHashp (unsigned int transform_model, unsigned int language_model,
	  unsigned int input_position, int context_position,
	  float total_codelength, float symbol_codelength,
	  boolean *update, boolean *added);
/* Adds the tuple [transform_model, language_model, input position,
   context position] into the hashp table. Returns non-zero update if the
   position is new or updated; and non-zero added if the position is new. */

void
dumpHashm (unsigned int file, unsigned int transform_model);

unsigned int
countHashm (unsigned int transform_model);
/* Returns the count of the number of entries in the hash
   table. */

void
releaseHashm (unsigned int transform_model);
/* Releases the hashm table to memory for latter re-use. */

void
initHashm (unsigned int transform_model);

struct hashmType *
findHashm (unsigned int transform_model, unsigned int language_model);
/* Finds and returns the position of the tuple [transform_model, language_model]
   in the hashm table. */

struct hashmType *
updateHashm (unsigned int transform_model, unsigned int language_model,
	     unsigned int source_pos, unsigned int source_symbol);
/* Updates and returns the hashm entry for the tuple [transform_model,
   language_model]. */

void
startHashm (unsigned int transform_model, unsigned int transform_type,
	    unsigned int language_model);
/* Starts a new hashm entry for the tuple [transform_model, language_model]. */

