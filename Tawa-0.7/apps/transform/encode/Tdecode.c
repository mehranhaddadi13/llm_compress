/* Decodes the input file using multiple PPM models. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "track.h"
#include "paths.h"
#include "coder.h"
#include "sss_model.h"
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
decode_transform_text (unsigned int coder)
/* Decodes the marked up text. */
{
    unsigned int model, this_model, sss_model, sss_context, pos;
    unsigned int symbol, sentinel_symbol;

    sentinel_symbol = TXT_sentinel_symbol ();

    arith_decode_start (Stdin_File);

    /* Create SSS model for decoding the first model number */
    sss_model = TLM_create_model
        (TLM_SSS_Model, "This model is used for decoding the first model number",
	 0, 2, 32);
    sss_context = TLM_create_context (sss_model);
    model = TLM_decode_symbol (sss_model, sss_context, coder);
    TLM_release_context (sss_model, sss_context);
    TLM_release_model (sss_model);

    pos = 0;
    for (;;)
      { /* decode symbols until sentinel symbol is encountered */
	assert (TLM_valid_model (model));

	/* Copy existing context so that if a "switch" symbol is
	   decoded (it signals a switch to a different model), then
	   the main context stays the way it was before: */
	TLM_overlay_context (model, Contexts [model], Switch_Contexts [model]);

	if (IsDynamic [model])
	    TLM_suspend_update (model);
	symbol = TLM_decode_symbol (model, Switch_Contexts [model], coder);
	if (IsDynamic [model])
	    TLM_resume_update (model);
	if (symbol == sentinel_symbol)
	    break;

	if (symbol >= Alphabet_Sizes [model])
	  /* We have found a switch symbol - extract the new model number
	     from it */
	    model = symbol - Alphabet_Sizes [model] + 1;
	else
	  {
	    if (Debug.range)
	      fprintf (stderr,
		       "Decoded symbol %c (%d) at position %d using model %d\n",
		       symbol, symbol, pos, model);
	    fputc (symbol, stdout);
	    pos++;

	    /* update contexts */
	    TLM_reset_modelno ();
	    while ((this_model = TLM_next_modelno ()))
	      {
		if (Debug.level > 0)
		    fprintf (stderr, "Updating model %d with symbol %d\n",
			     this_model, symbol);
		TLM_update_context (this_model, Contexts [this_model], symbol);
	      }
	    if (Debug.level > 0)
	        TLM_dump_model (Stderr_File, 3, NULL);
	  }
    }

    arith_decode_finish (Stdin_File);
}

int
main (int argc, char *argv[])
{
    unsigned int model_type, model_form, max_symbol;
    unsigned int model, coder, dynamic_model, numberof_models;
    unsigned int alphabet_size;
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
	  }
      }

    coder = TLM_create_arithmetic_coder ();

    decode_transform_text (coder);

    /*TLM_release_models (); - fudge for segmentation fault */

    /*dump_memory (stderr);*/

    exit (0);
}
