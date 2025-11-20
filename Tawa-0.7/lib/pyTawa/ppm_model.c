/* TLM routines based on PPM models. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "io.h"
#include "model.h"
#include "ppm_trie.h"
#include "ppm_context.h"
#include "ppm_model.h"

#define PPM_MODELS_SIZE 4          /* Initial max. number of models */

/* Global variables used for storing the PPM models */
struct PPM_modelType *PPM_Models = NULL;/* List of PPM models */
unsigned int PPM_Models_max_size = 0;   /* Current max. size of models array */
unsigned int PPM_Models_used = NIL;     /* List of deleted model records */
unsigned int PPM_Models_unused = 1;     /* Next unused model record */

boolean
PPM_valid_model (unsigned int ppm_model)
/* Returns non-zero if the PPM model is valid, zero otherwize. */
{
    if (ppm_model == NIL)
        return (FALSE);
    else if (ppm_model >= PPM_Models_unused)
        return (FALSE);
    else if (PPM_Models [ppm_model].P_max_order < -1)
        /* The max order gets set to -2 when the model gets deleted;
	   this way you can test to see if the model has been deleted or not */
        return (FALSE);
    else
        return (TRUE);
}

unsigned int
create_PPM_model (void)
/* Creates and returns a new pointer to a PPM model record. */
{
    unsigned int ppm_model, old_size;

    if (PPM_Models_used != NIL)
    { /* use the first record on the used list */
        ppm_model = PPM_Models_used;
	PPM_Models_used = PPM_Models [ppm_model].P_next;
    }
    else
    {
	ppm_model = PPM_Models_unused;
        if (PPM_Models_unused >= PPM_Models_max_size)
	{ /* need to extend PPM_Models array */
	    old_size = PPM_Models_max_size * sizeof (struct modelType);
	    if (PPM_Models_max_size == 0)
	        PPM_Models_max_size = PPM_MODELS_SIZE;
	    else
	        PPM_Models_max_size = 10*(PPM_Models_max_size+50)/9;

	    PPM_Models = (struct PPM_modelType *)
	        Realloc (84, PPM_Models, PPM_Models_max_size *
			 sizeof (struct PPM_modelType), old_size);

	    if (PPM_Models == NULL)
	    {
	        fprintf (stderr, "Fatal error: out of PPM models space\n");
		exit (1);
	    }
	}
	PPM_Models_unused++;
    }

    PPM_Models [ppm_model].P_max_order = -1;
    PPM_Models [ppm_model].P_performs_full_excls = TRUE;
    PPM_Models [ppm_model].P_performs_update_excls = TRUE;
    PPM_Models [ppm_model].P_escape_method = TLM_PPM_Method_D;
    PPM_Models [ppm_model].P_trie = NULL;
    PPM_Models [ppm_model].P_ptable = NULL;
    PPM_Models [ppm_model].P_next = NIL;

    return (ppm_model);
}

unsigned int
PPM_create_model (unsigned int alphabet_size, int max_order,
		  unsigned int escape_method, boolean performs_full_excls,
		  boolean performs_update_excls)
/* Creates and returns a new pointer to a PPM model record.

   The model_type argument specified the type of model to be created e.g.
   TLM_PPM_Model or TLM_PCFG_Model. It is followed by a variable number of
   parameters used to hold model information which differs between
   implementations of the different model types. For example, the
   TLM_PPM_Model implementation uses it to specify
   the maximum order of the PPM model, the escape method (A, B, C or D -
   as specified by the constants TLM_PPM_Method_A etc.) and whether the model
   should perform update exclusions.

   The alphabet_size argument specifies the number of symbols permitted in
   the alphabet (all symbols for this model must have values from 0 to one
   less than alphabet_size). An alphabet_size of 0 specifies that the
   alphabet is unbounded. (This is useful for word-based alphabets, for
   example). In this case, allowable symbols ranging from 0 up to a
   special "expand_alphabet" symbol which is equal to the current maximum
   symbol number (this is one more than the highest previously seen symbol
   number). If the current symbol becomes the expand_alphabet symbol, then
   the current maximum symbol number is incremented by 1, thus effectively
   expanding the size of the alphabet by 1. The current maximum symbol number
   may be obtained by calling the routine TLM_get_model. One further
   symbol is permitted in the current alphabet - the sentinel symbol.
*/

