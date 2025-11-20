/* TLM routines based on PT models. */

#include "pt_ptable.h"
#include "pt_ctable.h"

#ifndef PT_MODEL_H
#define PT_MODEL_H

struct PT_modelType
{ /* PT model record */
    struct PTc_table_type *PT_table;       /* Probability table; context must be non-NIL */
    boolean PT_deleted;                    /* Set when record has been deleted) */
    unsigned int PT_next;                  /* Next in unused list (used for deleting) */
};

/* Global variables used for storing the PT models */
extern struct PT_modelType *PT_Models;     /* List of PT models */
extern struct PT_contextType *PT_Contexts; /* List of PT contexts */

extern unsigned int PT_Novel_Symbols;      /* Used to incidate when new symbols are novel */

boolean
PT_valid_model (unsigned int pt_model);
/* Returns non-zero if the PT model is valid, zero otherwize. */

unsigned int
PT_create_model (void);
/* Creates and returns a new pointer to a PT model record. */

void
PT_release_model (unsigned int pt_model);
/* Releases the memory allocated to the model and the model number (which may
   be reused in later PT_create_model or PT_load_model calls). */

unsigned int
PT_load_model (unsigned int file, unsigned int model_form);
/* Loads the PT model which has been previously saved to the file into memory
   and allocates it a new model number which is returned. */

void
PT_write_model (unsigned int file, unsigned int model,
		unsigned int model_form);
/* Writes out the PT model to the file. */

void
PT_dump_model (unsigned int file, unsigned int pt_model);
/* Dumps out the PT model (for debugging purposes). */

void
PT_find_symbol (unsigned int model, unsigned int context_text,
		unsigned int symbols_text);
/* Finds the codelength for encoding the symbols_text in the
   PT context record. symbols_text is assumed to be a text record
   i.e. a sequence of text symbols.
   Returns additional information as specified by the routine
   TLM_set_context_operation. */

void
PT_update_context (unsigned int model, unsigned int context,
		   unsigned int symbols);
/* Updates the PT context record for the current symbols_text.
   symbols_text is assumed to be a text record i.e. a sequence of text symbols.
   Returns additional information as specified by the routine
   TLM_set_context_operation. */

void
PT_update_decode_context (unsigned int model, unsigned int symbols_text);
/* Performs all the updates that were deferred during decoding
   that could not be performed until the symbols_text has been
   decoded. */

void
PT_encode_symbol (unsigned int model, unsigned int context_text,
		  unsigned int coder, unsigned int symbols_text);
/* Encodes & updates the PT context record for the current symbols_text.
   symbols_text is assumed to be a text record i.e. a sequence of text symbols. */

unsigned int
PT_decode_symbol (unsigned int model, unsigned int context_text,
		  unsigned int coder);
/* Decodes & updates the PT context record for the current symbols_text.
   symbols_text is assumed to be a text record i.e. a sequence of text symbols. */

#endif
