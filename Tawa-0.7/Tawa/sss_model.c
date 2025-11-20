/* Routines for encoding Start-Stop-Step codes. (See Bell, Cleary, Witten
   1990, "Text Compression", page 294).

   A (start, step, stop) unary code of the integers is defined as
   follows:  The Nth codeword has N ones followed by a zero followed by
   a field of size START + (N * STEP).  If the field width is equal to
   STOP then the preceding zero can be omitted.  The integers are laid
   out sequentially through these codewords.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "io.h"
#include "sets.h"
#include "text.h"
#include "coder.h"
#include "model.h"
#include "sss_model.h"

#define SSS_MODELS_SIZE 4          /* Initial max. number of models */
#define SSS_CONTEXTS_SIZE 64       /* Initial max. number of context records */

/* Global variables used for storing the SSS models */
struct SSS_modelType *SSS_Models = NULL;/* List of SSS models */
unsigned int SSS_Models_max_size = 0;   /* Current max. size of models array */
unsigned int SSS_Models_used = NIL;     /* List of deleted model records */
unsigned int SSS_Models_unused = 1;     /* Next unused model record */

struct SSS_contextType
{ /* Record of current context */
    unsigned int S_symbol;              /* Current symbol being encoded */
    unsigned int S_next;                /* Next in the deleted list */
    /* The codelengths and coderanges are calculated directly each time */
};

struct SSS_contextType *SSS_Contexts; /* List of context records */
unsigned int SSS_Contexts_max_size = 0; /* Current max. size of the SSS_Contexts array */
unsigned int SSS_Contexts_used = NIL;   /* List of deleted context records */
unsigned int SSS_Contexts_unused = 1;   /* Next unused context record */

boolean
SSS_valid_model (unsigned int sss_model)
/* Returns non-zero if the SSS model is valid, zero otherwize. */
{
    if (sss_model == NIL)
        return (FALSE);
    else if (sss_model >= SSS_Models_unused)
        return (FALSE);
    else if (SSS_Models [sss_model].S_next != 0)
        return (FALSE);
    else
        return (TRUE);
}

unsigned int
SSS_create_model1 (void)
/* Creates and returns a new pointer to a SSS model record. */
{
    unsigned int sss_model, old_size;

    if (SSS_Models_used != NIL)
    { /* use the first record on the used list */
        sss_model = SSS_Models_used;
	SSS_Models_used = SSS_Models [sss_model].S_next;
    }
    else
    {
	sss_model = SSS_Models_unused;
        if (SSS_Models_unused >= SSS_Models_max_size)
	{ /* need to extend SSS_Models array */
	    old_size = SSS_Models_max_size * sizeof (struct modelType);
	    if (SSS_Models_max_size == 0)
	        SSS_Models_max_size = SSS_MODELS_SIZE;
	    else
	        SSS_Models_max_size = 10*(SSS_Models_max_size+50)/9;

	    SSS_Models = (struct SSS_modelType *)
	        Realloc (84, SSS_Models, SSS_Models_max_size *
			 sizeof (struct SSS_modelType), old_size);

	    if (SSS_Models == NULL)
	    {
	        fprintf (stderr, "Fatal error: out of SSS models space\n");
		exit (1);
	    }
	}
	SSS_Models_unused++;
    }

    SSS_Models [sss_model].S_start = 0;
    SSS_Models [sss_model].S_step = 0;
    SSS_Models [sss_model].S_stop = 0;
    SSS_Models [sss_model].S_next = NIL;

    return (sss_model);
}

unsigned int
SSS_create_model (unsigned int start, unsigned int step, unsigned int stop)
/* Creates and returns a new pointer to a SSS model record. */
{
    unsigned int sss_model;

    sss_model = SSS_create_model1 ();

    SSS_Models [sss_model].S_start = start;
    SSS_Models [sss_model].S_step = step;
    SSS_Models [sss_model].S_stop = stop;

    return (sss_model);
}

void
SSS_get_model (unsigned int sss_model, unsigned int *start,
	       unsigned int *step, unsigned int *stop)