{
    unsigned int ppm_model;

    ppm_model = create_PPM_model ();

    PPM_Models [ppm_model].P_alphabet_size = alphabet_size;
    if (alphabet_size == 0)
        PPM_Models [ppm_model].P_max_symbol = 0;
    else
        PPM_Models [ppm_model].P_max_symbol = alphabet_size - 1;

    PPM_Models [ppm_model].P_max_order = max_order;
    PPM_Models [ppm_model].P_escape_method = escape_method;
    PPM_Models [ppm_model].P_performs_full_excls = performs_full_excls;
    PPM_Models [ppm_model].P_performs_update_excls = performs_update_excls;

    assert (max_order >= -1);
    if ((max_order < 0) || ((alphabet_size == 0) && (max_order == 0)))
        /* The (order -1) or (order 0, unbounded alphabet)
	   models never need to perform any exclusions */
      {
        performs_full_excls = FALSE;
        performs_update_excls = FALSE;
      }

    assert ((TLM_PPM_Method_A <= escape_method) && (escape_method <= TLM_PPM_Method_D));
    if (escape_method == TLM_PPM_Method_B)
      {
	fprintf (stderr, "PPM escape method %d is not yet implemented\n", escape_method);
	exit (1);
      }

    if (max_order < 0)
        PPM_Models [ppm_model].P_trie = NULL;
    else
        PPM_Models [ppm_model].P_trie = PPM_create_trie (TLM_Dynamic);

    if (alphabet_size != 0)
	PPM_Models [ppm_model].P_ptable = NULL;
    else /* unbounded alphabet size */
	PPM_Models [ppm_model].P_ptable = ptable_create_table (0);

    return (ppm_model);
}

void
PPM_get_model (unsigned int model, unsigned int type, unsigned int *value)
/* Returns information about the PPM model. The arguments type is
   the information to be returned (i.e. PPM_Get_Alphabet_Size,
   PPM_Get_Max_Symbol, PPM_Get_Max_Order, PPM_Get_Escape_Method,
   PPM_Get_Performs_Full_Excls or PPM_Get_Performs_Update_Excls). */
{
    unsigned int ppm_model;

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    switch (type)
      {
      case PPM_Get_Alphabet_Size:
	*value = PPM_Models [ppm_model].P_alphabet_size;
	break;
      case PPM_Get_Max_Symbol:
	*value = PPM_Models [ppm_model].P_max_symbol;
	break;
      case PPM_Get_Max_Order:
	*value = (unsigned int) PPM_Models [ppm_model].P_max_order;
	break;
      case PPM_Get_Escape_Method:
	*value = PPM_Models [ppm_model].P_escape_method;
	break;
      case PPM_Get_Performs_Full_Excls:
	*value = PPM_Models [ppm_model].P_performs_full_excls;
	break;
      case PPM_Get_Performs_Update_Excls:
	*value = PPM_Models [ppm_model].P_performs_update_excls;
	break;
      default:
	fprintf (stderr, "Invalid type (%d) specified for PPM_get_model ()\n", type);
	exit (1);
	break;
      }
}

void
PPM_set_model (unsigned int model, unsigned int type, unsigned int value)
/* Sets information about the PPM model. The argument type is the information
   to be set (i.e. PPM_Set_Alphabet_Size or PPM_Set_Max_Symbol)
   and its value is set to the argument value.

   The type PPM_Alphabet_Size is used for specifying the size of the
   expanded alphabet (which must be equal to 0 - indicating an unbounded
   alphabet, or greater than the existing size to accomodate symbols used by
   the existing model). */
{
    unsigned int ppm_model;
    unsigned int alphabet_size, max_symbol;

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    switch (type)
      {
      case PPM_Set_Alphabet_Size:
	alphabet_size = value;
	assert ((alphabet_size == 0) ||
		(alphabet_size > PPM_Models [ppm_model].P_alphabet_size));
	PPM_Models [ppm_model].P_alphabet_size = alphabet_size;
	break;
      case PPM_Set_Max_Symbol:
	max_symbol = value;
	alphabet_size = PPM_Models [ppm_model].P_alphabet_size;
	assert ((alphabet_size == 0) ||
		(alphabet_size > PPM_Models [ppm_model].P_alphabet_size));
	if (max_symbol > alphabet_size)
	    max_symbol--;
	PPM_Models [ppm_model].P_max_symbol = max_symbol;
	break;
      default:
	fprintf (stderr, "Invalid type (%d) specified for PPM_set_model ()\n", type);
	exit (1);
	break;
      }
}

