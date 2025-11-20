/* TLM routines based on CPT models. */

#include "cpt_ptable.h"
#include "cpt_ctable.h"

#ifndef CPT_MODEL_H
#define CPT_MODEL_H

struct CPT_modelType
{ /* CPT model record */
    struct CPTp_table_type *CPT_ptable;  /* Probability table used when context is NIL */
    struct CPTc_table_type *CPT_ctable;  /* Probability table used when context is non-NIL */
    boolean CPT_deleted;                /* Set when record has been deleted) */
    unsigned int CPT_next;              /* Next in unused list (used for deleting) */
};

/* Global variables used for storing the CPT models */
extern struct CPT_modelType *CPT_Models;/* List of CPT models */

boolean
CPT_valid_model (unsigned int pt_model);
/* Returns non-zero if the CPT model is valid, zero otherwize. */

unsigned int
CPT_create_model (void);
/* Creates and returns a new pointer to a CPT model record. */

void
CPT_get_model (unsigned int pt_model, unsigned int *alphabet_size,
		int *max_order);
/* Returns information about the CPT model. The arguments alphabet_size,
   and max_order are values used to create the model in CPT_create_model(). */

void
CPT_release_model (unsigned int pt_model);
/* Releases the memory allocated to the model and the model number (which may
   be reused in later CPT_create_model or CPT_load_model calls). */

void
CPT_write_model (unsigned int file, unsigned int model,
		unsigned int model_form);
/* Writes out the CPT model to the file. */

void
CPT_dump_model (unsigned int file, unsigned int pt_model);
/* Dumps out the CPT model (for debugging purposes). */

void
CPT_update_context (unsigned int model, unsigned int context,
		   unsigned int symbol);
/* Updates the CPT context record so that the current symbol becomes symbol_text. */

#endif