/* Returns information about the SSS model. The arguments start, step and
   stop were used to create the model in SSS_create_model(). */
{
    assert (SSS_valid_model (sss_model));

    *start = SSS_Models [sss_model].S_start;
    *step = SSS_Models [sss_model].S_step;
    *stop = SSS_Models [sss_model].S_stop;
}

void
SSS_release_model (unsigned int sss_model)
/* Releases the memory allocated to the model and the model number (which may
   be reused in later SSS_create_model or SSS_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active SSS_Contexts pointing at it. */
{
    assert (SSS_valid_model (sss_model));

    /* add model record at the head of the SSS_Models_used list */
    SSS_Models [sss_model].S_next = SSS_Models_used;
    SSS_Models_used = sss_model;
}

unsigned int
SSS_load_model (unsigned int file)
/* Loads the SSS model which has been previously saved to the file into memory
   and allocates it a new model number which is returned. */
{
    unsigned int sss_model;

    sss_model = SSS_create_model1 (); /* Get next unused SSS model record */
    assert (sss_model != NIL);

    /* read in the start, step and stop parameters of the model */
    SSS_Models [sss_model].S_start = fread_int (file, INT_SIZE);
    SSS_Models [sss_model].S_step = fread_int (file, INT_SIZE);
    SSS_Models [sss_model].S_stop = fread_int (file, INT_SIZE);

    return (sss_model);
}

void
SSS_write_model (unsigned int file, unsigned int model)
/* Writes out the SSS model to the file (which can then be loaded
   by other applications later). */
{
    unsigned int sss_model;

    sss_model = TLM_verify_model (model, TLM_SSS_Model, SSS_valid_model);

    assert (TXT_valid_file (file));

    /* write out the start, step and stop parameters of the model */
    fwrite_int (file, SSS_Models [sss_model].S_start, INT_SIZE);
    fwrite_int (file, SSS_Models [sss_model].S_step, INT_SIZE);
    fwrite_int (file, SSS_Models [sss_model].S_stop, INT_SIZE);
}

void
SSS_dump_model (unsigned int file, unsigned int sss_model)
/* Dumps out the SSS model (for debugging purposes). */
{
    assert (SSS_valid_model (sss_model));

    fprintf (Files [file], "Start parameter = %d\n",
	     SSS_Models [sss_model].S_start);
    fprintf (Files [file], "Step parameter = %d\n",
	     SSS_Models [sss_model].S_step);
    fprintf (Files [file], "Stop parameter = %d\n",
	     SSS_Models [sss_model].S_stop);
}

boolean
SSS_valid_context (unsigned int context)
/* Returns non-zero if the SSS context is valid, zero otherwize. */
{
    if (context == NIL)
        return (FALSE);
    else if (context >= SSS_Contexts_unused)
        return (FALSE);
    else if (SSS_Contexts [context].S_next != 0)
        return (FALSE);
    else
        return (TRUE);
}
 
unsigned int
SSS_create_context1 (void)
/* Creates and returns a nre context record. */
{
    unsigned int context, old_size;

    if (SSS_Contexts_used != NIL)
    {	/* use the first record on the used list */
	context = SSS_Contexts_used;
	SSS_Contexts_used = - SSS_Contexts [context].S_next;
    }
    else
    {
	context = SSS_Contexts_unused;
	if (SSS_Contexts_unused >= SSS_Contexts_max_size)
	{ /* need to extend SSS_Contexts array */
	    old_size = SSS_Contexts_max_size * sizeof (struct SSS_contextType);
	    if (SSS_Contexts_max_size == 0)
		SSS_Contexts_max_size = SSS_CONTEXTS_SIZE;
	    else
		SSS_Contexts_max_size = 10*(SSS_Contexts_max_size+50)/9; 

	    SSS_Contexts = (struct SSS_contextType *)
	        Realloc (85, SSS_Contexts, SSS_Contexts_max_size *
			 sizeof (struct SSS_contextType), old_size);

	    if (SSS_Contexts == NULL)
	    {
		fprintf (stderr, "Fatal error: out of SSS_Contexts space\n");
		exit (1);
	    }
	}
	SSS_Contexts_unused++;
    }

    SSS_Contexts [context].S_symbol = NIL;
    SSS_Contexts [context].S_next = NIL;

    return (context);
}