void
PPM_release_model (unsigned int ppm_model)
/* Releases the memory allocated to the model and the model number (which may
   be reused in later PPM_create_model or PPM_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active PPM_Contexts pointing at it. */
{
    assert (PPM_valid_model (ppm_model));

    /* Release the model's trie as well */
    PPM_release_trie (PPM_Models [ppm_model].P_trie);

    /* add model record at the head of the PPM_Models_used list */
    PPM_Models [ppm_model].P_next = PPM_Models_used;
    PPM_Models_used = ppm_model;

    PPM_Models [ppm_model].P_max_order = -2; /* Used for testing if model no.
						is valid or not */
}

unsigned int
PPM_copy_model (unsigned int model)
/* Copies the model. */
{
    unsigned int ppm_model, new_ppm_model;
    unsigned int model_form;

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    model_form = Models [model].M_model_form;

    new_ppm_model =
       PPM_create_model (PPM_Models [ppm_model].P_alphabet_size,
			 PPM_Models [ppm_model].P_max_order,
			 PPM_Models [ppm_model].P_escape_method,
			 PPM_Models [ppm_model].P_performs_full_excls,
			 PPM_Models [ppm_model].P_performs_update_excls);
    PPM_Models [new_ppm_model].P_max_symbol = PPM_Models [ppm_model].P_max_symbol;

    if (model_form == TLM_Static) /* if model is static, just copy pointer to it */
      {
        PPM_Models [new_ppm_model].P_trie = PPM_Models [ppm_model].P_trie;
	PPM_Models [new_ppm_model].P_ptable = PPM_Models [ppm_model].P_ptable;
      }
    else /* otherwise duplicate the whole model in memory */
      {
        PPM_Models [new_ppm_model].P_trie =
	    PPM_copy_trie (PPM_Models [ppm_model].P_trie);
        PPM_Models [new_ppm_model].P_ptable =
	    ptable_copy_table (PPM_Models [ppm_model].P_ptable);
      }

    return (new_ppm_model);
}

void
PPM_nullify_model (unsigned int model)
/* Replaces the model with the null model and releases the memory allocated
   to it. */
{
    unsigned int ppm_model;

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    PPM_release_trie (PPM_Models [ppm_model].P_trie);
    PPM_Models [ppm_model].P_trie = NULL;
}

unsigned int
PPM_load_model (unsigned int file, unsigned int model_form)
/* Loads the PPM model which has been previously saved to the file into memory
   and allocates it a new model number which is returned. */
{
    unsigned int ppm_model, trie_size, input_size, alphabet_size, escape_method, p;
    struct PPM_trieType *trie;
    int max_order;

    ppm_model = create_PPM_model (); /* Get next unused PPM model record */
    assert (ppm_model != NIL);

    /* read in the size of the model's alphabet */
    alphabet_size = fread_int (file, INT_SIZE);
    PPM_Models [ppm_model].P_alphabet_size = alphabet_size;

    /* read in the model's maximum symbol number */
    PPM_Models [ppm_model].P_max_symbol = fread_int (file, INT_SIZE);

    /* read in the order of the model */
    max_order = fread_int (file, INT_SIZE);
    PPM_Models [ppm_model].P_max_order = max_order;

    /* read in the escape method of the model */
    escape_method = fread_int (file, INT_SIZE);
    PPM_Models [ppm_model].P_escape_method = escape_method;

    /* read in whether the model performs full exclusions or not */
    PPM_Models [ppm_model].P_performs_full_excls = fread_int (file, INT_SIZE);

    /* read in whether the model performs update exclusions or not */
    PPM_Models [ppm_model].P_performs_update_excls = fread_int (file, INT_SIZE);

    if (max_order < 0)
      { /* order -1 */
	PPM_Models [ppm_model].P_trie = NULL;
      }
    else
      { /* read in the model trie */
	/*fprintf( stderr, "Loading trie model...\n" );*/
	trie = PPM_create_trie (model_form);
	PPM_Models [ppm_model].P_trie = trie;
	trie_size = fread_int (file, INT_SIZE);
	trie->T_size = trie_size;
	trie->T_nodes = (int *) Malloc (2, (trie_size+1) * sizeof(int));
	p = 0;
	for (p = 0; p < trie_size; p++)
	  {
	    trie->T_nodes [p] = fread_int (file, INT_SIZE);
	    /*fprintf (stderr, "Reading in %d\n", trie->T_nodes [p]);*/
	  }

	/* PPM_dump_trie (Stderr_File, trie, max_order, NULL); */

	if (model_form == TLM_Dynamic)
	  { /* read in the dynamic part, if one exists */
	    trie->T_unused = trie->T_size;

	    /*fprintf( stderr, "Loading input list...\n" );*/
	    input_size = fread_int (file, INT_SIZE);	
	    trie->T_input_size = input_size;
	    trie->T_input_len = fread_int (file, INT_SIZE);
	    trie->T_input = (unsigned int *) Malloc (92, (input_size+1) * sizeof(int));
	    for (p = 0; p < input_size; p++)
	      trie->T_input [p] = fread_int (file, INT_SIZE);
	  }

	if (alphabet_size == 0)
	  { /* read in the ptable for word_based alphabets */
	    /*fprintf( stderr, "Loading probability table...\n" );*/
	    PPM_Models [ppm_model].P_ptable = ptable_load_table (file);
	    /*ptable_dump_symbols (stderr, PPM_Models [ppm_model].P_ptable);*/
	  }
      }

    /* fprintf (stderr, "Trie model loaded.\n");*/

    return (ppm_model);
}

