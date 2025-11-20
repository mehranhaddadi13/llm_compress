/* TLM routines for language models. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#include "io.h"
#include "text.h"
#include "samm_model.h"
#include "ppm_trie.h"
#include "ppm_context.h"
#include "ppm_model.h"
#include "ppmq_model.h"
#include "ppmo_model.h"
#include "pt_model.h"
#include "hmm_model.h"
#include "sss_model.h"
#include "tag_model.h"
#include "user_model.h"
#include "coder.h"
#include "model.h"

#define MODELS_SIZE 4               /* Initial max. number of models */
#define CODERS_SIZE 4               /* Initial max. number of coder records */
#define CODER_MAXFREQ (1 << 27)     /* Max. frequency for the default
				       arithmetic coder */

struct debugType Debug              /* For setting various debugging options */
= { 0, 0, 0, 0, 0, 0, 0 };

/* Global variables used for storing the models */
struct modelType *Models = NULL;    /* List of models */
unsigned int Models_max_size = 0;   /* Current max. size of the models array */
unsigned int Models_count = 0;      /* Current number of models (always <= max. size) */
unsigned int Models_used = NIL;     /* List of deleted model records */
unsigned int Models_unused = 1;     /* Next unused model record */
unsigned int Models_head = NIL;     /* Head of the models list */
unsigned int Models_tail = NIL;     /* Tail of the models list */
unsigned int Models_ptr = NIL;      /* Current position in the models list */

/* Global variables used for storing the arithmetic coders */
struct coderType *Coders = NULL;    /* List of coder records */
unsigned int Coders_max_size = 0;   /* Current max. size of the coders array */
unsigned int Coders_used = NIL;     /* List of deleted coder records */
unsigned int Coders_unused = 1;     /* Next unused coder record */

/* Global variables used for storing the latest codelength and coderanges
   for the routines TLM_next_symbol, TLM_get_symbol, TLM_find_symbol or
   TLM_update_context */
unsigned int TLM_Context_Operation = TLM_Get_Nothing;
unsigned int TLM_Symbol;
unsigned int TLM_Coderanges = NIL;
float TLM_Codelength;

/* Global variables used by TLM_Load_Model to alter characteristics of the model
   loaded from disk. */
boolean TLM_Load_Do_ChangeTitle = FALSE;
char *TLM_Load_Title;
boolean TLM_Load_Do_ChangeModelType = FALSE;
unsigned int TLM_Load_Model_Type;

unsigned int TLM_Tag_Word = NIL; /* Used by TAG_decode_symbol () */
 
/* Global variables used by TLM_Write_Model to alter characteristics of the model
   written to disk. */
boolean TLM_Write_Do_ChangeModelType = FALSE;
unsigned int TLM_Write_Model_Type;

/* Global variables used by TLM_get_PPM_model. */
char *TLM_PPM_title; /* The title of the PPM model */
unsigned int TLM_PPM_model_form = NIL;        /* TLM_Dynamic or TLM_Static depending on
						 whether the model is static or dynamic. */
unsigned int TLM_PPM_alphabet_size = 0;       /* The PPM model's alphabet size */
int TLM_PPM_max_order = 5;                    /* The PPM model's max order, >= -1 */
char TLM_PPM_escape_method = 'D';             /* The escape method used by the PPM model */
boolean TLM_PPM_performs_full_excls = TRUE;   /* The PPM model performs full exclusions */
boolean TLM_PPM_performs_update_excls = TRUE; /* The PPM model performs update exclusions */
  
unsigned int
create_model (void)
/* Creates and returns a new pointer to a model record. */
{
    unsigned int model, old_size;

    if (Models_used != NIL)
    { /* use the first record on the used list */
        model = Models_used;
	Models_used = Models [model].M_next;
    }
    else
    {
	model = Models_unused;
        if (Models_unused >= Models_max_size)
	{ /* need to extend Models array */
	    old_size = Models_max_size * sizeof (struct modelType);
	    if (Models_max_size == 0)
	        Models_max_size = MODELS_SIZE;
	    else
	        Models_max_size = 10*(Models_max_size+50)/9;

	    Models = (struct modelType *) Realloc (84, Models,
                     Models_max_size * sizeof (struct modelType), old_size);

	    if (Models == NULL)
	    {
	        fprintf (stderr, "Fatal error: out of models space\n");
		exit (1);
	    }
	}
	Models_unused++;
    }

    Models_count++;

    /* add the new model at the tail of the models list */
    if (Models_tail == NIL)
        Models_head = model;
    else
    {
        Models [model].M_prev = Models_tail;
	Models [Models_tail].M_next = model;
    }
    Models_tail = model;

    Models [model].M_next = NIL;
    Models [model].M_title = NULL;
    Models [model].M_tag = NULL;
    Models [model].M_model = NIL;
    Models [model].M_model_type = NIL;
    Models [model].M_model_form = NIL;
    Models [model].M_contexts = 0;
    Models [model].M_version_no = TLM_Version_No;

    return (model);
}

unsigned int
create_coder (void)
/* Creates a coder record. */
{
    unsigned int c, old_size;

    if (Coders_used != NIL)
    {	/* use the first list of coders on the used list */
	c = Coders_used;
	Coders_used = Coders [c].A_next;
    }
    else
    {
	c = Coders_unused;
	if (Coders_unused+1 >= Coders_max_size)
	{ /* need to extend Coders array */
	    old_size = Coders_max_size * sizeof (struct coderType);
	    if (Coders_max_size == 0)
		Coders_max_size = CODERS_SIZE;
	    else
		Coders_max_size *= 2; /* Keep on doubling the array on demand */
	    Coders = (struct coderType *) Realloc (83, Coders,
		     Coders_max_size * sizeof (struct coderType), old_size);

	    if (Coders == NULL)
	    {
		fprintf (stderr, "Fatal error: out of coders space\n");
		exit (1);
	    }
	}
	Coders_unused++;
    }

    if (c != NIL)
    {
        Coders [c].A_max_frequency = 1;
        Coders [c].A_next = NIL;
    }
    return (c);
}

unsigned int
TLM_create_coder
( unsigned int max_frequency,
  unsigned int encoder_input_file, unsigned int encoder_output_file,
  unsigned int decoder_input_file, unsigned int decoder_output_file,
  void (*arithmetic_encode) (unsigned int, unsigned int, unsigned int, unsigned int),
  void (*arithmetic_decode) (unsigned int, unsigned int, unsigned int, unsigned int),
  unsigned int (*arithmetic_decode_target) (unsigned int, unsigned int))
/* Creates and returns an unsigned integer which provides a reference to a coder
   record associated with an arithmetic coder. The argument max_frequency
   specifies the maximum frequency allowed for the coder. The arguments
   arithmetic_encode, arithmetic_decode and arithmetic_decode_target are
   pointers to the necessary routines required for encoding and decoding. Both
   arithmetic_encode and arithmetic_decode take three unsigned integers
   as arguments that specify the current arithmetic coding range (low, high
   and total); arithmetic_decode_target takes just a single unsigned integer
   as an argument, which is set to the total of the current coding range. */
{
    unsigned int coder1;
    struct coderType *coder;

    coder1 = create_coder ();
    coder = Coders + coder1;

    coder->A_encoder_input_file = encoder_input_file;
    coder->A_encoder_output_file = encoder_output_file;
    coder->A_decoder_input_file = decoder_input_file;
    coder->A_decoder_output_file = decoder_output_file;
    coder->A_max_frequency = max_frequency;
    coder->A_arithmetic_encode = arithmetic_encode;
    coder->A_arithmetic_decode = arithmetic_decode;
    coder->A_arithmetic_decode_target = arithmetic_decode_target;
    return (coder1);
}