unsigned int
SSS_create_context (unsigned int model)
/* Creates and returns an unsigned integer which provides a reference to a SSS
   context record associated with the model's context. The current position is
   set to the null string. The current symbol is set to the first predicted
   symbol. */
{
  /*unsigned int sss_model;*/
    unsigned int context;

    /* sss_model = */ TLM_verify_model (model, TLM_SSS_Model, SSS_valid_model);

    Models [model].M_contexts++;

    context = SSS_create_context1 ();

    assert (context != NIL);
    return (context);
}

void
SSS_release_context (unsigned int model, unsigned int context)
/* Releases the memory allocated to the SSS context and the context number
   (which may be reused in later SSS_create_context or SSS_copy_context
   calls). */
{
  /*unsigned int sss_model;*/

  /*sss_model =*/ TLM_verify_model (model, TLM_SSS_Model, SSS_valid_model);

    assert (SSS_valid_context (context));

    if (Models [model].M_contexts > 0)
        Models [model].M_contexts--;
 
    /* Append onto head of the used list */
    SSS_Contexts [context].S_next = SSS_Contexts_used;
    SSS_Contexts_used = context;
}

void
SSS_copy_context1 (unsigned int model, unsigned int context,
		   unsigned int new_context)
/* Copies the contents of the specified context into the new context. */
{
    SSS_Contexts [new_context].S_symbol = SSS_Contexts [context].S_symbol;
    SSS_Contexts [new_context].S_next   = SSS_Contexts [context].S_next;
}

unsigned int
SSS_copy_context (unsigned int model, unsigned int context)
/* Creates a new SSS context record, copies the contents of the specified
   contex into it, and returns an integer reference to it. A run-time error
   occurs if the SSS context being copied is for a dynamic model. */
{
  /*unsigned int sss_model;*/
    unsigned int new_context;

    /*sss_model =*/ TLM_verify_model (model, TLM_SSS_Model, SSS_valid_model);

    assert (SSS_valid_context (context));

    new_context = SSS_create_context1 ();

    SSS_copy_context1 (model, context, new_context);
    return (new_context);
}

void
SSS_overlay_context (unsigned int model, unsigned int old_context,
		     unsigned int context)
/* Overlays the SSS context by copying the old context into it. This will
   copy the context even for dynamic models. This is necessary when dynamic
   models with supplementary symbols are being created. */
{
  /*unsigned int sss_model;*/

  /*sss_model =*/ TLM_verify_model (model, TLM_SSS_Model, SSS_valid_model);

    assert (SSS_valid_context (context));
    assert (SSS_valid_context (old_context));

    SSS_copy_context1 (model, old_context, context);
}

void
SSS_encode_bit (boolean one_bit, codingType coding_type, unsigned int coder)
{
    assert (coder = TLM_valid_coder (coder));

    switch (coding_type)
      {
      case FIND_CODELENGTH_TYPE:
        TLM_Codelength++; /* add 1 bit to the codelength */
	break;
      case FIND_CODERANGES_TYPE:
	if (one_bit)
	  TLM_append_coderange (TLM_Coderanges, 1, 2, 2); /* encode a 1-bit */
	else
	  TLM_append_coderange (TLM_Coderanges, 0, 1, 2); /* encode a 0-bit */
	break;
      case ENCODE_TYPE:
	if (one_bit)
	  Coders [coder].A_arithmetic_encode
	    (Coders [coder].A_encoder_output_file, 1, 2, 2);
	else
	  Coders [coder].A_arithmetic_encode
	    (Coders [coder].A_encoder_output_file, 0, 1, 2);
	break;
      default:
	break;
      }
}

void
SSS_encode_position (unsigned int sss_model, unsigned int context,
		     unsigned int symbol, codingType coding_type,
		     unsigned int coder)