void
PPM_write_model (unsigned int file, unsigned int model,
		 unsigned int model_form)
/* Writes out the PPM model to the file (which can then be loaded
   by other applications later). The argument model_form must have the value
   TLM_Static or TLM_Dynamic and determines whether the model is static or
   dynamic when it is later reloaded using TLM_load_model. */
{
    unsigned int ppm_model, model_size, alphabet_size;
    unsigned int online_model_form, escape_method;
    unsigned int input_size, dynamic_to_static, p;
    struct PPM_trieType *trie;
    int max_order;

    assert (TXT_valid_file (file));

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    online_model_form = Models [model].M_model_form;

    trie = PPM_Models [ppm_model].P_trie;
        
    if ((model_form == TLM_Dynamic) && (online_model_form == TLM_Static))
      {
	fprintf (stderr, "Fatal error: This implementation of TLM_write_model () cannot write out\n");
	fprintf (stderr, "a dynamic model when a static model has been loaded\n");
	exit (1);
      }

    /* write out the size of the model's alphabet */
    alphabet_size = PPM_Models [ppm_model].P_alphabet_size;
    fwrite_int (file, alphabet_size, INT_SIZE);

    /* write out the model's maximum symbol number */
    fwrite_int (file, PPM_Models [ppm_model].P_max_symbol, INT_SIZE);

    /* write out the order of the model */
    max_order = PPM_Models [ppm_model].P_max_order;
    /* fprintf (stderr, "\nMax order of model = %d\n", max_order); */
    fwrite_int (file, max_order, INT_SIZE);

    /* write out the escape method of the model */
    escape_method = PPM_Models [ppm_model].P_escape_method;
    /* fprintf (stderr, "\nEscape method of the model = %d\n", escape_method); */
    fwrite_int (file, escape_method, INT_SIZE);

    /* write out whether the model performs exclusions or not */
    fwrite_int (file, PPM_Models [ppm_model].P_performs_full_excls, INT_SIZE);

    /* write out whether the model performs full exclusions or not */
    fwrite_int (file, PPM_Models [ppm_model].P_performs_update_excls, INT_SIZE);

    if (max_order < 0)
        return; /* order -1 model - nothing more to do */

    dynamic_to_static = ((model_form == TLM_Static) && (online_model_form == TLM_Dynamic));
    if (dynamic_to_static)
      { /* Writes out the dynamic trie to a static strie, compressing it
	   at the same time. (Which is one of the main reasons you want to do this
	   in the first place!). */
	trie = PPM_build_static_trie (trie, max_order);
	/*fprintf (stderr, "Writing out the model in static form\n");*/

	/* (replaces trie pointer with pointer to static version of it) */
      }
    if (online_model_form == TLM_Static)
        model_size = trie->T_size;
    else
        model_size = trie->T_unused;

    /* write out the model trie */
    /* fprintf (stderr, "Writing out the trie model...\n");*/
    /*fprintf (stderr, "Size of trie model = %d\n", model_size);*/
    fwrite_int (file, model_size, INT_SIZE);

    if ((model_form == TLM_Dynamic) && (online_model_form == TLM_Dynamic))
      { /* compress the dynamic trie */
	PPM_build_compressed_input (trie, max_order); /* compress input str. */
      }

    /* copy out the trie as it is */
    p = 0;
    for (p = 0; p < model_size; p++)
      {
	fwrite_int (file, (unsigned int) trie->T_nodes [p], INT_SIZE);
	/*fprintf (stderr, "Writing out %d\n", (unsigned int) trie->T_nodes [p]);*/
      }

    if ((model_form == TLM_Dynamic) && (online_model_form == TLM_Dynamic))
      { /* write out the dynamic part, if one exists */

	fprintf (stderr, "Writing out the input sequence...\n");
	input_size = trie->T_input_size;
	fwrite_int (file, input_size, INT_SIZE);
	fwrite_int (file, trie->T_input_len, INT_SIZE);
	p = 0;
	for (p = 0; p < input_size; p++)
	    fwrite_int (file, trie->T_input [p], INT_SIZE);
      }
    if (dynamic_to_static)
        PPM_release_trie (trie);

    if (alphabet_size == 0)
      { /* write out the ptable for word_based alphabets */
	/* fprintf (stderr, "Writing probability table...\n");*/
	ptable_write_table (file, PPM_Models [ppm_model].P_ptable);
	/*ptable_dump_symbols (stderr, PPM_Models [ppm_model].P_ptable);*/
      }

}