unsigned int
TLM_create_arithmetic_coder (void)
/* Creates and returns the default arithmetic coder. */
{
    unsigned int coder;

    coder = TLM_create_coder
      (CODER_MAX_FREQUENCY, Stdin_File, Stdout_File, Stdin_File, Stdout_File,
       arithmetic_encode, arith_decode, arith_decode_target);
    return (coder);
}

unsigned int
TLM_create_arithmetic_encoder (unsigned int input_file, unsigned int output_file)
/* Creates and returns the arithmetic encoder. */
{
    unsigned int coder;

    coder = TLM_create_coder
      (CODER_MAX_FREQUENCY, input_file, output_file, NIL, NIL,
       arithmetic_encode, arith_decode, arith_decode_target);
    return (coder);
}

unsigned int
TLM_create_arithmetic_decoder (unsigned int input_file, unsigned int output_file)
/* Creates and returns the arithmetic decoder. */
{
    unsigned int coder;

    coder = TLM_create_coder
      (CODER_MAX_FREQUENCY, NIL, NIL, input_file, output_file,
       arithmetic_encode, arith_decode, arith_decode_target);
    return (coder);
}

void
TLM_release_coder (unsigned int coder)
/* Releases the memory allocated to the coder and the coder number (which may
   be reused in later TLM_create_coder calls). */
{
    /* Check for the default coder */
    if (coder == NIL)
        return;

    assert (coder = TLM_valid_coder (coder));
 
    Coders [coder].A_max_frequency = 0; /* Used for testing later on if coder no. is valid or not */

    /* Append onto head of the used list */
    Coders [coder].A_next = Coders_used;
    Coders_used = coder;
}

