/* TLM routines based on PT models. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "io.h"
#include "text.h"
#include "model.h"
#include "pt_model.h"

#define PT_MODELS_SIZE 4                   /* Initial max. number of models */
#define PT_DECODE_CONTEXTS_SIZE 20         /* Max. number of contexts; required
					      during decoding to defer update
					      of tables until symbols are known */

/* Global variables used for storing the PT models */
struct PT_modelType *PT_Models = NULL;     /* List of PT models */
unsigned int PT_Models_max_size = 0;       /* Current max. size of models array */
unsigned int PT_Models_used = NIL;         /* List of deleted model records */
unsigned int PT_Models_unused = 1;         /* Next unused model record */

unsigned int PT_Novel_Symbols = FALSE;     /* Used to incidate when new symbols are novel */

struct PT_decodeContextType
{ /* PT decode context record used to store information so that update of contexts
     can be easily deferred until the symbols have been decoded */
    struct PTc_table_type *PT_table;       /* Probability table that needs to be
					      updated */
    unsigned int PT_context_text;          /* The context */
};
struct PT_decodeContextType PT_Decode_Contexts [PT_DECODE_CONTEXTS_SIZE];
unsigned int PT_Decode_Contexts_Count = 0; /* Current size of contexts that have had
					      their update deferred until symbols are
					      decoded */
boolean PT_Decode_Contexts_Init = FALSE;   /* TRUE when the Decode_Contexts have been
					      initialised */

boolean
PT_valid_model (unsigned int model)
/* Returns non-zero if the PT model is valid, zero otherwize. */
{
    if (model == NIL)
        return (FALSE);
    else if (model >= PT_Models_unused)
        return (FALSE);
    else if (PT_Models [model].PT_deleted)
        /* This gets set to TRUE when the model gets deleted */
        return (FALSE);
    else
        return (TRUE);
}

unsigned int
create_PT_model (void)
/* Creates and returns a new pointer to a PT model record. */
{
    unsigned int model, old_size;

    if (PT_Models_used != NIL)
    { /* use the first record on the used list */
        model = PT_Models_used;
	PT_Models_used = PT_Models [model].PT_next;
    }
    else
    {
	model = PT_Models_unused;
        if (PT_Models_unused >= PT_Models_max_size)
	{ /* need to extend PT_Models array */
	    old_size = PT_Models_max_size * sizeof (struct modelType);
	    if (PT_Models_max_size == 0)
	        PT_Models_max_size = PT_MODELS_SIZE;
	    else
	        PT_Models_max_size = 10*(PT_Models_max_size+50)/9;

	    PT_Models = (struct PT_modelType *)
	        Realloc (124, PT_Models, PT_Models_max_size *
			 sizeof (struct PT_modelType), old_size);

	    if (PT_Models == NULL)
	    {
	        fprintf (stderr, "Fatal error: out of PT models space\n");
		exit (1);
	    }
	}
	PT_Models_unused++;
    }

    return (model);
}

unsigned int
PT_create_model (void)
/* Creates and returns a new pointer to a PT model record. */
{
    unsigned int pt_model;

    pt_model = create_PT_model ();

    PT_Models [pt_model].PT_table = PTc_create_table ();

    PT_Models [pt_model].PT_deleted = FALSE;

    PT_Models [pt_model].PT_next = NIL;

    return (pt_model);
}

void
PT_release_model (unsigned int pt_model)
/* Releases the memory allocated to the model and the model number (which may
   be reused in later PT_create_model or PT_load_model calls). */
{
    assert (PT_valid_model (pt_model));

    /* add model record at the head of the PT_Models_used list */
    PT_Models [pt_model].PT_next = PT_Models_used;
    PT_Models_used = pt_model;

    PT_Models [pt_model].PT_deleted = TRUE; /* Used for testing if model no. is valid or not */
}