void
PPM_dump_model (unsigned int file, unsigned int ppm_model, boolean dumptrie,
		void (*dump_symbol_function) (unsigned int, unsigned int))
/* Dumps out the PPM model (for debugging purposes). */
{
    struct PPM_trieType *trie;

    assert (PPM_valid_model (ppm_model));
    trie = PPM_Models [ppm_model].P_trie;

    fprintf (Files [file], "Size of alphabet = %d\n\n",
	     PPM_Models [ppm_model].P_alphabet_size);
    fprintf (Files [file], "Maximum (non-static) symbol number = %d\n\n",
	     PPM_Models [ppm_model].P_max_symbol);
    fprintf (Files [file], "Max order of model = %d\n",
	     PPM_Models [ppm_model].P_max_order);

    fprintf (Files [file], "Escape method = method ");
    switch (PPM_Models [ppm_model].P_escape_method)
      {
      case TLM_PPM_Method_A:
	fprintf (Files [file], "A\n");
	break;
      case TLM_PPM_Method_B:
	fprintf (Files [file], "B\n");
	break;
      case TLM_PPM_Method_C:
	fprintf (Files [file], "C\n");
	break;
      case TLM_PPM_Method_D:
	fprintf (Files [file], "D\n");
	break;
      default:
	fprintf (Files [file], "*** invalid method ***\n");
	break;
      }

    fprintf (Files [file], "Perform full exclusions = %d\n",
	     PPM_Models [ppm_model].P_performs_full_excls);

    fprintf (Files [file], "Perform update exclusions = %d\n",
	     PPM_Models [ppm_model].P_performs_update_excls);

    fprintf (Files [file], "Size of trie = %d\n\n", trie->T_size);

    if (dumptrie)
      {
	PPM_dump_trie (file, trie, PPM_Models [ppm_model].P_max_order,
		       dump_symbol_function);
	PPM_dump_input (file, trie, dump_symbol_function);
      }
    ptable_dump_symbols (file, PPM_Models [ppm_model].P_ptable);
}

