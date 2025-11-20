/* TLM routines based on CPT models. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "io.h"
#include "sets.h"
#include "text.h"
#include "model.h"
#include "cpt_model.h"

#define CPT_MODELS_SIZE 4          /* Initial max. number of models */

/* Global variables used for storing the CPT models */
struct CPT_modelType *CPT_Models = NULL; /* List of CPT models */
unsigned int CPT_Models_max_size = 0;     /* Current max. size of models array */
unsigned int CPT_Models_used = NIL;       /* List of deleted model records */
unsigned int CPT_Models_unused = 1;       /* Next unused model record */

boolean
CPT_valid_model (unsigned int model)
/* Returns non-zero if the CPT model is valid, zero otherwize. */
{
    if (model == NIL)
        return (FALSE);
    else if (model >= CPT_Models_unused)
        return (FALSE);
    else if (CPT_Models [model].CPT_deleted)
        /* This gets set to TRUE when the model gets deleted */
        return (FALSE);
    else
        return (TRUE);
}

unsigned int
create_CPT_model (void)
/* Creates and returns a new pointer to a CPT model record. */
{
    unsigned int model, old_size;

    if (CPT_Models_used != NIL)
    { /* use the first record on the used list */
        model = CPT_Models_used;
	CPT_Models_used = CPT_Models [model].CPT_next;
    }
    else
    {
	model = CPT_Models_unused;
        if (CPT_Models_unused >= CPT_Models_max_size)
	{ /* need to extend CPT_Models array */
	    old_size = CPT_Models_max_size * sizeof (struct modelType);
	    if (CPT_Models_max_size == 0)
	        CPT_Models_max_size = CPT_MODELS_SIZE;
	    else
	        CPT_Models_max_size = 10*(CPT_Models_max_size+50)/9;

	    CPT_Models = (struct CPT_modelType *)
	        Realloc (124, CPT_Models, CPT_Models_max_size *
			 sizeof (struct CPT_modelType), old_size);

	    if (CPT_Models == NULL)
	    {
	        fprintf (stderr, "Fatal error: out of CPT models space\n");
		exit (1);
	    }
	}
	CPT_Models_unused++;
    }

    return (model);
}

unsigned int
CPT_create_model (void)
/* Creates and returns a new pointer to a CPT model record.

   The model_type argument specified the type of model to be created e.g.
   TLM_CPT_Model or TLM_PCFG_Model. It is followed by a variable number of
   parameters used to hold model information which differs between
   implementations of the different model types. For example, the
   TLM_CPT_Model implementation uses it to specify
   the maximum order of the CPT model, and whether the model should perform
   update exclusions.

   The alphabet_size argument specifies the number of symbols permitted in
   the alphabet (all symbols for this model must have values from 0 to one
   less than alphabet_size).
*/
{
    unsigned int model;

    model = create_CPT_model ();

    CPT_Models [model].CPT_ptable = CPTp_create_table ();
    CPT_Models [model].CPT_ctable = CPTc_create_table ();

    CPT_Models [model].CPT_deleted = FALSE;
    CPT_Models [model].CPT_next = NIL;

    return (model);
}

void
CPT_release_model (unsigned int model)
/* Releases the memory allocated to the model and the model number (which may
   be reused in later CPT_create_model or CPT_load_model calls). */
{
    assert (CPT_valid_model (model));

    /* add model record at the head of the CPT_Models_used list */
    CPT_Models [model].CPT_next = CPT_Models_used;
    CPT_Models_used = model;

    CPT_Models [model].CPT_deleted = TRUE; /* Used for testing if model no. is valid or not */
}

void
CPT_write_model (unsigned int file, unsigned int model,
		  unsigned int model_form)
/* Writes out the CPT model to the file as a static CPT model (note: not fast CPT) 
   (which can then be loaded by other applications later). */
{
    unsigned int cpt_model;

    assert (TXT_valid_file (file));
    assert (model_form == TLM_Static);

    cpt_model = TLM_verify_model (model, TLM_CPT_Model, CPT_valid_model);

    /* write out the tables */
    /* fprintf (stderr, "Writing out the (order 0) ptable...\n");*/
    assert (CPT_Models [cpt_model].CPT_ptable != NULL);
    CPTp_write_table (file, CPT_Models [cpt_model].CPT_ptable, TLM_Static);
    /* fprintf (stderr, "Writing out the ctable...\n");*/
    assert (CPT_Models [cpt_model].CPT_ctable != NULL);
    CPTc_write_table (file, CPT_Models [cpt_model].CPT_ctable, TLM_Static);
}

void
CPT_dump_model (unsigned int file, unsigned int pt_model)
/* Dumps out the CPT model (for debugging purposes). */
{
    assert (CPT_valid_model (pt_model));

    fprintf (Files [file], "Dump of ptable:\n");
    CPTp_dump_table (file, CPT_Models [pt_model].CPT_ptable);

    fprintf (Files [file], "\nDump of ctable:\n");
    CPTc_dump_table (file, CPT_Models [pt_model].CPT_ctable);
}

void
CPT_update_context (unsigned int model, unsigned int context,
		  unsigned int symbol_text)
/* Updates the CPT context record so that the current symbol becomes symbol_text.
   Symbol_text is assumed to be a text record i.e. a sequence of text symbols.
   Returns additional information as specified by the routine
   TLM_set_context_operation. */
{
    unsigned int cpt_model;
    unsigned int lbnd, hbnd, totl;

    cpt_model = TLM_verify_model (model, TLM_CPT_Model, CPT_valid_model);

    /* First find out the codelength for the new symbol if required */
    if (TLM_Context_Operation != TLM_Get_Nothing)
      {
	if (context == NIL)
	  CPTp_encode_arith_range (CPT_Models [cpt_model].CPT_ptable, NULL /* No exclusions */,
				  symbol_text, &lbnd, &hbnd, &totl);
	else
	  CPTc_encode_arith_range (CPT_Models [cpt_model].CPT_ctable, NULL /* No exclusions */,
				  context, symbol_text, &lbnd, &hbnd, &totl);
      }
    TLM_Codelength = Codelength (lbnd, hbnd, totl);

    /* next update the probability tables */
    if (context == NIL)
        CPTp_update_table (CPT_Models [cpt_model].CPT_ptable, symbol_text, 1);
    else
        CPTc_update_table (CPT_Models [cpt_model].CPT_ctable, context, symbol_text, 1, NULL);
}
