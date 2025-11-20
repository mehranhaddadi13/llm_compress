/* Transforms the input file using multiple PPM models. This is the prior
   step before the encoding step. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "paths.h"
#include "model.h"
#include "ppm_model.h"
#include "transform.h"

#define DYNAMIC_TITLE "Dynamic model"
#define PERFORMS_EXCLS TRUE
#define MAX_ORDER 5       /* Maximum order of the model */

#define ALPHABET_SIZE 256 /* Default max. alphabet size for all PPM models */

unsigned int Viterbi = FALSE;
unsigned int Dynamic = FALSE;
unsigned int Dynamics = 0;
unsigned int Stack_depth = 0;
unsigned int Stack_path_extension = 0;

unsigned int Alphabet_size = ALPHABET_SIZE;
unsigned int Max_order = MAX_ORDER;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: play [options] training-model <input-text\n"
	     "\n"
	     "options:\n"
	     "  -V\tUse Viterbi algorithm\n"
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

    while ((opt = getopt (argc, argv, "1:2:L:VDM:a:o:d:l:m:p:r")) != -1)
	switch (opt)
	{
	case '1':
	    Stack_depth = atoi (optarg);
	    break;
	case '2':
	    Stack_path_extension = atoi (optarg);
	    break;
	case 'V':
	    Viterbi = TRUE;
	    break;
	case 'D':
	    Dynamic = TRUE;
	    break;
	case 'M':
	    Dynamics = atoi (optarg);
	    break;
	case 'a':
	    Alphabet_size = atoi (optarg);
	    break;
	case 'o':
	    Max_order = atoi (optarg);
	    break;
	case 'p':
	    Debug.progress = atoi (optarg);
	    break;
	case 'l':
	    Debug.level = atoi (optarg);
	    break;
	case 'L':
	    Debug.level1 = atoi (optarg);
	    break;
	case 'd':
	    Debug.level1 = atoi (optarg);
	    break;
	case 'r':
	    Debug.range = TRUE;
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
dump_transform_text (FILE *fp, unsigned int text)
/* Dumps out the marked up text. */
{
    unsigned int textlen, symbol, sentinel_symbol, prevsymbol, model, p;

    assert (TXT_valid_text (text));

    sentinel_symbol = TXT_sentinel_symbol ();
    prevsymbol = sentinel_symbol;
    model = NIL;
    textlen = TXT_length_text (text);

    for (p=0; p < textlen; p++)
    {
        TXT_get_symbol (text, p, &symbol);
	if (symbol == sentinel_symbol)
	  {
	    if (model)
	        fprintf (fp, "</%s>", TLM_get_tag (model));
	  }
	else if (prevsymbol == sentinel_symbol)
	  {
	    model = symbol;
	    fprintf (fp, "<%s>", TLM_get_tag (model));
	  }
	else if (symbol == '\n')
	    fprintf (fp, "\n");
	else if (symbol == '\t')
	    fprintf (fp, "\t");
	else if ((symbol <= 31) || (symbol >= 127))
	    fprintf (fp, "<%d>", symbol);
	else
	    fprintf (fp, "%c", symbol);
	prevsymbol = symbol;
    }
}

void
transformText (unsigned int transform_model, unsigned int input_text)
/* Transforms the text from the input using the transform model. */
{
    unsigned int transform_text;   /* The marked up text */
    unsigned int model;         /* A language model */

    /*dump_memory (stderr);*/

    TLM_reset_modelno ();
    while ((model = TLM_next_modelno ()))
        TTM_start_transform (transform_model, TTM_transform_single_context,
			  input_text, model);

    transform_text = TTM_perform_transform (transform_model, input_text);

    /*
      printf ("Input  = ");
      TXT_dump_text (stdout, input_text, TXT_dump_symbol);
      printf ("\n");
    */

    if (Debug.level1 > 0)
      {
	/*dump_transform_text (stdout, transform_text);*/
      }	

    TXT_dump_file (Stdout_File, transform_text);

    TXT_release_text (transform_text);
    TXT_release_text (input_text);

    /*dump_memory (stderr);*/
}

int
main (int argc, char *argv[])
{
    unsigned int model_type, model_form;
    unsigned int model, transform_model, dynamic_model;
    unsigned int input_text, alphabet_size, numberof_models;
    int max_order;
    char title [48], *old_title;

    init_arguments (argc, argv);

    if (Dynamics)
      {
	for (max_order=-1; max_order <= (int) Dynamics; max_order++)
	  {
	    sprintf (title, "Dynamic, %2d", max_order);
	    dynamic_model = TLM_create_model
	        (TLM_PPM_Model, DYNAMIC_TITLE, Alphabet_size, max_order,
		 TLM_PPM_Method_D, PERFORMS_EXCLS);
	    TLM_set_tag (dynamic_model, title);
	  }
      }
    else if (Dynamic)
      {
        dynamic_model = TLM_create_model
	    (TLM_PPM_Model, DYNAMIC_TITLE, Alphabet_size, Max_order,
	     TLM_PPM_Method_D, PERFORMS_EXCLS);
	TLM_set_tag (dynamic_model, "Dynamic");
      }

    /* Expand each model's alphabet by adding extra symbols for each model;
       these will be used to signal a switch into a new model */
    numberof_models = TLM_numberof_models ();
    TLM_reset_modelno ();
    while ((model = TLM_next_modelno ()))
      {
	TLM_get_model_type (model, &model_type, &model_form, &old_title);
	TLM_get_model (model, PPM_Get_Alphabet_Size, &alphabet_size);
	if ((alphabet_size > 0) && (model_type == TLM_PPM_Model))
	  {
	    alphabet_size += numberof_models;
	    /*fprintf (stderr, "alphabet_size for model %d = %d\n", model,
	      alphabet_size);*/
	    TLM_set_model (model, PPM_Set_Alphabet_Size, alphabet_size);
	  }
      }

    if (Viterbi)
        transform_model = TTM_create_transform (TTM_Viterbi);
    else
	transform_model = TTM_create_transform (TTM_Stack, TTM_Stack_type0,
					  Stack_depth, Stack_path_extension);

    TTM_add_transform (transform_model, 0.0, "%w", "%w"); /* add transform for every character first */

    TLM_reset_modelno ();
    while ((model = TLM_next_modelno ()))
      TTM_add_transform (transform_model, 0.0, "%w", "%m%w", model); /* add transform for every model */

    /*TTM_dump_transform (stdout, transform_model);*/
    if (Debug.level1 > 1)
        TTM_debug_transform (dumpPathSymbols);

    input_text = TXT_load_text (Stdin_File);
    transformText (transform_model, input_text);

    /*TTM_release_transform (transform_model); - fudge for segmentation fault */
    /*TLM_release_models (); - fudge for segmentation fault */

    /*dump_memory (stderr);*/

    exit (0);
}