unsigned int
PPM_create_context (unsigned int model)
/* Creates and returns an unsigned integer which provides a reference to a PPM
   context record associated with the model's context. The current position is
   set to the null string. The current symbol is set to the first predicted
   symbol. A run-time error occurs if the PPM context being copied is for a
   dynamic model. 
*/
{
    unsigned int model_form;
    unsigned int ppm_model;
    unsigned int context;
    int max_order;

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    model_form = Models [model].M_model_form;
    /*
    if ((model_form == TLM_Dynamic) && (Models [model].M_contexts > 0))
      {
        fprintf (stderr, "Fatal error: a dynamic model is allowed to have only one active context\n");
        exit (1);
      }
    */

    max_order = PPM_get_max_order (model);

    Models [model].M_contexts++;

    context = PPM_create_context1 ();
    assert (context != NIL);

    PPM_Contexts [context].C_position = NULL;

    /* start the context off at the root: */
    if (max_order < 0)
        PPM_Contexts [context].C_node = NIL;
    else
        PPM_Contexts [context].C_node = TRIE_ROOT_NODE;

    PPM_create_suffixlist (model, context);
    PPM_init_suffixlist (model, context);
    PPM_start_suffix (model, context); /* Starts the suffix list at ROOT */

    return (context);
}

unsigned int
PPM_copy_context (unsigned int model, unsigned int context)
/* Creates a new PPM context record, copies the contents of the specified
   contex into it, and returns an integer reference to it. A run-time error
   occurs if the PPM context being copied is for a dynamic model. */
{
    unsigned int model_form, new_context, ppm_model;

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    /*fprintf (stderr, "Copy context: model %d context %d\n", model, context);*/
    assert (PPM_valid_context (context));

    new_context = PPM_create_context1 ();
    PPM_create_suffixlist (model, new_context);

    model_form = Models [model].M_model_form;
    /*
    if ((model_form == TLM_Dynamic) && (Models [model].M_contexts > 0))
      {
	fprintf (stderr, "Fatal error: a dynamic model is allowed to have only one active context\n");
	exit (1);
      }
    */

    PPM_copy_context1 (model, context, new_context);
    return (new_context);
}

unsigned int
PPM_clone_context (unsigned int model, unsigned int context)
/* Creates a new PPM context record, copies the contents of the specified
   context into it, and returns an integer reference to it. Note: A run-time error
   does not occur if the PPM context being copied is for a dynamic model. */
{
    unsigned int ppm_model;
    unsigned int new_context;

    assert (TLM_valid_model (model));

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    assert (PPM_valid_context (context));

    new_context = PPM_create_context1 ();
    PPM_create_suffixlist (model, new_context);

    PPM_copy_context1 (model, context, new_context);
    return (new_context);
}

void
PPM_overlay_context (unsigned int model, unsigned int old_context,
		     unsigned int context)
/* Overlays the PPM context by copying the old context into it. This will
   copy the context even for dynamic models. This is necessary when dynamic
   models with supplementary symbols are being created. */
{
    unsigned int ppm_model;

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    assert (PPM_valid_context (context));
    assert (PPM_valid_context (old_context));

    PPM_copy_context1 (model, old_context, context);
}

void
PPM_find_symbol (unsigned int model, unsigned int context,
		 unsigned int symbol)
/* Finds the predicted symbol in the PPM context. */
{
    unsigned int ppm_model;
    struct PPM_positionType *position;
    codingType coding_type;

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    assert (PPM_valid_context (context));

    switch (TLM_Context_Operation)
      {
      case TLM_Get_Codelength:
      default:
	coding_type = FIND_CODELENGTH_TYPE;
	break;
      case TLM_Get_Maxorder:
	coding_type = FIND_MAXORDER_TYPE;
	break;
      case TLM_Get_Coderanges:
	coding_type = FIND_CODERANGES_TYPE;
	break;
      }

    PPM_validate_symbol (context, symbol, &PPM_Context_Position);

    position =
        PPM_start_position (model, context, FIND_SYMBOL_TYPE, coding_type,
			    NO_CODER, &PPM_Context_Position);
    PPM_find_position (model, context, FIND_SYMBOL_TYPE, coding_type, NO_CODER,
		       position);

    switch (TLM_Context_Operation)
      {
      case TLM_Get_Codelength:
      case TLM_Get_Maxorder:
      default:
	TLM_Codelength = position->P_codelength;
	break;
      case TLM_Get_Coderanges:
	TLM_Coderanges = TLM_copy_coderanges (position->P_coderanges);
	break;
      }
}

void
PPM_update_context (unsigned int model, unsigned int context,
		    unsigned int symbol)