unsigned int
PT_load_model (unsigned int file, unsigned int model_form)
/* Loads the PT model which has been previously saved to the file into memory
   and allocates it a new model number which is returned. */
{
    unsigned int pt_model;

    assert (TXT_valid_file (file));
    assert (model_form == TLM_Static);

    pt_model = PT_create_model (); /* Get next unused PT model record */
    assert (PT_valid_model (pt_model));

    /* Read in the tables */
    /* fprintf (stderr, "Reading in the table...\n");*/
    PT_Models [pt_model].PT_table = PTc_load_table (file);

    return (pt_model);
}

void
PT_write_model (unsigned int file, unsigned int model,
		unsigned int model_form)
/* Writes out the PT model to the file as a static PT model (note: not fast PT) 
   (which can then be loaded by other applications later). */
{
    unsigned int pt_model;

    pt_model = TLM_verify_model (model, TLM_PT_Model, PT_valid_model);

    assert (TXT_valid_file (file));
    assert (model_form == TLM_Static);

    /* write out the tables */
    /* fprintf (stderr, "Writing out the ctable...\n");*/
    assert (PT_Models [pt_model].PT_table != NULL);
    PTc_write_table (file, PT_Models [pt_model].PT_table, model_form);
}

void
PT_dump_model (unsigned int file, unsigned int pt_model)
/* Dumps out the PT model (for debugging purposes). */
{
    assert (PT_valid_model (pt_model));

    fprintf (Files [file], "\nDump of table:\n");
    PTc_dump_table (file, PT_Models [pt_model].PT_table);
}

void
PT_update_decode_context1 (struct PTc_table_type *table, unsigned int context_text)
/* Performs all the updates that were deferred during decoding
   that could not be performed until the symbols_text has been
   decoded. */
{
    unsigned int p;

    assert (PT_Decode_Contexts_Count < PT_DECODE_CONTEXTS_SIZE);

    if (!PT_Decode_Contexts_Init)
      {
	unsigned int p;

 	PT_Decode_Contexts_Init = TRUE;
 	for (p = 0; p < PT_DECODE_CONTEXTS_SIZE; p++)
 	  PT_Decode_Contexts [p].PT_context_text = TXT_create_text ();
      }

    p = PT_Decode_Contexts_Count;
    PT_Decode_Contexts [p].PT_table = table;
    TXT_overwrite_text (PT_Decode_Contexts [p].PT_context_text, context_text);

    PT_Decode_Contexts_Count++;
}

void
PT_encode_position (unsigned int model, unsigned int context_text,
		    unsigned int symbols_text, codingType coding_type,
		    unsigned int coder)
/* Updates the PT context record for the current symbols_text.
   symbols_text is assumed to be a text record i.e. a sequence of text symbols.
   Returns additional information as specified by the routine
   TLM_set_context_operation. */
{
    unsigned int model_form, pt_model, key;
    unsigned int target, lbnd = 0, hbnd = 0, totl = 0;
    boolean decoded_escape = FALSE; /* TRUE if decoded an escape during decoding */

    pt_model = TLM_verify_model (model, TLM_PT_Model, PT_valid_model);

    assert (context_text != NIL);

    model_form = Models [model].M_model_form;

    if (coding_type != DECODE_TYPE)
        PTc_encode_arith_range (PT_Models [pt_model].PT_table, NULL/*No exclusions*/,
				context_text, symbols_text, &lbnd, &hbnd, &totl);

    switch (coding_type)
      {
      case FIND_CODELENGTH_TYPE:
      case UPDATE_TYPE:
	if (totl)
	    TLM_Codelength = Codelength (lbnd, hbnd, totl);
	break;
      case ENCODE_TYPE:
	if (totl)
	    Coders [coder].A_arithmetic_encode
	      (Coders [coder].A_encoder_output_file, lbnd, hbnd, totl);
	break;
      case DECODE_TYPE:
	TXT_setlength_text (symbols_text, 0);
	totl = PTc_decode_arith_total
	  (PT_Models [pt_model].PT_table, NULL/*No exclusions*/, context_text);
	if (!totl)
	    decoded_escape = TRUE;
	else
	  {
	    target = Coders [coder].A_arithmetic_decode_target
	      (Coders [coder].A_decoder_input_file, totl);
	    key = PTc_decode_arith_key
	      (PT_Models [pt_model].PT_table, NULL/*No exclusions*/, context_text,
	       target, totl, &lbnd, &hbnd);
	    Coders [coder].A_arithmetic_decode
	      (Coders [coder].A_decoder_input_file, lbnd, hbnd, totl);
	    if (lbnd == 0)
	        decoded_escape = TRUE;
	    else
	        TXT_overwrite_text (symbols_text, key);
	  }
	break;
      default:
	break;
      }

    if (Debug.range)
      {
        fprintf (stderr, "PT Model %d Context symbols: {", model);
	if (context_text == NIL)
	    fprintf (stderr, "NIL");
	else
	    TXT_dump_text (Stderr_File, context_text, TXT_dump_symbol);
        fprintf (stderr, "} Word symbols: {");
        TXT_dump_text (Stderr_File, symbols_text, TXT_dump_symbol);
        fprintf (stderr, "} Coding range: lbnd %d hbnd %d totl %d codelength %.3f\n",
		 lbnd, hbnd, totl, Codelength (lbnd, hbnd, totl));
      }

    if (coding_type != FIND_CODELENGTH_TYPE)
      {
	if (lbnd == 0)
	    PT_Novel_Symbols = TRUE;
	else
	    PT_Novel_Symbols = FALSE;

	/* next update the probability tables */
	if (model_form != TLM_Static)
	  {
	    if (!decoded_escape)
	      PTc_update_table (PT_Models [pt_model].PT_table, context_text,
				symbols_text, 1, NULL);
	    else
	      /* Defer all the updates during decoding until the symbols_text
		 has been decoded; do this by storing them during the escaping
		 down from the higher order models */
	      PT_update_decode_context1 (PT_Models [pt_model].PT_table,
					 context_text);
	  }
      }
}

