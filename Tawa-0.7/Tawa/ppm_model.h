/* TLM routines based on PPM models. */

#include "ppm_trie.h"
#include "ptable.h"
#include "io.h"

#ifndef PPM_MODEL_H
#define PPM_MODEL_H

/* Types of data to be returned by PPM_get_model (). */
#define PPM_Get_Alphabet_Size 0         /* Gets the alphabet size of the PPM model */
#define PPM_Get_Max_Symbol 1            /* Gets the maximum symbol number of the PPM model */
#define PPM_Get_Max_Order 2             /* Gets the maximum order of the PPM model */
#define PPM_Get_Escape_Method 3         /* Gets the escape method of the PPM model */
#define PPM_Get_Performs_Full_Excls 4   /* Returns true if the PPM model performs full exclusions */
#define PPM_Get_Performs_Update_Excls 5 /* Returns true if the PPM model performs update exclusions */

/* Types of data to be set/reset by PPM_set_model (). */
#define PPM_Set_Alphabet_Size 0       /* Sets the alphabet size of the PPM model */
#define PPM_Set_Max_Symbol 1          /* Resets the maximum symbol number of the PPM model */

struct PPM_modelType
{ /* PPM model record */
  unsigned int P_alphabet_size;    /* The number of symbols in the alphabet;
				      0 means an unbounded sized alphabet (which
				      incrementally increases as each new
				      symbol is added */
  unsigned int P_max_symbol;       /* Current max. symbol number for unbounded
				      sized alphabets or max. non-static symbol
				      for other alphabets */
  int P_max_order;                 /* The max. order of the model */
  unsigned int P_escape_method;    /* The escape method of the model */
  boolean P_performs_full_excls;   /* Indicates whether model performs full exclusions or not */
  boolean P_performs_update_excls; /* Indicates whether model performs update exclusions or not */
  struct PPM_trieType *P_trie;     /* The PPM trie model */
  ptable_type *P_ptable;           /* Cumulative probability for order 0 context,
				      used for unbounded alphabets only */
  unsigned int P_next;             /* Next in deleted list of records list */
};

/* Global variables used for storing the PPM models */
extern struct PPM_modelType *PPM_Models;/* List of PPM models */

boolean
PPM_valid_model (unsigned int ppm_model);
/* Returns non-zero if the PPM model is valid, zero otherwize. */

unsigned int
PPM_create_model (unsigned int alphabet_size, int max_order,
		  unsigned int escape_method, boolean performs_full_excls,
		  boolean performs_update_excls);
/* Creates and returns a new pointer to a PPM model record.

   The model_type argument specified the type of model to be created e.g.
   TLM_PPM_Model or TLM_PCFG_Model. It is followed by a variable number of
   parameters used to hold model information which differs between
   implementations of the different model types. For example, the
   the maximum order of the PPM model, the escape method (A, B, C or D -
   as specified by the constants TLM_PPM_Method_A etc.) and whether the model
   should perform update exclusions.

   The alphabet_size argument specifies the number of symbols permitted in
   the alphabet (all symbols for this model must have values from 0 to one
   less than alphabet_size). An alphabet_size of 0 specifies that the
   alphabet is unbounded. (This is useful for word-based alphabets, for
   example). In this case, allowable symbol numbers range from 0 up to a
   special "expand_alphabet" symbol which is equal to the current maximum
   symbol number (this is one more than the highest previously seen symbol
   number). If the current symbol becomes the expand_alphabet symbol, then
   the current maximum symbol number is incremented by 1, thus effectively
   expanding the size of the alphabet by 1. The current maximum symbol number
   may be obtained by calling the routine TLM_get_model. One further
   symbol is permitted in the current alphabet - the sentinel symbol.
*/

void
PPM_get_model (unsigned int model, unsigned int type, unsigned int *value);
/* Returns information about the PPM model. The arguments type is
   the information to be returned (i.e. PPM_Get_Alphabet_Size,
   PPM_Get_Max_Symbol, PPM_Get_Max_Order, PPM_Get_Escape_Method,
   PPM_Get_Performs_Full_Excls or PPM_Get_Performs_Update_Excls). */

void
PPM_set_model (unsigned int model, unsigned int type, unsigned int value);
/* Sets information about the PPM model. The argument type is the information
   to be set (i.e. PPM_Set_Alphabet_Size or PPM_Set_Max_Symbol)
   and its value is set to the argument value.

   The type PPM_Alphabet_Size is used for specifying the size of the
   expanded alphabet (which must be equal to 0 - indicating an unbounded
   alphabet, or greater than the existing size to accomodate symbols used by
   the existing model).
*/