/* Updates the PPM context record so that the current symbol becomes symbol.
   Returns additional information as specified by the routine
   TLM_set_context_operation. */
{
    struct PPM_positionType *position;
    codingType coding_type;
    unsigned int ppm_model;

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    assert (PPM_valid_context (context));

    switch (TLM_Context_Operation)
      {
      case TLM_Get_Codelength:
	coding_type = UPDATE_CODELENGTH_TYPE;
	break;
      case TLM_Get_Maxorder:
	coding_type = UPDATE_MAXORDER_TYPE;
	break;
      case TLM_Get_Coderanges:
	coding_type = UPDATE_CODERANGES_TYPE;
	break;
      default:
	coding_type = UPDATE_TYPE;
	break;
      }

    PPM_validate_symbol (context, symbol, &PPM_Context_Position);

    position =
        PPM_start_position (model, context, FIND_SYMBOL_TYPE, coding_type,
			    NO_CODER, &PPM_Context_Position);
    PPM_find_position (model, context, FIND_SYMBOL_TYPE, coding_type, NO_CODER,
		       position);

    switch (TLM_Context_Operation)
      {
      case TLM_Get_Codelength:
      case TLM_Get_Maxorder:
	TLM_Codelength = position->P_codelength;
	break;
      case TLM_Get_Coderanges:
	TLM_Coderanges = TLM_copy_coderanges (position->P_coderanges);
	break;
      default:
	break;
      }
}

void
PPM_release_context (unsigned int model, unsigned int context)
/* Releases the memory allocated to the PPM context and the context number
   (which may be reused in later PPM_create_context or PPM_copy_context and
   TLM_copy_dynamic_context calls). */
{
    unsigned int ppm_model, model_form;

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    assert (PPM_valid_context (context));

    /* Decrement contexts count only for static models; if a dynamic model,
       use the count to ensure only one context ever gets created */
    model_form = Models [model].M_model_form;
    if ((Models [model].M_model_form != TLM_Dynamic) &&
	(Models [model].M_contexts > 0))
        Models [model].M_contexts--;
 
    PPM_release_suffixlist (model, context);

    PPM_release_position (PPM_Contexts [context].C_position);

    /* Mark as invalid record by having non-zero suffix ptr and NULL suffix list */
    PPM_Contexts [context].C_suffixptr = 1;
    PPM_Contexts [context].C_suffixlist = NIL;

    /* Append onto head of the used list */
    PPM_Contexts [context].C_next = PPM_Contexts_used;
    PPM_Contexts_used = context;

    /*fprintf (stderr, "Releasing context %d model %d\n", context, model);*/
}

void
PPM_reset_symbol (unsigned int model, unsigned int context)
/* Resets the PPM context record to point at the first predicted symbol of the
   current position. */
{
    unsigned int ppm_model;

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    assert (PPM_valid_context (context));

    if (PPM_Contexts [context].C_position != NULL)
        PPM_Contexts [context].C_position = PPM_start_position
	  (model, context, NEXT_SYMBOL_TYPE, FIND_CODELENGTH_TYPE, NO_CODER,
	   PPM_Contexts [context].C_position);

    PPM_reset_suffixlist (model, context); /* Start at the head of the suffix list */
}

boolean
PPM_next_symbol (unsigned int model, unsigned int context,
		 unsigned int *symbol)
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
{
    struct PPM_positionType *position;
    codingType coding_type;
    unsigned int ppm_model;
    unsigned int found;

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    assert (PPM_valid_context (context));

    coding_type = FIND_CODELENGTH_TYPE;
    switch (TLM_Context_Operation)
      {
      case TLM_Get_Maxorder:
	coding_type = FIND_MAXORDER_TYPE;
	break;
      case TLM_Get_Coderanges:
	coding_type = FIND_CODERANGES_TYPE;
	break;
      }

    position = PPM_Contexts [context].C_position;
    if (position == NULL)
        position = PPM_start_position (model, context, NEXT_SYMBOL_TYPE,
            coding_type, NO_CODER, NULL /* null position */ );

    found = PPM_find_position (model, context, NEXT_SYMBOL_TYPE,
			  coding_type, NO_CODER, position);

    *symbol = position->P_symbol;
    PPM_Contexts [context].C_position = position;

    switch (TLM_Context_Operation)
      {
      case TLM_Get_Codelength:
      case TLM_Get_Maxorder:
      default:
	TLM_Codelength = position->P_codelength;
	break;
      case TLM_Get_Coderanges:
	TLM_Coderanges = TLM_copy_coderanges (position->P_coderanges);
	break;
      }

    return (found);
}