void
PT_find_symbol (unsigned int model, unsigned int context_text,
		unsigned int symbols_text)
/* Finds the codelength for encoding the symbols_text in the
   PT context record. symbols_text is assumed to be a text record
   i.e. a sequence of text symbols.
   Returns additional information as specified by the routine
   TLM_set_context_operation. */
{
    PT_encode_position (model, context_text, symbols_text,
			FIND_CODELENGTH_TYPE, NO_CODER);
}

void
PT_update_context (unsigned int model, unsigned int context_text,
		   unsigned int symbols_text)
/* Updates the PT context record for the current symbols_text.
   symbols_text is assumed to be a text record i.e. a sequence of text symbols.
   Returns additional information as specified by the routine
   TLM_set_context_operation. */
{
    PT_encode_position (model, context_text, symbols_text,
			UPDATE_TYPE, NO_CODER);
}

void
PT_update_decode_context (unsigned int model, unsigned int symbols_text)
/* Performs all the updates that were deferred during decoding
   that could not be performed until the symbols_text has been
   decoded. */
{
    unsigned int model_form, p;

    assert (TXT_valid_text (symbols_text));

    model_form = Models [model].M_model_form;
    if (model_form != TLM_Static)
      {
	for (p = 0; p < PT_Decode_Contexts_Count; p++)
	  PTc_update_table
	    (PT_Decode_Contexts [p].PT_table, PT_Decode_Contexts [p].PT_context_text,
	     symbols_text, 1, NULL);
      }

    PT_Decode_Contexts_Count = 0;
}

void
PT_encode_symbol (unsigned int model, unsigned int context_text,
		  unsigned int coder, unsigned int symbols_text)
/* Encodes & updates the PT context record for the current symbols_text.
   symbols_text is assumed to be a text record i.e. a sequence of text symbols. */
{
    PT_encode_position (model, context_text, symbols_text,
			ENCODE_TYPE, coder);
}

unsigned int
PT_decode_symbol (unsigned int model, unsigned int context_text,
		  unsigned int coder)
/* Decodes & updates the PT context record for the current symbols_text.
   symbols_text is assumed to be a text record i.e. a sequence of text symbols. */
{
    unsigned int symbols_text;

    symbols_text = TXT_create_text ();
    PT_encode_position (model, context_text, symbols_text,
			DECODE_TYPE, coder);
    return (symbols_text);
}