/* Updates the SSS context so that the current symbol becomes
   symbol and encodes the symbol. */
{
    unsigned step1, lowerbound, power, bits, p;
    unsigned int start, step, stop;

    assert (SSS_valid_model (sss_model));
    assert (coder = TLM_valid_coder (coder));

    start = SSS_Models [sss_model].S_start;
    step = SSS_Models [sss_model].S_step;
    stop = SSS_Models [sss_model].S_stop;

    SSS_Contexts [context].S_symbol = symbol;

    switch (TLM_Context_Operation)
      {
      case TLM_Get_Nothing:
	break;
      case TLM_Get_Coderanges:
        TLM_release_coderanges (TLM_Coderanges);
	TLM_Coderanges = TLM_create_coderanges ();
	if (coding_type != ENCODE_TYPE)
	    coding_type = FIND_CODERANGES_TYPE;
	break;
      default:
        TLM_Codelength = 0.0;
	if (coding_type != ENCODE_TYPE)
	    coding_type = FIND_CODELENGTH_TYPE;
	break;
      }

    lowerbound = 0;
    bits = start;
    step1 = start;
    power = POWER_OF_2 (1, start);
    assert (power > 0); /* check for overflow */

    /* Encode the unary prefix code */
    while (power && (symbol >= lowerbound + power) && (step1 < stop))
      {
	/* encode a 0-bit in the prefix unary code */
	SSS_encode_bit (ZERO_BIT, coding_type, coder);

	step1 += step;
	lowerbound += power;
	bits += step;
	power = POWER_OF_2 (power, step);
      }

    /* Encode the end of the unary prefix code */
    if (step1 < stop)
        /* encode a 1-bit in the prefix unary code */
	SSS_encode_bit (ONE_BIT, coding_type, coder);

    /* Now encode the binary part of the code */
    symbol -= lowerbound;

    for (p = bits; p > 0; p--)
      {
	if (UINT_ISSET (symbol, p-1))
	    /* encode a 1-bit in the suffix binary code */
	    SSS_encode_bit (ONE_BIT, coding_type, coder);
	else
	    /* encode a 0-bit in the suffix binary code */
	    SSS_encode_bit (ZERO_BIT, coding_type, coder);
      }
}

unsigned int
SSS_decode_position (unsigned int sss_model, unsigned int context,
		     unsigned int coder)
/* Updates the SSS context so that the current symbol becomes
   symbol and returns the decoded symbol. */
{
    unsigned step1, lowerbound, power, bits, symbol, p;
    unsigned int start, step, stop;

    assert (SSS_valid_model (sss_model));
    assert (coder = TLM_valid_coder (coder));

    start = SSS_Models [sss_model].S_start;
    step = SSS_Models [sss_model].S_step;
    stop = SSS_Models [sss_model].S_stop;

    symbol = 0;

    lowerbound = 0;
    bits = start;
    step1 = start;
    power = POWER_OF_2 (1, start);
    assert (power > 0); /* check for overflow */

    /* Decode the prefix unary code */
    while (power && (step1 < stop))
      {
	if (Coders [coder].A_arithmetic_decode_target
	    (Coders [coder].A_decoder_input_file, 2) < 1)
	    /* decode a 0-bit: */
	    Coders [coder].A_arithmetic_decode
	      (Coders [coder].A_decoder_input_file, 0, 1, 2);
	else
	  {
	    /* decode a 1-bit: */
	    Coders [coder].A_arithmetic_decode
	      (Coders [coder].A_decoder_input_file, 1, 2, 2);
	    break;
	  }
	step1 += step;
	lowerbound += power;
	bits += step;
	power = POWER_OF_2 (power, step);
      }

    /* Now decode the binary part of the code */

    for (p = bits; p > 0; p--)
      {
	if (Coders [coder].A_arithmetic_decode_target
	    (Coders [coder].A_decoder_input_file, 2) < 1)
	    /* decode a 0 bit: */
	    Coders [coder].A_arithmetic_decode
	      (Coders [coder].A_decoder_input_file, 0, 1, 2);
	else
	  {
	    UINT_SET (symbol, p-1); /* encode a 1-bit into n */
	    /* decode a 1 bit: */
	    Coders [coder].A_arithmetic_decode
	      (Coders [coder].A_decoder_input_file, 1, 2, 2);
	  }
      }

    symbol += lowerbound;
    SSS_Contexts [context].S_symbol = symbol;
    return (symbol);
}

void
SSS_find_symbol (unsigned int model, unsigned int context,
		 unsigned int symbol)