void
PPM_encode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder, unsigned int symbol)
/* Encodes the specified symbol using the arithmetic coder.
   Updates the context record so that the last symbol in the
   PPM context becomes the encoded symbol. */
{
    struct PPM_positionType *position;
    unsigned int ppm_model;

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    assert (PPM_valid_context (context));

    assert (coder = TLM_valid_coder (coder));

    PPM_validate_symbol (context, symbol, &PPM_Context_Position);

    position =
        PPM_start_position  (model, context, FIND_SYMBOL_TYPE, ENCODE_TYPE,
			     coder, &PPM_Context_Position); 
    PPM_find_position (model, context, FIND_SYMBOL_TYPE, ENCODE_TYPE, coder,
		       position);
}

unsigned int
PPM_decode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder)
/* Returns the symbol decoded using the arithmetic coder. Updates the
   PPM context record so that the last symbol in the context becomes the
   decoded symbol. */
{
    struct PPM_positionType *position;
    unsigned int ppm_model;

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    assert (PPM_valid_context (context));

    assert (coder = TLM_valid_coder (coder));

    position =
        PPM_start_position (model, context, FIND_TARGET_TYPE, DECODE_TYPE,
			    coder, &PPM_Context_Position);
    PPM_find_position (model, context, FIND_TARGET_TYPE, DECODE_TYPE, coder,
		       position);

    return (position->P_symbol);
}

unsigned int
PPM_getcontext_position (unsigned int model, unsigned int context)
/* Returns an integer which uniquely identifies the current position
   associated with the PPM context. (One implementation is to return a memory
   location corresponding to the current position. This routine is useful if
   you need to check whether different contexts have encoded the same prior
   symbols as when checking whether the context pathways converge in the
   Viterbi or trellis-based algorithms.) */
{
    unsigned int ppm_model;

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    return ((unsigned int) PPM_Contexts [context].C_node);
}

unsigned int
PPM_minlength_model (unsigned int model)
/* Returns the minimum number of bits needed to write the PPM model
   out to disk as a static model and recover it later. This is
   useful for computing minimum description lengths of messages.

   Note that the amount returned will always be slightly less than
   the resulting size of the static model produced by TLM_write_model
   as this excludes extra overhead data (including the model's title)
   that is necessary for the functioning of the API. */
{
    unsigned int ppm_model, size;
    struct PPM_trieType *trie;
    int max_order;

    assert (TLM_valid_model (model));

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    trie = PPM_Models [ppm_model].P_trie;

    if (trie == NULL) /* order -1 */
        size = 0;
    else if (trie->T_form == TLM_Static)
        size = trie->T_size * sizeof (int) * 8;
    else
      {
	max_order = PPM_Models [ppm_model].P_max_order;
	trie = PPM_build_static_trie (trie, max_order);
	/* (replaces trie pointer with pointer to static version of it) */
	if (trie == NULL)
	    size = 0;
	else
	    size = trie->T_unused * sizeof (int) * 8;
        PPM_release_trie (trie);
      }
    return (size);
}

unsigned int
PPM_sizeof_model (unsigned int model)
/* Returns the current number of bits needed to store the
   model in memory. */
{
    unsigned int ppm_model;
    struct PPM_trieType *trie;

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    trie = PPM_Models [ppm_model].P_trie;

    if (trie == NULL) /* order -1 */
        return (0);
    else if (trie->T_form == TLM_Static)
        return (trie->T_size * sizeof (int));
    else
        return ((trie->T_unused + trie->T_input_len) *sizeof (int));
}

void
PPM_stats_model (unsigned int file, unsigned int model)
/* Writes out statistics about the PPM model in human readable form. */
{
    unsigned int ppm_model;

    assert (TXT_valid_file (file));
    assert (TLM_valid_model (model));

    ppm_model = TLM_verify_model (model, TLM_PPM_Model, PPM_valid_model);

    PPM_stats_trie (file, PPM_Models [ppm_model].P_trie,
		    PPM_Models [ppm_model].P_max_order);
}
