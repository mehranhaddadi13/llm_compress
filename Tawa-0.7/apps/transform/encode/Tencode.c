/* Encodes the input file using multiple PPM models. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "paths.h"
#include "coder.h"
#include "model.h"
#include "transform.h"

#define DYNAMIC_TITLE "Dynamic model"
#define PERFORMS_EXCLS TRUE
#define MAX_ORDER 5       /* Maximum order of the model */

#define ALPHABET_SIZE 256 /* Default max. alphabet size for all PPM models */

unsigned int Dynamic = FALSE;
unsigned int Dynamics = 0;

unsigned int Dynamic_alphabet_size = ALPHABET_SIZE;
unsigned int Dynamic_max_order = MAX_ORDER;

unsigned int *Contexts = NULL;
unsigned int *Switch_Contexts = NULL;
unsigned int *Alphabet_Sizes = NULL;
boolean *IsDynamic = NULL;

void
debugEncode ()
{
    fprintf (stderr, "Got here\n");
}

void
usage (void)
{
    fprintf (stderr,
	     "Usage: play [options] training-model <input-text\n"
	     "\n"
	     "options:\n"
	     "  -D\tInclude single dynamic model in transform\n"
	     "  -M n\tInclude multiple dynamic models in transform up to order n\n"
	     "  -l n\tdebug level=n\n"
	     "  -d n\tdebug paths=n\n"
	     "  -p n\tdebug progress=n\n"
	     "  -r\tdebug coding ranges\n"
	     "  -m fn\tmodel filename=fn\n"
	     "  -o n\tmaximum order of dynamic model(s)\n"
	     "  -1 n\tDepth of stack=n\n"
	     "  -2 n\tPath extension=n\n"
	     "(One of -D, -M or -m must be present)\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind; 
    unsigned int models_found = FALSE;
    int opt;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "DM:a:o:d:l:m:p:r")) != -1)
	switch (opt)
	{
	case 'D':
	    Dynamic = TRUE;
	    break;
	case 'M':
	    Dynamics = atoi (optarg);
	    break;
	case 'a':
	    Dynamic_alphabet_size = atoi (optarg);
	    break;
	case 'o':
	    Dynamic_max_order = atoi (optarg);
	    break;
	case 'p':
	    Debug.progress = atoi (optarg);
	    break;
	case 'r':
	    Debug.range = TRUE;
	    break;
	case 'l':
	    Debug.level = atoi (optarg);
	    break;
	case 'd':
	    Debug.level1 = atoi (optarg);
	    break;
	case 'm':
	    TLM_load_models (optarg);
	    models_found = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!models_found && !Dynamics && !Dynamic)
        usage ();
    for (; optind < argc; optind++)
	usage ();
}

void
encode_transform_text (unsigned int text, unsigned int coder)
/* Encodes the marked up text. */
{
    unsigned int model, this_model, new_model, sss_model, sss_context, pos, p;
    unsigned int textlen, symbol, prevsymbol;
    unsigned int sentinel_symbol, switch_symbol;

    assert (TXT_valid_text (text));
    sentinel_symbol = TXT_sentinel_symbol ();

    arith_encode_start (Stdout_File);

    /* Create SSS model for encoding the first model number */
    sss_model = TLM_create_model
        (TLM_SSS_Model, "This model is used for encoding the first model number",
	 0, 2, 32);
    sss_context = TLM_create_context (sss_model);

    prevsymbol = sentinel_symbol;
    model = NIL;
    textlen = TXT_length_text (text);
    if (textlen) /* ignore sentinel symbol at end of file */
        TXT_setlength_text (text, textlen-1);

    pos = 0;
    for (p=0; p < textlen-1; p++)
    {
        TXT_get_symbol (text, p, &symbol);
	if (symbol == sentinel_symbol)
	  {
	    if (!TXT_get_symbol (text, ++p, &symbol))
	        break;
	    assert (symbol == TRANSFORM_MODEL_TYPE);

	    if (!TXT_get_symbol (text, ++p, &new_model))
	        break;

	    if (model == NIL)
	      { /* first time through - encode the first model number directly */
		TLM_encode_symbol (sss_model, sss_context, coder, new_model);
		TLM_release_context (sss_model, sss_context);
		TLM_release_model (sss_model);
	      }
	    else
	      {
		if (Debug.level > 1)
		    fprintf (stderr, "</%s>", TLM_get_tag (model));

		/* Copy existing context, encode a "switch" symbol (to signal a
		   switch to a different model) using it, then reset context to
		   be what it was before: */
		switch_symbol = Alphabet_Sizes [model] + new_model - 1;
		TLM_overlay_context (model, Contexts [model],
				     Switch_Contexts [model]);

		if (IsDynamic [model])
		    TLM_suspend_update (model);
		TLM_encode_symbol (model, Switch_Contexts [model], coder,
				   switch_symbol);
		if (IsDynamic [model])
		    TLM_resume_update (model);
	      }

	    model = new_model;

	    if (Debug.level > 1)
	        fprintf (stderr, "<%s>", TLM_get_tag (model));
	  }
	else
	  {
	    assert (model != NIL);

	    if (Debug.range)
	        fprintf (stderr,
		     "Encoding symbol %c (%d) at position %d using model %d\n",
		     symbol, symbol, pos, model);
	    pos++;

	    /* encode each symbol and update contexts */
	    TLM_reset_modelno ();
	    while ((this_model = TLM_next_modelno ()))
	      {
		if (model == this_model)
		    TLM_encode_symbol (this_model, Contexts [this_model], coder,
				       symbol);
		else
		  {
		    if (Debug.level > 0)
		        fprintf (stderr, "Updating model %d with symbol %d\n",
				 this_model, symbol);
		    TLM_update_context (this_model, Contexts [this_model], symbol);
		  }
	      }
	    if (Debug.level > 0)
	        TLM_dump_model (Stderr_File, 3, NULL);

	    if (Debug.level > 1)
	      {
		if (symbol == '\n')
		  fprintf (stderr, "\n");
		else if (symbol == '\t')
		  fprintf (stderr, "\t");
		else if ((symbol <= 31) || (symbol >= 127))
		  fprintf (stderr, "<%d>", symbol);
		else
		  fprintf (stderr, "%c", symbol);
	      }
	  }
	prevsymbol = symbol;
    }

    assert (model != NIL);
    TLM_encode_symbol (model, Contexts [model], coder, sentinel_symbol);

    arith_encode_finish (Stdout_File);
}