void
PPM_release_model (unsigned int ppm_model);
/* Releases the memory allocated to the model and the model number (which may
   be reused in later PPM_create_model or PPM_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active PPM_Contexts pointing at it. */

unsigned int
PPM_copy_model (unsigned int model);
/* Copies the model. */

void
PPM_nullify_model (unsigned int model);
/* Replaces the model with the null model and releases the memory allocated
   to it. */

unsigned int
PPM_load_model (unsigned int file, unsigned int model_form);
/* Loads the PPM model which has been previously saved to the file into memory
   and allocates it a new model number which is returned. */

void
PPM_write_model (unsigned int file, unsigned int ppm_model,
		 unsigned int model_form);
/* Writes out the PPM model to the file (which can then be loaded
   by other applications later). The argument model_form must have the value
   TLM_Static or TLM_Dynamic and determines whether the model is static or
   dynamic when it is later reloaded using TLM_load_model. */

void
PPM_dump_model (unsigned int file, unsigned int ppm_model, boolean dumptrie,
		void (*dump_symbol_function) (unsigned int, unsigned int));
/* Dumps out the PPM model (for debugging purposes). */

unsigned int
PPM_create_context (unsigned int model);
/* Creates and returns an unsigned integer which provides a reference to a PPM
   context record associated with the model's context. The current position is
   set to the null string. The current symbol is set to the first predicted
   symbol. A run-time error occurs if the PPM context being copied is for a
   dynamic model. */

unsigned int
PPM_copy_context (unsigned int model, unsigned int context);
/* Creates a new PPM context record, copies the contents of the specified
   context into it, and returns an integer reference to it. A run-time error
   occurs if the PPM context being copied is for a dynamic model. */

unsigned int
PPM_clone_context (unsigned int model, unsigned int context);
/* Creates a new PPM context record, copies the contents of the specified
   context into it, and returns an integer reference to it. Note: A run-time error
   does not occur if the PPM context being copied is for a dynamic model. */

void
PPM_overlay_context (unsigned int model, unsigned int old_context,
		     unsigned int context);
/* Overlays the PPM context by copying the old context into it. This will
   copy the context even for dynamic models. This is necessary when dynamic
   models with supplementary symbols are being created. */

void
PPM_find_symbol (unsigned int model, unsigned int context,
		 unsigned int symbol);
/* Finds the predicted symbol in the PPM context. */

void
PPM_update_context (unsigned int model, unsigned int context,
		    unsigned int symbol);
/* Updates the PPM context record so that the current symbol becomes symbol.
   Returns additional information as specified by the routine
   TLM_set_context_type. */

void
PPM_release_context (unsigned int model, unsigned int context);
/* Releases the memory allocated to the PPM context and the context number
   (which may be reused in later PPM_create_context or PPM_copy_context and
   TLM_copy_dynamic_context calls). */

void
PPM_reset_symbol (unsigned int model, unsigned int context);
/* Resets the PPM context record to point at the first predicted symbol of the
   current position. */

boolean
PPM_next_symbol (unsigned int model, unsigned int context,
		 unsigned int *symbol);
/* Returns the next predicted symbol in the PPM context and the cost in bits of
   encoding it. The context record is not updated.

   If a sequence of calls to PPM_next_symbol are made, every symbol in the
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
PPM_encode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder, unsigned int symbol);
/* Encodes the specified symbol using the arithmetic coder.
   Updates the context record so that the last symbol in the
   PPM context becomes the encoded symbol. */

unsigned int
PPM_decode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder);
/* Returns the symbol decoded using the arithmetic coder. Updates the
   PPM context record so that the last symbol in the context becomes the
   decoded symbol. */

unsigned int
PPM_getcontext_position (unsigned int model, unsigned int context);
/* Returns an integer which uniquely identifies the current position
   associated with the PPM context. (One implementation is to return a memory
   location corresponding to the current position. This routine is useful if
   you need to check whether different contexts have encoded the same prior
   symbols as when checking whether the context pathways converge in the
   Viterbi or trellis-based algorithms.) */

unsigned int
PPM_minlength_model (unsigned int model);
/* Returns the minimum number of bits needed to write the PPM model
   out to disk as a static model and recover it later. This is
   useful for computing minimum description lengths of messages.

   Note that the amount returned will always be slightly less than
   the resulting size of the static model produced by TLM_write_model
   as this excludes extra overhead data (including the model's title)
   that is necessary for the functioning of the API. */

unsigned int
PPM_sizeof_model (unsigned int model);
/* Returns the current number of bits needed to store the
   model in memory. */

void
PPM_stats_model (unsigned int file, unsigned int model);
/* Writes out statistics about the PPM model in human readable form. */

#endif