boolean
TLM_valid_context (unsigned int model, unsigned int context)
/* Returns non-zero if the model is valid, zero otherwize. */
{
    unsigned int model_type, function;
    boolean (*valid_context_function) (unsigned int);

    assert (TLM_valid_model (model));

    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	return (PPM_valid_context (context));
	break;
      case TLM_PPMq_Model:
	return (PPMq_valid_context (context));
	break;
      case TLM_PPMo_Model:
	return (PPMo_valid_context (context));
	break;
      case TLM_SAMM_Model:
	return (SAMM_valid_context (context));
	break;
      case TLM_PT_Model:
	return (TXT_valid_text (context));
	break;
      case TLM_HMM_Model:
	return (HMM_valid_context (context));
	break;
      case TLM_SSS_Model:
	return (SSS_valid_context (context));
	break;
      case TLM_USER_Model:
	USER_get_model (model, USER_Get_Valid_Context_Function, &function);
	valid_context_function = (boolean (*) (unsigned int)) function;
	return (valid_context_function (context));
	break;
      default:
	fprintf (stderr, "Function valid_context for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }

    return (NIL);
}

unsigned int
TLM_create_context (unsigned int model)
/* Creates and returns an unsigned integer which provides a reference to a
   context record associated with the model's context. The current position is
   set to the null string. The current symbol is set to the first predicted
   symbol. */
{
    unsigned int model_type, function;
    unsigned int (*create_context_function) (unsigned int);

    assert (TLM_valid_model (model));

    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	return (PPM_create_context (model));
	break;
      case TLM_PPMq_Model:
	return (PPMq_create_context (model));
	break;
      case TLM_PPMo_Model:
	return (PPMo_create_context (model));
	break;
      case TLM_SAMM_Model:
	return (SAMM_create_context (model));
	break;
      case TLM_PT_Model:
	return (TXT_create_text ());
	break;
      case TLM_HMM_Model:
	return (HMM_create_context (model));
	break;
      case TLM_SSS_Model:
	return (SSS_create_context (model));
	break;
      case TLM_TAG_Model:
	return (TAG_create_context (model));
	break;
      case TLM_USER_Model:
	USER_get_model (model, USER_Get_Create_Context_Function, &function);
	create_context_function = (unsigned int (*) (unsigned int)) function;
	return (create_context_function (model));
	break;
      default:
	fprintf (stderr, "Function create_context for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }

    return (NIL);
}

unsigned int
TLM_copy_context (unsigned int model, unsigned int context)
/* Creates a new context record, copies the contents of the specified context
   into it, and returns an integer reference to it. A run-time error occurs
   if the context being copied is for a dynamic model. */
{
    unsigned int model_type;

    assert (TLM_valid_model (model));

    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	return (PPM_copy_context (model, context));
	break;
      case TLM_PPMo_Model:
	return (PPMo_copy_context (model, context));
	break;
      case TLM_HMM_Model:
	return (HMM_copy_context (model, context));
	break;
      case TLM_SSS_Model:
	return (SSS_copy_context (model, context));
	break;
      case TLM_TAG_Model:
	return (TAG_copy_context (model, context));
	break;
      default:
	fprintf (stderr, "Function copy_context for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }

    return (NIL);
}

unsigned int
TLM_clone_context (unsigned int model, unsigned int context)
/* Creates a new context record, copies the contents of the specified context
   into it, and returns an integer reference to it. A run-time error does not
   occur if the context being copied is for a dynamic model. */
{
    unsigned int model_type;

    assert (TLM_valid_model (model));

    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	return (PPM_clone_context (model, context));
	break;
      default:
	fprintf (stderr, "Function clone_context for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }
}

void
TLM_overlay_context (unsigned int model, unsigned int old_context,
		     unsigned int context)
/* Overlays the context by copying the old context into it. This will
   copy the context even for dynamic models. This is necessary when dynamic
   models with supplementary symbols are being created. */
{
    unsigned int model_type;

    assert (TLM_valid_model (model));

    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	PPM_overlay_context (model, old_context, context);
	break;
      case TLM_HMM_Model:
	HMM_overlay_context (model, old_context, context);
	break;
      case TLM_SSS_Model:
	SSS_overlay_context (model, old_context, context);
	break;
      default:
	fprintf (stderr, "Function overlay_context for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }
}

void
TLM_set_context_operation (unsigned int context_operation)
/* Sets the type of operation to be performed by the routines
   TLM_next_symbol, TLM_get_symbol, TLM_find_symbol and TLM_update_context.
   The argument operation_type is one of the following:
     TLM_Get_Nothing
         Default operation; no additional information is returned.
     TLM_Get_Codelength
	 Returned in the global variable TLM_Codelength is a float value 
	 which is set to the codelength for encoding the specified symbol
	 (i.e. the cost in bits of encoding it given the current context).
     TLM_Get_Coderanges
         Returned in the global variable TLM_Coderanges is an unsigned int
	 pointer to the list of arithmetic coding ranges required for
	 encoding the specified symbol given the current context.
     TLM_Get_Maxorder
	 Returned in the global variable TLM_Codelength is a float value 
	 which is set to the codelength for encoding the specified symbol
	 (i.e. the cost in bits of encoding it given the current context)
	 assuming only the maxorder symbols are being coded (i.e. that no
	 escapes occur). */
{
    TLM_Context_Operation = context_operation;
    if (context_operation == TLM_Get_Coderanges)
      {
	TLM_release_coderanges (TLM_Coderanges);
	TLM_Coderanges = TLM_create_coderanges ();
      }
}

void
TLM_find_symbol (unsigned int model, unsigned int context,
		 unsigned int symbol)
/* Finds the predicted symbol in the context. */
{
    unsigned int model_type;

    assert (TLM_valid_model (model));

    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	PPM_find_symbol (model, context, symbol);
	break;
      case TLM_HMM_Model:
	HMM_find_symbol (model, context, symbol);
	break;
      case TLM_SSS_Model:
	SSS_find_symbol (model, context, symbol);
	break;
      case TLM_PT_Model:
	PT_find_symbol (model, context, symbol);
	break;
      default:
	fprintf (stderr, "Function find_symbol for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }
}

void
TLM_update_context (unsigned int model, unsigned int context,
		    unsigned int symbol)
/* Updates the context record so that the current symbol becomes symbol.
   Returns additional information as specified by the routine
   TLM_set_context_operation. (For faster updates, set the option
   TLM_Get_Nothing, so that the routine does not return
   any additional information such as the cost of encoding
   the symbol in bits (in TLM_Codelength).

   The ``sentinel symbol'' (whose value is obtained using the
   TXT_sentinel_symbol () routine) is used where there is a break
   required in the updating of the context, such as when the end of
   string has been reached or when more than one model is being used
   to encode different parts of a string. The effect of encoding the
   sentinel symbol is that the prior context is forced to the null
   string i.e. the subsequent context will contain just the sentinel
   symbol itself. This is useful during training if there are statistics
   that differ markedly at the start of some text than in the middle of
   it (for example, individual names, and titles within a long list).

   This routine is often used with the routines TLM_next_symbol,
   TLM_get_symbol, TLM_find_symbol. For example,
       TLM_update_context (context, TXT_sentinel_symbol (), ...)
   will update the context record so that the current symbol becomes the
   sentinel symbol. */
{
    unsigned int function;
    void (*update_context_function) (unsigned int, unsigned int, unsigned int);
    unsigned int model_type;

    assert (TLM_valid_model (model));

    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	PPM_update_context (model, context, symbol);
	break;
      case TLM_PPMq_Model:
	PPMq_update_context (model, context, symbol);
	break;
      case TLM_PPMo_Model:
	PPMo_update_context (model, context, symbol);
	break;
      case TLM_SAMM_Model:
	SAMM_update_context (model, context, symbol);
	break;
      case TLM_PT_Model:
	PT_update_context (model, context, symbol);
	break;
      case TLM_HMM_Model:
	HMM_update_context (model, context, symbol);
	break;
      case TLM_SSS_Model:
	SSS_update_context (model, context, symbol);
	break;
      case TLM_TAG_Model:
	TAG_update_context (model, context, symbol);
	break;
      case TLM_USER_Model:
	USER_get_model (model, USER_Get_Update_Context_Function, &function);
	update_context_function = (void (*) (unsigned int, unsigned int, unsigned int)) function;
	update_context_function (model, context, symbol);
	break;
      default:
	fprintf (stderr, "Function update_context for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }
}

void
TLM_suspend_update (unsigned int model)
/* Suspends the update for a dynamic model temporarily (i.e. the
   model becomes a temporary static model and TLM_update_context
   will not update any of the internal statistics of the model.
   The update can be resumed using TLM_resume_update ().

   This is useful if it needs to be determined in advance which
   of two or more dynamic models a sequence of text should be
   added to (based on how much each requires to encode it, say). */
{
    assert (TLM_valid_model (model));
    assert (Models [model].M_model_form == TLM_Dynamic);

    Models [model].M_model_form = TLM_Static;
}

void
TLM_resume_update (unsigned int model)
/* Resumes the update for a model. See TLM_suspend_update (). */
{
    assert (TLM_valid_model (model));
    assert (Models [model].M_model_form == TLM_Static);

    Models [model].M_model_form = TLM_Dynamic;
}

void
TLM_release_context (unsigned int model, unsigned int context)
/* Releases the memory allocated to the context and the context number
   (which may be reused in later TLM_create_context or TLM_copy_context and
   TLM_copy_dynamic_context calls). */
{
    unsigned int model_type, function;
    void (*release_context_function) (unsigned int, unsigned int);

    assert (TLM_valid_model (model));

    model_type = Models [model].M_model_type;

    switch (model_type)
      {
      case TLM_PPM_Model:
	PPM_release_context (model, context);
	break;
      case TLM_PPMq_Model:
	PPMq_release_context (model, context);
	break;
      case TLM_PPMo_Model:
	PPMo_release_context (model, context);
	break;
      case TLM_SAMM_Model:
	SAMM_release_context (model, context);
	break;
      case TLM_PT_Model:
	TXT_release_text (context);
	break;
      case TLM_HMM_Model:
	HMM_release_context (model, context);
	break;
      case TLM_SSS_Model:
	SSS_release_context (model, context);
	break;
      case TLM_TAG_Model:
	TAG_release_context (model, context);
	break;
      case TLM_USER_Model:
	USER_get_model (model, USER_Get_Release_Context_Function, &function);
	release_context_function = (void (*) (unsigned int, unsigned int)) function;
	release_context_function (model, context);
	break;
      default:
	fprintf (stderr, "Function release_context for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }
}

void
TLM_reset_symbol (unsigned int model, unsigned int context)
/* Resets the context record to point at the first predicted symbol of the
   current position. */
{
    unsigned int model_type;

    assert (TLM_valid_model (model));

    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	PPM_reset_symbol (model, context);
	break;
      case TLM_HMM_Model:
	HMM_reset_symbol (model, context);
	break;
      case TLM_SSS_Model:
	SSS_reset_symbol (model, context);
	break;
      default:
	fprintf (stderr, "Function reset_symbol for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }
}

boolean
TLM_next_symbol (unsigned int model, unsigned int context,
		 unsigned int *symbol)
/* Returns the next predicted symbol in the context and the cost in bits of
   encoding it. The context record is not updated.

   The global variable TLM_Codelength is set to the symbol's codelength.

   If a sequence of calls to TLM_next_symbol are made, every symbol in the
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
    unsigned int model_type;

    assert (TLM_valid_model (model));

    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	return (PPM_next_symbol (model, context, symbol));
	break;
      case TLM_HMM_Model:
	return (HMM_next_symbol (model, context, symbol));
	break;
      case TLM_SSS_Model:
	return (SSS_next_symbol (model, context, symbol));
	break;
      default:
	fprintf (stderr, "Function next_symbol for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }

    return (FALSE);
}

boolean
TLM_get_symbol (unsigned int model, unsigned int context)
/* Returns the next predicted symbol in the context and the cost in bits of
   encoding it. The context record is not updated.

   The global variables TLM_Symbol and TLM_Codelength is set to the symbol's
   value and codelength respectively.

   If a sequence of calls to TLM_get_symbol are made, every symbol in the
   alphabet will be visited exactly once although the order in which they are
   visited is undefined being implementation and data dependent. The function
   returns FALSE when there are no more symbols to process. TLM_reset_symbol
   will reset the current position to point back at the first predicted symbol
   of the current context.

   The codelength value is the same as that returned by TLM_update_context
   which may use a faster search method to find the symbol's codelength
   more directly (rather than sequentially as TLM_get_symbol does). A call
   to TLM_update_context or other routines will have no affect on subsequent
   calls to TLM_get_symbol. */
{
    unsigned int symbol;
    boolean found_symbol;

    found_symbol = TLM_next_symbol (model, context, &symbol);
    if (found_symbol)
        TLM_Symbol = symbol;
    else
        TLM_Symbol = TXT_sentinel_symbol ();

    return (found_symbol);
}

void
TLM_encode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder, unsigned int symbol)
/* Encodes the specified symbol using the arithmetic coder.
   Updates the context record so that the last symbol in the
   context becomes the encoded symbol. */
{
    unsigned int model_type;

    assert (TLM_valid_model (model));
    assert (coder = TLM_valid_coder (coder));

    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	PPM_encode_symbol (model, context, coder, symbol);
	break;
      case TLM_PPMq_Model:
	PPMq_encode_symbol (model, context, coder, symbol);
	break;
      case TLM_PPMo_Model:
	PPMo_encode_symbol (model, context, coder, symbol);
	break;
      case TLM_SAMM_Model:
	SAMM_encode_symbol (model, context, coder, symbol);
	break;
      case TLM_PT_Model:
	PT_encode_symbol (model, context, coder, symbol);
	break;
      case TLM_HMM_Model:
	HMM_encode_symbol (model, context, coder, symbol);
	break;
      case TLM_SSS_Model:
	SSS_encode_symbol (model, context, coder, symbol);
	break;
      case TLM_TAG_Model:
	TAG_encode_symbol (model, context, coder, symbol);
	break;
      default:
	fprintf (stderr, "Function encode_symbol for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }
}

unsigned int
TLM_decode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder)
/* Returns the symbol decoded using the arithmetic coder. Updates the
   context record so that the last symbol in the context becomes the
   decoded symbol. */
{
    unsigned int model_type;

    assert (TLM_valid_model (model));
    assert (coder = TLM_valid_coder (coder));

    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	return (PPM_decode_symbol (model, context, coder));
	break;
      case TLM_PPMq_Model:
	return (PPMq_decode_symbol (model, context, coder));
	break;
      case TLM_PPMo_Model:
	return (PPMo_decode_symbol (model, context, coder));
	break;
      case TLM_SAMM_Model:
	return (SAMM_decode_symbol (model, context, coder));
	break;
      case TLM_PT_Model:
	return (PT_decode_symbol (model, context, coder));
	break;
      case TLM_HMM_Model:
	return (HMM_decode_symbol (model, context, coder));
	break;
      case TLM_SSS_Model:
	return (SSS_decode_symbol (model, context, coder));
	break;
      case TLM_TAG_Model:
	return (TAG_decode_symbol (model, context, coder));
	break;
      default:
	fprintf (stderr, "Function decode_symbol for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }
}

boolean
TLM_valid_model (unsigned int model)
/* Returns non-zero if the model is valid, zero otherwize. */
{
    if (model == NIL)
        return (FALSE);
    else if (model >= Models_unused)
        return (FALSE);
    else if (Models [model].M_version_no != TLM_Version_No)
        return (FALSE); /* The version number gets reset to 0 when the model gets deleted;
		       this way you can test to see if the model has been deleted or not */
    else
        return (TRUE);
}

unsigned int
TLM_verify_model (unsigned int model, unsigned int model_type,
		  boolean (*valid_model_function) (unsigned int))
/* Verifies the model_type and number for the model and returns the internal model number. */
{
    unsigned int model_no;

    assert (TLM_valid_model (model));
    assert (model_type == Models [model].M_model_type);

    model_no = Models [model].M_model;
    assert (valid_model_function (model_no));

    return (model_no);
}

unsigned int
TLM_valid_coder (unsigned int coder)
/* Returns a non-zero coder number if the coder is a valid coder,
   zero otherwize. */
{
    if (coder == NIL)
        return (NIL);
    else if (coder >= Coders_unused)
        return (NIL);
    else if (Coders [coder].A_max_frequency == 0)
        return (NIL); /* The max frequency number gets reset to 0 when the
			   model gets deleted; this way you can test to see
			   if the coder has been deleted or not */
    else
        return (coder);
}

unsigned int
TLM_create_model (unsigned int model_type, char *title, ...)
/* Creates a new empty dynamic model. Returns the new model number allocated
   to it if the model was created successfully, NIL if not.

   The title argument is intended to be a short human readable text description
   of the origins and content of the model.
*/
{
    va_list args; /* points to each unnamed argument in turn */
    unsigned int model, len, alphabet_size, escape_method, tagset_table;
    unsigned int start, step, stop, N, M, mask_orders;
    unsigned int create_context_function, release_context_function,
      update_context_function, valid_context_function;
    boolean performs_full_excls, performs_update_excls, has_multiple_chars_models;
    int max_order, tags_max_order, chars_max_order, os_model_threshold;
    int order_model_max_order, symbol_model_max_order;
    unsigned int model_no; /* Used to fix unknown (probably compiler) bug
			      in value returned by TAG_create_model () */

    va_start (args, title); /* make args point to first unnamed arg */

    model = create_model (); /* Get next unused model record */

    Models [model].M_model_type = model_type;
    Models [model].M_model_form = TLM_Dynamic; /* Model is dynamic */
    Models [model].M_version_no = TLM_Version_No;
    /* This is also used for testing later on if model no. is valid or not */

    assert (title != NULL);
    len = strlen (title);
    Models [model].M_title = (char *) Malloc (90, (len+1) * sizeof(char));
    strcpy (Models [model].M_title, title);

    Models [model].M_tag = NULL;

    /* Now process the parameters; for the PPM implementation, there
       are two, the maximum order of the model and whether the model performs
       exclusions or not */
    switch (model_type)
      {
      case TLM_PPM_Model:
	alphabet_size = va_arg (args, unsigned int);
	max_order = va_arg (args, int);
	escape_method = va_arg (args, unsigned int);
	performs_full_excls = va_arg (args, unsigned int);
	performs_update_excls = va_arg (args, unsigned int);
	Models [model].M_model =
	  PPM_create_model (alphabet_size, max_order, escape_method, performs_full_excls,
			    performs_update_excls);
	break;
      case TLM_PPMq_Model:
	alphabet_size = va_arg (args, unsigned int);
	max_order = va_arg (args, int);
	escape_method = va_arg (args, unsigned int);
	performs_full_excls = va_arg (args, unsigned int);
	Models [model].M_model =
	    PPMq_create_model (alphabet_size, max_order, escape_method, performs_full_excls);
	break;
      case TLM_PPMo_Model:
	alphabet_size = va_arg (args, unsigned int);
	order_model_max_order = va_arg (args, int);
	symbol_model_max_order = va_arg (args, int);
	mask_orders = va_arg (args, unsigned int);
	os_model_threshold = va_arg (args, int);
	performs_full_excls = va_arg (args, unsigned int);
	Models [model].M_model =
	    PPMo_create_model (alphabet_size, order_model_max_order, symbol_model_max_order,
			       mask_orders, os_model_threshold, performs_full_excls);
	break;
      case TLM_SAMM_Model:
	alphabet_size = va_arg (args, unsigned int);
	max_order = va_arg (args, int);
	escape_method = va_arg (args, unsigned int);
	Models [model].M_model =
	    SAMM_create_model (alphabet_size, max_order, escape_method);
	break;
      case TLM_PT_Model:
	Models [model].M_model = PT_create_model ();
	break;
      case TLM_HMM_Model:
	N = va_arg (args, unsigned int);
	M = va_arg (args, int);
	Models [model].M_model = HMM_create_model (N, M);
	break;
      case TLM_SSS_Model:
	start = va_arg (args, unsigned int);
	step = va_arg (args, unsigned int);
	stop = va_arg (args, unsigned int);
	Models [model].M_model =
	    SSS_create_model (start, step, stop);
	break;
      case TLM_TAG_Model:
	tagset_table = va_arg (args, unsigned int);
	tags_max_order = va_arg (args, int);
	chars_max_order = va_arg (args, int);
	has_multiple_chars_models = va_arg (args, int);
	model_no = TAG_create_model
	    (model, tagset_table, tags_max_order, chars_max_order, has_multiple_chars_models);
	Models [model].M_model = model_no;
	break;
      case TLM_USER_Model:
	/* store function pointers as an unsigned int to be recast */
	create_context_function = va_arg (args, unsigned int);
	release_context_function = va_arg (args, unsigned int);
	update_context_function = va_arg (args, unsigned int);
	valid_context_function = va_arg (args, unsigned int);
	Models [model].M_model = USER_create_model
	  ((unsigned int (*) (unsigned int)) create_context_function,
	   (void (*) (unsigned int, unsigned int)) release_context_function,
	   (void (*) (unsigned int, unsigned int, unsigned int)) update_context_function,
	   (boolean (*) (unsigned int)) valid_context_function);
	break;
      default:
	fprintf (stderr, "Function create_model for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }

    va_end (args);

    return (model);
}

unsigned int
TLM_create_PPM_model (char *title, unsigned int alphabet_size, int max_order,
		      unsigned int escape_method, unsigned int performs_full_excls,
		      unsigned int performs_update_excls)
/* Creates a new empty dynamic PPM model. Returns the new model number allocated
   to it if the model was created successfully, NIL if not.

   The title argument is intended to be a short human readable text description
   of the origins and content of the model.
   alphabet_size is the size of the alphabet.
   max_order is the maximum order of the PPM model.
   escape_method is the escape method being used by the model. e.g. TLM_PPM_Method_C for PPMC and TLM_PPM_Method_D for PPMD.
   performs_full_excls is TRUE if the model is to perform full exclusions.
   performs_update_excls is TRUE if the model is to perform exclusions.
*/
{
    unsigned int model;

    model = TLM_create_model (TLM_PPM_Model, title, alphabet_size, max_order, escape_method, performs_full_excls, performs_update_excls);
    return (model);
}

boolean
TLM_get_model_type (unsigned int model, unsigned int *model_type,
		    unsigned int *model_form, char **title)
/* Returns information describing the model. Returns NIL if the model does not
   exist (and leaves the other parameters unmodified in this case), non-zero
   otherwise.  The arguments title and model_type are the
   values used to create the model in TLM_create_model().
   The argument model_form is set to TLM_Dynamic or TLM_Static depending on
   whether the model is static or dynamic. */
{
    unsigned int modeltype;

    assert (TLM_valid_model (model));

    modeltype = Models [model].M_model_type;
    *model_type = modeltype;
    *title = Models [model].M_title;
    *model_form = Models [model].M_model_form;

    return (model); /* This model exists */
}

boolean
TLM_get_PPM_model (unsigned int model)
/* Returns information describing the PPM model such as the title for the model  
   and the model_form which is TLM_Dynamic or TLM_Static depending on whether
   the model is static or dynamic. The arguments title and model_type are the
   values used to create the model in TLM_create_PPM_model(). */
{
    unsigned int ppm_model;
  
    assert (TLM_valid_model (model));

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    TLM_PPM_title = Models [model].M_title;
    TLM_PPM_model_form = Models [model].M_model_form;
    TLM_PPM_alphabet_size = PPM_Models [ppm_model].P_alphabet_size;
    TLM_PPM_max_order = PPM_Models [ppm_model].P_max_order;
    TLM_PPM_escape_method = PPM_Models [ppm_model].P_escape_method;
    TLM_PPM_performs_full_excls = PPM_Models [ppm_model].P_performs_full_excls;
    TLM_PPM_performs_update_excls = PPM_Models [ppm_model].P_performs_update_excls;

    return (model); /* This model exists */
}

boolean
TLM_get_model (unsigned int model, ...)
/* Returns information describing the model. Returns NIL if the model does not
   exist (and leaves the other parameters unmodified in this case), non-zero
   otherwise. The argument model is followed by a variable number of parameters
   used to hold model information which differs between implementations of the
   different model types. */
{
    va_list args; /* points to each unnamed argument in turn */
    unsigned int model_type, type, *value;
    unsigned int *tagset_table, *words_table, *words_index;
    unsigned int *tags_alphabet_size, *tags_model;
    unsigned int *chars_model, **chars_models;
    unsigned int *start, *step, *stop;
    unsigned int *N, *M;

    va_start (args, model); /* make arags point to first unnamed argument */

    assert (TLM_valid_model (model));

    model_type = Models [model].M_model_type;

    switch (model_type)
      {
      case TLM_PPM_Model:
	type = va_arg (args, unsigned int);
	value = va_arg (args, unsigned int *);
	PPM_get_model (model, type, value);
	break;
      case TLM_PPMq_Model:
	type = va_arg (args, unsigned int);
	value = va_arg (args, unsigned int *);
	PPMq_get_model (model, type, value);
	break;
      case TLM_PPMo_Model:
	type = va_arg (args, unsigned int);
	value = va_arg (args, unsigned int *);
	PPMo_get_model (model, type, value);
	break;
      case TLM_SAMM_Model:
	type = va_arg (args, unsigned int);
	value = va_arg (args, unsigned int *);
	SAMM_get_model (model, type, value);
	break;
      case TLM_HMM_Model:
	N = va_arg (args, unsigned int *);
	M = va_arg (args, unsigned int *);
	HMM_get_model (Models [model].M_model, N, M);
	break;
      case TLM_SSS_Model:
	start = va_arg (args, unsigned int *);
	step = va_arg (args, unsigned int *);
	stop = va_arg (args, unsigned int *);
	SSS_get_model (Models [model].M_model, start, step, stop);
	break;
      case TLM_TAG_Model:
	tagset_table = va_arg (args, unsigned int *);
	words_table = va_arg (args, unsigned int *);
	words_index = va_arg (args, unsigned int *);
	tags_alphabet_size = va_arg (args, unsigned int *);
	tags_model = va_arg (args, unsigned int *);
	chars_model = va_arg (args, unsigned int *);
	chars_models = va_arg (args, unsigned int **);
	TAG_get_model (Models [model].M_model, tagset_table, words_table,
		       words_index, tags_alphabet_size, tags_model,
		       chars_model, chars_models);
	break;
      default:
	fprintf (stderr, "Function get_model for model type %d not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }

    va_end (args);

    return (model); /* This model exists */
}

void
TLM_set_model (unsigned int model, ...)
/* Sets information that describes the model. The argument model
   is followed by a variable number of parameters used to hold model
   information which differs between implementations of the different
   model types. For example, the implementation of a TLM_PPM_Model uses it to
   specify the new (extended) size of the alphabet and the maximum
   symbol number for which statistics will be updated (symbols that
   exceed this number will become "static" symbols). */

{
    va_list args; /* points to each unnamed argument in turn */
    unsigned int model_type, type, value;

    va_start (args, model); /* make args point to first unnamed argument */

    assert (TLM_valid_model (model));
    model_type = Models [model].M_model_type;

    switch (model_type)
      {
      case TLM_PPM_Model:
	type = va_arg (args, unsigned int);
	value = va_arg (args, unsigned int);
	PPM_set_model (Models [model].M_model, type, value);
	break;
      case TLM_PPMq_Model:
	type = va_arg (args, unsigned int);
	value = va_arg (args, unsigned int);
	PPMq_set_model (Models [model].M_model, type, value);
	break;
      default:
	fprintf (stderr, "Function set_model for model type %d not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }

    va_end (args);
}

void
TLM_dump_model (unsigned int file, unsigned int model,
		void (*dump_symbol_function) (unsigned int, unsigned int))
/* Prints a human readable version of the model (intended mainly for debugging).
   The argument dump_symbol_function is a pointer to a function for printing symbols.
   If this is NULL, then each symbol will be printed as an unsigned int surrounded by
   angle brackets (e.g. <123>), unless it is human readable ASCII, in which case it will
   be printed as a char. */
{
    unsigned int model_type;

    assert (TXT_valid_file (file));
    assert (TLM_valid_model (model));

    fprintf (Files [file], "Model number %d\n", model);
    fprintf (Files [file], "----------------\n");

    model_type = Models [model].M_model_type;
    fprintf (Files [file], "Title = %s", Models [model].M_title);
    if (Models [model].M_tag)
        fprintf (Files [file], " (tag = %s)", Models [model].M_tag);
    fprintf (Files [file], "\n\n");

    if (Models [model].M_model_form == TLM_Dynamic)
        fprintf (Files [file], "Model is dynamic.\n");
    else
        fprintf (Files [file], "Model is static.\n");
    fprintf (Files [file], "Version number = %d\n",
	     Models [model].M_version_no);

    switch (model_type)
      {
      case TLM_PPM_Model:
        fprintf (Files [file],
		 "\nModel type is PPM. PPM-based information follows:\n");
	PPM_dump_model (file, Models [model].M_model, TRUE,
			dump_symbol_function);
	break;
      case TLM_PPMq_Model:
        fprintf (Files [file],
		 "\nModel type is PPMq. PPMq-based information follows:\n");
	PPMq_dump_model (file, Models [model].M_model,
			dump_symbol_function);
	break;
      case TLM_PPMo_Model:
        fprintf (Files [file],
		 "\nModel type is PPMo. PPMo-based information follows:\n");
	PPMo_dump_model (file, Models [model].M_model,
			dump_symbol_function);
	break;
      case TLM_SAMM_Model:
        fprintf (Files [file],
		 "\nModel type is PPM-fast. PPM-based information follows:\n");
	SAMM_dump_model (file, Models [model].M_model, dump_symbol_function);
	break;
      case TLM_PT_Model:
        fprintf (Files [file],
		 "\nModel type is PT. PT-based information follows:\n");
	PT_dump_model (file, Models [model].M_model);
	break;
      case TLM_HMM_Model:
        fprintf (Files [file],
		 "\nModel type is HMM. HMM-based information follows:\n");
	HMM_dump_model (file, Models [model].M_model);
	break;
      case TLM_SSS_Model:
        fprintf (Files [file],
		 "\nModel type is SSS. SSS-based information follows:\n");
	SSS_dump_model (file, Models [model].M_model);
	break;
      case TLM_TAG_Model:
        fprintf (Files [file],
		 "\nModel type is TAG. TAG-based information follows:\n");
	TAG_dump_model (file, Models [model].M_model);
	break;
      case TLM_USER_Model:
        fprintf (Files [file],
		 "\nModel type is USER defined. USER-defined information follows:\n");
	USER_dump_model (file, Models [model].M_model);
	break;
      default:
	fprintf (stderr, "Function dump_model for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }
}

void
TLM_dump_PPM_model (unsigned int file, unsigned int model)
/* Prints a human readable version of the PPM model (intended mainly for debugging). */
{
    fprintf (Files [file],
	     "\nModel type is PPM. PPM-based information follows:\n");
    PPM_dump_model (file, Models [model].M_model, TRUE, NULL);
}


void
TLM_check_model (unsigned int file, unsigned int model,
		 void (*dump_symbol_function) (unsigned int, unsigned int))
/* Checks the model is consistent (for debugging purposes). */
{
    unsigned int model_type;

    assert (TXT_valid_file (file));
    assert (TLM_valid_model (model));

    model_type = Models [model].M_model_type;

    switch (model_type)
      {
      case TLM_SAMM_Model:
	SAMM_check_model (file, Models [model].M_model, dump_symbol_function);
	break;
      default:
	fprintf (stderr, "Function check_model for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }
}

void
TLM_set_load_operation (unsigned int load_operation, ...)
/* Sets the type of operation to be performed by the routine
   TLM_load_model. The argument is followed by a variable number of parameters
   used to hold various information depending on the load_operation parameter:
     TLM_Load_Change_Title
	 This specifies that the following parameter is used to change the
         title of the model after it gets loaded.
     TLM_Load_Change_Model_Type
         The following parameter is used to specify the new type that the model
         is transformed into as it is loaded. */
{
    va_list args; /* points to each unnamed argument in turn */
    unsigned int len;
    char *title;

    va_start (args, load_operation); /* make arags point to first unnamed argument */

    switch (load_operation)
      {
      case TLM_Load_Change_Title:
	TLM_Load_Do_ChangeTitle = TRUE;
	title = va_arg (args, char *);
	len = strlen (title);
	TLM_Load_Title = (char *) Malloc (90, (len+1) * sizeof(char));
	strcpy (TLM_Load_Title, title);
	break;
      case TLM_Load_Change_Model_Type:
	TLM_Load_Do_ChangeModelType = TRUE;
	TLM_Load_Model_Type = va_arg (args, unsigned int);
	break;
      default:
	fprintf (stderr, "Invalid load operation %d - not yet implemented\n",
		 load_operation);
	exit (1);
	break;
      }

    va_end (args);
}

unsigned int
TLM_load_model (unsigned int file)
/* Loads a model which has been previously saved to the file into memory and
   allocates it a new model number which is returned. */
{
  unsigned int model, model_type, model_form;
    unsigned int version_no;
    unsigned int model_no; /* Used to fix unknown (probably compiler) bug
			      in value returned by TAG_create_model () */

    assert (TXT_valid_file (file));

    /* read in the version number of the model */
    version_no = fread_int (file, INT_SIZE);
    assert (version_no == TLM_Version_No); /* Do not allow models with different version
					  numbers to be loaded at the moment */

    model = create_model (); /* Get next unused model record */
    assert (model != NIL);

    /* set the version number of the model */
    Models [model].M_version_no = version_no;

    /* read in the model's type (e.g. TLM_PPM_model, TLM_PCFG_model etc.) */
    model_type = fread_int (file, INT_SIZE);
    if (TLM_Load_Do_ChangeModelType)
        model_type = TLM_Load_Model_Type;
    Models [model].M_model_type = model_type;

    /* read in the model's form (i.e. either TLM_Static or TLM_Dynamic) */
    model_form = fread_int (file, INT_SIZE);
    Models [model].M_model_form = model_form;

    /* read in (or replace) the long description (title) of the model */
    Models [model].M_title = fread_str (file);
    if (TLM_Load_Do_ChangeTitle)
        Models [model].M_title = TLM_Load_Title;
    switch (model_type)
      {
      case TLM_PPM_Model:
	Models [model].M_model =
	    PPM_load_model (file, model_form);
	break;
      case TLM_PPMo_Model:
	Models [model].M_model =
	    PPMo_load_model (file, model_form);
	break;
      case TLM_SAMM_Model:
	Models [model].M_model =
	    SAMM_load_model (file, model_form);
	break;
      case TLM_PT_Model:
	Models [model].M_model =
	    PT_load_model (file, model_form);
	break;
      case TLM_HMM_Model:
	Models [model].M_model = HMM_load_model (file, model_form);
	break;
      case TLM_SSS_Model:
	Models [model].M_model = SSS_load_model (file);
	break;
      case TLM_TAG_Model:
	model_no = TAG_load_model (file, model, model_form);
	Models [model].M_model = model_no;
	break;
      default:
	fprintf (stderr, "Invalid model type %d\n", model_type);
	exit (1);
	break;
      }

    /* reset the next load operation to do nothing extra by default */
    TLM_Load_Do_ChangeTitle = FALSE; 
    TLM_Load_Do_ChangeModelType = FALSE; 
    return (model);
}

#define TAG_SIZE 256      /* Maximum size of a tag */
#define FILENAME_SIZE 256 /* Maximum size of a filename */

void
TLM_load_models (char *filename)
/* Load the models and their associated tags from the specified file. */
{
    char tag [TAG_SIZE], model_filename [FILENAME_SIZE];
    unsigned int model;
    FILE *fp;

    fprintf (stderr, "Loading models from file %s\n", filename);
    if ((fp = fopen (filename, "r")) == NULL)
      {
	fprintf (stderr, "TLM: can't open models file %s\n", filename);
	exit (1);
      }

    while ((fscanf (fp, "%s %s", tag, model_filename) != EOF))
    {
	model = TLM_read_model
	    (model_filename, "Loading training model from file",
	     "TLM: can't open training file");
	TLM_set_tag (model, tag);
    }
    fclose (fp);
}

unsigned int
TLM_read_model (char *filename, char *debug_line, char *error_line)
/* Reads in the model directly by loading the model from the file
   with the specified filename into a new model and returns it. */
{
    unsigned int file, model;

    file = TXT_open_file (filename, "r", debug_line, error_line);
    assert (TXT_valid_file (file));

    model = TLM_load_model (file);
    assert (TLM_valid_model (model));

    TXT_close_file (file);

    return (model);
}

void
TLM_set_write_operation (unsigned int write_operation, ...)
/* Sets the type of operation to be performed by the routine
   TLM_write_model. The argument is followed by a variable number of parameters
   used to hold various information depending on the write_operation parameter:
     TLM_Write_Change_Model_Type
         The following parameter is used to specify the new type that the model
         is transformed into as it is written out. */
{
    va_list args; /* points to each unnamed argument in turn */

    va_start (args, write_operation); /* make arags point to first unnamed argument */

    switch (write_operation)
      {
      case TLM_Write_Change_Model_Type:
	TLM_Write_Do_ChangeModelType = TRUE;
	TLM_Write_Model_Type = va_arg (args, unsigned int);
	break;
      default:
	fprintf (stderr, "Invalid write operation %d - not yet implemented\n",
		 write_operation);
	exit (1);
	break;
      }

    va_end (args);
}

void
TLM_write_model (unsigned int file, unsigned int model,
		 unsigned int model_form)
/* Writes out the model to the file (which can then be loaded
   by other applications later). The argument model_form must have the value
   TLM_Static or TLM_Dynamic and determines whether the model is static or
   dynamic when it is later reloaded using TLM_load_model. */
{
    unsigned int model_type;

    assert (TXT_valid_file (file));
    assert (TLM_valid_model (model));

    /* write out the version number to identify the model type */
    fwrite_int (file, TLM_Version_No, INT_SIZE);

    /* write out the model's type (i.e. TLM_PPM_Model or TLM_PCFG_Model) */
    model_type = Models [model].M_model_type;
    if (TLM_Write_Do_ChangeModelType)
        model_type = TLM_Write_Model_Type; /* Model changes to new type when it is written out */
    fwrite_int (file, model_type, INT_SIZE);

    /* write out the model's form (i.e. either TLM_Static or TLM_Dynamic) */
    fwrite_int (file, model_form, INT_SIZE);

    /* write out the long description (title) of the model */
    fwrite_str (file, Models [model].M_title);

    switch (model_type)
      {
      case TLM_PPM_Model:
	PPM_write_model (file, model, model_form);
	break;
      case TLM_PPMo_Model:
	PPMo_write_model (file, model, model_form);
	break;
      case TLM_SAMM_Model:
	SAMM_write_model (file, model, model_form, model_type);
	break;
      case TLM_PT_Model:
	PT_write_model (file, model, model_form);
	break;
      case TLM_HMM_Model:
	HMM_write_model (file, model, model_form);
	break;
      case TLM_SSS_Model:
	SSS_write_model (file, model);
	break;
      case TLM_TAG_Model:
	TAG_write_model (file, model, model_form);
	break;
      default:
	fprintf (stderr, "Function write_model for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }

    /* reset the next write operation to do nothing extra by default */
    TLM_Write_Do_ChangeModelType = FALSE; 
}

void
TLM_release_model (unsigned int model)
/* Releases the memory allocated to the model and the model number (which may
   be reused in later TLM_create_model or TLM_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active contexts pointing at it. */
{
    unsigned int model_form;
    unsigned int mnext, mprev;
    unsigned int model_type;

    assert (TLM_valid_model (model));

    model_form = Models [model].M_model_form;
    if ((model_form == TLM_Static) && (Models [model].M_contexts > 0))
      {
	fprintf (stderr, "Fatal error: cannot delete model %d as it still has active PPM_Contexts pointing to it\n",
		 model);
	exit (1);
      }

    /* Model must be non-NIL here; delete model from the doubly linked list */
    mnext = Models [model].M_next;
    mprev = Models [model].M_prev;
    if (mnext)
        Models [mnext].M_prev = mprev;
    else
        Models_tail = mprev;
    if (mprev)
        Models [mprev].M_next = mnext;
    else
        Models_head = mnext;

    /* add model record at the head of the Models_used list */
    Models [model].M_next = Models_used;
    Models_used = model;
    Models [model].M_prev = NIL; /* Not used for the Models_used list */

    Models [model].M_version_no = 0; /* Used for testing if model no. is valid or not */

    Models_count--;

    /* release any further local data specific to the model's type as well */
    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	PPM_release_model (Models [model].M_model);
	break;
      case TLM_PPMq_Model:
	PPMq_release_model (Models [model].M_model);
	break;
      case TLM_PPMo_Model:
	PPMo_release_model (Models [model].M_model);
	break;
      case TLM_SAMM_Model:
	SAMM_release_model (Models [model].M_model);
	break;
      case TLM_PT_Model:
	PT_release_model (Models [model].M_model);
	break;
      case TLM_HMM_Model:
	HMM_release_model (Models [model].M_model);
	break;
      case TLM_SSS_Model:
	SSS_release_model (Models [model].M_model);
	break;
      case TLM_TAG_Model:
	TAG_release_model (Models [model].M_model);
	break;
      case TLM_USER_Model:
	USER_release_model (Models [model].M_model);
	break;
      default:
	fprintf (stderr, "Function release_model for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }
}

unsigned int
TLM_copy_model (unsigned int model)
/* Copies the model. */
{
    unsigned int new_model, new_ppm_model, model_type, len;

    assert (TLM_valid_model (model));

    new_model = NIL;
    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	new_ppm_model = PPM_copy_model (model);
	break;
      default:
	fprintf (stderr, "Function copy_model for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }

    new_model = create_model (); /* Get next unused model record */

    Models [new_model].M_model_type = new_ppm_model;

    Models [new_model].M_model_type = Models [model].M_model_type;
    Models [new_model].M_model_form = Models [model].M_model_form;
    Models [new_model].M_version_no = Models [model].M_version_no;

    if (Models [model].M_title != NULL)
      {
        len = strlen (Models [model].M_title);
	Models [new_model].M_title = (char *) Malloc (90, (len+1) * sizeof(char));
	strcpy (Models [new_model].M_title, Models [model].M_title);
      }

    if (Models [model].M_tag != NULL)
      {
        len = strlen (Models [model].M_tag);
	Models [new_model].M_tag = (char *) Malloc (90, (len+1) * sizeof(char));
	strcpy (Models [new_model].M_tag, Models [model].M_tag);
      }

    return (new_model);
}

void
TLM_nullify_model (unsigned int model)
/* Replaces the model with the null model and releases the memory allocated
   to it. */
{
    unsigned int model_type;

    assert (TLM_valid_model (model));

    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	/* Release the model's trie as well */
	PPM_nullify_model (model);
	break;
      case TLM_HMM_Model:
	HMM_nullify_model (model);
      case TLM_SSS_Model:
	/* No need to do anything as an SSS model uses a "null"
	   model by default */
	break;
      default:
	fprintf (stderr, "Function nullify_model for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }
}

unsigned int
TLM_numberof_models (void)
/* Returns the number of currently valid models. */
{
    return (Models_count);
}

void
dump_modelnos (void)
/* Dumps out the model numbers. */
{
    unsigned int m;

    m = Models_head;
    while (m)
      {
	fprintf (stderr, "model number %d\n", m);
	m = Models [m].M_next;
      }
}

void
TLM_reset_modelno (void)
/* Resets the current model number so that the next call to TLM_next_modelno
   will return the first valid model number (or NIL if there are none). */
{
    Models_ptr = Models_head;
}

unsigned int
TLM_next_modelno (void)
/* Returns the model number of the next valid model. Returns NIL if
   there isn't any. */
{
    unsigned int m;

    m = Models_ptr;
    if (m)
        Models_ptr = Models [m].M_next;
    return (m);
}

unsigned int
TLM_getcontext_position (unsigned int model, unsigned int context)
/* Returns an integer which uniquely identifies the current position
   associated with the model's context. (One implementation is to return
   a memory location corresponding to the current position. This routine is
   useful if you need to check whether different contexts have encoded
   the same prior symbols as when checking whether the context pathways
   converge in the Viterbi or trellis-based algorithms.) */
{
    unsigned int model_type;

    assert (TLM_valid_model (model));

    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	return (PPM_getcontext_position (model, context));
	break;
      case TLM_PPMo_Model:
	return (PPMo_getcontext_position (model, context));
	break;
      case TLM_HMM_Model:
	return (HMM_getcontext_position (model, context));
	break;
      case TLM_SSS_Model:
	return (context); /* Just return the context itself */
	break;
      case TLM_TAG_Model:
	return (TAG_getcontext_position (model, context));
	break;
      default:
	fprintf (stderr, "Function getcontext_position for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }

    return (0);
}

char *
TLM_get_title (unsigned int model)
/* Return the title associated with the model. */
{
    assert (TLM_valid_model (model));

    return (Models [model].M_title);
}

void
TLM_set_tag (unsigned int model, char *tag)
/* Sets the tag associated with the model. */
{
    unsigned int len;

    assert (TLM_valid_model (model));

    assert (tag != NULL);
    len = strlen (tag);
    Models [model].M_tag = (char *) Malloc (91, (len+1) * sizeof(char));
    strcpy (Models [model].M_tag, tag);
}

char *
TLM_get_tag (unsigned int model)
/* Return the tag associated with the model. */
{
    assert (TLM_valid_model (model));

    if (Models [model].M_tag)
        return (Models [model].M_tag);
    else if (Models [model].M_title)
        return (Models [model].M_title);
    else
        return ("<Unknown Tag>");
}

unsigned int
TLM_getmodel_tag (char *tag)
/* Returns the model associated with the model's tag. If the tag
   occurs more than once, it will return the lowest model number. */
{
    unsigned int m;

    TLM_reset_modelno ();
    while ((m = TLM_next_modelno ()))
      {
	if (!strcmp (tag, Models [m].M_tag))
	  return (m);
      }

    return (NIL); /* Tag not found */
}

unsigned int
TLM_minlength_model (unsigned int model)
/* Returns the minimum number of bits needed to write the model
   out to disk as a static model and recover it later. This is
   useful for computing minimum description lengths of messages.

   Note that the amount returned will always be slightly less than
   the resulting size of the static model produced by TLM_write_model
   as this excludes extra overhead data (including the model's title)
   that is necessary for the functioning of the API. */
{
    unsigned int model_type;

    assert (TLM_valid_model (model));

    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	return (PPM_minlength_model (model));
	break;
      case TLM_HMM_Model:
	return (HMM_minlength_model (model));
	break;
      case TLM_SSS_Model:
	return (SSS_sizeof_model (model));
	break;
      default:
	fprintf (stderr, "Function minlength_model for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }

    return (0);
}

unsigned int
TLM_sizeof_model (unsigned int model)
/* Returns the current number of bits needed to store the
   model in memory. */
{
    unsigned int model_type;

    assert (TLM_valid_model (model));

    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	return (PPM_sizeof_model (model));
	break;
      case TLM_SAMM_Model:
	return (SAMM_sizeof_model (model));
	break;
      case TLM_HMM_Model:
	return (HMM_sizeof_model (model));
	break;
      case TLM_SSS_Model:
	return (SSS_sizeof_model (model));
	break;
      default:
	fprintf (stderr, "Function sizeof_model for model type %d is not yet implemented\n", 
		 model_type);
	exit (1);
	break;
      }

    return (0);
}

void
TLM_dump_models (unsigned int file, void (*dump_symbol_function) (unsigned int, unsigned int))
/* Writes a human readable version of all the currently valid models to the
   file. The argument dump_symbol_function is a pointer to a function for printing symbols.
   If this is NULL, then each symbol will be printed as an unsigned int surrounded by
   angle brackets (e.g. <123>), unless it is human readable ASCII, in which case it will
   be printed as a char. */
{
    unsigned int m;

    assert (TXT_valid_file (file));

    fprintf (Files [file], "Dump of models:\n");
    fprintf (Files [file], "---------------\n");
    TLM_reset_modelno ();
    while ((m = TLM_next_modelno ()))
	TLM_dump_model (file, m, dump_symbol_function);
    fprintf (Files [file], "---------------\n");
}

void
TLM_dump_models1 (unsigned int file)
/* Writes a human readable version of all the currently valid models to the
   file. */
{
    TLM_dump_models (file, NULL);
}

void
TLM_release_models (void)
/* Releases the memory used by all the models. */
{
    unsigned int m;

    /*fprintf (stderr, "Releasing models:\n");*/
    TLM_reset_modelno ();
    while ((m = TLM_next_modelno ()))
	TLM_release_model (m);
}

void
TLM_stats_model (unsigned int file, unsigned int model)
/* Writes out statistics about the model in human readable form. */
{
    unsigned int model_type;

    assert (TXT_valid_file (file));
    assert (TLM_valid_model (model));

    model_type = Models [model].M_model_type;
    switch (model_type)
      {
      case TLM_PPM_Model:
	PPM_stats_model (file, model);
	break;
      case TLM_HMM_Model:
	HMM_stats_model (file, model);
	break;
      case TLM_SSS_Model:
	SSS_stats_model (file, model);
	break;
      default:
	fprintf (stderr, "Function stats_model for model type %d is not yet implemented\n",
		 model_type);
	exit (1);
	break;
      }
}