int
main (int argc, char *argv[])
{
    unsigned int model_type, model_form, max_symbol;
    unsigned int model, coder, dynamic_model, numberof_models;
    unsigned int transform_text, alphabet_size;
    boolean performs_excls;
    int max_order;
    char title [48], *old_title;

    init_arguments (argc, argv);

    if (Dynamics)
      {
	for (max_order=-1; max_order <= (int) Dynamics; max_order++)
	  {
	    sprintf (title, "Dynamic, %2d", max_order);
	    dynamic_model = TLM_create_model
	        (TLM_PPM_Model, DYNAMIC_TITLE, Dynamic_alphabet_size, max_order,
		 PERFORMS_EXCLS);
	    TLM_set_tag (dynamic_model, title);
	  }
      }
    else if (Dynamic)
      {
        dynamic_model = TLM_create_model
	    (TLM_PPM_Model, DYNAMIC_TITLE, Dynamic_alphabet_size,
	     Dynamic_max_order, PERFORMS_EXCLS);
	TLM_set_tag (dynamic_model, "Dynamic");
      }

    /* Create contexts and expand each model's alphabet by adding extra symbols
       for each model; these will be used to signal a switch into a new model */
    numberof_models = TLM_numberof_models ();
    Contexts = (unsigned int *)
        malloc ((numberof_models+1) * sizeof (unsigned int));
    Switch_Contexts = (unsigned int *)
        malloc ((numberof_models+1) * sizeof (unsigned int));
    Alphabet_Sizes = (unsigned int *)
        malloc ((numberof_models+1) * sizeof (unsigned int));
    IsDynamic = (boolean *)
        malloc ((numberof_models+1) * sizeof (boolean));

    TLM_reset_modelno ();
    while ((model = TLM_next_modelno ()))
      {
	Contexts [model] = TLM_create_context (model);
	Switch_Contexts [model] = TLM_create_context (model);

	TLM_get_model (model, &model_type, &model_form, &old_title,
		       &alphabet_size, &max_symbol, &max_order, &performs_excls);
	IsDynamic [model] = (model_form == TLM_Dynamic);
	Alphabet_Sizes [model] = alphabet_size;
	if ((alphabet_size > 0) && (model_type == TLM_PPM_Model))
	  {
	    alphabet_size += numberof_models;
	    TLM_set_model (model, alphabet_size, max_symbol);
	    /* This expands the alphabet and sets all the new symbols
	       to be "static" symbols at the same time */
	  }
      }

    transform_text = TXT_load_file (Stdin_File);

    coder = TLM_create_arithmetic_coder ();

    encode_transform_text (transform_text, coder);

    /*TLM_release_models (); - fudge for segmentation fault */

    /*dump_memory (stderr);*/

    exit (0);
}
