/* Routines for encoding Start-Stop-Step codes. (See Bell, Cleary, Witten
   1990, "Text Compression", page 294).

   A (start, step, stop) unary code of the integers is defined as
   follows:  The Nth codeword has N ones followed by a zero followed by
   a field of size START + (N * STEP).  If the field width is equal to
   STOP then the preceding zero can be omitted.  The integers are laid
   out sequentially through these codewords.
*/


#include "io.h"

#ifndef SSS_MODEL_H
#define SSS_MODEL_H

struct SSS_modelType
{ /* SSS model record */
  unsigned int S_start;  /* The start parameter for the SSS code */
  unsigned int S_step;   /* The step parameter for the SSS code */
  unsigned int S_stop;   /* The stop parameter for the SSS code */
  unsigned int S_next;   /* Used when the record gets deleted */
};

/* Global variables used for storing the SSS models */
extern struct SSS_modelType *SSS_Models;/* List of SSS models */

boolean
SSS_valid_model (unsigned int sss_model);
/* Returns non-zero if the SSS model is valid, zero otherwize. */

unsigned int
SSS_create_model (unsigned int start, unsigned int step, unsigned int stop);
/* Creates and returns a new pointer to a SSS model record. */

void
SSS_get_model (unsigned int sss_model, unsigned int *start,
	       unsigned int *step, unsigned int *stop);
/* Returns information about the SSS model. The arguments start, step and
   stop were used to create the model in SSS_create_model(). */

void
SSS_release_model (unsigned int sss_model);
/* Releases the memory allocated to the model and the model number (which may
   be reused in later SSS_create_model or SSS_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active SSS_Contexts pointing at it. */

void
SSS_nullify_model (unsigned int model);
/* Replaces the model with the null model and releases the memory allocated
   to it. */

unsigned int
SSS_load_model (unsigned int file);
/* Loads the SSS model which has been previously saved to the file into memory
   and allocates it a new model number which is returned. */

void
SSS_write_model (unsigned int file, unsigned int sss_model);
/* Writes out the SSS model to the file (which can then be loaded
   by other applications later). */

void
SSS_dump_model (unsigned int file, unsigned int sss_model);
/* Dumps out the SSS model (for debugging purposes). */

boolean
SSS_valid_context (unsigned int context);
/* Returns non-zero if the SSS context is valid, zero otherwize. */

unsigned int
SSS_create_context (unsigned int model);
/* Creates and returns an unsigned integer which provides a reference to a SSS
   context record associated with the model's context. The current position is
   set to the null string. The current symbol is set to the first predicted
   symbol. */

unsigned int
SSS_copy_context (unsigned int model, unsigned int context);
/* Creates a new SSS context record, copies the contents of the specified
   context into it, and returns an integer reference to it. A run-time error
   occurs if the SSS context being copied is for a dynamic model. */

void
SSS_overlay_context (unsigned int model, unsigned int old_context,
		     unsigned int context);
/* Overlays the SSS context by copying the old context into it. This will
   copy the context even for dynamic models. This is necessary when dynamic
   models with supplementary symbols are being created. */

void
SSS_find_symbol (unsigned int model, unsigned int context,
		 unsigned int symbol);
/* Finds the predicted symbol in the SSS context. */

void
SSS_update_context (unsigned int model, unsigned int context,
		    unsigned int symbol);
/* Updates the SSS context record so that the current symbol becomes symbol.
   Returns additional information as specified by the routine
   TLM_set_context_type. */

void
SSS_release_context (unsigned int model, unsigned int context);
/* Releases the memory allocated to the SSS context and the context number
   (which may be reused in later SSS_create_context or SSS_copy_context and
   TLM_copy_dynamic_context calls). */

void
SSS_reset_symbol (unsigned int model, unsigned int context);
/* Resets the SSS context record to point at the first predicted symbol of the
   current position. */

boolean
SSS_next_symbol (unsigned int model, unsigned int context,
		 unsigned int *symbol);
/* Returns the next predicted symbol in the SSS context and the cost in bits of
   encoding it. The context record is not updated.

   If a sequence of calls to SSS_next_symbol are made, every symbol in the
   alphabet will be visited exactly once although the order in which they are
   visited is undefined being implementation and data dependent. The function
   returns FALSE when there are no more symbols to process. TLM_reset_symbol
   will reset the current position to point back at the first predicted symbol
   of the current context.

   The codelength value is the same as that returned by TLM_update_context
   which may use a faster search method to find the symbol's codelength
   more directly (rather than sequentially as TLM_next_symbol does). A call
   to TLM_update_context or other routines will have no affect on subsequent
   calls to TLM_next_symbol. */

void
SSS_encode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder, unsigned int symbol);
/* Encodes the specified symbol using the arithmetic coder.
   Updates the context record so that the last symbol in the
   SSS context becomes the encoded symbol. */

unsigned int
SSS_decode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder);
/* Returns the symbol decoded using the arithmetic coder. Updates the
   SSS context record so that the last symbol in the context becomes the
   decoded symbol. */

unsigned int
SSS_getcontext_position (unsigned int model, unsigned int context);
/* Returns an integer which uniquely identifies the current position
   associated with the SSS context. (One implementation is to return a memory
   location corresponding to the current position. This routine is useful if
   you need to check whether different contexts have encoded the same prior
   symbols as when checking whether the context pathways converge in the
   Viterbi or trellis-based algorithms.) */

unsigned int
SSS_sizeof_model (unsigned int model);
/* Returns the current number of bits needed to store the
   model in memory. */

void
SSS_stats_model (unsigned int file, unsigned int model);
/* Writes out statistics about the SSS model in human readable form. */

#endif