/* Finds the predicted symbol in the SSS context. */
{
  /*unsigned int sss_model;*/

  /*sss_model =*/ TLM_verify_model (model, TLM_SSS_Model, SSS_valid_model);

    assert (SSS_valid_context (context));

    SSS_encode_position (Models [model].M_model, context, symbol,
			 UPDATE_TYPE, NO_CODER);
}

void
SSS_update_context (unsigned int model, unsigned int context,
		    unsigned int symbol)
/* Updates the SSS context record so that the current symbol becomes symbol.
   Returns additional information as specified by the routine
   TLM_set_context_operation. */
{
  /*unsigned int sss_model;*/

  /*sss_model =*/ TLM_verify_model (model, TLM_SSS_Model, SSS_valid_model);

    assert (SSS_valid_context (context));
    SSS_Contexts [context].S_symbol = symbol;

    if (TLM_Context_Operation != TLM_Get_Nothing)
        SSS_encode_position (Models [model].M_model, context, symbol,
			     UPDATE_TYPE, NO_CODER);
}

void
SSS_reset_symbol (unsigned int model, unsigned int context)
/* Resets the SSS context record to point at the first predicted symbol of the
   current position. */
{
  /*unsigned int sss_model;*/

  /*sss_model =*/ TLM_verify_model (model, TLM_SSS_Model, SSS_valid_model);

    assert (SSS_valid_context (context));

    SSS_Contexts [context].S_symbol = TXT_sentinel_symbol ();
    /* This means start at symbol zero when SSS_next_symbol is next called */
}

boolean
SSS_next_symbol (unsigned int model, unsigned int context,
		 unsigned int *symbol)
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
{
  /*unsigned int sss_model;*/
    unsigned int symbol1;

    /*sss_model =*/ TLM_verify_model (model, TLM_SSS_Model, SSS_valid_model);

    assert (SSS_valid_context (context));

    symbol1 = SSS_Contexts [context].S_symbol;

    if (symbol1 == TXT_sentinel_symbol ())
        symbol1 = 0; /* start at symbol zero */
    else if (symbol1 == TXT_sentinel_symbol () - 1)
        return (FALSE); /* exceeded maximum symbol */
    else
        symbol1++;
    *symbol = symbol1;

    SSS_encode_position (Models [model].M_model, context, symbol1,
			 UPDATE_TYPE, NO_CODER);

    return (TRUE);
}

void
SSS_encode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder, unsigned int symbol)
/* Encodes the specified symbol using the arithmetic coder.
   Updates the context record so that the last symbol in the
   SSS context becomes the encoded symbol. */
{
  /*unsigned int sss_model;*/

  /*sss_model =*/ TLM_verify_model (model, TLM_SSS_Model, SSS_valid_model);

    assert (SSS_valid_context (context));
    SSS_Contexts [context].S_symbol = symbol;

    SSS_encode_position (Models [model].M_model, context, symbol,
			 ENCODE_TYPE, coder);
}

unsigned int
SSS_decode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder)
/* Returns the symbol decoded using the arithmetic coder. Updates the
   SSS context record so that the last symbol in the context becomes the
   decoded symbol. */
{
  /*unsigned int sss_model;*/
    unsigned int symbol;

    /*sss_model =*/ TLM_verify_model (model, TLM_SSS_Model, SSS_valid_model);

    assert (SSS_valid_context (context));
    assert (coder = TLM_valid_coder (coder));

    symbol = SSS_decode_position (Models [model].M_model, context, coder);
    return (symbol);
}

unsigned int
SSS_sizeof_model (unsigned int model)
/* Returns the current number of bits needed to store the
   model in memory. */
{
  /*unsigned int sss_model;*/

  /*sss_model =*/ TLM_verify_model (model, TLM_SSS_Model, SSS_valid_model);

    return (sizeof (struct SSS_modelType));
}

void
SSS_stats_model (unsigned int file, unsigned int model)
/* Writes out statistics about the SSS model in human readable form. */
{
  /*unsigned int sss_model;*/

  /*sss_model =*/ TLM_verify_model (model, TLM_SSS_Model, SSS_valid_model);

    assert (TXT_valid_file (file));

    TXT_write_file (file, "No statistics available for SSS model\n");
}
